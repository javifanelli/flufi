#include "eyes.h"
#include "led.h"
#include "board_config.h"

#include <math.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_random.h"
#include "esp_log.h"

static const char *TAG = "OJOS";

#define PI ((float)M_PI)

// ─── Forma de los ojos (¡jueguen con estos numeros!) ─────────────────────────
#define OJO_ANCHO        30   // pixeles
#define OJO_ALTO         40
#define OJO_RADIO        11   // que tan redondeadas son las esquinas
#define OJO_SEPARACION   22   // espacio entre los dos ojos
#define OJO_ALTO_CERRADO 3    // grosor de la rayita cuando esta cerrado
#define OJOS_Y           60   // altura del centro de los ojos (la pantalla mide 128)
#define PESTANA_GROSOR   2

// Hasta donde se pueden mover los ojos
#define MAX_X  ((LCD_H_RES - 2 * OJO_ANCHO - OJO_SEPARACION) / 2)   // 23
#define MAX_Y  12

#define ABAJO_DESDE      86   // zona de abajo de los ojos, para textos y ondas
#define CUADRO_MS        30   // cada cuanto se dibuja un cuadro de la animacion

// ─── Color del LED para cada emocion (¡jueguen con estos colores!) ───────────
// LED(rojo, verde, azul), de 0 a 255
typedef struct { uint8_t r, g, b; } color_led_t;
#define LED(r, g, b) ((color_led_t){ (r), (g), (b) })

#define LED_TRANQUILA  LED(0, 180, 255)     // celeste
#define LED_CONTENTA   LED(255, 110, 0)     // naranja
#define LED_GUINO      LED(150, 0, 255)     // violeta
#define LED_SORPRESA   LED(255, 255, 255)   // blanco
#define LED_ENOJADA    LED(120, 0, 20)      // bordo
#define LED_TRISTE     LED(0, 255, 0)       // verde
#define LED_MAREADA    LED(255, 255, 255)   // blanco
#define LED_ENAMORADA  LED(255, 0, 0)       // rojo
#define LED_VERANO     LED(255, 200, 0)     // amarillo
#define LED_SOSPECHA   LED(0, 0, 255)       // azul
#define LED_SUENO      LED(0, 0, 0)         // apagado

// ─── Colores de los dibujos ──────────────────────────────────────────────────
#define COLOR_CORAZON   COLOR(255, 40, 80)
#define COLOR_LAGRIMA   COLOR(190, 230, 255)
#define COLOR_MARCO     COLOR(255, 120, 190)   // marco de los lentes (rosa)
#define COLOR_VIDRIO    COLOR(80, 25, 60)      // vidrio color ciruela
#define COLOR_BRILLO    COLOR(255, 190, 225)   // brillitos del vidrio

// ─── Como esta la cara en este momento ───────────────────────────────────────
typedef struct {
    float x, y;               // corrimiento desde el centro (adonde mira)
    float ancho, alto;        // tamano de cada ojo
    float abre_izq, abre_der; // 0 = cerrado, 1 = abierto
    float feliz;              // 0 a 1: ojitos felices ^ ^ y sonrisa
    float enojo;              // 0 a 1: cejas hacia adentro
    float triste;             // 0 a 1: cejas hacia afuera
    float espiral;            // 0 o 1: ojos en espiral (mareada)
    float giro;               // cuanto giro la espiral
    float lagrima;            // 0 a 1: por donde va cayendo la lagrima
    float zzz;                // 0 a 1: por donde va subiendo la "Z"
    float corazon;            // 0 o 1: ojos de corazon (enamorada)
    float corazoncitos;       // 0 a 1: por donde van subiendo los corazoncitos
    float lentes;             // 0 a 1: lentes de sol (bajan desde arriba)
    float asombro;            // 0 a 1: boca en "O"
    float sospecha;           // 0 a 1: boca torcida
    float sueno;              // 0 a 1: boquita de dormida
    float ondas;              // 0 a 1: ondas de WiFi abajo
    float texto;              // 0 a 1: texto abajo de los ojos
    float fase;               // "reloj" de las animaciones (cada +1 es una vuelta)
    float comida_x, comida_y; // donde esta la comida que le estan dando
} cara_t;

static const cara_t CARA_NORMAL = {
    .ancho = OJO_ANCHO, .alto = OJO_ALTO, .abre_izq = 1, .abre_der = 1,
};

static cara_t cara;
static color_t color_ojos, color_pestanas, color_boca, color_texto, color_fondo;

static char texto_abajo[64];          // texto de abajo de los ojos (eyes_decir)
static int  texto_escala = 2;
static const char *globo = NULL;      // texto de arriba (lo que dice mientras come, etc.)

static comida_t comida_actual = COMIDA_NINGUNA;
static int mordidas = 0;

// ─── El LED ──────────────────────────────────────────────────────────────────

static color_led_t led_deseado;
static color_led_t led_puesto = { 1, 2, 3 };   // distinto de todo, para que se mande la primera vez
static bool arcoiris = false;

static void poner_led(color_led_t c) { led_deseado = c; }

static void actualizar_led(void)
{
    if (arcoiris) {
        led_arcoiris_paso();
        led_puesto.r = 1;                     // para que al terminar se vuelva a mandar el color
        return;
    }
    if (led_deseado.r != led_puesto.r || led_deseado.g != led_puesto.g || led_deseado.b != led_puesto.b) {
        led_color(led_deseado.r, led_deseado.g, led_deseado.b);
        led_puesto = led_deseado;
    }
}

// ─── Ayudantes ───────────────────────────────────────────────────────────────

static int azar(int min, int max)
{
    return min + (int)(esp_random() % (uint32_t)(max - min + 1));
}

static void esperar(int ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

static void arco(float cx, float cy, float r, float desde, float hasta, int grosor, color_t c)
{
    for (float a = desde; a <= hasta; a += 0.04f)
        display_fill_rect((int)(cx + r * cosf(a)) - grosor / 2, (int)(cy + r * sinf(a)) - grosor / 2,
                          grosor, grosor, c);
}

static void anillo(int cx, int cy, int r_afuera, int r_adentro, color_t c)
{
    display_fill_circle(cx, cy, r_afuera, c);
    display_fill_circle(cx, cy, r_adentro, color_fondo);
}

// ─── Dibujos de cada parte ───────────────────────────────────────────────────

/*
 * Las pestañas: tres rayitas que salen del borde de arriba del ojo, del lado
 * de afuera, abiertas como un abanico (lado = -1 ojo izquierdo, +1 derecho).
 * Si el ojo esta casi cerrado, apuntan para abajo (ojito cerrado).
 */
static const struct { int base_x, base_y, punta_x, punta_y; } pestanas[3] = {
    { 13, 3, 21, -5 },
    {  8, 1, 13, -8 },
    {  2, 0,  4, -9 },
};

static void dibujar_pestanas(float cx, float arriba, int lado, float abre)
{
    int hacia = (abre < 0.3f) ? -1 : 1;
    for (int k = 0; k < 3; k++) {
        display_draw_line((int)(cx + lado * pestanas[k].base_x), (int)(arriba + pestanas[k].base_y * hacia),
                          (int)(cx + lado * pestanas[k].punta_x), (int)(arriba + pestanas[k].punta_y * hacia),
                          PESTANA_GROSOR, color_pestanas);
    }
}

static void dibujar_espiral(float cx, float cy, float radio, float giro)
{
    const float fin = 2.5f * 2 * PI;
    for (float a = 0; a < fin; a += 0.05f) {
        float r = radio * a / fin;
        display_fill_rect((int)(cx + r * cosf(a + giro)), (int)(cy + r * sinf(a + giro)), 2, 2, color_ojos);
    }
}

static void dibujar_corazon(float cx, float cy, float r, color_t c)
{
    if (r < 1) r = 1;
    int y0 = (int)(cy - r * 0.5f);
    display_fill_circle((int)(cx - r), y0, (int)(r + 0.5f), c);
    display_fill_circle((int)(cx + r), y0, (int)(r + 0.5f), c);
    display_fill_triangle((int)(cx - 2 * r), y0, (int)(cx + 2 * r), y0, (int)cx, (int)(y0 + 2 * r), c);
}

static void dibujar_lagrima(int x, int y)
{
    display_fill_circle(x, y, 3, COLOR_LAGRIMA);
    display_fill_triangle(x - 3, y, x + 3, y, x, y - 7, COLOR_LAGRIMA);
}

static void dibujar_z(int x, int y, int s)
{
    display_fill_rect(x, y, s, 2, color_texto);
    display_fill_rect(x, y + s - 2, s, 2, color_texto);
    for (int k = 0; k < s; k++) display_fill_rect(x + s - 1 - k, y + k, 2, 1, color_texto);
}

// Lentes de sol "ojo de gato": marco rosa con las puntas de afuera para arriba,
// vidrio color ciruela con brillitos y una piedrita brillante en cada punta.
// "puestos" va de 0 (arriba, afuera de la pantalla) a 1 (en los ojos).
static void dibujar_lentes(float cx_izq, float cx_der, float cy, float puestos)
{
    const int w = OJO_ANCHO + 10, h = 28;
    int arriba = (int)(cy - 2 - h / 2 - (1 - puestos) * 80);
    int xi = (int)(cx_izq - w / 2.0f), xd = (int)(cx_der - w / 2.0f);

    display_fill_rect(0, arriba + 4, xi, 2, COLOR_MARCO);                       // patillas
    display_fill_rect(xd + w, arriba + 4, LCD_H_RES - (xd + w), 2, COLOR_MARCO);
    display_fill_rect(xi + w, arriba + 3, xd - (xi + w), 2, COLOR_MARCO);       // puente finito

    for (int lado = -1; lado <= 1; lado += 2) {
        int x = (lado < 0) ? xi : xd;
        int afuera = (lado < 0) ? x : x + w;              // borde de afuera del lente

        // Punta de "ojo de gato": un triangulo que sube hacia afuera
        display_fill_triangle(afuera - lado * 16, arriba, afuera - lado * 2, arriba + 14,
                              afuera + lado * 7, arriba - 7, COLOR_MARCO);

        display_fill_round_rect(x, arriba, w, h, 10, COLOR_MARCO);                // marco
        display_fill_round_rect(x + 2, arriba + 2, w - 4, h - 4, 8, COLOR_VIDRIO); // vidrio
        for (int k = 0; k < 11; k++)                                              // brillitos
            display_fill_rect(x + 8 + k, arriba + 17 - k, 3, 1, COLOR_BRILLO);
        for (int k = 0; k < 5; k++)
            display_fill_rect(x + 18 + k, arriba + 19 - k, 1, 1, COLOR_BRILLO);

        display_fill_circle(afuera + lado * 5, arriba - 5, 2, BLANCO);           // piedrita
        display_set_pixel(afuera + lado * 5, arriba - 9, BLANCO);                 // con su destello
        display_set_pixel(afuera + lado * 9, arriba - 5, BLANCO);
    }
}

// lado: -1 = ojo izquierdo, +1 = ojo derecho
static void dibujar_ojo(float cx, float cy, float ancho, float alto, float abre, int lado)
{
    if (cara.espiral > 0.5f) {
        dibujar_espiral(cx, cy, fminf(ancho, alto) / 2 - 1, cara.giro);
        return;
    }
    if (cara.corazon > 0.5f) {
        dibujar_corazon(cx, cy, ancho / 4, COLOR_CORAZON);
        return;
    }

    int h = (int)(OJO_ALTO_CERRADO + (alto - OJO_ALTO_CERRADO) * abre);
    int w = (int)ancho;
    int x = (int)(cx - w / 2.0f);
    int y = (int)(cy - h / 2.0f);
    int x_adentro = (lado < 0) ? x + w + 2 : x - 2;
    int x_afuera  = (lado < 0) ? x - 2     : x + w + 2;

    display_fill_round_rect(x, y, w, h, OJO_RADIO, color_ojos);

    // Ojitos felices: una "loma" del color del fondo tapa la parte de abajo
    if (cara.feliz > 0.01f && h > OJO_ALTO_CERRADO) {
        int tapa = (int)(h * 0.6f * cara.feliz);
        display_fill_round_rect(x - 4, y + h - tapa, w + 8, h, (w + 8) / 2, color_fondo);
    }
    // Enojada: corta la esquina de arriba que da a la nariz
    if (cara.enojo > 0.01f)
        display_fill_triangle(x_afuera, y - 2, x_adentro, y - 2, x_adentro, y + (int)(h * 0.55f * cara.enojo), color_fondo);
    // Triste: corta la esquina de arriba del lado de afuera
    if (cara.triste > 0.01f)
        display_fill_triangle(x_adentro, y - 2, x_afuera, y - 2, x_afuera, y + (int)(h * 0.5f * cara.triste), color_fondo);

    dibujar_pestanas(cx, y, lado, abre);

    if (cara.lagrima > 0.01f && cara.lagrima < 0.99f) {
        int lx = (int)(cx + lado * w * 0.25f);
        int desde = y + h + 6, hasta = LCD_V_RES + 8;
        dibujar_lagrima(lx, desde + (int)((hasta - desde) * cara.lagrima));
    }
}

// ─── La boca ─────────────────────────────────────────────────────────────────
// Cada emocion tiene su boca. Gana la emocion mas fuerte; si ninguna llega a
// 0,3, Flufi esta tranquila y lleva una sonrisita sencilla.

typedef enum {
    BOCA_SENCILLA, BOCA_SONRISA, BOCA_TRISTE, BOCA_ENOJADA, BOCA_O, BOCA_MAREADA,
    BOCA_BESO, BOCA_CANCHERA, BOCA_SOSPECHA, BOCA_DORMIDA,
} boca_t;

static void dibujar_boca(float cy, float perspectiva)
{
    float mas_fuerte = 0.3f;
    boca_t boca = BOCA_SENCILLA;
#define CANDIDATA(peso, tipo) if ((peso) > mas_fuerte) { mas_fuerte = (peso); boca = (tipo); }
    CANDIDATA(cara.feliz,    BOCA_SONRISA);
    CANDIDATA(cara.triste,   BOCA_TRISTE);
    CANDIDATA(cara.enojo,    BOCA_ENOJADA);
    CANDIDATA(cara.asombro,  BOCA_O);
    CANDIDATA(cara.espiral,  BOCA_MAREADA);
    CANDIDATA(cara.corazon,  BOCA_BESO);
    CANDIDATA(cara.lentes,   BOCA_CANCHERA);
    CANDIDATA(cara.sospecha, BOCA_SOSPECHA);
    CANDIDATA(cara.sueno,    BOCA_DORMIDA);
#undef CANDIDATA

    float mx = LCD_H_RES / 2.0f + cara.x;

    if (boca == BOCA_SONRISA) {        // crece con la felicidad
        float boca_y = cy + 8 + (1 - cara.feliz) * 8;
        arco(mx, boca_y, 6 + 8 * cara.feliz, 0.2f * PI, 0.8f * PI, 3, color_boca);
        return;
    }

    // ¿Donde terminan los ojos? La boca va justo abajo
    float mitad_ojo;
    if (cara.espiral > 0.5f)      mitad_ojo = fminf(cara.ancho, cara.alto) / 2;
    else if (cara.corazon > 0.5f) mitad_ojo = 1.5f * cara.ancho / 4;
    else {
        float abre = (boca == BOCA_SENCILLA) ? 1 : fmaxf(cara.abre_izq, cara.abre_der);
        float alto = cara.alto * (1 + fabsf(perspectiva));
        mitad_ojo = (OJO_ALTO_CERRADO + (alto - OJO_ALTO_CERRADO) * abre) / 2;
    }
    if (cara.lentes > 0.5f) mitad_ojo = fmaxf(mitad_ojo, 14);
    float y = cy + mitad_ojo + 4;

    switch (boca) {
    case BOCA_SENCILLA:
        arco(mx, y - 4, 7, 0.25f * PI, 0.75f * PI, 2, color_boca);
        break;
    case BOCA_TRISTE:
        arco(mx, y + 11, 11, 1.2f * PI, 1.8f * PI, 3, color_boca);
        break;
    case BOCA_ENOJADA:
        for (int i = 0; i < 6; i++) {
            int x0 = (int)(mx - 12 + i * 4);
            display_draw_line(x0, (int)(y + (i % 2 ? 4 : 1)), x0 + 4, (int)(y + (i % 2 ? 1 : 4)), 2, color_boca);
        }
        break;
    case BOCA_O:
        anillo((int)mx, (int)(y + 5), 3 + (int)(2 * cara.asombro), 1 + (int)(2 * cara.asombro), color_boca);
        break;
    case BOCA_MAREADA:
        for (int dx = -12; dx <= 12; dx++)
            display_fill_rect((int)mx + dx, (int)(y + 2 + 2 * sinf(dx * 0.5f + cara.giro)), 2, 2, color_boca);
        break;
    case BOCA_BESO:
        arco(mx - 2, y + 4, 4, -0.5f * PI, 0.5f * PI, 2, color_boca);
        arco(mx - 2, y + 11, 4, -0.5f * PI, 0.5f * PI, 2, color_boca);
        break;
    case BOCA_CANCHERA:
        display_draw_line((int)(mx - 8), (int)(y + 4), (int)(mx + 3), (int)(y + 4), 2, color_boca);
        display_draw_line((int)(mx + 3), (int)(y + 4), (int)(mx + 9), (int)y, 2, color_boca);
        break;
    case BOCA_SOSPECHA:
        display_draw_line((int)(mx - 6), (int)(y + 3), (int)(mx + 6), (int)(y + 1), 2, color_boca);
        break;
    case BOCA_DORMIDA:
        anillo((int)mx, (int)(y + 3), 3, 1, color_boca);
        break;
    default:
        break;
    }
}

// ─── Dibujar un cuadro ───────────────────────────────────────────────────────

static void dibujar_ondas(void)
{
    const float ox = LCD_H_RES / 2.0f, oy = 120;
    int ondas = ((int)(cara.fase * 4)) % 4;
    display_fill_circle((int)ox, (int)oy, 2, color_texto);
    for (int k = 0; k < ondas; k++)
        arco(ox, oy, 8 + 6 * k, 1.25f * PI, 1.75f * PI, 2, color_texto);
}

static void dibujar(void)
{
    // Perspectiva: el ojo del lado hacia donde mira se ve un poquito mas grande
    float p = (cara.x / MAX_X) * 0.12f;
    float cy     = OJOS_Y + cara.y;
    float cx_izq = LCD_H_RES / 2.0f - OJO_SEPARACION / 2.0f - cara.ancho / 2.0f + cara.x;
    float cx_der = LCD_H_RES / 2.0f + OJO_SEPARACION / 2.0f + cara.ancho / 2.0f + cara.x;

    display_clear(color_fondo);
    dibujar_ojo(cx_izq, cy, cara.ancho, cara.alto * (1 - p), cara.abre_izq, -1);
    dibujar_ojo(cx_der, cy, cara.ancho, cara.alto * (1 + p), cara.abre_der, +1);

    // La boca (salvo cuando abajo hay un texto o las ondas)
    if (cara.texto < 0.5f && cara.ondas < 0.5f) dibujar_boca(cy, p);

    // Corazoncitos que suben por toda la pantalla
    if (cara.corazoncitos > 0.01f && cara.corazoncitos < 0.99f) {
        float t1 = cara.corazoncitos, t2 = fmodf(cara.corazoncitos + 0.5f, 1.0f);
        float recorrido = LCD_V_RES + 20;
        dibujar_corazon(10,              LCD_V_RES + 6 - recorrido * t1, 3, COLOR_CORAZON);
        dibujar_corazon(LCD_H_RES - 11,  LCD_V_RES + 6 - recorrido * t1, 3, COLOR_CORAZON);
        dibujar_corazon(LCD_H_RES / 2.0f, LCD_V_RES + 6 - recorrido * t2, 3, COLOR_CORAZON);
    }

    if (cara.lentes > 0.01f) dibujar_lentes(cx_izq, cx_der, cy, cara.lentes);

    if (cara.zzz > 0.01f && cara.zzz < 0.99f) {
        int s = 5 + (int)(7 * cara.zzz);
        dibujar_z((int)(cx_der + cara.ancho / 2 + 2 + 6 * cara.zzz), (int)(cy - 10 - 30 * cara.zzz), s);
    }

    if (comida_actual != COMIDA_NINGUNA)
        comida_dibujar(comida_actual, cara.comida_x, cara.comida_y, 0.9f, mordidas, color_fondo);

    if (cara.ondas > 0.5f) dibujar_ondas();
    if (cara.texto > 0.5f) {
        int alto = display_text_height(texto_abajo, texto_escala);
        display_draw_text_centered(texto_abajo, ABAJO_DESDE + (LCD_V_RES - ABAJO_DESDE - alto) / 2,
                                   texto_escala, color_texto);
    }
    if (globo != NULL) display_draw_text_centered(globo, 4, 2, color_texto);

    display_flush();
    actualizar_led();
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

        cara.x            = mezclar(inicio.x,            destino.x,            t);
        cara.y            = mezclar(inicio.y,            destino.y,            t);
        cara.ancho        = mezclar(inicio.ancho,        destino.ancho,        t);
        cara.alto         = mezclar(inicio.alto,         destino.alto,         t);
        cara.abre_izq     = mezclar(inicio.abre_izq,     destino.abre_izq,     t);
        cara.abre_der     = mezclar(inicio.abre_der,     destino.abre_der,     t);
        cara.feliz        = mezclar(inicio.feliz,        destino.feliz,        t);
        cara.enojo        = mezclar(inicio.enojo,        destino.enojo,        t);
        cara.triste       = mezclar(inicio.triste,       destino.triste,       t);
        cara.espiral      = mezclar(inicio.espiral,      destino.espiral,      t);
        cara.giro         = mezclar(inicio.giro,         destino.giro,         t);
        cara.lagrima      = mezclar(inicio.lagrima,      destino.lagrima,      t);
        cara.zzz          = mezclar(inicio.zzz,          destino.zzz,          t);
        cara.corazon      = mezclar(inicio.corazon,      destino.corazon,      t);
        cara.corazoncitos = mezclar(inicio.corazoncitos, destino.corazoncitos, t);
        cara.lentes       = mezclar(inicio.lentes,       destino.lentes,       t);
        cara.asombro      = mezclar(inicio.asombro,      destino.asombro,      t);
        cara.sospecha     = mezclar(inicio.sospecha,     destino.sospecha,     t);
        cara.sueno        = mezclar(inicio.sueno,        destino.sueno,        t);
        cara.ondas        = mezclar(inicio.ondas,        destino.ondas,        t);
        cara.texto        = mezclar(inicio.texto,        destino.texto,        t);
        cara.fase         = mezclar(inicio.fase,         destino.fase,         t);
        cara.comida_x     = mezclar(inicio.comida_x,     destino.comida_x,     t);
        cara.comida_y     = mezclar(inicio.comida_y,     destino.comida_y,     t);

        dibujar();
        xTaskDelayUntil(&ultimo, pdMS_TO_TICKS(CUADRO_MS));
    }
}

static void mover_a(float x, float y, int ms)
{
    cara_t d = cara;
    d.x = x;
    d.y = y;
    animar(d, ms);
}

static void cerrar_y_abrir(bool izq, bool der, int cerrado_ms)
{
    cara_t abierta = cara, cerrada = cara;
    if (izq) cerrada.abre_izq = 0;
    if (der) cerrada.abre_der = 0;
    animar(cerrada, 70);
    esperar(cerrado_ms);
    animar(abierta, 110);
}

// ─── Juegos (cada uno con el color de LED de su emocion) ────────────────────

static void juego_mirar(void)
{
    poner_led(LED_TRANQUILA);
    mover_a(azar(-MAX_X, MAX_X), azar(-MAX_Y, MAX_Y), azar(150, 400));
    esperar(azar(400, 1500));
}

static void juego_centro(void)
{
    poner_led(LED_TRANQUILA);
    animar(CARA_NORMAL, 300);
    esperar(azar(300, 1000));
}

static void juego_doble_parpadeo(void)
{
    poner_led(LED_TRANQUILA);
    cerrar_y_abrir(true, true, 30);
    esperar(80);
    cerrar_y_abrir(true, true, 30);
}

static void juego_guino(void)
{
    poner_led(LED_GUINO);
    cara_t d = cara;
    d.feliz = 0.5f;
    animar(d, 150);
    bool izquierdo = azar(0, 1);
    cerrar_y_abrir(izquierdo, !izquierdo, 350);
    d = cara;
    d.feliz = 0;
    animar(d, 150);
}

static void juego_sorpresa(void)
{
    poner_led(LED_SORPRESA);
    cara_t d = cara;
    d.ancho = OJO_ANCHO + 6;
    d.alto  = OJO_ALTO + 8;
    d.y     = -6;
    d.asombro = 1;
    animar(d, 120);
    esperar(azar(500, 900));
    d.ancho = OJO_ANCHO;
    d.alto  = OJO_ALTO;
    d.y     = 0;
    d.asombro = 0;
    animar(d, 250);
}

static void juego_contento(void)
{
    poner_led(LED_CONTENTA);
    cara_t d = CARA_NORMAL;
    d.feliz = 1;
    animar(d, 200);
    for (int i = 0; i < 3; i++) {
        mover_a(0, -6, 120);
        mover_a(0, 0, 120);
    }
    esperar(400);
    animar(CARA_NORMAL, 200);
}

static void juego_girar(void)
{
    poner_led(LED_TRANQUILA);
    float r_x = MAX_X * 0.7f, r_y = MAX_Y * 0.8f;
    int sentido = azar(0, 1) ? 1 : -1;
    mover_a(r_x, 0, 150);
    for (int i = 1; i <= 16; i++) {
        float ang = sentido * i * 2 * PI / 16;
        mover_a(r_x * cosf(ang), r_y * sinf(ang), 60);
    }
    animar(CARA_NORMAL, 200);
}

static void juego_sospecha(void)
{
    poner_led(LED_SOSPECHA);
    cara_t d = CARA_NORMAL;
    d.abre_izq = d.abre_der = 0.35f;
    d.sospecha = 1;
    animar(d, 200);
    mover_a(-MAX_X, 0, 500);
    esperar(300);
    mover_a(MAX_X, 0, 700);
    esperar(300);
    animar(CARA_NORMAL, 250);
}

static void juego_sueno(void)
{
    poner_led(LED_SUENO);
    cara_t d = CARA_NORMAL;
    d.y = 4;
    d.abre_izq = d.abre_der = 0.3f;
    d.sueno = 1;
    animar(d, 1200);
    d.abre_izq = d.abre_der = 0;
    for (int i = 0; i < 2; i++) {            // duerme un ratito, con la "Z"
        cara.zzz = 0;
        d.zzz = 1;
        animar(d, 1200);
        d.zzz = 0;
    }
    cara.zzz = 0;
    poner_led(LED_TRANQUILA);
    animar(CARA_NORMAL, 100);                // ¡se desperto!
    juego_doble_parpadeo();
}

static void juego_pasear(void)
{
    poner_led(LED_TRANQUILA);
    int lado = azar(0, 1) ? 1 : -1;
    mover_a(lado * MAX_X, azar(-MAX_Y, MAX_Y), 400);
    esperar(300);
    mover_a(-lado * MAX_X, azar(-MAX_Y, MAX_Y), 700);
    esperar(300);
    mover_a(0, 0, 400);
}

static void juego_mareado(void)
{
    poner_led(LED_MAREADA);
    cara_t d = CARA_NORMAL;
    d.espiral = 1;
    animar(d, 60);
    for (int i = 0; i < 30; i++) {
        d.giro += 0.9f;
        d.x = 8 * cosf(i * 0.5f);
        d.y = 5 * sinf(i * 0.5f);
        animar(d, 60);
    }
    cara = CARA_NORMAL;                      // vuelve en si
    cara.abre_izq = cara.abre_der = 0;
    poner_led(LED_TRANQUILA);
    dibujar();
    for (int i = 0; i < 2; i++) {
        mover_a(-6, 0, 90);
        mover_a(6, 0, 90);
    }
    animar(CARA_NORMAL, 200);
    juego_doble_parpadeo();
}

static void juego_enojado(void)
{
    poner_led(LED_ENOJADA);
    cara_t d = CARA_NORMAL;
    d.enojo = 1;
    d.abre_izq = d.abre_der = 0.8f;
    d.y = 2;
    animar(d, 250);
    for (int i = 0; i < 6; i++) {
        mover_a(-3, 2, 50);
        mover_a(3, 2, 50);
    }
    mover_a(0, 2, 50);
    esperar(azar(800, 1500));
    animar(CARA_NORMAL, 300);
}

static void juego_llorar(void)
{
    poner_led(LED_TRISTE);
    cara_t d = CARA_NORMAL;
    d.triste = 1;
    d.abre_izq = d.abre_der = 0.75f;
    d.y = 3;
    animar(d, 400);
    int lagrimas = azar(2, 4);
    for (int i = 0; i < lagrimas; i++) {
        cara.lagrima = 0;
        d.lagrima = 1;
        animar(d, 900);
        d.lagrima = 0;
        mover_a(0, 1, 100);
        mover_a(0, 3, 100);
    }
    cara.lagrima = 0;
    esperar(500);
    animar(CARA_NORMAL, 400);
}

static void juego_enamorado(void)
{
    poner_led(LED_ENAMORADA);
    cara_t d = CARA_NORMAL;
    d.corazon = 1;
    animar(d, 100);
    for (int i = 0; i < 12; i++) {
        bool grande = (i % 2 == 0);
        d.ancho = grande ? OJO_ANCHO + 6 : OJO_ANCHO - 2;
        d.x     = grande ? -4 : 4;
        if (i % 4 == 0) cara.corazoncitos = 0;
        d.corazoncitos = (i % 4 + 1) / 4.0f;
        animar(d, grande ? 120 : 200);
    }
    cara.corazoncitos = 0;
    animar(CARA_NORMAL, 150);
    juego_doble_parpadeo();
}

static void juego_verano(void)
{
    poner_led(LED_VERANO);
    cara_t d = CARA_NORMAL;
    animar(d, 150);
    d.lentes = 1;
    d.abre_izq = d.abre_der = 0.55f;
    animar(d, 500);
    esperar(300);
    for (int i = 0; i < 4; i++) {
        mover_a(-6, 2, 300);
        mover_a(6, -2, 300);
    }
    mover_a(0, 0, 200);
    esperar(500);
    d = cara;
    d.lentes = 0;
    d.abre_izq = d.abre_der = 1;
    animar(d, 400);
    poner_led(LED_GUINO);                    // termina guiñando un ojo
    d.feliz = 1;
    animar(d, 150);
    cerrar_y_abrir(false, true, 300);
    animar(CARA_NORMAL, 200);
}

typedef struct {
    const char *nombre;
    void (*hacer)(void);
    int peso;   // mas peso = sale mas seguido
} juego_t;

static const juego_t juegos[] = {
    { "mirar",          juego_mirar,          10 },
    { "centro",         juego_centro,          4 },
    { "doble parpadeo", juego_doble_parpadeo,  3 },
    { "guino",          juego_guino,           2 },
    { "sorpresa",       juego_sorpresa,        2 },
    { "contento",       juego_contento,        2 },
    { "girar",          juego_girar,           1 },
    { "sospecha",       juego_sospecha,        1 },
    { "sueno",          juego_sueno,           1 },
    { "pasear",         juego_pasear,          2 },
    { "mareado",        juego_mareado,         1 },
    { "enojado",        juego_enojado,         1 },
    { "llorar",         juego_llorar,          1 },
    { "enamorado",      juego_enamorado,       2 },
    { "verano",         juego_verano,          2 },
};
#define CANT_JUEGOS ((int)(sizeof(juegos) / sizeof(juegos[0])))

// ─── Funciones publicas ──────────────────────────────────────────────────────

void eyes_init(color_t ojos, color_t pestanas_color, color_t boca, color_t texto, color_t fondo)
{
    color_ojos     = ojos;
    color_pestanas = pestanas_color;
    color_boca     = boca;
    color_texto    = texto;
    color_fondo    = fondo;
    cara = CARA_NORMAL;
    poner_led(LED_TRANQUILA);
    dibujar();
}

void eyes_parpadear(void)
{
    cerrar_y_abrir(true, true, 40);
}

void eyes_jugar(void)
{
    int total = 0;
    for (int i = 0; i < CANT_JUEGOS; i++) total += juegos[i].peso;

    int r = azar(0, total - 1);
    for (int i = 0; i < CANT_JUEGOS; i++) {
        if (r < juegos[i].peso) {
            ESP_LOGI(TAG, "Juego: %s", juegos[i].nombre);
            juegos[i].hacer();
            poner_led(LED_TRANQUILA);
            return;
        }
        r -= juegos[i].peso;
    }
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
    esperar(ms / 2);
    eyes_parpadear();
    esperar(ms / 2);
    d = cara;
    d.texto = 0;
    animar(d, 150);
}

// ─── Reacciones con arco iris ────────────────────────────────────────────────

void eyes_saludar(void)
{
    ESP_LOGI(TAG, "¡Hola! Saludando");
    arcoiris = true;
    animar(CARA_NORMAL, 150);
    juego_sorpresa();
    juego_contento();
    arcoiris = false;
    poner_led(LED_TRANQUILA);
}

void eyes_decir_que_no(void)
{
    ESP_LOGI(TAG, "Estoy llena");
    arcoiris = true;
    cara_t d = CARA_NORMAL;
    d.sospecha = 1;
    animar(d, 200);
    for (int i = 0; i < 3; i++) {            // no, no, no
        mover_a(-10, 0, 110);
        mover_a(10, 0, 110);
    }
    mover_a(0, 0, 110);
    eyes_decir("Estoy\nllena!", 2, 2000);
    animar(CARA_NORMAL, 300);
    arcoiris = false;
    poner_led(LED_TRANQUILA);
}

// La comida entra, se acerca a la boca y Flufi le da 3 mordiscos (o 3 sorbos)
static void comer_o_tomar(comida_t c)
{
    bool bebida = comida_es_bebida(c);
    arcoiris = true;

    // Flufi se corre un poco a la izquierda; la comida entra por la derecha
    cara_t d = CARA_NORMAL;
    d.x = -18;
    d.comida_x = 160;
    d.comida_y = 96;
    cara.comida_x = d.comida_x;
    cara.comida_y = d.comida_y;
    mordidas = 0;
    globo = NULL;
    comida_actual = c;
    animar(d, 300);

    d.comida_x = 104;                        // 1. llega
    d.asombro = 0.5f;
    animar(d, 500);
    globo = bebida ? "Sed!" : "Mmm!";
    esperar(600);
    globo = NULL;

    float boca_x = LCD_H_RES / 2.0f + d.x;   // 2. se acerca a la boca
    d.comida_x = boca_x + 26;
    d.comida_y = 100;
    d.asombro = bebida ? 0.35f : 1;
    animar(d, 400);

    for (int m = 1; m <= 3; m++) {           // 3. mordiscos o sorbos
        if (bebida) {
            globo = (c == COMIDA_MATE && m == 3) ? "Slurp" : "Glu";
            d.comida_y = 96;
            animar(d, 200);
            mordidas = m;
            d.comida_y = 100;
            animar(d, 200);
            globo = NULL;
        } else {
            d.asombro = 0;
            animar(d, 120);
            if (m < 3) mordidas = m;
            else       comida_actual = COMIDA_NINGUNA;
            d.asombro = (m < 3) ? 1 : 0;
            animar(d, 160);
        }
        esperar(150);
    }

    if (bebida) {                            // se llevan el vaso vacio
        d.comida_x = 160;
        d.asombro = 0;
        animar(d, 400);
    } else {                                 // mastica
        for (int i = 0; i < 4; i++) {
            d.asombro = 0.45f; d.y = -1; animar(d, 110);
            d.asombro = 0;     d.y = 0;  animar(d, 110);
        }
    }
    comida_actual = COMIDA_NINGUNA;

    d.feliz = 1;                             // 4. ¡rico!
    globo = bebida ? "Ahh!" : "¡Rico!";
    animar(d, 250);
    esperar(1200);
    globo = NULL;
    animar(CARA_NORMAL, 400);
    arcoiris = false;
    poner_led(LED_TRANQUILA);
}

void eyes_comer(comida_t comida)
{
    if (comida == COMIDA_NINGUNA || comida_es_bebida(comida)) return;
    ESP_LOGI(TAG, "¡A comer!");
    comer_o_tomar(comida);
}

void eyes_tomar(comida_t bebida)
{
    if (!comida_es_bebida(bebida)) return;
    ESP_LOGI(TAG, "¡A tomar!");
    comer_o_tomar(bebida);
}
