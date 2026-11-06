#ifndef FLUFI_WIFI_H
#define FLUFI_WIFI_H

#include <stdbool.h>

// Conexion a una red WiFi.
// Los datos de la red estan en wifi_secretos.h (que NO se sube al repo).
// Si ese archivo no existe, Flufi funciona igual, sin WiFi.

// Arranca la conexion. No espera: se conecta "de fondo" y, si se corta,
// reintenta solo cada vez mas espaciado (hasta 1 minuto).
void wifi_iniciar(void);

// true mientras esta conectado y tiene direccion en la red
bool wifi_conectado(void);

// true si hay datos de una red (wifi_secretos.h), o sea, si va a intentar conectarse
bool wifi_configurado(void);

#endif // FLUFI_WIFI_H
