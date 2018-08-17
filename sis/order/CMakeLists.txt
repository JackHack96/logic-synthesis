cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 11)

add_library(liborder STATIC
        dfs_order.c
        random_order.c
        )