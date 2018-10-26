docdir = @SIS_DOCDIR@
AM_CPPFLAGS = -I../include

if !SIS_COND_CUDD
noinst_LIBRARIES = libst.a
libst_a_SOURCES = st.c
pkginclude_HEADERS = st.h
dist_doc_DATA = st.txt
endif

EXTRA_DIST = st.c st.h st.txt st_bench1.c
