
add_library(io STATIC
        com_io.c
        eqnlex.l
        ../include/io_int.h
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