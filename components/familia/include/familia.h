#ifndef FAMILIA_H
#define FAMILIA_H

#include <stdbool.h>

// ─────────────────────────────────────────────────────────────────────────────
//  La familia: los personajes de la casa (Pitri, Flufi, ...) se hablan por la
//  red WiFi. Este componente es IGUAL en todos los personajes: para sumar uno
//  nuevo, se copia la carpeta y se llama a familia_iniciar() con su nombre.
//  Como son los mensajes: ver docs/FAMILIA.md
// ─────────────────────────────────────────────────────────────────────────────
//
// La palabra secreta de la familia esta en familia_secretos.h (que NO se sube al
// repo, y tiene que ser la misma en todos). Si ese archivo no existe, el
// personaje funciona igual, pero solito.

#define FAMILIA_MAX_NOMBRE   16     // letras de un nombre (sin espacios)
#define FAMILIA_MAX_DATO     48     // letras del dato que puede llevar un mensaje
#define FAMILIA_MAX_MIEMBROS 8      // cuantos personajes puede conocer

// Lo que se pueden decir. Para agregar uno: sumarlo aca (al final), en la lista
// "palabras" de familia.c y en la tabla de docs/FAMILIA.md.
typedef enum {
    FAMILIA_HOLA,        // "¡me acabo de conectar!" (o lo escuchamos por primera vez)
    FAMILIA_SALUDO,      // "¡hola!" (respuesta a HOLA)
    FAMILIA_BESO,        // un beso
    FAMILIA_CHARLA,      // un turno de una charla. dato: "<charla> <renglon>" (te toca decir ese renglon)
    FAMILIA_VISITA,      // "¡llego alguien!" (lo vio el sensor de movimiento)
    FAMILIA_COMIDA,      // te convido algo de comer
    FAMILIA_COSQUILLAS,  // cosquillas
    FAMILIA_JUGAR,       // "¡juguemos a esto!" dato: el nombre del juego (los dos lo hacen a la vez)
    FAMILIA_TRISTE,      // "estoy llorando" (el otro consuela)
    FAMILIA_CLIMA,       // el tiempo de afuera. dato: "<temperatura en decimas> <como esta el cielo>"
    FAMILIA_SENSOR,      // lo que mide mi sensor. dato: "<temperatura en decimas> <humedad en %>"
    FAMILIA_MENSAJE,     // "¡me llego un mensaje!" (de Telegram)
    FAMILIA_DORMIR,      // "me voy a dormir"
    FAMILIA_CANT_TIPOS
} familia_tipo_t;

// Algo que le dijeron a este personaje
typedef struct {
    familia_tipo_t tipo;
    char de[FAMILIA_MAX_NOMBRE];        // quien lo mando
    char dato[FAMILIA_MAX_DATO];        // dato extra ("" si no hay)
} familia_evento_t;

// Arranca: espera a que haya WiFi, avisa "HOLA" a todos y se queda escuchando
void familia_iniciar(const char *mi_nombre);

// true si hay palabra secreta (familia_secretos.h)
bool familia_configurada(void);

// true si hay algo esperando, sin sacarlo
bool familia_hay_evento(void);

// Saca lo proximo que le dijeron. false si no hay nada.
bool familia_proximo_evento(familia_evento_t *evento);

// Le manda algo a otro personaje ("para" = su nombre, o "*" para todos)
void familia_mandar(const char *para, familia_tipo_t tipo, const char *dato);

// Copia los nombres de los personajes conectados ahora (los que se escucharon
// hace poco). Devuelve cuantos hay.
int familia_conectados(char nombres[][FAMILIA_MAX_NOMBRE], int max);

// Elige al azar a alguien conectado. false si no hay nadie.
bool familia_alguien_al_azar(char nombre[FAMILIA_MAX_NOMBRE]);

// ─── Charlas ─────────────────────────────────────────────────────────────────
// Las charlas son iguales para todos (estan en charlas.c). El que empieza dice
// el renglon 0 y le manda CHARLA "<n> 1" al otro, que dice el 1 y le manda
// "<n> 2", y asi. Cada uno dice sus renglones en su pantalla.

// Como se siente el que dice un renglon (para acompañarlo con la cara)
typedef enum {
    FAMILIA_EMOCION_NADA,
    FAMILIA_EMOCION_CONTENTA,
    FAMILIA_EMOCION_AMOR,
    FAMILIA_EMOCION_RISA,
} familia_emocion_t;

int familia_cant_charlas(void);

// El renglon "renglon" de la charla "charla" (NULL si ya no hay mas)
const char *familia_charla(int charla, int renglon, familia_emocion_t *emocion);

// Los juegos que se pueden hacer juntos (todos los personajes los tienen)
int familia_cant_juegos_juntos(void);
const char *familia_juego_junto(int i);

#endif // FAMILIA_H
