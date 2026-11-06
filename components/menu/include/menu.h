#ifndef MENU_H
#define MENU_H

#include "display.h"

// Menu en pantalla que se maneja con los tres botones:
//   boton B    = pasar a la opcion siguiente (al llegar al final vuelve arriba)
//   boton A    = elegir la opcion marcada
//   boton BOOT = volver atras

#define MENU_SE_CERRO  (-1)   // nadie toco nada por un rato
#define MENU_ATRAS     (-2)   // apretaron BOOT

// Los colores del menu
void menu_colores(color_t fondo, color_t texto, color_t barra, color_t texto_barra);

// Muestra "titulo" y la lista de "opciones" y espera a que se elija una.
//   titulo: "" = sin titulo (asi entran 6 opciones en vez de 5)
//   elegida:     que opcion aparece marcada al abrir (0 = la primera)
//   destacada:   opcion que lleva un puntito al costado (por ejemplo, el modo
//                actual), o -1 para ninguna
//   espera_max_ms: si nadie toca nada durante este tiempo, el menu se cierra
// Devuelve el numero de la opcion elegida (0, 1, 2...), MENU_SE_CERRO o MENU_ATRAS.
int menu_elegir(const char *titulo, const char *const opciones[], int cantidad,
                int elegida, int destacada, int espera_max_ms);

// Igual, pero mostrando las opciones de a una, cada una con un dibujo.
// "dibujar_opcion" dibuja la opcion numero "opcion" en el medio de la pantalla
// (entre y = 24 e y = 118). Abajo aparecen puntitos que muestran en que opcion
// estas.
int menu_elegir_dibujos(const char *titulo, int cantidad, void (*dibujar_opcion)(int opcion),
                        int elegida, int espera_max_ms);

#endif // MENU_H
