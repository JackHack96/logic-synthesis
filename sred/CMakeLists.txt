project(sred)
set(project "sred")

set(${project}, "")
list(APPEND ${project}_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/action.c
        ${CMAKE_CURRENT_SOURCE_DIR}/boolcmp.c
        ${CMAKE_CURRENT_SOURCE_DIR}/chinclus.c
        ${CMAKE_CURRENT_SOURCE_DIR}/chiusura.c
        ${CMAKE_CURRENT_SOURCE_DIR}/choose.c
        ${CMAKE_CURRENT_SOURCE_DIR}/clinchain.c
        ${CMAKE_CURRENT_SOURCE_DIR}/clinclus.c
        ${CMAKE_CURRENT_SOURCE_DIR}/clincop.c
        ${CMAKE_CURRENT_SOURCE_DIR}/closure.c
        ${CMAKE_CURRENT_SOURCE_DIR}/coloring.c
        ${CMAKE_CURRENT_SOURCE_DIR}/comp.c
        ${CMAKE_CURRENT_SOURCE_DIR}/connected.c
        ${CMAKE_CURRENT_SOURCE_DIR}/esp_color.c
        ${CMAKE_CURRENT_SOURCE_DIR}/exist.c
        ${CMAKE_CURRENT_SOURCE_DIR}/existdf.c
        ${CMAKE_CURRENT_SOURCE_DIR}/generate.c
        ${CMAKE_CURRENT_SOURCE_DIR}/gram.y
        ${CMAKE_CURRENT_SOURCE_DIR}/ibincompa.c
        ${CMAKE_CURRENT_SOURCE_DIR}/incompat.c
        ${CMAKE_CURRENT_SOURCE_DIR}/iobincomp.c
        ${CMAKE_CURRENT_SOURCE_DIR}/label.c
        ${CMAKE_CURRENT_SOURCE_DIR}/lex.l
        ${CMAKE_CURRENT_SOURCE_DIR}/main.c
        ${CMAKE_CURRENT_SOURCE_DIR}/makeout.c
        ${CMAKE_CURRENT_SOURCE_DIR}/misc.c
        ${CMAKE_CURRENT_SOURCE_DIR}/mxcomptbl.c
        ${CMAKE_CURRENT_SOURCE_DIR}/newnstate.c
        ${CMAKE_CURRENT_SOURCE_DIR}/newoutput.c
        ${CMAKE_CURRENT_SOURCE_DIR}/newprime.c
        ${CMAKE_CURRENT_SOURCE_DIR}/obincompa.c
        ${CMAKE_CURRENT_SOURCE_DIR}/prime.c
        ${CMAKE_CURRENT_SOURCE_DIR}/primeones.c
        ${CMAKE_CURRENT_SOURCE_DIR}/procargs.c
        ${CMAKE_CURRENT_SOURCE_DIR}/qsort.c
        ${CMAKE_CURRENT_SOURCE_DIR}/qspart.c
        ${CMAKE_CURRENT_SOURCE_DIR}/qspivot.c
        ${CMAKE_CURRENT_SOURCE_DIR}/reduced.c
        ${CMAKE_CURRENT_SOURCE_DIR}/reductio.h
        ${CMAKE_CURRENT_SOURCE_DIR}/solution.c
        ${CMAKE_CURRENT_SOURCE_DIR}/strbar.c
        ${CMAKE_CURRENT_SOURCE_DIR}/strcom.c
        )
add_executable(sred ${${project}_SOURCES})

target_link_libraries(sred ${CMAKE_SOURCE_DIR}/bin/utility/libutility.a)