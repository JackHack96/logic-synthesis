docdir = @SIS_DOCDIR@
AM_CPPFLAGS = -DSIS -I../include
AM_YFLAGS = -d

BUILT_SOURCES = read_eqn.c read_eqn.h eqnlex.c
CLEANFILES = $(BUILT_SOURCES)

noinst_LIBRARIES = libio.a
libio_a_SOURCES = com_io.c plot_blif.c read_blif.c read_kiss.c read_pla.c \
    read_slif.c read_util.c write_bdnet.c write_blif.c write_eqn.c \
    write_kiss.c write_pla.c write_slif.c write_util.c io_int.h \
    read_eqn.y eqnlex.l
pkginclude_HEADERS = io.h
dist_doc_DATA = io.txt

read_eqn.h: read_eqn.c
read_eqn.c read_eqn.h: read_eqn.y
	$(YACC) $(YFLAGS) $(AM_YFLAGS) $<
	sed 's/yy/EQN_yy/g' y.tab.c > read_eqn.c
	sed 's/yy/EQN_yy/g' y.tab.h > read_eqn.h
	$(RM) y.tab.c y.tab.h
eqnlex.c: eqnlex.l read_eqn.h
	$(LEX) $(LFLAGS) $(AM_LFLAGS) $<
	sed 's/yy/EQN_yy/g' lex.yy.c > eqnlex.c
	$(RM) lex.yy.c
