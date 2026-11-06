#ifndef EYES_H
#define EYES_H

#include "display.h"

// Los ojos de Flufi: dos ojos redondeados con pestañas.

// Dibuja los ojos abiertos, en el centro, con estos colores
void eyes_init(color_t ojos, color_t pestanas, color_t texto, color_t fondo);

// Un parpadeo (cierra y abre los ojos, ~200 ms)
void eyes_parpadear(void);

// Un "paso" (~1,2 s) de las ondas de WiFi abajo de los ojos, que se van
// encendiendo de a una. Llamarla una y otra vez mientras se conecta.
void eyes_onditas(void);

// Muestra "texto" abajo de los ojos durante "ms" milisegundos (acepta '\n').
// Con escala 2 entran 2 renglones de 10 letras.
void eyes_decir(const char *texto, int escala, int ms);

#endif // EYES_H
