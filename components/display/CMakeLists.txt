idf_component_register(SRCS "display.c"
                       INCLUDE_DIRS "include"
                       REQUIRES board_config esp_driver_spi esp_driver_gpio)
