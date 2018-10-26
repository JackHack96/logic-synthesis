docdir = @SIS_DOCDIR@
AM_CPPFLAGS = -DSIS -I../include

noinst_LIBRARIES = libastg.a
libastg_a_SOURCES = astg_blif.c astg_cmds.c astg_contract.c astg_core1.c \
    astg_core2.c astg_flow.c astg_irred.c astg_lkgraph.c astg_marking.c \
    astg_persist.c astg_read.c astg_reduce.c bwd_code.c bwd_com.c \
    bwd_hazard.c bwd_io.c bwd_lp.c bwd_min_delay.c bwd_slow.c bwd_stg_to_f.c \
    bwd_util.c com_astg.c si_com.c si_encode.c si_min.c astg_core.h \
    astg_int.h bwd_int.h min_delay_int.h si_int.h
pkginclude_HEADERS = astg.h
dist_doc_DATA = astg.txt
