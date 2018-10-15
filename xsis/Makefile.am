xsis_SOURCES_local = NetPlot.c NetPlot.h NetPlotP.h main.c xastg.c \
    xblif.c xcmd.c xhelp.c xsis.c xsis.h xutil.c \
    blif50.px ghost.px help50.px sis50.px
AM_CPPFLAGS = -I../sis/include -I@SIS_X_INCLUDES@
AM_LDFLAGS = -L@SIS_X_LIBRARIES@
LDADD = ../sis/libsis.a -lXaw -lXmu -lXt -lXext -lX11 -lm

if SIS_COND_X
  bin_PROGRAMS = xsis
  xsis_SOURCES = $(xsis_SOURCES_local)
  dist_man1_MANS = xsis.1
endif

EXTRA_DIST = $(xsis_SOURCES_local) xsis.1 fakesis.c
