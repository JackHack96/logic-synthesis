cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 11)

add_library(libsparse STATIC
        cols.c
        matrix.c
        rows.c
        sparse_int.h
        )