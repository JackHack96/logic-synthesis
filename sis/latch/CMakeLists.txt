set(CMAKE_C_FLAGS -DSIS)
add_library(latch STATIC
        com_latch.c
        latch.c
        )
