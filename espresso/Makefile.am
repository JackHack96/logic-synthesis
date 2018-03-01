AM_CPPFLAGS = -I$(top_srcdir)/utility -I$(top_srcdir)/port
LDADD = ../utility/libutility.a

SUBDIRS = examples

bin_PROGRAMS = espresso
espresso_SOURCES = cofactor.c cols.c compl.c contain.c cubestr.c \
    cvrin.c cvrm.c cvrmisc.c cvrout.c dominate.c equiv.c espresso.c \
    essen.c exact.c expand.c gasp.c getopt.c gimpel.c globals.c hack.c \
    indep.c irred.c main.c map.c matrix.c mincov.c opo.c pair.c part.c \
    primes.c reduce.c rows.c set.c setc.c sharp.c sminterf.c solution.c \
    sparse.c unate.c verify.c espresso.h main.h mincov.h mincov_int.h \
    sparse.h sparse_int.h
dist_man1_MANS = espresso.1
dist_man5_MANS = espresso.5 pla.5
