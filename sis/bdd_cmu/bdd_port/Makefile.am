AM_CPPFLAGS = -I../../include -I$(srcdir)/../bdd_cmu -I$(srcdir)/../mem

libbdd_port_a_SOURCES_local = bdditer.c bddport.c bdd.h

if SIS_COND_CMUBDD
noinst_LIBRARIES = libbdd_port.a
libbdd_port_a_SOURCES = $(libbdd_port_a_SOURCES_local)
endif

EXTRA_DIST = $(libbdd_port_a_SOURCES_local)
