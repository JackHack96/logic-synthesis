/* -------------------------------------------------------------------------- *\
   xastg.c -- defines astg shell.

	$Revision: 1.1.1.1 $
	$Date: 2004/02/07 10:14:14 $
	$Author: pchong $
	$Source: /users/pchong/CVS/sis/xsis/xastg.c,v $

   Copyright 1991 by the Regents of the University of California.

   All rights reserved.  Permission to use, copy, modify and distribute
   this software is hereby granted, provided that the above copyright
   notice and this permission notice appear in all copies.  This software
   is made available as is, with no warranties.
\* -------------------------------------------------------------------------- */

#include "xsis.h"
#include "NetPlot.h"

#define XSIS_ASTG	"astg"

typedef struct xastg_info {
    Widget	 shell;		/* Top shell for xastg window.	*/
    Widget	 plot;		/* NetPlot widget of astg.	*/
    Widget	 label;		/* Label widget for feedback.	*/
    String	 geom;		/* Initial geometry string.	*/
} xastg_info;


static void stnplot_read (info)
xastg_info* info;
{
    np_node *node, *fanout;
    np_arc *arc;
    char buffer[250];
    float x,y;
    Boolean is_itrans, is_place;

    while (xsis_get_token (buffer,sizeof(buffer)) != EOF) {

        if (!strcmp(buffer,".geometry")) {
            xsis_get_token (buffer,sizeof(buffer));
            info->geom = XtNewString(buffer);
        }
	else if (!strcmp(buffer,".strans")) {
            xsis_get_token(buffer,sizeof(buffer));
	    node = np_find_node (info->plot,buffer,True);
	    if (node == NULL) continue;
            xsis_get_token(buffer,sizeof(buffer));
	    fanout = np_find_node (info->plot,buffer,True);
	    if (fanout == NULL) continue;
            xsis_get_token(buffer,sizeof(buffer));
	    np_create_arc (node,fanout,buffer);
	}
	else if (!strcmp(buffer,".place") || !strcmp(buffer,".state")) {
	    is_place = !strcmp(buffer,".place");
            xsis_get_token(buffer,sizeof(buffer));
	    node = np_find_node (info->plot,buffer,True);
	    if (node == NULL) continue;
	    np_set_shape (node,np_ellipse);
	    if (is_place) np_set_label (node," ");
	    while (xsis_get_token(buffer,sizeof(buffer)) == '\0') {
		fanout = np_find_node (info->plot,buffer,True);
		if (fanout) np_create_arc (node,fanout,NULL);
	    }
	}
	else if (!strcmp(buffer,".itrans") || !strcmp(buffer,".otrans")) {
	    is_itrans = !strcmp(buffer,".itrans");
            xsis_get_token(buffer,sizeof(buffer));
	    node = np_find_node (info->plot,buffer,True);
	    if (node == NULL) continue;
	    np_set_shape (node,(is_itrans)?np_itrans:np_otrans);
            while (xsis_get_token(buffer,sizeof(buffer)) == '\0') {
                fanout = np_find_node (info->plot,buffer,True);
                if (fanout) np_create_arc (node,fanout,NULL);
            }
	}
	else if (!strcmp(buffer,".posn")) {
            xsis_get_token(buffer,sizeof(buffer));
	    node = np_find_node (info->plot,buffer,True);
	    if (node == NULL) continue;
            xsis_get_token(buffer,sizeof(buffer));
	    sscanf(buffer,"%f",&x);
            xsis_get_token(buffer,sizeof(buffer));
	    sscanf(buffer,"%f",&y);
	    np_set_position (node,x,y);
	}
	else if (!strcmp(buffer,".edge")) {
            xsis_get_token(buffer,sizeof(buffer));
	    node = np_find_node (info->plot,buffer,True);
	    if (node == NULL) continue;
            xsis_get_token(buffer,sizeof(buffer));
	    fanout = np_find_node (info->plot,buffer,True);
	    if (fanout == NULL) continue;
	    while (xsis_get_token(buffer,sizeof(buffer)) == '\0') {
		arc = np_find_arc (node,fanout);
		if (arc == NULL) break;
		if (sscanf(buffer,"%f,%f",&x,&y) == 2) {
		    node = np_insert_bend (arc);
		    np_set_position (node,x,y);
		}
	    }
	}

	xsis_eat_line ();
    }
}

static void xastg_hilite (closure)
XtPointer closure;
{
    /*	Read array of nodes to highlight as a path. */

    xastg_info *info = (xastg_info*) closure;
    char *buffer = xsis_world.buffer;
    int buflen = xsis_world.buflen;
    Boolean on = True;
    np_node *node, *fanin;
    np_arc *arc;

    while (xsis_get_token (buffer,buflen) != EOF) {

	if (!strcmp(buffer,".nodes")) {
	    while (xsis_get_token (buffer,buflen) == '\0') {
		if ((node = np_find_node (info->plot,buffer,False)) != NULL) {
		    np_highlight_node (node,on);
		}
	    }
	}
	else if (!strcmp(buffer,".path")) {
	    fanin = NULL;
	    while (xsis_get_token (buffer,buflen) == '\0') {
		node = np_find_node (info->plot,buffer,False);
		if (node == NULL) break;
		np_highlight_node (node,on);
		if (fanin != NULL) {
		    arc = np_find_arc (fanin,node);
		    np_highlight_arc (arc,True);
		}
		fanin = node;
	    }
	}
	else if (!strcmp(buffer,".clear")) {
	    xsis_get_token (buffer,buflen);
	    np_new_highlight (info->plot,buffer);
	}

	xsis_eat_line ();
    }
}

static void free_astg (w, closure, call_data)
Widget w;			/*  ARGSUSED */
XtPointer closure;		/*u xastg_info.		*/
XtPointer call_data;		/*  ARGSUSED */
{
    /*	Called when an xastg window is destroyed. */

    xastg_info *info = (xastg_info*) closure;
    xfree (info->geom);
}

static void change_label (w, closure, call_data)
Widget w;			/*  ARGSUSED */
XtPointer closure;		/*u xastg_info.		*/
XtPointer call_data;		/*i selection data.	*/
{
    /*	Called whenever plot selection changes. */

    xastg_info *info = (xastg_info*) closure;
    NetPlotSelectData *data = (NetPlotSelectData*) call_data;
    XtVaSetValues (info->label,XtNlabel,data->select_name,NULL);
}

static xsis_cmds xastg_cmd_list[] = {
    { "highlight",		xastg_hilite },
    { NULL }
};

void xastg_new (closure)
XtPointer closure;
{
    xsis_info *world = &xsis_world;
    String title = (String) closure;
    xastg_info *info = XtNew (xastg_info);
    Widget paned, btnbox, closeme;

    info->shell = XtVaCreatePopupShell (title,topLevelShellWidgetClass,
			world->app_shell,NULL);

    info->geom = NULL;
    paned  = xvwidget ("astgPane",panedWidgetClass, info->shell, NULL);
    btnbox = xvwidget ("box",boxWidgetClass, paned, NULL);
    closeme = xvwidget ("close",commandWidgetClass, btnbox, NULL);

    info->label = xvwidget ("label",labelWidgetClass, btnbox, NULL);
    XtVaSetValues (info->label,XtNlabel,title,NULL);

    info->plot = xvwidget ("stnPlot", netPlotWidgetClass, paned, NULL);
    np_set_title (info->plot, title);
    np_orient (info->plot, 1);
    stnplot_read (info);

    XtAddCallback (closeme,XtNcallback,xsis_close_window,(XtPointer)info->shell);
    XtAddCallback (info->plot,XtNselectCallback,change_label,(XtPointer)info);
    XtAddCallback (info->shell,XtNdestroyCallback,free_astg,(XtPointer)info);

    if (info->geom) XtVaSetValues (info->shell,XtNgeometry,info->geom,NULL);

    xsis_add_shell (title,info->shell,XSIS_ASTG,xastg_cmd_list,(XtPointer)info);
}
