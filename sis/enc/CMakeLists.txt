# Generating the library libenc.a
set(project "enc")

set(${project}, "")
list(APPEND ${project}_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/com_enc.c
        ${CMAKE_CURRENT_SOURCE_DIR}/dic.c
        ${CMAKE_CURRENT_SOURCE_DIR}/dic_to_sm.c
        ${CMAKE_CURRENT_SOURCE_DIR}/enc_int.h
        ${CMAKE_CURRENT_SOURCE_DIR}/gen_eqn.c
        ${CMAKE_CURRENT_SOURCE_DIR}/input.c
        )
add_library(enc STATIC ${${project}_SOURCES})