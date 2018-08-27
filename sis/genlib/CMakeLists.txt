set(CMAKE_C_FLAGS -DSIS)
add_library(genlib STATIC
        aoi.c
        com_genlib.c
        comb.c
        count.c
        genlib.c
        io.c
        nand.c
        permute.c
        readlib.y
        readliblex.l
        sptree.c
        )
#TODO: aggiungere regole yacc/lex