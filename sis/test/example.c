/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/test/example.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:52 $
 *
 */
#include "sis.h"
#include "test_int.h"


/*
 *  Sample code for a command (with argument parsing)
 */

com_test(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    node_t *f, *g, *g_not, *g_lit, *q, *t1, *t2, *r;
    array_t *node_vec;
    int c, use_complement;

    /*
     *  use util_getopt() to parse the arguments (in this case, a debug flag)
     */
    use_complement = 0;
    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "c")) != EOF) {
	switch(c) {
	case 'c':
	    use_complement = 1;
	    break;
	default:
	    goto usage;
	}
    }

    /* 
     *  map all remaining arguments into node names, and find the nodes
     */
    node_vec = com_get_nodes(*network, argc-util_optind+1, argv+util_optind-1);
    if (array_n(node_vec) != 2) goto usage;

    f = array_fetch(node_t *, node_vec, 0);
    g = array_fetch(node_t *, node_vec, 1);

    /* divide by the function (uncomplemented) */
    q = node_div(f, g, &r);
    g_lit = node_literal(g, 1);
    t1 = node_and(q, g_lit);
    t2 = node_or(t1, r); 
    node_replace(f, t2);		/* free's t2 */
    node_free(r);
    node_free(t1);
    node_free(q);
    node_free(g_lit);

    if (use_complement) {
	/* divide by the function (uncomplemented) */
	g_not = node_not(g);
	q = node_div(f, g_not, &r);
	g_lit = node_literal(g, 0);
	t1 = node_and(q, g_lit);
	t2 = node_or(t1, r); 
	node_replace(f, t2);		/* free's t2 */
	node_free(r);
	node_free(t1);
	node_free(q);
	node_free(g_lit);
    }

    return 0;		/* normal exit */

usage:
    (void) fprintf(miserr, "usage: test [-c] n1 n2\n");
    (void) fprintf(miserr, "    -c\t\tuse complement of n2 in division\n");
    return 1;		/* error exit */
}
