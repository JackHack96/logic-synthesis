# Generating the library libmap.a
set(project "map")

set(${project}, "")
list(APPEND ${project}_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/addinv.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bin_delay.c
        ${CMAKE_CURRENT_SOURCE_DIR}/bin_delay_static.h
        ${CMAKE_CURRENT_SOURCE_DIR}/bin_int.h
        ${CMAKE_CURRENT_SOURCE_DIR}/bottom_up.c
        ${CMAKE_CURRENT_SOURCE_DIR}/chkform.c
        ${CMAKE_CURRENT_SOURCE_DIR}/cluster.c
        ${CMAKE_CURRENT_SOURCE_DIR}/cluster.h
        ${CMAKE_CURRENT_SOURCE_DIR}/cluster_static.h
        ${CMAKE_CURRENT_SOURCE_DIR}/com_map.c
        ${CMAKE_CURRENT_SOURCE_DIR}/fanout_alg.h
        ${CMAKE_CURRENT_SOURCE_DIR}/fanout_alg_macro.h
        ${CMAKE_CURRENT_SOURCE_DIR}/fanout_delay.c
        ${CMAKE_CURRENT_SOURCE_DIR}/fanout_delay.h
        ${CMAKE_CURRENT_SOURCE_DIR}/fanout_delay_static.h
        ${CMAKE_CURRENT_SOURCE_DIR}/fanout_dump.c
        ${CMAKE_CURRENT_SOURCE_DIR}/fanout_est.c
        ${CMAKE_CURRENT_SOURCE_DIR}/fanout_est_static.h
        ${CMAKE_CURRENT_SOURCE_DIR}/fanout_int.h
        ${CMAKE_CURRENT_SOURCE_DIR}/fanout_log.c
        ${CMAKE_CURRENT_SOURCE_DIR}/fanout_opt.c
        ${CMAKE_CURRENT_SOURCE_DIR}/fanout_opt_static.h
        ${CMAKE_CURRENT_SOURCE_DIR}/fanout_tree.c
        ${CMAKE_CURRENT_SOURCE_DIR}/fanout_tree_static.h
        ${CMAKE_CURRENT_SOURCE_DIR}/fanout_util.c
        ${CMAKE_CURRENT_SOURCE_DIR}/gate_link.c
        ${CMAKE_CURRENT_SOURCE_DIR}/gate_link.h
        ${CMAKE_CURRENT_SOURCE_DIR}/hackit.c
        ${CMAKE_CURRENT_SOURCE_DIR}/lib_int.h
        ${CMAKE_CURRENT_SOURCE_DIR}/library.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libutil.c
        ${CMAKE_CURRENT_SOURCE_DIR}/lt_trees.c
        ${CMAKE_CURRENT_SOURCE_DIR}/map_defs.h
        ${CMAKE_CURRENT_SOURCE_DIR}/map_delay.c
        ${CMAKE_CURRENT_SOURCE_DIR}/map_delay.h
        ${CMAKE_CURRENT_SOURCE_DIR}/map_int.h
        ${CMAKE_CURRENT_SOURCE_DIR}/map_interface.c
        ${CMAKE_CURRENT_SOURCE_DIR}/map_interface_static.h
        ${CMAKE_CURRENT_SOURCE_DIR}/map_macros.h
        ${CMAKE_CURRENT_SOURCE_DIR}/maputil.c
        ${CMAKE_CURRENT_SOURCE_DIR}/match.c
        ${CMAKE_CURRENT_SOURCE_DIR}/mix_lt_trees.c
        ${CMAKE_CURRENT_SOURCE_DIR}/multi_array.c
        ${CMAKE_CURRENT_SOURCE_DIR}/multi_array.h
        ${CMAKE_CURRENT_SOURCE_DIR}/noalg.c
        ${CMAKE_CURRENT_SOURCE_DIR}/prim.c
        ${CMAKE_CURRENT_SOURCE_DIR}/pwl.c
        ${CMAKE_CURRENT_SOURCE_DIR}/pwl.h
        ${CMAKE_CURRENT_SOURCE_DIR}/pwl_static.h
        ${CMAKE_CURRENT_SOURCE_DIR}/replace.c
        ${CMAKE_CURRENT_SOURCE_DIR}/top_down.c
        ${CMAKE_CURRENT_SOURCE_DIR}/tree.c
        ${CMAKE_CURRENT_SOURCE_DIR}/treemap.c
        ${CMAKE_CURRENT_SOURCE_DIR}/treesize.c
        ${CMAKE_CURRENT_SOURCE_DIR}/two_level.c
        ${CMAKE_CURRENT_SOURCE_DIR}/virtual_del.c
        ${CMAKE_CURRENT_SOURCE_DIR}/virtual_net.c
        )
add_library(map STATIC ${${project}_SOURCES})