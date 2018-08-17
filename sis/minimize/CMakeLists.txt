cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 11)

add_library(libminimize STATIC
        dcsimp.c
        minimize.c
        ros.c

        min_int.h
        ros.h
        )