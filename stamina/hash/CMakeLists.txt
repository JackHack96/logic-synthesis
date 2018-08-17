project(hash)
set(project "hash")

set(${project}, "")
list(APPEND ${project}_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/hash.c
        ${CMAKE_CURRENT_SOURCE_DIR}/hash.h
        ${CMAKE_CURRENT_SOURCE_DIR}/hash_drive.c
        ${CMAKE_CURRENT_SOURCE_DIR}/hash_dump.c
        ${CMAKE_CURRENT_SOURCE_DIR}/hash_initial.c
        ${CMAKE_CURRENT_SOURCE_DIR}/install.c
        ${CMAKE_CURRENT_SOURCE_DIR}/lookup.c
        )
add_library(hash STATIC ${${project}_SOURCES})