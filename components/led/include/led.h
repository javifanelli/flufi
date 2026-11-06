#ifndef LED_H
#define LED_H

#include <stdint.h>

// El LED de colores de la placa (un WS2812 en GPIO 8).
// Este LED no se apaga poniendo el pin en 0: hay que mandarle el color "apagado".

// Prepara el LED y lo deja apagado. "brillo" va de 0 a 100 (%): este LED es muy
// fuerte, con 20-30 % alcanza.
void led_iniciar(uint8_t brillo);

// Lo apaga
void led_apagar(void);

// Lo prende de un color (rojo, verde y azul de 0 a 255)
void led_color(uint8_t rojo, uint8_t verde, uint8_t azul);

// Arco iris: cada vez que se llama, el LED pasa al color siguiente
void led_arcoiris_paso(void);

#endif // LED_H
