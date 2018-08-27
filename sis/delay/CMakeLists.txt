set(CMAKE_C_FLAGS -DSIS)
add_library(delay STATIC
        com_delay.c
        delay.c
        mapdelay.c
        tdc_delay.c
        )