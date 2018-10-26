docdir = @SIS_DOCDIR@
AM_CPPFLAGS = -DSIS -I../include

noinst_LIBRARIES = libtiming.a
libtiming_a_SOURCES = com_timing.c timing_comp.c timing_graph.c \
    timing_seq.c timing_util.c timing_verify.c timing_int.h
dist_doc_DATA = timing.txt
EXTRA_DIST = timing_fast_comp.c
