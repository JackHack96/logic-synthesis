# Generating the library libastg.a
set(project "astg")

set(${project}, "")
list(APPEND ${project}_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/astg_blif.c
        ${CMAKE_CURRENT_SOURCE_DIR}/astg_cmds.c
        ${CMAKE_CURRENT_SOURCE_DIR}/astg_contract.c
        ${CMAKE_CURRENT_SOURCE_DIR}/astg_core.h
        ${CMAKE_CURRENT_SOURCE_DIR}/astg_core1.c
        ${CMAKE_CURRENT_SOURCE_DIR}/astg_core2.c
        ${CMAKE_CURRENT_SOURCE_DIR}/astg_flow.c
        ${CMAKE_CURRENT_SOURCE_DIR}/astg_int.h
        ${CMAKE_CURRENT_SOURCE_DIR}/astg_irred.c
        ${CMAKE_CURRENT_SOURCE_DIR}/astg_lkgraph.c
        ${CMAKE_CURRENT_SOURCE_DIR}/astg_marking.c
        ${CMAKE_CURRENT_SOURCE_DIR}/astg_persist.c
        ${CMAKE_CURRENT_SOURCE_DIR}/astg_read.c
        ${CMAKE_CURRENT_SOURCE_DIR}/astg_reduce.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bwd_code.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bwd_com.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bwd_hazard.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bwd_int.h
        ${CMAKE_CURRENT_SOURCE_DIR}/bwd_io.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bwd_lp.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bwd_min_delay.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bwd_slow.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bwd_stg_to_f.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bwd_util.c
        ${CMAKE_CURRENT_SOURCE_DIR}/com_astg.c
        ${CMAKE_CURRENT_SOURCE_DIR}/min_delay_int.h
        ${CMAKE_CURRENT_SOURCE_DIR}/si_com.c
        ${CMAKE_CURRENT_SOURCE_DIR}/si_encode.c
        ${CMAKE_CURRENT_SOURCE_DIR}/si_int.h
        ${CMAKE_CURRENT_SOURCE_DIR}/si_min.c
        )
add_library ( astg STATIC ${${project}_SOURCES} )