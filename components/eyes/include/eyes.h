#ifndef EYES_H
#define EYES_H

#include <stdbool.h>
#include "display.h"
#include "comida.h"

// Los ojos de Flufi: dos ojos redondeados con pestañas, rulos rubios y labios
// rojos, sus emociones y juegos. Cada emocion prende el LED de la placa de un color.

// Dibuja la cara en el centro, con estos colores
void eyes_init(color_t ojos, color_t pestanas, color_t boca, color_t texto, color_t fondo);

// Le da a los ojos una funcion que diga "¡para!" (por ejemplo, si se apreto el
// boton del menu). Mientras devuelva true, lo que esten haciendo termina enseguida.
void eyes_set_interrupcion(bool (*funcion)(void));

// Vuelve a dibujar la cara tranquila en el centro (por ejemplo, al salir del menu)
void eyes_reiniciar(void);

// Un parpadeo (cierra y abre los ojos, ~200 ms)
void eyes_parpadear(void);

// Elige un juego (emocion) al azar y lo hace (la funcion vuelve cuando termina)
void eyes_jugar(void);

// Un "paso" (~1,2 s) de la cara alejada mirando las ondas de WiFi, que se van
// encendiendo de a una. Llamarla una y otra vez mientras se conecta.
void eyes_onditas(void);

// La cara se aleja (se achica y se corre a la izquierda) y "texto" aparece a la
// derecha durante "ms" milisegundos (acepta '\n'). "escala" es el tamaño de
// letra mas grande que se quiere: si el texto no entra, se usa uno mas chico.
// Con escala 2 entran 5 letras por renglon; con escala 1, 9.
void eyes_decir(const char *texto, int escala, int ms);

// ─── Reacciones (con el LED en arco iris) ────────────────────────────────────
void eyes_saludar(void);               // se sorprende y se pone contenta
void eyes_decir_que_no(void);          // esta llena: mueve la cabeza diciendo que no
void eyes_comer(comida_t comida);      // 3 mordiscos y "¡Rico!"
void eyes_tomar(comida_t bebida);      // 3 sorbos y "Ahh!"

// ─── Animales (modo Animales) ────────────────────────────────────────────────
// Flufi hace de animal, pero siempre con sus ojos, sus pestañas y sus labios.
typedef enum {
    ANIMAL_NINGUNO,
    ANIMAL_CONEJA,
    ANIMAL_UNICORNIO,
    ANIMAL_RATONCITA,
    ANIMAL_MARIPOSA,
    ANIMAL_ZORRITA,
    CANT_ANIMALES
} eyes_animal_t;

// Se disfraza de ese animal (si no lo estaba) y hace lo suyo (~5 segundos)
void eyes_actuar(eyes_animal_t animal);

// Se saca el disfraz y vuelve a ser Flufi (con sus rulos)
void eyes_sacar_disfraz(void);

#endif // EYES_H
