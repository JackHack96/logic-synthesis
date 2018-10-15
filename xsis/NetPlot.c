/* -------------------------------------------------------------------------- *\
   NetPlot.c -- implementation of NetPlot Widget.

	$Revision: 1.1.1.1 $
	$Date: 2004/02/07 10:14:14 $
	$Author: pchong $
	$Source: /users/pchong/CVS/sis/xsis/NetPlot.c,v $

   Copyright 1991 by the Regents of the University of California.

   All rights reserved.  Permission to use, copy, modify and distribute
   this software is hereby granted, provided that the above copyright
   notice and this permission notice appear in all copies.  This software
   is made available as is, with no warranties.

   Globals: netPlotWidgetClass

   Have to use XDrawString instead of XDrawImageString in order to get the
   GC function to be recognized.

   Maybe change to allow NP_MAX_COLORS to be set as resource before the
   widget is created?

   Scale factor from user to widget coordinates is defined by the widget font
   size: font height = 1.0 user units.  Thus nodes have a fixed height and
   width regardless of widget size (immutable).  Positioning depends on widget
   size and scale factors.  As the user space is "compressed" the objects get
   closer together, but remain the same size.  Thus the arcs between objects
   change size/position (mutable).
\* -------------------------------------------------------------------------- */

#include "NetPlotP.h"

#define xalloc(type,n) \
    (type*)XtMalloc((Cardinal)(sizeof(type)*(unsigned)(n)))
#define xrealloc(type,p,n) \
    (type*)XtRealloc((char*)(p),(Cardinal)sizeof(type)*(unsigned)(n))
#define xfree(p) \
    XtFree((char*)p)


#define X_PAD		6
#define Y_PAD		6
#define ARROW_LENGTH    12
#define ARROW_ANGLE    (3.1415926 / 10)

#define x_trans(xd,x)	ceil(((x)-(xd)->xt_user)*(xd)->x_scale)
#define y_trans(xd,y)	ceil(((y)-(xd)->yt_user)*(xd)->y_scale)


/* --------------------------- float Rectangles ----------------------------- */

static void np_clear_rect (r)
XRectangle* r;
{
    r->x = r->y = r->width = r->height = 0;
}

static Boolean np_empty_rect (r)
XRectangle* r;
{
    return (r->width <= 0 || r->height <= 0);
}

static void np_wrap_rect (dest,r1,r2)
XRectangle* dest;
XRectangle* r1;
XRectangle* r2;
{
    /*	Note: includes r2 even if it has zero area. */
    float max_x, max_y;

    if (np_empty_rect (r1)) {
	*dest = *r2;
    } else {
	max_x = MAX (r1->x+r1->width,r2->x+r2->width);
	max_y = MAX (r1->y+r1->height,r2->y+r2->height);
	dest->x = MIN (r1->x,r2->x);
	dest->y = MIN (r1->y,r2->y);
	dest->width = max_x - dest->x;
	dest->height = max_y - dest->y;
    }
}

/* --------------------------- User Coordinates ----------------------------- */

static float winToUserX (view,win_x)
NetPlotPart* view;
int win_x;
{
    float x = win_x;
    x = (x/view->x_scale) + view->xt_user;
    return x;
}

static float winToUserY (view,win_y)
NetPlotPart* view;
int win_y;
{
    float y = win_y;
    y = (y/view->y_scale) + view->yt_user;
    return y;
}

/* ---------------------------- Implementation ------------------------------ */

static int np_numlines (label)
char *label;
{
    /*	Eventually check for newline characters in label. */
    /*  ARGSUSED */

    return 1;
}

int np_dummy (node,action)
np_node *node;
np_action action;
{
    /*	For dummy internal nodes: do nothing at all. */

    NetPlotPart *xd = &(node->widget->netPlot);

    if (action == NP_FRAME) {
        XFillRectangle (xd->dpy,xd->win,xd->gc,
                        node->bounds.x-X_PAD/2, node->bounds.y-Y_PAD/2,
			X_PAD, Y_PAD);
    }
    return 0;
}

static String np_name (node)
np_node *node;
{
    String label = node->name;
    if (label == NULL) label = "?";
    return label;
}

static String np_label (node)
np_node *node;
{
    /*	Return some non-NULL label string for this node. */

    String label = node->label;
    if (label == NULL) {
	label = np_name(node);
	if (label == NULL) label = " ";
    }
    return label;
}

static void np_draw_label (node)
np_node* node;
{
    /*	Draw standard label for a netPlot node. */

    char *label = np_label(node);
    NetPlotPart *xd = &(node->widget->netPlot);

    if (label != NULL) {
	XDrawString (xd->dpy,xd->win,xd->gc,
	     node->bounds.x + X_PAD/2,
	     node->bounds.y + Y_PAD/2 + xd->font->max_bounds.ascent,
	     label, strlen(label));
    }
}

int np_itrans (node,action)
np_node *node;
np_action action;
{
    int result = 0;
    int y;
    NetPlotPart *xd = &(node->widget->netPlot);
    char *label = np_label(node);

    if (action == NP_ERASE) {
        XFillRectangle (xd->dpy, xd->win, xd->gc,
                        node->bounds.x+X_PAD/2, node->bounds.y+Y_PAD/2,
                        node->bounds.width-X_PAD, node->bounds.height-X_PAD);
    }
    else if (action == NP_DRAW || action == NP_FRAME) {
        np_draw_label (node);
	y = node->bounds.y + node->bounds.height - Y_PAD/2 - 
		    xd->font->max_bounds.descent/2;
	XDrawLine (xd->dpy,xd->win,xd->gc,
		   node->bounds.x+X_PAD/2,y,
		   node->bounds.x+node->bounds.width-X_PAD/2,y);
    }
    else if (action == NP_CALC_WIDTH) {
	result = X_PAD + XTextWidth (xd->font, label, strlen(label));
    }
    else if (action == NP_CALC_HEIGHT) {
        result = Y_PAD + xd->font_height * (np_numlines(label));
    }
    return result;
}

int np_otrans (node,action)
np_node *node;
np_action action;
{
    int result = 0;
    NetPlotPart *xd = &(node->widget->netPlot);
    char *label = np_label(node);

    if (action == NP_ERASE) {
        XFillRectangle (xd->dpy, xd->win, xd->gc,
                        node->bounds.x+X_PAD/2, node->bounds.y+Y_PAD/2,
                        node->bounds.width-X_PAD, node->bounds.height-Y_PAD);
    }
    else if (action == NP_DRAW || action == NP_FRAME) {
        np_draw_label (node);
    }
    else if (action == NP_CALC_WIDTH) {
	result = X_PAD + XTextWidth (xd->font, label, strlen(label));
    }
    else if (action == NP_CALC_HEIGHT) {
        result = Y_PAD + xd->font_height * (np_numlines(label));
    }
    return result;
}

int np_textblock (node,action)
np_node *node;
np_action action;
{
    int result = 0;
    NetPlotPart *xd = &(node->widget->netPlot);
    char *label = np_label(node);

    if (action == NP_ERASE) {
        XFillRectangle (xd->dpy, xd->win, xd->gc,
                        node->bounds.x, node->bounds.y,
                        node->bounds.width, node->bounds.height);
    }
    else if (action == NP_DRAW || action == NP_FRAME) {
        np_draw_label (node);
    }
    else if (action == NP_CALC_WIDTH) {
	result = XTextWidth (xd->font, label, strlen(label));
    }
    else if (action == NP_CALC_HEIGHT) {
        result = xd->font_height * (np_numlines(label));
    }
    return result;
}

int np_rectangle (node,action)
np_node *node;
np_action action;
{
    int result = 0;
    NetPlotPart *xd = &(node->widget->netPlot);
    char *label = np_label(node);

    if (action == NP_ERASE) {
        XFillRectangle (xd->dpy, xd->win, xd->gc,
			node->bounds.x, node->bounds.y,
			node->bounds.width, node->bounds.height);
    }
    else if (action == NP_DRAW || action == NP_FRAME) {
        XDrawRectangle (xd->dpy, xd->win, xd->gc,
			node->bounds.x, node->bounds.y,
			node->bounds.width-1, node->bounds.height-1);
	if (action == NP_DRAW) np_draw_label (node);
    }
    else if (action == NP_CALC_WIDTH) {
	result = XTextWidth (xd->font, label, strlen(label));
    }
    else if (action == NP_CALC_HEIGHT) {
        result = xd->font_height * (np_numlines(label));
    }
    return result;
}

int np_ellipse (node,action)
np_node *node;
np_action action;
{
    int result = 0;
    NetPlotPart *xd = &(node->widget->netPlot);
    String label = np_label(node);

    if (action == NP_ERASE) {
        XFillArc (xd->dpy, xd->win, xd->gc,
		 node->bounds.x, node->bounds.y,
		 node->bounds.width, node->bounds.height,
		 0, 360 * 64);
    }
    else if (action == NP_DRAW || action == NP_FRAME) {
        XDrawArc (xd->dpy, xd->win, xd->gc,
		 node->bounds.x, node->bounds.y,
		 node->bounds.width-1, node->bounds.height-1,
		 0, 360 * 64);
	if (action == NP_DRAW) np_draw_label (node);
    }
    else if (action == NP_CALC_WIDTH) {
	if (!strcmp(label," ")) {
	    result = xd->font_height;
	} else {
	    result = XTextWidth (xd->font, label, strlen(label));
	}
    }
    else if (action == NP_CALC_HEIGHT) {
        result = xd->font_height * (np_numlines(label));
    }

    return result;
}

static Boolean np_sect_rect (r1,r2)
XRectangle* r1;
XRectangle* r2;
{
    /*	Return False if two rectangles do not intersect, True otherwise. */

    int r1r = r1->x + r1->width, r1b = r1->y + r1->height;
    int r2r = r2->x + r2->width, r2b = r2->y + r2->height;
    Boolean disjoint =
       (r1->x >= r2r) || (r1->y >= r2b) || (r2->x >= r1r) || (r2->y >= r1b);
    return !disjoint;
}

np_arc* np_create_arc (from,to,label)
np_node* from;
np_node* to;
char *label;
{
    np_arc *arc = xalloc(np_arc, 1);

    assert(from->widget == to->widget);
    arc->label = (label) ? XtNewString(label) : NULL;
    arc->widget = from->widget;
    arc->from = from;
    arc->to = to;
    arc->style = LineSolid;
    arc->highlighted = False;

    lsNewEnd (arc->widget->netPlot.arcs, (lsGeneric) arc, (lsHandle *) NULL);
    lsNewEnd (from->fanouts, (lsGeneric) arc, (lsHandle *) NULL);
    lsNewEnd (to->fanins, (lsGeneric) arc, (lsHandle *) NULL);

    return arc;
}

np_arc* np_find_arc (from,to)
np_node* from;
np_node* to;
{
    lsGen gen;
    np_arc *arc, *found = NULL;

    np_foreach_fanout (from,gen,arc) {
	if (arc->to == to) {
	    found = arc;
	}
    }

    return found;
}

void np_set_arc_props (arc,linestyle,label)
np_arc* arc;
int linestyle;
char* label;
{
    arc->style = linestyle;
    if (arc->label) xfree(arc->label);
    arc->label = (label) ? XtNewString(label) : NULL;
}

static void np_get_arc_props (arc,linestyle,label)
np_arc* arc;
int* linestyle;
char** label;
{
    *linestyle = arc->style;
    *label = arc->label;
}

static lsStatus lsDelValue (list,userdata)
lsList list;
lsGeneric userdata;
{
    lsGeneric curr_item;
    lsGen gen = lsStart (list);

    while (lsNext(gen, &curr_item, (lsHandle *) NULL) == LS_OK) {
        if (curr_item == userdata) lsDelBefore(gen, &curr_item);
    }
    
    lsFinish (gen);
    return (LS_OK);
}

static void np_delete_arc (arc)
np_arc* arc;
{
    lsDelValue (arc->from->fanouts, (lsGeneric) arc);
    lsDelValue (arc->to->fanins, (lsGeneric) arc);
    lsDelValue (arc->widget->netPlot.arcs, (lsGeneric) arc);
    xfree (arc);
}

/* ----------------------- netplot node stuff ------------------------------- */

np_node* np_create_node2 (widget,name)
NetPlotWidget widget;
char *name;
{
    NetPlotPart *graph = &(widget->netPlot);
    np_node*node = xalloc(np_node, 1);

    lsNewEnd(graph->nodes, (lsGeneric) node,  (lsHandle *) NULL);
    node->widget = widget;

    node->dummy = False;
    node->shape = np_rectangle;
    node->fanins  = lsCreate();
    node->fanouts = lsCreate();
    node->highlighted = 0;
    node->label = NULL;
    node->color = 1;
    node->bounds.height = node->bounds.width = 0;

    if (name != NULL) {
	node->name = XtNewString(name);
	st_insert (graph->node_table,node->name,(char*)node);
    } else {
	node->name = NULL;
    }

    return node;
}

np_node* np_create_node (w,name)
Widget w;
char *name;
{
    return np_create_node2 ((NetPlotWidget)w, name);
}

np_node *np_find_node (w,name,create)
Widget w;
char *name;
Bool create;
{
    NetPlotWidget widget = (NetPlotWidget) w;
    NetPlotPart *graph = &widget->netPlot;
    np_node *node = NULL;

    if (!st_lookup(graph->node_table,name,(char**)&node) && create) {
	node = np_create_node2 (widget,name);
    }
    return node;
}

static void np_free_node (data)
lsGeneric data;
{
    /* Callback for lsDestroy. */

    np_node *node = (np_node*) data;
    char *name = node->name;
    lsGen gen;
    np_arc *arc;

    np_foreach_fanout (node, gen, arc) np_delete_arc (arc);
    np_foreach_fanin  (node, gen, arc) np_delete_arc (arc);

    lsDestroy (node->fanins,  NULL);
    lsDestroy (node->fanouts, NULL);
    if (node->label != NULL && node->label != name) {
	xfree(node->label);
    }
    if (name != NULL) {
	st_delete (node->widget->netPlot.node_table, &name, NULL);
	xfree(node->name);
    }
    xfree(node);
}

void np_set_node_color (node,color)
np_node *node;
unsigned color;
{
    if (color >= NP_MAX_COLORS) {
	node->color = NP_MAX_COLORS - 1;
    } else {
	node->color = color;
    }
}

void np_set_label (node,label)
np_node *node;
String label;
{
    /* Change the node label. */
    xfree (node->label);
    node->label = (label) ? XtNewString(label) : NULL;
}

void np_set_shape (node,shape)
np_node *node;
np_shape shape;
{
    /*  Change the node shape. */
    node->shape = shape;
}

/* ----------------------- Generic Node Placement --------------------------- *\

   Assign values to (node->x,node->y) for each node to place it on an
   arbitrary grid.  Node fields level, vertpos, and grids_high are used as
   work field.  Dummy nodes are added to long edges to allow them to be
   positioned more precisely.  We could use splines or some other curve
   fitting for these, but it is not currently supported.

   The original version of the placement code was written by Wayne A.
   Christopher, CAD group 1988.
\* -------------------------------------------------------------------------- */

np_node* np_insert_bend (arc)
np_arc* arc;
{
    np_node *newnode;
    int linestyle;
    char *label;
    
    newnode = np_create_node2 (arc->widget,NULL);
    newnode->dummy = True;
    newnode->level = 0;
    newnode->shape = np_dummy;

    np_get_arc_props (arc, &linestyle, &label);
    np_create_arc (arc->from, newnode, label);
    np_create_arc (newnode, arc->to, NULL);
    np_delete_arc (arc);
    return newnode;
}

static void np_check_uniform (node)
np_node* node;
{
    /*	Make sure that every path from the inputs to the outputs of a
	graph has a node at every level. */

    lsGen gen;
    np_arc *arc;
    np_node *fanout, *bend;

    np_foreach_fanout (node, gen, arc) {
        fanout = arc->to;
        if (fanout->level < node->level - 1) {
            bend = np_insert_bend (arc);
	    bend->level = node->level - 1;
	}
    }

    np_foreach_fanout (node, gen, arc) {
        fanout = arc->to;
        np_check_uniform (fanout);
    }
}

static void np_assign_levels (node)
np_node* node;
{
    lsGen gen;
    np_arc *arc;
    np_node *fanout;

    if (node->level < 0) {
	node->level = 0;
	np_foreach_fanout (node, gen, arc) {
	    fanout = arc->to;
	    np_assign_levels (fanout);
	    if (fanout->level >= node->level) node->level = fanout->level + 1;
	}
    }
}

static void np_make_uniform (graph)
NetPlotPart *graph;
{
    np_node *node;
    lsGen gen;

    np_foreach_node (graph, gen, node) {
        node->level = -1;
    }

    np_foreach_node (graph, gen, node) {
	np_assign_levels (node);
    }

    np_foreach_node (graph, gen, node) {
        if (num_fanins(node) == 0) np_check_uniform (node);
    }
}

/* -------------------------------------------------------------------------- *\
   Here's the real stuff.  Assign relative positions of the nodes in all
   the levels in the graph.  Assume the order of column 0 is
   fixed.  Then determine the best posttions for column 1 based on this,
   then column 2, etc.  The primary inputs are fixed also.  The go back
   towards the primary outputs, using the orders of both the previous and
   next columns.
\* -------------------------------------------------------------------------- */

#define dw(n)    ((((np_node*)n2)->dummy) ? 0 : 0)

static int fits (map,size,place,maxverts)
int* map;
int size;
int place;
int maxverts;
{
    int i;

    if (size == 0) return (1);

    if (place + size / 2 >= maxverts) return(0);
    if (place - size / 2 < 0) return(0);

    for (i = 0; i < size; i++)
        if (map[place - size / 2 + i])
            return (0);
    
    return (1);
}

static int findPlace (fixed,size,avg,maxverts)
int* fixed;
int size;
int avg;
int maxverts;
{
    int i;

    if (fits(fixed, size, avg, maxverts))
        return (avg);
    else {
        for (i = 1; i < maxverts; i++) {
            if ((avg + i < maxverts) &&
                    fits(fixed, size, avg + i, maxverts))
                return (avg + i);
            if ((avg - i >= 0) &&
                    fits(fixed, size, avg - i, maxverts))
                return (avg - i);
        }
    }
    return (-1);
}

static int nodeCompareFanouts (item1,item2)
lsGeneric item1;
lsGeneric item2;
{
    np_node *n1 = (np_node*) item1;
    np_node *n2 = (np_node*) item2;

    return ((num_fanouts(n2) + n2->grids_high + dw(n2)) -
        (num_fanouts(n1) + n1->grids_high + dw(n1)));
}

static int nodeCompareBoth (item1,item2)
lsGeneric item1;
lsGeneric item2;
{
    np_node *n1 = (np_node*) item1;
    np_node *n2 = (np_node*) item2;

    return ((num_fanouts(n2) + num_fanins(n2) +
            n2->grids_high + dw(n2)) -
        (num_fanouts(n1) + num_fanins(n1) +
            n1->grids_high + dw(n1)));
}

static int
np_place_nodes (graph,level,maxverts,prevonly)
NetPlotPart* graph;
int level;
int maxverts;
Bool prevonly;
{
    lsList lev;
    lsGen gen, gen2;
    np_node *node, *fanin, *fanout, *node2;
    np_arc *arc;
    int *fixed;
    int i, j, num, avg, try, size;

    /* Fixed is 0 if there is nothing here, 1 if there are only
       dummy nodes here, and 2 if there is a real node here.  */
    fixed = xalloc(int,maxverts);
    for (i=0; i < maxverts; i++) fixed[i] = 0;

    lev = lsCreate();

    np_foreach_node(graph, gen, node) {
        if (node->level == level) {
            lsNewEnd(lev, (lsGeneric) node, (lsHandle *) NULL);
	}
    }

    if (prevonly) {
        lsSort(lev, nodeCompareFanouts);
    } else {
        lsSort(lev, nodeCompareBoth);
    }
    
    np_lsForeachItem (lev, gen, node) {
        avg = 0;
        num = 0;
        np_foreach_fanout(node, gen2, arc) {
            fanout = arc->to;
	    if (node->level > fanout->level) {
		avg += fanout->vertpos;
		num++;
	    }
        }
        if (!prevonly) {
            np_foreach_fanin(node, gen2, arc) {
                fanin = arc->from;
		if (fanin->level > node->level) {
		    avg += fanin->vertpos * 2;
		    num += 2;
		}
            }
        }
        if (!num)
            avg = 0;
        else
            avg /= num;

        if (node->dummy) {
            size = node->grids_high;
            if (!fits(fixed, size, avg, maxverts)) size = 0;
            try = avg;
        } else {
            for (size=node->grids_high; size >= 0; size-=(size>1)?2:1) {
                try = findPlace (fixed, size, avg, maxverts);
                if (try >= 0) break;
            }
            assert(try >= 0);
            if ((try == 0) && (node->grids_high > 1)) {
                try += node->grids_high / 2;
	    }
        }

        if (size < node->grids_high) {
            /* Make room... */
            i = node->grids_high - size;
            maxverts += i;
            fixed = xrealloc(int, fixed, maxverts);
	    for (j=maxverts-i; j < maxverts; j++)
		fixed[j] = 0;
            for (j=maxverts-1; j > try + i; j--)
                fixed[j] = fixed[j - i];
            np_lsForeachItem (lev, gen2, node2) {
                if (node2 == node) {
                    lsFinish(gen2);
                    break;
                }
                if (node2->vertpos > try) {
                    node2->vertpos += i;
		}
            }
        }

        for (i=0; i < node->grids_high; i++) {
            assert(try - node->grids_high / 2 + i >= 0);
            assert(try - node->grids_high / 2 + i < maxverts);
            fixed[try - node->grids_high / 2 + i] = 2;
        }

        node->vertpos = try + node->grids_high / 2;
    }

    lsDestroy(lev, (void (*)()) NULL);
    xfree(fixed);

    return maxverts;
}

static void np_generic_place (graph)
NetPlotPart* graph;
{
    /*	Run a node placement algorithm based on levelizing the graph and
	assigning nodes to a grid position.  Output is x,y set for every
	node in the graph. */

    int level, i;
    lsGen gen;
    np_node *node;
    int maxverts = 0;
    int maxlevel = 0;

    np_make_uniform (graph);

    np_foreach_node(graph, gen, node) {
	if (node->level > maxlevel) maxlevel = node->level;
	/* These are "magic" numbers from the old plot code. */
	node->grids_high = (node->shape == np_ellipse) ? 3 : 2;
	if (node->dummy) node->grids_high = 0;
    }

    for (level=maxlevel; level >= 0; level--) {
	i = 0;
	np_foreach_node (graph,gen,node) {
	    if (node->level == level) i += node->grids_high;
	}
	if (i > maxverts) maxverts = i;
    }

    /* Go from the outputs to the inputs. */
    maxverts = np_place_nodes(graph, 0, maxverts, True);
    for (i=1; i < maxlevel; i++) {
	maxverts = np_place_nodes (graph, i, maxverts, True);
    }
    maxverts = np_place_nodes (graph, maxlevel, maxverts, True);

    /* Go from the inputs to the outputs. */
    for (i=maxlevel-1; i > 0; i--) {
	maxverts = np_place_nodes (graph, i, maxverts, False);
    }
    maxverts = np_place_nodes (graph, 0, maxverts, False);

    np_foreach_node (graph,gen,node) {
	node->x = (float) -node->level;
	node->y = (float) node->vertpos;
	/* On even levels, shift real nodes, odd levels shift dummy nodes. */
/*
	if ((node->level%2 == 0) ^ (node->dummy == False)) {
	    node->y += 0.5;
	}
 */
    }
}

/* ------------------------ Map Grid to User -------------------------------- *\

   After a generic placement algorithm on an arbitrary grid, convert the
   grid into user coordinates, and set the bounds rect for each node.
\* -------------------------------------------------------------------------- */

void np_set_position (node,x,y)
np_node* node;
double x;
double y;
{
    /*	Set the position of one node.  If one node is placed, then no
	automatic placement is done for any nodes. */

    node->widget->netPlot.unplaced = False;
    node->x = x;  node->y = y;
}

void np_autoplace (w)
Widget w;
{
    /*	Set up transformation between user and window coordinates, and init.
	the bounding box for all nodes.  If the graph is unplaced, a generic
	placement algorithm is run first. */

    NetPlotWidget widget = (NetPlotWidget) w;
    NetPlotPart *xd = &widget->netPlot;
    lsGen gen;
    np_node *node;
    float xmin,xmax, ymin,ymax;
    XRectangle r, user_frame;
    Boolean first;

#ifdef WIDGET_DEBUG
    printf("autoplace size %d,%d\n",widget->core.width,widget->core.height);
#endif

    np_foreach_node (xd, gen, node) {
	node->bounds.width  = (*node->shape) (node,NP_CALC_WIDTH);
	node->bounds.height = (*node->shape) (node,NP_CALC_HEIGHT);
	if (!node->dummy) {
	    node->bounds.width  += X_PAD;
	    node->bounds.height += Y_PAD;
	}
    }

    if (xd->unplaced) {		/* Assign some x,y coordinates. */
	np_generic_place (xd);
	xd->unplaced = False;
    }

    /* Find min/max of user coordinate centers, and of widget window. */

    first = True;
    xmin = ymin = 0.0;
    xmax = ymax = 1.0;
    np_foreach_node (xd,gen,node) {
	if (first) {
	    xmax = xmin = node->x;
	    ymax = ymin = node->y;
	    first = False;
	} else {
	    if (node->x > xmax) xmax = node->x;
	    if (node->x < xmin) xmin = node->x;
	    if (node->y > ymax) ymax = node->y;
	    if (node->y < ymin) ymin = node->y;
	}
    }

    xd->xt_user = xmin;
    xd->yt_user = ymin;
    xd->x_scale = (float) widget->core.width  / (xmax-xmin);
    xd->y_scale = (float) widget->core.height / (ymax-ymin);

    /* Use this temp. scale factor to calculate the amount not visible. */

    np_clear_rect (&user_frame);
    np_foreach_node (xd,gen,node) {
	r.width  = node->bounds.width;
	r.height = node->bounds.height;
	r.x = ceil((node->x - xmin) * xd->x_scale) - r.width/2;
	r.y = ceil((node->y - ymin) * xd->y_scale) - r.height/2;
	np_wrap_rect (&user_frame, &user_frame,&r);
    }

    /* Allow a border around the plot. */
#define BMM	10

    user_frame.x -= ceil(BMM * xd->x_pix_per_mm);
    user_frame.y -= ceil(BMM * xd->y_pix_per_mm);
    user_frame.width  += ceil(2*BMM * xd->x_pix_per_mm);
    user_frame.height += ceil(2*BMM * xd->y_pix_per_mm);

    /* Adjust the origin and scale factor approximately. */

    xd->x_scale -= (user_frame.width  - widget->core.width ) / (xmax - xmin);
    xd->y_scale -= (user_frame.height - widget->core.height) / (ymax - ymin);
    xd->xt_user += user_frame.x / xd->x_scale;
    xd->yt_user += user_frame.y / xd->y_scale;

    /* Calculate final bounding box location for every node. */

    np_foreach_node (xd, gen, node) {
	node->bounds.x = x_trans(xd,node->x) - node->bounds.width/2;
	node->bounds.y = y_trans(xd,node->y) - node->bounds.height/2;
    }
}

/* --------------------------- Low level X Stuff ---------------------------- */

static void np_top_center (node,tc)
np_node* node;
XPoint* tc;
{
    tc->x = node->bounds.x + node->bounds.width/2;
    tc->y = node->bounds.y;
}

static void np_bottom_center (node,tc)
np_node* node;
XPoint* tc;
{
    tc->x = node->bounds.x + node->bounds.width/2;
    tc->y = node->bounds.y + node->bounds.height;
}

static void np_left_center (node,tc)
np_node* node;
XPoint* tc;
{
    tc->x = node->bounds.x;
    tc->y = node->bounds.y + node->bounds.height/2;
}

static void np_right_center (node,tc)
np_node* node;
XPoint* tc;
{
    tc->x = node->bounds.x + node->bounds.width;
    tc->y = node->bounds.y + node->bounds.height/2;
}

static void
np_arrow_comp (xd,ends,arrangle,arrlength)
NetPlotPart* xd;
XPoint* ends;
double arrangle;
int arrlength;
{
    /*	Draw arrowhead on line from ends[0] to ends[1].  Changes of sign on
	some y calculations because X11 positive y values go down, but trig.
	functions expect positive y values mean up. */

    XPoint verts[3];
    double angle;
    int dx, dy, iangle;

    dy = ends[0].y - ends[1].y;
    dx = ends[0].x - ends[1].x;
    if (ABS(dx) < arrlength && ABS(dy) < arrlength) return;
    angle = (dx == 0 && dy == 0) ? 0.0 : atan2((double)-dy,(double)dx);

    if (xd->vertical) {
	/* Convert to degrees and draw arc. */
	iangle = ceil(angle * 180. * 64. / 3.1415926);
        XFillArc (xd->dpy,xd->win,xd->gc,
		    ends[1].x-arrlength,ends[1].y-arrlength,
		    2*arrlength,2*arrlength,
		    iangle-18*64,(18+18)*64);
    } else {
	verts[0].x = ends[1].x + ceil(arrlength * cos(angle - arrangle));
	verts[0].y = ends[1].y - ceil(arrlength * sin(angle - arrangle));
	verts[1]   = ends[1];
	verts[2].x = ends[1].x + ceil(arrlength * cos(angle + arrangle));
	verts[2].y = ends[1].y - ceil(arrlength * sin(angle + arrangle));
	XDrawLines (xd->dpy,xd->win,xd->gc, verts,3, CoordModeOrigin);
    }
}

static void np_calc_arc (arc,verts,covers)
np_arc* arc;
XPoint* verts;
XRectangle* covers;
{
    /*	Compute the endpoints and covering rectangle for the arc. */

    np_node *from = arc->from;
    np_node *to   = arc->to;

    if (arc->widget->netPlot.vertical) {
	if (from->bounds.y + from->bounds.height < to->bounds.y) {
	    np_bottom_center (from,&verts[0]);
	    np_top_center (to,&verts[1]);
	} else if (from->bounds.y > to->bounds.y + to->bounds.height) {
	    np_top_center (from,&verts[0]);
	    np_bottom_center (to,&verts[1]);
	} else if (from->bounds.x > to->bounds.x + to->bounds.width) {
	    np_left_center (from,&verts[0]);
	    np_right_center (to,&verts[1]);
	} else {
	    np_right_center (from,&verts[0]);
	    np_left_center (to,&verts[1]);
	}
    } else {
	if (from->bounds.x > to->bounds.x + to->bounds.width) {
	    np_left_center (from,&verts[0]);
	    np_right_center (to,&verts[1]);
	} else if (from->bounds.x + from->bounds.width < to->bounds.x) {
	    np_right_center (from,&verts[0]);
	    np_left_center (to,&verts[1]);
	} else if (from->bounds.y + from->bounds.height < to->bounds.y) {
	    np_bottom_center (from,&verts[0]);
	    np_top_center (to,&verts[1]);
	} else {
	    np_top_center (from,&verts[0]);
	    np_bottom_center (to,&verts[1]);
	}
    }

    covers->x = MIN(verts[0].x,verts[1].x);
    covers->y = MIN(verts[0].y,verts[1].y);
    covers->width  = ABS(verts[1].x-verts[0].x);
    covers->height = ABS(verts[1].y-verts[0].y);
}

static void np_sect_arc (r,arc)
XRectangle* r;
np_arc* arc;
{
    /*	Add the arc's rectangle to the bounding box. */

    XPoint verts[2];
    XRectangle covers;
    np_calc_arc (arc,verts,&covers);
    np_wrap_rect (r, r,&covers);
}

static void np_draw_an_arc (xd,area,arc)
NetPlotPart* xd;
XRectangle* area;
np_arc* arc;
{
    /*	Draw one segment of an arc, including its label.  We need to do
	bounds checking on the area for the label yet.  Arc is drawn based
	on the bounds for the from/to nodes. */

    XPoint verts[2];
    XRectangle covers;
    int len;

    np_calc_arc (arc,verts,&covers);

    if (area == NULL || np_sect_rect(area,&covers)) {
	XDrawLines (xd->dpy,xd->win,xd->gc, verts,2, CoordModeOrigin);
	if (!arc->to->dummy) {
	    np_arrow_comp (xd, verts, ARROW_ANGLE, ARROW_LENGTH);
	}
    }

    if (arc->label != NULL) {
	/* Find center of arc. */
	int cx = (verts[0].x + verts[1].x) / 2;
	int cy = (verts[0].y + verts[1].y) / 2;
	/* Center the label on the arc. */
	len = strlen(arc->label);
	cx -= XTextWidth(xd->font,arc->label,len) / 2;
	cy += xd->font_height/2 - 6;
	XDrawImageString (xd->dpy,xd->win,xd->gc, cx,cy, arc->label,len);
    }
}

static void np_draw_arc (win,area,arc)
NetPlotPart* win;
XRectangle* area;
np_arc* arc;
{
    if (arc->from->dummy) return;

    np_draw_an_arc (win,area, arc);

    while (arc->to->dummy) {
	assert(num_fanouts(arc->to) == 1);
	lsFirstItem (arc->to->fanouts, (lsGeneric *) &arc, LS_NH);
	np_draw_an_arc (win,area, arc);
    }
}

static np_node *np_real_fanout (arc)
np_arc *arc;
{
    while (arc->to->dummy) {
	assert(num_fanouts(arc->to) == 1);
	lsFirstItem (arc->to->fanouts, (lsGeneric *) &arc, LS_NH);
    }
    return arc->to;
}

static np_node *np_real_fanin (arc)
np_arc *arc;
{
    while (arc->from->dummy) {
	assert(num_fanins(arc->from) == 1);
	lsFirstItem (arc->from->fanins, (lsGeneric *) &arc, LS_NH);
    }
    return arc->from;
}

/* -------------------------- Highlighting ---------------------------------- */

static void np_do_shape (node,function,action)
np_node* node;
int function;
np_action action;
{
    NetPlotPart *xd = &(node->widget->netPlot);
    if (xd->exposed) {
        XSetForeground (xd->dpy,xd->gc, xd->overlay);
        XSetFunction   (xd->dpy,xd->gc, function);
        (*node->shape) (node,action);
        XSetFunction   (xd->dpy,xd->gc, GXcopy);
    }
}

void np_overlay (node)
np_node* node;
{
    /*	If the node is being highlighted, set function to OR so that the
	overlay pixels are turned on.  Otherwise, set function to AND-INV
	to clear just the overlay plane to remove any and all highlighting. */

    np_do_shape (node,
		 (node->highlighted) ? GXor : GXandInverted,
		 (node->highlighted==1) ? NP_FRAME : NP_DRAW);
}

void np_lowlight (node)
np_node *node;
{
    if (node->highlighted) {
	node->highlighted = 0;
	np_overlay (node);
    }
}

void np_highlight (node,what)
np_node *node;
Bool what;
{
    if (node->highlighted != what) {
	node->highlighted = what;
	np_overlay (node);
    }
}

void np_highlight_node (node,on)
np_node *node;
Bool on;
{
    np_highlight (node,on?2:0);
}

void np_overlay_arc (arc,area)
np_arc* arc;
XRectangle* area;
{
    NetPlotPart *xd = &(arc->from->widget->netPlot);
    if (xd->exposed) {
	XSetForeground (xd->dpy,xd->gc, xd->overlay);
	XSetFunction   (xd->dpy,xd->gc, arc->highlighted?GXor:GXandInverted);
	np_draw_an_arc (xd,area,arc);
	XSetFunction   (xd->dpy,xd->gc, GXcopy);
    }
}

void np_highlight_arc (arc,on)
np_arc* arc;
Bool on;
{
    /*	Set the arc highlighting to the boolean value. */

    if (arc->highlighted != on) {
	arc->highlighted = on;
	np_overlay_arc (arc,NULL);
    }
}

void np_new_highlight (w,label)
Widget w;
String label;
{
    NetPlotWidget widget = (NetPlotWidget) w;
    NetPlotPart *graph = &widget->netPlot;
    lsGen gen;
    np_node *node = NULL;
    np_arc *arc;
    NetPlotSelectData sel_data;

    if (graph->sel_label != NULL) xfree (graph->sel_label);
    if (label == NULL) label = graph->label;
    graph->sel_label = XtNewString (label);
    sel_data.select_name = label;
    XtCallCallbackList (w,graph->select_callbacks,(XtPointer)&sel_data);

    np_foreach_node (graph, gen, node) {
	np_lowlight (node);
    }
    np_foreach_arc (graph, gen, arc) {
	np_highlight_arc (arc,False);
    }
}

void np_set_title (w,s)
Widget w;
char *s;
{
    /*	Change title of widget. */

    NetPlotWidget widget = (NetPlotWidget) w;
    NetPlotPart *graph = &widget->netPlot;

    if (graph->label != NULL) xfree (graph->label);
    graph->label = XtNewString (s);
}

void np_orient (w,vert)
Widget w;
int vert;
{
    NetPlotWidget widget = (NetPlotWidget) w;
    NetPlotPart *graph = &widget->netPlot;

    graph->vertical = (vert != 0);
}

void np_clear_plot (w)
Widget w;
{
    /*  Deallocate any instance memory. */

    NetPlotWidget widget = (NetPlotWidget) w;
    NetPlotPart *graph = &widget->netPlot;

#ifdef WIDGET_DEBUG
    fprintf(stderr,"clear plot for 0x%lX\n",widget);
#endif
    lsDestroy (graph->nodes, np_free_node);
    assert(lsLength(graph->arcs) == 0);
    lsDestroy (graph->arcs, (void (*)()) NULL);
    st_free_table (graph->node_table);

    /* Now reinitialize the graph. */
    graph->nodes = lsCreate();
    graph->arcs = lsCreate();
    graph->node_table = st_init_table (strcmp,st_strhash);
    widget->netPlot.unplaced = True;
    widget->netPlot.unscaled = True;
    if (widget->netPlot.exposed) {
	/* Force the widget to be redrawn. */
	XClearArea (graph->dpy,graph->win,0,0,0,0,True);
    }
}

void np_redraw (w,area,clear)
Widget w;
XRectangle* area;
int clear;
{
    NetPlotWidget widget = (NetPlotWidget) w;
    NetPlotPart *graph = &widget->netPlot;
    lsGen gen;
    np_arc *arc;
    np_node *node;

    if (!graph->exposed) return;

    if (graph->unscaled) {
#ifdef WIDGET_DEBUG
	fprintf(stderr,"Figuring initial geometry.\n");
#endif
	np_autoplace (w);
	graph->unscaled = False;
    }

    if (clear) XClearWindow (graph->dpy, graph->win);

    XSetForeground (graph->dpy, graph->gc, widget->netPlot.colors[4]);
    np_foreach_arc (graph, gen, arc) {
        np_draw_arc (graph,area,arc);
    }

    XSetForeground (graph->dpy, graph->gc, widget->netPlot.colors[0]);
    np_foreach_node (graph, gen, node) {
	if (area == NULL || np_sect_rect (area,&node->bounds)) {
	    (*node->shape) (node,NP_ERASE);
	}
    }

    np_foreach_node (graph, gen, node) {
	if (area == NULL || np_sect_rect (area,&node->bounds)) {
	    XSetForeground (graph->dpy, graph->gc, widget->netPlot.colors[node->color]);
	    (*node->shape) (node,NP_DRAW);
	}
    }

    np_foreach_node (graph, gen, node) {
	if (node->highlighted && np_sect_rect (area,&node->bounds)) {
	    np_overlay (node);
	}
    }

    np_foreach_arc (graph, gen, arc) {
	if (arc->highlighted) {
	    np_overlay_arc (arc,area);
	}
    }
}

/* ------------------------------- Methods ---------------------------------- */

static void NetPlotInitialize (treq,tnew,arg_list,n_arg)
Widget treq;		/*  ARGSUSED */
Widget tnew;		/*u new instance of NetPlot widget.  */
ArgList arg_list;	/*  ARGSUSED */
Cardinal* n_arg;	/*  ARGSUSED */
{
    /*  Check public fields of widget instance, and initialize private fields.
        Translation is set to (0,0) since no STN is available yet. */

    NetPlotWidget widget = (NetPlotWidget) tnew;
    NetPlotPart *xd = &widget->netPlot;
    unsigned long plane_masks[1], colors[NP_MAX_COLORS];
    float text_scale;
    int i;
    int screen_num;

    xd->exposed = False;
    xd->label = XtNewString("<untitled>");
    xd->sel_label = NULL;
    xd->nodes = lsCreate();
    xd->arcs = lsCreate();
    xd->node_table = st_init_table (strcmp,st_strhash);
    xd->track_on = False;
    xd->vertical = False;


#ifdef WIDGET_DEBUG
    fprintf(stderr,"Initialize for 0x%lX\n",widget);
#endif

    xd->dpy = XtDisplay(widget);
    screen_num = XScreenNumberOfScreen (XtScreen(widget));
    xd->gc  = XCreateGC (xd->dpy,RootWindowOfScreen(XtScreen(widget)),0,NULL);

    /*	Calculate pixels/mm in each direction. */
    xd->x_pix_per_mm = (float)DisplayWidth(xd->dpy,screen_num) /
		       (float)DisplayWidthMM(xd->dpy,screen_num);
    xd->y_pix_per_mm = (float)DisplayHeight(xd->dpy,screen_num) /
		       (float)DisplayHeightMM(xd->dpy,screen_num);

    xd->font_height = xd->font->max_bounds.ascent
		    + xd->font->max_bounds.descent;

    /* Do we really need to set these? */
    text_scale = (float)xd->font_height;
    xd->y_scale = text_scale;
    xd->x_scale = (text_scale * xd->x_pix_per_mm) / xd->y_pix_per_mm;
    xd->xt_user = xd->yt_user = 0.0;

#ifdef WIDGET_DEBUG
    printf("screen: (%.1f,%.1f) pix/mm\n",xd->x_pix_per_mm,xd->y_pix_per_mm);
    printf("text:   %.2f pix/unit\n",text_scale);
    printf("view:   (%.2f,%.2f) pix/unit\n",xd->x_scale, xd->y_scale);
    printf("        (%.1f,%.1f) units xlate\n",xd->xt_user, xd->yt_user);
#endif

    if (widget->core.depth > 1 &&
	XAllocColorCells (xd->dpy,widget->core.colormap,False,
			    plane_masks,1, colors,NP_MAX_COLORS) != 0) {
	/* Get RGB values for widget fore/back/highlight colors. */
	XColor color_defs[2*NP_MAX_COLORS];
	for (i=0; i < NP_MAX_COLORS; i++) {
	    color_defs[i].pixel = widget->netPlot.colors[i];
	    color_defs[i+NP_MAX_COLORS].pixel = widget->netPlot.highlight;
	}
	XQueryColors (xd->dpy,widget->core.colormap,color_defs,2*NP_MAX_COLORS);
	/* Store these RGB values in our writable color cells. */
	for (i=0; i < NP_MAX_COLORS; i++) {
	    color_defs[i].pixel = colors[i];
	    widget->netPlot.colors[i] = colors[i];
	    color_defs[i+NP_MAX_COLORS].pixel = colors[i] | plane_masks[0];
	}
	XStoreColors (xd->dpy,widget->core.colormap,color_defs,2*NP_MAX_COLORS);
	/* Can XOR plane mask to toggle hilighting. */
	widget->netPlot.overlay = plane_masks[0];
    } else {
	fprintf(stderr,"Couldn't allocate color cells.\n");
    }

    XSetFont (xd->dpy,xd->gc, widget->netPlot.font->fid);
    XSetForeground (xd->dpy,xd->gc, widget->netPlot.colors[1]);
    XSetBackground (xd->dpy,xd->gc,widget->netPlot.colors[0]);
    XSetLineAttributes (xd->dpy,xd->gc, xd->linewidth,
				LineSolid, CapButt, JoinMiter);

    widget->core.width = 400;
    widget->core.height = 360;
    widget->netPlot.unplaced = True;
    widget->netPlot.unscaled = True;
}

static void NetPlotExpose (w,event,uprgn)
Widget w;
XEvent* event;
Region uprgn;
{
    /*	Handle expose events: set clip region to the rectangle which was
	exposed, redraw the plot, then restore the full clip region. */

    /* ARGSUSED */
    NetPlotWidget widget = (NetPlotWidget) w;
    NetPlotPart *graph = &widget->netPlot;
    XExposeEvent *expose = &(event->xexpose);
    XRectangle area;

#ifdef WIDGET_DEBUG
    fprintf(stderr,"Expose %d for 0x%lX (%d,%d),(%d,%d)\n",
	expose->count,widget,expose->x,expose->y,expose->width,expose->height);
#endif
    graph->win = widget->core.window;
    if (!graph->exposed) {
	XSetWindowBackground (graph->dpy, widget->core.window,
					widget->netPlot.colors[0]);
	XClearWindow (graph->dpy, widget->core.window);
	graph->exposed = True;
    }

    area.x = expose->x;		area.y = expose->y;
    area.width = expose->width;	area.height = expose->height;

    XSetClipRectangles (graph->dpy,graph->gc, 0,0, &area,1, YXBanded);
    np_redraw (w,&area,False);
    XSetClipMask (graph->dpy,graph->gc,None);
}

static void NetPlotDestroy (w)
Widget w;
{
    /*  Deallocate any instance memory. */

    NetPlotWidget widget = (NetPlotWidget) w;
    NetPlotPart *graph = &widget->netPlot;

#ifdef WIDGET_DEBUG
    fprintf(stderr,"Destroy for 0x%lX\n",widget);
#endif
    XFreeGC (graph->dpy,graph->gc);
    lsDestroy (graph->nodes, np_free_node);
    assert(lsLength(graph->arcs) == 0);
    lsDestroy (graph->arcs, (void (*)()) NULL);
    st_free_table (graph->node_table);
}

static void NetPlotResize (w)
Widget w;
{
    /*  Called when widget size is changed. */

#ifdef WIDGET_DEBUG
    fprintf(stderr,"Resize for 0x%lX.\n",w);
#endif
    np_autoplace (w);
}

static XtGeometryResult NetPlotQueryGeometry (w, proposed, reply)
Widget w;
XtWidgetGeometry *proposed, *reply;
{
    /*  Decide what a suitable geometry is. */

#ifdef WIDGET_DEBUG
    fprintf(stderr,"Query Geometry for 0x%lX.\n",w);
#endif
    reply->request_mode = CWWidth | CWHeight;

    reply->width  = 400;
    reply->height = 360;

    if (((proposed->request_mode & (CWWidth | CWHeight)) ==
		(CWWidth | CWHeight)) &&
	 proposed->width  == reply->width &&
	 proposed->height == reply->height) {
	return XtGeometryYes;
    }

    return XtGeometryAlmost;
}

/* ------------------------------- Actions ---------------------------------- */

static np_node *hit_test (graph,bx,by)
NetPlotPart* graph;
int bx;
int by;
{
    /*	Return node which was hit by point (bx,by), or NULL if none.  Only
	check for hitting a dummy node if no regular node was hit. */

    XRectangle hit;
    lsGen gen;
    np_node *node, *best = NULL;
    int dy, dx;

    hit.x = bx;  hit.y = by;
    hit.height = hit.width = 1;

    np_foreach_node (graph, gen, node) {
	if (node->dummy) continue;
	if (np_sect_rect (&hit,&node->bounds)) {
	    if (node->highlighted || best == NULL || !best->highlighted) {
		best = node;
	    }
	}
    }

    if (best == NULL) {
	np_foreach_node (graph, gen, node) {
	    if (!node->dummy) continue;
	    dx = bx - node->bounds.x;
	    dy = by - node->bounds.y;
	    if (ABS(dx) < 10 && ABS(dy) < 10) {
		best = node;
	    }
	}
    }

    return best;
}

static void show_sel (w,node,param)
Widget w;
np_node *node;
String param;
{
    np_arc *arc;
    lsGen gen;

    np_new_highlight (w,node->name);
    np_highlight (node,2);

    if (param != NULL) {

	if (!strcmp(param,"in") || !strcmp(param,"inout")) {
	    np_foreach_fanin  (node, gen, arc) {
		np_highlight (np_real_fanin(arc),1);
	    }
	}

	if (!strcmp(param,"out") || !strcmp(param,"inout")) {
	    np_foreach_fanout (node, gen, arc) {
		np_highlight (np_real_fanout(arc),1);
	    }
	}
    }
}

static void TrackNode (w,event,params,n_param)
Widget w;
XEvent* event;
String* params;		/*  ARGSUSED */
Cardinal* n_param;	/*  ARGSUSED */
{
    /* Try to track the mouse down. */

    NetPlotWidget widget = (NetPlotWidget) w;
    NetPlotPart *graph = &widget->netPlot;
    np_node *node = graph->selected;
    int dx = event->xbutton.x - graph->hot_spot.x;
    int dy = event->xbutton.y - graph->hot_spot.y;

    if (node == NULL) return;

    if (!graph->track_on) {	/* Clear general highlighting. */
	np_new_highlight (w,node->name);
    }

    /* Highlight new position using xor. */
    node->bounds.x += dx;
    node->bounds.y += dy;
    np_do_shape (node, GXxor, NP_FRAME);
    node->bounds.x -= dx;
    node->bounds.y -= dy;

    /* Erase old position if there was one. */
    if (graph->track_on) {
	node->bounds.x += graph->cur_diff.x;
	node->bounds.y += graph->cur_diff.y;
	np_do_shape (node, GXxor, NP_FRAME);
	node->bounds.x -= graph->cur_diff.x;
	node->bounds.y -= graph->cur_diff.y;
    }

    /* Now remember tracking is on, and save new delta. */
    graph->track_on = True;
    graph->cur_diff.x = dx;
    graph->cur_diff.y = dy;
}

static void MoveNode (w,event,params,n_param)
Widget w;
XEvent* event;
String* params;		/*  ARGSUSED */
Cardinal* n_param;	/*  ARGSUSED */
{
    /* Finished moving something. */

    NetPlotWidget widget = (NetPlotWidget) w;
    NetPlotPart *graph = &widget->netPlot;
    np_node *node = graph->selected;
    int dx = event->xbutton.x - graph->hot_spot.x;
    int dy = event->xbutton.y - graph->hot_spot.y;
    lsGen gen;
    np_arc *arc;
    XRectangle covers;

    if (graph->track_on) {
	graph->selected->bounds.x += graph->cur_diff.x;
	graph->selected->bounds.y += graph->cur_diff.y;
	np_do_shape (graph->selected, GXxor, NP_FRAME);
	graph->selected->bounds.x -= graph->cur_diff.x;
	graph->selected->bounds.y -= graph->cur_diff.y;
	graph->track_on = False;
    }

    if (node != NULL && (dx != 0 || dy != 0)) {
	/* Expose area where node used to be. */
	covers = node->bounds;
	np_foreach_fanin (node,gen,arc) np_sect_arc (&covers,arc);
	np_foreach_fanout (node,gen,arc) np_sect_arc (&covers,arc);

	/* Move the node (and arcs) to their new position. */
	node->bounds.x += dx;  node->bounds.y += dy;
	node->x = winToUserX (graph,node->bounds.x + node->bounds.width/2);
	node->y = winToUserY (graph,node->bounds.y + node->bounds.height/2);

	/* Expose area where node is now. */
	np_wrap_rect (&covers, &covers, &node->bounds);
	np_foreach_fanin (node,gen,arc) np_sect_arc (&covers,arc);
	np_foreach_fanout (node,gen,arc) np_sect_arc (&covers,arc);

	/* Clear entire rect, forcing it to be redrawn. */
	XClearArea (graph->dpy,graph->win,
		covers.x-ARROW_LENGTH, covers.y-ARROW_LENGTH,
		covers.width+2*ARROW_LENGTH,covers.height+2*ARROW_LENGTH, True);
    }
}

static void SelectNode (w,event,params,n_param)
Widget w;
XEvent* event;
String* params;		/*  ARGSUSED */
Cardinal* n_param;	/*  ARGSUSED */
{
    /*  Select or unselect the item closest to the mouse coordinates. */

    NetPlotWidget widget = (NetPlotWidget) w;
    NetPlotPart *graph = &widget->netPlot;
    int bx = event->xbutton.x, by = event->xbutton.y;
    np_node *node;

    graph->selected = NULL;

    if (event->type == ButtonPress) {

	node = hit_test (graph, bx,by);

	if (node == NULL) {
	    np_new_highlight (w,graph->label);
	} else {
	    show_sel (w,node,(*n_param==0)?NULL:params[0]);
	    graph->selected = node;
	    graph->hot_spot.x = bx;  graph->hot_spot.y = by;
	}
    }
}

static void DoNodeCmd (w,event,params,n_param)
Widget w;
XEvent* event;
String* params;		/*i [0]=for node, [1]=otherwise.	*/
Cardinal* n_param;	/*i Note that all params are optional.	*/
{
    /*	Execute a sis node command on the node nearest to the cursor. */

    NetPlotWidget widget = (NetPlotWidget) w;
    NetPlotPart *graph = &widget->netPlot;
    int bx = event->xbutton.x, by = event->xbutton.y;
    char print_cmd[80];
    np_node *node;
    extern void xsis_execute ();

    node = hit_test (graph, bx,by);

    if (node != NULL && *n_param >= 1) {
	sprintf (print_cmd,params[0],np_name(node));
	xsis_execute (print_cmd,False);
    } else if (node == NULL && *n_param >= 2) {
	xsis_execute (params[1],False);
    }
}

/* ----------------------------- Resource List ------------------------------ */

static XtResource resources[] = {
#define offset(field) XtOffset(NetPlotWidget, netPlot.field)
    /* {name, class, type, size, offset, default_type, default_addr}, */
    { XtNbackground, XtCBackground, XtRPixel,
      sizeof(Pixel),offset(colors[0]), XtRString, XtDefaultBackground },
    { XtNforeground1, XtCForeground, XtRPixel,
      sizeof(Pixel),offset(colors[1]), XtRString, XtDefaultForeground },
    { XtNforeground2, XtCForeground, XtRPixel,
      sizeof(Pixel),offset(colors[2]), XtRString, XtDefaultForeground },
    { XtNforeground3, XtCForeground, XtRPixel,
      sizeof(Pixel),offset(colors[3]), XtRString, XtDefaultForeground },
    { XtNforeground4, XtCForeground, XtRPixel,
      sizeof(Pixel),offset(colors[4]), XtRString, XtDefaultForeground },
    { XtNhighlight,   XtCForeground, XtRPixel,
      sizeof(Pixel),offset(highlight), XtRString, XtDefaultForeground },
    { XtNfont, XtCFont, XtRFontStruct,
      sizeof(Pixel),offset(font), XtRString, "times_bold14" },
    { XtNselectCallback, XtCCallback, XtRCallback, sizeof(XtPointer),
      offset(select_callbacks), XtRCallback, (XtPointer)NULL },
    { XtNlineWidth, XtCWidth, XtRInt,
      sizeof(int),offset(linewidth), XtRString, (XtPointer)"0" }
#undef offset
};

/* ------------------------------ Actions Table ----------------------------- */

static XtActionsRec actions[] =
{
 /* { name,		function}, */
    {"SelectNode",	SelectNode},
    {"TrackNode",	TrackNode},
    {"MoveNode",	MoveNode},
    {"DoNodeCmd",	DoNodeCmd},
};

/* -------------------------- Default Translation Table --------------------- */

static char translations[] = "\
<Btn1Down>:		SelectNode(inout)	\n\
<Btn2Down>:		DoNodeCmd(\"print %s\",print_stats)	\n\
<Btn3Down>:		SelectNode()	\n\
<Btn3Motion>:		TrackNode()	\n\
<Btn3Up>:		MoveNode()	\n\
";

/* ------------------------ Class Record Initialization --------------------- */

static NetPlotClassRec netPlotClassRec = {
  { /* core fields */
    /* superclass		*/	(WidgetClass) &widgetClassRec,
    /* class_name		*/	"NetPlot",
    /* widget_size		*/	sizeof(NetPlotRec),
    /* class_initialize		*/	NULL,
    /* class_part_initialize	*/	NULL,
    /* class_inited		*/	FALSE,
    /* initialize		*/	NetPlotInitialize,
    /* initialize_hook		*/	NULL,
    /* realize			*/	XtInheritRealize,
    /* actions			*/	actions,
    /* num_actions		*/	XtNumber(actions),
    /* resources		*/	resources,
    /* num_resources		*/	XtNumber(resources),
    /* xrm_class		*/	NULLQUARK,
    /* compress_motion		*/	TRUE,
    /* compress_exposure	*/	TRUE,
    /* compress_enterleave	*/	TRUE,
    /* visible_interest		*/	FALSE,
    /* destroy			*/	NetPlotDestroy,
    /* resize			*/	NetPlotResize,	/* or NULL */
    /* expose			*/	NetPlotExpose,
    /* set_values		*/	NULL,
    /* set_values_hook		*/	NULL,
    /* set_values_almost	*/	XtInheritSetValuesAlmost,
    /* get_values_hook		*/	NULL,
    /* accept_focus		*/	NULL,
    /* version			*/	XtVersion,
    /* callback_private		*/	NULL,
    /* tm_table			*/	translations,
    /* query_geometry		*/	NetPlotQueryGeometry,
    /* display_accelerator	*/	XtInheritDisplayAccelerator,
    /* extension		*/	NULL
  },
  { /* netPlot fields */
    /* empty			*/	0
  }
};


WidgetClass netPlotWidgetClass = (WidgetClass) &netPlotClassRec;
