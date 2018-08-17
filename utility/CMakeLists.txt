project(utility)

add_library(utility STATIC
        cpu_time.c
        pathsearch.c
        pipefork.c
        prtime.c
        strsav.c
        texpand.c
        tmpfile.c
        utility.h
        )