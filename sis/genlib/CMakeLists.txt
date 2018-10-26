set(CMAKE_C_FLAGS -DSIS)

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

set(BISON_OUTPUT ${CMAKE_SOURCE_DIR}/sis/genlib/readlib.c)
if (BISON_FOUND)
    add_custom_command(
            OUTPUT ${BISON_OUTPUT}
            COMMAND ${BISON_EXECUTABLE}
            --output=${BISON_OUTPUT}
            -d
            ${CMAKE_SOURCE_DIR}/sis/genlib/readlib.y
            COMMENT "Generating readlib.c"
    )
endif ()

set(FLEX_OUTPUT ${CMAKE_SOURCE_DIR}/sis/genlib/readliblex.c)
if (FLEX_FOUND)
    add_custom_command(
            OUTPUT ${FLEX_OUTPUT}
            COMMAND ${FLEX_EXECUTABLE}
            --outfile=${FLEX_OUTPUT}
            ${CMAKE_SOURCE_DIR}/sis/genlib/readliblex.l
            COMMENT "Generating readliblex.c"
    )
endif ()

add_library(genlib STATIC
        aoi.c
        com_genlib.c
        comb.c
        count.c
        genlib.c
        io.c
        nand.c
        permute.c
        sptree.c
        ${BISON_OUTPUT}
        ${FLEX_OUTPUT}
        )
add_custom_command(TARGET genlib POST_BUILD
        COMMAND sed -i 's/yy/GENLIB_yy/g' ${CMAKE_SOURCE_DIR}/sis/genlib/readlib.c
        COMMAND sed -i 's/yy/GENLIB_yy/g' ${CMAKE_SOURCE_DIR}/sis/genlib/readlib.h
        COMMAND sed -i 's/yy/GENLIB_yy/g' ${CMAKE_SOURCE_DIR}/sis/genlib/readliblex.c
        )