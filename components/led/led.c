#include "led.h"
#include "board_config.h"

#include "led_strip.h"
#include "esp_log.h"

static const char *TAG = "LED";

static led_strip_handle_t tira = NULL;

void led_iniciar(void)
{
    led_strip_config_t config = {
        .strip_gpio_num = PIN_LED_RGB,
        .max_leds = 1,                                          // un solo LED
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags.invert_out = false,
    };
    led_strip_rmt_config_t rmt = {
        .resolution_hz = 10 * 1000 * 1000,
        .flags.with_dma = false,
    };

    if (led_strip_new_rmt_device(&config, &rmt, &tira) != ESP_OK) {
        ESP_LOGW(TAG, "No se pudo preparar el LED");
        tira = NULL;
        return;
    }
    led_apagar();
    ESP_LOGI(TAG, "LED apagado");
}

void led_apagar(void)
{
    if (tira != NULL) led_strip_clear(tira);
}

void led_color(uint8_t rojo, uint8_t verde, uint8_t azul)
{
    if (tira == NULL) return;
    led_strip_set_pixel(tira, 0, rojo, verde, azul);
    led_strip_refresh(tira);
}
