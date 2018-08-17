# Generating the library libio.a
set(project "io")

set(${project}, "")
list(APPEND ${project}_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/com_io.c
        ${CMAKE_CURRENT_SOURCE_DIR}/eqnlex.l
        ${CMAKE_CURRENT_SOURCE_DIR}/io_int.h
        ${CMAKE_CURRENT_SOURCE_DIR}/plot_blif.c
        ${CMAKE_CURRENT_SOURCE_DIR}/read_blif.c
        ${CMAKE_CURRENT_SOURCE_DIR}/read_eqn.y
        ${CMAKE_CURRENT_SOURCE_DIR}/read_kiss.c
        ${CMAKE_CURRENT_SOURCE_DIR}/read_pla.c
        ${CMAKE_CURRENT_SOURCE_DIR}/read_slif.c
        ${CMAKE_CURRENT_SOURCE_DIR}/read_util.c
        ${CMAKE_CURRENT_SOURCE_DIR}/write_bdnet.c
        ${CMAKE_CURRENT_SOURCE_DIR}/write_blif.c
        ${CMAKE_CURRENT_SOURCE_DIR}/write_eqn.c
        ${CMAKE_CURRENT_SOURCE_DIR}/write_kiss.c
        ${CMAKE_CURRENT_SOURCE_DIR}/write_pla.c
        ${CMAKE_CURRENT_SOURCE_DIR}/write_slif.c
        ${CMAKE_CURRENT_SOURCE_DIR}/write_util.c
        )
add_library ( io STATIC ${${project}_SOURCES} )