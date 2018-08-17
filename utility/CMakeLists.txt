set(project "utility")

set(${project}, "")
list(APPEND ${project}_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/cpu_time.c
        ${CMAKE_CURRENT_SOURCE_DIR}/pathsearch.c
        ${CMAKE_CURRENT_SOURCE_DIR}/pipefork.c
        ${CMAKE_CURRENT_SOURCE_DIR}/prtime.c
        ${CMAKE_CURRENT_SOURCE_DIR}/strsav.c
        ${CMAKE_CURRENT_SOURCE_DIR}/texpand.c
        ${CMAKE_CURRENT_SOURCE_DIR}/tmpfile.c
        ${CMAKE_CURRENT_SOURCE_DIR}/utility.h
        )
add_library(utility STATIC ${${project}_SOURCES})