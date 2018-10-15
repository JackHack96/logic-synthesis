docdir = @SIS_DOCDIR@
AM_CPPFLAGS = -DSIS -I../include

noinst_LIBRARIES = libdelay.a
libdelay_a_SOURCES = com_delay.c delay.c mapdelay.c tdc_delay.c \
    delay_int.h tdc_int.h
pkginclude_HEADERS = delay.h
dist_doc_DATA = delay.doc
