/* -------------------------------------------------------------------------- *\
   xhelp.c -- do "help" shell.

	$Revision: 1.1.1.1 $
	$Date: 2004/02/07 10:14:14 $
	$Author: pchong $
	$Source: /users/pchong/CVS/sis/xsis/xhelp.c,v $

   Copyright 1991 by the Regents of the University of California.

   All rights reserved.  Permission to use, copy, modify and distribute
   this software is hereby granted, provided that the above copyright
   notice and this permission notice appear in all copies.  This software
   is made available as is, with no warranties.

   We read the file ourselves because the asciiText widget just exits if
   it cannot find the file, and it also does not handle ^H well.
\* -------------------------------------------------------------------------- */

#include "xsis.h"
#include "NetPlot.h"
#include "help50.px"

#define XSIS_HELP	"help"
#define HELP_SUFFIX	"fmt"

typedef struct TextBlock {
    String	 text;		/* Allocated text string.	*/
    int		 n_byte;	/* strlen of current string.	*/
    int		 n_alloc;	/* Allocated size of string.	*/
} TextBlock;

typedef struct xhelp_info {
    Widget	 shell;
    Widget	 text;
    Widget	 label;
    TextBlock	 help_text;
    String	*topics;
    int		 show_all;
} xhelp_info;

static xhelp_info *help_opened = NULL;

static void TextBlockClear (text)
TextBlock* text;
{
    text->n_byte = 0;
    *text->text = '\0';
}

static void TextBlockInit (text)
TextBlock* text;
{
    /*	Start with some memory and an empty string. */
    text->n_alloc = 2048;
    text->text = xalloc (char,text->n_alloc);
    text->n_byte = 0;
    *text->text = '\0';
}

static void TextBlockFree (text)
TextBlock* text;
{
    XFree (text->text);
}

static void TextBlockCat (text,s)
TextBlock* text;
String s;
{
    /*	Concatenate a string to the text block.  First reallocate enough
	memory, then copy the string in, handling backspace appropriately. */

    int max_len = text->n_byte+strlen(s)+1;
    String p;
    int c;

    if (max_len >= text->n_alloc) {
	text->n_alloc *= 2;
	if (text->n_alloc < max_len) text->n_alloc = max_len;
	text->text = xrealloc (char,text->text,text->n_alloc);
    }

    for (p=text->text+text->n_byte; (c=(*s)) != '\0'; s++) {
	if (c == '\010') {
	    if (p != text->text) p--;
	} else {
	    *(p++) = c;
	}
    }

    text->n_byte = (p - text->text);
    *p = '\0';
}

static void CloseHelp (widget,closure,data)
Widget widget;			/*  ARGSUSED */
XtPointer closure;		/*  ARGSUSED */
XtPointer data;			/*  ARGSUSED */
{
    /*	Deallocate memory associated with the help window. */
    xhelp_info *info = (xhelp_info*) closure;
    String *topic;

    for (topic=info->topics; *topic != NULL; topic++) {
	xfree (*topic);
    }
    TextBlockFree (&info->help_text);
    help_opened = NULL;
}

static void SetHelp (info, topic)
xhelp_info *info;
char *topic;
{
    /*	Find and read the help file for the given topic and change the
	help shell text to display this file. */

    FILE *fp;
    char *s, *p, *fname, filename[100], buffer[200];
    char *paths = xsis_get_flag ("open_path");
    int len, n_line = 0;

    sprintf(filename,"help/%s.%s",topic,HELP_SUFFIX);
    fname = util_file_search (filename,paths,"r");
    TextBlockClear (&info->help_text);

    if (fname == NULL || (fp=fopen(fname,"r")) == NULL) {
	sprintf (buffer,"Could not find help file \"%s.fmt\".\n",topic);
	TextBlockCat (&info->help_text,buffer);
	TextBlockCat (&info->help_text,"Tried directories:\n\n");
	for (p=paths; p != NULL; p=(s)?s+1:NULL) {
	    s = strchr(p+1,':');
	    len = (s == NULL) ? strlen(p) : (s-p);
	    strcat(strncat(strcpy(buffer,"    "), p,len), "/help\n");
	    TextBlockCat (&info->help_text,buffer);
	}
	TextBlockCat(&info->help_text,
		"\nUse \"set open_path\" to change this list.");
    }
    else {
	/* The first 3 lines of sis help files aren't interesting. */
	while (fgets(buffer,sizeof(buffer),fp) != NULL) {
	    if (++n_line > 3) TextBlockCat (&(info->help_text),buffer);
	}

	fclose (fp);
    }

    XtVaSetValues (info->label, XtNlabel,topic, NULL);
    XtVaSetValues (info->text,  XtNstring,info->help_text.text,
			 	XtNlength, strlen(info->help_text.text),
				NULL);
    FREE (fname);
}

static void SetTopic (widget,closure,call_data)
Widget widget;			/*  ARGSUSED */
XtPointer closure;		/*u Help information struct.	*/
XtPointer call_data;		/*i List widget information.	*/  
{
    /*	Callback to change help text based on list selection.  Can
	also use info->list_index for list widgets. */

    XawListReturnStruct *info = (XawListReturnStruct*) call_data;
    SetHelp ((xhelp_info*)closure,info->string);
}

static xsis_cmds xhelp_cmds[] = {
    { NULL }
};

void xhelp_popup (topic,geom,show_all)
String topic;
String geom;
int show_all;
{
    /*	Create help shell if necessary and set it to given topic. */

    Widget paned, pane2;
    Widget btnbox, closeme, topic_list, topic_view;
    Pixmap icon_px;
    int n_cmd, i,j;
    String sis_topic;
    xhelp_info *info;

    if (help_opened != NULL) {
	SetHelp (help_opened,topic);
	return;
    }

    info = XtNew (xhelp_info);
    TextBlockInit (&info->help_text);

    icon_px = XCreateBitmapFromData (
	    XtDisplay(xsis_world.app_shell),
	    RootWindowOfScreen(XtScreen(xsis_world.app_shell)),
	    help50_bits,help50_width,help50_height);

    info->shell = XtVaCreatePopupShell (
	    XSIS_HELP,topLevelShellWidgetClass, xsis_world.app_shell,
			XtNiconPixmap,icon_px,
			XtNiconName, "xsis help",
			NULL);
    if (geom != NULL) XtVaSetValues (info->shell,XtNgeometry,geom,NULL);

    paned = xvwidget("paned1",panedWidgetClass, info->shell, NULL);
    btnbox = xvwidget ("box",boxWidgetClass, paned, NULL);

    pane2 = xvwidget("paned2",panedWidgetClass, paned,
			XtNorientation,XtorientHorizontal,
			NULL);

    closeme = xvwidget ("close",commandWidgetClass, btnbox, NULL);
    info->label = xvwidget ("label",labelWidgetClass, btnbox, NULL);

    topic_view = xvwidget ("topicList",viewportWidgetClass,pane2,
			XtNallowVert,True,
			NULL);

    /* Create list of topics to choose from. */
    n_cmd = array_n(xsis_world.sis_commands);
    info->topics = xalloc (String,n_cmd+1);
    for (i=j=0; i < n_cmd; i++) {
	sis_topic = array_fetch (String,xsis_world.sis_commands,i);
	if (!show_all && sis_topic[0] == '_') continue;
	info->topics[j++] = XtNewString(sis_topic);
    }
    info->topics[j] = NULL;
    
    topic_list = xvwidget ("list",listWidgetClass,topic_view,
			XtNdefaultColumns, 1,
			XtNlist, info->topics,
			XtNverticalList, True,
			XtNforceColumns, True,
			NULL);

    info->text = xvwidget ("helpText",asciiTextWidgetClass, pane2,
			XtNdisplayCaret, False,
			XtNeditType, XawtextRead,
			XtNlength, 0,
			XtNstring, "",
			XtNtype, XawAsciiString,
			XtNuseStringInPlace, True,
			NULL);

    XtAddCallback (topic_list, XtNcallback, SetTopic, (XtPointer)info);
    XtAddCallback (closeme, XtNcallback, xsis_close_window, (XtPointer)info->shell);
    XtAddCallback (info->shell, XtNdestroyCallback, CloseHelp, (XtPointer)info);
    xsis_add_shell ("help",info->shell,XSIS_HELP,xhelp_cmds,(XtPointer)info);		
    if (topic != NULL) SetHelp (info,topic);
    help_opened = info;
}

void xhelp_new (closure)
XtPointer closure;	/*  ARGSUSED */
{
    /*	Read filename and display it as help. */

    xsis_info *world = &xsis_world;
    int show_all = False;
    char *geom = NULL;

    while (xsis_get_token(world->buffer,world->buflen) != EOF) {
	if (!strcmp(world->buffer,".topic")) {
	    xsis_get_token(world->buffer,world->buflen);
	    xhelp_popup (world->buffer,geom,show_all);
	}
	else if (!strcmp(world->buffer,".all")) {
	    show_all = True;
	}
	else if (!strcmp(world->buffer,".geometry")) {
	    xsis_get_token(world->buffer,world->buflen);
	    geom = XtNewString (world->buffer);
	}
	xsis_eat_line ();
    }

    xfree (geom);
}
