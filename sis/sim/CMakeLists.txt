cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 11)

add_library(libsim STATIC
        codegen.c
        com_sim.c
        interpret.c
        sim_int.h
        )