# Generating the library libmain.a
set(project "main")

set(${project}, "")
list(APPEND ${project}_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/sis.c
        ${CMAKE_CURRENT_SOURCE_DIR}/sis_glbs.c
        ${CMAKE_CURRENT_SOURCE_DIR}/sis_init.c
        ${CMAKE_CURRENT_SOURCE_DIR}/version.c
        )
add_library(main STATIC ${${project}_SOURCES})