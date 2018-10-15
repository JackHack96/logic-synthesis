/* -------------------------------------------------------------------------- *\
   xsis.c -- handle special cases of input from sis child process.

	$Revision: 1.1.1.1 $
	$Date: 2004/02/07 10:14:14 $
	$Author: pchong $
	$Source: /users/pchong/CVS/sis/xsis/xsis.c,v $

   Copyright 1991 by the Regents of the University of California.

   All rights reserved.  Permission to use, copy, modify and distribute
   this software is hereby granted, provided that the above copyright
   notice and this permission notice appear in all copies.  This software
   is made available as is, with no warranties.

   Primary entry point is xsis_main which creates initial command shell
   and handles event loop.

   Special graphics commands come from sis in the form:

	<GRAPHICS-START-MESSAGE>
	shelltype \t command \t shellname \n
	..command-specific data..
	<GRAPHICS-END-MESSAGE>

   This makes it possible to survive unrecognized graphics commands.

   Using XrmoptionSkipArg did not work for the -sis option, which is
   handled by the driver.  So we pretend it is a real resource using
   XrmoptionSepArg and then never use it.
\* -------------------------------------------------------------------------- */

#include "xsis.h"
#include "NetPlot.h"
#include "ghost.px"
#include "sis50.px"

#define XSIS_SIS	"sis"

static xsis_cmds window_creators[] = {
    /* Type	Creator  */
    {  "cmd",	xcmd_new },
    {  "help",	xhelp_new },
    {  "blif",	xblif_new },
    {  "astg",	xastg_new },
    NULL
};

static XrmOptionDescRec xsis_options[] = {
    {"-debug",	"debug",	XrmoptionNoArg,		"True"},
    {"-sis",	".sis",		XrmoptionSepArg,	NULL}
};

xsis_info xsis_world;		/* Global interface data.	*/


static XtResource app_resources[] = {
#define offset(field) XtOffset(xsis_info*, field)
    { "debug", "Debug", XtRBoolean, sizeof(Boolean),
      offset(debug), XtRImmediate, (XtPointer) False }
#undef offset
};

static String xsis_fallback_resources[] = { 

    "XSis.geometry:			820x300",
    "XSis*background:			grey75",
    "XSis*input:			True",
    "XSis*showGrip:			on",
    "XSis*font:				courier_bold10",
    "XSis*Command*background:		lightblue",
    "XSis*Label*borderWidth:		0",
    "XSis*quit.label:			Quit",
    "XSis*help.label:			Help",
    "XSis*clear.label:			Clear",
    "XSis*close.label:			Close",
    "XSis*cmdline*background:		lightblue",
    "XSis*cmdline*editType:		edit",
    "XSis*cmdline.scrollVertical:	never",
    "XSis*cmdline.scrollHorizontal:	whenNeeded",
    "XSis*cmdline*wrap:			never",
    "XSis*cmdline*autoFill:		False",
    "XSis*cmdline*displayNonprinting:	True",
    "XSis*cmdline.Translations: #override \\n\
	<Key>Escape: EnterCmd(no-op, filec) \\n\
	Ctrl<Key>c:  EnterCmd(intr,  kill,intr) \\n\
	Ctrl<Key>d:  EnterCmd(eof,   filel) \\n\
	Ctrl<Key>u:  EnterCmd(no-op, kill) \\n\
	<Key>Return: EnterCmd(enter, enter)",
    "XSis*cmdline.accelerators: #override \\n\
	<ButtonPress>Button2:   insert-selection(PRIMARY, CUT_BUFFER0)",
    "XSis*textout*editType:		edit",
    "XSis*textout.scrollVertical:	whenNeeded",
    "XSis*textout.scrollHorizontal:	whenNeeded",
    "XSis*textout*cursor:		crosshair",
    "XSis*textout.bottomMargin:		9",
    "XSis*textout.Translations: #override\\n\
	<Key>: no-op() \\n\
	<Btn2Down>: no-op()",
    "XSis*blifPane.label.justify:	left",
    "XSis*helpText*font:		times_roman10",
    "XSis*lineWidth:			0",
    "XSis*helpText.width:		570",
    "XSis*helpText*background:		mistyrose3",
    "XSis*helpText.scrollVertical:	always",
    "XSis*helpText.scrollHorizontal:	whenNeeded",
    "XSis*helpText.Translations: #override \\n\
	<Key>1:    	beginning-of-file() \\n\
	<Key>space:	next-page() \\n\
	<Key>f:		next-page() \\n\
	<Key>b:		previous-page() \\n\
	<Key>Return:	scroll-one-line-up() \\n\
	<Key>/:    	search(forward) \\n\
	:<Key>?:    	search(backward) \\n\
	<Key>:		no-op()",
    "XSis*topicList.height:		340",
    "XSis*netPlot*font:			times_bold10",
    "XSis*netPlot.foreground1:		royalblue2",
    "XSis*netPlot.foreground2:		indianred",
    "XSis*netPlot.foreground3:		forestgreen",
    "XSis*netPlot.foreground4:		grey20",
    "XSis*netPlot.highlight:		yellow",
    "XSis*stnPlot*font:			times_bold10",
    "XSis*stnPlot.foreground1:		royalblue2",
    "XSis*stnPlot.foreground4:		grey20",
    "XSis*stnPlot.highlight:		yellow",
    "XSis*stnPlot*Translations: #override \\n\
     	<Btn1Down>:	SelectNode(out) \\n\
     	<Btn2Down>:	SelectNode(in)",
    "XSis*Dialog*value.translations: #override \\n\
     	<Key>Return:	Ok()",
    "XSis*Dialog*label.resizable:	TRUE",

NULL };

void xsis_io_error (message)
char *message;
{
    printf("input error: %s\n",message);
    fail("bad file");
}

char *xsis_find_sis (argc,argv)
int argc;
char** argv;
{
    /*	Locate a copy of sis to run by checking: (1) filename in -sis option,
	(2) $PATH variable, (3) directory portion of argv[0].  As a side
	effect, it also checks for the -debug option so that debugging
	messages can start even before Xt parses the options. */

    char *path, *p, *sis_path = NULL;
    int arg_n;

    for (arg_n=1; arg_n < argc; arg_n++) {
	if (!strcmp(argv[arg_n],"-sis")) {
	    if (arg_n == argc-1) {
		printf("%s: -sis requires a pathname argument.\n",argv[0]);
		return NULL;
	    } else {
		sis_path = XtNewString(argv[arg_n+1]);
	    }
	} else if (!strcmp(argv[arg_n],"-debug")) {
	    xsis_world.debug = True;
	    printf("Debugging output enabled.\n");
	}
    }

    if (sis_path == NULL) {	/* Then try $PATH dirs. */
	sis_path = util_path_search ("sis");
    }

    if (sis_path == NULL && (p=strrchr(argv[0],'/')) != NULL) {
	/* Try the directory that xsis was found in. */
	path = xalloc(char,strlen(argv[0])+10);
	strcat(strncpy(path, argv[0], p-argv[0]), ":.");
	sis_path = util_file_search ("sis",path,"x");
	xfree(path);
    }

    return sis_path;
}

/* It would be ideal if I/O on the pty connection to the sis child could be
   done using stdio calls after doing an fdopen(3S) on the pty controller
   file descriptor.  Unfortunately the input functions (fgetc, fgets, ...)
   did not work correctly for some machines (e.g. Sun4, IBM RS/6000).  So
   we have to reinvent the wheel and derive a set of equivalent calls based
   directly on read(2) which does work correctly.  Since I only want to do
   this for the pty, I did not declare an analog to the FILE data type, and
   just hardcoded these functions to use the pty from the xsis_world global. */

static char xsis_in_buffer[4096];
static int rnext = 0, rlast = 0;

static int xsis_getc ()
{
    /*	Same as getc(3S) but for the sis pty. */
    int x;

    if (xsis_world.input_status == XSIS_GRAPHICS_EOF) return EOF;

    if (rnext == rlast) {
	rnext = rlast = 0;
	x = read (xsis_world.sis_pty,xsis_in_buffer,sizeof(xsis_in_buffer));
	if (x == 0) {
	    return EOF;
	} else if (x == -1) {
	    /* Ignore some "expected" error conditions. */
	    if (errno != EWOULDBLOCK && errno != EIO && errno != EAGAIN) {
		xsis_perror ("read1");
	    }
	    return EOF;
	} else {
	    rlast = x;
	}
    }

    return xsis_in_buffer[rnext++];
}

static void xsis_ungetc (c)
int c;
{
    /*	Similar to ungetc(3S) but specific for the sis pty. */
    if (rnext > 0 && c != EOF) {
	rnext--;
	assert (c == xsis_in_buffer[rnext]);
    }
}

void xsis_eat_line ()
{
    int c;
    do {
	c = xsis_getc();
    } while (c !='\n' && c != EOF);
}

int xsis_get_token (buffer,buflen)
char *buffer;
int buflen;
{
    char *p = buffer;
    int c, curr_len = 0;

    if ((c=xsis_getc()) == '\n' || c == EOF) {
	xsis_ungetc(c);
	return c;
    }

    /* Allow one tab to separate each token. */
    if (c == '\t') c = xsis_getc();

    while (c != '\n' && c != '\t' && c != EOF) {
	if (++curr_len < buflen) {
	    *(p++) = c;
	}
	c = xsis_getc();
    }

    if (xsis_world.input_status == XSIS_READING_GRAPHICS) {
	/* Check for end-graphics message. */
	p[0] = c; p[1] = '\0';
	if (!strcmp(buffer,COM_GRAPHICS_MSG_END) && c == '\n') {
	    xsis_world.input_status = XSIS_GRAPHICS_EOF;
	    return EOF;
	}
    }

    xsis_ungetc(c);
    *p = '\0';
    return '\0';
}

char *xsis_fgets (buffer,len,drop_nl)
char* buffer;
int len;
int drop_nl;
{
    /*  Like gets(3s) and fgets(3s), this reads input until 1) there is no
	more, 2) a newline is found, 3) end of the buffer is reached.  If
	the drop_nl flag is nonzero, a terminating newline is not saved in
	the buffer (i.e. behaves like gets), otherwise it is saved in the
	buffer (i.e. like fgets).  Hard-coded for the sis pty. */

    char *p = buffer;
    int c;

    while ((c=xsis_getc()) != EOF) {
	if (--len == 0) break;
	if (c == '\n') {
	    if (xsis_world.input_status == XSIS_READING_GRAPHICS) {
		p[0] = c; p[1] = '\0';
		if (!strcmp(buffer,COM_GRAPHICS_MSG_END)) {
		    xsis_world.input_status = XSIS_GRAPHICS_EOF;
		    return NULL;
		}
	    }
	    if (!drop_nl) *(p++) = '\n';
	    break;
	}
	*(p++) = c;
    }

    *p = '\0';
    if (p == buffer && c == EOF) return NULL;
    return buffer;
}

void xsis_dump_string (message,s)
String message;
String s;
{
    /*	Write a detailed description of the string s using the format
	   <message>: "<s>"
		    : <octal>
	with control characters in <s> written in a displayed format, and
	the octal form using the \ddd format. */

    String p;
    int i, c;

    fputs (message,stdout);
    fputs (": \"",stdout);

    for (p=s; (c=(*p)) != '\0'; p++) {
	if (c == '\n') {
	    fputs("\\n",stdout);
	} else if (c == '\t') {
	    fputs("\\t",stdout);
	} else if (iscntrl(c)) {
	    putchar('^'); putchar('A'+c-1);
	} else {
	    putchar(c);
	}
    }
    fputs ("\"\n",stdout);

    for (i=strlen(message); i--; ) fputc(' ',stdout);
    fputs (": ",stdout);
    for (p=s; (c=(*p)) != '\0'; p++) {
	fprintf(stdout,"\\%.3o",c);
    }
    fputs ("\n",stdout);
}

void xsis_execute (str,special)
String str;
int special;
{
    /*	Start execution of the given command by writing it to sis child.  For
	special commands don't add a newline.  Otherwise do the command as is
	and add a newline. */

    if (xsis_world.debug) {
	xsis_dump_string ("c->s",str);
	if (!strcmp(str,"bailout")) _exit(0);
    }
    if (write (xsis_world.sis_pty,str,strlen(str)) == -1) {
	xsis_perror("write");
    }
    if (!special && write (xsis_world.sis_pty,"\n",1) == -1) {
	xsis_perror("writeln");
    }
}

/* ---------------------------- A Simple Popup ------------------------------ */

static void DestroyPopupPrompt (widget,client_data,call_data)
Widget widget;
XtPointer client_data;
XtPointer call_data;
{
    /*	The client_data is the dialog object, which is a direct child
	of the popup shell to destroy. */

    /* ARGSUSED */
    Widget popup = XtParent( (Widget) client_data);
    XtDestroyWidget(popup);
    xsis_world.keep_running = False;
}

void xsis_alert (message)
String message;
{
    /*	Simple use of popup dialog.  See popup.c example for more details. */

    Widget	popup, dialog;
    Position	x, y;
    Dimension	width, height;
    Pixmap	icon_px;
    Widget	top_level = xsis_world.app_shell;

    /* Position the top left corner of the popup at the center of the parent
     * widget.  This does not handle the case where the popup is off screen.
     */

    icon_px = XCreateBitmapFromData (
	xsis_world.display, xsis_world.root_win,
	ghost_dev88_bits,ghost_dev88_width,ghost_dev88_height);

    XtVaGetValues (top_level, XtNwidth,&width, XtNheight,&height, NULL);
    XtTranslateCoords(top_level, (Position)(width/2), (Position)(height/2),
		      &x, &y);

    popup = XtVaCreatePopupShell("xsis alert",transientShellWidgetClass,
		top_level, XtNx,x, XtNy,y, NULL);

    dialog = xvwidget ("dialog",dialogWidgetClass, popup,
			XtNlabel, message,
			XtNicon, icon_px,
			NULL);

    XawDialogAddButton (dialog, "R.I.P.", DestroyPopupPrompt,(XtPointer)dialog);

    XtPopup (popup, XtGrabExclusive);
}

void xsis_close_window (w,closure,data)
Widget w;
XtPointer closure;
XtPointer data;
{
    /*	Callback for destroying a toplevel shell. */
    /* ARGSUSED */

    XtDestroyWidget ((Widget)closure);
}

static void xsis_eat_command ()
{
    /*	If the command is unrecognized or has an error, call eat to
	remove the rest of it from the sis graphics stream. */

    char buffer[100];
    int num_lines = 0;

    if (xsis_world.debug) printf("eating command...\n");
    while (xsis_fgets(buffer,sizeof(buffer),1) != NULL) {
	if (xsis_world.debug && ++num_lines < 6) {
	    printf("eating \"%s\"\n",buffer);
	}
    }
    if (xsis_world.debug) printf("...%d lines eaten.\n",num_lines);
}

static void xsis_cleanup ()
{
    /*	After an error from the sis child, clean up any remaining input
	procs or timers which would otherwise gobble up CPU time. */

    if (xsis_world.cmd_proc_active) {
	XtRemoveInput (xsis_world.cmd_proc_id);
	xsis_world.cmd_proc_active = False;
    }
    xsis_world.in_proc = NULL;
    if (xsis_world.cmd_timer_active) {
	XtRemoveTimeOut (xsis_world.cmd_timer_id);
	xsis_world.cmd_timer_active = False;
    }
}

/* ---------------------------- Input from sis child ----------------------- *\

   Reading from the sis pty is handled with an Xt input proc.  However, to
   allow the user to cancel an output command using the xsis interface, we
   must alternate between reading from sis and handling X events.  To do this,
   we process only up to a specified number of lines on an input proc call.
   But then it is not well defined if the input proc will be called again or
   not (it wasn't on the system we used for development) so if there may be
   more to read, we add an Xt timer proc to read again in a short while.  This
   should give Xt time to dispatch a click on CANCEL or an accelerator to
   abort the command.  This can also give the user a fraction of a second to
   read the output.
\* ------------------------------------------------------------------------- */

#define TIMER_DELAY	50
#define SCREEN_FULL	10

static int xsis_proc (info,cmd)
xsis_win_info* info;
String cmd;
{
    /*	Handle general xsis application commands. */
    /*  ARGSUSED */

    xsis_cmds *cmd_p;
    int status = 1;

    for (cmd_p=info->cmd_list; status && cmd_p->cmd_name != NULL; cmd_p++) {
	if (!strcmp(cmd_p->cmd_name, cmd)) {
	    (*cmd_p->cmd_proc) (info->shell_info);
	    status = 0;
	}
    }

    if (status == 1) {
	if (!strcmp(cmd,"close")) {
	    XtDestroyWidget (info->shell);
	    xsis_eat_command ();
	    status = 0;
	}
    }

    return status;
}

static int xsis_new_window (type,title)
String type;
String title;
{
    xsis_cmds *cmd_p;

    for (cmd_p=window_creators; cmd_p->cmd_name != NULL; cmd_p++) {
	if (!strcmp(cmd_p->cmd_name, type)) {
	    (*cmd_p->cmd_proc) ((XtPointer)title);
	    return 0;
	}
    }

    if (xsis_world.debug) {
	printf("xsis: don't know how to create type %s.\n",type);
    }
    return 1;
}

static int xsis_cmd_shell (type,title,cmd)
String type;
String title;
String cmd;
{
    xsis_win_info *info;
    int status = 1;
    lsGen gen;

    if (xsis_world.debug) {
	printf("gfx: type %s, title %s, cmd %s.\n",type,title,cmd);
    }

    if (!strcmp(cmd,"new")) {
	return xsis_new_window (type,title);
    }

    xsis_foreach_shell (&xsis_world,gen,info) {
	if (!strcmp(info->type,type) && !strcmp(info->title,title)) {
	    status = xsis_proc (info,cmd);
	}
    }

    return status;
}

static void xsis_filter (bufin)
String bufin;
{
    /*	Filter an input line for the special graphics start sequence.  This
	assumes the first character of COM_GRAPHICS_MSG_START is unique in
	the string, to make matching easier. */
	
    char buffer[256], *shell_type, *title, *command;
    char *cmd_status, *first_g, *target;
    char *start_msg = COM_GRAPHICS_MSG_START;
    int first_g_len, target_len;

    if (xsis_world.filter_index == 0) {
	first_g = strrchr (bufin,start_msg[0]);
	if (first_g == NULL) {
	    xcmd_puts (bufin);
	    return;
	} else if (first_g != bufin) {
	    /* This may be part text, part command. */
	    *first_g = '\0';
	    xcmd_puts (bufin);
	    *first_g = start_msg[0];
	}
    } else {
	first_g = bufin;
    }

    target = start_msg + xsis_world.filter_index;
    first_g_len = strlen(first_g);
    target_len = strlen(target);

    if (first_g_len < target_len && !strncmp(first_g,target,first_g_len)) {
	xsis_world.filter_index += first_g_len;
    }
    else if (first_g_len == target_len && !strcmp(first_g,target)) {
	xsis_world.n_gfx_cmd++;
	xsis_world.filter_index = 0;
	/* Set blocked mode for safe transfer. */
	xsis_tty_block (xsis_world.sis_pty,1);
	cmd_status = xsis_fgets (buffer,sizeof(buffer),0);
	if (cmd_status != NULL) {
	    shell_type = strtok(buffer,"\t");
	    command = strtok(NULL,"\t");
	    title = strtok(NULL,"\n");
	    cmd_status = title;
	}
	xsis_world.input_status = XSIS_READING_GRAPHICS;
	if (cmd_status==NULL || xsis_cmd_shell (shell_type,title,command)) {
	    if (xsis_world.debug) {
		xsis_dump_string("Unknown graphics command",buffer);
	    }
	    xsis_eat_command ();
	}
	/* Restore unblocked mode for input. */
	xsis_tty_block (xsis_world.sis_pty,0);
	xsis_world.input_status = XSIS_UNBLOCKED;
    }
    else {
	/* No more match and any earlier match was just output too. */
	if (xsis_world.filter_index != 0) {
	    buffer[0] = '\0';
	    strncat(buffer,start_msg,xsis_world.filter_index);
	    xcmd_puts (buffer);
	    xsis_world.filter_index = 0;
	}
	xcmd_puts (first_g);
    }
}

static void xsis_pty_error ()
{
    /*	Handle error on a filtered read.  Status EAGAIN or EWOULDBLOCK on an
	unblocked read means no more data at this time.  If the sis child
	process has already quit, child_status should already be set to a
	negative value (see catch_sigcld in main.c).  If debugging is not on,
	we exit directly from here, since the child is already gone, and it
	is hard to exit the Xt main loop without a graphics event. */

    if (xsis_world.debug) {
	printf("read=");
	if (errno == EAGAIN)
	    printf("EAGAIN");
	else if (errno == EWOULDBLOCK)
	    printf("EWOULDBLOCK");
	else
	    printf("%d",errno);
	printf(" (%d)\n",xsis_world.child_status);
    }

    /* Under the following conditions, all is okay. */
    if (errno == EAGAIN || errno == EWOULDBLOCK) return;
    if (errno == 0 && xsis_world.child_status >= 0) return;

    xsis_world.keep_running = False;
    if (xsis_world.child_status == XSIS_KILLED) {
	char bufin[100];
	sprintf(bufin,"SIS was killed\nby signal %d.\n",
	    xsis_world.exit_code);
	xsis_world.keep_running = True;
	xsis_alert (bufin);
    } else {
	if (xsis_world.child_status >= 0 && errno != 0 && errno != EIO) {
	    xsis_perror ("read");
	}
	exit(xsis_world.exit_code);
    }
    xsis_cleanup ();
}

static Boolean xsis_read_child ()
{
    /*	Called when input may be available from the sis child process.
	Limit the amount of data processed at one time to give Xt time to
	attend to the user.  Returns True if there may still be more data
	to read. */

    char bufin[1000];
    int n_lines;
    Boolean more = True;

    for (n_lines=0; more && n_lines < SCREEN_FULL; n_lines++) {
	bufin[0] = '\0';
        if (xsis_fgets(bufin,sizeof(bufin),0) != NULL) {
	    if (xsis_world.debug) xsis_dump_string("sis_pty",bufin);
	    xsis_filter (bufin);
	}
	else {
	    more = False;
	    xsis_pty_error ();
	}
    }

    return more;
}

static void xsis_timer_proc (client_data,id)
XtPointer client_data;
XtIntervalId* id;
{
    /*	Process any additional input from sis child.  If more is still
	available after that, reschedule another timer, otherwise restore
	the input proc.  Timer interval is in milliseconds. */

    /* ARGSUSED */
    xsis_world.cmd_timer_active = False;

    if (xsis_read_child ()) {
	xsis_world.cmd_timer_active = True;
	XtAppAddTimeOut (xsis_world.app_context,TIMER_DELAY,
				xsis_timer_proc,client_data);
    } else if (xsis_world.in_proc != NULL) {
	xsis_world.cmd_proc_id = XtAppAddInput (
		xsis_world.app_context,xsis_world.sis_pty,
		(XtPointer)XtInputReadMask,xsis_world.in_proc,(XtPointer)NULL);
	xsis_world.in_proc = NULL;
	xsis_world.cmd_proc_active = True;
    }
}

static void xsis_input_proc (client_data,src,id)
XtPointer client_data;
int* src;
XtInputId* id;
{
    /*	Called when input arrives on pty from sis child.  If not all the
	input can be conveniently handled immediately, the input proc is
	removed and a timer proc is added to check again soon.  This gives
	time to process Xt events even during long output sequences. */

    /* ARGSUSED */

    if (xsis_read_child ()) {
	XtRemoveInput (xsis_world.cmd_proc_id);
	xsis_world.cmd_proc_active = False;
	xsis_world.in_proc = xsis_input_proc;
	xsis_world.cmd_timer_id = XtAppAddTimeOut (
		xsis_world.app_context,TIMER_DELAY,
		xsis_timer_proc,client_data);
	xsis_world.cmd_timer_active = True;
    }
}

static void xsis_del_info (data)
lsGeneric data;
{
    xsis_win_info *info = (xsis_win_info*) data;
    xfree (info->type);
    xfree (info->title);
    xfree (info);
}

static void xsis_rm_shell (w,closure,data)
Widget w;
XtPointer closure;
XtPointer data;
{
    /*	Add this to destroyCallback list of all shell widgets.  It
	removes the shell from the xsis list of shells. */
    /* ARGSUSED */

    xsis_win_info *info;
    lsGeneric ls_data;
    lsGen gen;

    xsis_foreach_shell (&xsis_world,gen,info) {
	if (info->shell == w) {
	    lsDelBefore (gen,&ls_data);
	    xsis_del_info (ls_data);
	    return;
	}
    }

    printf("Lost shell %s\n", XtName(w));
}

void xsis_add_shell (title,w,type,cmd_list,info)
String title;			/*i Shell title.		*/
Widget w;			/*i New shell widget.		*/
char *type;			/*i Shell type string.		*/
xsis_cmds *cmd_list;		/*u Array of graphics commands.	*/
XtPointer info;			/*u Shell-specific data.	*/
{
    /*	Add a new shell to the xsis shell list, realize it and
	pop it up as a modeless top level shell. */

    xsis_win_info *new_info = XtNew (xsis_win_info);

    new_info->shell = w;
    new_info->type = XtNewString(type);
    new_info->title = XtNewString(title);

    if (w != NULL) {
	XtAddCallback (w,XtNdestroyCallback,xsis_rm_shell,(XtPointer)NULL);
	XtRealizeWidget (w);
	/* Can we do this for the application shell without damage? */
	XtPopup (w, XtGrabNone);
    }

    new_info->cmd_list = cmd_list;
    new_info->shell_info = info;
    lsNewBegin (xsis_world.shell_list, (lsGeneric) new_info, LS_NH);
    if (xsis_world.debug) printf("add shell %s, %s\n",type,new_info->title);
}

XtPointer xsis_shell_info (w)
Widget w;
{
    /*	Find the information for the specified widget. */

    xsis_win_info *info;
    lsGen gen;
    Widget toplevel = w;

    while (!XtIsShell(toplevel)) {
	toplevel = XtParent (toplevel);
    }

    xsis_foreach_shell (&xsis_world,gen,info) {
	if (info->shell == toplevel) {
	    return info->shell_info;
	}
    }
    return NULL;
}

XtPointer xsis_find_shell (type,title)
String type;
String title;
{
    /*	Return the shell with given title, or NULL if not found. */

    xsis_win_info *info;
    lsGen gen;

    xsis_foreach_shell (&xsis_world,gen,info) {
	if (!strcmp(info->type,type) && !strcmp(info->title,title)) {
	    return info->shell_info;
	}
    }
    return NULL;	/* not found */
}

static void xsis_do_add_cmd (closure)
XtPointer closure;
{
    xsis_info *world = (xsis_info*) closure;
    char *line;

    xsis_foreach_line (world,line) {
	array_insert_last (String,world->sis_commands,XtNewString(line));
    }
}

char *xsis_get_flag (flag)
String flag;
{
    char *value = "";
    if (!st_lookup (xsis_world.set_values,flag,&value)) {
	if (!strcmp(flag,"prompt")) {
	    value = "sis> ";
	}
    }
    return value;
}

void xsis_set_flag (flag,value)
String flag;
String value;
{
    /*	Store flag/value pair for later lookup using xsis_get_flag. */
    st_insert (xsis_world.set_values,XtNewString(flag),XtNewString(value));
}

static void xsis_do_set (closure)
XtPointer closure;
{
    xsis_info *world = (xsis_info*) closure;
    char *p, *item, *value;

    xsis_foreach_line (world,p) {
	item = strtok (p,"\t");
	value = strtok (NULL,"");
	if (value == NULL) continue;
	if (world->debug) printf ("set \"%s\" to \"%s\"\n",item,value);
	xsis_set_flag (item,value);
    }
}

static xsis_cmds xsis_cmd_list[] = {

    { "set",			xsis_do_set },
    { "commands",		xsis_do_add_cmd },
    { NULL }

};

int xsis_main (argc,argv)
int argc;
char** argv;
{
    XEvent event;
    Pixmap icon_px;
    int i;
    xsis_info *world = &xsis_world;

    world->app_shell =
	XtVaAppInitialize (
	    &(world->app_context), "XSis",
	    xsis_options, XtNumber(xsis_options),
	    &argc, argv, xsis_fallback_resources, NULL
	);

    XtGetApplicationResources (world->app_shell,
	world,app_resources,XtNumber(app_resources),NULL,ZERO);

    world->display = XtDisplay (world->app_shell);
    world->root_win = RootWindowOfScreen(XtScreen(world->app_shell));
    world->keep_running = True;
    world->buflen = 256;
    world->buffer = xalloc (char,world->buflen);

    if (world->debug) {		/* Print acceptable icon sizes. */
	XIconSize *sizes;
	int sc;
	printf("pty: master=%s, slave=%s\n",
		world->pty_master_name,world->pty_slave_name);

	if (XGetIconSizes (world->display,world->root_win,&sizes,&sc) != 0) {
	    while (sc--) {
		printf("icon: width %d-%d by %d, height %d-%d by %d\n",
		sizes[sc].min_width,sizes[sc].max_width,sizes[sc].width_inc,
		sizes[sc].min_height,sizes[sc].max_height,sizes[sc].height_inc);
	    }
	    XFree ((caddr_t)sizes);
	}
    }

    icon_px = XCreateBitmapFromData (
	world->display, world->root_win,
	sis50_bits,sis50_width,sis50_height);
    XtVaSetValues (world->app_shell, XtNiconPixmap, icon_px, NULL);

    world->shell_list = lsCreate();
    world->n_gfx_cmd = 0;
    world->sis_commands = array_alloc (String,100);
    world->set_values = st_init_table (strcmp,st_strhash);

    if (argc != 1) {
	printf("%s: unrecognized option%signored: ",argv[0],argc==1?" ":"s ");
	for (i=1; i < argc; i++) printf(" %s",argv[i]);
	printf("\n");
    }

    if (world->debug) {
	XSynchronize (world->display,True);
    }
    xsis_add_shell (XSIS_SIS,NULL,XSIS_SIS,xsis_cmd_list,(XtPointer)world);

    /*	Set up an input proc to watch for data from the sis child over
	the sis pty.  Use unblocked reads since most output from sis
	is unformatted and unpredictable. */

    world->in_proc = NULL;
    world->cmd_proc_id = XtAppAddInput (
	    world->app_context, world->sis_pty,
	    (XtPointer)XtInputReadMask, xsis_input_proc, (XtPointer)NULL);
    world->cmd_proc_active = True;
    world->cmd_timer_active = False;
    world->filter_index = 0;
    world->input_status = XSIS_UNBLOCKED;

    while (world->keep_running) {		/* Custom XtAppMainLoop. */
	XtAppNextEvent (world->app_context, &event);
	XtDispatchEvent (&event);
    }

    if (world->debug) {				/* Free memory. */
	printf("freeing data\n");
	for (i=array_n(world->sis_commands); i--; ) {
	    xfree (array_fetch(String,world->sis_commands,i));
	}
	array_free (world->sis_commands);
	lsDestroy (world->shell_list,xsis_del_info);
	XtDestroyApplicationContext (world->app_context);
	printf("exit xsis_main\n");
    }

    return 0;
}
