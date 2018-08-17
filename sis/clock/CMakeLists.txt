# Generating the library libclock.a
set(project "clock")

set(${project}, "")
list(APPEND ${project}_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/clock.c
        ${CMAKE_CURRENT_SOURCE_DIR}/clock_util.c
        ${CMAKE_CURRENT_SOURCE_DIR}/com_clock.c
        )
add_library(clock STATIC ${${project}_SOURCES})