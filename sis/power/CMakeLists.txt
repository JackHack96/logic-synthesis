cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 11)

add_library(libpower STATIC
        com_power.c
        power_comb.c
        power_comp.c

        power_dynamic.c
        power_main.c
        power_pipe.c
        power_psAppr.c

        power_psExact.c
        power_sample.c
        power_seq.c
        power_sim.c
        power_util.c

        power_int.h
        )