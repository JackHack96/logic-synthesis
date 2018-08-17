

add_library(retime STATIC
        com_retime.c
        re_computeWD.c
        re_delay.c
        re_export.c

        re_graph.c
        re_initial.c
        re_milp.c
        re_minreg.c
        re_nanni.c
        re_net.c

        re_simplx.c
        re_util.c
        retime_util.c
        ../include/retime_int.h
        )