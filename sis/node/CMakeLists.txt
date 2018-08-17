cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 11)

add_library(libnode STATIC
        cofct.c
        collapse.c
        com_node.c
        cubehack.c
        divide.c
        fan.c
        invert.c
        iphase.c
        libhack.c
        names.c
        node.c
        nodecheck.c
        nodeindex.c
        nodemisc.c
        nodeutil.c
        print.c
        sethack.c
        substitute.c
        node_int.h
        )