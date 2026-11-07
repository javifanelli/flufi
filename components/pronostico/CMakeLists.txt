idf_component_register(SRCS "pronostico.c"
                       INCLUDE_DIRS "include"
                       REQUIRES wifi esp_http_client mbedtls esp_timer)

# clima_secretos.h no se sube al repo, asi que puede aparecer DESPUES de una
# compilacion. CONFIGURE_DEPENDS hace que en cada build se revise si el archivo
# aparecio o desaparecio, y que pronostico.c se recompile si cambia.
file(GLOB secretos CONFIGURE_DEPENDS "${COMPONENT_DIR}/clima_secretos.h")
if(secretos)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE FLUFI_HAY_SECRETOS_CLIMA=1)
    set_source_files_properties(pronostico.c PROPERTIES OBJECT_DEPENDS "${secretos}")
endif()
