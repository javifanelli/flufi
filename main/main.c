#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"

#include "flufi_config.h"
#include "board_config.h"
#include "display.h"
#include "eyes.h"
#include "led.h"
#include "wifi.h"
#include "reloj.h"
#include "bmp280.h"
#include "botones.h"
#include "menu.h"
#include "pronostico.h"
#include "familia.h"

static const char *TAG = "FLUFI";

static bool ya_festejo_wifi  = false;
static bool ya_saludo_hora   = false;

// Muestra un texto grande en el centro de la pantalla
static void mostrar_mensaje(const char *texto)
{
    if (texto[0] == '\0') return;      // mensaje vacio: no mostrar nada
    display_clear(COLOR_FONDO);
    display_draw_text_centered(texto, -1, MENSAJE_ARRANQUE_TAMANO, COLOR_TEXTO);
    display_flush();
    vTaskDelay(pdMS_TO_TICKS(MENSAJE_ARRANQUE_MS));
}

static bool atendiendo_familia = false;   // true mientras reacciona a lo que le dijo la familia

// ¿Hay que cortar lo que esta haciendo? Si aprietan el boton A (menu) o si le
// hablo alguien de la familia (salvo que ya este contestandole a alguien).
static bool boton_menu_apretado(void)
{
    return botones_pendiente(BOTON_A) || (!atendiendo_familia && familia_hay_evento());
}

// "Tengo wifi!" al costado de la cara (solo la primera vez que se conecta)
static void festejar_wifi(void)
{
    ya_festejo_wifi = true;
    ESP_LOGI(TAG, "Tengo wifi!");
    if (MENSAJE_WIFI[0] != '\0') eyes_decir(MENSAJE_WIFI, 2, MOSTRAR_WIFI_MS);
}

// Saluda segun la hora: buen dia, buenas tardes o buenas noches
static void saludar_segun_la_hora(void)
{
    struct tm t;
    if (!reloj_hora(&t)) return;
    ya_saludo_hora = true;

    const char *saludo;
    if (t.tm_hour >= 6 && t.tm_hour < 12)       saludo = "¡Buen\ndia!";
    else if (t.tm_hour >= 12 && t.tm_hour < 20) saludo = "¡Buenas\ntardes!";
    else                                         saludo = "¡Buenas\nnoches!";

    ESP_LOGI(TAG, "Son las %02d:%02d, saludo", t.tm_hour, t.tm_min);
    eyes_decir(saludo, 2, MOSTRAR_SALUDO_MS);
}

// ─── Pantallas automaticas (en punto): la cara se aleja y el dato va al costado ──

static void decir_hora(const struct tm *t)
{
    char texto[32];
    snprintf(texto, sizeof(texto), "%02d:%02d", t->tm_hour, t->tm_min);
    ESP_LOGI(TAG, "Son las %s", texto);
    eyes_decir(texto, 2, MOSTRAR_HORA_MS);
}

static void decir_sensor(void)
{
    int16_t t, p;
    if (!bmp280_ultima_lectura(&t, &p)) return;    // todavia no hay ninguna lectura

    char texto[64];
    snprintf(texto, sizeof(texto), "Temp:\n%s%d.%d°C\n\nPresion:\n%d hPa",
             t < 0 ? "-" : "", abs(t) / 10, abs(t) % 10, p);
    ESP_LOGI(TAG, "Sensor: %s%d.%d C, %d hPa", t < 0 ? "-" : "", abs(t) / 10, abs(t) % 10, p);
    eyes_decir(texto, 1, MOSTRAR_SENSOR_MS);
}

// El tiempo de afuera: un dibujo animado, como esta el cielo, la temperatura
// y si hace frio o calor. Devuelve false si todavia no hay pronostico.
static bool contar_clima(void)
{
    pronostico_t p;
    if (!pronostico_ultimo(&p)) return false;
    eyes_contar_clima(p.codigo, p.temp_decimas, p.es_de_dia, MOSTRAR_CLIMA_MS);
    return true;
}

// Le cuenta el tiempo de afuera a la familia (Pitri no tiene pronostico)
static void compartir_clima(void)
{
    pronostico_t p;
    if (!pronostico_ultimo(&p)) return;
    char dato[32];
    snprintf(dato, sizeof(dato), "%d %s", p.temp_decimas, eyes_palabra_clima(p.codigo, p.es_de_dia));
    familia_mandar("*", FAMILIA_CLIMA, dato);
}

// Muestra la hora en x:00, x:05, x:10..., el sensor en x:00, x:15, x:30, x:45 y
// el tiempo de afuera en x:00 y x:30. Si todavia no sabe la hora, muestra el
// sensor cada SENSOR_CADA_MINUTOS contando desde que se prendio. (Como Pitri)
static void revisar_horarios(void)
{
    static int ultima_hora_mostrada = -1;      // "minuto del año" de la ultima vez
    static int ultimo_sensor_mostrado = -1;
    static int ultimo_clima_mostrado = -1;
    static int64_t ultimo_sensor_sin_hora_us = 0;

    struct tm t;
    if (reloj_hora(&t)) {
        int este_minuto = t.tm_yday * 24 * 60 + t.tm_hour * 60 + t.tm_min;

        if (t.tm_min % HORA_CADA_MINUTOS == 0 && este_minuto != ultima_hora_mostrada) {
            ultima_hora_mostrada = este_minuto;
            decir_hora(&t);
        }
        if (t.tm_min % SENSOR_CADA_MINUTOS == 0 && este_minuto != ultimo_sensor_mostrado) {
            ultimo_sensor_mostrado = este_minuto;
            decir_sensor();
        }
        if (t.tm_min % CLIMA_CADA_MINUTOS == 0 && este_minuto != ultimo_clima_mostrado) {
            ultimo_clima_mostrado = este_minuto;
            if (contar_clima()) compartir_clima();
        }
    } else {
        int64_t ahora = esp_timer_get_time();
        if (ahora - ultimo_sensor_sin_hora_us >= (int64_t)SENSOR_CADA_MINUTOS * 60 * 1000 * 1000) {
            ultimo_sensor_sin_hora_us = ahora;
            decir_sensor();
        }
    }
}

// ─── Pantallas del menu: Hora y Sensor ───────────────────────────────────────

static void mostrar_texto(const char *texto, int ms)
{
    display_clear(COLOR_FONDO);
    display_draw_text_centered(texto, -1, 2, COLOR_TEXTO);
    display_flush();
    vTaskDelay(pdMS_TO_TICKS(ms));
}

// La hora grande, el dia y la fecha. Si se aprieta cualquier boton, sale antes.
static void mostrar_hora(void)
{
    static const char *const dias[7] = {
        "Domingo", "Lunes", "Martes", "Miercoles", "Jueves", "Viernes", "Sabado",
    };
    struct tm t;
    if (!reloj_hora(&t)) {
        mostrar_texto(wifi_conectado() ? "Buscando\nla hora..." : "Sin WiFi\nno tengo\nhora", 2500);
        return;
    }
    ESP_LOGI(TAG, "Mostrando la hora");
    botones_olvidar();
    for (int ms = 0; ms < MOSTRAR_EN_MENU_MS; ms += 250) {
        if (botones_se_apreto(BOTON_A) || botones_se_apreto(BOTON_B) || botones_se_apreto(BOTON_BOOT)) break;
        reloj_hora(&t);
        // Los dos puntos titilan: aparecen un segundo si y otro no
        char hora[32], fecha[40];
        snprintf(hora, sizeof(hora), "%02d%c%02d", t.tm_hour, (t.tm_sec % 2) ? ' ' : ':', t.tm_min);
        snprintf(fecha, sizeof(fecha), "%02d/%02d/%04d", t.tm_mday, t.tm_mon + 1, t.tm_year + 1900);
        display_clear(COLOR_FONDO);
        display_draw_text_centered(hora, 18, 4, COLOR_TEXTO);
        display_draw_text_centered(dias[t.tm_wday], 64, 2, COLOR_PESTANAS);
        display_draw_text_centered(fecha, 94, 2, COLOR_OJOS);
        display_flush();
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

// La temperatura grande y la presion del aire. Si se aprieta cualquier boton, sale antes.
static void mostrar_sensor(void)
{
    int16_t t, p;
    if (!bmp280_ultima_lectura(&t, &p)) {
        mostrar_texto("Todavia\nno hay\ndatos", 2500);
        return;
    }
    ESP_LOGI(TAG, "Mostrando el sensor");
    botones_olvidar();
    for (int ms = 0; ms < MOSTRAR_EN_MENU_MS; ms += 250) {
        if (botones_se_apreto(BOTON_A) || botones_se_apreto(BOTON_B) || botones_se_apreto(BOTON_BOOT)) break;
        bmp280_ultima_lectura(&t, &p);
        char temp[32], presion[32];
        snprintf(temp, sizeof(temp), "%s%d.%d°C", t < 0 ? "-" : "", abs(t) / 10, abs(t) % 10);
        snprintf(presion, sizeof(presion), "%d hPa", p);
        display_clear(COLOR_FONDO);
        display_draw_text_centered("Temperatura", 10, 1, COLOR_PESTANAS);
        display_draw_text_centered(temp, 24, 3, COLOR_TEXTO);
        display_draw_text_centered("Presion del aire", 66, 1, COLOR_PESTANAS);
        display_draw_text_centered(presion, 80, 2, COLOR_OJOS);
        display_flush();
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

// ─── Comer y tomar ───────────────────────────────────────────────────────────
// Flufi puede comer hasta COMIDAS_POR_HORA veces y tomar hasta BEBIDAS_POR_HORA
// veces en una hora. Si ya llego a eso, esta llena y dice que no. No le pasa
// nada malo si no come ni toma: Flufi nunca muere.

static const comida_t comidas[] = { COMIDA_MEDIALUNA, COMIDA_MUFFIN, COMIDA_DONA };
static const char *const nombres_comidas[] = { "Medialuna", "Muffin", "Dona" };
#define CANT_COMIDAS 3

static const comida_t bebidas[] = { COMIDA_TE, COMIDA_LICUADO, COMIDA_GASEOSA };
static const char *const nombres_bebidas[] = { "Te", "Licuado", "Gaseosa" };
#define CANT_BEBIDAS 3

// Cuando comio y cuando tomo las ultimas veces (microsegundos desde que se prendio)
static int64_t cuando_comio[COMIDAS_POR_HORA];
static int64_t cuando_tomo[BEBIDAS_POR_HORA];

// Cuantas de las veces anotadas fueron en los ultimos 60 minutos
static int veces_en_la_ultima_hora(const int64_t *veces, int cantidad)
{
    const int64_t una_hora_us = 60LL * 60 * 1000 * 1000;
    int64_t ahora = esp_timer_get_time();
    int cuantas = 0;
    for (int i = 0; i < cantidad; i++)
        if (veces[i] != 0 && ahora - veces[i] < una_hora_us) cuantas++;
    return cuantas;
}

// Anota "ahora" en el lugar de la vez mas vieja
static void anotar(int64_t *veces, int cantidad)
{
    int mas_vieja = 0;
    for (int i = 1; i < cantidad; i++)
        if (veces[i] < veces[mas_vieja]) mas_vieja = i;
    veces[mas_vieja] = esp_timer_get_time();
}

static void dar_de_comer(int cual)
{
    eyes_reiniciar();
    if (veces_en_la_ultima_hora(cuando_comio, COMIDAS_POR_HORA) >= COMIDAS_POR_HORA) {
        ESP_LOGI(TAG, "Flufi esta llena (ya comio %d veces en la ultima hora)", COMIDAS_POR_HORA);
        eyes_decir_que_no();
        return;
    }
    anotar(cuando_comio, COMIDAS_POR_HORA);
    ESP_LOGI(TAG, "Flufi come %s (%d de %d en la ultima hora)", nombres_comidas[cual],
             veces_en_la_ultima_hora(cuando_comio, COMIDAS_POR_HORA), COMIDAS_POR_HORA);
    eyes_comer(comidas[cual]);
}

static void dar_de_tomar(int cual)
{
    eyes_reiniciar();
    if (veces_en_la_ultima_hora(cuando_tomo, BEBIDAS_POR_HORA) >= BEBIDAS_POR_HORA) {
        ESP_LOGI(TAG, "Flufi esta llena (ya tomo %d veces en la ultima hora)", BEBIDAS_POR_HORA);
        eyes_decir_que_no();
        return;
    }
    anotar(cuando_tomo, BEBIDAS_POR_HORA);
    ESP_LOGI(TAG, "Flufi toma %s (%d de %d en la ultima hora)", nombres_bebidas[cual],
             veces_en_la_ultima_hora(cuando_tomo, BEBIDAS_POR_HORA), BEBIDAS_POR_HORA);
    eyes_tomar(bebidas[cual]);
}

// Dibuja una opcion de los menus "Comer" y "Tomar": el dibujo grande con su
// nombre abajo, o la palabra "Atras" si ya se pasaron todas
static void dibujar_opcion(const comida_t *lista, const char *const *nombres, int cantidad, int opcion)
{
    if (opcion < cantidad) {
        comida_dibujar(lista[opcion], LCD_H_RES / 2.0f, 62, 1.6f, 0, COLOR_FONDO);
        display_draw_text_centered(nombres[opcion], 100, 2, COLOR_TEXTO);
    } else {
        display_draw_text_centered("Atras", -1, 3, COLOR_TEXTO);
    }
}

static void dibujar_opcion_comida(int opcion) { dibujar_opcion(comidas, nombres_comidas, CANT_COMIDAS, opcion); }
static void dibujar_opcion_bebida(int opcion) { dibujar_opcion(bebidas, nombres_bebidas, CANT_BEBIDAS, opcion); }

// ─── La familia (Pitri, Flufi, ...) ──────────────────────────────────────────
// Los personajes de la casa se hablan por la red WiFi (ver docs/FAMILIA.md).

static int64_t proxima_iniciativa_us = 0;   // cuando le toca empezar una charla o un juego

// Dice algo de la familia al costado de la cara
static void decir_familia(const char *texto)
{
    eyes_decir(texto, 2, MOSTRAR_FAMILIA_MS);
}

// La cara que acompaña a un renglon de una charla
static void emocionarse(familia_emocion_t emocion)
{
    switch (emocion) {
    case FAMILIA_EMOCION_CONTENTA:
    case FAMILIA_EMOCION_RISA: eyes_jugar_juego("contento"); break;
    case FAMILIA_EMOCION_AMOR: eyes_enamorarse();             break;
    default:                                                  break;
    }
}

// Dice el renglon "renglon" de la charla y le pasa el turno a "con"
static void decir_renglon(int charla, int renglon, const char *con)
{
    familia_emocion_t emocion;
    const char *texto = familia_charla(charla, renglon, &emocion);
    if (texto == NULL) return;
    decir_familia(texto);
    emocionarse(emocion);
    if (familia_charla(charla, renglon + 1, NULL) != NULL) {      // si sigue, le toca al otro
        char dato[16];
        snprintf(dato, sizeof(dato), "%d %d", charla, renglon + 1);
        familia_mandar(con, FAMILIA_CHARLA, dato);
    }
}

static void empezar_charla(const char *con)
{
    ESP_LOGI(TAG, "Empiezo una charla con %s", con);
    bool antes = atendiendo_familia;
    atendiendo_familia = true;          // que no se corte lo que dice
    decir_renglon((int)(esp_random() % familia_cant_charlas()), 0, con);
    atendiendo_familia = antes;
}

// Le propone un juego a "con" y lo hacen los dos a la vez
static void jugar_con(const char *con)
{
    const char *juego = familia_juego_junto((int)(esp_random() % familia_cant_juegos_juntos()));
    ESP_LOGI(TAG, "Juego a \"%s\" con %s", juego, con);
    familia_mandar(con, FAMILIA_JUGAR, juego);
    bool antes = atendiendo_familia;
    atendiendo_familia = true;          // que no se corte el juego
    eyes_jugar_juego(juego);
    atendiendo_familia = antes;
}

// Cada tanto, si hay alguien conectado, empieza una charla o un juego
static void iniciativa_familia(void)
{
    int64_t ahora = esp_timer_get_time();
    int64_t espera = (FAMILIA_INICIATIVA_MIN_S +
                      esp_random() % (FAMILIA_INICIATIVA_MAX_S - FAMILIA_INICIATIVA_MIN_S)) * 1000000LL;
    if (proxima_iniciativa_us == 0) proxima_iniciativa_us = ahora + espera;
    if (ahora < proxima_iniciativa_us) return;
    proxima_iniciativa_us = ahora + espera;

    char con[FAMILIA_MAX_NOMBRE];
    if (!familia_alguien_al_azar(con)) return;
    if (esp_random() % 3 == 0) jugar_con(con);       // a veces juegan, y mas seguido charlan
    else                       empezar_charla(con);
}

// Reacciona a lo que le dijeron los demas
static void atender_familia(void)
{
    familia_evento_t e;
    char texto[64];
    atendiendo_familia = true;          // mientras reacciona, lo que llega espera su turno
    while (familia_proximo_evento(&e)) {
        switch (e.tipo) {
        case FAMILIA_HOLA:
        case FAMILIA_SALUDO:
            eyes_saludar();
            snprintf(texto, sizeof(texto), "¡Hola\n%s!", e.de);
            decir_familia(texto);
            break;
        case FAMILIA_BESO:
            eyes_enamorarse();
            snprintf(texto, sizeof(texto), "¡Un beso\nde\n%s!", e.de);
            decir_familia(texto);
            break;
        case FAMILIA_CHARLA: {
            int charla, renglon;
            if (sscanf(e.dato, "%d %d", &charla, &renglon) == 2) decir_renglon(charla, renglon, e.de);
            break;
        }
        case FAMILIA_VISITA:
            eyes_saludar();
            decir_familia("¡Llego\nalguien!");
            break;
        case FAMILIA_COMIDA:
            snprintf(texto, sizeof(texto), "¡Gracias\n%s!", e.de);
            decir_familia(texto);
            eyes_comer(comidas[esp_random() % CANT_COMIDAS]);
            break;
        case FAMILIA_COSQUILLAS:
            eyes_jugar_juego("contento");
            snprintf(texto, sizeof(texto), "¡Jaja!\n¡Basta,\n%s!", e.de);
            decir_familia(texto);
            break;
        case FAMILIA_JUGAR:
            eyes_jugar_juego(e.dato);       // el mismo juego que el otro, a la vez
            break;
        case FAMILIA_TRISTE:                // ¡a consolarlo!
            snprintf(texto, sizeof(texto), "¡No\nllores,\n%s!", e.de);
            decir_familia(texto);
            familia_mandar(e.de, FAMILIA_BESO, NULL);
            break;
        case FAMILIA_CLIMA: {               // el tiempo de afuera, que le cuenta otro
            int t;
            char cielo[20];
            if (sscanf(e.dato, "%d %19s", &t, cielo) == 2) {
                int grados = (t + (t >= 0 ? 5 : -5)) / 10;
                snprintf(texto, sizeof(texto), "Afuera:\n%s\n%d°C", cielo, grados);
                decir_familia(texto);
            }
            break;
        }
        case FAMILIA_SENSOR: {              // lo que mide el sensor de otro
            int t, h;
            if (sscanf(e.dato, "%d %d", &t, &h) == 2) {
                snprintf(texto, sizeof(texto), "%s:\nHumedad\n%d%%", e.de, h);
                decir_familia(texto);
            }
            break;
        }
        case FAMILIA_MENSAJE:
            snprintf(texto, sizeof(texto), "¡%s\ntiene un\nmensaje!", e.de);
            decir_familia(texto);
            break;
        case FAMILIA_DORMIR:
            eyes_jugar_juego("sueno");      // le da sueño a el tambien
            snprintf(texto, sizeof(texto), "%s se\ndurmio", e.de);
            decir_familia(texto);
            break;
        default:
            break;
        }
    }
    atendiendo_familia = false;
}

// Desde el menu: elegir a alguien de la familia y que hacerle
static void menu_familia(void)
{
    if (!familia_configurada()) {
        mostrar_texto("Sin\nfamilia", 2000);
        return;
    }
    static char nombres[FAMILIA_MAX_MIEMBROS][FAMILIA_MAX_NOMBRE];
    int n = familia_conectados(nombres, FAMILIA_MAX_MIEMBROS);
    if (n == 0) {
        mostrar_texto("No hay\nnadie\nconectado", 2000);
        return;
    }
    const char *opciones[FAMILIA_MAX_MIEMBROS + 1];
    for (int i = 0; i < n; i++) opciones[i] = nombres[i];
    opciones[n] = "Atras";
    int quien = menu_elegir("Familia", opciones, n + 1, 0, -1, MENU_ESPERA_MAX_MS);
    if (quien < 0 || quien >= n) return;                 // "Atras" o se cerro solo
    const char *con = nombres[quien];

    static const char *const acciones[] = { "Beso", "Cosquis", "Convidar", "Charlar", "Jugar", "Atras" };
    switch (menu_elegir(con, acciones, 6, 0, -1, MENU_ESPERA_MAX_MS)) {
    case 0: familia_mandar(con, FAMILIA_BESO, NULL);       mostrar_texto("¡Beso\nenviado!", 1500);  break;
    case 1: familia_mandar(con, FAMILIA_COSQUILLAS, NULL); mostrar_texto("¡Cosqui\ncosqui!", 1500); break;
    case 2: familia_mandar(con, FAMILIA_COMIDA, NULL);     mostrar_texto("¡Le\nconvide!", 1500);    break;
    case 3: eyes_reiniciar(); empezar_charla(con); break;
    case 4: eyes_reiniciar(); jugar_con(con);      break;
    default: break;
    }
}

// ─── Modos ───────────────────────────────────────────────────────────────────
// En el modo Normal Flufi juega con sus emociones. En los otros hace de
// personaje (siempre con sus ojos, sus pestañas y sus labios).

typedef enum { MODO_NORMAL, MODO_ANIMALES, MODO_FANTASIA, MODO_EJERCICIO, CANT_MODOS } modo_t;
static const char *const nombres_modos[CANT_MODOS] = { "Normal", "Animales", "Fantasia", "Ejercicio" };
static modo_t modo = MODO_NORMAL;

// El primer personaje de cada modo (los 5 de cada modo estan seguidos en eyes.h)
static const eyes_personaje_t primer_personaje[CANT_MODOS] = {
    [MODO_ANIMALES]  = PRIMER_ANIMAL,
    [MODO_FANTASIA]  = PRIMER_FANTASIA,
    [MODO_EJERCICIO] = PRIMER_EJERCICIO,
};

static eyes_personaje_t ultimo_personaje = PERSONAJE_NINGUNO;

// Elige un personaje del modo de ahora, distinto del ultimo que hizo
static eyes_personaje_t elegir_personaje(void)
{
    eyes_personaje_t p;
    do {
        p = (eyes_personaje_t)(primer_personaje[modo] + esp_random() % PERSONAJES_POR_MODO);
    } while (p == ultimo_personaje);
    ultimo_personaje = p;
    return p;
}

static void cambiar_modo(modo_t nuevo)
{
    modo = nuevo;
    ultimo_personaje = PERSONAJE_NINGUNO;
    ESP_LOGI(TAG, "Modo %s", nombres_modos[modo]);
    char texto[32];
    snprintf(texto, sizeof(texto), "Modo\n%s", nombres_modos[modo]);
    mostrar_texto(texto, 1500);
}

// ─── El menu ─────────────────────────────────────────────────────────────────
//   Menu:  Modo / Comer / Tomar / Hora / Sensor / Clima / Familia / Salir
//   Modo:  Normal / Animales / Fantasia / Ejercicio / Atras (el de ahora tiene un puntito)
//   Comer: Medialuna / Muffin / Dona / Atras (con dibujos)
//   Tomar: Te / Licuado / Gaseosa / Atras (con dibujos)
// Boton A = elegir, boton B = siguiente, boton BOOT = volver atras

static void abrir_menu(void)
{
    static const char *const opciones_menu[] = { "Modo", "Comer", "Tomar", "Hora", "Sensor", "Clima", "Familia", "Salir" };
    enum { OPCION_MODO, OPCION_COMER, OPCION_TOMAR, OPCION_HORA, OPCION_SENSOR, OPCION_CLIMA, OPCION_FAMILIA, OPCION_SALIR };
    static const char *const opciones_modo[CANT_MODOS + 1] = { "Normal", "Animales", "Fantasia", "Ejercicio", "Atras" };

    ESP_LOGI(TAG, "Abriendo el menu");
    led_color(255, 120, 190);          // LED rosa mientras esta el menu
    int marcada = 0;

    while (1) {
        int elegida = menu_elegir("", opciones_menu, 8, marcada, -1, MENU_ESPERA_MAX_MS);
        if (elegida >= 0) marcada = elegida;   // al volver, queda marcada la misma

        if (elegida == OPCION_MODO) {
            int m = menu_elegir("Modo", opciones_modo, CANT_MODOS + 1, modo, modo, MENU_ESPERA_MAX_MS);
            if (m == MENU_SE_CERRO) break;
            if (m < 0 || m == CANT_MODOS) continue;   // "Atras" o BOOT: vuelve al menu
            cambiar_modo((modo_t)m);
            break;                              // y vuelve a la cara
        }
        if (elegida == OPCION_COMER) {
            int c = menu_elegir_dibujos("Comer", CANT_COMIDAS + 1, dibujar_opcion_comida, 0, MENU_ESPERA_MAX_MS);
            if (c == CANT_COMIDAS || c == MENU_ATRAS) continue;
            if (c >= 0) dar_de_comer(c);
            break;                              // despues vuelve a la cara
        }
        if (elegida == OPCION_TOMAR) {
            int b = menu_elegir_dibujos("Tomar", CANT_BEBIDAS + 1, dibujar_opcion_bebida, 0, MENU_ESPERA_MAX_MS);
            if (b == CANT_BEBIDAS || b == MENU_ATRAS) continue;
            if (b >= 0) dar_de_tomar(b);
            break;
        }
        if (elegida == OPCION_HORA) {
            mostrar_hora();
            break;
        }
        if (elegida == OPCION_FAMILIA) {
            menu_familia();
            break;
        }
        if (elegida == OPCION_CLIMA) {
            eyes_reiniciar();
            if (!contar_clima())
                mostrar_texto(!pronostico_configurado() ? "Sin\npronostico" :
                              wifi_conectado() ? "Buscando\nel tiempo" : "Sin WiFi", 2500);
            break;
        }
        if (elegida == OPCION_SENSOR) {
            mostrar_sensor();
            break;
        }
        break;                                  // "Salir", BOOT o se cerro solo
    }

    botones_olvidar();                          // apretadas sueltas no cuentan
    eyes_reiniciar();                           // vuelve la cara de Flufi
}

// ─── Arranque ────────────────────────────────────────────────────────────────

// Espera (con las ondas al costado) hasta que "listo" sea true, pase el tiempo
// o aprieten el boton del menu
static bool esperar_con_ondas(bool (*listo)(void), int max_ms)
{
    int64_t inicio = esp_timer_get_time();
    while (!listo() && !botones_pendiente(BOTON_A) &&
           esp_timer_get_time() - inicio < (int64_t)max_ms * 1000) {
        eyes_onditas();
    }
    return listo();
}

// Al arrancar: se conecta mostrando ondas, festeja el WiFi y saluda segun la hora
static void conectarse_y_saludar(void)
{
    if (!wifi_configurado()) return;          // sin datos de red no hay nada que esperar

    if (!esperar_con_ondas(wifi_conectado, WIFI_ESPERA_ARRANQUE_MS)) {
        ESP_LOGI(TAG, "Todavia no hay wifi: sigo y lo festejo cuando llegue");
        return;
    }
    festejar_wifi();

    if (esperar_con_ondas(reloj_sincronizado, ESPERA_HORA_MS)) saludar_segun_la_hora();
}

// Un descansito, pero corta si aprietan el boton del menu
static void descansar(int ms)
{
    for (int t = 0; t < ms && !botones_pendiente(BOTON_A) && !familia_hay_evento(); t += 50)
        vTaskDelay(pdMS_TO_TICKS(50));
}

void app_main(void)
{
    ESP_LOGI(TAG, "¡Hola! Soy Flufi :)");

    led_iniciar(LED_BRILLO);           // el LED de la placa (arranca apagado)
    display_init();
    wifi_iniciar();                    // se conecta "de fondo"
    reloj_iniciar(ZONA_HORARIA);       // cuando haya WiFi, pone la hora
    bmp280_iniciar(TIEMPO_ENTRE_LECTURAS_MS);   // temperatura y presion
    pronostico_iniciar();              // cuando haya WiFi, consulta el tiempo de afuera
    familia_iniciar(MI_NOMBRE);        // cuando haya WiFi, saluda a la familia y la escucha
    botones_iniciar();                 // A = menu y elegir, B = siguiente, BOOT = atras
    menu_colores(COLOR_FONDO, COLOR_TEXTO, COLOR_PESTANAS, COLOR_FONDO);

    // Saludo inicial
    mostrar_mensaje(MENSAJE_ARRANQUE);
    mostrar_mensaje(MENSAJE_ARRANQUE_2);

    eyes_set_interrupcion(boton_menu_apretado);
    eyes_init(COLOR_OJOS, COLOR_PESTANAS, COLOR_BOCA, COLOR_TEXTO, COLOR_FONDO);
    conectarse_y_saludar();
    if (!botones_pendiente(BOTON_A)) eyes_saludar();   // ¡con el LED en arco iris!

    while (1) {
        // Boton A: abre el menu (B y BOOT solo sirven adentro del menu)
        botones_se_apreto(BOTON_B);
        botones_se_apreto(BOTON_BOOT);
        if (botones_se_apreto(BOTON_A)) {
            abrir_menu();
            continue;
        }

        // Lo que le dijeron los demas de la familia (saludos, besos, charlas...)
        atender_familia();
        // Y cada tanto, Flufi empieza una charla o un juego
        iniciativa_familia();

        // Si el WiFi o la hora llegaron tarde, los festeja ahora
        if (!ya_festejo_wifi && wifi_conectado()) festejar_wifi();
        if (ya_festejo_wifi && !ya_saludo_hora && reloj_sincronizado()) saludar_segun_la_hora();

        // En punto: la hora cada 5 minutos y el sensor cada 15
        revisar_horarios();

        // Juega con sus emociones (modo Normal) o hace de personaje (los otros modos)
        if (modo != MODO_NORMAL) eyes_actuar(elegir_personaje());
        else if (strcmp(eyes_jugar(), "llorar") == 0) familia_mandar("*", FAMILIA_TRISTE, NULL);   // ¡que la consuelen!

        // Un descansito entre juego y juego, y a veces un parpadeo
        descansar(DESCANSO_MIN_MS + esp_random() % (DESCANSO_MAX_MS - DESCANSO_MIN_MS));
        if (!botones_pendiente(BOTON_A) && esp_random() % 2) eyes_parpadear();
    }
}
