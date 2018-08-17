project(nova)
set(project "nova")

set(${project}, "")
list(APPEND ${project}_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/alloc.c
        ${CMAKE_CURRENT_SOURCE_DIR}/inc/analisi.h
        ${CMAKE_CURRENT_SOURCE_DIR}/anneal_code.c
        ${CMAKE_CURRENT_SOURCE_DIR}/auxil.c
        ${CMAKE_CURRENT_SOURCE_DIR}/coloring.c
        ${CMAKE_CURRENT_SOURCE_DIR}/comb_objects.c
        ${CMAKE_CURRENT_SOURCE_DIR}/inc/decls.h
        ${CMAKE_CURRENT_SOURCE_DIR}/exact_backtrack.c
        ${CMAKE_CURRENT_SOURCE_DIR}/exact_code.c
        ${CMAKE_CURRENT_SOURCE_DIR}/exact_graph.c
        ${CMAKE_CURRENT_SOURCE_DIR}/exact_lbound.c
        ${CMAKE_CURRENT_SOURCE_DIR}/exact_mini.c
        ${CMAKE_CURRENT_SOURCE_DIR}/exact_output.c
        ${CMAKE_CURRENT_SOURCE_DIR}/get_constr.c
        ${CMAKE_CURRENT_SOURCE_DIR}/ihybrid_code.c
        ${CMAKE_CURRENT_SOURCE_DIR}/input_fsm.c
        ${CMAKE_CURRENT_SOURCE_DIR}/iohybrid_code.c
        ${CMAKE_CURRENT_SOURCE_DIR}/iovariant_code.c
        ${CMAKE_CURRENT_SOURCE_DIR}/log_approx.c
        ${CMAKE_CURRENT_SOURCE_DIR}/log_code.c
        ${CMAKE_CURRENT_SOURCE_DIR}/log_mini.c
        ${CMAKE_CURRENT_SOURCE_DIR}/log_output.c
        ${CMAKE_CURRENT_SOURCE_DIR}/log_precode.c
        ${CMAKE_CURRENT_SOURCE_DIR}/log_sel.c
        ${CMAKE_CURRENT_SOURCE_DIR}/lower_bound.c
        ${CMAKE_CURRENT_SOURCE_DIR}/nova.c
        ${CMAKE_CURRENT_SOURCE_DIR}/inc/nova.h
        ${CMAKE_CURRENT_SOURCE_DIR}/nova_summ.c
        ${CMAKE_CURRENT_SOURCE_DIR}/inc/oldport.h
        ${CMAKE_CURRENT_SOURCE_DIR}/options.c
        ${CMAKE_CURRENT_SOURCE_DIR}/out_encoder.c
        ${CMAKE_CURRENT_SOURCE_DIR}/inc/out_encoder.h
        ${CMAKE_CURRENT_SOURCE_DIR}/out_eval.c
        ${CMAKE_CURRENT_SOURCE_DIR}/show.c
        ${CMAKE_CURRENT_SOURCE_DIR}/symbolic_loop.c
        ${CMAKE_CURRENT_SOURCE_DIR}/user_codes.c
        )
add_executable(nova ${${project}_SOURCES})
target_link_libraries(nova ${CMAKE_SOURCE_DIR}/bin/utility/libutility.a)
target_link_libraries(nova m)