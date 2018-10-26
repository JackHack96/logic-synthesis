set(CMAKE_C_FLAGS -DSIS)
add_library(clock STATIC
        clock.c
        clock_util.c
        com_clock.c
        )