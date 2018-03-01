/* -------------------------------------------------------------------------- *\
   xblif.c -- Handles "blif" shells.

	$Revision: 1.1.1.1 $
	$Date: 2004/02/07 10:14:14 $
	$Author: pchong $
	$Source: /users/pchong/CVS/sis/xsis/xblif.c,v $

   Copyright 1991 by the Regents of the University of California.

   All rights reserved.  Permission to use, copy, modify and distribute
   this software is hereby granted, provided that the above copyright
   notice and this permission notice appear in all copies.  This software
   is made available as is, with no warranties.
\* -------------------------------------------------------------------------- */

#include "xsis.h"
#include "NetPlot.h"
#include "blif50.px"

#define XSIS_BLIF	"blif"

typedef struct xblif_button {
    Widget command;		/* Command widget.		*/
    String group;		/* Command group.		*/
    String sis_cmd;		/* Associated sis command.	*/
    Boolean use_nodes;		/* Replace %s with selection?	*/
    String help_text;		/* Description of command.	*/
} xblif_button;

typedef struct xblif_info {
    Widget shell;		/* Top shell for xblif window.	*/
    Widget label;		/* Label widget for feedback.	*/
    Widget plot;		/* NetPlot widget of network.	*/
    String geom;		/* Initial geometry string.	*/
    String label_text;		/* To restore after help msgs.	*/
    Widget button_box;		/* Box for extra buttons.	*/
    lsList buttons;		/* List of additional buttons.	*/
} xblif_info;


#define xblif_foreach_button(info,gen,button)				\
        for (gen=lsStart((info)->buttons);				\
                (lsNext(gen, (lsGeneric *) &(button), LS_NH) == LS_OK)	\
                || ((void) lsFinish(gen), 0); )


static void xblif_relabel (info,s)
xblif_info* info;
String s;
{
    if (info->label_text != NULL) xfree (info->label_text);
    info->label_text = XtNewString(s);
    XtVaSetValues (info->label,XtNlabel,info->label_text,NULL);
}

static np_node* netplot_add_pi (w,name)
Widget w;
char *name;
{
    np_node *node = np_find_node (w,name,True);
    np_set_shape (node,np_ellipse);
    np_set_node_color (node,2);
    return node;
}

static np_node* netplot_add_latch (w,name)
Widget w;
char *name;
{
    np_node *node = np_find_node (w,name,True);
    np_set_shape (node,np_textblock);
    return node;
}

static np_node* netplot_add_po (w,name)
Widget w;
char *name;
{
    np_node *node = np_find_node (w,name,True);
    np_set_shape (node,np_ellipse);
    np_set_node_color (node,3);
    return node;
}

static void netplot_read_blif (info)
xblif_info *info;
{
    char buffer[180], buffer2[180];
    char arrow_name[100];
    np_node *node, *latch, *fanin;
    Widget plot = info->plot;
    int i;

    while (xsis_get_token (buffer,sizeof(buffer)) != EOF) {

	if (!strcmp(buffer,".geometry")) {
	    xsis_get_token (buffer,sizeof(buffer));
	    info->geom = XtNewString(buffer);
	}
	else if (!strcmp(buffer,".model")) {
	    xsis_get_token (buffer,sizeof(buffer));
	    xblif_relabel (info,buffer);
	    np_set_title (plot,buffer);
	}
	else if (!strcmp(buffer,".inputs")) {
	    while (xsis_get_token(buffer,sizeof(buffer)) == '\0') {
		netplot_add_pi (plot,buffer);
	    }
	}
	else if (!strcmp(buffer,".latch")) {
	    xsis_get_token (buffer,sizeof(buffer));
	    xsis_get_token (buffer2,sizeof(buffer2));
	    node = netplot_add_po (plot,buffer);
	    sprintf(arrow_name,"to %s", buffer2);
	    latch = netplot_add_latch (plot,arrow_name);
	    np_create_arc (node,latch,NULL);
	    node = netplot_add_pi (plot,buffer2);
	    sprintf(arrow_name,"from %s", buffer);
	    latch = netplot_add_latch (plot,arrow_name);
	    np_create_arc (latch,node,NULL);
	}
	else if (!strcmp(buffer,".outputs")) {
	    while (xsis_get_token(buffer,sizeof(buffer)) == '\0') {
		netplot_add_po (plot,buffer);
	    }
	}
	else if (!strcmp(buffer,".node")
	         || !strcmp(buffer,".names")) {
	    for (i=1; xsis_get_token(buffer,sizeof(buffer)) == '\0'; i++) {
		fanin = np_find_node (plot,buffer,TRUE);
		if (i == 1) {
		    node = fanin;
		} else {
		    np_create_arc (fanin,node,NULL);
		}
	    }
	}
	else if (!strcmp(buffer,".label")) {
	    xsis_get_token (buffer,sizeof(buffer));
	    node = np_find_node (plot,buffer,TRUE);
	    xsis_get_token (buffer,sizeof(buffer));
	    if (node != NULL) np_set_label (node,buffer);
	}
	else if (xsis_world.debug) {
	    printf("xblif: ignored %s\n",buffer);
	}
	xsis_eat_line ();
    }
}

static void
HelpLabel (w,event,params,n_params)
Widget w;                       /*i button widget.	*/
XEvent *event;                  /*  ARGSUSED */
String *params;                 /*  ARGSUSED */
Cardinal *n_params;             /*  ARGSUSED */
{
    xblif_info *info = (xblif_info*) xsis_shell_info (w);
    xblif_button *button;
    lsGen gen;
    if (info == NULL) return;

    xblif_foreach_button (info,gen,button) {
	if (button->command == w) {
	    XtVaSetValues (info->label,XtNlabel,button->help_text,NULL);
	    break;
	}
    }
}

static void
ResetLabel (w,event,params,n_params)
Widget w;                       /*i button widget.	*/
XEvent *event;                  /*  ARGSUSED */
String *params;                 /*  ARGSUSED */
Cardinal *n_params;             /*  ARGSUSED */
{
    xblif_info *info = (xblif_info*) xsis_shell_info (w);

    if (info == NULL) return;
    XtVaSetValues (info->label,XtNlabel,info->label_text,NULL);
}

static void xblif_replace (closure)
XtPointer closure;
{
    /*	Replace the network in an existing blif window. */

    xblif_info *info = (xblif_info*) closure;

    np_clear_plot (info->plot);
    netplot_read_blif (info);
    np_autoplace (info->plot);
}

static void xblif_label (closure)
XtPointer closure;
{
    /*	Allow relabelling of nodes in the xblif window. */

    xblif_info *info = (xblif_info*) closure;

    netplot_read_blif (info);
    np_autoplace (info->plot);
    np_redraw (info->plot,NULL,True);
}

static void do_blif_cmd (w, closure, call_data)
Widget w;			/*  ARGSUSED */
XtPointer closure;		/*u xblif_info.		*/
XtPointer call_data;		/*i selection data.	*/
{
    /*	Callback to execute sis command for a button. */

    xblif_button *button = (xblif_button*) closure;
    xsis_execute (button->sis_cmd,False);
}

static void clear_group (info,group)
xblif_info* info;
String group;
{
    lsGen gen;
    xblif_button *button;

    xblif_foreach_button (info,gen,button) {
	if (!strcmp(button->group,group)) {
	    XtDestroyWidget (button->command);
	    xfree (button->group);
	    xfree (button->sis_cmd);
	    xfree (button->help_text);
	    lsDelBefore (gen, (lsGeneric*)&button);
	}
    }
}

static void add_button (info,w,button)
xblif_info* info;
Widget w;
xblif_button* button;
{
    /*	Set translation on command-line object. */

    static char defaultTranslations[] = "#override\n\
	    <EnterWindow>: highlight() HelpLabel() \n\
	    <LeaveWindow>: reset() ResetLabel()";
    XtTranslations my_translations =
		    XtParseTranslationTable(defaultTranslations);

    button->command = w;
    XtOverrideTranslations (button->command, my_translations);
    lsNewEnd (info->buttons, (lsGeneric)button, LS_NH);
}

static void xblif_hilite (closure)
XtPointer closure;
{
    /*	Read array of nodes to highlight as a path. */

    xblif_info *info = (xblif_info*) closure;
    char *buffer = xsis_world.buffer;
    int buflen = xsis_world.buflen;
    Boolean on = True;
    np_node *node;
    xblif_button *button;
    int n_buttons = 0;

    while (xsis_get_token (buffer,buflen) != EOF) {

	if (!strcmp(buffer,".nodes")) {
	    while (xsis_get_token (buffer,buflen) == '\0') {
		if ((node = np_find_node (info->plot,buffer,False)) != NULL) {
		    np_highlight_node (node,on);
		}
	    }
	}
	else if (!strcmp(buffer,".clear")) {
	    xsis_get_token (buffer,buflen);
	    np_new_highlight (info->plot,buffer);
	}
	else if (!strcmp(buffer,".command")) {
	    /* .command \t group \t sis command \t label \t help \n */
	    button = xalloc (xblif_button,1);
	    xsis_get_token (buffer,buflen);
	    button->group = XtNewString (buffer);
	    if (n_buttons++ == 0) clear_group (info,buffer);
	    xsis_get_token (buffer,buflen);
	    button->sis_cmd = XtNewString (buffer);
	    xsis_get_token (buffer,buflen);
	    button->command = xvwidget (buffer, commandWidgetClass,
						info->button_box, NULL);
	    add_button (info,button->command,button);
	    xsis_get_token (buffer,buflen);
	    button->help_text = XtNewString (buffer);
	    XtAddCallback (button->command, XtNcallback, do_blif_cmd, (XtPointer) button);
	}
	xsis_eat_line ();
    }
}

static void free_blif (w, closure, call_data)
Widget w;			/*  ARGSUSED */
XtPointer closure;		/*u xblif_info.		*/
XtPointer call_data;		/*  ARGSUSED */
{
    /*	Called when a blif window is destroyed. */

    xblif_info *info = (xblif_info*) closure;

    xfree (info->geom);
    xfree (info->label_text);
    lsDestroy (info->buttons, NULL);
}

static void change_label (w, closure, call_data)
Widget w;			/*  ARGSUSED */
XtPointer closure;		/*u xblif_info.		*/
XtPointer call_data;		/*i selection data.	*/
{
    /*	Called whenever plot selection changes. */

    xblif_info *info = (xblif_info*) closure;
    NetPlotSelectData *data = (NetPlotSelectData*) call_data;
    xblif_relabel (info,data->select_name);
}

static xsis_cmds xblif_cmd_list[] = {
    { "replace",		xblif_replace },
    { "label",			xblif_label },
    { "highlight",		xblif_hilite },
    { NULL }
};

static XtActionsRec xblif_actions[] = {           /* Actions table. */
    {"HelpLabel",	HelpLabel},
    {"ResetLabel",	ResetLabel}
};


void xblif_new (closure)
XtPointer closure;
{
    /*	Read a network description in BLIF format and use a NetPlot
	widget to display it. */

    xsis_info *world = &xsis_world;
    String title = (String) closure;
    xblif_info* info = XtNew (xblif_info);
    Widget paned, btnbox, closeme;
    Pixmap icon_px;
    static Boolean added_actions = False;

    if (!added_actions) {
	XtAppAddActions (xsis_world.app_context,xblif_actions,XtNumber(xblif_actions));
	added_actions = True;
    }

    icon_px = XCreateBitmapFromData (
            XtDisplay(world->app_shell),
            RootWindowOfScreen(XtScreen(world->app_shell)),
            blif50_bits,blif50_width,blif50_height);

    info->label_text = NULL;
    info->geom  = NULL;
    info->shell = XtVaCreatePopupShell (
		title,topLevelShellWidgetClass, world->app_shell,
		XtNiconPixmap,icon_px,
		NULL);

    paned  = xvwidget ("blifPane",panedWidgetClass, info->shell, NULL);
    btnbox = xvwidget ("box",boxWidgetClass, paned, NULL);
    info->button_box = btnbox;
    info->buttons = lsCreate();
    closeme = xvwidget ("close",commandWidgetClass, btnbox, NULL);

    info->label = xvwidget ("label",labelWidgetClass, paned, NULL);
    xblif_relabel (info,title);

    info->plot = xvwidget ("netPlot", netPlotWidgetClass, paned, NULL);
    netplot_read_blif (info);
    if (info->geom) XtVaSetValues (info->shell,XtNgeometry,info->geom,NULL);
    np_autoplace (info->plot);
    
    {	xblif_button* button = xalloc (xblif_button,1);
	lsNewEnd (info->buttons, (lsGeneric)button, LS_NH);
	button->group = XtNewString("");
	button->help_text = XtNewString("Close plot window.");
	button->sis_cmd = NULL;
	add_button (info,closeme,button);
    }

    XtAddCallback (closeme,XtNcallback,xsis_close_window,(XtPointer)info->shell);

    XtAddCallback (info->plot,XtNselectCallback,change_label,(XtPointer)info);
    XtAddCallback (info->shell,XtNdestroyCallback,free_blif,(XtPointer)info);

    xsis_add_shell (title,info->shell,XSIS_BLIF,xblif_cmd_list,(XtPointer)info);
}
