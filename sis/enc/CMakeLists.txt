set(CMAKE_C_FLAGS -DSIS)
add_library(enc STATIC
        com_enc.c
        dic.c
        dic_to_sm.c
        gen_eqn.c
        input.c
        )
