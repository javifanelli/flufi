#ifndef COMIDA_H
#define COMIDA_H

#include <stdbool.h>
#include "display.h"

// Comidas y bebidas que se le pueden dar a Flufi
typedef enum {
    COMIDA_NINGUNA,
    // Para comer
    COMIDA_MEDIALUNA,
    COMIDA_MUFFIN,
    COMIDA_DONA,
    // Para tomar
    COMIDA_TE,        // taza con saquito
    COMIDA_LICUADO,   // vaso alto con frutilla y pajita
    COMIDA_GASEOSA,   // lata con burbujitas
} comida_t;

// true si es para tomar
bool comida_es_bebida(comida_t c);

// Dibuja la comida con su centro en (cx, cy). "tamano" la agranda o achica
// (1 = unos 30 pixeles de alto). "mordidas" (0 a 2) le dibuja mordiscos; en las
// bebidas son los sorbos (0 a 3) y el liquido va bajando. "fondo" es el color de
// atras (para los mordiscos y el adentro de los recipientes).
void comida_dibujar(comida_t c, float cx, float cy, float tamano, int mordidas, color_t fondo);

#endif // COMIDA_H
