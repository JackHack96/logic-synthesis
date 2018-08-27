set(CMAKE_C_FLAGS -DSIS)
add_library(decomp STATIC
        com_dec.c
        dec_alg.c
        dec_sm.c
        dec_tech.c
        dec_util.c
        decomp.c
        disj.c
        partition.c
        )