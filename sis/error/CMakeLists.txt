# Generating the library liberror.a
set(project "error")

set(${project}, "")
list(APPEND ${project}_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/error.c
        )
add_library(error STATIC ${${project}_SOURCES})