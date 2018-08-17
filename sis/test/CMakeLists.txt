cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 11)

add_library(libtest STATIC
        end_test.c
        example.c
        init_test.c
        test.h
        test_int.h
        )