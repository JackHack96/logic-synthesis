cmake_minimum_required(VERSION 2.8)
set (project lsort)
set(${project}_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/lsort.h
        ${CMAKE_CURRENT_SOURCE_DIR}/luniq.h
        )
add_library(${project} STATIC ${${project}_SOURCES} )