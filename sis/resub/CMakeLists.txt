cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 11)

add_library(libresub STATIC
        aresub.c
        bresub.c
        com_resub.c
        resub_int.h
        )