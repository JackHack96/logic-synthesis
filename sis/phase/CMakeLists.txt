cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 11)

add_library(libphase STATIC
        ai.c
        com_ph.c
        greedy.c
        phase.c
        phase_lib.c

        phase_int.h
        )