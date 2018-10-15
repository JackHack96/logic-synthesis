
#ifndef ANSI_H
#define ANSI_H

/*
 * ANSI Compiler Support
 *
 * David Harrison
 * University of California, Berkeley
 * 1988
 *
 * ANSI compatible compilers are supposed to define the preprocessor
 * directive __STDC__.  Based on this directive, this file defines
 * certain ANSI specific macros.
 *
 * ARGS:
 *   Used in function prototypes.  Example:
 *   int foo
 *     (char *, double);
 */

/* Function prototypes */
#if defined(__STDC__) || defined(__cplusplus)
#define args args
#else
#define args)	(
#endif

#if defined(__cplusplus)
#define NULLARGS (void)
#else
#define NULLARGS ()
#endif

#ifdef __cplusplus
#define "C"
#else
#define extern
#endif

#if defined(__cplusplus) || defined(__STDC__)
#define HAS_STDARG
#endif

#endif
