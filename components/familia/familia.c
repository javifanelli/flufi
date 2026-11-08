#include "familia.h"
#include "wifi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_random.h"
#include "esp_wifi.h"
#include "lwip/sockets.h"

// La palabra secreta esta en familia_secretos.h, que NO se sube al repo (es
// publico). Si el archivo existe, CMakeLists.txt define FAMILIA_HAY_SECRETOS.
#ifdef FAMILIA_HAY_SECRETOS
#include "familia_secretos.h"
#endif

static const char *TAG = "FAMILIA";

/*
 * Como se hablan (los detalles estan en docs/FAMILIA.md):
 *   - Por la red WiFi de la casa, con mensajitos UDP al "puerto" FAMILIA_PUERTO
 *     (un puerto es como el numero de departamento de una direccion).
 *   - Cada mensaje es una linea de texto:
 *         FAMILIA 2 <clave> <de> <para> <numero> <TIPO> [dato]
 *     por ejemplo:  FAMILIA 2 secreto Flufi Pitri 1234 BESO
 *     "<para>" puede ser "*" (para todos).
 *   - Por WiFi un mensaje se puede perder, asi que cada uno se manda VECES
 *     veces. Cada mensaje tiene su "numero": si llega dos veces el mismo, el
 *     que lo recibe se da cuenta y le hace caso una sola vez.
 *   - Cada tanto cada uno avisa "ESTOY" a todos: asi se sabe quien esta conectado.
 */
#define FAMILIA_PUERTO     47100
#define VERSION            "2"
#define VECES              3              // cuantas veces se manda cada mensaje
#define ENTRE_VECES_MS     40
#define AVISO_CADA_MS      (30 * 1000)    // cada cuanto avisa "ESTOY"
#define CONECTADO_MS       (95 * 1000)    // si no se lo escucha en este tiempo, se fue
#define SALUDO_CADA_MS     (60 * 1000)    // no saludar dos veces seguidas al mismo
#define RECUERDA_NUMEROS   8              // cuantos numeros de mensaje recuerda de cada uno

static char mi_nombre[FAMILIA_MAX_NOMBRE];

// ─── Quienes son y donde estan ───────────────────────────────────────────────

typedef struct {
    char     nombre[FAMILIA_MAX_NOMBRE];
    uint32_t ip;                           // su direccion en la red
    int64_t  visto_us;                     // la ultima vez que se lo escucho
    int64_t  saludado_us;                  // la ultima vez que lo saludamos (0 = nunca)
    uint32_t numeros[RECUERDA_NUMEROS];    // los ultimos mensajes que le hicimos caso
    int      prox_numero;
} miembro_t;

static miembro_t miembros[FAMILIA_MAX_MIEMBROS];
static SemaphoreHandle_t candado;
static QueueHandle_t eventos;

static int64_t ahora_us(void) { return esp_timer_get_time(); }

static bool esta_conectado(const miembro_t *m)
{
    return m->nombre[0] != '\0' && ahora_us() - m->visto_us < (int64_t)CONECTADO_MS * 1000;
}

int familia_conectados(char nombres[][FAMILIA_MAX_NOMBRE], int max)
{
    if (candado == NULL) return 0;
    int n = 0;
    xSemaphoreTake(candado, portMAX_DELAY);
    for (int i = 0; i < FAMILIA_MAX_MIEMBROS && n < max; i++)
        if (esta_conectado(&miembros[i])) snprintf(nombres[n++], FAMILIA_MAX_NOMBRE, "%s", miembros[i].nombre);
    xSemaphoreGive(candado);
    return n;
}

bool familia_alguien_al_azar(char nombre[FAMILIA_MAX_NOMBRE])
{
    char nombres[FAMILIA_MAX_MIEMBROS][FAMILIA_MAX_NOMBRE];
    int n = familia_conectados(nombres, FAMILIA_MAX_MIEMBROS);
    if (n == 0) return false;
    snprintf(nombre, FAMILIA_MAX_NOMBRE, "%s", nombres[esp_random() % n]);
    return true;
}

bool familia_hay_evento(void)
{
    return eventos != NULL && uxQueueMessagesWaiting(eventos) > 0;
}

bool familia_proximo_evento(familia_evento_t *evento)
{
    return eventos != NULL && xQueueReceive(eventos, evento, 0) == pdTRUE;
}

bool familia_configurada(void)
{
#ifdef FAMILIA_HAY_SECRETOS
    return true;
#else
    return false;
#endif
}

#ifdef FAMILIA_HAY_SECRETOS

// Las palabras de cada tipo de mensaje (en el orden de familia_tipo_t)
static const char *const palabras[FAMILIA_CANT_TIPOS] = {
    "HOLA", "SALUDO", "BESO", "CHARLA", "VISITA", "COMIDA", "COSQUILLAS",
    "JUGAR", "TRISTE", "CLIMA", "SENSOR", "MENSAJE", "DORMIR",
};

static int sock = -1;
static uint32_t ultimo_numero;      // el numero del ultimo mensaje que mandamos

// Lo busca (o lo anota si es nuevo). Hay que tener el candado.
static miembro_t *anotar(const char *nombre, uint32_t ip, bool *es_nuevo)
{
    miembro_t *libre = NULL, *mas_viejo = &miembros[0];
    *es_nuevo = false;
    for (int i = 0; i < FAMILIA_MAX_MIEMBROS; i++) {
        miembro_t *m = &miembros[i];
        if (m->nombre[0] != '\0' && strcmp(m->nombre, nombre) == 0) {
            if (!esta_conectado(m)) *es_nuevo = true;    // hacia mucho que no estaba: volvio
            m->ip = ip;
            m->visto_us = ahora_us();
            return m;
        }
        if (m->nombre[0] == '\0' && libre == NULL) libre = m;
        if (m->visto_us < mas_viejo->visto_us) mas_viejo = m;
    }
    miembro_t *m = libre ? libre : mas_viejo;           // si no hay lugar, pisa al que hace mas que no se ve
    memset(m, 0, sizeof(*m));
    snprintf(m->nombre, sizeof(m->nombre), "%s", nombre);
    m->ip = ip;
    m->visto_us = ahora_us();
    *es_nuevo = true;
    ESP_LOGI(TAG, "¡Conoci a %s!", nombre);
    return m;
}

// ─── Mandar ──────────────────────────────────────────────────────────────────

// Manda una linea con la palabra "palabra" (por ejemplo "ESTOY") a "para"
static void mandar_palabra(const char *para, const char *palabra, const char *dato, int veces)
{
    if (sock < 0 || !wifi_conectado()) return;

    xSemaphoreTake(candado, portMAX_DELAY);             // (pueden mandar dos tareas a la vez)
    uint32_t num = ++ultimo_numero;
    xSemaphoreGive(candado);

    char linea[160];
    int largo = snprintf(linea, sizeof(linea), "FAMILIA %s %s %s %s %lu %s%s%s", VERSION, FAMILIA_CLAVE,
                         mi_nombre, para, (unsigned long)num, palabra,
                         (dato && dato[0]) ? " " : "", dato ? dato : "");
    if (largo <= 0 || largo >= (int)sizeof(linea)) return;

    // A uno solo: directo a su direccion, si ya la conocemos. Si no, a todos.
    struct sockaddr_in destino = { .sin_family = AF_INET, .sin_port = htons(FAMILIA_PUERTO) };
    destino.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    if (strcmp(para, "*") != 0) {
        xSemaphoreTake(candado, portMAX_DELAY);
        for (int i = 0; i < FAMILIA_MAX_MIEMBROS; i++)
            if (miembros[i].nombre[0] != '\0' && strcmp(miembros[i].nombre, para) == 0 && miembros[i].ip != 0)
                destino.sin_addr.s_addr = miembros[i].ip;
        xSemaphoreGive(candado);
    }
    for (int i = 0; i < veces; i++) {                   // varias veces, por si alguna se pierde
        sendto(sock, linea, largo, 0, (struct sockaddr *)&destino, sizeof(destino));
        if (i + 1 < veces) vTaskDelay(pdMS_TO_TICKS(ENTRE_VECES_MS));
    }
}

void familia_mandar(const char *para, familia_tipo_t tipo, const char *dato)
{
    if ((unsigned)tipo >= FAMILIA_CANT_TIPOS) return;
    ESP_LOGI(TAG, "Le mando %s a %s", palabras[tipo], para);
    mandar_palabra(para, palabras[tipo], dato, VECES);
}

// ─── Escuchar ────────────────────────────────────────────────────────────────

static void avisar(familia_tipo_t tipo, const char *de, const char *dato)
{
    familia_evento_t e = { .tipo = tipo };
    snprintf(e.de, sizeof(e.de), "%s", de);
    snprintf(e.dato, sizeof(e.dato), "%s", dato ? dato : "");
    xQueueSend(eventos, &e, 0);
}

// Para no saludar dos veces seguidas al mismo. Hay que tener el candado.
static bool toca_saludar(miembro_t *m)
{
    if (m->saludado_us != 0 && ahora_us() - m->saludado_us < (int64_t)SALUDO_CADA_MS * 1000) return false;
    m->saludado_us = ahora_us();
    return true;
}

static void procesar(char *linea, uint32_t ip)
{
    // FAMILIA <version> <clave> <de> <para> <numero> <TIPO> [dato]
    char *resto = NULL;
    char *marca   = strtok_r(linea, " ", &resto);
    char *version = strtok_r(NULL, " ", &resto);
    char *clave   = strtok_r(NULL, " ", &resto);
    char *de      = strtok_r(NULL, " ", &resto);
    char *para    = strtok_r(NULL, " ", &resto);
    char *numero  = strtok_r(NULL, " ", &resto);
    char *palabra = strtok_r(NULL, " ", &resto);
    const char *dato = resto ? resto : "";
    if (!marca || !version || !clave || !de || !para || !numero || !palabra) return;
    if (strcmp(marca, "FAMILIA") != 0) return;
    if (strcmp(version, VERSION) != 0) {
        ESP_LOGW(TAG, "%s habla otra version (%s): ¡hay que actualizarlo!", de, version);
        return;
    }
    if (strcmp(clave, FAMILIA_CLAVE) != 0) return;          // no es de la familia
    if (strcmp(de, mi_nombre) == 0) return;                 // es mio (me escucho a mi mismo)
    if (strcmp(para, "*") != 0 && strcmp(para, mi_nombre) != 0) return;   // es para otro
    if (strlen(de) >= FAMILIA_MAX_NOMBRE) return;
    uint32_t num = (uint32_t)strtoul(numero, NULL, 10);

    xSemaphoreTake(candado, portMAX_DELAY);
    bool es_nuevo;
    miembro_t *m = anotar(de, ip, &es_nuevo);               // cualquier mensaje dice "estoy"

    // ¿Ya le hicimos caso a este mensaje? (llega varias veces)
    bool repetido = false;
    for (int i = 0; i < RECUERDA_NUMEROS; i++)
        if (m->numeros[i] == num && num != 0) repetido = true;
    if (!repetido) {
        m->numeros[m->prox_numero] = num;
        m->prox_numero = (m->prox_numero + 1) % RECUERDA_NUMEROS;
    }
    // Si recien aparece (y no es su HOLA), lo saludamos igual: quizas su HOLA se perdio
    bool saludar_nuevo = es_nuevo && strcmp(palabra, "HOLA") != 0 && toca_saludar(m);
    xSemaphoreGive(candado);

    if (saludar_nuevo) {
        mandar_palabra(de, palabras[FAMILIA_SALUDO], NULL, VECES);
        avisar(FAMILIA_HOLA, de, NULL);
    }
    if (repetido || strcmp(palabra, "ESTOY") == 0) return;

    int tipo = -1;
    for (int i = 0; i < FAMILIA_CANT_TIPOS; i++)
        if (strcmp(palabra, palabras[i]) == 0) tipo = i;
    if (tipo < 0) {
        ESP_LOGI(TAG, "%s me dijo \"%s\", pero todavia no se que es: lo ignoro", de, palabra);
        return;
    }

    if (tipo == FAMILIA_HOLA) mandar_palabra(de, palabras[FAMILIA_SALUDO], NULL, VECES);   // contestamos

    if (tipo == FAMILIA_HOLA || tipo == FAMILIA_SALUDO) {
        xSemaphoreTake(candado, portMAX_DELAY);
        bool toca = toca_saludar(m);
        xSemaphoreGive(candado);
        if (!toca) return;
    }

    ESP_LOGI(TAG, "%s me mando %s", de, palabra);
    avisar((familia_tipo_t)tipo, de, dato);
}

static bool abrir_socket(void)
{
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) return false;
    int si = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &si, sizeof(si));
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &si, sizeof(si));
    struct timeval espera = { .tv_sec = 1, .tv_usec = 0 };   // recvfrom no se queda trabado
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &espera, sizeof(espera));

    struct sockaddr_in yo = { .sin_family = AF_INET, .sin_port = htons(FAMILIA_PUERTO) };
    yo.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sock, (struct sockaddr *)&yo, sizeof(yo)) < 0) {
        close(sock);
        sock = -1;
        return false;
    }
    return true;
}

static void tarea_familia(void *arg)
{
    bool estaba_conectado = false;
    int64_t ultimo_aviso_us = 0;
    char buf[200];

    while (1) {
        if (!wifi_conectado()) {
            estaba_conectado = false;
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
        if (sock < 0 && !abrir_socket()) {
            ESP_LOGW(TAG, "No pude abrir el puerto %d: reintento en un rato", FAMILIA_PUERTO);
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }
        if (!estaba_conectado) {                     // recien conectado: ¡hola a todos!
            estaba_conectado = true;
            // Sin el "ahorro de energia" del WiFi: con el ahorro, la placa a veces
            // esta "dormida" y se pierde los mensajes para todos
            esp_wifi_set_ps(WIFI_PS_NONE);
            ESP_LOGI(TAG, "Soy %s y aviso HOLA a la familia", mi_nombre);
            mandar_palabra("*", palabras[FAMILIA_HOLA], NULL, VECES);
            ultimo_aviso_us = ahora_us();
        }
        if (ahora_us() - ultimo_aviso_us >= (int64_t)AVISO_CADA_MS * 1000) {   // "sigo aca"
            mandar_palabra("*", "ESTOY", NULL, 1);
            ultimo_aviso_us = ahora_us();
        }

        struct sockaddr_in origen;
        socklen_t largo_origen = sizeof(origen);
        int n = recvfrom(sock, buf, sizeof(buf) - 1, 0, (struct sockaddr *)&origen, &largo_origen);
        if (n > 0) {
            buf[n] = '\0';
            char *fin = strpbrk(buf, "\r\n");
            if (fin) *fin = '\0';
            procesar(buf, origen.sin_addr.s_addr);
        }
    }
}

#else   // sin palabra secreta: el personaje esta solito

void familia_mandar(const char *para, familia_tipo_t tipo, const char *dato)
{
    (void)para; (void)tipo; (void)dato;
}

#endif // FAMILIA_HAY_SECRETOS

void familia_iniciar(const char *nombre)
{
    snprintf(mi_nombre, sizeof(mi_nombre), "%s", nombre);
    candado = xSemaphoreCreateMutex();
    eventos = xQueueCreate(8, sizeof(familia_evento_t));
#ifdef FAMILIA_HAY_SECRETOS
    ultimo_numero = esp_random() % 1000000;        // asi, si se reinicia, no repite numeros viejos
    xTaskCreate(tarea_familia, "familia", 4096, NULL, 4, NULL);
#else
    ESP_LOGW(TAG, "No hay palabra secreta de la familia: %s esta solito. Para hablar con los "
                  "demas, copia familia_secretos.ejemplo.h como familia_secretos.h", mi_nombre);
#endif
}
