docdir = @SIS_DOCDIR@
AM_CPPFLAGS = -DSIS -I../include

noinst_LIBRARIES = libextract.a
libextract_a_SOURCES = best_subk.c com_ex.c conv.c cube.c cubeindex.c \
    ddivisor.c fast_extract.c gdivisor.c gen_2c_kernel.c genkern.c \
    greedycol.c greedyrow.c heap.c kernel.c misc.c pingpong.c qdivisor.c \
    rect.c sdivisor.c value.c \
    extract_int.h fast_extract_int.h heap.h
pkginclude_HEADERS = extract.h
dist_doc_DATA = extract.doc
