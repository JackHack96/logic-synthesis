set(CMAKE_C_FLAGS -DSIS -DCUR_DATE=STRING(TIMESTAMP TODAY "%Y%m%d"))

add_library(main STATIC
        sis.c
        sis_glbs.c
        sis_init.c
        version.c
        )