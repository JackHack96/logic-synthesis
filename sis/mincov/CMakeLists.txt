cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 11)

add_library(libmincov STATIC
        bin_mincov.c
        bin_sol.c
        dominate.c
        gimpel.c

        indep.c
        main.c
        mincov.c
        part.c
        solution.c
        mincov_int.h
        )