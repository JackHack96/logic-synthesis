# Generating the library libgcd.a
set(project "gcd")

set(${project}, "")
list(APPEND ${project}_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/com_gcd.c
        ${CMAKE_CURRENT_SOURCE_DIR}/gcd.c
        ${CMAKE_CURRENT_SOURCE_DIR}/gcd_int.h
        )
add_library(gcd STATIC ${${project}_SOURCES})