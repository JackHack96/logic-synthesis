docdir = @SIS_DOCDIR@
AM_CPPFLAGS = -DSIS -I../include

noinst_LIBRARIES = libgraph.a
libgraph_a_SOURCES = com_graph.c graph.c graph.txt graph_dfs.c graph_s.c \
    graph_int.h graph_static_int.h
pkginclude_HEADERS = graph.h graph_static.h
dist_doc_DATA = graph.txt graph_static.txt
