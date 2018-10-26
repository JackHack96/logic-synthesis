AM_CPPFLAGS = -I../sis/include
AM_YFLAGS = -d
LDADD = ../sis/libsis.a -lm

BUILT_SOURCES = gram.c gram.h lex.c
CLEANFILES = $(BUILT_SOURCES)

bin_PROGRAMS = sred
sred_SOURCES = action.c boolcmp.c chinclus.c chiusura.c choose.c clinchain.c \
    clinclus.c clincop.c closure.c coloring.c comp.c connected.c esp_color.c \
    exist.c existdf.c generate.c ibincompa.c incompat.c iobincomp.c \
    label.c main.c makeout.c misc.c mxcomptbl.c newnstate.c newoutput.c \
    newprime.c obincompa.c prime.c primeones.c procargs.c qsort.c qspart.c \
    qspivot.c reduced.c reductio.h solution.c strbar.c strcom.c \
    gram.y lex.l
dist_man1_MANS = sred.1
