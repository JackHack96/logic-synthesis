cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 11)

add_library(libntbdd STATIC
        bdd_at_node.c
        bdd_to_ntwk.c
        com_ntbdd.c
        manager.c

        node_to_bdd.c
        verify_ntwk.c
        ntbdd_int.h
        )