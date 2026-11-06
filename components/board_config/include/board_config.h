#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

/*
 * ═════════════════════════════════════════════════════════════════════════════
 *  FLUFI — Conexiones de todos los perifericos
 * ═════════════════════════════════════════════════════════════════════════════
 *
 *  Es el mismo hardware del proyecto color1.44.
 *  Placa: ESP32-C3 DevKit (1 nucleo RISC-V, 4 MB de flash, WiFi y Bluetooth)
 *
 *  ┌──────────────────────┬────────────┬────────────┬─────────────────────────┐
 *  │ Periferico           │ Pata       │ ESP32-C3   │ Notas                   │
 *  ├──────────────────────┼────────────┼────────────┼─────────────────────────┤
 *  │ Pantalla TFT 1.44"   │ SCL (SCLK) │ GPIO 5     │ ST7735, 128x128, color  │
 *  │ (SPI)                │ SDA (MOSI) │ GPIO 6     │                         │
 *  │                      │ CS         │ GPIO 4     │                         │
 *  │                      │ DC (RS)    │ GPIO 7     │                         │
 *  │                      │ RES (RST)  │ GPIO 3     │                         │
 *  │                      │ LEDA       │ 3V3        │ Luz de fondo, directa   │
 *  ├──────────────────────┼────────────┼────────────┼─────────────────────────┤
 *  │ Sensor BMP280 (I2C)  │ SDA        │ GPIO 1     │ Temperatura y presion   │
 *  │                      │ SCL        │ GPIO 0     │ Direccion 0x76 o 0x77   │
 *  ├──────────────────────┼────────────┼────────────┼─────────────────────────┤
 *  │ Boton A              │            │ GPIO 2     │ A GND, pull-up interno  │
 *  │ Boton B              │            │ GPIO 10    │ A GND, pull-up interno  │
 *  │ Boton BOOT (placa)   │            │ GPIO 9     │ Ya viene en la placa    │
 *  ├──────────────────────┼────────────┼────────────┼─────────────────────────┤
 *  │ LED RGB (placa)      │            │ GPIO 8     │ WS2812 (un solo LED)    │
 *  └──────────────────────┴────────────┴────────────┴─────────────────────────┘
 *
 *  Pines de arranque del ESP32-C3: 2, 8 y 9. Ya estan usados por los botones y
 *  el LED, y funcionan bien asi (no conectarles otras cosas).
 */

#include "driver/gpio.h"

// ─── Pantalla TFT ST7735 (SPI) ────────────────────────────────────────────────
#define PIN_LCD_SCLK      GPIO_NUM_5
#define PIN_LCD_MOSI      GPIO_NUM_6
#define PIN_LCD_CS        GPIO_NUM_4
#define PIN_LCD_DC        GPIO_NUM_7
#define PIN_LCD_RST       GPIO_NUM_3
#define LCD_SPI_HOST      SPI2_HOST
#define LCD_SPI_HZ        (26 * 1000 * 1000)
#define LCD_H_RES         128
#define LCD_V_RES         128

// La pantalla esta montada dada vuelta. Segun la orientacion, el ST7735 necesita
// un pequeño corrimiento (su memoria es un poco mas grande que la pantalla).
#define DISPLAY_ROTAR_180 1
#if DISPLAY_ROTAR_180
#define LCD_MADCTL        0xC8     // espejado en X e Y, colores en orden BGR
#define LCD_OFFSET_X      2
#define LCD_OFFSET_Y      3
#else
#define LCD_MADCTL        0x08     // colores en orden BGR
#define LCD_OFFSET_X      2
#define LCD_OFFSET_Y      1
#endif

// ─── Sensor BMP280 (I2C) ──────────────────────────────────────────────────────
#define PIN_I2C_SDA       GPIO_NUM_1
#define PIN_I2C_SCL       GPIO_NUM_0

// ─── Botones ──────────────────────────────────────────────────────────────────
#define PIN_BTN_A         GPIO_NUM_2
#define PIN_BTN_B         GPIO_NUM_10
#define PIN_BTN_BOOT      GPIO_NUM_9
#define BTN_NIVEL_APRETADO 0          // con pull-up: apretado = 0

// ─── LED RGB de la placa ──────────────────────────────────────────────────────
#define PIN_LED_RGB       GPIO_NUM_8

#endif // BOARD_CONFIG_H
