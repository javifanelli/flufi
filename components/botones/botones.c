#include "botones.h"
#include "board_config.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "BOTONES";

/*
 * Como se lee un boton:
 *   - Cada boton esta entre su pin y GND. Con el pull-up interno, el pin lee 1
 *     cuando el boton esta suelto y 0 cuando esta apretado (BTN_NIVEL_APRETADO).
 *   - Al apretar, el metal "rebota" y durante unos milisegundos el pin salta
 *     entre 0 y 1. Para no contar varias apretadas, solo creemos un cambio
 *     cuando se mantiene igual durante CAMBIO_FIRME lecturas seguidas.
 */
#define REVISAR_CADA_MS 10
#define CAMBIO_FIRME    3      // 3 lecturas iguales = 30 ms sin rebotar

static const gpio_num_t pines[CANT_BOTONES] = { PIN_BTN_A, PIN_BTN_B, PIN_BTN_BOOT };
static const char *const nombres[CANT_BOTONES] = { "A", "B", "BOOT" };

static volatile bool se_apreto[CANT_BOTONES];

static void tarea_botones(void *arg)
{
    bool apretado[CANT_BOTONES] = { false };    // lo que creemos que esta pasando
    int  lecturas_distintas[CANT_BOTONES] = { 0 };

    while (1) {
        for (int i = 0; i < CANT_BOTONES; i++) {
            bool ahora = gpio_get_level(pines[i]) == BTN_NIVEL_APRETADO;

            if (ahora == apretado[i]) {
                lecturas_distintas[i] = 0;          // nada cambio
            } else if (++lecturas_distintas[i] >= CAMBIO_FIRME) {
                apretado[i] = ahora;                // cambio de verdad (no es rebote)
                lecturas_distintas[i] = 0;
                if (ahora) {
                    ESP_LOGI(TAG, "¡Boton %s apretado!", nombres[i]);
                    se_apreto[i] = true;
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(REVISAR_CADA_MS));
    }
}

void botones_iniciar(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << PIN_BTN_A) | (1ULL << PIN_BTN_B) | (1ULL << PIN_BTN_BOOT),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,     // suelto = 1, apretado = 0
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&cfg));

    xTaskCreate(tarea_botones, "botones", 2560, NULL, 5, NULL);
    ESP_LOGI(TAG, "Botones listos (A = GPIO %d, B = GPIO %d, BOOT = GPIO %d)",
             PIN_BTN_A, PIN_BTN_B, PIN_BTN_BOOT);
}

bool botones_pendiente(boton_t boton)
{
    if ((unsigned)boton >= CANT_BOTONES) return false;
    return se_apreto[boton];
}

bool botones_se_apreto(boton_t boton)
{
    if ((unsigned)boton >= CANT_BOTONES) return false;
    if (!se_apreto[boton]) return false;
    se_apreto[boton] = false;
    return true;
}

void botones_olvidar(void)
{
    for (int i = 0; i < CANT_BOTONES; i++) se_apreto[i] = false;
}
