
/* LINTLIBRARY */

#include <stdio.h>
#include "util.h"

#ifdef IBM_WATC		/* IBM Waterloo-C compiler (same as bsd 4.2) */
#define void int
#define BSD_SIS
#endif

#ifdef __hpux		/* HPUX/C compiler -- times() w/system-dependent clk */
#undef BSD_SIS

/* The following code for HPUX is based on a man page that claims POSIX  */
/* compliance.  Accordingly, it should work on any POSIX-compliant       */
/* system.  It should definitely work for HPUX 8.07 and beyond.  I can't */
/* vouch for pre-8.07 systems, but I recall that earlier versions of     */
/* HPUX (like 6.5) had a different way of accessing the clock rate for   */
/* times().  - DEW                                                       */

#include <sys/times.h>
#include <unistd.h>
#endif

#ifdef vms		/* VAX/C compiler -- times() with 100 HZ clock */
#define UNIX100
#endif

#ifdef BSD_SIS
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#endif

#ifdef UNIX50
#include <sys/types.h>
#include <sys/times.h>
#endif

#ifdef UNIX60
#include <sys/types.h>
#include <sys/times.h>
#endif

#ifdef UNIX100
#include <sys/types.h>
#include <sys/times.h>
#endif



/*
 *   util_cpu_time -- return a long which represents the elapsed processor
 *   time in milliseconds since some constant reference
 */
long 
util_cpu_time()
{
    long t = 0;

#ifdef BSD_SIS
    struct rusage rusage;
    (void) getrusage(RUSAGE_SELF, &rusage);
    t = (long) rusage.ru_utime.tv_sec*1000 + rusage.ru_utime.tv_usec/1000;
#endif

#ifdef IBMPC
    long ltime;
    (void) time(&ltime);
    t = ltime * 1000;
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

#ifdef __hpux         /* times() with processor-dependent resolution (POSIX) */
    struct tms buffer;
    long nticks;                /* number of clock ticks per second */

    nticks = sysconf(_SC_CLK_TCK);
    times(&buffer);
    t = buffer.tms_utime * (1000.0/nticks);
    
#endif

#ifdef vms			/* VAX/C compiler - times() with 100 HZ clock */
    struct {int p1, p2, p3, p4;} buffer;
    static long ref_time;
    times(&buffer);
    t = buffer.p1 * 10;
    if (ref_time == 0)
      ref_time = t;
    t = t - ref_time;
#endif


    return t;
}
