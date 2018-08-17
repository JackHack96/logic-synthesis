

add_library(simplify STATIC
        com_simp.c
        compute_dc.c
        dc_filter.c
        filter_util.c

        simp.c
        simp_daemon.c
        simp_dc.c
        simp_image.c
        simp_sm.c
        simp_util.c

        ../include/simp_int.h
        )