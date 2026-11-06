#ifndef BOTONES_H
#define BOTONES_H

#include <stdbool.h>

// Los 3 botones de Flufi (pines en board_config.h).
// Una tarea los vigila todo el tiempo y anota cuando se aprietan.
typedef enum {
    BOTON_A = 0,      // abre el menu y elige
    BOTON_B,          // pasa a la opcion siguiente
    BOTON_BOOT,       // el de la placa: vuelve atras
    CANT_BOTONES
} boton_t;

void botones_iniciar(void);

// true si el boton se apreto desde la ultima vez que se pregunto.
// Avisa una sola vez por cada apretada.
bool botones_se_apreto(boton_t boton);

// true si hay una apretada esperando, pero SIN consumirla (la proxima llamada
// a botones_se_apreto() la va a seguir viendo)
bool botones_pendiente(boton_t boton);

// Se olvida de todas las apretadas que estaban esperando
void botones_olvidar(void);

#endif // BOTONES_H
