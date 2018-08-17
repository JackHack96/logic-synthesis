# Generating the library libdecomp.a
set(project "decomp")

set(${project}, "")
list(APPEND ${project}_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/com_dec.c
        ${CMAKE_CURRENT_SOURCE_DIR}/dec_alg.c
        ${CMAKE_CURRENT_SOURCE_DIR}/dec_sm.c
        ${CMAKE_CURRENT_SOURCE_DIR}/dec_tech.c
        ${CMAKE_CURRENT_SOURCE_DIR}/dec_util.c
        ${CMAKE_CURRENT_SOURCE_DIR}/decomp.c
        ${CMAKE_CURRENT_SOURCE_DIR}/decomp_int.h
        ${CMAKE_CURRENT_SOURCE_DIR}/disj.c
        ${CMAKE_CURRENT_SOURCE_DIR}/partition.c
        )
add_library(decomp STATIC ${${project}_SOURCES})