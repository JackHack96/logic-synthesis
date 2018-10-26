set(CMAKE_C_FLAGS -DSIS)

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

set(BISON_OUTPUT ${CMAKE_SOURCE_DIR}/sis/io/read_eqn.c)
if (BISON_FOUND)
    add_custom_command(
            OUTPUT ${BISON_OUTPUT}
            COMMAND ${BISON_EXECUTABLE}
            --output=${BISON_OUTPUT}
            -d
            ${CMAKE_SOURCE_DIR}/sis/io/read_eqn.y
            COMMENT "Generating read_eqn.c"
    )
endif ()

set(FLEX_OUTPUT ${CMAKE_SOURCE_DIR}/sis/io/eqnlex.c)
if (FLEX_FOUND)
    add_custom_command(
            OUTPUT ${FLEX_OUTPUT}
            COMMAND ${FLEX_EXECUTABLE}
            --outfile=${FLEX_OUTPUT}
            ${CMAKE_SOURCE_DIR}/sis/io/eqnlex.l
            COMMENT "Generating eqnlex.c"
    )
endif ()

add_library(io STATIC
        com_io.c
        plot_blif.c
        read_blif.c
        read_kiss.c
        read_pla.c
        read_slif.c
        read_util.c
        write_bdnet.c
        write_blif.c
        write_eqn.c
        write_kiss.c
        write_pla.c
        write_slif.c
        write_util.c
        ${BISON_OUTPUT}
        ${FLEX_OUTPUT}
        )
add_custom_command(TARGET io POST_BUILD
        COMMAND sed -i 's/yy/EQN_yy/g' ${CMAKE_SOURCE_DIR}/sis/io/read_eqn.c
        COMMAND sed -i 's/yy/EQN_yy/g' ${CMAKE_SOURCE_DIR}/sis/io/read_eqn.h
        COMMAND sed -i 's/yy/EQN_yy/g' ${CMAKE_SOURCE_DIR}/sis/io/eqnlex.c
        )