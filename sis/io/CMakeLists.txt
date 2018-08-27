set(CMAKE_C_FLAGS -DSIS)
add_library(io STATIC
        com_io.c
        plot_blif.c
        read_blif.c
        read_eqn.y
        read_kiss.c
        read_pla.c
        read_slif.c
        read_util.c
        write_bdnet.c
        write_blif.c
        write_eqn.c
        write_kiss.c
        write_pla.c
        write_slif.c
        write_util.c
        )

#TODO: aggiungere regole yacc e lex