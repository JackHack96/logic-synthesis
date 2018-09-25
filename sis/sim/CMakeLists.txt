set(CMAKE_C_FLAGS -DSIS)
add_library(sim STATIC
        codegen.c
        com_sim.c
        interpret.c

        )