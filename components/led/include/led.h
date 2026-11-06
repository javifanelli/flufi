#ifndef LED_H
#define LED_H

#include <stdint.h>

// El LED de colores de la placa (un WS2812 en GPIO 8).
// Este LED no se apaga poniendo el pin en 0: hay que mandarle el color "apagado".

// Prepara el LED y lo deja apagado
void led_iniciar(void);

// Lo apaga
void led_apagar(void);

// Lo prende de un color (rojo, verde y azul de 0 a 255)
void led_color(uint8_t rojo, uint8_t verde, uint8_t azul);

#endif // LED_H
