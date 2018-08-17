project(options)
set(project "options")

set(${project}, "")
list(APPEND ${project}_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/dfltoptions.c
        ${CMAKE_CURRENT_SOURCE_DIR}/dflttap.c
        ${CMAKE_CURRENT_SOURCE_DIR}/options.c
        ${CMAKE_CURRENT_SOURCE_DIR}/options.h
        )
add_library(options STATIC ${${project}_SOURCES})