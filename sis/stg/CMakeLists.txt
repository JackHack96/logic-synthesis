cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 11)

add_library(libstg STATIC
        com_stg.c
        enumerate.c
        level_c.c
        senum_main.c
        stg.c
        stg_sc_sim.c
        stg_util.c
        stg_int.h
        )