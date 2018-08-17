# Generating the library libmem.a
set(project "mem")

set(${project}, "")
list(APPEND ${project}_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/memblock.c
        ${CMAKE_CURRENT_SOURCE_DIR}/memint.h
        ${CMAKE_CURRENT_SOURCE_DIR}/memrec.c
        ${CMAKE_CURRENT_SOURCE_DIR}/memuser.h
        )

add_library(mem STATIC ${${project}_SOURCES})