/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/jedi/util.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:08 $
 *
 */
/*
 * Bill Lin
 * University of California, Berkeley
 * Comments to billlin@ic.Berkeley.EDU
 *
 * Copyright (c) 1989 Bill Lin, UC Berkeley CAD Group
 *     Permission is granted to do anything with this
 *     code except sell it or remove this message.
 */

#include "jedi.h"

int distance();			/* forward declaration */
char *int_to_binary();		/* forward declaration */
int binary_to_int();		/* forward declaration */
int parse_line();		/* forward declaration */
FILE *my_fopen();		/* forward declaration */

int distance(code1, code2, width)
char *code1, *code2;
int width;
{
    int dist;
    int pdist;
    int i;

    dist = 0; pdist = 0;
    for (i=0; i<width; i++) {
	if (code1[i] != code2[i] && code1[i] != '-' && code2[i] != '-') {
	    dist++;
	} else if ((code1[i] == '-' && code2[i] != '-') ||
		   (code1[i] != '-' && code2[i] == '-')) {
	    pdist++;
	}
    }

    return (dist + pdist/2);
} /* end of distance */


char *int_to_binary(num, width)
int num, width;
{
    char *buffer;
    int i;
    int j;

    buffer = ALLOC(char, width+1);
    j = num;

    buffer[width] = '\0';
    for (i=width-1; i>=0; i--) {
	if (j & 1) {
	    buffer[i] = '1';
	} else {
	    buffer[i] = '0';
	}
	j = j >> 1;
    }

    return buffer;
} /* end of int_to_binary */


int binary_to_int(binary, width)
char *binary;
int width;
{
    int i, total;
    int mask;

    total = 0;
    mask = 1;
    for (i=width-1; i>=0; i--) {
	if (binary[i] == '1') {
	    total = total | mask;
	}
	mask = mask << 1;
    }

    return total;
} /* end of binary_to_int */


parse_line(line)
register char *line;
{     
    register char **carg = targv;
    register char ch;

    targc = 0;
    while (ch = *line++) {
	if (ch <= ' ') continue;
	targc++;
	*carg++ = line-1;
	while ((ch = *line) && ch > ' ' ) line++;
	if (ch) *line++ = '\0';
    }
    *carg = 0;
} /*  end of parse_line  */


FILE *my_fopen(fname, mode)
char *fname;
char *mode;
{
    FILE *fp;

    if ((fp = fopen(fname, mode)) == NULL) {
	(void) fprintf(stderr, 
	  "error: couldn't open file %s under mode (%s)\n",
	  fname, mode);
	exit(1);
    }

    return fp;
} /* end of my_fopen */
