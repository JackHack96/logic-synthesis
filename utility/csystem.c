/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/utility/csystem.c,v $
 * $Author: pchong $
 * $Revision: 1.2 $
 * $Date: 2004/03/14 17:14:14 $
 *
 */
/* LINTLIBRARY */
#include "copyright.h"
#include "port.h"
#include <sys/wait.h>
#include "utility.h"
#include "config.h"

int
util_csystem(s)
char *s;
{
    RETSIGTYPE (*istat)(), (*qstat)();
#if defined(_IBMR2) || defined(__osf__)
    int status;    
#else
    union wait status;
#endif
    int pid, w, retval;

    if ((pid = vfork()) == 0) {
	(void) execl("/bin/csh", "csh", "-f", "-c", s, 0);
	(void) _exit(127);
    }

    /* Have the parent ignore interrupt and quit signals */
    istat = signal(SIGINT, SIG_IGN);
    qstat = signal(SIGQUIT, SIG_IGN);

    while ((w = wait(&status)) != pid && w != -1)
	    ;
    if (w == -1) {		/* check for no children ?? */
	retval = -1;
    } else {
#if defined(_IBMR2) || defined(__osf__)
	retval = status;
#else
	retval = status.w_status;
#endif
    }

    /* Restore interrupt and quit signal handlers */
    (void) signal(SIGINT, istat);
    (void) signal(SIGQUIT, qstat);
    return retval;
}
