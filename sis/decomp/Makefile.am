docdir = @SIS_DOCDIR@
AM_CPPFLAGS = -DSIS -I../include

noinst_LIBRARIES = libdecomp.a
libdecomp_a_SOURCES = com_dec.c dec_alg.c dec_sm.c dec_tech.c dec_util.c \
    decomp.c disj.c partition.c decomp_int.h
pkginclude_HEADERS = decomp.h
dist_doc_DATA = decomp.txt
