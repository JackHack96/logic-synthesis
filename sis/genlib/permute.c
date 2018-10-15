/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/genlib/permute.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:30 $
 *
 */
/* file @(#)permute.c	1.1                      */
/* last modified on 5/29/91 at 12:35:30   */
#include "genlib_int.h"

static void gl_permute_recur();


gl_permute(array, n, fct, state)
char **array;
int n;
void (*fct)();
char *state;
{
    gl_permute_recur(array, n, n, fct, state);
}


static void
gl_permute_recur(array, m, n, fct, state)
char **array;
int m;	
int n;
void (*fct)();
char *state;
{
    int i;
    char *t;

    if (m <= 1) {
	(*fct)(array-n+1, n, state);
    } else {
	for(i = 0; i < m; i++) {
	    /* Swap first element with ith element */
	    t = array[i];
	    array[i] = array[0];
	    array[0] = t;

	    /* recur for last m-1 elements */
	    gl_permute_recur(array+1, m-1, n, fct, state);

	    /* swap it back */
	    t = array[i];
	    array[i] = array[0];
	    array[0] = t;
	}
    }
}


#ifdef TEST
#include <stdio.h>

int gl_print_it();


main(argc, argv)
char **argv;
{
    int i, n, array[40];

    if (argc == 2) {
	n = atoi(argv[1]);
    } else {
	printf("usage: permute n\n");
	exit(2);
    }
	
    for(i = 0; i < n; i++) {
	array[i] = i+1;
    }

    gl_permute((char **) array, n, gl_print_it, (char *) 0);
}


gl_print_it(array, n)
char **array;
int n;
{
    int i;
    for(i = 0; i < n; i++) {
	printf(" %d", array[i]);
    }
    printf("\n");
}
#endif
