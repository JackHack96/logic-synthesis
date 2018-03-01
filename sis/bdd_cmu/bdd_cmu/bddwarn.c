/* BDD error and argument checking routines */


#include <stdio.h>
#if defined(__STDC__)
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include "bddint.h"


#if defined(__STDC__)
extern void exit(int);
#else
extern void exit();
#endif


/* cmu_bdd_warning(message) prints a warning and returns. */

void
#if defined(__STDC__)
cmu_bdd_warning(char *message)
#else
cmu_bdd_warning(message)
     char *message;
#endif
{
  fprintf(stderr, "BDD library: warning: %s\n", message);
}


/* cmu_bdd_fatal(message) prints an error message and exits. */

void
#if defined(__STDC__)
cmu_bdd_fatal(char *message)
#else
cmu_bdd_fatal(message)
     char *message;
#endif
{
  fprintf(stderr, "BDD library: error: %s\n", message);
  exit(1);
  /* NOTREACHED */
}


int
#if defined(__STDC__)
bdd_check_arguments(int count, ...)
{
  int all_valid;
  va_list ap;
  bdd f;

  va_start(ap, count);
#else
bdd_check_arguments(va_alist)
     va_dcl
{
  int count;
  int all_valid;
  va_list ap;
  bdd f;

  va_start(ap);
  count=va_arg(ap, int);
#endif
  all_valid=1;
  while (count)
    {
      f=va_arg(ap, bdd);
      {
	BDD_SETUP(f);
	if (!f)
	  all_valid=0;
	else if (BDD_REFS(f) == 0)
	  cmu_bdd_fatal("bdd_check_arguments: argument has zero references");
      }
      --count;
    }
  return (all_valid);
}


void
#if defined(__STDC__)
bdd_check_array(bdd *fs)
#else
bdd_check_array(fs)
     bdd *fs;
#endif
{
  while (*fs)
    {
      bdd_check_arguments(1, *fs);
      ++fs;
    }
}
