// ─────────────────────────────────────────────────────────────────────────────
//  Los dibujos de la comida y la bebida (a color)
// ─────────────────────────────────────────────────────────────────────────────
//
//  Cada dibujo se hace con las formas de siempre (rectangulos, circulos,
//  triangulos). Todas las medidas se multiplican por "s" (el tamano), asi el
//  mismo dibujo sirve grande o chiquito.
//
#include "comida.h"

#include <math.h>

#define PI ((float)M_PI)

static color_t fondo;     // el color de atras, para "borrar" pedacitos

// ─── Herramientas (aceptan numeros con coma) ─────────────────────────────────

static void rect(float x, float y, float w, float h, color_t c)
{
    if (w < 1) w = 1;
    if (h < 1) h = 1;
    display_fill_rect((int)x, (int)y, (int)w, (int)h, c);
}

static void rect_redondo(float x, float y, float w, float h, float r, color_t c)
{
    display_fill_round_rect((int)x, (int)y, (int)w, (int)h, (int)r, c);
}

static void circulo(float cx, float cy, float r, color_t c)
{
    display_fill_circle((int)cx, (int)cy, (int)(r + 0.5f), c);
}

static void triangulo(float x0, float y0, float x1, float y1, float x2, float y2, color_t c)
{
    display_fill_triangle((int)x0, (int)y0, (int)x1, (int)y1, (int)x2, (int)y2, c);
}

static void arco(float cx, float cy, float r, float desde, float hasta, color_t c)
{
    for (float a = desde; a <= hasta; a += 0.06f)
        rect(cx + r * cosf(a), cy + r * sinf(a), 1, 1, c);
}

// ─── Comidas ─────────────────────────────────────────────────────────────────

// 🎂 Torta: plato, bizcochuelo, crema rosa que chorrea, granitos de colores y una velita
static void dibujar_torta(float cx, float cy, float s)
{
    const color_t plato = COLOR(200, 200, 200), bizcochuelo = COLOR(230, 180, 110);
    const color_t relleno = COLOR(140, 70, 40), crema = COLOR(255, 160, 200);
    cy += 6 * s;

    rect(cx - 17 * s, cy + 10 * s, 34 * s, 2 * s, plato);
    rect_redondo(cx - 13 * s, cy - 4 * s, 26 * s, 14 * s, 2 * s, bizcochuelo);
    rect(cx - 13 * s, cy + 3 * s, 26 * s, 1.5f * s, relleno);
    rect_redondo(cx - 14 * s, cy - 9 * s, 28 * s, 7 * s, 3 * s, crema);
    circulo(cx - 9 * s, cy - 2 * s, 2 * s, crema);               // chorreados de crema
    circulo(cx - 1 * s, cy - 1 * s, 2.5f * s, crema);
    circulo(cx + 8 * s, cy - 2 * s, 2 * s, crema);

    rect(cx - 9 * s, cy - 7 * s, s, s, COLOR(255, 60, 60));      // granitos
    rect(cx - 3 * s, cy - 6 * s, s, s, COLOR(60, 200, 60));
    rect(cx + 4 * s, cy - 7 * s, s, s, COLOR(60, 120, 255));
    rect(cx + 10 * s, cy - 6 * s, s, s, COLOR(255, 220, 0));

    rect(cx - 1 * s, cy - 16 * s, 3 * s, 7 * s, BLANCO);                        // velita
    circulo(cx + 0.5f * s, cy - 19 * s, 2 * s, COLOR(255, 160, 0));             // llama
    triangulo(cx - 1.5f * s, cy - 19 * s, cx + 2.5f * s, cy - 19 * s, cx + 0.5f * s, cy - 24 * s,
              COLOR(255, 220, 0));
}

// 🍟 Papas fritas: cajita roja con una sonrisa y papas amarillas
static void dibujar_papas(float cx, float cy, float s)
{
    const color_t papa = COLOR(255, 210, 60), caja = COLOR(220, 30, 30);
    cy += 2.5f * s;

    static const int8_t alto_papa[7] = { 13, 16, 12, 17, 14, 15, 12 };
    for (int k = 0; k < 7; k++)
        rect(cx + (-9 + 3 * k) * s, cy - alto_papa[k] * s, 2 * s, (alto_papa[k] - 1) * s, papa);

    rect(cx - 9 * s, cy - 2 * s, 18 * s, 14 * s, caja);
    triangulo(cx - 12 * s, cy - 2 * s, cx - 9 * s, cy - 2 * s, cx - 9 * s, cy + 12 * s, caja);
    triangulo(cx + 12 * s, cy - 2 * s, cx + 9 * s, cy - 2 * s, cx + 9 * s, cy + 12 * s, caja);
    arco(cx, cy + 1 * s, 5 * s, 0.2f * PI, 0.8f * PI, papa);
}

// 🍔 Hamburguesa: pan con semillas, lechuga, carne y pan de abajo
static void dibujar_hamburguesa(float cx, float cy, float s)
{
    const color_t pan = COLOR(220, 140, 60), semilla = COLOR(255, 240, 200);
    const color_t lechuga = COLOR(80, 200, 60), carne = COLOR(110, 60, 30);

    circulo(cx, cy - 1 * s, 13 * s, pan);                        // pan de arriba (medio circulo)
    rect(cx - 14 * s, cy - 1 * s + 1, 28 * s, 15 * s, fondo);

    rect(cx - 6 * s, cy - 9 * s, s, 2 * s, semilla);
    rect(cx,         cy - 11 * s, s, 2 * s, semilla);
    rect(cx + 6 * s, cy - 8 * s, s, 2 * s, semilla);
    rect(cx - 2 * s, cy - 5 * s, s, 2 * s, semilla);
    rect(cx + 4 * s, cy - 4 * s, s, 2 * s, semilla);

    for (float dx = -14 * s; dx <= 14 * s; dx += 1)              // lechuga ondulada
        rect(cx + dx, cy + 1 * s + 1.0f * s * sinf(dx / s), 1, 1.5f * s, lechuga);

    rect_redondo(cx - 13 * s, cy + 4 * s, 26 * s, 4 * s, 2 * s, carne);
    rect_redondo(cx - 13 * s, cy + 9 * s, 26 * s, 5 * s, 2 * s, pan);
}

// ─── Bebidas ─────────────────────────────────────────────────────────────────

static float cuanto_queda(int sorbos)
{
    if (sorbos < 0) sorbos = 0;
    if (sorbos > 3) sorbos = 3;
    return (3 - sorbos) / 3.0f;
}

// 💧 Agua: botella con tapita y el agua adentro
static void dibujar_agua(float cx, float cy, float s, int sorbos)
{
    const color_t vidrio = COLOR(170, 220, 255), tapa = COLOR(40, 80, 220), agua = COLOR(60, 140, 255);
    cy += 1 * s;

    rect(cx - 4 * s, cy - 19 * s, 8 * s, 3 * s, tapa);
    rect(cx - 3 * s, cy - 15 * s, 6 * s, 8 * s, vidrio);
    rect(cx - 1.5f * s, cy - 15 * s, 3 * s, 7 * s, fondo);
    rect_redondo(cx - 8 * s, cy - 8 * s, 16 * s, 24 * s, 4 * s, vidrio);
    rect_redondo(cx - 6 * s, cy - 6 * s, 12 * s, 20 * s, 3 * s, fondo);

    float alto = 18 * s * cuanto_queda(sorbos);
    if (alto >= 1) rect(cx - 5 * s, cy + 13 * s - alto, 10 * s, alto, agua);
}

// ☕ Cafe: taza con asa, platito, el cafe adentro y vapor si esta llena
static void dibujar_cafe(float cx, float cy, float s, int sorbos)
{
    const color_t taza = BLANCO, cafe = COLOR(120, 70, 30), vapor = COLOR(180, 180, 180);
    cy += 2 * s;

    circulo(cx + 10 * s, cy + 1 * s, 6 * s, taza);
    circulo(cx + 10 * s, cy + 1 * s, 3.5f * s, fondo);
    rect_redondo(cx - 12 * s, cy - 8 * s, 20 * s, 19 * s, 3 * s, taza);
    rect(cx - 10 * s, cy - 8 * s, 16 * s, 17 * s, fondo);
    rect(cx - 16 * s, cy + 12 * s, 30 * s, 2 * s, taza);

    float alto = 14 * s * cuanto_queda(sorbos);
    if (alto >= 1) rect(cx - 9 * s, cy + 8 * s - alto, 14 * s, alto, cafe);

    if (sorbos == 0 && s >= 1) {
        for (float dy = 0; dy < 8 * s; dy += 1) {
            rect(cx - 5 * s + 1.5f * s * sinf(dy / s * 0.8f), cy - 11 * s - dy, 1, 1, vapor);
            rect(cx + 2 * s + 1.5f * s * sinf(dy / s * 0.8f + 1.5f), cy - 11 * s - dy, 1, 1, vapor);
        }
    }
}

// 🧉 Mate: calabacita, virola, yerba asomando y la bombilla
static void dibujar_mate(float cx, float cy, float s)
{
    const color_t calabaza = COLOR(160, 100, 50), metal = COLOR(200, 200, 210), yerba = COLOR(100, 170, 60);
    cy += 2.5f * s;

    circulo(cx, cy + 4 * s, 9.5f * s, calabaza);
    rect_redondo(cx - 7 * s, cy - 8 * s, 14 * s, 10 * s, 3 * s, calabaza);
    rect(cx - 7 * s, cy - 4 * s, 14 * s, 1.5f * s, metal);                       // virola
    rect(cx - 6 * s, cy + 14 * s, 12 * s, 2 * s, calabaza);
    rect_redondo(cx - 5.5f * s, cy - 8 * s, 11 * s, 3.5f * s, 1.5f * s, yerba);  // yerba

    for (float t = 0; t <= 1; t += 0.05f)                                         // bombilla
        rect(cx + 2 * s + 6 * s * t, cy - 7 * s - 11 * s * t, 1.5f * s, 1.5f * s, metal);
    rect(cx + 6 * s, cy - 19 * s, 5 * s, 1.5f * s, metal);
}

// ─── Publico ─────────────────────────────────────────────────────────────────

bool comida_es_bebida(comida_t c)
{
    return c == COMIDA_AGUA || c == COMIDA_CAFE || c == COMIDA_MATE;
}

void comida_dibujar(comida_t c, float cx, float cy, float s, int mordidas, color_t color_fondo)
{
    fondo = color_fondo;
    float medio_ancho = 13;

    switch (c) {
    case COMIDA_TORTA:       dibujar_torta(cx, cy, s);            break;
    case COMIDA_PAPAS:       dibujar_papas(cx, cy, s); medio_ancho = 11; break;
    case COMIDA_HAMBURGUESA: dibujar_hamburguesa(cx, cy, s);      break;
    case COMIDA_AGUA:        dibujar_agua(cx, cy, s, mordidas);   return;   // en las bebidas,
    case COMIDA_CAFE:        dibujar_cafe(cx, cy, s, mordidas);   return;   // "mordidas" son
    case COMIDA_MATE:        dibujar_mate(cx, cy, s);             return;   // los sorbos
    default:                 return;
    }

    // Mordiscos: "bocados" del color del fondo, del lado que da a la boca
    float borde = cx - medio_ancho * s;
    if (mordidas >= 1) circulo(borde, cy - 3 * s, 6 * s, fondo);
    if (mordidas >= 2) circulo(borde + 2 * s, cy + 6 * s, 7 * s, fondo);
}
