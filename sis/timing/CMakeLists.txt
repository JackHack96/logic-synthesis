cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 11)

add_library(libtiming STATIC
        com_timing.c
        timing_comp.c
        timing_graph.c

        timing_seq.c
        timing_util.c
        timing_verify.c
        timing_int.h
        )