project(hash)

add_library(hash STATIC
        hash.c
        hash.h
        hash_drive.c
        hash_dump.c
        hash_initial.c
        install.c
        lookup.c
        )