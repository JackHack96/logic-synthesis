# Generating the library libgenlib.a
set(project "genlib")

set(${project}, "")
list(APPEND ${project}_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/aoi.c
        ${CMAKE_CURRENT_SOURCE_DIR}/com_genlib.c
        ${CMAKE_CURRENT_SOURCE_DIR}/comb.c
        ${CMAKE_CURRENT_SOURCE_DIR}/comb.h
        ${CMAKE_CURRENT_SOURCE_DIR}/count.c
        ${CMAKE_CURRENT_SOURCE_DIR}/genlib.c
        ${CMAKE_CURRENT_SOURCE_DIR}/genlib.h
        ${CMAKE_CURRENT_SOURCE_DIR}/genlib_int.h
        ${CMAKE_CURRENT_SOURCE_DIR}/io.c
        ${CMAKE_CURRENT_SOURCE_DIR}/nand.c
        ${CMAKE_CURRENT_SOURCE_DIR}/permute.c
        ${CMAKE_CURRENT_SOURCE_DIR}/readlib.y
        ${CMAKE_CURRENT_SOURCE_DIR}/readliblex.l
        ${CMAKE_CURRENT_SOURCE_DIR}/sptree.c
        ${CMAKE_CURRENT_SOURCE_DIR}/sptree.h
        )
add_library(genlib STATIC ${${project}_SOURCES})