#ifndef EYES_H
#define EYES_H

#include "display.h"
#include "comida.h"

// Los ojos de Flufi: dos ojos redondeados con pestañas, sus emociones y juegos.
// Cada emocion prende el LED de la placa de un color.

// Dibuja los ojos abiertos, en el centro, con estos colores
void eyes_init(color_t ojos, color_t pestanas, color_t boca, color_t texto, color_t fondo);

// Un parpadeo (cierra y abre los ojos, ~200 ms)
void eyes_parpadear(void);

// Elige un juego (emocion) al azar y lo hace (la funcion vuelve cuando termina)
void eyes_jugar(void);

// Un "paso" (~1,2 s) de las ondas de WiFi abajo de los ojos, que se van
// encendiendo de a una. Llamarla una y otra vez mientras se conecta.
void eyes_onditas(void);

// Muestra "texto" abajo de los ojos durante "ms" milisegundos (acepta '\n').
// Con escala 2 entran 2 renglones de 10 letras.
void eyes_decir(const char *texto, int escala, int ms);

// ─── Reacciones (con el LED en arco iris) ────────────────────────────────────
void eyes_saludar(void);               // se sorprende y se pone contenta
void eyes_decir_que_no(void);          // esta llena: mueve la cabeza diciendo que no
void eyes_comer(comida_t comida);      // 3 mordiscos y "¡Rico!"
void eyes_tomar(comida_t bebida);      // 3 sorbos y "Ahh!"

#endif // EYES_H
