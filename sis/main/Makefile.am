docdir = @SIS_DOCDIR@
MYDATE = $(shell date '+%Y-%m-%d %H:%M:%S')
AM_CPPFLAGS = -DSIS '-DCUR_DATE="$(MYDATE)"' \
'-DLIBRARY="@SIS_SISLIBDIR@"' -I../include

noinst_LIBRARIES = libmain.a
libmain_a_SOURCES = sis.c sis_glbs.c sis_init.c version.c
pkginclude_HEADERS = sis.h
dist_doc_DATA = sis.doc
