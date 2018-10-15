/* -------------------------------------------------------------------------- *\
   NetPlot.h -- public declarations for NetPlot Widget.

	$Revision: 1.1.1.1 $
	$Date: 2004/02/07 10:14:14 $
	$Author: pchong $
	$Source: /users/pchong/CVS/sis/xsis/NetPlot.h,v $

   Copyright 1991 by the Regents of the University of California.

   All rights reserved.  Permission to use, copy, modify and distribute
   this software is hereby granted, provided that the above copyright
   notice and this permission notice appear in all copies.  This software
   is made available as is, with no warranties.

   Resources recognized by NetPlot widgets.  Since NetPlot widgets use more
   than one foreground color, you should use the class Foreground to set
   all foreground colors to a single color, e.g. "NetPlot*Foreground" vs.
   "NetPlot*foreground".

   Name			Class		RepType		Default Value
   ----			-----		-------		-------------
   background		Background	Pixel		White
   foreground1		Foreground	Pixel		Black
   foreground2		Foreground	Pixel		Black
   foreground3		Foreground	Pixel		Black
   foreground4		Foreground	Pixel		Black
   hilight		Hilight		Pixel		Black
   font			Font		FontStruct	?
   selectCallback	Callback	Callback	NULL		
   lineWidth		LineWidth	Width		0
\* -------------------------------------------------------------------------- */

#ifndef _NetPlot_h
#define _NetPlot_h

#define XtNselectCallback	"selectCallback"
#define XtNforeground1		"foreground1"
#define XtNforeground2		"foreground2"
#define XtNforeground3		"foreground3"
#define XtNforeground4		"foreground4"
#define XtNlineWidth		"lineWidth"

	/* Class constant (i.e. factory object). */

extern WidgetClass netPlotWidgetClass;

	/* Convenience functions. */

typedef enum np_action {
    NP_ERASE,			/* Erase the background area.	*/
    NP_FRAME,			/* Draw suitable frame shape.	*/
    NP_DRAW,			/* Draw entire graphic.		*/
    NP_CALC_WIDTH,		/* Calc. width field.		*/
    NP_CALC_HEIGHT		/* Calc. height field.		*/
} np_action;


typedef struct NetPlotSelectData {	/* XtNselectCallback data.	*/
    String select_name;			/* Name of new selection.	*/
} NetPlotSelectData;

typedef struct np_node	np_node;
typedef struct np_arc	np_arc;

typedef int (*np_shape) ARGS((np_node *, np_action));

extern int np_dummy	ARGS((np_node *, np_action));
extern int np_textblock	ARGS((np_node *, np_action));
extern int np_rectangle	ARGS((np_node *, np_action));
extern int np_ellipse	ARGS((np_node *, np_action));
extern int np_itrans	ARGS((np_node *, np_action));
extern int np_otrans	ARGS((np_node *, np_action));

extern void	 np_clear_plot ARGS((Widget));
extern np_node	*np_find_node ARGS((Widget, String, Bool));
extern np_node	*np_create_node ARGS((Widget, String));
extern void	 np_set_shape ARGS((np_node *, np_shape));
extern void	 np_set_label ARGS((np_node *, String));
extern void	 np_set_position ARGS((np_node *, double, double));
extern void	 np_orient ARGS((Widget, int));
extern void	 np_set_node_color ARGS((np_node *, unsigned));
extern np_arc	*np_create_arc ARGS((np_node *, np_node *, String));
extern np_arc	*np_find_arc ARGS((np_node *, np_node *));
extern np_node	*np_insert_bend ARGS((np_arc *));
extern void	 np_redraw ARGS((Widget, XRectangle*, int));
extern void	 np_autoplace ARGS((Widget));
extern void	 np_set_title ARGS((Widget, String));
extern void	 np_new_highlight ARGS((Widget, String));
extern void	 np_highlight_node ARGS((np_node *, Bool));
extern void	 np_highlight_arc ARGS((np_arc *, Bool));

#endif /* _NetPlot_h */
