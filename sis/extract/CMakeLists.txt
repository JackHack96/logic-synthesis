set(CMAKE_C_FLAGS -DSIS)
add_library(extract STATIC
        best_subk.c
        com_ex.c
        conv.c
        cube.c
        cubeindex.c
        ddivisor.c
        fast_extract.c
        gdivisor.c
        gen_2c_kernel.c
        genkern.c
        greedycol.c
        greedyrow.c
        heap.c
        kernel.c
        misc.c
        pingpong.c
        qdivisor.c
        rect.c
        sdivisor.c
        value.c
        )