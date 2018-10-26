docdir = @SIS_DOCDIR@
AM_CPPFLAGS = -DSIS -I../include -I$(top_srcdir)/port

noinst_LIBRARIES = libcommand.a
libcommand_a_SOURCES = addcom.c alias.c cmd_set.c command.c commisc.c \
    filec.c get_nodes.c get_true_po.c help.c open_file.c source.c undo.c \
    com_int.h
pkginclude_HEADERS = command.h
dist_doc_DATA = command.txt
