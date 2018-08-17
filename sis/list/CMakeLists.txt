# Generating the library liblist.a
set(project "list")

set(${project}, "")
list(APPEND ${project}_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/list.c
        )
add_library(list STATIC ${${project}_SOURCES})