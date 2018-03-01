docdir = @SIS_DOCDIR@
AM_CPPFLAGS = -DSIS -I../include

noinst_LIBRARIES = libnode.a
libnode_a_SOURCES = cofct.c collapse.c com_node.c cubehack.c divide.c \
    fan.c invert.c iphase.c libhack.c names.c node.c nodecheck.c \
    nodeindex.c nodemisc.c nodeutil.c print.c sethack.c substitute.c \
    node_int.h
pkginclude_HEADERS = node.h nodeindex.h
dist_doc_DATA = node.doc nodeindex.doc
