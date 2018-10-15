docdir = @SIS_DOCDIR@
AM_CPPFLAGS = -DSIS -I../include
AM_YFLAGS = -d

BUILT_SOURCES = readlib.c readlib.h readliblex.c
CLEANFILES = $(BUILT_SOURCES)

noinst_LIBRARIES = libgenlib.a
libgenlib_a_SOURCES = aoi.c com_genlib.c comb.c comb.h count.c genlib.c \
    genlib.h genlib_int.h io.c nand.c permute.c sptree.c sptree.h \
    readlib.y readliblex.l
dist_doc_DATA = genlib.doc

readlib.h: readlib.c
readlib.c readlib.h: readlib.y
	$(YACC) $(YFLAGS) $(AM_YFLAGS) $<
	sed 's/yy/GENLIB_yy/g' y.tab.c > readlib.c
	sed 's/yy/GENLIB_yy/g' y.tab.h > readlib.h
	$(RM) y.tab.c y.tab.h
readliblex.c: readliblex.l readlib.h
	$(LEX) $(LFLAGS) $(AM_LFLAGS) $<
	sed 's/yy/GENLIB_yy/g' lex.yy.c > readliblex.c
	$(RM) lex.yy.c
