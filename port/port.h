
#ifndef PORT_H
#define PORT_H

#ifdef SABER
#define volatile
#endif

#ifdef _IBMR2
#define _BSD
#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE /* Argh!  IBM strikes again */
#endif
#ifndef _ALL_SOURCE
#define _ALL_SOURCE /* Argh!  IBM strikes again */
#endif
#ifndef _ANSI_C_SOURCE
#define _ANSI_C_SOURCE /* Argh!  IBM strikes again */
#endif
#endif

/*
 * int32 should be defined as the most economical sized integer capable of
 * holding a 32 bit quantity
 * int16 should be similarly defined
 */

/* XXX hack */
#ifndef MACHDEP_INCLUDED
#define MACHDEP_INCLUDED
#ifdef vax
typedef int int32;
typedef short int16;
#else
/* Ansi-C promises that these definitions should always work */
typedef long int32;
typedef int int16;
#endif /* vax */
#endif /* MACHDEP_INCLUDED */

/*
 *   Do not do anything if already somebody else is doing it.
 *  _std_h     is defined by the g++ std.h file
 *  __STDC__
 */
#if !defined(__STDC__) && !defined(_std_h)

#ifndef __DATE__
#ifdef CUR_DATE
#define __DATE__ CUR_DATE
#else
#define __DATE__ "unknown-date"
#endif /* CUR_DATE */
#endif /* __DATE__ */

#ifndef __TIME__
#ifdef CUR_TIME
#define __TIME__ CUR_TIME
#else
#define __TIME__ "unknown-time"
#endif /* CUR_TIME */
#endif /* __TIME__ */
#endif /* __STDC__ */

#ifdef sun386
#define PORTAR
#endif

#include <ctype.h>
#include <stdio.h>
#include <sys/types.h>

#undef HUGE

#include <math.h>
#include <signal.h>

#if defined(ultrix)   /* { */
#if defined(_SIZE_T_) /* { */
#define ultrix4
#else                /* } else { */
#if defined(SIGLOST) /* { */
#define ultrix3
#else /* } else { */
#define ultrix2
#endif /* } */
#endif /* } */
#endif /* } */

#if defined(ultrix3) && defined(mips)
double rint();
double trunc();
#endif

#if defined(sun) && defined(FD_SETSIZE)
#define sunos4
#else
#define sunos3
#endif

#if defined(sequent) || defined(news800)
#define LACK_SYS5
#endif

#if defined(ultrix3) || defined(sunos4) || defined(_IBMR2) ||                  \
    defined(ultrix4) || defined(__osf__)
#define SIGNAL_FN void
#else
/* sequent, ultrix2, 4.3BSD (vax, hp), sunos3 */
#define SIGNAL_FN int
#endif

/* Some systems have 'fixed' certain functions which used to be int */
#if defined(ultrix) || defined(SABER) || defined(__hpux) || defined(aiws) ||   \
    defined(apollo) || defined(AIX) || defined(__STDC__)
#define VOID_HACK void
#else
#define VOID_HACK int
#endif

#ifndef NULL
#define NULL 0
#endif /* NULL */

/*
 * CHARBITS should be defined only if the compiler lacks "unsigned char".
 * It should be a mask, e.g. 0377 for an 8-bit machine.
 */

#ifndef CHARBITS
#define UNSCHAR(c) ((unsigned char)(c))
#else
#define UNSCHAR(c) ((c)&CHARBITS)
#endif

#define SIZET int

#ifdef __STDC__
#define CONST const
#define VOIDSTAR void *
#else
#define CONST
#define VOIDSTAR char *
#endif /* __STDC__ */

/* Some machines fail to define some functions in stdio.h */
#if !defined(__STDC__) && !defined(sprite)
FILE *popen(), *tmpfile();
int pclose();
#ifndef clearerr /* is a macro on many machines, but not all */
VOID_HACK clearerr();
#endif /* clearerr */
#ifndef _IBMR2
#ifndef __osf__
#ifndef rewind
VOID_HACK rewind();
#endif /* rewind */
#endif /* __osf__ */
#endif /* _IBMR2 */
#endif /* __STDC__ */

/* most machines don't give us a header file for these */
#if defined(__STDC__) || defined(sprite) || defined(__osf__) || defined(__hpux)

#include <stdlib.h>

#if defined(__hpux)
#include <errno.h> /* For perror() defininition */
#endif             /* __hpux */
#else
#ifdef _IBMR2
int abort(), exit();
void free(), perror();
#else
VOID_HACK abort(), free(), exit(), perror();
#endif
char *getenv();
#ifdef ultrix4
void *malloc(), *realloc(), *calloc();
#else
char *malloc(), *realloc(), *calloc();
#endif
#if defined(aiws)
int sprintf();
#else
#ifndef _IBMR2
char *sprintf();
#endif
#endif
int system();
double atof();
long atol();
#ifndef _IBMR2
int sscanf();
#endif
#endif /* __STDC__ */

/* some call it strings.h, some call it string.h; others, also have memory.h */
#if defined(__STDC__) || defined(sprite)

#include <string.h>

#else
/* ANSI C string.h -- 1/11/88 Draft Standard */
#if defined(ultrix4) || defined(__hpux)
#include <strings.h>
#else
char *strcpy(), *strncpy(), *strcat(), *strncat(), *strerror();
char *strpbrk(), *strtok(), *strchr(), *strrchr(), *strstr();
int strcoll(), strxfrm(), strncmp(), strlen(), strspn(), strcspn();
char *memmove(), *memccpy(), *memchr(), *memcpy(), *memset();
int memcmp(), strcmp();
#endif /* ultrix4 */
#endif /* __STDC__ */

#ifdef lint
#undef putc /* correct lint '_flsbuf' bug */
#endif      /* lint */

/* a few extras */
#if !defined(_std_h)
#if defined(__hpux)
#define random() lrand48()
#define srandom(a) srand48(a)
#define bzero(a, b) memset(a, 0, b)
#else
#if !defined(__osf__) && !defined(__CYGWIN__)

VOID_HACK srandom();

long random();

#endif
#endif
#endif /* _std_h */

/* assertion macro */
#ifndef assert
#if defined(__STDC__) || defined(sprite)

#include <assert.h>

#else
#ifndef NDEBUG
#define assert(ex)                                                             \
  {                                                                            \
    if (!(ex)) {                                                               \
      (void)fprintf(stderr, "Assertion failed: file %s, line %d\n", __FILE__,  \
                    __LINE__);                                                 \
      (void)fflush(stdout);                                                    \
      abort();                                                                 \
    }                                                                          \
  }
#else
#define assert(ex)                                                             \
  { ; }
#endif
#endif
#endif

/* handle the various limits */
#if defined(__STDC__) || defined(POSIX)

#include <limits.h>

#else
#ifndef _IBMR2
#ifndef __osf__
#define USHRT_MAX (~(unsigned short int)0)
#define UINT_MAX (~(unsigned int)0)
#define ULONG_MAX (~(unsigned long int)0)
#define SHRT_MAX ((short int)(USHRT_MAX >> 1))
#define INT_MAX ((int)(UINT_MAX >> 1))
#define LONG_MAX ((long int)(ULONG_MAX >> 1))
#endif
#endif
#endif

/*
 *
 */

#ifdef sequent
#define SIG_FLAGS(s) (s).sv_onstack
#else
#define SIG_FLAGS(s) (s).sv_flags
#endif

#endif /* PORT_H */