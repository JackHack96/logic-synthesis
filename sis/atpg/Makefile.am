docdir = @SIS_DOCDIR@
AM_CPPFLAGS = -DSIS -I../include

noinst_LIBRARIES = libatpg.a
libatpg_a_SOURCES = atpg_clauses.c atpg_comb.c atpg_faults.c \
    atpg_faultsim.c atpg_gen_test.c atpg_init.c atpg_seq.c atpg_seq_util.c \
    atpg_util.c com_atpg.c com_redund.c com_short_t.c fast_avl.c sat.c \
    atpg_int.h fast_avl.h sat_int.h
pkginclude_HEADERS = atpg.h sat.h
dist_doc_DATA = atpg.txt
