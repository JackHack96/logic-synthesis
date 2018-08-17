cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 11)

add_library(libseqbdd STATIC
        bull.c
        bull_util.c
        com_verify.c
        consistency.c

        manual_order.c
        network_info.c
        ordering.c
        prioqueue.c
        prioqueue.h

        prl_dep.c
        prl_equiv.c
        prl_extract.c
        prl_ordering.c
        prl_product.c

        prl_remlatch.c
        prl_seqbdd.h
        prl_seqinfo.c
        prl_util.c
        prl_util.h

        product.c
        seqbdd_cycle.c
        verif_util.c

        )