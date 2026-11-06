#include "led.h"
#include "board_config.h"

#include "led_strip.h"
#include "esp_log.h"

static const char *TAG = "LED";

static led_strip_handle_t tira = NULL;
static uint8_t brillo_pct = 25;

void led_iniciar(uint8_t brillo)
{
    brillo_pct = (brillo > 100) ? 100 : brillo;

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
    ESP_LOGI(TAG, "LED listo (brillo %d %%)", brillo_pct);
}

void led_apagar(void)
{
    if (tira != NULL) led_strip_clear(tira);
}

void led_color(uint8_t rojo, uint8_t verde, uint8_t azul)
{
    if (tira == NULL) return;
    // Bajamos el brillo para que no encandile
    led_strip_set_pixel(tira, 0, rojo * brillo_pct / 100, verde * brillo_pct / 100, azul * brillo_pct / 100);
    led_strip_refresh(tira);
}

// El arco iris recorre la "rueda de colores": rojo → amarillo → verde → celeste
// → azul → violeta → rojo. "tono" dice en que parte de la rueda estamos (0 a 359).
void led_arcoiris_paso(void)
{
    static int tono = 0;
    tono = (tono + 30) % 360;

    int parte = tono / 60;                    // 6 tramos de 60 grados
    int sube  = (tono % 60) * 255 / 60;       // dentro del tramo: de 0 a 255
    int baja  = 255 - sube;
    uint8_t r = 0, g = 0, b = 0;
    switch (parte) {
    case 0: r = 255;  g = sube; b = 0;    break;   // rojo → amarillo
    case 1: r = baja; g = 255;  b = 0;    break;   // amarillo → verde
    case 2: r = 0;    g = 255;  b = sube; break;   // verde → celeste
    case 3: r = 0;    g = baja; b = 255;  break;   // celeste → azul
    case 4: r = sube; g = 0;    b = 255;  break;   // azul → violeta
    default: r = 255; g = 0;    b = baja; break;   // violeta → rojo
    }
    led_color(r, g, b);
}
