#ifndef FLUFI_CONFIG_H
#define FLUFI_CONFIG_H

#include "display.h"

// ─── Como es Flufi (¡jueguen con estos numeros!) ─────────────────────────────
// Los pines del hardware estan en board_config.h

// Colores: COLOR(rojo, verde, azul), cada uno de 0 a 255
#define COLOR_FONDO     NEGRO
#define COLOR_OJOS      COLOR(120, 210, 255)   // celeste
#define COLOR_PESTANAS  COLOR(255, 120, 190)   // rosa
#define COLOR_TEXTO     COLOR(255, 200, 230)   // rosa clarito
#define COLOR_BOCA      COLOR(225, 20, 50)     // rojo lapiz labial, ¡de diva!
// El color de los rulos esta arriba de todo en components/eyes/eyes.c

// Brillo del LED de la placa, de 0 a 100 %. Los colores de cada emocion estan
// arriba de todo en components/eyes/eyes.c
#define LED_BRILLO      25

// ─── Saludo al prenderse ─────────────────────────────────────────────────────
// '\n' = cambiar de renglon. Con tamaño 4 entran 5 letras por renglon.
#define MENSAJE_ARRANQUE        "¡Hola\nSofi!"
#define MENSAJE_ARRANQUE_2      "Soy\nFlufi"     // segundo mensaje ("" = no mostrarlo)
#define MENSAJE_ARRANQUE_TAMANO 4                // 1 = chiquito, 4 = grandote
#define MENSAJE_ARRANQUE_MS     3000             // cuanto tiempo se ve cada uno

// ─── WiFi y hora ─────────────────────────────────────────────────────────────
// Los datos de la red van en components/wifi/wifi_secretos.h (no se sube al repo)
#define WIFI_ESPERA_ARRANQUE_MS (20 * 1000)  // si no conecta en este tiempo, sigue igual
#define MENSAJE_WIFI            "Tengo\nwifi!"   // abajo de los ojos ("" = no mostrarlo)
#define MOSTRAR_WIFI_MS         3000

// Diferencia con la hora mundial (UTC), en formato TZ: "ART3" = 3 horas menos
#define ZONA_HORARIA            "ART3"
#define ESPERA_HORA_MS          (10 * 1000)  // cuanto espera la hora despues de conectarse
#define MOSTRAR_SALUDO_MS       3000         // "¡Buen dia!", "¡Buenas tardes!"...

// ─── Hora y sensor en punto (igual que Pitri) ───────────────────────────────
#define HORA_CADA_MINUTOS        5      // la hora en x:00, x:05, x:10...
#define SENSOR_CADA_MINUTOS      15     // temperatura y presion en x:00, x:15, x:30, x:45
// La cara se aleja (se achica y se corre a la izquierda) y el dato aparece al costado
#define MOSTRAR_HORA_MS          4000   // cuanto se ve la hora
#define MOSTRAR_SENSOR_MS        6000   // cuanto se ven los datos del sensor
#define TIEMPO_ENTRE_LECTURAS_MS (10 * 1000)   // cada cuanto lee el sensor

// ─── El tiempo de afuera (pronostico) ────────────────────────────────────────
// El lugar va en components/pronostico/clima_secretos.h (no se sube al repo).
// Lo que es "frio" y "calor" esta en components/eyes/clima.c
#define CLIMA_CADA_MINUTOS       30     // el tiempo en x:00 y x:30
#define MOSTRAR_CLIMA_MS         6000   // cuanto se ve

// ─── La familia (Pitri, Flufi, ...) ──────────────────────────────────────────
// La palabra secreta va en components/familia/familia_secretos.h (no se sube al repo)
#define MI_NOMBRE           "Flufi"   // como la conocen los demas (sin espacios)
#define MOSTRAR_FAMILIA_MS  3000      // cuanto se ve "¡Hola Pitri!", "¡Un beso de Pitri!"...
// Cada cuanto empieza una charla o un juego con alguien de la familia (al azar
// entre estos dos tiempos, en segundos)
#define FAMILIA_INICIATIVA_MIN_S  (4 * 60)
#define FAMILIA_INICIATIVA_MAX_S  (10 * 60)

// ─── Menu ────────────────────────────────────────────────────────────────────
// Boton A = abrir el menu y elegir, boton B = siguiente, boton BOOT = volver atras
#define MENU_ESPERA_MAX_MS  (20 * 1000)   // si nadie toca nada, se cierra solo
#define MOSTRAR_EN_MENU_MS  5000          // cuanto se ve la hora o el sensor al elegirlos

// ─── Comida y bebida ─────────────────────────────────────────────────────────
#define COMIDAS_POR_HORA  3        // cuantas veces puede comer en una hora
#define BEBIDAS_POR_HORA  3        // cuantas veces puede tomar en una hora

// ─── Juegos ───────────────────────────────────────────────────────────────────
// Descanso entre un juego y otro (se elige un valor al azar en este rango)
#define DESCANSO_MIN_MS  300
#define DESCANSO_MAX_MS  1500

#endif // FLUFI_CONFIG_H
