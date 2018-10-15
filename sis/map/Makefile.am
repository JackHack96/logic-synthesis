docdir = @SIS_DOCDIR@
AM_CPPFLAGS = -DSIS -I../include

noinst_LIBRARIES = libmap.a
libmap_a_SOURCES = addinv.c bin_delay.c bottom_up.c chkform.c cluster.c \
    com_map.c fanout_delay.c fanout_dump.c fanout_est.c fanout_log.c \
    fanout_opt.c fanout_tree.c fanout_util.c gate_link.c hackit.c library.c \
    libutil.c lt_trees.c map_delay.c map_interface.c maputil.c match.c \
    mix_lt_trees.c multi_array.c noalg.c prim.c pwl.c replace.c top_down.c \
    tree.c treemap.c treesize.c two_level.c virtual_del.c virtual_net.c \
    bin_delay_static.h bin_int.h cluster.h cluster_static.h fanout_alg.h \
    fanout_alg_macro.h fanout_delay.h fanout_delay_static.h \
    fanout_est_static.h fanout_int.h fanout_opt_static.h \
    fanout_tree_static.h gate_link.h lib_int.h map_defs.h \
    map_delay.h map_int.h map_interface_static.h map_macros.h multi_array.h \
    pwl.h pwl_static.h
pkginclude_HEADERS = library.h map.h
dist_doc_DATA = library.doc map.doc
