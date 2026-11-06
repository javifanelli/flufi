#include "wifi.h"

#include <string.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "esp_log.h"

// Los datos de la red viven en un archivo aparte que NO se sube al repo.
// Si no esta, compilamos igual con datos vacios y Flufi sigue sin WiFi.
// (FLUFI_HAY_SECRETOS_WIFI lo define el CMakeLists.txt cuando encuentra el archivo)
#if defined(FLUFI_HAY_SECRETOS_WIFI)
#include "wifi_secretos.h"
#else
#define WIFI_NOMBRE_RED  ""
#define WIFI_CONTRASENA  ""
#endif

static const char *TAG = "WIFI";

// Cuanto espera antes de reintentar: arranca en 1 s y se duplica hasta 1 minuto
#define ESPERA_MIN_MS  1000
#define ESPERA_MAX_MS  60000

static volatile bool    conectado = false;
static uint32_t         espera_ms = ESPERA_MIN_MS;
static esp_timer_handle_t timer_reintento;

// Explica en castellano por que se corto (los codigos vienen de esp_wifi_types.h)
static const char *motivo_en_castellano(uint8_t motivo)
{
    switch (motivo) {
    case WIFI_REASON_NO_AP_FOUND:            return "no encuentro la red (¿esta prendido el router? ¿el nombre esta bien?)";
    case WIFI_REASON_AUTH_FAIL:
    case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
    case WIFI_REASON_HANDSHAKE_TIMEOUT:      return "la red no me deja entrar (¿la contraseña esta bien?)";
    case WIFI_REASON_BEACON_TIMEOUT:         return "deje de escuchar a la red (¿estoy muy lejos del router?)";
    case WIFI_REASON_ASSOC_LEAVE:            return "me desconecte";
    default:                                 return "se corto la conexion";
    }
}

static void reintentar(void *arg)
{
    esp_wifi_connect();
}

// Aca llegan los "avisos" del WiFi: arranco, se conecto, se corto...
static void al_recibir_aviso(void *arg, esp_event_base_t tipo, int32_t aviso, void *datos)
{
    if (tipo == WIFI_EVENT && aviso == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Buscando la red...");
        esp_wifi_connect();

    } else if (tipo == WIFI_EVENT && aviso == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t *info = datos;
        conectado = false;
        ESP_LOGW(TAG, "Sin conexion: %s (codigo %d). Reintento en %lu s",
                 motivo_en_castellano(info->reason), info->reason, (unsigned long)(espera_ms / 1000));
        esp_timer_start_once(timer_reintento, (uint64_t)espera_ms * 1000);
        espera_ms *= 2;
        if (espera_ms > ESPERA_MAX_MS) espera_ms = ESPERA_MAX_MS;

    } else if (tipo == IP_EVENT && aviso == IP_EVENT_STA_GOT_IP) {
        // No mostramos el nombre de la red: los logs a veces se comparten
        conectado = true;
        espera_ms = ESPERA_MIN_MS;
        ESP_LOGI(TAG, "¡Conectado a la red WiFi!");
    }
}

// Copia un texto a un arreglo de bytes de tamaño fijo (el SSID puede ocupar
// los 32 bytes justos, sin el 0 del final, por eso no usamos strcpy)
static void copiar(uint8_t *destino, size_t tamano, const char *origen)
{
    size_t n = strlen(origen);
    if (n > tamano) n = tamano;
    memcpy(destino, origen, n);
}

void wifi_iniciar(void)
{
    if (strlen(WIFI_NOMBRE_RED) == 0) {
        ESP_LOGW(TAG, "No hay datos de WiFi: sigo sin internet. Para conectarme, copia "
                      "components/wifi/wifi_secretos.ejemplo.h como wifi_secretos.h y completalo");
        return;
    }

    // El WiFi guarda calibraciones en la memoria "NVS": hay que prepararla
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    // Los datos de la red quedan solo en RAM (no se graban en la memoria flash)
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, al_recibir_aviso, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, al_recibir_aviso, NULL));

    const esp_timer_create_args_t timer_cfg = { .callback = reintentar, .name = "wifi_reintento" };
    ESP_ERROR_CHECK(esp_timer_create(&timer_cfg, &timer_reintento));

    wifi_config_t red = { 0 };
    copiar(red.sta.ssid,     sizeof(red.sta.ssid),     WIFI_NOMBRE_RED);
    copiar(red.sta.password, sizeof(red.sta.password), WIFI_CONTRASENA);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));   // "estacion": se conecta a un router
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &red));
    ESP_ERROR_CHECK(esp_wifi_start());
}

bool wifi_conectado(void)
{
    return conectado;
}

bool wifi_configurado(void)
{
    return strlen(WIFI_NOMBRE_RED) > 0;
}
