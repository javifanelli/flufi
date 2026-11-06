// ─────────────────────────────────────────────────────────────────────────────
//  Los dibujos de la comida y la bebida (a color)
// ─────────────────────────────────────────────────────────────────────────────
//
//  Cada dibujo se hace con las formas de siempre (rectangulos, circulos,
//  triangulos). Todas las medidas se multiplican por "s" (el tamano), asi el
//  mismo dibujo sirve grande (en el menu) o chiquito (cuando come).
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

// Una rayita gruesa de (x0, y0) a (x1, y1)
static void palito(float x0, float y0, float x1, float y1, float grosor, color_t c)
{
    for (float t = 0; t <= 1; t += 0.05f)
        rect(x0 + (x1 - x0) * t, y0 + (y1 - y0) * t, grosor, grosor, c);
}

// ─── Comidas ─────────────────────────────────────────────────────────────────

// 🥐 Medialuna: pedacitos dorados que se van achicando hacia las puntas
static void dibujar_medialuna(float cx, float cy, float s)
{
    const color_t borde = COLOR(170, 95, 30), masa = COLOR(235, 165, 60), brillo = COLOR(255, 215, 130);
    static const float x[4]  = { 17, 12, 6, 0 };     // de la punta al centro
    static const float dy[4] = { 7, 3, 0, -2 };
    static const float r[4]  = { 3, 4.5f, 6, 7 };

    for (int k = 0; k < 4; k++) {
        for (int lado = -1; lado <= 1; lado += 2) {
            float px = cx + lado * x[k] * s, py = cy + dy[k] * s;
            circulo(px, py, (r[k] + 1) * s, borde);      // el borde marca cada pedacito
            circulo(px, py, r[k] * s, masa);
            rect(px - s, py - r[k] * 0.6f * s, 2 * s, s, brillo);
            if (k == 3) break;                         // el del centro va una sola vez
        }
    }
}

// 🧁 Muffin: pirotin rosa a rayas, copete de chocolate con chispitas y una cereza
static void dibujar_muffin(float cx, float cy, float s)
{
    const color_t papel = COLOR(255, 130, 180), raya = COLOR(220, 80, 140), masa = COLOR(160, 90, 40);
    const color_t chispa = COLOR(70, 35, 15), cereza = COLOR(230, 20, 40), cabito = COLOR(60, 160, 50);
    cy += 3 * s;

    circulo(cx, cy - 4 * s, 11 * s, masa);                                  // copete
    rect(cx - 9 * s, cy, 18 * s, 13 * s, papel);                            // pirotin
    triangulo(cx - 12 * s, cy, cx - 9 * s, cy, cx - 9 * s, cy + 13 * s, papel);
    triangulo(cx + 12 * s, cy, cx + 9 * s, cy, cx + 9 * s, cy + 13 * s, papel);
    for (int k = -2; k <= 2; k++)
        rect(cx + k * 3.5f * s, cy + 2 * s, s, 10 * s, raya);
    rect_redondo(cx - 13 * s, cy - 4 * s, 26 * s, 6 * s, 3 * s, masa);     // el borde que se desborda

    rect(cx - 6 * s, cy - 10 * s, 2 * s, 1.5f * s, chispa);                 // chispitas
    rect(cx + 3 * s, cy - 12 * s, 2 * s, 1.5f * s, chispa);
    rect(cx + 7 * s, cy - 6 * s,  2 * s, 1.5f * s, chispa);
    rect(cx - 2 * s, cy - 6 * s,  2 * s, 1.5f * s, chispa);
    rect(cx - 10 * s, cy - 3 * s, 2 * s, 1.5f * s, chispa);

    palito(cx + 1 * s, cy - 17 * s, cx + 4 * s, cy - 22 * s, s, cabito);    // cereza
    circulo(cx + 1 * s, cy - 15 * s, 3 * s, cereza);
    rect(cx, cy - 17 * s, s, s, BLANCO);
}

// 🍩 Dona: masa dorada, glaseado rosa y granitos de colores (vista de arriba)
static void dibujar_dona(float cx, float cy, float s)
{
    const color_t masa = COLOR(220, 160, 90), glaseado = COLOR(255, 120, 180), brillo = COLOR(255, 200, 225);

    circulo(cx, cy, 14 * s, masa);
    circulo(cx, cy, 11 * s, glaseado);
    for (int k = 0; k < 8; k += 2) {                                        // chorreaditos
        float a = k * PI / 4 + 0.4f;
        circulo(cx + 11 * s * cosf(a), cy + 11 * s * sinf(a), 2 * s, glaseado);
    }
    circulo(cx, cy, 5.5f * s, masa);                                        // el agujero
    circulo(cx, cy, 4 * s, fondo);

    static const struct { int8_t x, y; uint8_t r, g, b; } granitos[] = {
        { -8, -3, 255, 255, 255 }, { -3, -8, 60, 200, 255 }, { 4, -7, 255, 230, 0 },
        { 8, -1, 120, 220, 80 },   { 5, 6, 255, 255, 255 },  { -2, 8, 255, 230, 0 },
        { -7, 5, 60, 200, 255 },   { 0, -10, 120, 220, 80 },
    };
    for (unsigned k = 0; k < sizeof(granitos) / sizeof(granitos[0]); k++)
        rect(cx + granitos[k].x * s, cy + granitos[k].y * s, 2 * s, s,
             COLOR(granitos[k].r, granitos[k].g, granitos[k].b));

    rect(cx - 9 * s, cy - 6 * s, s, 3 * s, brillo);                         // brillito
}

// ─── Bebidas ─────────────────────────────────────────────────────────────────

static float cuanto_queda(int sorbos)
{
    if (sorbos < 0) sorbos = 0;
    if (sorbos > 3) sorbos = 3;
    return (3 - sorbos) / 3.0f;
}

// 🍵 Te: taza con asa, platito, el te adentro, el saquito colgando y vapor
static void dibujar_te(float cx, float cy, float s, int sorbos)
{
    const color_t taza = BLANCO, te = COLOR(200, 110, 30), vapor = COLOR(180, 180, 180);
    const color_t hilo = COLOR(230, 230, 230), etiqueta = COLOR(255, 210, 60);
    cx += 2 * s;
    cy += 2 * s;

    circulo(cx + 10 * s, cy + 1 * s, 6 * s, taza);                          // asa
    circulo(cx + 10 * s, cy + 1 * s, 3.5f * s, fondo);
    rect_redondo(cx - 12 * s, cy - 8 * s, 20 * s, 19 * s, 3 * s, taza);
    rect(cx - 10 * s, cy - 8 * s, 16 * s, 17 * s, fondo);
    rect(cx - 16 * s, cy + 12 * s, 30 * s, 2 * s, taza);                    // platito

    float alto = 14 * s * cuanto_queda(sorbos);
    if (alto >= 1) rect(cx - 9 * s, cy + 8 * s - alto, 14 * s, alto, te);

    palito(cx - 5 * s, cy - 2 * s, cx - 11 * s, cy - 10 * s, s, hilo);      // el hilo del saquito
    palito(cx - 13 * s, cy - 10 * s, cx - 15 * s, cy - 3 * s, s, hilo);
    rect(cx - 18 * s, cy - 3 * s, 5 * s, 6 * s, etiqueta);                  // la etiqueta

    if (sorbos == 0 && s >= 1) {
        for (float dy = 0; dy < 8 * s; dy += 1) {
            rect(cx - 5 * s + 1.5f * s * sinf(dy / s * 0.8f), cy - 11 * s - dy, 1, 1, vapor);
            rect(cx + 2 * s + 1.5f * s * sinf(dy / s * 0.8f + 1.5f), cy - 11 * s - dy, 1, 1, vapor);
        }
    }
}

// 🍓 Licuado: vaso alto, licuado rosa de frutilla, pajita a rayas y una frutilla en el borde
static void dibujar_licuado(float cx, float cy, float s, int sorbos)
{
    const color_t vidrio = COLOR(200, 230, 255), licuado = COLOR(255, 110, 160);
    const color_t frutilla = COLOR(230, 30, 50), hoja = COLOR(60, 180, 60), semilla = COLOR(255, 230, 120);
    cy += 1 * s;

    rect_redondo(cx - 8 * s, cy - 12 * s, 16 * s, 27 * s, 3 * s, vidrio);   // vaso
    rect(cx - 6.5f * s, cy - 12 * s, 13 * s, 25.5f * s, fondo);

    float alto = 22 * s * cuanto_queda(sorbos);
    if (alto >= 1) rect(cx - 6.5f * s, cy + 13.5f * s - alto, 13 * s, alto, licuado);

    for (int k = 0; k < 6; k++) {                                           // pajita a rayas
        float t0 = k / 6.0f, t1 = (k + 1) / 6.0f;
        palito(cx + (1 + 8 * t0) * s, cy + (8 - 30 * t0) * s, cx + (1 + 8 * t1) * s, cy + (8 - 30 * t1) * s,
               2 * s, (k % 2) ? BLANCO : COLOR(255, 60, 90));
    }

    triangulo(cx - 12 * s, cy - 12 * s, cx - 4 * s, cy - 12 * s, cx - 8 * s, cy - 5 * s, frutilla);   // frutilla
    circulo(cx - 8 * s, cy - 12 * s, 4 * s, frutilla);
    rect(cx - 10 * s, cy - 12 * s, s, s, semilla);
    rect(cx - 7 * s, cy - 10 * s, s, s, semilla);
    rect(cx - 9 * s, cy - 8 * s, s, s, semilla);
    triangulo(cx - 11 * s, cy - 16 * s, cx - 5 * s, cy - 16 * s, cx - 8 * s, cy - 14 * s, hoja);
}

// 🥤 Gaseosa: lata roja con una onda blanca, el anillito arriba y burbujitas que
// suben (cada sorbo se lleva una burbuja)
static void dibujar_gaseosa(float cx, float cy, float s, int sorbos)
{
    const color_t lata = COLOR(220, 30, 40), metal = COLOR(200, 200, 210), brillo = COLOR(255, 130, 130);
    const color_t burbuja = COLOR(200, 230, 255);
    cy += 3 * s;

    rect_redondo(cx - 8 * s, cy - 12 * s, 16 * s, 26 * s, 3 * s, lata);
    rect(cx - 7 * s, cy - 14 * s, 14 * s, 3 * s, metal);                        // tapa
    rect(cx - 7 * s, cy + 13 * s, 14 * s, 2 * s, metal);                        // base
    for (float dx = -8 * s; dx < 8 * s - 1; dx += 1)                            // la onda blanca
        rect(cx + dx, cy + 1.5f * s * sinf(dx / s * 0.45f), 1, 2.5f * s, BLANCO);
    rect(cx - 6 * s, cy - 9 * s, 1.5f * s, 7 * s, brillo);                      // brillito
    rect(cx - 6 * s, cy + 5 * s, 1.5f * s, 6 * s, brillo);

    circulo(cx + 2 * s, cy - 15 * s, 2 * s, metal);                             // anillito
    circulo(cx + 2 * s, cy - 15 * s, 0.8f * s, fondo);

    // Burbujitas: 3 con la lata llena, ninguna cuando se termino
    static const float bx[3] = { -3, 1, -1 }, by[3] = { -19, -23, -27 };
    for (int k = 0; k < 3 - sorbos; k++)
        circulo(cx + bx[k] * s, cy + by[k] * s, 1.5f * s, burbuja);
}

// ─── Publico ─────────────────────────────────────────────────────────────────

bool comida_es_bebida(comida_t c)
{
    return c == COMIDA_TE || c == COMIDA_LICUADO || c == COMIDA_GASEOSA;
}

void comida_dibujar(comida_t c, float cx, float cy, float s, int mordidas, color_t color_fondo)
{
    fondo = color_fondo;
    float medio_ancho = 14;

    switch (c) {
    case COMIDA_MEDIALUNA: dibujar_medialuna(cx, cy, s); medio_ancho = 19; break;
    case COMIDA_MUFFIN:    dibujar_muffin(cx, cy, s);                       break;
    case COMIDA_DONA:      dibujar_dona(cx, cy, s);                         break;
    case COMIDA_TE:        dibujar_te(cx, cy, s, mordidas);      return;   // en las bebidas,
    case COMIDA_LICUADO:   dibujar_licuado(cx, cy, s, mordidas); return;   // "mordidas" son
    case COMIDA_GASEOSA:   dibujar_gaseosa(cx, cy, s, mordidas); return;   // los sorbos
    default:               return;
    }

    // Mordiscos: "bocados" del color del fondo, del lado que da a la boca
    float borde = cx - medio_ancho * s;
    if (mordidas >= 1) circulo(borde, cy - 1 * s, 6 * s, fondo);
    if (mordidas >= 2) circulo(borde + 3 * s, cy + 7 * s, 7 * s, fondo);
}
