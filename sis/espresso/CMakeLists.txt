# Generating the library libespresso.a
set(project "espresso")

set(${project}, "")
list(APPEND ${project}_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/cofactor.c
        ${CMAKE_CURRENT_SOURCE_DIR}/compl.c
        ${CMAKE_CURRENT_SOURCE_DIR}/contain.c
        ${CMAKE_CURRENT_SOURCE_DIR}/cubestr.c
        ${CMAKE_CURRENT_SOURCE_DIR}/cvrin.c
        ${CMAKE_CURRENT_SOURCE_DIR}/cvrm.c
        ${CMAKE_CURRENT_SOURCE_DIR}/cvrmisc.c
        ${CMAKE_CURRENT_SOURCE_DIR}/cvrout.c
        ${CMAKE_CURRENT_SOURCE_DIR}/equiv.c
        ${CMAKE_CURRENT_SOURCE_DIR}/espresso.c
        ${CMAKE_CURRENT_SOURCE_DIR}/essen.c
        ${CMAKE_CURRENT_SOURCE_DIR}/exact.c
        ${CMAKE_CURRENT_SOURCE_DIR}/expand.c
        ${CMAKE_CURRENT_SOURCE_DIR}/gasp.c
        ${CMAKE_CURRENT_SOURCE_DIR}/globals.c
        ${CMAKE_CURRENT_SOURCE_DIR}/hack.c
        ${CMAKE_CURRENT_SOURCE_DIR}/irred.c
        ${CMAKE_CURRENT_SOURCE_DIR}/map.c
        ${CMAKE_CURRENT_SOURCE_DIR}/opo.c
        ${CMAKE_CURRENT_SOURCE_DIR}/pair.c
        ${CMAKE_CURRENT_SOURCE_DIR}/primes.c
        ${CMAKE_CURRENT_SOURCE_DIR}/reduce.c
        ${CMAKE_CURRENT_SOURCE_DIR}/set.c
        ${CMAKE_CURRENT_SOURCE_DIR}/setc.c
        ${CMAKE_CURRENT_SOURCE_DIR}/sharp.c
        ${CMAKE_CURRENT_SOURCE_DIR}/sminterf.c
        ${CMAKE_CURRENT_SOURCE_DIR}/sparse.c
        ${CMAKE_CURRENT_SOURCE_DIR}/unate.c
        ${CMAKE_CURRENT_SOURCE_DIR}/verify.c
        )
add_library(espresso STATIC ${${project}_SOURCES})