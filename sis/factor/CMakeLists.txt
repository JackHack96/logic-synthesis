# Generating the library libfactor.a
set(project "factor")

set(${project}, "")
list(APPEND ${project}_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/alg_ft.c
        ${CMAKE_CURRENT_SOURCE_DIR}/com_ft.c
        ${CMAKE_CURRENT_SOURCE_DIR}/elim.c
        ${CMAKE_CURRENT_SOURCE_DIR}/factor.c
        ${CMAKE_CURRENT_SOURCE_DIR}/factor_int.h
        ${CMAKE_CURRENT_SOURCE_DIR}/ft_print.c
        ${CMAKE_CURRENT_SOURCE_DIR}/ft_util.c
        ${CMAKE_CURRENT_SOURCE_DIR}/ft_value.c
        )
add_library(factor STATIC ${${project}_SOURCES})