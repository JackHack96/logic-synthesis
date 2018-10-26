/*
 * Symbolic encoding program for compiling a symbolic
 * description into a binary representation.  Target
 * is multi-level logic synthesis
 *
 * History:
 *
 * Bill Lin
 * University of California, Berkeley
 * Comments to billlin@ic.Berkeley.EDU
 *
 * Copyright (c) 1989 Bill Lin, UC Berkeley CAD Group
 *     Permission is granted to do anything with this
 *     code except sell it or remove this message.
 */
#include "jedi.h"

extern int currentEnumtype;

cluster_embedding()
{
    int i;
    double cost;

    for (i=0; i<ne ; i++) {
	cluster_embed(enumtypes[i]);

	if (verboseFlag) {
	    currentEnumtype = i;
	    (void) cost_function(&cost);
	    (void) fprintf(stderr, "final cost = %d\n", (int) cost);
	}
    }
}

cluster_embed(type)
struct Enumtype type;
{
    int j;
    int ns, nc;
    int *sa;
    int t, c, s;
    long time;

    /* start time */
    time = util_cpu_time();

    ns = type.ns;
    nc = type.nc;
    sa = ALLOC(int, ns);
    t = ns;
    for (j=0; j<ns; j++) sa[j] = FALSE;
    for (j=0; j<nc; j++ ) type.codes[j].assigned = FALSE;

    while (t > 0) {
	s = select_state(type, sa);
	sa[s] = TRUE;
	c = select_code(type, sa, s);
	type.symbols[s].code_ptr = c;
	type.codes[c].symbol_ptr = s;
	type.codes[c].assigned = TRUE;
	t--;

	if (verboseFlag) {
	    (void) fprintf(stderr, "\tassigning %s with %s ...\n",
	    type.symbols[s].token, type.codes[c].bit_vector);
	}
    }

    FREE(sa);

    if (verboseFlag) {
	time = util_cpu_time() - time;
	(void) fprintf(stderr,"total cpu time is %s.\n", util_print_time(time));
    }
}

int
select_state(type, sa)
struct Enumtype type;
int *sa;
{
    int s, i, j;
    int ns, we;
    int best, force;

    best = -1;
    ns = type.ns;
    for (i=0;i<ns;i++) {
	if (sa[i] == FALSE) {
	    force = 0;
	    for (j=0;j<ns;j++) {
		if (sa[j] == TRUE) {
		    we = type.links[i][j].weight;
		    force += we;
		}
	    }

	    if (force > best) {
		best = force;
		s = i;
	    }
	}
    }
    return s;
}

int select_code(type, sa, s)
struct Enumtype type;
int *sa, s;
{
    int c;
    int best, force;
    int ns, nc;
    int we, dist;
    int i, f, p;

    best = -1;
    ns = type.ns;
    nc = type.nc;
    for (i=0;i<nc;i++) {
	if (type.codes[i].assigned == FALSE) {
	    force = 0;
	    for (f=0;f<ns;f++) {
		if (sa[f] == TRUE && f != s) {
		    we = type.links[s][f].weight;
		    p = type.symbols[f].code_ptr;
		    dist = type.distances[i][p];
		    force += we * (dist - 1);
		}
	    }
	
	    if (force < best || best < 0) {
		best = force;
		c = i;
	    }
	}
    }
    return c;
}
