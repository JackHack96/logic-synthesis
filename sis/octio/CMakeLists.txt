cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 11)

add_library(liboctio STATIC
        farray.h
        octio.c
        octio.h
        octread.c
        octwrite.c
        )