/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/utility/cpu_time.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:13 $
 *
 */
/* LINTLIBRARY */
#include "copyright.h"
#include "port.h"
#include "utility.h"

#ifdef IBM_WATC		/* IBM Waterloo-C compiler (same as bsd 4.2) */
#ifndef BSD
#define BSD
#endif
#ifndef void
#define void int
#endif
#endif

#ifdef ultrix
#ifndef BSD
#define BSD
#endif
#endif

#ifdef __hpux	/* HP/UX - system dependent clock speed */
#ifndef UNIXMULTI
#define UNIXMULTI
#endif
#endif

#ifdef aiws
#ifndef UNIX10
#define UNIX10
#endif
#endif

#ifdef vms		/* VAX/C compiler -- times() with 100 HZ clock */
#ifndef UNIX100
#define UNIX100
#endif
#endif

/* default */
#if !defined(BSD) && !defined(UNIX10) && !defined(UNIX60) && !defined(UNIX100) && !defined(UNIX50) && !defined(UNIXMULTI)
#define BSD
#endif

#ifdef BSD
#include <sys/time.h>
#include <sys/resource.h>
#endif

#ifdef UNIX10
#include <sys/times.h>
#endif

#ifdef UNIX50
#include <sys/times.h>
#endif

#ifdef UNIX60
#include <sys/times.h>
#endif

#ifdef UNIX100
#include <sys/times.h>
#endif

#ifdef UNIXMULTI
/* The following code for HPUX is based on a man page that claims POSIX  */
/* compliance.  Accordingly, it should work on any POSIX-compliant       */
/* system.  It should definitely work for HPUX 8.07 and beyond.  I can't */
/* vouch for pre-8.07 systems, but I recall that earlier versions of     */
/* HPUX (like 6.5) had a different way of accessing the clock rate for   */
/* times().  - DEW                                                       */

#include <sys/times.h>
#include <unistd.h>
#endif

/*
 *   util_cpu_time -- return a long which represents the elapsed processor
 *   time in milliseconds since some constant reference
 */
long 
util_cpu_time()
{
    long t = 0;

#ifdef BSD
    struct rusage rusage;
    (void) getrusage(RUSAGE_SELF, &rusage);
    t = (long) rusage.ru_utime.tv_sec*1000 + rusage.ru_utime.tv_usec/1000;
#endif

#ifdef IBMPC
    long ltime;
    (void) time(&ltime);
    t = ltime * 1000;
#endif

#ifdef UNIX10			/* times() with 10 Hz resolution */
    struct tms buffer;
    (void) times(&buffer);
    t = buffer.tms_utime * 100;
#endif

#ifdef UNIX50			/* times() with 50 Hz resolution */
    struct tms buffer;
    times(&buffer);
    t = buffer.tms_utime * 20;
#endif

#ifdef UNIX60			/* times() with 60 Hz resolution */
    struct tms buffer;
    times(&buffer);
    t = buffer.tms_utime * 16.6667;
#endif

#ifdef UNIX100
    struct tms buffer;		/* times() with 100 Hz resolution */
    times(&buffer);
    t = buffer.tms_utime * 10;
#endif

#ifdef UNIXMULTI      /* times() with processor-dependent resolution (POSIX) */
    struct tms buffer;
    long nticks;                /* number of clock ticks per second */

    nticks = sysconf(_SC_CLK_TCK);
    times(&buffer);
    t = buffer.tms_utime * (1000.0/nticks);

#endif
    return t;
}
