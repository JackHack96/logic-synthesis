AM_CPPFLAGS = -I$(srcdir)/../hash -I$(top_srcdir)/sis/mincov \
    -I$(top_srcdir)/sis/sparse -I$(top_srcdir)/sis/util
LDADD = ../hash/libhash.a ../../sis/mincov/libmincov.a \
    ../../sis/sparse/libsparse.a ../../sis/util/libutil.a

bin_PROGRAMS = stamina
stamina_SOURCES = bcover.c class.c conf.c disjoint.c global.h \
    install_state.c iso.c main.c map.c max.c max.h merge.c merge.h \
    misc.c prime.c read_fsm.c stack.c stack.h struct.h user.h
