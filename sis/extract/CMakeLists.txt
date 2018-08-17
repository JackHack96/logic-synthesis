# Generating the library libextract.a
set(project "extract")

set(${project}, "")
list(APPEND ${project}_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/best_subk.c
        ${CMAKE_CURRENT_SOURCE_DIR}/com_ex.c
        ${CMAKE_CURRENT_SOURCE_DIR}/conv.c
        ${CMAKE_CURRENT_SOURCE_DIR}/cube.c
        ${CMAKE_CURRENT_SOURCE_DIR}/cubeindex.c
        ${CMAKE_CURRENT_SOURCE_DIR}/ddivisor.c
        ${CMAKE_CURRENT_SOURCE_DIR}/extract_int.h
        ${CMAKE_CURRENT_SOURCE_DIR}/fast_extract.c
        ${CMAKE_CURRENT_SOURCE_DIR}/fast_extract_int.h
        ${CMAKE_CURRENT_SOURCE_DIR}/gdivisor.c
        ${CMAKE_CURRENT_SOURCE_DIR}/gen_2c_kernel.c
        ${CMAKE_CURRENT_SOURCE_DIR}/genkern.c
        ${CMAKE_CURRENT_SOURCE_DIR}/greedycol.c
        ${CMAKE_CURRENT_SOURCE_DIR}/greedyrow.c
        ${CMAKE_CURRENT_SOURCE_DIR}/heap.c
        ${CMAKE_CURRENT_SOURCE_DIR}/heap.h
        ${CMAKE_CURRENT_SOURCE_DIR}/kernel.c
        ${CMAKE_CURRENT_SOURCE_DIR}/misc.c
        ${CMAKE_CURRENT_SOURCE_DIR}/pingpong.c
        ${CMAKE_CURRENT_SOURCE_DIR}/qdivisor.c
        ${CMAKE_CURRENT_SOURCE_DIR}/rect.c
        ${CMAKE_CURRENT_SOURCE_DIR}/sdivisor.c
        ${CMAKE_CURRENT_SOURCE_DIR}/value.c
        )
add_library(extract STATIC ${${project}_SOURCES})