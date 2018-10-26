project(nova)
add_executable(nova
        alloc.c

        anneal_code.c
        auxil.c
        coloring.c
        comb_objects.c

        exact_backtrack.c
        exact_code.c
        exact_graph.c
        exact_lbound.c
        exact_mini.c
        exact_output.c
        get_constr.c
        ihybrid_code.c
        input_fsm.c
        iohybrid_code.c
        iovariant_code.c
        log_approx.c
        log_code.c
        log_mini.c
        log_output.c
        log_precode.c
        log_sel.c
        lower_bound.c
        nova.c

        nova_summ.c

        options.c
        out_encoder.c

        out_eval.c
        show.c
        symbolic_loop.c
        user_codes.c
        )
include_directories(${CMAKE_SOURCE_DIR}/sis/espresso)
add_dependencies(nova sis)
target_link_libraries(nova utility m)