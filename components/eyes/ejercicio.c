// ─────────────────────────────────────────────────────────────────────────────
//  Los personajes del modo Ejercicio
// ─────────────────────────────────────────────────────────────────────────────
//
//  Flufi hace deporte: patina, baila, juega al futbol, anda en bici y levanta
//  pesas. SIEMPRE con sus ojos, sus pestañas, sus labios rojos y sus rulos.
//
//  Como se arma un personaje: ver eyes_interno.h.
//
#include "eyes_interno.h"

// Cosas que aparecen solo mientras hace el ejercicio
static bool hay_viento = false;      // patinar: rayitas de velocidad
static bool hay_notas  = false;      // bailar: notas musicales
static bool hay_pelota = false;      // futbol
static bool hay_gotas  = false;      // pesas: gotitas de transpiracion

static void limpiar(void)
{
    hay_viento = hay_notas = hay_pelota = hay_gotas = false;
}

// Donde estan los rulos (se mueven un poquito menos que los ojos)
static float pelo_x(void) { return LCD_H_RES / 2.0f + cara.x * 0.5f; }
static float pelo_y(void) { return cara.y * 0.3f; }

// Vincha deportiva sobre los rulos, a rayas de dos colores
static void vincha(color_t c1, color_t c2)
{
    float x = pelo_x(), y = 16 + pelo_y();
    for (int k = -10; k < 10; k++)
        rect(x + k * 6, y, 6, 5, (k % 2) ? c1 : c2);
}

// Una gotita (de transpiracion)
static void gotita(float x, float y)
{
    const color_t agua = COLOR(150, 210, 255);
    circulo(x, y, 2, agua);
    triangulo(x - 2, y, x + 2, y, x, y - 5, agua);
}

// ═════════════════════════════════════════════════════════════════════════════
//  ⛸️ Patinar: patines rosas con rueditas que giran, y se desliza de lado a lado
// ═════════════════════════════════════════════════════════════════════════════

static void patin(float x, float y)
{
    const color_t bota = COLOR(255, 120, 190), suela = COLOR(200, 70, 140), rueda = COLOR(255, 220, 0);
    display_fill_round_rect((int)(x - 6), (int)(y - 14), 12, 14, 3, bota);          // la bota
    display_fill_round_rect((int)(x - 6), (int)(y - 5), 20, 6, 3, bota);            // la punta
    rect(x - 6, y, 20, 2, suela);
    for (int k = 0; k < 3; k++) rect(x - 3, y - 12 + k * 3, 6, 1, BLANCO);          // cordones
    for (int k = 0; k < 3; k++) {                                                    // rueditas
        float rx = x - 2 + k * 7, ry = y + 5;
        circulo(rx, ry, 3, rueda);
        float a = cara.fase * 2 * PI * 3 + k;                                        // giran
        rect(rx + 2 * cosf(a), ry + 2 * sinf(a), 1, 1, COLOR(160, 120, 0));
    }
}

static void dibujar_patinar_atras(float mx, float cy)
{
    if (!hay_viento) return;
    // Rayitas de velocidad del lado de donde viene
    float lado = (onda(0.5f) >= 0) ? -1 : 1;
    for (int k = 0; k < 4; k++) {
        float y = cy - 20 + k * 14;
        rect(mx + lado * (50 + k % 2 * 6), y, 10, 1, COLOR(200, 200, 255));
    }
}

static void dibujar_patinar_adelante(float mx, float cy)
{
    vincha(COLOR(170, 80, 255), BLANCO);
    patin(mx - 26, 118);
    patin(mx + 12, 118);
}

static void actuar_patinar(void)
{
    cara_t d = cara;
    float y_normal = d.y;

    hay_viento = true;
    for (int i = 0; i < 30; i++) {                      // se desliza de lado a lado
        d.fase += 0.08f;
        d.x = 18 * sinf(i * 0.21f);
        d.y = y_normal + 2 * sinf(i * 0.42f);
        globo = (i >= 8 && i < 18) ? "¡Wiii!" : NULL;
        animar(d, 80);
    }
    globo = NULL;
    hay_viento = false;
    d.x = 0;
    d.y = y_normal;
    d.feliz = 1;
    animar(d, 250);
    esperar(500);
    d.feliz = 0;
    animar(d, 200);
}

// ═════════════════════════════════════════════════════════════════════════════
//  💃 Bailar: auriculares, notas musicales y el LED cambia de color con el ritmo
// ═════════════════════════════════════════════════════════════════════════════

static void nota(float x, float y, color_t c)
{
    circulo(x, y, 3, c);
    rect(x + 2, y - 11, 2, 11, c);
    rect(x + 2, y - 11, 6, 2, c);
}

static void dibujar_bailar_atras(float mx, float cy)
{
    // La vincha de los auriculares, por arriba de los rulos
    arco(pelo_x(), cy - 4, 60, 1.12f * PI, 1.88f * PI, 4, COLOR(255, 80, 160));
}

static void dibujar_bailar_adelante(float mx, float cy)
{
    for (int lado = -1; lado <= 1; lado += 2) {                         // los auriculares
        float x = pelo_x() + lado * 58;
        display_fill_round_rect((int)(x - 6), (int)(cy - 16), 12, 22, 5, COLOR(255, 80, 160));
        display_fill_round_rect((int)(x - 3), (int)(cy - 12), 6, 14, 3, COLOR(120, 20, 70));
    }
    if (!hay_notas) return;
    static const color_t colores[3] = { COLOR(255, 230, 80), COLOR(120, 220, 255), COLOR(255, 140, 220) };
    for (int k = 0; k < 4; k++) {                                       // notas que suben
        float t = fmodf(cara.fase * 1.5f + k * 0.25f, 1.0f);
        float x = (k % 2) ? LCD_H_RES - 16 : 10;
        nota(x + 4 * sinf(t * 8), LCD_V_RES - t * 90, colores[k % 3]);
    }
}

static void actuar_bailar(void)
{
    static const color_led_t luces[6] = {
        LED(255, 0, 120), LED(0, 160, 255), LED(255, 200, 0), LED(140, 0, 255), LED(0, 255, 120), LED(255, 60, 0),
    };
    cara_t d = cara;
    float y_normal = d.y;

    hay_notas = true;
    for (int i = 0; i < 28; i++) {
        d.fase += 0.05f;
        d.y = y_normal - 5 * fabsf(sinf(i * 0.8f));     // salta al ritmo
        d.x = 8 * sinf(i * 0.4f);                       // y se mueve de lado a lado
        d.feliz = (i % 8 < 4) ? 0.7f : 0;
        poner_led(luces[(i / 3) % 6]);                  // luces de colores, como en una fiesta
        globo = (i >= 4 && i < 14) ? "¡Baila!" : NULL;
        animar(d, 90);
    }
    globo = NULL;
    hay_notas = false;
    d.x = 0;
    d.y = y_normal;
    d.feliz = 0;
    animar(d, 200);
}

// ═════════════════════════════════════════════════════════════════════════════
//  ⚽ Futbol: vincha celeste y blanca, hace jueguito con la pelota ¡y gol!
// ═════════════════════════════════════════════════════════════════════════════

static void pelota(float x, float y)
{
    const color_t negro = COLOR(30, 30, 30);
    circulo(x, y, 9, BLANCO);
    circulo(x, y, 3, negro);                                            // los parches
    for (int k = 0; k < 5; k++) {
        float a = k * 2 * PI / 5 + cara.fase * 2 * PI;                  // gira
        circulo(x + 7 * cosf(a), y + 7 * sinf(a), 2, negro);
    }
}

static void dibujar_futbol_atras(float mx, float cy) { (void)mx; (void)cy; }

static void dibujar_futbol_adelante(float mx, float cy)
{
    vincha(COLOR(110, 190, 255), BLANCO);
    if (hay_pelota) pelota(cara.comida_x, cara.comida_y);
}

static void actuar_futbol(void)
{
    cara_t d = cara;
    float y_normal = d.y;

    hay_pelota = true;                                  // jueguito
    cara.comida_x = d.comida_x = 100;
    cara.comida_y = d.comida_y = 118;
    for (int k = 0; k < 4; k++) {
        d.comida_y = 92;  d.fase += 0.25f; d.y = y_normal - 3; animar(d, 230);
        d.comida_y = 118; d.fase += 0.25f; d.y = y_normal;     animar(d, 230);
    }
    d.comida_x = 170;                                   // ¡patada!
    d.comida_y = 30;
    d.fase += 1;
    animar(d, 300);
    hay_pelota = false;

    arcoiris = true;                                    // ¡gol!
    globo = "¡Gol!";
    d.feliz = 1;
    for (int i = 0; i < 2; i++) {
        d.y = y_normal - 8; animar(d, 150);
        d.y = y_normal;     animar(d, 150);
    }
    esperar(500);
    globo = NULL;
    arcoiris = false;
    d.feliz = 0;
    animar(d, 200);
}

// ═════════════════════════════════════════════════════════════════════════════
//  🚲 Bici: casco rosa sobre los rulos y una bici abajo con las ruedas girando
// ═════════════════════════════════════════════════════════════════════════════

static void rueda(float x, float y)
{
    const color_t goma = COLOR(60, 60, 70), rayo = COLOR(190, 190, 200);
    circulo(x, y, 12, goma);
    circulo(x, y, 10, color_fondo);
    for (int k = 0; k < 2; k++) {                                       // rayos que giran
        float a = cara.fase * 2 * PI * 2 + k * PI / 2;
        linea(x - 9 * cosf(a), y - 9 * sinf(a), x + 9 * cosf(a), y + 9 * sinf(a), 1, rayo);
    }
    circulo(x, y, 2, rayo);
}

static void dibujar_bici_atras(float mx, float cy) { (void)mx; (void)cy; }

static void dibujar_bici_adelante(float mx, float cy)
{
    // Casco: medio ovalo rosa sobre los rulos, con ranuras y correas
    const color_t casco = COLOR(255, 90, 170), oscuro = COLOR(170, 40, 110);
    float x = pelo_x(), y = 22 + pelo_y();
    for (int dy = -22; dy <= 0; dy++) {
        float medio = 54 * sqrtf(1 - (dy * dy) / (22.0f * 22.0f));
        rect(x - medio, y + dy, 2 * medio + 1, 1, casco);
    }
    for (int k = -1; k <= 1; k++) rect(x + k * 16 - 2, y - 17, 4, 9, oscuro);       // ranuras
    rect(x - 54, y, 108, 3, oscuro);                                                 // visera
    linea(x - 50, y + 2, mx - 44, cy + 6, 1, oscuro);                                // correas
    linea(x + 50, y + 2, mx + 44, cy + 6, 1, oscuro);

    // La bici
    const color_t cuadro = COLOR(80, 200, 255);
    float ry = 114;
    rueda(mx - 28, ry);
    rueda(mx + 28, ry);
    linea(mx - 28, ry, mx - 4, ry - 14, 2, cuadro);
    linea(mx - 4, ry - 14, mx + 16, ry - 14, 2, cuadro);
    linea(mx - 4, ry - 14, mx + 4, ry, 2, cuadro);
    linea(mx + 4, ry, mx - 28, ry, 2, cuadro);
    linea(mx + 16, ry - 14, mx + 28, ry, 2, cuadro);
    linea(mx + 16, ry - 14, mx + 19, ry - 21, 2, cuadro);                            // manubrio
    rect(mx + 15, ry - 22, 9, 2, COLOR(60, 60, 70));
    rect(mx - 10, ry - 18, 10, 3, COLOR(60, 60, 70));                                // asiento

    // El piso que pasa
    float corrido = fmodf(cara.fase * 40, 16);
    for (float px = -corrido; px < LCD_H_RES; px += 16) rect(px, LCD_V_RES - 1, 8, 1, COLOR(120, 120, 120));
}

static void actuar_bici(void)
{
    cara_t d = cara;
    float y_normal = d.y;

    for (int i = 0; i < 28; i++) {                      // pedalea
        d.fase += 0.1f;
        d.y = y_normal + (i % 2);
        globo = (i >= 10 && i < 20) ? "¡Rin rin!" : NULL;
        animar(d, 80);
    }
    globo = NULL;
    d.y = y_normal;
    d.feliz = 1;
    animar(d, 250);
    esperar(500);
    d.feliz = 0;
    animar(d, 200);
}

// ═════════════════════════════════════════════════════════════════════════════
//  🏋️ Pesas: vincha, una mancuerna a cada lado ¡y cuenta las repeticiones!
// ═════════════════════════════════════════════════════════════════════════════

static void mancuerna(float x, float y)
{
    const color_t barra = COLOR(190, 190, 200), peso = COLOR(80, 80, 95);
    rect(x - 8, y - 1, 16, 3, barra);
    display_fill_round_rect((int)(x - 12), (int)(y - 6), 5, 13, 2, peso);
    display_fill_round_rect((int)(x + 7), (int)(y - 6), 5, 13, 2, peso);
}

static void dibujar_pesas_atras(float mx, float cy) { (void)mx; (void)cy; }

static void dibujar_pesas_adelante(float mx, float cy)
{
    vincha(COLOR(255, 60, 140), BLANCO);
    // Primero sube la de la izquierda y despues la de la derecha
    float sube = onda(1);
    mancuerna(mx - 52, cy + 34 - 30 * fmaxf(0, sube));
    mancuerna(mx + 52, cy + 34 - 30 * fmaxf(0, -sube));
    if (hay_gotas) {
        gotita(mx - 44, cy - 26 + 6 * fabsf(sube));
        gotita(mx + 46, cy - 22 + 6 * fabsf(sube));
    }
}

static void actuar_pesas(void)
{
    static const char *const cuenta[3] = { "1", "2", "3" };
    cara_t d = cara;

    hay_gotas = true;
    d.enojo = 0.3f;                                     // cara de esfuerzo
    d.fase = floorf(d.fase);
    for (int rep = 0; rep < 3; rep++) {
        globo = cuenta[rep];
        d.fase += 0.5f; animar(d, 450);                 // sube la izquierda
        d.fase += 0.5f; animar(d, 450);                 // sube la derecha
    }
    hay_gotas = false;
    d.enojo = 0;
    d.feliz = 1;
    globo = "¡Fuerte!";
    animar(d, 250);
    esperar(800);
    globo = NULL;
    d.feliz = 0;
    animar(d, 200);
}

// ═════════════════════════════════════════════════════════════════════════════
//  La lista (en el orden de eyes.h)
// ═════════════════════════════════════════════════════════════════════════════

const personaje_info_t personajes_ejercicio[PERSONAJES_POR_MODO] = {
    //  nombre      rulos ancho alto  y   LED                fondo
    { "patinar",  true, 30,   40,  -4, LED(0, 200, 255),  NEGRO, dibujar_patinar_atras, dibujar_patinar_adelante, actuar_patinar, limpiar },
    { "bailar",   true, 30,   40,   0, LED(255, 0, 120),  NEGRO, dibujar_bailar_atras,  dibujar_bailar_adelante,  actuar_bailar,  limpiar },
    { "futbol",   true, 28,   36,  -2, LED(0, 220, 60),   NEGRO, dibujar_futbol_atras,  dibujar_futbol_adelante,  actuar_futbol,  limpiar },
    { "bici",     true, 28,   36,  -6, LED(255, 200, 0),  NEGRO, dibujar_bici_atras,    dibujar_bici_adelante,    actuar_bici,    limpiar },
    { "pesas",    true, 28,   36,   0, LED(255, 40, 40),  NEGRO, dibujar_pesas_atras,   dibujar_pesas_adelante,   actuar_pesas,   limpiar },
};
