/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/util/stub.c,v $
 * $Author: pchong $
 * $Revision: 1.2 $
 * $Date: 2004/03/13 08:31:21 $
 *
 */
/* LINTLIBRARY */

#include "config.h"

#if !HAVE_MEMCPY
char *
memcpy(s1, s2, n)
char *s1, *s2;
int n;
{
    extern bcopy();
    bcopy(s2, s1, n);
    return s1;
}
#endif

#if !HAVE_MEMSET
char *
memset(s, c, n)
char *s;
int c;
int n;
{
    extern bzero();
    register int i;

    if (c == 0) {
	bzero(s, n);
    } else {
	for(i = n-1; i >= 0; i--) {
	    *s++ = c;
	}
    }
    return s;
}
#endif

#if !HAVE_STRCHR
char *
strchr(s, c)
char *s;
int c;
{
    extern char *index();
    return index(s, c);
}
#endif

#if !HAVE_STRRCHR
char *
strrchr(s, c)
char *s;
int c;
{
    extern char *rindex();
    return rindex(s, c);
}
#endif

#include <stdio.h>

#if !HAVE_POPEN
/*ARGSUSED*/
FILE *
popen(string, mode)
char *string;
char *mode;
{
    (void) fprintf(stderr, "popen not supported on your operating system\n");
    return NULL;
}
#endif

#if !HAVE_PCLOSE
/*ARGSUSED*/
int
pclose(fp)
FILE *fp;
{
    (void) fprintf(stderr, "pclose not supported on your operating system\n");
    return -1;
}
#endif

/* put something here in case some compilers abort on empty files ... */
util_do_nothing()
{
    return 1;
}
