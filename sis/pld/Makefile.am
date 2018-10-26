docdir = @SIS_DOCDIR@
AM_CPPFLAGS = -DSIS -I../include

noinst_LIBRARIES = libpld.a
libpld_a_SOURCES = act_apply.c act_bdd.c act_bool.c act_collapse.c \
    act_create.c act_delay.c act_dutil.c act_init.c act_ite.c act_ite_new.c \
    act_leaf.c act_map.c act_misc.c act_order.c act_read.c act_reduce.c \
    act_remove.c act_urp.c act_util.c com_ite.c com_pld.c ite_break.c \
    ite_collapse.c ite_factor.c ite_imp.c ite_leaf.c ite_map.c ite_mroot.c \
    ite_mux_net.c ite_new_map.c ite_new_urp.c ite_pld.c ite_urp.c ite_util.c \
    pld_util.c xln_ULM_util.c xln_aodecomp.c xln_aux.c xln_collapse.c \
    xln_cube.c xln_dec_merge.c xln_feasible.c xln_filter.c xln_imp.c \
    xln_k_de_area.c xln_k_decomp.c xln_level.c xln_lindo.c xln_map_par.c \
    xln_merge.c xln_move_d.c xln_new_part.c xln_part_dec.c xln_ufind.c \
    act_bool.h ite_int.h pld_int.h
pkginclude_HEADERS = pld.h
dist_doc_DATA = pld.txt
