cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 11)

add_library(libnetwork STATIC
        acyclic.c
        append.c
        cleanup.c
        com_network.c
        dfs.c
        esp.c
        net2pla.c
        net_seq.c
        net_verify.c
        netchk.c
        netclp.c
        netmake.c
        network_util.c
        pla2net.c
        sweep.c
        )