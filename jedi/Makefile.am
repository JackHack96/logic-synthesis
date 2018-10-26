AM_CPPFLAGS = -I$(top_srcdir)/port -I$(top_srcdir)/utility
LDADD = ../utility/libutility.a -lm

bin_PROGRAMS = jedi
jedi_SOURCES = cluster.c expand.c getopt.c hill.c hot.c jedi.c jedi.h \
    jedi_int.h options.c random.c read.c rp.c rp.h rp_int.h util.c util.h \
    weights.c write.c
dist_man1_MANS = jedi.1
