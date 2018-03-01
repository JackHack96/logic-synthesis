/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sred/procargs.c,v $
 * $Author: pchong $
 * $Revision: 1.2 $
 * $Date: 2004/03/14 17:14:14 $
 *
 */
#include "reductio.h"

#define max(a,b) (a>b?a:b)
#define min(a,b) (a<b?a:b)

procargs(argc,argv)

int argc;
char *argv[];

{
	char c;

	util_getopt_reset ();
	do_print_classes = 0;
	debug = 0;
	coloring_algo = "fast";
	while ((c = util_getopt(argc, argv, "v:hca:")) != EOF) {
	switch (c) {
	case 'c':
		do_print_classes = 1;
		break;
	case 'a':
		coloring_algo = util_optarg;
		break;
	case 'v':
		debug = atoi (util_optarg);
		break;
	case 'h':
	default:
		fprintf (stderr, "usage: %s [-c] [-a coloring_algorithm]\n", argv[0]);
		fprintf (stderr, "       -c: print out minimized state classes\n"); 
		fprintf (stderr, "       -a: select the coloring algorithm among:\n");
		fprintf (stderr, "           fast: a fast and greedy heuristics\n");
		fprintf (stderr, "           espresso, expand_and_cover, espresso_exact:\n");
		fprintf (stderr, "           use logic minimization (can be expensive)\n");
		exit (1);
		break;
	}
	}
}

