# Generating the library libatpg.a
set(project "atpg")

set(${project}, "")
list(APPEND ${project}_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/atpg_clauses.c
        ${CMAKE_CURRENT_SOURCE_DIR}/atpg_comb.c
        ${CMAKE_CURRENT_SOURCE_DIR}/atpg_faults.c
        ${CMAKE_CURRENT_SOURCE_DIR}/atpg_faultsim.c
        ${CMAKE_CURRENT_SOURCE_DIR}/atpg_gen_test.c
        ${CMAKE_CURRENT_SOURCE_DIR}/atpg_init.c
        ${CMAKE_CURRENT_SOURCE_DIR}/atpg_int.h
        ${CMAKE_CURRENT_SOURCE_DIR}/atpg_seq.c
        ${CMAKE_CURRENT_SOURCE_DIR}/atpg_seq_util.c
        ${CMAKE_CURRENT_SOURCE_DIR}/atpg_util.c
        ${CMAKE_CURRENT_SOURCE_DIR}/com_atpg.c
        ${CMAKE_CURRENT_SOURCE_DIR}/com_redund.c
        ${CMAKE_CURRENT_SOURCE_DIR}/com_short_t.c
        ${CMAKE_CURRENT_SOURCE_DIR}/fast_avl.c
        ${CMAKE_CURRENT_SOURCE_DIR}/fast_avl.h
        ${CMAKE_CURRENT_SOURCE_DIR}/sat.c
        ${CMAKE_CURRENT_SOURCE_DIR}/sat_int.h
        )
add_library ( atpg STATIC ${${project}_SOURCES} )