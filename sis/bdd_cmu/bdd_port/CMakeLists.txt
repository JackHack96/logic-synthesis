# Generating the library libbdd_port.a
set(project "bdd_port")

set(${project}, "")
list(APPEND ${project}_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/bdd.h
        ${CMAKE_CURRENT_SOURCE_DIR}/bdditer.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bddport.c
        )
add_library(bdd_port STATIC ${${project}_SOURCES})