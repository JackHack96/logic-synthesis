/* -------------------------------------------------------------------------- *\
   NetPlotP.h -- private declarations of NetPlot Widget.

	$Revision: 1.1.1.1 $
	$Date: 2004/02/07 10:14:14 $
	$Author: pchong $
	$Source: /users/pchong/CVS/sis/xsis/NetPlotP.h,v $

   Copyright 1991 by the Regents of the University of California.

   All rights reserved.  Permission to use, copy, modify and distribute
   this software is hereby granted, provided that the above copyright
   notice and this permission notice appear in all copies.  This software
   is made available as is, with no warranties.
\* -------------------------------------------------------------------------- */

#ifndef _NetPlotP_h
#define _NetPlotP_h

#ifndef MAKE_DEPEND
#include <stdio.h>
#include <math.h>
#include <X11/Xlib.h>
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>
#include <X11/CoreP.h>		/* Superclass private header file.	*/
#include <util.h>
#include <list.h>
#include <st.h>
#endif
#include "NetPlot.h"		/* Public declaractions for widget.	*/


typedef struct _NetPlotClassRec*	NetPlotWidgetClass;
typedef struct _NetPlotRec*		NetPlotWidget;

typedef struct np_rect {	/* rect with float dimensions.		*/
    float x,y;
    float width,height;
} np_rect;

typedef struct {		/* Extensions to parent class.		*/
    int dummy;			/* No extensions.			*/
} NetPlotClassPart;

typedef struct _NetPlotClassRec {
    CoreClassPart	core_class;
    NetPlotClassPart	netPlot_class;
} NetPlotClassRec;

#define NP_MAX_COLORS	5

typedef struct {		/* Extensions to widget instance.	*/
    Pixel	 colors[NP_MAX_COLORS];	/* Drawing colors for figures.	*/
    Pixel	 highlight;	/* To highlight parts of the net.	*/
    XFontStruct *font;		/* Font for labels in widget.		*/
    int		 font_height;	/* Height for graphics scaling.		*/
    XtCallbackList select_callbacks; /* When selection changes.		*/
    int		 linewidth;	/* For drawing in the GC.		*/
    				/* Private state.			*/
    String	 label;		/* Label of graph.			*/
    Bool	 unplaced;	/* True if objects never placed.	*/
    Bool	 unscaled;	/* True=user coord. frame unknown.	*/
    Bool	 exposed;	/* True if window has been exposed.	*/
    Display	*dpy;		/* Display for widget window.		*/
    Window	 win;		/* Window of plot widget.		*/
    GC		 gc;		/* Single changable GC.			*/
    Pixel	 overlay;	/* Mask for drawing overlays on image.	*/
    st_table	*node_table;	/* For direct access by name.		*/
    lsList	 nodes;
    lsList	 arcs;
    Bool	 vertical;	/* Arcs attach to top/bottom?		*/

    float	 xt_user;	/* Transform user/window coordinates.	*/
    float	 yt_user;	/* translate user origin to win origin  */
    float	 x_scale;	/* x scale: pixels per user unit.	*/
    float	 y_scale;	/* y scale: pixels per user unit.	*/

    float	 x_pix_per_mm;	/* display scale: pixels per mm.	*/
    float	 y_pix_per_mm;	/* display scale: pixels per mm.	*/
    int		 win_w, win_h;	/* From drawstg.c -- coordinate info.	*/
    short        root_height;
    short        root_width;

    String	 sel_label;	/* Label of current selection.		*/
    /* int		 maxx, maxy; window dimensions */
    int		 xlo, xhi;
    int		 ylo, yhi;
    int		 xspacing;
    int		 yspacing;
    Bool	 track_on;	/* True when track_rect is drawn.	*/
    XPoint	 hot_spot;	/* Where initial click was made.	*/
    XPoint	 cur_diff;	/* Place where last was highlighted.	*/
    np_node	*selected;	/* Current selected node.		*/
} NetPlotPart;

typedef struct _NetPlotRec {	/* New widget instance struct.		*/
    CorePart	 core;
    NetPlotPart	 netPlot;
} NetPlotRec;


struct np_node {
    NetPlotWidget widget;	/* Graph containing this node.	*/
    np_shape	 shape;		/* How to draw this thing.	*/
    char	*name;		/* Name for table lookup.	*/
    char	*label;		/* Label to print in plot.	*/
    int		 highlighted;	/* 0=none, 1=frame, 2=all.	*/
    XRectangle	 bounds;	/* Window bounds for object.	*/
    Bool	 dummy;		/* Node is a dummy node.	*/
    int		 color;		/* Index into colors table.	*/
    int		 level,vertpos;	/* Generic x,y placement.	*/
    float	 x,y;		/* User coord. of object.	*/
    int		 grids_high;	/* For generic placement algo.	*/
    lsList	 fanins;	/* Fanin edges.			*/
    lsList	 fanouts;	/* Fanout edges.		*/
};

struct np_arc {
    NetPlotWidget widget;
    char	*label;
    np_node	*from;
    np_node	*to;
    int		 style;		/* LineSolid, LineOnOffDash, ...*/
    Bool	 highlighted;
};


#define num_fanins(node)	lsLength((node)->fanins)
#define num_fanouts(node)	lsLength((node)->fanouts)

#define np_lsForeachItem(list, gen, data)				\
	for (gen = lsStart(list);					\
		(lsNext(gen, (lsGeneric *) &data, LS_NH) == LS_OK)	\
		|| ((void) lsFinish(gen), 0); )

#define np_foreach_node(graph, gen, node)	\
	np_lsForeachItem((graph)->nodes, gen, node)

#define np_foreach_arc(graph, gen, arc)	\
	np_lsForeachItem((graph)->arcs, gen, arc)

#define np_foreach_fanin(node, gen, arc)	\
	np_lsForeachItem((node)->fanins, gen, arc)

#define np_foreach_fanout(node, gen, arc)	\
	np_lsForeachItem((node)->fanouts, gen, arc)

#endif /* _NetPlotP_h */
