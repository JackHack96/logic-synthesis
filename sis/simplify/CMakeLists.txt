cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 11)

add_library(libsimplify STATIC
        com_simp.c
        compute_dc.c
        dc_filter.c
        filter_util.c

        simp.c
        simp_daemon.c
        simp_dc.c
        simp_image.c
        simp_sm.c
        simp_util.c

        simp_int.h
        )