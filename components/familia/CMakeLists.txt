idf_component_register(SRCS "familia.c" "charlas.c"
                       INCLUDE_DIRS "include"
                       REQUIRES wifi lwip esp_timer esp_wifi)

# familia_secretos.h no se sube al repo, asi que puede aparecer DESPUES de una
# compilacion. CONFIGURE_DEPENDS hace que en cada build se revise si el archivo
# aparecio o desaparecio, y que familia.c se recompile si cambia.
file(GLOB secretos CONFIGURE_DEPENDS "${COMPONENT_DIR}/familia_secretos.h")
if(secretos)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE FAMILIA_HAY_SECRETOS=1)
    set_source_files_properties(familia.c PROPERTIES OBJECT_DEPENDS "${secretos}")
endif()
