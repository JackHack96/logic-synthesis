set(CMAKE_C_FLAGS -DSIS)

add_library(speed STATIC
        absorb.c
        buf_delay.c
        buf_recur.c
        buf_replace.c

        buf_trans2.c
        buf_util.c
        com_speed.c
        gbx.c
        new_speed.c
        new_wght_util.c

        nsp_util.c
        sp_buffer.c
        sp_network.c
        speed_2c.c
        speed_and.c

        speed_delay.c
        speed_loop.c
        speed_net.c
        speed_no.c
        speed_or.c

        speed_plot.c
        speed_util.c
        speedup.c
        weight.c
        weight_util.c





        )