# Generating the library libgraphics.a
set(project "graphics")

set(${project}, "")
list(APPEND ${project}_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/com_graphics.c
        )
add_library(graphics STATIC ${${project}_SOURCES})