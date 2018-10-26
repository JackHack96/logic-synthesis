docdir = @SIS_DOCDIR@
AM_CPPFLAGS = -I../include

libbdd_ucb_a_SOURCES_local = adhoc_cache.c and_smooth.c assert_frame.c \
    assert_heap.c bdd_cofactor.c bdd_compose.c bdd_cproject.c bdd_end.c \
    bdd_ite.c bdd_iter.c bdd_min_sibl.c bdd_min_util.c bdd_print.c \
    bdd_quantify.c bdd_start.c bdd_substit.c bdd_support.c bdd_tovar.c \
    bdd_util.c boolean_ops.c config.c const_cache.c dmp_ext_ptrs.c \
    dmp_mgr_stat.c dmp_node_age.c external_ptr.c find_or_add.c \
    garb_collect.c hash_cache.c ite.c ite_common.c ite_constant.c \
    new_node.c print_stats.c resize_table.c bdd_int.h

if SIS_COND_UCBBDD
noinst_LIBRARIES = libbdd_ucb.a
libbdd_ucb_a_SOURCES = $(libbdd_ucb_a_SOURCES_local)
pkginclude_HEADERS = bdd.h
dist_doc_DATA = bdd.txt
endif

EXTRA_DIST = $(libbdd_ucb_a_SOURCES_local) bdd.txt bdd.h
