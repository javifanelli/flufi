#ifndef EYES_INTERNO_H
#define EYES_INTERNO_H

// Cosas que comparten eyes.c (ojos, emociones y juegos) y animales.c (los
// animales del modo Animales). NO es parte de lo que usa main.c: eso esta en eyes.h.

#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "eyes.h"
#include "board_config.h"

#define PI ((float)M_PI)

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
    float lejos;              // 0 = cara normal, 1 = cara alejada (chiquita, a la izquierda)
    float ondas;              // 0 a 1: ondas de WiFi al costado
    float texto;              // 0 a 1: texto al costado de la cara (entra deslizandose)
    float fase;               // "reloj" de las animaciones (cada +1 es una vuelta)
    float comida_x, comida_y; // donde esta la comida (o lo que trae un animal, como el queso)
} cara_t;

extern const cara_t CARA_NORMAL;
extern cara_t cara;                       // la cara de este momento
extern color_t color_fondo;
extern const char *globo;                 // lo que dice en el globito (o NULL)
extern bool arcoiris;                     // true = el LED hace arco iris
extern eyes_animal_t animal_actual;       // que animal esta haciendo (o ninguno)

// Colores del LED: LED(rojo, verde, azul), de 0 a 255
typedef struct { uint8_t r, g, b; } color_led_t;
#define LED(r, g, b) ((color_led_t){ (r), (g), (b) })

// ─── Herramientas de eyes.c ──────────────────────────────────────────────────
int  azar(int min, int max);                  // numero al azar entre min y max
void esperar(int ms);                         // espera (se corta si aprietan el boton)
void animar(cara_t destino, int ms);          // pasa suave a la cara "destino"
void mover_a(float x, float y, int ms);       // como animar, pero solo la posicion
void cerrar_y_abrir(bool izq, bool der, int cerrado_ms);
void poner_led(color_led_t c);
void poner_led_tranquila(void);
void empezar_accion(void);                    // las funciones publicas la llaman al empezar
void arco(float cx, float cy, float r, float desde, float hasta, int grosor, color_t c);
void dibujar_corazon(float cx, float cy, float r, color_t c);
// Los rulos, pintados con estos colores (de izquierda a derecha, corridos "corrimiento")
void dibujar_rulos_de_colores(const color_t *colores, int cantidad, int corrimiento, color_t vueltita);

// ─── Herramientas de animales.c ──────────────────────────────────────────────
bool animal_lleva_rulos(void);                    // ¿se ven los rulos de Flufi?
void dibujar_animal_atras(float mx, float cy);    // lo que va detras de los ojos (orejas, alas...)
void dibujar_animal_adelante(float mx, float cy); // lo que va delante (nariz, bigotes...)

#endif // EYES_INTERNO_H
