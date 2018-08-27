set(CMAKE_C_FLAGS -DSIS)

add_library(seqbdd STATIC
        bull.c
        bull_util.c
        com_verify.c
        consistency.c

        manual_order.c
        network_info.c
        ordering.c
        prioqueue.c


        prl_dep.c
        prl_equiv.c
        prl_extract.c
        prl_ordering.c
        prl_product.c

        prl_remlatch.c

        prl_seqinfo.c
        prl_util.c


        product.c
        seqbdd_cycle.c
        verif_util.c

        )