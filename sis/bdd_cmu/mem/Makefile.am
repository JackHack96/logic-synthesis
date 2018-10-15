libmem_a_SOURCES_local = memblock.c memrec.c memint.h memuser.h

if SIS_COND_CMUBDD
noinst_LIBRARIES = libmem.a
libmem_a_SOURCES = $(libmem_a_SOURCES_local)
dist_man3_MANS = mem.3
endif

EXTRA_DIST = $(libmem_a_SOURCES_local) mem.3
