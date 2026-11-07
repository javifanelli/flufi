#ifndef FLUFI_PRONOSTICO_H
#define FLUFI_PRONOSTICO_H

#include <stdbool.h>
#include <stdint.h>

// El tiempo de afuera, sacado de internet (Open-Meteo, gratis y sin clave).
// El lugar esta en clima_secretos.h (que NO se sube al repo). Si ese archivo no
// existe, Flufi funciona igual, sin pronostico.

typedef struct {
    int     codigo;            // codigo del tiempo (WMO): 0 = despejado, 3 = nublado, 61 = lluvia...
    int16_t temp_decimas;      // temperatura de afuera en decimas de grado (235 = 23,5 °C)
    bool    es_de_dia;         // true de dia, false de noche
} pronostico_t;

// Arranca la tarea que consulta el pronostico "de fondo" (espera a que haya WiFi
// y despues pregunta cada PRONOSTICO_CADA_MIN minutos)
void pronostico_iniciar(void);

// true si hay datos del lugar (clima_secretos.h)
bool pronostico_configurado(void);

// Copia el ultimo pronostico. Devuelve false si todavia no hay ninguno (o si el
// ultimo es muy viejo: mas de 2 horas).
bool pronostico_ultimo(pronostico_t *p);

#endif // FLUFI_PRONOSTICO_H
