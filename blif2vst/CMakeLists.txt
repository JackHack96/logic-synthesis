cmake_minimum_required(VERSION 3.8)
project(blif2vst)

set(CMAKE_CXX_STANDARD 11)
set(project "blif2vst")

set(${project}, "")
list(APPEND ${project}_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/blif2vst.c
        )
add_executable(blif2vst ${${project}_SOURCES})