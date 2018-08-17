# Generating the library libdelay.a
set(project "delay")

set(${project}, "")
list(APPEND ${project}_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/com_delay.c
        ${CMAKE_CURRENT_SOURCE_DIR}/delay.c
        ${CMAKE_CURRENT_SOURCE_DIR}/delay_int.h
        ${CMAKE_CURRENT_SOURCE_DIR}/mapdelay.c
        ${CMAKE_CURRENT_SOURCE_DIR}/tdc_delay.c
        ${CMAKE_CURRENT_SOURCE_DIR}/tdc_int.h
        )
add_library(delay STATIC ${${project}_SOURCES})