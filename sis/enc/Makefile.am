docdir = @SIS_DOCDIR@
AM_CPPFLAGS = -DSIS -I../include

noinst_LIBRARIES = libenc.a
libenc_a_SOURCES = com_enc.c dic.c dic_to_sm.c gen_eqn.c input.c \
    enc_int.h
pkginclude_HEADERS = enc.h
dist_doc_DATA = enc.txt

EXTRA_DIST = interface.c
