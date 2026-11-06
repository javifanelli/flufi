#include "display.h"
#include "board_config.h"
#include "font5x7.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

static const char *TAG = "DISPLAY";

/*
 * Como funciona la pantalla ST7735:
 *   - Se le habla por SPI (un cable de datos y uno de reloj).
 *   - El cable DC dice si lo que mandamos es un COMANDO (DC = 0) o DATOS (DC = 1).
 *   - Cada pixel son 2 bytes de color (RGB565), y la pantalla los quiere con el
 *     byte "grande" primero. Por eso en la hoja los guardamos ya dados vuelta.
 */

static spi_device_handle_t spi;
static uint16_t *hoja;    // LCD_H_RES x LCD_V_RES pixeles, listos para mandar

// ─── Hablar con la pantalla ──────────────────────────────────────────────────

static void enviar(const uint8_t *bytes, size_t cantidad, bool son_datos)
{
    if (cantidad == 0) return;
    gpio_set_level(PIN_LCD_DC, son_datos ? 1 : 0);

    spi_transaction_t t = { .length = cantidad * 8 };
    if (cantidad <= 4) {                 // poquitos bytes: van adentro de la transaccion
        t.flags = SPI_TRANS_USE_TXDATA;
        memcpy(t.tx_data, bytes, cantidad);
    } else {
        t.tx_buffer = bytes;
    }
    ESP_ERROR_CHECK(spi_device_polling_transmit(spi, &t));
}

static void comando(uint8_t cmd)
{
    enviar(&cmd, 1, false);
}

static void comando_con_datos(uint8_t cmd, const uint8_t *datos, size_t cantidad)
{
    comando(cmd);
    enviar(datos, cantidad, true);
}

// Elige el rectangulo de la pantalla donde van a ir los proximos pixeles
static void elegir_ventana(int x0, int y0, int x1, int y1)
{
    uint8_t columnas[4] = { 0, (uint8_t)(x0 + LCD_OFFSET_X), 0, (uint8_t)(x1 + LCD_OFFSET_X) };
    uint8_t filas[4]    = { 0, (uint8_t)(y0 + LCD_OFFSET_Y), 0, (uint8_t)(y1 + LCD_OFFSET_Y) };
    comando_con_datos(0x2A, columnas, 4);    // CASET: columnas
    comando_con_datos(0x2B, filas, 4);       // RASET: filas
}

// ─── Arranque ────────────────────────────────────────────────────────────────

void display_init(void)
{
    ESP_LOGI(TAG, "Inicializando la pantalla ST7735...");

    hoja = heap_caps_malloc(LCD_H_RES * LCD_V_RES * sizeof(uint16_t), MALLOC_CAP_DMA);
    assert(hoja != NULL);

    gpio_config_t pines = {
        .pin_bit_mask = (1ULL << PIN_LCD_DC) | (1ULL << PIN_LCD_RST),
        .mode         = GPIO_MODE_OUTPUT,
    };
    ESP_ERROR_CHECK(gpio_config(&pines));

    spi_bus_config_t bus = {
        .mosi_io_num     = PIN_LCD_MOSI,
        .miso_io_num     = -1,
        .sclk_io_num     = PIN_LCD_SCLK,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = LCD_H_RES * LCD_V_RES * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_SPI_HOST, &bus, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t dispositivo = {
        .clock_speed_hz = LCD_SPI_HZ,
        .mode           = 0,
        .spics_io_num   = PIN_LCD_CS,
        .queue_size     = 1,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(LCD_SPI_HOST, &dispositivo, &spi));

    // Reset de la pantalla: el pin RST a 0 un ratito y despues a 1
    gpio_set_level(PIN_LCD_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(PIN_LCD_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(100));

    comando(0x01);                              // SWRESET: reset por software
    vTaskDelay(pdMS_TO_TICKS(150));
    comando(0x11);                              // SLPOUT: despertate
    vTaskDelay(pdMS_TO_TICKS(150));
    uint8_t colores = 0x05;
    comando_con_datos(0x3A, &colores, 1);       // COLMOD: 16 bits por pixel
    uint8_t orientacion = LCD_MADCTL;
    comando_con_datos(0x36, &orientacion, 1);   // MADCTL: orientacion y orden de colores
    comando(0x20);                              // INVOFF: colores sin invertir
    comando(0x29);                              // DISPON: prendete
    vTaskDelay(pdMS_TO_TICKS(50));

    display_clear(NEGRO);
    display_flush();
    ESP_LOGI(TAG, "Pantalla lista");
}

// ─── La hoja ─────────────────────────────────────────────────────────────────

// La pantalla quiere el byte grande primero: lo damos vuelta al guardarlo
static inline uint16_t dar_vuelta(color_t c)
{
    return (uint16_t)((c >> 8) | (c << 8));
}

void display_clear(color_t color)
{
    uint16_t v = dar_vuelta(color);
    for (int i = 0; i < LCD_H_RES * LCD_V_RES; i++) hoja[i] = v;
}

void display_flush(void)
{
    elegir_ventana(0, 0, LCD_H_RES - 1, LCD_V_RES - 1);
    comando(0x2C);                              // RAMWR: ahi van los pixeles
    enviar((const uint8_t *)hoja, LCD_H_RES * LCD_V_RES * sizeof(uint16_t), true);
}

// ─── Formas ──────────────────────────────────────────────────────────────────

void display_set_pixel(int x, int y, color_t color)
{
    if (x < 0 || x >= LCD_H_RES || y < 0 || y >= LCD_V_RES) return;
    hoja[y * LCD_H_RES + x] = dar_vuelta(color);
}

void display_fill_rect(int x, int y, int w, int h, color_t color)
{
    for (int j = y; j < y + h; j++)
        for (int i = x; i < x + w; i++)
            display_set_pixel(i, j, color);
}

void display_fill_round_rect(int x, int y, int w, int h, int r, color_t color)
{
    if (w <= 0 || h <= 0) return;
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;

    for (int j = 0; j < h; j++) {
        // Distancia vertical al centro de la esquina mas cercana
        int dy = 0;
        if (j < r)           dy = r - j;
        else if (j >= h - r) dy = j - (h - r - 1);

        // Cuanto "se mete" la fila por la curva de la esquina
        int adentro = 0;
        if (dy > 0) {
            int dx = 0;
            while ((dx + 1) * (dx + 1) + dy * dy <= r * r) dx++;
            adentro = r - dx;
        }
        for (int i = x + adentro; i < x + w - adentro; i++)
            display_set_pixel(i, y + j, color);
    }
}

void display_fill_circle(int cx, int cy, int r, color_t color)
{
    for (int j = -r; j <= r; j++)
        for (int i = -r; i <= r; i++)
            if (i * i + j * j <= r * r)
                display_set_pixel(cx + i, cy + j, color);
}

// Dice de que lado de la linea (a→b) queda el punto p
static int lado_de_linea(int ax, int ay, int bx, int by, int px, int py)
{
    return (bx - ax) * (py - ay) - (by - ay) * (px - ax);
}

void display_fill_triangle(int x0, int y0, int x1, int y1, int x2, int y2, color_t color)
{
    int min_x = x0 < x1 ? (x0 < x2 ? x0 : x2) : (x1 < x2 ? x1 : x2);
    int max_x = x0 > x1 ? (x0 > x2 ? x0 : x2) : (x1 > x2 ? x1 : x2);
    int min_y = y0 < y1 ? (y0 < y2 ? y0 : y2) : (y1 < y2 ? y1 : y2);
    int max_y = y0 > y1 ? (y0 > y2 ? y0 : y2) : (y1 > y2 ? y1 : y2);

    // Un punto esta adentro si queda del mismo lado de las tres lineas
    for (int y = min_y; y <= max_y; y++) {
        for (int x = min_x; x <= max_x; x++) {
            int a = lado_de_linea(x0, y0, x1, y1, x, y);
            int b = lado_de_linea(x1, y1, x2, y2, x, y);
            int c = lado_de_linea(x2, y2, x0, y0, x, y);
            if ((a >= 0 && b >= 0 && c >= 0) || (a <= 0 && b <= 0 && c <= 0))
                display_set_pixel(x, y, color);
        }
    }
}

void display_draw_line(int x0, int y0, int x1, int y1, int grosor, color_t color)
{
    int dx = x1 - x0, dy = y1 - y0;
    int pasos = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
    if (pasos == 0) pasos = 1;
    for (int i = 0; i <= pasos; i++) {
        int x = x0 + dx * i / pasos;
        int y = y0 + dy * i / pasos;
        display_fill_rect(x - grosor / 2, y - grosor / 2, grosor, grosor, color);
    }
}

// ─── Texto ───────────────────────────────────────────────────────────────────

#define SEPARACION_LETRAS  1   // columnas vacias entre letra y letra
#define SEPARACION_RENGLON 2   // filas vacias entre renglon y renglon

// Caracteres especiales (fuera del ASCII)
#define GLIFO_ABRE_EXCLAMACION 1   // ¡
#define GLIFO_ABRE_PREGUNTA    2   // ¿
#define GLIFO_GRADO            3   // °

static const uint8_t columnas_grado[FONT_ANCHO] = { 0x06, 0x09, 0x09, 0x06, 0x00 };

// Lee el proximo caracter del texto (entiende UTF-8 para ¡ ¿ °) y avanza el puntero
static int proximo_caracter(const char **p)
{
    const unsigned char *s = (const unsigned char *)*p;
    if (*s == 0) return 0;
    if (*s < 0x80) { (*p)++; return *s; }
    if (s[0] == 0xC2 && s[1] == 0xA1) { *p += 2; return GLIFO_ABRE_EXCLAMACION; }
    if (s[0] == 0xC2 && s[1] == 0xBF) { *p += 2; return GLIFO_ABRE_PREGUNTA; }
    if (s[0] == 0xC2 && s[1] == 0xB0) { *p += 2; return GLIFO_GRADO; }

    // Otro caracter UTF-8 que no tenemos: lo salteamos entero y dibujamos '?'
    (*p)++;
    while ((**p & 0xC0) == 0x80) (*p)++;
    return '?';
}

// Las 5 columnas de un caracter (¡ y ¿ son ! y ? dados vuelta)
static void columnas_de(int c, uint8_t col[FONT_ANCHO])
{
    if (c == GLIFO_GRADO) { memcpy(col, columnas_grado, FONT_ANCHO); return; }

    bool dar_vuelta_letra = false;
    if (c == GLIFO_ABRE_EXCLAMACION) { c = '!'; dar_vuelta_letra = true; }
    if (c == GLIFO_ABRE_PREGUNTA)    { c = '?'; dar_vuelta_letra = true; }
    if (c < FONT_PRIMERO || c > FONT_ULTIMO) c = '?';

    for (int i = 0; i < FONT_ANCHO; i++) {
        uint8_t b = font5x7[c - FONT_PRIMERO][i];
        if (dar_vuelta_letra) {
            uint8_t r = 0;
            for (int k = 0; k < FONT_ALTO; k++)
                if (b & (1 << k)) r |= 1 << (FONT_ALTO - 1 - k);
            b = r;
        }
        col[i] = b;
    }
}

static void dibujar_caracter(int x, int y, int c, int escala, color_t color)
{
    uint8_t col[FONT_ANCHO];
    columnas_de(c, col);
    for (int i = 0; i < FONT_ANCHO; i++)
        for (int k = 0; k < FONT_ALTO; k++)
            if (col[i] & (1 << k))
                display_fill_rect(x + i * escala, y + k * escala, escala, escala, color);
}

// Ancho del renglon que empieza en p (hasta '\n' o el final)
static int ancho_renglon(const char *p, int escala)
{
    int letras = 0, c;
    while ((c = proximo_caracter(&p)) != 0 && c != '\n') letras++;
    if (letras == 0) return 0;
    return (letras * (FONT_ANCHO + SEPARACION_LETRAS) - SEPARACION_LETRAS) * escala;
}

int display_text_width(const char *texto, int escala)
{
    int max = 0;
    const char *p = texto;
    while (1) {
        int w = ancho_renglon(p, escala);
        if (w > max) max = w;
        const char *salto = strchr(p, '\n');
        if (!salto) break;
        p = salto + 1;
    }
    return max;
}

int display_text_height(const char *texto, int escala)
{
    int renglones = 1;
    for (const char *p = texto; *p; p++)
        if (*p == '\n') renglones++;
    return (renglones * (FONT_ALTO + SEPARACION_RENGLON) - SEPARACION_RENGLON) * escala;
}

static void dibujar_texto(int x, int y, const char *texto, int escala, color_t color, bool centrado)
{
    const char *p = texto;
    int alto_renglon = (FONT_ALTO + SEPARACION_RENGLON) * escala;
    int cx = centrado ? (LCD_H_RES - ancho_renglon(p, escala)) / 2 : x;
    int c;

    while ((c = proximo_caracter(&p)) != 0) {
        if (c == '\n') {
            y += alto_renglon;
            cx = centrado ? (LCD_H_RES - ancho_renglon(p, escala)) / 2 : x;
            continue;
        }
        dibujar_caracter(cx, y, c, escala, color);
        cx += (FONT_ANCHO + SEPARACION_LETRAS) * escala;
    }
}

void display_draw_text(int x, int y, const char *texto, int escala, color_t color)
{
    dibujar_texto(x, y, texto, escala, color, false);
}

void display_draw_text_centered(const char *texto, int y, int escala, color_t color)
{
    if (y < 0) y = (LCD_V_RES - display_text_height(texto, escala)) / 2;
    dibujar_texto(0, y, texto, escala, color, true);
}
