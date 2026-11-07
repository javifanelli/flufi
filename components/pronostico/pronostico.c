#include "pronostico.h"
#include "wifi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"

// El lugar esta en clima_secretos.h, que NO se sube al repo (es publico). Si el
// archivo existe, CMakeLists.txt define FLUFI_HAY_SECRETOS_CLIMA.
#ifdef FLUFI_HAY_SECRETOS_CLIMA
#include "clima_secretos.h"
#endif

static const char *TAG = "PRONOSTICO";

/*
 * Como funciona:
 *   - Open-Meteo es un servicio gratis de pronostico. Se le pregunta por internet
 *     "¿como esta el tiempo en este lugar?" y contesta con numeros (en JSON):
 *       "current": { "temperature_2m": 23.5, "weather_code": 3, "is_day": 1 }
 *   - weather_code es un numero que dice como esta el cielo (despejado,
 *     nublado, lluvia, nieve, tormenta...). Es un codigo mundial (WMO).
 *   - Los datos se actualizan cada 15 minutos, asi que no tiene sentido
 *     preguntar mas seguido.
 */
#define PRONOSTICO_CADA_MIN  10                       // cada cuanto pregunta
#define REINTENTO_MS         (60 * 1000)              // si falla, cuanto espera para volver a probar
#define VIEJO_US             (2LL * 60 * 60 * 1000000) // un pronostico de mas de 2 horas ya no sirve
#define MAX_RESPUESTA        2048

static pronostico_t ultimo;
static int64_t cuando_us = 0;          // cuando llego el ultimo (0 = nunca)
static SemaphoreHandle_t candado;

bool pronostico_ultimo(pronostico_t *p)
{
    if (candado == NULL) return false;
    xSemaphoreTake(candado, portMAX_DELAY);
    bool hay = cuando_us != 0 && esp_timer_get_time() - cuando_us < VIEJO_US;
    if (hay && p != NULL) *p = ultimo;
    xSemaphoreGive(candado);
    return hay;
}

#ifdef FLUFI_HAY_SECRETOS_CLIMA

static char   respuesta[MAX_RESPUESTA];
static size_t largo_respuesta;

static esp_err_t al_recibir(esp_http_client_event_t *evento)
{
    if (evento->event_id == HTTP_EVENT_ON_DATA && largo_respuesta + evento->data_len < MAX_RESPUESTA) {
        memcpy(respuesta + largo_respuesta, evento->data, evento->data_len);
        largo_respuesta += evento->data_len;
    }
    return ESP_OK;
}

// Busca "nombre": dentro de "desde" y devuelve un puntero al numero que sigue
static const char *buscar_numero(const char *desde, const char *nombre)
{
    char clave[40];
    snprintf(clave, sizeof(clave), "\"%s\":", nombre);
    const char *p = strstr(desde, clave);
    return p ? p + strlen(clave) : NULL;
}

// Lee la parte "current" de la respuesta. Devuelve false si falta algo.
static bool leer_respuesta(const char *json, pronostico_t *p)
{
    const char *actual = strstr(json, "\"current\":{");   // (antes esta "current_units", con textos)
    if (actual == NULL) return false;
    const char *temp   = buscar_numero(actual, "temperature_2m");
    const char *codigo = buscar_numero(actual, "weather_code");
    const char *dia    = buscar_numero(actual, "is_day");
    if (temp == NULL || codigo == NULL || dia == NULL) return false;

    float grados  = strtof(temp, NULL);
    p->temp_decimas = (int16_t)(grados * 10 + (grados >= 0 ? 0.5f : -0.5f));
    p->codigo     = (int)strtol(codigo, NULL, 10);
    p->es_de_dia  = strtol(dia, NULL, 10) != 0;
    return true;
}

static bool consultar(void)
{
    char url[256];
    snprintf(url, sizeof(url),
             "https://api.open-meteo.com/v1/forecast?latitude=%s&longitude=%s"
             "&current=temperature_2m,weather_code,is_day&timezone=auto",
             CLIMA_LATITUD, CLIMA_LONGITUD);

    esp_http_client_config_t cfg = {
        .url               = url,
        .crt_bundle_attach = esp_crt_bundle_attach,   // conexion segura
        .timeout_ms        = 15000,
        .event_handler     = al_recibir,
    };
    esp_http_client_handle_t cliente = esp_http_client_init(&cfg);
    if (cliente == NULL) return false;

    largo_respuesta = 0;
    esp_err_t err = esp_http_client_perform(cliente);
    int codigo_http = esp_http_client_get_status_code(cliente);
    esp_http_client_cleanup(cliente);

    if (err != ESP_OK || codigo_http != 200) {
        ESP_LOGW(TAG, "No pude consultar el pronostico (%s, codigo %d)", esp_err_to_name(err), codigo_http);
        return false;
    }
    respuesta[largo_respuesta] = '\0';

    pronostico_t p;
    if (!leer_respuesta(respuesta, &p)) {
        ESP_LOGW(TAG, "No entendi la respuesta del pronostico");
        return false;
    }
    xSemaphoreTake(candado, portMAX_DELAY);
    ultimo = p;
    cuando_us = esp_timer_get_time();
    xSemaphoreGive(candado);

    ESP_LOGI(TAG, "Afuera: %s%d.%d C, codigo %d, %s", p.temp_decimas < 0 ? "-" : "",
             abs(p.temp_decimas) / 10, abs(p.temp_decimas) % 10, p.codigo, p.es_de_dia ? "de dia" : "de noche");
    return true;
}

static void tarea_pronostico(void *arg)
{
    while (1) {
        if (!wifi_conectado()) {
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }
        bool bien = consultar();
        vTaskDelay(pdMS_TO_TICKS(bien ? PRONOSTICO_CADA_MIN * 60 * 1000 : REINTENTO_MS));
    }
}

#endif // FLUFI_HAY_SECRETOS_CLIMA

bool pronostico_configurado(void)
{
#ifdef FLUFI_HAY_SECRETOS_CLIMA
    return true;
#else
    return false;
#endif
}

void pronostico_iniciar(void)
{
    candado = xSemaphoreCreateMutex();
#ifdef FLUFI_HAY_SECRETOS_CLIMA
    xTaskCreate(tarea_pronostico, "pronostico", 6144, NULL, 3, NULL);
#else
    ESP_LOGW(TAG, "No se donde estamos: sigo sin pronostico. Para usarlo, copia "
                  "clima_secretos.ejemplo.h como clima_secretos.h y completalo");
#endif
}
