/*LINTLIBRARY*/
#include "port.h"
#ifdef LACK_SYS5
/*
 * strncmp - compare at most n characters of string s1 to s2
 */

int				/* <0 for <, 0 for ==, >0 for > */
strncmp(s1, s2, n)
CONST char *s1;
CONST char *s2;
SIZET n;
{
	register CONST char *scan1;
	register CONST char *scan2;
	register SIZET count;

	scan1 = s1;
	scan2 = s2;
	count = n;
	while (--count >= 0 && *scan1 != '\0' && *scan1 == *scan2) {
		scan1++;
		scan2++;
	}
	if (count < 0)
		return 0;

	/*
	 * The following case analysis is necessary so that characters
	 * which look negative collate low against normal characters but
	 * high against the end-of-string NUL.
	 */
	if (*scan1 == '\0' && *scan2 == '\0')
		return 0;
	else if (*scan1 == '\0')
		return -1;
	else if (*scan2 == '\0')
		return 1;
	else
		return *scan1 - *scan2;
}
#endif
