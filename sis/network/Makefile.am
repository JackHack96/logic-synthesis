docdir = @SIS_DOCDIR@
AM_CPPFLAGS = -DSIS -I../include

noinst_LIBRARIES = libnetwork.a
libnetwork_a_SOURCES = acyclic.c append.c cleanup.c com_network.c \
    dfs.c esp.c net2pla.c net_seq.c net_verify.c netchk.c netclp.c \
    netmake.c network_util.c pla2net.c sweep.c
pkginclude_HEADERS = network.h
dist_doc_DATA = network.txt
