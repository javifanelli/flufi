// ─────────────────────────────────────────────────────────────────────────────
//  Los personajes de los modos (lo que comparten todos)
// ─────────────────────────────────────────────────────────────────────────────
//
//  Los personajes estan en animales.c, fantasia.c y ejercicio.c. Aca esta lo
//  que usan todos: las herramientas para dibujar, como se cambia de disfraz y
//  como se busca la informacion de cada personaje.
//
#include "eyes_interno.h"

#include "esp_log.h"

static const char *TAG = "PERSONAJES";

eyes_personaje_t personaje_actual = PERSONAJE_NINGUNO;

// ─── Herramientas de dibujo (aceptan numeros con coma) ───────────────────────

void rect(float x, float y, float w, float h, color_t c)
{
    display_fill_rect((int)x, (int)y, (int)(w < 1 ? 1 : w), (int)(h < 1 ? 1 : h), c);
}

void circulo(float cx, float cy, float r, color_t c)
{
    display_fill_circle((int)cx, (int)cy, (int)(r + 0.5f), c);
}

void triangulo(float x0, float y0, float x1, float y1, float x2, float y2, color_t c)
{
    display_fill_triangle((int)x0, (int)y0, (int)x1, (int)y1, (int)x2, (int)y2, c);
}

void linea(float x0, float y0, float x1, float y1, int grosor, color_t c)
{
    display_draw_line((int)x0, (int)y0, (int)x1, (int)y1, grosor, c);
}

// Un ovalo relleno, renglon por renglon
void elipse(float cx, float cy, float rx, float ry, color_t c)
{
    if (rx < 1 || ry < 1) return;
    for (int dy = -(int)ry; dy <= (int)ry; dy++) {
        float medio = rx * sqrtf(1 - (dy * dy) / (ry * ry));
        rect(cx - medio, cy + dy, 2 * medio + 1, 1, c);
    }
}

// Estrellita de 4 puntas
void estrella(float x, float y, float r, color_t c)
{
    rect(x - r, y, 2 * r + 1, 1, c);
    rect(x, y - r, 1, 2 * r + 1, c);
    rect(x - 1, y - 1, 3, 3, c);
}

// Va de -1 a 1 y vuelve, al ritmo de cara.fase ("veces" vueltas por cada +1)
float onda(float veces)
{
    return sinf(cara.fase * veces * 2 * PI);
}

// ─── Buscar un personaje ─────────────────────────────────────────────────────

static bool es_valido(eyes_personaje_t p)
{
    return p > PERSONAJE_NINGUNO && p < CANT_PERSONAJES;
}

// La informacion de un personaje (esta en la lista de su modo)
static const personaje_info_t *info(eyes_personaje_t p)
{
    if (!es_valido(p)) return NULL;
    if (p >= PRIMER_EJERCICIO) return &personajes_ejercicio[p - PRIMER_EJERCICIO];
    if (p >= PRIMER_FANTASIA)  return &personajes_fantasia[p - PRIMER_FANTASIA];
    return &personajes_animales[p - PRIMER_ANIMAL];
}

// La cara de base de un personaje (tamaño y altura de los ojos)
static cara_t cara_de(eyes_personaje_t p)
{
    cara_t c = CARA_NORMAL;
    const personaje_info_t *i = info(p);
    if (i == NULL) return c;
    c.ancho = i->ancho;
    c.alto  = i->alto;
    c.y     = i->y;
    return c;
}

// ─── Lo que usa eyes.c ───────────────────────────────────────────────────────

bool personaje_lleva_rulos(void)
{
    const personaje_info_t *i = info(personaje_actual);
    return i == NULL || i->rulos;
}

void dibujar_personaje_atras(float mx, float cy)
{
    const personaje_info_t *i = info(personaje_actual);
    if (i != NULL) i->atras(mx, cy);
}

void dibujar_personaje_adelante(float mx, float cy)
{
    const personaje_info_t *i = info(personaje_actual);
    if (i != NULL) i->adelante(mx, cy);
}

// ─── Lo que usa main.c ───────────────────────────────────────────────────────

static void limpiar(eyes_personaje_t p)
{
    const personaje_info_t *i = info(p);
    if (i != NULL && i->limpiar != NULL) i->limpiar();
    globo = NULL;
}

// Cierra los ojos, cambia de disfraz y los vuelve a abrir
static void cambiar_a(eyes_personaje_t nuevo)
{
    cara_t d = cara;
    d.abre_izq = d.abre_der = 0;
    animar(d, 150);

    limpiar(personaje_actual);
    personaje_actual = nuevo;
    limpiar(nuevo);

    const personaje_info_t *i = info(nuevo);
    color_fondo = (i != NULL && i->fondo != NEGRO) ? i->fondo : color_fondo_normal;
    if (i != NULL) poner_led(i->led);
    else           poner_led_tranquila();

    cara = cara_de(nuevo);
    cara.abre_izq = cara.abre_der = 0;
    animar(cara_de(nuevo), 200);
}

void eyes_actuar(eyes_personaje_t personaje)
{
    const personaje_info_t *i = info(personaje);
    if (i == NULL) return;
    empezar_accion();
    if (personaje != personaje_actual) cambiar_a(personaje);

    ESP_LOGI(TAG, "Actuando de %s", i->nombre);
    i->actuar();
    limpiar(personaje);
    arcoiris = false;
    poner_led(i->led);
}

void eyes_sacar_disfraz(void)
{
    if (personaje_actual == PERSONAJE_NINGUNO) return;
    cambiar_a(PERSONAJE_NINGUNO);
}
