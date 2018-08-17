# Generating the library libcommand.a
set(project "command")

set(${project}, "")
list(APPEND ${project}_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/addcom.c
        ${CMAKE_CURRENT_SOURCE_DIR}/alias.c
        ${CMAKE_CURRENT_SOURCE_DIR}/cmd_set.c
        ${CMAKE_CURRENT_SOURCE_DIR}/com_int.h
        ${CMAKE_CURRENT_SOURCE_DIR}/command.c
        ${CMAKE_CURRENT_SOURCE_DIR}/commisc.c
        ${CMAKE_CURRENT_SOURCE_DIR}/filec.c
        ${CMAKE_CURRENT_SOURCE_DIR}/get_nodes.c
        ${CMAKE_CURRENT_SOURCE_DIR}/get_true_po.c
        ${CMAKE_CURRENT_SOURCE_DIR}/help.c
        ${CMAKE_CURRENT_SOURCE_DIR}/open_file.c
        ${CMAKE_CURRENT_SOURCE_DIR}/source.c
        ${CMAKE_CURRENT_SOURCE_DIR}/undo.c
        )
add_library(command STATIC ${${project}_SOURCES})