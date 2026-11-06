#include "eyes.h"
#include "board_config.h"

#include <math.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ─── Forma de los ojos (¡jueguen con estos numeros!) ─────────────────────────
#define OJO_ANCHO        30   // pixeles
#define OJO_ALTO         40
#define OJO_RADIO        11   // que tan redondeadas son las esquinas
#define OJO_SEPARACION   22   // espacio entre los dos ojos
#define OJO_ALTO_CERRADO 3    // grosor de la rayita cuando esta cerrado
#define OJOS_Y           60   // altura del centro de los ojos (la pantalla mide 128)

#define PESTANA_GROSOR   2

// Zona de abajo de los ojos, para textos y ondas
#define ABAJO_DESDE      86

// Cada cuanto se dibuja un cuadro nuevo de la animacion
#define CUADRO_MS 30

// ─── Como esta la cara en este momento ───────────────────────────────────────
typedef struct {
    float x, y;                 // corrimiento desde el centro
    float abre_izq, abre_der;   // 0 = cerrado, 1 = abierto
    float ondas;                // 0 a 1: ondas de WiFi abajo (mientras se conecta)
    float texto;                // 0 a 1: texto abajo de los ojos
    float fase;                 // "reloj" de las animaciones (cada +1 es una vuelta)
} cara_t;

static const cara_t CARA_NORMAL = { .x = 0, .y = 0, .abre_izq = 1, .abre_der = 1 };

static cara_t cara;
static color_t color_ojos, color_pestanas, color_texto, color_fondo;

static char texto_abajo[64];
static int  texto_escala = 2;

// ─── Dibujar ─────────────────────────────────────────────────────────────────

/*
 * Las pestañas: tres rayitas que salen del borde de arriba del ojo, del lado
 * de afuera, abiertas como un abanico. Para el ojo izquierdo "afuera" es la
 * izquierda; para el derecho, la derecha (lado = -1 o +1).
 *
 *        \ | /         ← puntas
 *         \|/
 *       ████████       ← borde de arriba del ojo
 *
 * Si el ojo esta casi cerrado, las pestañas apuntan para abajo (ojito cerrado).
 */
static const struct { int base_x, base_y, punta_x, punta_y; } pestanas[3] = {
    { 13, 3, 21, -5 },    // la de mas afuera, sale de la esquina redondeada
    {  8, 1, 13, -8 },
    {  2, 0,  4, -9 },    // la de mas adentro, casi derecha para arriba
};

static void dibujar_pestanas(float cx, float arriba, int lado, float abre)
{
    int hacia = (abre < 0.3f) ? -1 : 1;     // ojo cerrado: las pestañas miran para abajo
    for (int k = 0; k < 3; k++) {
        int bx = (int)(cx + lado * pestanas[k].base_x);
        int by = (int)(arriba + pestanas[k].base_y * hacia);
        int px = (int)(cx + lado * pestanas[k].punta_x);
        int py = (int)(arriba + pestanas[k].punta_y * hacia);
        display_draw_line(bx, by, px, py, PESTANA_GROSOR, color_pestanas);
    }
}

static void dibujar_ojo(float cx, float cy, float abre, int lado)
{
    int h = (int)(OJO_ALTO_CERRADO + (OJO_ALTO - OJO_ALTO_CERRADO) * abre);
    int x = (int)(cx - OJO_ANCHO / 2.0f);
    int y = (int)(cy - h / 2.0f);

    display_fill_round_rect(x, y, OJO_ANCHO, h, OJO_RADIO, color_ojos);
    dibujar_pestanas(cx, y, lado, abre);
}

// Pedazo de circulo (centro, radio, angulos en radianes) con trazo de "grosor"
static void dibujar_arco(float cx, float cy, float r, float desde, float hasta, int grosor, color_t color)
{
    for (float a = desde; a <= hasta; a += 0.04f)
        display_fill_rect((int)(cx + r * cosf(a)) - grosor / 2, (int)(cy + r * sinf(a)) - grosor / 2,
                          grosor, grosor, color);
}

// Simbolo de WiFi abajo de los ojos: un puntito y 3 ondas que se encienden de a una
static void dibujar_ondas(void)
{
    const float ox = LCD_H_RES / 2.0f, oy = 120;
    int ondas = ((int)(cara.fase * 4)) % 4;
    display_fill_circle((int)ox, (int)oy, 2, color_texto);
    for (int k = 0; k < ondas; k++)
        dibujar_arco(ox, oy, 8 + 6 * k, 1.25f * (float)M_PI, 1.75f * (float)M_PI, 2, color_texto);
}

static void dibujar(void)
{
    float cy     = OJOS_Y + cara.y;
    float cx_izq = LCD_H_RES / 2.0f - OJO_SEPARACION / 2.0f - OJO_ANCHO / 2.0f + cara.x;
    float cx_der = LCD_H_RES / 2.0f + OJO_SEPARACION / 2.0f + OJO_ANCHO / 2.0f + cara.x;

    display_clear(color_fondo);
    dibujar_ojo(cx_izq, cy, cara.abre_izq, -1);
    dibujar_ojo(cx_der, cy, cara.abre_der, +1);

    if (cara.ondas > 0.5f) dibujar_ondas();

    if (cara.texto > 0.5f) {
        // Centrado en la zona de abajo de los ojos
        int alto = display_text_height(texto_abajo, texto_escala);
        int y = ABAJO_DESDE + (LCD_V_RES - ABAJO_DESDE - alto) / 2;
        display_draw_text_centered(texto_abajo, y, texto_escala, color_texto);
    }

    display_flush();
}

// ─── Animar ──────────────────────────────────────────────────────────────────

static float mezclar(float a, float b, float t) { return a + (b - a) * t; }

// Pasa suavemente de la cara actual a "destino" en "ms" milisegundos
static void animar(cara_t destino, int ms)
{
    cara_t inicio = cara;
    int pasos = ms / CUADRO_MS;
    if (pasos < 1) pasos = 1;

    TickType_t ultimo = xTaskGetTickCount();
    for (int i = 1; i <= pasos; i++) {
        float t = (float)i / pasos;
        t = t * t * (3 - 2 * t);    // arranca y frena despacito

        cara.x        = mezclar(inicio.x,        destino.x,        t);
        cara.y        = mezclar(inicio.y,        destino.y,        t);
        cara.abre_izq = mezclar(inicio.abre_izq, destino.abre_izq, t);
        cara.abre_der = mezclar(inicio.abre_der, destino.abre_der, t);
        cara.ondas    = mezclar(inicio.ondas,    destino.ondas,    t);
        cara.texto    = mezclar(inicio.texto,    destino.texto,    t);
        cara.fase     = mezclar(inicio.fase,     destino.fase,     t);

        dibujar();
        xTaskDelayUntil(&ultimo, pdMS_TO_TICKS(CUADRO_MS));
    }
}

// ─── Funciones publicas ──────────────────────────────────────────────────────

void eyes_init(color_t ojos, color_t pestanas_color, color_t texto, color_t fondo)
{
    color_ojos     = ojos;
    color_pestanas = pestanas_color;
    color_texto    = texto;
    color_fondo    = fondo;
    cara = CARA_NORMAL;
    dibujar();
}

void eyes_parpadear(void)
{
    cara_t abierta = cara;
    cara_t cerrada = cara;
    cerrada.abre_izq = cerrada.abre_der = 0;

    animar(cerrada, 70);       // se cierran rapido...
    vTaskDelay(pdMS_TO_TICKS(40));
    animar(abierta, 110);      // ...y se abren un poquito mas lento
}

void eyes_onditas(void)
{
    cara_t d = cara;
    d.texto = 0;
    d.ondas = 1;
    for (int i = 0; i < 4; i++) {
        d.fase += 0.25f;
        animar(d, 300);
    }
}

void eyes_decir(const char *texto, int escala, int ms)
{
    snprintf(texto_abajo, sizeof(texto_abajo), "%s", texto);
    texto_escala = escala;

    cara_t d = cara;
    d.ondas = 0;
    d.texto = 1;
    animar(d, 150);

    vTaskDelay(pdMS_TO_TICKS(ms / 2));
    eyes_parpadear();                  // un parpadeo en el medio, para que se vea viva
    vTaskDelay(pdMS_TO_TICKS(ms / 2));

    d = cara;
    d.texto = 0;
    animar(d, 150);
}
