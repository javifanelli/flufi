// ─────────────────────────────────────────────────────────────────────────────
//  El tiempo de afuera: Flufi cuenta si hace frio, calor, si llueve, si esta
//  nublado o soleado
// ─────────────────────────────────────────────────────────────────────────────
//
//  La cara se aleja (como cuando dice la hora) y al costado aparece un dibujo
//  animado del tiempo (el sol con sus rayos que giran, nubes, gotas que caen...)
//  y abajo el texto: como esta el cielo, la temperatura y si hace frio o calor.
//
#include "eyes_interno.h"

#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"

static const char *TAG = "CLIMA";

// ─── ¡Jueguen con estos numeros! ─────────────────────────────────────────────
#define FRIO_HASTA   14     // con esta temperatura o menos, "¡Frio!"  (grados)
#define CALOR_DESDE  27     // con esta temperatura o mas, "¡Calor!"

// Como esta el cielo (sale del codigo del tiempo)
typedef enum {
    CIELO_DESPEJADO,
    CIELO_ALGUNAS_NUBES,
    CIELO_NUBLADO,
    CIELO_NIEBLA,
    CIELO_LLUVIA,
    CIELO_TORMENTA,
    CIELO_NIEVE,
} cielo_t;

// El codigo del tiempo (WMO) es un numero que usan todos los servicios del mundo
static cielo_t cielo_de(int codigo)
{
    if (codigo <= 1)                  return CIELO_DESPEJADO;      // 0 despejado, 1 casi despejado
    if (codigo == 2)                  return CIELO_ALGUNAS_NUBES;
    if (codigo == 3)                  return CIELO_NUBLADO;
    if (codigo == 45 || codigo == 48) return CIELO_NIEBLA;
    if (codigo >= 95)                 return CIELO_TORMENTA;
    if ((codigo >= 71 && codigo <= 77) || codigo == 85 || codigo == 86) return CIELO_NIEVE;
    if ((codigo >= 51 && codigo <= 67) || (codigo >= 80 && codigo <= 82)) return CIELO_LLUVIA;
    return CIELO_NUBLADO;
}

// Lo que se esta mostrando (el dibujo lo necesita en cada cuadro)
static cielo_t cielo_actual;
static bool    de_dia;

// ─── Los dibujos ─────────────────────────────────────────────────────────────

static void sol(float x, float y, float r)
{
    const color_t amarillo = COLOR(255, 210, 0), clarito = COLOR(255, 240, 140);
    for (int k = 0; k < 8; k++) {                        // rayos que giran
        float a = k * PI / 4 + cara.fase * PI / 2;
        linea(x + (r + 3) * cosf(a), y + (r + 3) * sinf(a), x + (r + 8) * cosf(a), y + (r + 8) * sinf(a), 2, amarillo);
    }
    circulo(x, y, r, amarillo);
    circulo(x - r * 0.3f, y - r * 0.3f, r * 0.35f, clarito);
}

static void luna(float x, float y)
{
    circulo(x, y, 11, COLOR(245, 240, 200));
    circulo(x + 6, y - 4, 9, color_fondo);              // le "come" un pedazo: queda una medialuna
    estrella(x + 14, y + 8, 1 + 2 * fabsf(onda(1)), BLANCO);
    estrella(x - 14, y - 10, 1 + 2 * fabsf(onda(0.7f)), BLANCO);
}

static void nube(float x, float y, color_t c)
{
    circulo(x - 9, y + 3, 8, c);
    circulo(x + 1, y - 3, 11, c);
    circulo(x + 11, y + 3, 8, c);
    display_fill_round_rect((int)(x - 17), (int)(y + 2), 36, 10, 5, c);
}

// Cosas que caen de la nube (gotas o copos), cada una a su ritmo
static void caen(float x, float y, bool copos)
{
    for (int k = 0; k < 5; k++) {
        float t = fmodf(cara.fase * 1.5f + k * 0.37f, 1.0f);
        float px = x - 14 + k * 7, py = y + 14 + t * 16;
        if (copos) {
            estrella(px, py, 2, BLANCO);
        } else {
            const color_t agua = COLOR(90, 160, 255);
            circulo(px, py, 1.5f, agua);
            triangulo(px - 1.5f, py, px + 1.5f, py, px, py - 4, agua);
        }
    }
}

static void dibujar_tiempo(float x, float y)
{
    switch (cielo_actual) {
    case CIELO_DESPEJADO:
        if (de_dia) sol(x, y, 10);
        else        luna(x, y);
        break;
    case CIELO_ALGUNAS_NUBES:
        if (de_dia) sol(x + 8, y - 8, 7);
        else        luna(x + 6, y - 8);
        nube(x - 3, y + 5, COLOR(235, 235, 245));
        break;
    case CIELO_NUBLADO:
        nube(x + 6, y - 8, COLOR(150, 155, 170));       // una nube atras de la otra
        nube(x - 3, y + 4, COLOR(220, 220, 235));
        break;
    case CIELO_NIEBLA:
        for (int k = 0; k < 5; k++) {                   // rayas de niebla que se mueven
            float corre = 4 * onda(0.5f + k * 0.1f);
            rect(x - 20 + corre + (k % 2) * 4, y - 12 + k * 6, 36, 3, COLOR(170, 175, 190));
        }
        break;
    case CIELO_LLUVIA:
        nube(x, y - 6, COLOR(170, 175, 190));
        caen(x, y - 6, false);
        break;
    case CIELO_TORMENTA:
        nube(x, y - 6, COLOR(110, 110, 130));
        caen(x, y - 6, false);
        if (fmodf(cara.fase * 3, 1.0f) < 0.5f) {        // el rayo titila
            const color_t rayo = COLOR(255, 230, 0);
            triangulo(x + 2, y + 2, x - 6, y + 14, x + 1, y + 13, rayo);
            triangulo(x + 1, y + 11, x + 7, y + 11, x - 3, y + 26, rayo);
        }
        break;
    case CIELO_NIEVE:
        nube(x, y - 6, COLOR(225, 225, 240));
        caen(x, y - 6, true);
        break;
    }
}

// ─── Contarlo ────────────────────────────────────────────────────────────────

void eyes_contar_clima(int codigo, int temp_decimas, bool es_de_dia, int ms)
{
    empezar_accion();
    eyes_sacar_disfraz();
    cielo_actual = cielo_de(codigo);
    de_dia = es_de_dia;

    // Como esta el cielo, en palabras
    const char *cielo;
    switch (cielo_actual) {
    case CIELO_DESPEJADO:     cielo = de_dia ? "Soleado" : "Despejado"; break;
    case CIELO_ALGUNAS_NUBES: cielo = de_dia ? "Sol y\nnubes" : "Algunas\nnubes"; break;
    case CIELO_NUBLADO:       cielo = "Nublado";  break;
    case CIELO_NIEBLA:        cielo = "Neblina";  break;
    case CIELO_LLUVIA:        cielo = "¡Llueve!"; break;
    case CIELO_TORMENTA:      cielo = "Tormenta"; break;
    default:                  cielo = "¡Nieva!";  break;
    }

    // Frio o calor (redondeando a grados enteros)
    int grados = (temp_decimas + (temp_decimas >= 0 ? 5 : -5)) / 10;
    const char *sensacion = (grados <= FRIO_HASTA) ? "\n¡Frio!" : (grados >= CALOR_DESDE) ? "\n¡Calor!" : "";

    char texto[64];
    snprintf(texto, sizeof(texto), "%s\n%s%d.%d°C%s", cielo, temp_decimas < 0 ? "-" : "",
             abs(temp_decimas) / 10, abs(temp_decimas) % 10, sensacion);
    ESP_LOGI(TAG, "El tiempo: codigo %d, %d grados", codigo, grados);

    // El LED y la carita, segun el tiempo
    cara_t d = CARA_NORMAL;
    switch (cielo_actual) {
    case CIELO_DESPEJADO:
    case CIELO_ALGUNAS_NUBES:
        poner_led(de_dia ? LED(255, 200, 0) : LED(60, 60, 200));
        d.feliz = 0.6f;
        break;
    case CIELO_LLUVIA:   poner_led(LED(0, 80, 255));    d.triste = 0.4f; break;
    case CIELO_TORMENTA: poner_led(LED(140, 0, 255));   d.triste = 0.4f; break;
    case CIELO_NIEVE:    poner_led(LED(255, 255, 255)); d.asombro = 0.5f; break;
    default:             poner_led(LED(180, 180, 200)); break;   // nublado o niebla
    }
    if (grados <= FRIO_HASTA)  d.abre_izq = d.abre_der = 0.75f;  // ojitos apretados de frio
    animar(d, 250);

    decir_con_dibujo(texto, 2, dibujar_tiempo, ms);

    animar(CARA_NORMAL, 300);
    poner_led_tranquila();
}
