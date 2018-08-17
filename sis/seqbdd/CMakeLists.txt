

add_library(seqbdd STATIC
        bull.c
        bull_util.c
        com_verify.c
        consistency.c

        manual_order.c
        network_info.c
        ordering.c
        prioqueue.c
        ../include/prioqueue.h

        prl_dep.c
        prl_equiv.c
        prl_extract.c
        prl_ordering.c
        prl_product.c

        prl_remlatch.c
        ../include/prl_seqbdd.h
        prl_seqinfo.c
        prl_util.c
        ../include/prl_util.h

        product.c
        seqbdd_cycle.c
        verif_util.c

        )