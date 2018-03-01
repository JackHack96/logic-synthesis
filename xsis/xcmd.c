/* -------------------------------------------------------------------------- *\
   xcmd.c -- shell for command interface to sis.

	$Revision: 1.1.1.1 $
	$Date: 2004/02/07 10:14:14 $
	$Author: pchong $
	$Source: /users/pchong/CVS/sis/xsis/xcmd.c,v $

   Copyright 1991 by the Regents of the University of California.

   All rights reserved.  Permission to use, copy, modify and distribute
   this software is hereby granted, provided that the above copyright
   notice and this permission notice appear in all copies.  This software
   is made available as is, with no warranties.

   This handles the application shell which is the main top level shell of
   xsis.  It is set up exactly like any other shell, except that it uses
   world->app_shell for its shell instead of creating a new popup shell.
   The version number defined below is appended to the displayed icon name.
\* -------------------------------------------------------------------------- */

#include "xsis.h"

#define XSIS_VERSION		" 1.1"
#define XSIS_CMD		"cmd"


typedef struct xcmd_info {		/* Info for command shell.	*/
    Widget	 shell;			/* Command shell.		*/
    Widget	 version;		/* Version label widget.	*/
} xcmd_info;


static Widget g_textout = NULL;
static int caret_posn = 0;

static String xcmd_do_special (p)
String p;
{
    XawTextBlock new_text;
    XawTextPosition pos;

    if (p == NULL) return NULL;

    if (*p == '\015') {
	/* Try to clear the current line. */
	new_text.firstPos = 0;
	new_text.length = 1;
	new_text.ptr = "\n";
	new_text.format = FMT8BIT;
	XawTextSetInsertionPoint (g_textout,32765);
	pos = XawTextSearch (g_textout, XawsdLeft,&new_text);
	if (pos != XawTextSearchError) {
	    caret_posn = pos + 1;
	}
	p++;
    } else if (*p == '\010') {
	/* Try to delete the previous character. */
	if (caret_posn > 1) {
	    new_text.firstPos = 0;
	    new_text.length = 0;
	    new_text.ptr = "";
	    new_text.format = FMT8BIT;
	    XawTextReplace (g_textout,caret_posn-1,caret_posn,&new_text);
	    caret_posn -= 1;
	}
	p++;
    } else {
	new_text.firstPos = 0;
	new_text.length = 1;
	new_text.ptr = "?";
	new_text.format = FMT8BIT;
	XawTextReplace (g_textout,caret_posn,caret_posn+1,&new_text);
	caret_posn++;
	p++;
    }

    return p;
}

void xcmd_puts (str)
String str;
{
    /*	Append text to the global textout widget.  Does not add a
	newline automatically. */

    XawTextBlock new_text;
    String s = str, p;

    if (str == NULL) return;

    if (g_textout == NULL) {
	fputs (str,stdout);
	return;
    }

    while (s != NULL && *s != '\0') {
	p = strpbrk (s,"\007\010\015");
	/* Insert regular text. */
	new_text.firstPos = 0;
	new_text.length = (p==NULL) ? strlen(s) : (p-s);
	new_text.ptr = s;
	new_text.format = FMT8BIT;
	XawTextReplace (g_textout,caret_posn,caret_posn+new_text.length,&new_text);
	caret_posn += new_text.length;
	/* Handle special character sequences. */
	s = xcmd_do_special (p);
    }

    XawTextDisplayCaret (g_textout,False);
    /* This is necessary to scroll the bottom to be visible. */
    XawTextSetInsertionPoint (g_textout,32765);
}

static void print_file_list (w,filec)
Widget w;
xsis_filec* filec;
{
    /*	Print a list of the files the same as csh does for file completion. */

    String name, p;
    int i, j, k, width, len, max_len, n_col, n_row, n_full_row, elem_n;
    char buffer[80];
    XFontStruct *font;
    Dimension swide;

    xcmd_puts ("\n");
    if (array_n(filec->filenames) == 0) {
	xcmd_puts (xsis_get_flag("prompt"));
	return;
    }

    XtVaGetValues (w, XtNwidth,&swide, XtNfont,&font, NULL);
    width = swide / font->max_bounds.width;
    /* Add two for inter-item spacing, and allow for scroll bar in text. */
    max_len = xsis_string_array_len (filec->filenames) + 2;

    n_col = width/(max_len+1);
    if (n_col == 0) n_col = 1;
    n_row = (array_n(filec->filenames)+n_col-1) / n_col;
    n_col = (array_n(filec->filenames)+n_row-1) / n_row;
    n_full_row = array_n(filec->filenames) % n_row;
    if (n_full_row == 0) n_full_row = n_row;

    for (i=0; i < n_row; i++) {
	if (i == n_full_row) n_col--;
	elem_n = i;
	for (j=1; j <= n_col; j++) {
	    name = array_fetch(String,filec->filenames,elem_n);
	    strcat(strcpy(buffer,name), xsis_file_type(filec,elem_n));
	    if (j == n_col) {
		strcat(buffer,"\n");
	    } else {
		len = strlen(buffer);
		p = buffer + strlen(buffer);
		for (p=buffer+len,k=max_len-len; k--; p++) *p = ' ';
		*p = '\0';
	    }
	    xcmd_puts(buffer);
	    elem_n += n_row;
	}
    }

    xcmd_puts (xsis_get_flag("prompt"));
}

static void do_file_completion (w,filec)
Widget w;
xsis_filec* filec;
{
    /*	Complete the maximum common prefix of all the given names. */

    XawTextBlock tblock;
    String name;
    int len, pos;

    len = strlen(filec->partial);
    pos = xsis_string_array_prefix (filec->filenames);
    if (pos-len > 0) {
	name = array_fetch(String,filec->filenames,0);
	tblock.firstPos = len;
	tblock.length = pos - len;
	tblock.ptr = name;
	tblock.format = FMT8BIT;
	XawTextReplace (w,32765,32765,&tblock);
	XawTextSetInsertionPoint (w,32765);
    }
    if (array_n(filec->filenames) != 1) XBell (xsis_world.display,0);
}

static void FileCompletion (w,list_only)
Widget w;
int list_only;
{
    /*	Do file completion for text widget w.  If list_only, then
	print all files matching, otherwise try to complete the name. */

    String str, last_word;
    xsis_filec *filec;

    XtVaGetValues (w, XtNstring,&str, NULL);
    if (str == NULL) return;

			/* Back up to the start of the last ``word''. */
    last_word = str + strlen(str) - 1;
    for (last_word=str+strlen(str)-1; last_word > str; last_word--) {
	if (strchr(" \t\n;",*last_word) != NULL) {
	    last_word++;
	    break;
	}
    }

    /* Allow for file names as part of a command option. */
    if (last_word[0] == '-' && last_word[1] != '\0') last_word += 2;

    filec = xsis_file_completion (last_word,False);

    if (filec == NULL) {
	XBell (xsis_world.display,0);
    } else if (list_only) {
	print_file_list (w,filec);
    } else {
	do_file_completion (w,filec);
    }

    xsis_free_filec (filec);
}

static void special_action (w,action)
Widget w;
String action;
{
    /*	Do a single special action for a command buffer such as file
	completion, kill or interrupt. */

    XawTextBlock tblock;
    char special[3];
    String buffer;

    if (!strcmp(action,"enter")) {		/* Send buffer as is. */
	XawTextSetInsertionPoint (w,0);
	XtVaGetValues (w, XtNstring,&buffer, NULL);
	xsis_execute (buffer,False);
	XtVaSetValues (w, XtNstring,"", NULL);
    }
    else if (!strcmp(action,"intr")) {		/* Interrupt sis (^C). */
	/* Send interrupt signal to sis child. */
	xsis_intr_child();
    }
    else if (!strcmp(action,"eof")) {		/* Send EOF char to sis. */
	special[0] = xsis_pty_eof_char (xsis_world.sis_pty);
	special[1] = '\0';
	xsis_execute (special,True);
    }
    else if (!strcmp(action,"filel")) {		/* File list. */
	/* Do file completion on a partial string. */
	XtVaGetValues (w, XtNstring,&buffer, NULL);
	FileCompletion (w,True);
    }
    else if (!strcmp(action,"filec")) {		/* File completion. */
	FileCompletion (w,False);
    }
    else if (!strcmp(action,"kill")) {		/* Clear buffer. */
	XtVaSetValues (w, XtNstring,"", NULL);
    }
    else if (action[0] == '+') {		/* Insert literal in buffer. */
	tblock.firstPos = 1;
	tblock.length = strlen(action+1);
	if (action[tblock.length] == '+') tblock.length--;
	tblock.ptr = action;
	tblock.format = FMT8BIT;
	XawTextReplace (w,32765,32765,&tblock);
	XawTextSetInsertionPoint (w,32765);
    }
    else if (strcmp(action,"no-op")) {		/* Do absolutely nothing. */
	fprintf(stderr,"EnterCmd: unknown argument \"%s\".\n",action);
    }
}

static void EnterCmd (w,event,params,n_params)
Widget w;
XEvent* event;
String* params;
Cardinal* n_params;
{
    /*	Action procedure for entering a new command.  Send any text in the
	command widget, followed by optional command as argument.  See
	special_action() above for the list of recognized parameters.
	Parameter zero is what to do if the buffer is empty.  Parameters
	one and up are what to do if the buffer is nonempty. */

    /* ARGSUSED */
    String str;
    int i;

    XtVaGetValues (w, XtNstring,&str, NULL);
    if (str == NULL) return;	/* Maybe not a text widget? */

    if (strlen(str) == 0) {
	if (*n_params >= 1) special_action (w,params[0]);
    } else {
	for (i=1; i < *n_params; i++) {
	    special_action (w,params[i]);
	}
    }
}

static void CmdChanged (w,text_ptr,call_data)
Widget w;
XtPointer text_ptr;
XtPointer call_data;
{
    /* This gets called for every potential change in the text. */

    /* ARGSUSED */
    Widget cmdline = (Widget) text_ptr;
    String str;

    XtVaGetValues (cmdline, XtNstring,&str, NULL);
    /* Could also do command completion, etc. here. */
/*
    if (xsis_icanon_disabled (xsis_world.sis_pty)) {
	xsis_execute (str,True);
	XtVaSetValues (w, XtNstring,"", NULL);
    }
 */
}

static void ClearText (w,text_ptr,call_data)
Widget w;
XtPointer text_ptr;
XtPointer call_data;
{
    /*	Clear all text out of the text widget. */

    /* ARGSUSED */
    Widget text = (Widget) text_ptr;
    XtVaSetValues (text, XtNstring,"", NULL);
    if (xsis_world.child_status == XSIS_IDLE) {
	xcmd_puts (xsis_get_flag("prompt"));
    }
}

static void Quit (widget,closure,callData)
Widget widget;
XtPointer closure;
XtPointer callData;
{
    /* ARGSUSED */
    xsis_execute ("quit",False);
}

static void ShowHelp (w,closure,call_data)
Widget w;
XtPointer closure;
XtPointer call_data;
{
    /*	Open help shell if it isn't already. */

    /* ARGSUSED */
    xhelp_popup ("help",NULL,False);
}

static void do_version (info)
xcmd_info* info;
{
    /*	Read version string and set main window label.	*/

    char *p, buffer[100];
    char hostname[128];

    while (xsis_get_token(buffer,sizeof(buffer)) != EOF) {
	if (!strcmp(buffer,".version")) {
	    xsis_get_token(buffer,sizeof(buffer));
	    p = strchr (buffer,'(');
	    if (p != NULL && p > buffer) *(p-1) = '\0';
	    if (gethostname(hostname,sizeof(hostname)) == 0) {
		p = strchr(hostname,'.');
		if (p != NULL && p != hostname) *p = '\0';
		p = buffer + strlen(buffer);
		sprintf(p," (%s)",hostname);
	    }
	    if (info != NULL) {
		XtVaSetValues (info->version, XtNlabel,buffer, NULL);
	    }
	}
	else if (xsis_world.debug) {
	    xsis_dump_string("xcmd ignored",buffer);
	}
	xsis_eat_line ();
    }
}

static XtActionsRec cmd_actions[] = {		/* Actions table. */
    {"EnterCmd", EnterCmd}
};

static xsis_cmds xcmd_cmds[] = {
    { NULL }
};

void xcmd_new (closure)
XtPointer closure;
{
    /*	Special shell which uses world->app_shell as its shell. */

    xsis_info *world = &xsis_world;
    String title = (String) closure;
    String new_title;
    Widget source, quit, help, clear, textout, cmdline, box, paned;
    xcmd_info *info;
    char buffer[128];

    if (g_textout != NULL) {		/* Never allow two command shells. */
	do_version (NULL);
	return;
    }

    info = XtNew(xcmd_info);
    info->shell = world->app_shell;

    /* Append the RCS version number to the window title/icon name. */
    XtVaGetValues (info->shell, XtNiconName, &new_title, NULL);
    strcat(strcpy(buffer,new_title), XSIS_VERSION);
    XtVaSetValues (info->shell, XtNiconName, buffer, NULL);
    XtVaSetValues (info->shell, XtNtitle, buffer, NULL);

    XtAppAddActions (world->app_context,cmd_actions,XtNumber(cmd_actions));

    paned = xvwidget ("paned",panedWidgetClass, info->shell, NULL);

    box   = xvwidget ("box",boxWidgetClass, paned, NULL);
    quit  = xvwidget ("quit", commandWidgetClass, box, NULL);
    help  = xvwidget ("help", commandWidgetClass, box, NULL);
    clear = xvwidget ("clear",commandWidgetClass, box, NULL);

    info->version = xvwidget ("version",labelWidgetClass, box, NULL);

    cmdline = xvwidget ("cmdline",asciiTextWidgetClass, paned, NULL);

    {/*	Set command-line height to font height. */
	XFontStruct *font;
	int height;
	XtVaGetValues (cmdline,XtNfont,&font,NULL);
	height = font->ascent + font->descent + 6;
	XtVaSetValues (cmdline,XtNpreferredPaneSize,height,NULL);
    }

    textout = xvwidget ("textout",asciiTextWidgetClass, paned, NULL);
    g_textout = textout;

	/* Now add callbacks since all widgets are created. */

    XtVaGetValues (cmdline,XtNtextSource,&source,NULL);
    XtAddCallback (source, XtNcallback, CmdChanged, (XtPointer) cmdline);

    XtAddCallback (quit,  XtNcallback, Quit,	  (XtPointer) info->shell);
    XtAddCallback (help,  XtNcallback, ShowHelp,  (XtPointer) world);
    XtAddCallback (clear, XtNcallback, ClearText, (XtPointer) textout);
    XtSetKeyboardFocus (paned,cmdline);

    /* Allow events in textout (dest) to invoke actions in cmdline (source). */
    XtInstallAccelerators (textout,cmdline);

    do_version (info);

    xsis_add_shell (title,info->shell,XSIS_CMD,xcmd_cmds,(XtPointer)info);
}
