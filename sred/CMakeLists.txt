project(sred)

include_directories(../espresso/inc)
include_directories(../utility)

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

set(BISON_OUTPUT ${CMAKE_SOURCE_DIR}/sred/gram.c)
if (BISON_FOUND)
    add_custom_command(
            OUTPUT ${BISON_OUTPUT}
            COMMAND ${BISON_EXECUTABLE}
            --output=${BISON_OUTPUT}
            -d
            ${CMAKE_SOURCE_DIR}/sred/gram.y
            COMMENT "Generating gram.c"
    )
endif ()

set(FLEX_OUTPUT ${CMAKE_SOURCE_DIR}/sred/lex.c)
if (FLEX_FOUND)
    add_custom_command(
            OUTPUT ${FLEX_OUTPUT}
            COMMAND ${FLEX_EXECUTABLE}
            --outfile=${FLEX_OUTPUT}
            ${CMAKE_SOURCE_DIR}/sred/lex.c
            COMMENT "Generating lex.c"
    )
endif ()

add_executable(sred
        action.c
        boolcmp.c
        chinclus.c
        chiusura.c
        choose.c
        clinchain.c
        clinclus.c
        clincop.c
        closure.c
        coloring.c
        comp.c
        connected.c
        esp_color.c
        exist.c
        existdf.c
        generate.c
        ibincompa.c
        incompat.c
        iobincomp.c
        label.c
        main.c
        makeout.c
        misc.c
        mxcomptbl.c
        newnstate.c
        newoutput.c
        newprime.c
        obincompa.c
        prime.c
        primeones.c
        procargs.c
        qsort.c
        qspart.c
        qspivot.c
        reduced.c
        solution.c
        strbar.c
        strcom.c
        ${BISON_OUTPUT}
        ${FLEX_OUTPUT}
        )
add_dependencies(sred sis)
target_link_libraries(sred utility m)