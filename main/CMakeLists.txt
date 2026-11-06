idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES display eyes board_config led wifi reloj esp_timer)
