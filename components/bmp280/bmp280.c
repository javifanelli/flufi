#include "bmp280.h"
#include "board_config.h"

#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "BMP280";

/*
 * Como se habla con el BMP280 (ver su hoja de datos):
 *   - Se le habla por I2C: dos cables (SDA = datos, SCL = reloj) y una direccion
 *     (0x76 o 0x77, segun el modulo).
 *   - Adentro tiene "registros": cajoncitos numerados con informacion.
 *   - De fabrica trae unos numeros de "calibracion" distintos en cada sensor.
 *     Con esos numeros se corrigen las mediciones crudas para que den bien.
 */
#define REG_ID          0xD0    // dice que sensor es (0x58 = BMP280, 0x60 = BME280)
#define REG_CALIBRACION 0x88    // 24 bytes con los numeros de calibracion
#define REG_CTRL_MEAS   0xF4    // como medir
#define REG_CONFIG      0xF5    // filtro y cada cuanto medir
#define REG_DATOS       0xF7    // 6 bytes: presion (3) y temperatura (3)

static i2c_master_dev_handle_t sensor = NULL;

// Calibracion de fabrica
static uint16_t dig_T1;
static int16_t  dig_T2, dig_T3;
static uint16_t dig_P1;
static int16_t  dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;

static volatile bool    hay_lectura = false;
static volatile int16_t ultima_temp = 0;
static volatile int16_t ultima_presion = 0;

// ─── Leer y escribir registros ───────────────────────────────────────────────

static esp_err_t leer(uint8_t registro, uint8_t *datos, size_t cantidad)
{
    return i2c_master_transmit_receive(sensor, &registro, 1, datos, cantidad, 100);
}

static esp_err_t escribir(uint8_t registro, uint8_t valor)
{
    uint8_t buf[2] = { registro, valor };
    return i2c_master_transmit(sensor, buf, 2, 100);
}

// ─── Las cuentas de correccion (de la hoja de datos del BMP280) ──────────────

static int32_t t_fine;    // la temperatura "fina", que tambien se usa para la presion

// Devuelve la temperatura en centesimas de grado (2345 = 23,45 °C)
static int32_t corregir_temperatura(int32_t crudo)
{
    int32_t var1 = ((((crudo >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
    int32_t var2 = (((((crudo >> 4) - ((int32_t)dig_T1)) * ((crudo >> 4) - ((int32_t)dig_T1))) >> 12)
                    * ((int32_t)dig_T3)) >> 14;
    t_fine = var1 + var2;
    return (t_fine * 5 + 128) >> 8;
}

// Devuelve la presion en pascales (hay que llamarla despues de corregir_temperatura)
static uint32_t corregir_presion(int32_t crudo)
{
    int64_t var1 = ((int64_t)t_fine) - 128000;
    int64_t var2 = var1 * var1 * (int64_t)dig_P6;
    var2 = var2 + ((var1 * (int64_t)dig_P5) << 17);
    var2 = var2 + (((int64_t)dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)dig_P3) >> 8) + ((var1 * (int64_t)dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)dig_P1) >> 33;
    if (var1 == 0) return 0;              // para no dividir por cero

    int64_t p = 1048576 - crudo;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)dig_P7) << 4);
    return (uint32_t)(p / 256);
}

// ─── Arranque y lectura ──────────────────────────────────────────────────────

static bool preparar_sensor(void)
{
    i2c_master_bus_config_t bus_cfg = {
        .clk_source        = I2C_CLK_SRC_DEFAULT,
        .i2c_port          = 0,
        .sda_io_num        = PIN_I2C_SDA,
        .scl_io_num        = PIN_I2C_SCL,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus;
    if (i2c_new_master_bus(&bus_cfg, &bus) != ESP_OK) return false;

    // El sensor puede estar en 0x76 o en 0x77: probamos las dos
    uint16_t direccion = 0;
    if (i2c_master_probe(bus, 0x76, 100) == ESP_OK)      direccion = 0x76;
    else if (i2c_master_probe(bus, 0x77, 100) == ESP_OK) direccion = 0x77;
    else {
        ESP_LOGW(TAG, "No encuentro el sensor (revisar cables SDA=GPIO %d, SCL=GPIO %d)", PIN_I2C_SDA, PIN_I2C_SCL);
        return false;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = direccion,
        .scl_speed_hz    = 100 * 1000,
    };
    if (i2c_master_bus_add_device(bus, &dev_cfg, &sensor) != ESP_OK) return false;

    uint8_t id = 0;
    leer(REG_ID, &id, 1);
    ESP_LOGI(TAG, "Sensor en la direccion 0x%02X (id 0x%02X)", direccion, id);

    // Numeros de calibracion (vienen de a pares de bytes, el chiquito primero)
    uint8_t c[24];
    if (leer(REG_CALIBRACION, c, sizeof(c)) != ESP_OK) return false;
    dig_T1 = (uint16_t)(c[1] << 8 | c[0]);
    dig_T2 = (int16_t)(c[3] << 8 | c[2]);
    dig_T3 = (int16_t)(c[5] << 8 | c[4]);
    dig_P1 = (uint16_t)(c[7] << 8 | c[6]);
    dig_P2 = (int16_t)(c[9] << 8 | c[8]);
    dig_P3 = (int16_t)(c[11] << 8 | c[10]);
    dig_P4 = (int16_t)(c[13] << 8 | c[12]);
    dig_P5 = (int16_t)(c[15] << 8 | c[14]);
    dig_P6 = (int16_t)(c[17] << 8 | c[16]);
    dig_P7 = (int16_t)(c[19] << 8 | c[18]);
    dig_P8 = (int16_t)(c[21] << 8 | c[20]);
    dig_P9 = (int16_t)(c[23] << 8 | c[22]);

    // Que mida solo, todo el tiempo: cada 1 s, con filtro para que no salte
    escribir(REG_CONFIG, 0xB0);       // espera 1000 ms entre mediciones, filtro x16
    escribir(REG_CTRL_MEAS, 0x57);    // temperatura x2, presion x16, modo "normal"
    return true;
}

static void tarea_lecturas(void *arg)
{
    uint32_t intervalo_ms = (uint32_t)(uintptr_t)arg;

    if (!preparar_sensor()) {
        vTaskDelete(NULL);            // sin sensor no hay nada que leer
        return;
    }
    vTaskDelay(pdMS_TO_TICKS(1500));  // que termine la primera medicion

    while (1) {
        uint8_t d[6];
        if (leer(REG_DATOS, d, sizeof(d)) == ESP_OK) {
            int32_t presion_cruda = (int32_t)((d[0] << 12) | (d[1] << 4) | (d[2] >> 4));
            int32_t temp_cruda    = (int32_t)((d[3] << 12) | (d[4] << 4) | (d[5] >> 4));

            int32_t centesimas = corregir_temperatura(temp_cruda);
            uint32_t pascales  = corregir_presion(presion_cruda);

            ultima_temp    = (int16_t)((centesimas + (centesimas >= 0 ? 5 : -5)) / 10);
            ultima_presion = (int16_t)((pascales + 50) / 100);
            hay_lectura = true;
        }
        vTaskDelay(pdMS_TO_TICKS(intervalo_ms));
    }
}

void bmp280_iniciar(uint32_t intervalo_ms)
{
    if (intervalo_ms < 1000) intervalo_ms = 1000;
    xTaskCreate(tarea_lecturas, "bmp280", 3072, (void *)(uintptr_t)intervalo_ms, 4, NULL);
}

bool bmp280_ultima_lectura(int16_t *temperatura_decimas, int16_t *presion_hpa)
{
    if (!hay_lectura) return false;
    if (temperatura_decimas) *temperatura_decimas = ultima_temp;
    if (presion_hpa)         *presion_hpa = ultima_presion;
    return true;
}
