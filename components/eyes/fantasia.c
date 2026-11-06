// ─────────────────────────────────────────────────────────────────────────────
//  Los personajes del modo Fantasia (cuentos de hadas)
// ─────────────────────────────────────────────────────────────────────────────
//
//  Flufi hace de personaje de cuento, SIEMPRE con sus ojos, sus pestañas y sus
//  labios rojos (y casi siempre con sus rulos).
//
//  Como se arma un personaje: ver eyes_interno.h.
//
#include "eyes_interno.h"

// Cosas que aparecen solo mientras actua
static bool  hay_polvo   = false;    // hada: polvo de estrellas
static bool  hay_cola    = false;    // sirena: la cola de sirena
static float fuego       = 0;        // dragoncita: de 0 (nada) a 1 (llamarada larga)
static bool  hay_canasta = false;    // caperucita

static void limpiar(void)
{
    hay_polvo = hay_cola = hay_canasta = false;
    fuego = 0;
}

// Donde estan los rulos (se mueven un poquito menos que los ojos)
static float pelo_x(void) { return LCD_H_RES / 2.0f + cara.x * 0.5f; }
static float pelo_y(void) { return cara.y * 0.3f; }

// ═════════════════════════════════════════════════════════════════════════════
//  🧚 Hada: alas transparentes que aletean y una varita con estrella
// ═════════════════════════════════════════════════════════════════════════════

static void dibujar_hada_atras(float mx, float cy)
{
    const color_t ala = COLOR(170, 225, 255), ala_clara = COLOR(225, 245, 255);
    float abiertas = 0.4f + 0.6f * fabsf(cosf(cara.fase * 2 * PI));
    for (int lado = -1; lado <= 1; lado += 2) {
        float rx = 12 * abiertas, x = mx + lado * (38 + rx);             // ala de arriba
        elipse(x, cy - 10, rx, 19, ala);
        elipse(x, cy - 10, rx * 0.6f, 12, ala_clara);
        rx = 8 * abiertas; x = mx + lado * (36 + rx);                     // ala de abajo
        elipse(x, cy + 16, rx, 11, ala);
        elipse(x, cy + 16, rx * 0.5f, 6, ala_clara);
    }
}

static void dibujar_hada_adelante(float mx, float cy)
{
    // La varita, abajo a la derecha (se mueve mientras hace magia)
    float ang = -0.35f + (hay_polvo ? 0.35f * onda(1) : 0);
    float bx = mx + 48, by = cy + 46;
    float px = bx + 26 * sinf(ang), py = by - 26 * cosf(ang);
    linea(bx, by, px, py, 2, COLOR(255, 240, 200));
    estrella(px, py, 5, COLOR(255, 220, 40));
    circulo(px, py, 2, BLANCO);

    if (!hay_polvo) return;
    static const color_t colores[3] = { COLOR(255, 230, 80), COLOR(255, 160, 220), BLANCO };
    for (int k = 0; k < 6; k++) {                                         // polvo de estrellas
        float t = fmodf(cara.fase * 2 + k / 6.0f, 1.0f);
        estrella(px - 40 * t, py - 25 * t + 30 * t * t, 1 + 2 * (1 - t), colores[k % 3]);
    }
}

static void actuar_hada(void)
{
    cara_t d = cara;
    float y_normal = d.y;

    hay_polvo = true;
    for (int i = 0; i < 26; i++) {
        d.fase += 0.06f;
        d.y = y_normal + 3 * sinf(i * 0.5f);            // flota
        globo = (i >= 8 && i < 18) ? "¡Plin!" : NULL;
        animar(d, 80);
    }
    globo = NULL;
    hay_polvo = false;
    d.y = y_normal;
    d.fase = ceilf(d.fase);                             // alas abiertas
    d.feliz = 0.5f;
    animar(d, 250);
    cerrar_y_abrir(true, false, 300);                   // guiño
    d.feliz = 0;
    animar(d, 200);
}

// ═════════════════════════════════════════════════════════════════════════════
//  👑 Princesa: corona dorada con piedritas sobre los rulos y cachetes rosados
// ═════════════════════════════════════════════════════════════════════════════

static void dibujar_princesa_atras(float mx, float cy)
{
    // Cachetes rosados, al costado de los ojos
    for (int lado = -1; lado <= 1; lado += 2)
        elipse(mx + lado * 45, cy + 20, 5, 3, COLOR(255, 140, 170));
}

static void dibujar_princesa_adelante(float mx, float cy)
{
    const color_t oro = COLOR(255, 205, 50), oro_oscuro = COLOR(210, 150, 20);
    float x = pelo_x(), y = 14 + pelo_y();

    static const float puntas[5][2] = { { -15, 6 }, { -8, 3 }, { 0, 0 }, { 8, 3 }, { 15, 6 } };
    for (int k = 0; k < 5; k++) {
        triangulo(x + puntas[k][0] - 5, y, x + puntas[k][0] + 5, y, x + puntas[k][0], y - 12 + puntas[k][1], oro);
        circulo(x + puntas[k][0], y - 12 + puntas[k][1], 1.5f, BLANCO);                 // perlitas
    }
    rect(x - 19, y, 38, 7, oro);                                                         // la banda
    rect(x - 19, y + 6, 38, 1, oro_oscuro);
    circulo(x, y + 3, 2.5f, COLOR(230, 30, 60));                                        // piedritas
    circulo(x - 11, y + 3, 2, COLOR(60, 140, 255));
    circulo(x + 11, y + 3, 2, COLOR(60, 140, 255));
    estrella(x + 13, y - 9, 1 + 2 * fabsf(onda(2)), BLANCO);                            // brillo
}

static void actuar_princesa(void)
{
    cara_t d = cara;
    float y_normal = d.y;

    d.y = y_normal + 6;                                 // reverencia
    d.abre_izq = d.abre_der = 0.3f;
    d.feliz = 0.6f;
    animar(d, 450);
    esperar(300);
    d.y = y_normal;
    d.abre_izq = d.abre_der = 1;
    animar(d, 350);

    globo = "¡Saludos!";                                // corazoncitos y brillos
    cara.corazoncitos = 0;
    d.corazoncitos = 0.98f;
    for (int i = 0; i < 4; i++) {
        d.fase += 0.25f;
        animar(d, 400);
    }
    globo = NULL;
    cara.corazoncitos = 0;
    d.corazoncitos = 0;
    d.feliz = 0;
    animar(d, 250);
}

// ═════════════════════════════════════════════════════════════════════════════
//  🧜 Sirena: el fondo es el mar, con burbujas, algas, una conchita en el pelo
//  y la cola de sirena que se asoma al nadar
// ═════════════════════════════════════════════════════════════════════════════

#define SIRENA_MAR COLOR(0, 40, 90)

static void dibujar_sirena_atras(float mx, float cy)
{
    const color_t alga = COLOR(40, 170, 90);
    for (int lado = -1; lado <= 1; lado += 2) {                          // algas en las esquinas
        float x0 = (lado < 0) ? 6 : LCD_H_RES - 7;
        for (int k = 0; k < 6; k++) {
            float y0 = LCD_V_RES - k * 8, y1 = y0 - 8;
            float d0 = 3 * sinf(k * 1.2f + cara.fase * 2 * PI), d1 = 3 * sinf((k + 1) * 1.2f + cara.fase * 2 * PI);
            linea(x0 + d0, y0, x0 + d1, y1, 3, alga);
        }
    }
}

static void dibujar_sirena_adelante(float mx, float cy)
{
    // Conchita rosa en el pelo
    float x = pelo_x() + 44, y = 28 + pelo_y();
    const color_t concha = COLOR(255, 170, 190), concha_oscura = COLOR(220, 110, 140);
    elipse(x, y, 7, 5, concha);
    triangulo(x - 3, y + 3, x + 3, y + 3, x, y + 8, concha);
    for (int k = -1; k <= 1; k++) linea(x, y + 5, x + k * 5, y - 4, 1, concha_oscura);

    // Burbujas que suben
    for (int k = 0; k < 5; k++) {
        float t = fmodf(cara.fase * 0.5f + k * 0.2f, 1.0f);
        float bx = 14 + k * 25 + 4 * sinf(t * 6 + k), by = LCD_V_RES + 6 - t * 140;
        circulo(bx, by, 3, COLOR(150, 210, 255));
        circulo(bx, by, 1.5f, SIRENA_MAR);
    }

    if (!hay_cola) return;
    // La cola: sale de abajo a la derecha, se mueve, y termina en una aleta
    const color_t cola = COLOR(0, 190, 160), escama = COLOR(120, 240, 210);
    float ang = -2.2f + 0.4f * onda(1);
    const float x0 = LCD_H_RES + 4, y0 = LCD_V_RES + 4, largo = 34;
    for (float t = 0; t <= 1; t += 0.08f) {
        float cx = x0 + cosf(ang) * largo * t, cyy = y0 + sinf(ang) * largo * t;
        circulo(cx, cyy, 8 - 4 * t, cola);
        rect(cx - 1, cyy - 1, 2, 2, escama);
    }
    float fx = x0 + cosf(ang) * largo, fy = y0 + sinf(ang) * largo;    // la aleta
    float ax = cosf(ang + 0.6f) * 12, ay = sinf(ang + 0.6f) * 12;
    float bx = cosf(ang - 0.6f) * 12, by = sinf(ang - 0.6f) * 12;
    triangulo(fx, fy, fx + ax, fy + ay, fx + (ax + bx) * 0.35f, fy + (ay + by) * 0.35f, cola);
    triangulo(fx, fy, fx + bx, fy + by, fx + (ax + bx) * 0.35f, fy + (ay + by) * 0.35f, cola);
}

static void actuar_sirena(void)
{
    cara_t d = cara;
    float y_normal = d.y;

    hay_cola = true;                                    // nada
    for (int i = 0; i < 28; i++) {
        d.fase += 0.06f;
        d.x = 8 * sinf(i * 0.25f);
        d.y = y_normal + 5 * sinf(i * 0.5f);
        globo = (i >= 6 && i < 16) ? "¡Blub!" : NULL;
        animar(d, 80);
    }
    globo = NULL;
    hay_cola = false;
    d.x = 0;
    d.y = y_normal;
    d.feliz = 1;
    animar(d, 250);
    esperar(500);
    d.feliz = 0;
    animar(d, 200);
}

// ═════════════════════════════════════════════════════════════════════════════
//  🐉 Dragoncita: cuernitos entre los rulos, alitas verdes ¡y tira fuego!
// ═════════════════════════════════════════════════════════════════════════════

static void dibujar_dragoncita_atras(float mx, float cy)
{
    const color_t ala = COLOR(90, 200, 90), ala_clara = COLOR(160, 235, 150);
    float aleteo = 4 * onda(2);
    for (int lado = -1; lado <= 1; lado += 2) {
        triangulo(mx + lado * 40, cy - 14, mx + lado * 62, cy - 32 + aleteo, mx + lado * 58, cy + 8, ala);
        triangulo(mx + lado * 44, cy - 10, mx + lado * 57, cy - 22 + aleteo, mx + lado * 55, cy + 2, ala_clara);
    }
}

static void dibujar_dragoncita_adelante(float mx, float cy)
{
    // Cuernitos que salen de entre los rulos
    const color_t cuerno = COLOR(255, 200, 90), punta = COLOR(255, 140, 40);
    float x = pelo_x(), y = pelo_y();
    for (int lado = -1; lado <= 1; lado += 2) {
        triangulo(x + lado * 13, y + 22, x + lado * 25, y + 22, x + lado * 23, y + 2, cuerno);
        triangulo(x + lado * 20, y + 9, x + lado * 24, y + 9, x + lado * 23, y + 2, punta);
    }

    if (fuego < 0.01f) return;
    // La llamarada: sale de la boca hacia la derecha
    float base = cy + cara.alto / 2 + 7;
    float largo = 52 * fuego;
    for (float t = 0; t <= 1; t += 0.07f) {
        color_t c = (t < 0.3f) ? COLOR(255, 240, 120) : (t < 0.65f) ? COLOR(255, 150, 0) : COLOR(240, 50, 20);
        float temblor = 2 * sinf(cara.fase * 40 + t * 8);
        circulo(mx + 14 + largo * t, base + temblor, 2 + 6 * t, c);
    }
}

static void actuar_dragoncita(void)
{
    cara_t d = cara;

    for (int i = 0; i < 6; i++) {                       // mueve las alitas
        d.fase += 0.1f;
        animar(d, 90);
    }
    d.asombro = 1;                                      // toma aire...
    globo = "¡Grrr!";
    animar(d, 350);
    esperar(250);
    globo = NULL;

    d.asombro = 0.6f;                                   // ¡fuego!
    for (int i = 0; i < 16; i++) {
        fuego = fminf(1, (i + 1) / 5.0f);
        if (i > 12) fuego = (16 - i) / 4.0f;
        d.fase += 0.03f;
        poner_led((i % 2) ? LED(255, 140, 0) : LED(255, 30, 0));   // el LED titila como el fuego
        animar(d, 70);
    }
    fuego = 0;
    d.asombro = 0;
    d.feliz = 1;                                        // ¡je!
    globo = "¡Ups!";
    animar(d, 200);
    esperar(700);
    globo = NULL;
    d.feliz = 0;
    animar(d, 200);
}

// ═════════════════════════════════════════════════════════════════════════════
//  🧺 Caperucita: capucha roja (con el flequillo asomando), moño y su canasta
// ═════════════════════════════════════════════════════════════════════════════

#define CAPERUZA        COLOR(210, 30, 45)
#define CAPERUZA_OSCURA COLOR(150, 15, 30)

static void dibujar_caperucita_atras(float mx, float cy)
{
    elipse(mx, cy - 8, 60, 58, CAPERUZA);                    // la capucha
    elipse(mx, cy + 2, 50, 50, CAPERUZA_OSCURA);             // el borde de adentro
    elipse(mx, cy + 2, 47, 47, color_fondo);                 // el lugar de la cara
    rect(mx - 64, cy + 32, 128, 64, color_fondo);            // abierta abajo

    const color_t rubio = COLOR(255, 205, 70);               // flequillo de rulitos
    for (int k = -3; k <= 3; k++) circulo(mx + k * 10, cy - 41 + (k % 2 ? 1 : 0), 5, rubio);
    for (int lado = -1; lado <= 1; lado += 2) {
        circulo(mx + lado * 42, cy - 26, 5, rubio);
        circulo(mx + lado * 45, cy - 14, 5, rubio);
    }
}

static void dibujar_canasta(float x, float y)
{
    const color_t mimbre = COLOR(190, 130, 60), mimbre_oscuro = COLOR(140, 90, 40);
    arco(x, y, 13, PI, 2 * PI, 2, mimbre_oscuro);                                   // manija
    circulo(x - 6, y - 2, 4, COLOR(255, 120, 190));                                 // flores
    circulo(x + 4, y - 3, 4, COLOR(255, 230, 80));
    circulo(x - 6, y - 2, 1.5f, COLOR(255, 230, 80));
    circulo(x + 4, y - 3, 1.5f, COLOR(255, 120, 190));
    display_fill_round_rect((int)(x - 14), (int)y, 28, 13, 3, mimbre);
    for (int k = 0; k < 3; k++) rect(x - 13, y + 3 + k * 4, 26, 1, mimbre_oscuro);  // tejido
}

static void dibujar_caperucita_adelante(float mx, float cy)
{
    // El moño debajo de la cara, con las cintas que vienen de la capucha
    float y = cy + 42;
    linea(mx - 40, cy + 30, mx - 4, y, 2, CAPERUZA);
    linea(mx + 40, cy + 30, mx + 4, y, 2, CAPERUZA);
    triangulo(mx, y, mx - 10, y - 5, mx - 10, y + 5, CAPERUZA);
    triangulo(mx, y, mx + 10, y - 5, mx + 10, y + 5, CAPERUZA);
    circulo(mx, y, 2.5f, CAPERUZA_OSCURA);

    if (hay_canasta) dibujar_canasta(cara.comida_x, 108);
}

static void actuar_caperucita(void)
{
    cara_t d = cara;
    float y_normal = d.y;

    for (int i = 0; i < 20; i++) {                      // camina cantando
        d.x = 5 * sinf(i * 0.6f);
        d.y = y_normal - 3 * fabsf(sinf(i * 0.6f));
        d.fase += 0.05f;
        globo = (i >= 2 && i < 12) ? "¡Lalala!" : NULL;
        animar(d, 100);
    }
    globo = NULL;

    hay_canasta = true;                                 // muestra su canasta
    cara.comida_x = d.comida_x = 160;
    d.comida_x = 100;
    d.x = -6;
    d.y = y_normal;
    animar(d, 500);
    globo = "¡Abuela!";
    d.feliz = 1;
    animar(d, 200);
    esperar(1000);
    globo = NULL;
    d.feliz = 0;
    d.comida_x = 160;
    animar(d, 400);
    hay_canasta = false;
    d.x = 0;
    animar(d, 200);
}

// ═════════════════════════════════════════════════════════════════════════════
//  La lista (en el orden de eyes.h)
// ═════════════════════════════════════════════════════════════════════════════

const personaje_info_t personajes_fantasia[PERSONAJES_POR_MODO] = {
    //  nombre        rulos  ancho alto  y   LED                 fondo
    { "hada",       true,  26,   34,   4, LED(150, 220, 255), NEGRO,      dibujar_hada_atras,       dibujar_hada_adelante,       actuar_hada,       limpiar },
    { "princesa",   true,  28,   36,   4, LED(255, 170, 0),   NEGRO,      dibujar_princesa_atras,   dibujar_princesa_adelante,   actuar_princesa,   limpiar },
    { "sirena",     true,  28,   36,   2, LED(0, 200, 180),   SIRENA_MAR, dibujar_sirena_atras,     dibujar_sirena_adelante,     actuar_sirena,     limpiar },
    { "dragoncita", true,  28,   36,   4, LED(255, 60, 0),    NEGRO,      dibujar_dragoncita_atras, dibujar_dragoncita_adelante, actuar_dragoncita, limpiar },
    { "caperucita", false, 28,   36,   6, LED(255, 0, 0),     NEGRO,      dibujar_caperucita_atras, dibujar_caperucita_adelante, actuar_caperucita, limpiar },
};
