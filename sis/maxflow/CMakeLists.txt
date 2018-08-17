
add_library(maxflow STATIC
        com_max.c
        cutset.c
        maxflow.c
        ../include/maxflow.h
        ../include/maxflow_int.h
        mf_input.c
        )
