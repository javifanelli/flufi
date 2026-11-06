#include "reloj.h"
#include "wifi.h"

#include <stdlib.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_netif_sntp.h"
#include "esp_log.h"

static const char *TAG = "RELOJ";

// Servidor de hora: un grupo de servidores publicos de todo el mundo
#define SERVIDOR_NTP "pool.ntp.org"

static volatile bool sincronizado = false;

// La llama el SNTP cada vez que pone (o corrige) la hora
static void al_sincronizar(struct timeval *tv)
{
    bool primera_vez = !sincronizado;
    sincronizado = true;

    if (primera_vez) {
        struct tm ahora;
        char texto[32];
        localtime_r(&tv->tv_sec, &ahora);
        strftime(texto, sizeof(texto), "%d/%m/%Y %H:%M:%S", &ahora);
        ESP_LOGI(TAG, "¡Ya se la hora! %s", texto);
    }
}

// Espera a que haya WiFi y recien ahi le pregunta la hora a internet
static void tarea_reloj(void *arg)
{
    while (!wifi_conectado()) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    ESP_LOGI(TAG, "Preguntando la hora a %s...", SERVIDOR_NTP);
    esp_sntp_config_t cfg = ESP_NETIF_SNTP_DEFAULT_CONFIG(SERVIDOR_NTP);
    cfg.sync_cb = al_sincronizar;
    esp_netif_sntp_init(&cfg);   // de aca en mas se corrige sola cada tanto

    vTaskDelete(NULL);           // esta tarea ya hizo su trabajo
}

void reloj_iniciar(const char *zona_horaria)
{
    // La zona horaria se configura con la variable de entorno TZ
    setenv("TZ", zona_horaria, 1);
    tzset();

    xTaskCreate(tarea_reloj, "reloj", 3072, NULL, 3, NULL);
}

bool reloj_sincronizado(void)
{
    return sincronizado;
}

bool reloj_hora(struct tm *ahora)
{
    if (!sincronizado) return false;
    time_t segundos = time(NULL);
    localtime_r(&segundos, ahora);
    return true;
}
