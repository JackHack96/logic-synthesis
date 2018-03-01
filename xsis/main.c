/* -------------------------------------------------------------------------- *\
   main.c -- handle fork/exec for xsis/sis pair.
   
	$Revision: 1.1.1.1 $
	$Date: 2004/02/07 10:14:14 $
	$Author: pchong $
	$Source: /users/pchong/CVS/sis/xsis/main.c,v $

   Copyright 1991 by the Regents of the University of California.

   All rights reserved.  Permission to use, copy, modify and distribute
   this software is hereby granted, provided that the above copyright
   notice and this permission notice appear in all copies.  This software
   is made available as is, with no warranties.

   xsis runs a copy of sis as a child process.  A pty (pseudo-tty) is used to
   connect to sis stdin/stdout/stderr so that ^D works as EOF the same as for
   a tty, and so that library calls which set terminal characteristics will
   work.  Since output from sis is unpredictable, xsis reads the pty in
   unblocked mode until a graphics command is found.  In this configuration,
   xsis is the master (device) and sis is the slave.

   Porting this to System-V environments will require some work for the
   system-dependent file and process control.  There are several terminal
   interface definitions available -- xsis uses the termios(4) interface
   defined by the IEEE POSIX standard P1003.1.

   We clear the controlling terminal so programs like more(1) work correctly
   within xsis.  In general, the tcgetattr/tcsetattr calls only work on the
   slave side of the pty and not on the controller side.  So xsis cannot tell
   when sis is reading in raw mode for example.
\* -------------------------------------------------------------------------- */

#ifndef MAKE_DEPEND
#include <fcntl.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#ifndef TIOCNOTTY
# include <sys/ioctl.h>
#endif
#endif /* MAKE_DEPEND */

/* Under DEC OSF1, TIOCNOTTY is defined in <sys/ioctl.h>.  However, under
   SunOS 4.1.1, it is defined by including <fcntl.h> and further, including
   both <fcntl.h> and <sys/ioctl.h> causes errors with several dozen
   identifiers multiply defined (and defined differently).  So <sys/ioctl.h>
   is conditionally included if TIOCNOTTY is not already defined.  If you
   know of a better kludge, please let us know. */

typedef int xsis_pstat;		/* Default type of 2nd arg. to waitpid. */

#if defined(ultrix) && defined(mips)
  /* Other termios interfaces use ECHOCTL for this parameter. */
# define ECHOCTL TCTLECH
  /* Use a more efficient fork when readily available. */
# define fork vfork
# define xsis_pstat union wait
  /* The POSIX nonblocking I/O flag doesn't work under Ultrix 4.2. */
# undef  O_NONBLOCK
# define O_NONBLOCK O_NDELAY
#endif

#include "xsis.h"

static pid_t	xsis_child_pid;		/* sis child PID.	*/


static void catch_sigcld ()
{
    /*	Update global status when a child change is signaled.  If you get
	error messages for lines using the status parameter, check that
	the xsis_pstat type defined above is correct for the waitpid(2) call
	on your system. */

    xsis_pstat status;
    int sis_status;
    pid_t pid;

    if (xsis_world.debug) printf("Caught sigcld.\n");
    pid = waitpid (xsis_child_pid,&status,WNOHANG|WUNTRACED);
    if (xsis_world.debug) printf("Child PID %d changes status.\n",pid);

    if (pid > 0) {
	if (WIFEXITED(status)) {
	    sis_status = WEXITSTATUS(status);
	    if (xsis_world.debug) printf("..exit status %d\n",sis_status);
	    xsis_world.child_status = XSIS_EXITED;
	    xsis_world.exit_code = sis_status;
	    exit (sis_status);
	} else if (WIFSIGNALED(status)) {
	    sis_status = WTERMSIG(status);
	    if (xsis_world.debug) printf("..killed by signal %d\n",sis_status);
	    xsis_world.child_status = XSIS_KILLED;
	    xsis_world.exit_code = sis_status;
	} else if (WIFSTOPPED(status)) {
	    sis_status = WSTOPSIG(status);
	    if (xsis_world.debug) printf("..stopped by signal %d\n",sis_status);
	    /* Anything to do here? */
	}
    }
}

void xsis_intr_child ()
{
    /*	Send an interrupt to the child process. */

    if (xsis_world.debug) printf("kill(%d)\n",xsis_child_pid);
    if (kill (xsis_child_pid,SIGINT) != 0) {
	xsis_perror("kill");
    }
}

static int open_pty_pair (master,slave)
int* master;
int* slave;
{
    /*	Handle the messy details of opening a pty pair.  See pty(4). */

    static char pty_name[] = "/dev/pty..";
    static char tty_name[] = "/dev/tty..";
    static char altchar1[] = "pqrs";
    static char altchar2[] = "0123456789abcdef";

    int i,j;
    int failed = (-1);
 
#if !defined(SABER)
    /*	Clear controlling terminal so pty will become it. */
    {   int fdtty = open("/dev/tty",O_RDWR,0);
	if (fdtty > 0) {
	    ioctl(fdtty, TIOCNOTTY, 0);
	    close(fdtty);
	}
    }
#endif

    putenv ("TERM=xsis");
    putenv ("TERMCAP='Xaw|xsis:bs:co#80:do=^J:hc:os'");

    for (i=0; failed && i < sizeof(altchar1)/sizeof(char); i++) {
        tty_name[8] = pty_name[8] = altchar1[i];
        for (j=0; failed && j < sizeof(altchar2)/sizeof(char); j++) {
            tty_name[9] = pty_name[9] = altchar2[j];
            if (((*master)=open(pty_name,O_RDWR,0)) > -1) {
		if (((*slave)=open(tty_name,O_RDWR,0)) > -1) {
		    failed = 0;
		    xsis_world.pty_master_name = pty_name;
		    xsis_world.pty_slave_name = tty_name;
		} else {
		    close (*master);
		}
	    }
        }
    }

    return failed;
}

void xsis_tty_block (fildes,blocking)
int fildes;
int blocking;
{
    /*	Enable/disable blocking I/O on the specified file descriptor.  The
	flag O_NONBLOCK is a POSIX standard, defined for many systems.  Other
	names for it include O_NDELAY, FNBIO, and FNDELAY.  It is usually
	defined in the man page for fcntl(2). */

    int flags = fcntl(fildes,F_GETFL,0);
    if (blocking)
	flags &= ~O_NONBLOCK;
    else
	flags |=  O_NONBLOCK;
    fcntl (fildes,F_SETFL,flags);
}

int xsis_pty_eof_char (fildes)
int fildes;
{
    /*	Look up the EOF character for the pty.  Returns ^D (004) if we
	fail to get the pty information. */

    int veof = '\004';		/* default=^D */
    struct termios terminfo;
    if (tcgetattr(fildes,&terminfo) == 0) {
	veof = terminfo.c_cc[VEOF];
	if (xsis_world.debug) printf("pty eof char=%d\n",veof);
    }
    return veof;
}

int xsis_icanon_disabled (fildes)
int fildes;
{
    /*	Return 1 if canonical input processing (ICANON) is disabled. Note:
	this does not work on systems where only the slave device can make
	tcgetattr calls successfully, so we default to assume it is enabled. */

    int icanon_disabled = 0;
    struct termios terminfo;

    if (tcgetattr (fildes, &terminfo) == 0) {
	icanon_disabled = (terminfo.c_lflag & ICANON) == 0;
    }
    return icanon_disabled;
}

int main (argc,argv)
int argc;
char **argv;
{
    int		 master, slave;		/* pty file descriptors.	*/
    char	*sis_exec;		/* Path for sis executable.	*/
    pid_t	 pid = 0;		/* SIS child process id.	*/
    struct termios terminfo;		/* For setting terminal stuff.	*/
    struct stat  statrec;		/* For checking file mode.	*/

    if ((sis_exec=xsis_find_sis(argc,argv)) == NULL) {
	printf("%s: could not find \"sis\" to run.\n",argv[0]);
	return (-1);
    }

    if (stat(sis_exec,&statrec) != 0 || (statrec.st_mode&S_IFMT) != S_IFREG) {
	printf("%s: not an executable file.\n",sis_exec);
	return (-1);
    }

    if (open_pty_pair(&master,&slave) != 0) {
	printf("%s: failed to open pty.\n",argv[0]);
	return (-1);
    }

    if ((pid=fork()) == -1) {			/* ERROR */
	return xsis_perror("vfork");
    }
    else if (pid != 0) { 			/* PARENT */
	close (slave);
	xsis_child_pid = pid;
	xsis_world.sis_pty = master;
	xsis_world.exit_code = 0;
	xsis_world.child_status = XSIS_IDLE;
	xsis_tty_block (xsis_world.sis_pty,0);
	if (xsis_world.debug) printf("Starting %s, PID %d.\n",sis_exec,pid);
	signal (SIGCLD,catch_sigcld);
	xsis_main (argc,argv);
	close (master);
    }
    else { 					/* CHILD */
        close (master);
	if (tcgetattr (slave, &terminfo) != 0) {
	    xsis_perror ("tcgetattr");
	    return (-1);
	} else {
	    terminfo.c_oflag &= ~ONLCR;	/* Don't map NL on output.	*/
	    terminfo.c_oflag &= ~TAB3;	/* Don't expand tabs on output.	*/
	    terminfo.c_lflag |= ECHO;	/* Echo all input.		*/
	    terminfo.c_lflag |= ICANON;	/* Use canonical input.		*/
	    if (tcsetattr (slave, TCSADRAIN, &terminfo) != 0) {
		xsis_perror ("tcsetattr");
	    }
	}
        dup2 (slave, 0);	/* Connect slave pty to stdin,	*/
        dup2 (slave, 1);	/* stdout,			*/
        dup2 (slave, 2);	/* and stderr.			*/
        if (slave > 2) close (slave);
	execl (sis_exec, sis_exec, "-X 2", NULL);
	xsis_perror (sis_exec);
	_exit(-1);
    }

    return xsis_world.exit_code;
}
