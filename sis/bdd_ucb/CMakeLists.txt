# Generating the library libbdd_ucb.a
set(project "bdd_ucb")

set(${project}, "")
list(APPEND ${project}_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/adhoc_cache.c
        ${CMAKE_CURRENT_SOURCE_DIR}/and_smooth.c
        ${CMAKE_CURRENT_SOURCE_DIR}/assert_frame.c
        ${CMAKE_CURRENT_SOURCE_DIR}/assert_heap.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bdd_cofactor.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bdd_compose.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bdd_cproject.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bdd_end.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bdd_int.h
        ${CMAKE_CURRENT_SOURCE_DIR}/bdd_ite.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bdd_iter.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bdd_min_sibl.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bdd_min_util.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bdd_print.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bdd_quantify.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bdd_start.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bdd_substit.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bdd_support.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bdd_tovar.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bdd_util.c
        ${CMAKE_CURRENT_SOURCE_DIR}/boolean_ops.c
        ${CMAKE_CURRENT_SOURCE_DIR}/config.c
        ${CMAKE_CURRENT_SOURCE_DIR}/const_cache.c
        ${CMAKE_CURRENT_SOURCE_DIR}/dmp_ext_ptrs.c
        ${CMAKE_CURRENT_SOURCE_DIR}/dmp_mgr_stat.c
        ${CMAKE_CURRENT_SOURCE_DIR}/dmp_node_age.c
        ${CMAKE_CURRENT_SOURCE_DIR}/external_ptr.c
        ${CMAKE_CURRENT_SOURCE_DIR}/find_or_add.c
        ${CMAKE_CURRENT_SOURCE_DIR}/garb_collect.c
        ${CMAKE_CURRENT_SOURCE_DIR}/hash_cache.c
        ${CMAKE_CURRENT_SOURCE_DIR}/ite.c
        ${CMAKE_CURRENT_SOURCE_DIR}/ite_common.c
        ${CMAKE_CURRENT_SOURCE_DIR}/ite_constant.c
        ${CMAKE_CURRENT_SOURCE_DIR}/new_node.c
        ${CMAKE_CURRENT_SOURCE_DIR}/print_stats.c
        ${CMAKE_CURRENT_SOURCE_DIR}/resize_table.c
        )
add_library(bdd_ucb STATIC ${${project}_SOURCES})