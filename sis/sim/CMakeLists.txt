

add_library(sim STATIC
        codegen.c
        com_sim.c
        interpret.c
        ../include/sim_int.h
        )