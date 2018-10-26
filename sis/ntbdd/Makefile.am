docdir = @SIS_DOCDIR@
AM_CPPFLAGS = -DSIS -I../include

noinst_LIBRARIES = libntbdd.a
libntbdd_a_SOURCES = bdd_at_node.c bdd_to_ntwk.c com_ntbdd.c manager.c \
    node_to_bdd.c verify_ntwk.c ntbdd_int.h
pkginclude_HEADERS = ntbdd.h
dist_doc_DATA = ntbdd.txt
