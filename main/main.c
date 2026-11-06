#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"

#include "flufi_config.h"
#include "display.h"
#include "eyes.h"
#include "led.h"
#include "wifi.h"
#include "reloj.h"

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

// "Tengo wifi!" abajo de los ojos (solo la primera vez que se conecta)
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

// Espera (con las ondas abajo de los ojos) hasta que "listo" sea true o pase el tiempo
static bool esperar_con_ondas(bool (*listo)(void), int max_ms)
{
    int64_t inicio = esp_timer_get_time();
    while (!listo() && esp_timer_get_time() - inicio < (int64_t)max_ms * 1000) {
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

void app_main(void)
{
    ESP_LOGI(TAG, "¡Hola! Soy Flufi :)");

    led_iniciar(LED_BRILLO);           // el LED de la placa (arranca apagado)
    display_init();
    wifi_iniciar();                    // se conecta "de fondo"
    reloj_iniciar(ZONA_HORARIA);       // cuando haya WiFi, pone la hora

    // Saludo inicial
    mostrar_mensaje(MENSAJE_ARRANQUE);
    mostrar_mensaje(MENSAJE_ARRANQUE_2);

    eyes_init(COLOR_OJOS, COLOR_PESTANAS, COLOR_BOCA, COLOR_TEXTO, COLOR_FONDO);
    conectarse_y_saludar();
    eyes_saludar();                    // ¡con el LED en arco iris!

    while (1) {
        // Si el WiFi o la hora llegaron tarde, los festeja ahora
        if (!ya_festejo_wifi && wifi_conectado()) festejar_wifi();
        if (ya_festejo_wifi && !ya_saludo_hora && reloj_sincronizado()) saludar_segun_la_hora();

        // Juega con sus emociones (cada una prende el LED de un color)
        eyes_jugar();

        // Un descansito entre juego y juego, y a veces un parpadeo
        vTaskDelay(pdMS_TO_TICKS(DESCANSO_MIN_MS + esp_random() % (DESCANSO_MAX_MS - DESCANSO_MIN_MS)));
        if (esp_random() % 2) eyes_parpadear();
    }
}
