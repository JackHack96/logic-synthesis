cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 11)

add_library(libutil STATIC
        cpu_stats.c
        cpu_time.c
        getopt.c
        pathsearch.c

        pipefork.c
        prtime.c
        restart.c
        safe_mem.c
        saveimage.c
        state.c

        strsav.c
        texpand.c
        tmpfile.c
        )