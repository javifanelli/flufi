idf_component_register(SRCS "bmp280.c"
                       INCLUDE_DIRS "include"
                       REQUIRES board_config esp_driver_i2c)
