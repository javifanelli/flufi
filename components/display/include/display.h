#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdbool.h>
#include <stdint.h>

// Pantalla TFT a color de 128x128 (ST7735).
// Se dibuja en una "hoja" en memoria y despues display_flush() la manda entera
// a la pantalla.

// ─── Colores ─────────────────────────────────────────────────────────────────
// Cada color es un numero de 16 bits (RGB565): 5 bits de rojo, 6 de verde y 5 de
// azul. COLOR(r, g, b) lo arma a partir de rojo, verde y azul de 0 a 255.
typedef uint16_t color_t;
#define COLOR(r, g, b)  ((color_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)))

#define NEGRO           COLOR(0, 0, 0)
#define BLANCO          COLOR(255, 255, 255)

// ─── Funciones ───────────────────────────────────────────────────────────────
void display_init(void);
void display_clear(color_t color);    // pinta toda la hoja de un color
void display_flush(void);             // manda la hoja a la pantalla

void display_set_pixel(int x, int y, color_t color);
void display_fill_rect(int x, int y, int w, int h, color_t color);
// Rectangulo relleno con esquinas redondeadas de radio r
void display_fill_round_rect(int x, int y, int w, int h, int r, color_t color);
// Circulo relleno con centro (cx, cy) y radio r
void display_fill_circle(int cx, int cy, int r, color_t color);
// Triangulo relleno con esquinas en (x0,y0), (x1,y1), (x2,y2)
void display_fill_triangle(int x0, int y0, int x1, int y1, int x2, int y2, color_t color);
// Linea recta de (x0,y0) a (x1,y1), con trazo de "grosor" pixeles
void display_draw_line(int x0, int y0, int x1, int y1, int grosor, color_t color);

// ─── Texto ───────────────────────────────────────────────────────────────────
// Letras de 5x7 pixeles. "escala" las agranda: 1 = chiquitas, 2 = el doble...
// Acepta '\n' para cambiar de renglon y los signos ¡ ¿ °.

// Escribe el texto con la esquina de arriba a la izquierda en (x, y)
void display_draw_text(int x, int y, const char *texto, int escala, color_t color);
// Escribe el texto centrado de izquierda a derecha, empezando a la altura y.
// Con y = -1 tambien lo centra de arriba a abajo.
void display_draw_text_centered(const char *texto, int y, int escala, color_t color);
// Igual, pero cada renglon se centra alrededor de "centro_x" (y = -1: centrado de arriba a abajo)
void display_draw_text_centered_at(const char *texto, int centro_x, int y, int escala, color_t color);
// Ancho del renglon mas largo y alto total del texto, en pixeles
int  display_text_width(const char *texto, int escala);
int  display_text_height(const char *texto, int escala);

#endif // DISPLAY_H
