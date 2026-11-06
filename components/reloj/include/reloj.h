#ifndef RELOJ_H
#define RELOJ_H

#include <stdbool.h>
#include <time.h>

// La hora de Flufi, sacada de internet.
// Apenas hay WiFi, le pregunta la hora a un servidor NTP (relojes muy exactos
// que hay en internet) y pone en hora el reloj del ESP32. Despues la sigue
// corrigiendo sola cada tanto.

// Arranca la sincronizacion "de fondo" (no espera a que termine).
//   zona_horaria: cuantas horas de diferencia con la hora mundial (UTC), en el
//   formato de la variable TZ. Ejemplo: "ART3" = Argentina, 3 horas menos.
void reloj_iniciar(const char *zona_horaria);

// true si ya se puso en hora al menos una vez
bool reloj_sincronizado(void);

// Copia en "ahora" la fecha y hora local. Devuelve false si todavia no se
// sincronizo (en ese caso la hora no sirve).
bool reloj_hora(struct tm *ahora);

#endif // RELOJ_H
