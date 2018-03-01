docdir = @SIS_DOCDIR@

libutil_a_SOURCES_local = cpu_stats.c cpu_time.c getopt.c pathsearch.c \
    pipefork.c prtime.c restart.c safe_mem.c saveimage.c state.c \
    strsav.c stub.c texpand.c tmpfile.c

noinst_LIBRARIES = libutil.a
libutil_a_SOURCES = $(libutil_a_SOURCES_local)
dist_doc_DATA = util.doc

if SIS_COND_CUDD
pkginclude_HEADERS = ansi.h
else
pkginclude_HEADERS = ansi.h util.h
endif

EXTRA_DIST = test-restart.c util.h
