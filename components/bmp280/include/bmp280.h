#ifndef BMP280_H
#define BMP280_H

#include <stdbool.h>
#include <stdint.h>

// Sensor de temperatura y presion del aire BMP280 (por I2C).
// La temperatura esta en DECIMAS de grado: 234 = 23,4 °C
// La presion esta en hectopascales (hPa): al nivel del mar es mas o menos 1013.

// Busca el sensor y arranca una tarea que lo lee cada "intervalo_ms"
void bmp280_iniciar(uint32_t intervalo_ms);

// Devuelve la ultima lectura buena. false si todavia no hubo ninguna.
bool bmp280_ultima_lectura(int16_t *temperatura_decimas, int16_t *presion_hpa);

#endif // BMP280_H
