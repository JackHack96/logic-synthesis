/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/jedi/getopt.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:08 $
 *
 */
/*  File   : getopt.c
    Author : Henry Spencer, University of Toronto
    Updated: 28 April 1984
    Purpose: get option letter from argv.
*/
#include "../port/port.h"
#define	NullS	((char *) 0)

char	*optarg;	/* Global argument pointer. */
int	optind = 0;	/* Global argv index. */

int getopt(argc, argv, optstring)
    int argc;
    char *argv[];
    char *optstring;
    {
	register int c;
	register char *place;
	static char *scan = NullS;	/* Private scan pointer. */

	optarg = NullS;

	if (scan == NullS || *scan == '\0') {
	    if (optind == 0) optind++;
	    if (optind >= argc) return EOF;
	    place = argv[optind];
	    if (place[0] != '-' || place[1] == '\0') return EOF;
	    optind++;
	    if (place[1] == '-' && place[2] == '\0') return EOF;
	    scan = place+1;
	}

	c = *scan++;
	place = strchr(optstring, c);
	if (place == NullS || c == ':') {
	    (void) fprintf(stderr, "%s: unknown option %c\n", argv[0], c);
	    return '?';
	}
	if (*++place == ':') {
	    if (*scan != '\0') {
		optarg = scan, scan = NullS;
	    } else {
		optarg = argv[optind], optind++;
	    }
	}
	return c;
    }
