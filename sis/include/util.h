#ifndef UTIL_H
#define UTIL_H

#include <unistd.h>

/* for CUDD package */
#if SIZEOF_VOID_P == 8 && SIZEOF_INT == 4
typedef long util_ptrint;
#else
typedef int util_ptrint;
#endif

/* This was taken out and defined at compile time in the SIS Makefile
   that uses the OctTools.  When the OctTools are used, USE_MM is defined,
   because the OctTools contain libmm.a.  Otherwise, USE_MM is not defined,
   since the mm package is not distributed with SIS, only with Oct. */

/* #define USE_MM */ /* choose libmm.a as the memory allocator */

/**
 * Returns 0 properly casted into a pointer to an object of type
 * 'type'.  Strictly speaking, this macro is only required when
 * a 0 pointer is passed as an argument to a function.  Still,
 * some prefer the style of always casting their 0 pointers using
 * this macro.
 */
#define NIL(type) ((type *)0)

#ifdef USE_MM
/*
 *  assumes the memory manager is libmm.a
 *	- allows malloc(0) or realloc(obj, 0)
 *	- catches out of memory (and calls MMout_of_memory())
 *	- catch free(0) and realloc(0, size) in the macros
 */
#define ALLOC(type, num) ((type *)malloc(sizeof(type) * (num)))
#define REALLOC(type, obj, num)                                                \
  (obj) ? ((type *)realloc((char *)obj, sizeof(type) * (num)))                 \
        : ((type *)malloc(sizeof(type) * (num)))
#define FREE(obj) ((obj) ? (free((char *)(obj)), (obj) = 0) : 0)
#else
/*
 *  enforce strict semantics on the memory allocator
 *	- when in doubt, delete the '#define USE_MM' above
 */
/**
 * Allocates 'number' objects of type 'type'.  This macro should be
 * used rather than calling malloc() directly because it casts the
 * arguments appropriately, and ALLOC() will never return NIL(char),
 * choosing instead to terminate the program.
 */
#define ALLOC(type, num) ((type *)MMalloc((long)sizeof(type) * (long)(num)))

/**
 * Re-allocate 'obj' to hold 'number' objects of type 'type'.
 * This macro should be used rather than calling realloc()
 * directly because it casts the arguments appropriately, and
 * REALLOC() will never return NIL(char), instead choosing to
 * terminate the program.  It also guarantees that REALLOC(type, 0, n)
 * is the same as ALLOC(type, n).
 */
#define REALLOC(type, obj, num)                                                \
  ((type *)MMrealloc((char *)(obj), (long)sizeof(type) * (long)(num)))

/**
 * Free object 'obj'.  This macro should be used rather than
 * calling free() directly because it casts the argument
 * appropriately.  It also guarantees that FREE(0) will work
 * properly.
 */
#define FREE(obj) ((obj) ? (free((void *)(obj)), (obj) = 0) : 0)
#endif

/* Ultrix (and SABER) have 'fixed' certain functions which used to be int */
#if defined(ultrix) || defined(SABER) || defined(aiws) || defined(__hpux) ||   \
    defined(__STDC__) || defined(apollo)
#define VOID_HACK void
#else
#define VOID_HACK int
#endif

/* No machines seem to have much of a problem with these */
#include <ctype.h>
#include <stdio.h>

/* Some machines fail to define some functions in stdio.h */
#if !defined(__STDC__) && !defined(sprite) && !defined(_IBMR2) &&              \
    !defined(__osf__)
FILE *popen(), *tmpfile();
int pclose();
#ifndef clearerr /* is a macro on many machines, but not all */
VOID_HACK clearerr();
#endif
#ifndef rewind
VOID_HACK rewind();
#endif
#endif

#ifndef PORT_H

#include <signal.h>
#include <sys/types.h>

#if defined(ultrix)
#if defined(_SIZE_T_)
#define ultrix4
#else
#if defined(SIGLOST)
#define ultrix3
#else
#define ultrix2
#endif
#endif
#endif
#endif

/* most machines don't give us a header file for these */
#if defined(__STDC__) || defined(sprite) || defined(_IBMR2) ||                 \
    defined(__osf__) || defined(sunos4) || defined(__hpux)

#include <stdlib.h>

#if defined(__hpux)
#include <errno.h> /* For perror() defininition */
#endif             /* __hpux */
#else
VOID_HACK abort(), free(), exit(), perror();
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
#endif

#ifndef PORT_H
#if defined(ultrix3) || defined(sunos4) || defined(_IBMR2) || defined(__STDC__)
#define SIGNAL_FN void
#else
/* sequent, ultrix2, 4.3BSD (vax, hp), sunos3 */
#define SIGNAL_FN int
#endif
#endif

/* some call it strings.h, some call it string.h; others, also have memory.h */
#if defined(__STDC__) || defined(sprite)

#include <string.h>

#else
#if defined(ultrix4) || defined(__hpux)
#include <strings.h>
#else
#if defined(_IBMR2) || defined(__osf__)
#include <string.h>
#include <strings.h>
#else
/* ANSI C string.h -- 1/11/88 Draft Standard */
/* ugly, awful hack */
#ifndef PORT_H
char *strcpy(), *strncpy(), *strcat(), *strncat(), *strerror();
char *strpbrk(), *strtok(), *strchr(), *strrchr(), *strstr();
int strcoll(), strxfrm(), strncmp(), strlen(), strspn(), strcspn();
char *memmove(), *memccpy(), *memchr(), *memcpy(), *memset();
int memcmp(), strcmp();
#endif
#endif
#endif
#endif

/* a few extras */
#if defined(__hpux)
#define random() lrand48()
#define srandom(a) srand48(a)
#define bzero(a, b) memset(a, 0, b)
#else
#if !defined(__osf__) && !defined(linux) && !defined(__CYGWIN__)
/* these are defined as macros in stdlib.h */
VOID_HACK srandom();
long random();
#endif
#endif

#if defined(__STDC__) || defined(sprite)

#include <assert.h>

#else
#ifndef NDEBUG
#define assert(ex)                                                             \
  {                                                                            \
    if (!(ex)) {                                                               \
      (void)fprintf(stderr, "Assertion failed: file %s, line %d\n\"%s\"\n",    \
                    __FILE__, __LINE__, "ex");                                 \
      (void)fflush(stdout);                                                    \
      abort();                                                                 \
    }                                                                          \
  }
#else
#define assert(ex) ;
#endif
#endif

#define fail(why)                                                              \
  {                                                                            \
    (void)fprintf(stderr, "Fatal error: file %s, line %d\n%s\n", __FILE__,     \
                  __LINE__, why);                                              \
    (void)fflush(stdout);                                                      \
    abort();                                                                   \
  }

#ifdef lint
#undef putc  /* correct lint '_flsbuf' bug */
#undef ALLOC /* allow for lint -h flag */
#undef REALLOC
#define ALLOC(type, num) (((type *)0) + (num))
#define REALLOC(type, obj, num) ((obj) + (num))
#endif

#if !defined(MAXPATHLEN)
#define MAXPATHLEN 1024
#endif

/* These arguably do NOT belong in util.h */
#ifndef ABS
#define ABS(a) ((a) < 0 ? -(a) : (a))
#endif
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef USE_MM

void MMout_of_memory(long size);

char *MMalloc(long size);

char *MMrealloc(char *obj, long size);

void MMfree(char *obj);

#endif

/**
 * Dump to the given file a summary of processor usage statistics.
 * For BSD machines, this includes a formatted dump of the
 * getrusage(2) structure.
 */
void util_print_cpu_stats(FILE *fp);

/**
 * Returns the processor time used since some constant reference
 * in milliseconds.
 */
long util_cpu_time();

/**
 * Reset getopt argument parsing to start parsing a new argc/argv pair.
 * Not available from the standard getopt(3).
 */
void util_getopt_reset();

/**
 * Also known as getopt(3) for backwards compatability.
 * Parses options from an argc/argv command line pair.
 */
int util_getopt(int argc, char *argv[], char *optstring);

/**
 * Simulate the execvp(3) semantics of searching the user's environment
 * variable PATH for an executable 'program'.  Returns the file name
 * of the first executable matching 'program' in getenv("PATH"), or
 * returns NIL(char) if none was found.  This routines uses
 * util_file_search().
 */
char *util_path_search(char *prog);

/**
 * 'path' is string of the form "dir1:dir2: ...".  Each of the
 * directories is searched (in order) for a file matching 'file'
 * in that directory.  'mode' checks that the file can be accessed
 * with read permission ("r"), write permission ("w"), or execute
 * permission ("x").  The expanded filename is returned, or
 * NIL(char) is returned if no file could be found.  The returned
 * string should be freed by the caller.  Tilde expansion is
 * performed on both 'file' and any directory in 'path'.
 */
char *util_file_search(char *file, char *path, char *mode);

/**
 * Fork (using execvp(3)) the program argv[0] with argv[1] ...
 * argv[n] as arguments.  (argv[n+1] is set to NIL(char) to
 * indicate the end of the list).  Set up two-way pipes between
 * the child process and the parent, returning file pointer
 * 'toCommand' which can be used to write information to the
 * child, and the file pointer 'fromCommand' which can be used to
 * read information from the child.  As always with unix pipes,
 * watch out for dead-locks.
 * @return Returns 1 for success, 0 if any failure occured forking the child.
 */
int util_pipefork(char **argv, FILE **toCommand, FILE **fromCommand,
                         int *pid);

/**
 * Converts a time into a (static) printable string.  Intended to
 * allow different hosts to provide differing degrees of
 * significant digits in the result (e.g., IBM 3090 is printed to
 * the millisecond, and the IBM PC usually is printed to the
 * second).
 * @return Returns a string of the form "10.5 sec".
 */
char *util_print_time(long t);

/**
 * Save the text and data segments of the current executable
 * (which is the file 'old_file') into the file 'new_file'.
 * Returns 1 for success, 0 for failure.  'old_file' is required
 * in order to preserve symbol table information for the new
 * executable.  'old_file' can be derived from argv[0] of the
 * current executable using util_path_search().  NOTE: no stack
 * information is preserved.  When the program restarts, it
 * re-enters main() with no valid stack.  This is currently highly
 * BSD-specific, but should run on most operating systems which are
 * derived from Berkeley Unix 4.2.
 */
int util_save_image(char *orig_file_name, char *save_file_name);

/**
 * Also known as strsav() for backwards compatability.
 * Returns a copy of the string 's'.
 */
char *util_strsav(char *s);

/**
 * Returns a new string corresponding to 'tilde-expansion' of the
 * given filename (see csh(1), "filename substitution").  This
 * means recognizing ~user and ~/ constructs, and inserting the
 * appropriate user's home directory.  The returned string should
 * be free'd by the caller.
 */
char *util_tilde_expand(char *fname);

char *util_tempnam(char *dir, char *pfx);

/**
 * Returns a file pointer to a temporary file.  It uses util_tempnam()
 * to determine a unique filename.  If TMPDIR is defined in the
 * environment, it uses that directory.  Otherwise, it uses the
 * directory defined by the system as P_tmpdir.  If that is not
 * defined, it uses /tmp.  The file can be written to, and subsequently
 * read from by calling rewind before reading.  The file should be
 * closed using fclose when it is no longer needed.
 */
FILE *util_tmpfile();

#define ptime() util_cpu_time()
#define print_time(t) util_print_time(t)

/* util_getopt() global variables (ack !) */
int util_optind;
char *util_optarg;

/* for CUDD package */
long getSoftDataLimit(void);

#include <math.h>

#ifndef HUGE
#define HUGE 8.9884656743115790e+307
#endif
#ifndef HUGE_VAL
#define HUGE_VAL HUGE
#endif
#ifndef MAXINT
#define MAXINT (1 << 30)
#endif

#include <stdarg.h>

#endif
