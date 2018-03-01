docdir = @SIS_DOCDIR@
AM_CPPFLAGS = -DSIS -I../include

noinst_LIBRARIES = libpower.a
libpower_a_SOURCES = com_power.c power_comb.c power_comp.c \
    power_dynamic.c power_main.c power_pipe.c power_psAppr.c \
    power_psExact.c power_sample.c power_seq.c power_sim.c power_util.c \
    power_int.h
pkginclude_HEADERS = power.h
dist_doc_DATA = power.doc
