/* -------------------------------------------------------------------------- *\
   xsis.h -- general declarations for xsis program.

	$Revision: 1.1.1.1 $
	$Date: 2004/02/07 10:14:14 $
	$Author: pchong $
	$Source: /users/pchong/CVS/sis/xsis/xsis.h,v $

   Copyright 1991 by the Regents of the University of California.

   All rights reserved.  Permission to use, copy, modify and distribute
   this software is hereby granted, provided that the above copyright
   notice and this permission notice appear in all copies.  This software
   is made available as is, with no warranties.
\* -------------------------------------------------------------------------- */

#ifndef MAKE_DEPEND
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include "util.h"
#include "array.h"
#include "list.h"
#include "st.h"

#include "X11/Xlib.h"
#include "X11/Xutil.h"
#include "X11/cursorfont.h"
#include "X11/Intrinsic.h"
#include "X11/Shell.h"
#include "X11/StringDefs.h"

#include "X11/Xaw/AsciiText.h"
#include "X11/Xaw/Box.h"
#include "X11/Xaw/Cardinals.h"
#include "X11/Xaw/Command.h"
#include "X11/Xaw/Dialog.h"
#include "X11/Xaw/Form.h"
#include "X11/Xaw/Label.h"
#include "X11/Xaw/List.h"
#include "X11/Xaw/Paned.h"
#include "X11/Xaw/Text.h"
#include "X11/Xaw/Viewport.h"
#endif /* MAKE_DEPEND */


/* Strings for communicating with a graphical front end.  These must match
   the strings used in the sis command package, com_int.h. */

#define COM_GRAPHICS_MSG_START          "#START-GRAPHICS-COMMAND\n"
#define COM_GRAPHICS_MSG_END            "#END-GRAPHICS-COMMAND\n"

#define XSIS_IDLE		(0)
#define XSIS_WORKING		(1)
#define XSIS_EXITED		(-1)
#define XSIS_KILLED		(-2)

#define xvwidget	XtVaCreateManagedWidget

/* Structures for handling file lists and file completion. */

typedef struct xsis_filec {
    String dirname;			/* Directory pathname.		*/
    String partial;			/* Partial filename.		*/
    String statpath;			/* For getting file type.	*/
    String suffix;			/* File suffix of statpath.	*/
    array_t *filenames;			/* Array of Strings.		*/
} xsis_filec;

/* xutil.c */

int xsis_perror ARGS((String));
String xsis_string_cat ARGS((String, String));
String xsis_string_trim ARGS((String));
void xsis_free_string_array ARGS((array_t *));
int xsis_string_array_len ARGS((array_t *));
int xsis_string_array_prefix ARGS((array_t *));
xsis_filec *xsis_file_completion ARGS((String, int));
String xsis_file_type ARGS((xsis_filec *, int));
void xsis_free_filec ARGS((xsis_filec *));

/* Window Information.

   xsis manages multiple top level shells: a main window for sis commands
   and text output, a separate help shell, network plot shell, etc.  To
   manage graphics commands sent to each window, xsis maintains a list of
   currently open shells and a command list for each to handle commands sent
   to that window.  The order of windows in the list is important since
   shell names need not be unique.
 */

typedef enum xsis_pty_enum {
    XSIS_UNBLOCKED,		/* Unblocked, reading general input.	*/
    XSIS_READING_GRAPHICS,	/* Blocked, reading graphics cmd.	*/
    XSIS_GRAPHICS_EOF,		/* Blocked, at end of graphics cmd.	*/
} xsis_pty_enum;

typedef void (*xsis_cmd_proc) ARGS((XtPointer));

        /* This is used to make a list of shell creation functions. */

typedef struct xsis_cmds {
    char	*cmd_name;		/* Command string.		*/
    xsis_cmd_proc cmd_proc;		/* Callback to execute it.	*/
} xsis_cmds;


typedef struct xsis_win_info {
    Widget	 shell;			/* Shell widget.		*/
    char	*type;			/* Shell type.			*/
    char	*title;			/* Name to use for this shell.	*/
    xsis_cmds	*cmd_list;		/* List of recognized commands.	*/
    XtPointer	 shell_info;		/* Passed to command proc.	*/
} xsis_win_info;


typedef struct xsis_info {		/* Shell_info for app shell.	*/
    Boolean	 debug;			/* Enable debugging output.	*/
    int		 sis_pty;		/* pty connection to sis child.	*/
    char	*pty_master_name;	/* Name of master pty side.	*/
    char	*pty_slave_name;	/* Name of slave pty side.	*/
    char	*buffer;		/* Buffer for graphics input.	*/
    int		 buflen;		/* Length of input buffer.	*/
    xsis_pty_enum input_status;		/* Status of reading sis_pty.	*/
    Boolean	 cmd_proc_active;	/* True if cmd proc installed.	*/
    XtInputId	 cmd_proc_id;		/* Input proc on sis pty.	*/
    Boolean	 cmd_timer_active;	/* True if cmd timer installed.	*/
    XtInputId	 cmd_timer_id;		/* Timeout proc on sis pty.	*/
    XtInputCallbackProc in_proc;	/* Input proc to restore.	*/
    int		 filter_index;		/* For partial match of input.	*/
    Widget	 app_shell;		/* Application top level shell.	*/
    XtAppContext app_context;		/* Used for XtApp* calls.	*/
    lsList	 shell_list;		/* Ordered list of shells.	*/
    int		 n_gfx_cmd;		/* Graphics command count.	*/
    st_table	*set_values;		/* Values from set command.	*/
    array_t	*sis_commands;		/* List of sis commands.	*/
    Display	*display;		/* Opened display.		*/
    Drawable	 root_win;		/* ID of root window.		*/
    int		 child_status;		/* Updated asynchronously.	*/
    int		 exit_code;		/* For sis exit/signal status.	*/
    Boolean	 keep_running;		/* Keep event loop going?	*/
} xsis_info;



#define xsis_foreach_line(world,p) \
	for ((p)=(world)->buffer;\
	    xsis_fgets((world)->buffer,(world)->buflen,1) != NULL; )

#define xsis_foreach_shell(world,gen,win_info)				\
        for (gen=lsStart((world)->shell_list);				\
                (lsNext(gen, (lsGeneric *) &win_info, LS_NH) == LS_OK)	\
                || ((void) lsFinish(gen), 0); )

#define xalloc(type,num) \
    ((type*)XtMalloc((Cardinal)sizeof(type)*(unsigned)(num)))
#define xrealloc(type,ptr,num) \
    ((type*)XtRealloc((char*)(ptr),(Cardinal)sizeof(type)*(unsigned)(num)))
#define xfree(p) \
    XtFree((char*)p)


    /* This needs to be global to accomodate Xt action procs, and
       system signal handler functions. */

extern xsis_info xsis_world;


/* main.c */

int main ARGS((int, char**));
int xsis_icanon_disabled ARGS((int));
void xsis_intr_child ARGS((void));
void xsis_tty_block ARGS((int,int));
int xsis_pty_eof_char ARGS((int));

/* xsis.c */

int xsis_main ARGS((int, String*));
void xsis_io_error ARGS((String));
String xsis_find_sis ARGS((int, String*));
void xsis_execute ARGS((String, int));
void xsis_dump_string ARGS((String,String));
void xsis_close_window ARGS((Widget,XtPointer,XtPointer));
void xsis_add_shell ARGS((String, Widget, String, xsis_cmds*, XtPointer));
XtPointer xsis_find_shell ARGS((String, String));
XtPointer xsis_shell_info ARGS((Widget));
void xsis_alert ARGS((String));
void xsis_set_flag ARGS((String, String));
String xsis_get_flag ARGS((String));
String xsis_fgets ARGS((String, int, int));
int xsis_get_token ARGS((String, int));
void xsis_eat_line ARGS(());

/* xcmd.c */

void xcmd_new  ARGS((XtPointer));
void xcmd_puts ARGS((String));

/* xblif.c */

void xblif_new ARGS((XtPointer));

/* xastg.c */

void xastg_new ARGS((XtPointer));

/* xhelp.c */

void xhelp_new ARGS((XtPointer));
void xhelp_popup ARGS((String, String, int));
