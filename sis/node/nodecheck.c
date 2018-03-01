/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/node/nodecheck.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:47 $
 *
 */
#include "sis.h"
#include "node_int.h"


static void
chk_err(s, node)
char *s;
node_t *node;
{
    error_append("node_check: inconsistency detected");
    if (node != 0) {
	error_append(" at ");
	error_append(node_name(node));
    }
    error_append(" -- ");
    error_append(s);
    error_append("\n");
}


int
node_check(node)
node_t *node;
{
    node_literal_t l;
    pset support, last, last1, p, p1;
    int i, j;

    if (node->type == INTERNAL) {
	/* check for existence of a logic function */
	if (node->F == 0) {
	    chk_err("internal node does not have a logic function", node);
	    return 0;
	}

	/* Check that the node is minimum base */
	if (node->nin > 0) {
	    support = sf_and(node->F);
	    for(i = node->nin-1; i >= 0; i--) {
		if (GETINPUT(support, i) == TWO) {
		    chk_err("node is not minimum base", node);
		    return 0;
		}
	    }
	    set_free(support);
	}

	/* Check that the node is SCC-minimal */
	if (node->nin > 0) {
	    foreach_set(node->F, last, p) {
		foreach_set(node->F, last1, p1) {
		    if (p != p1 && setp_implies(p, p1)) {
			chk_err("node is not SCC-minimal", node);
			return 0;
		    }
		}
	    }
	}

	/* Check for any bad literals in F */
	foreach_set(node->F, last, p) {
	    for(j = node->nin-1; j >= 0; j--) {
		l = GETINPUT(p, j);
		if (l != ONE && l != TWO && l != ZERO) {
		    chk_err("node cube has a bad literal", node);
		    return 0;
		}
	    }
	}

	/* Check for any bad literals in R */
	if (node->R != 0) {
	    foreach_set(node->R, last, p) {
		for(j = node->nin-1; j >= 0; j--) {
		    l = GETINPUT(p, j);
		    if (l != ONE && l != TWO && l != ZERO) {
			chk_err("node offset cube has a bad literal", node);
			return 0;
		    }
		}
	    }
	}

	/* Check that F and R are disjoint and F+R=1 */
	if (node->R != 0) {
	    define_cube_size(node->nin);
	    foreach_set(node->F, last, p) {
		foreach_set(node->R, last1, p1) {
		    if (cdist0(p, p1)) {
			chk_err("node onset and offset are not disjoint", node);
			return 0;
		    }
		}
	    }
	    if (! tautology(cube2list(node->F, node->R))) {
		chk_err("missing.sh minterms from onset union offset", node);
		return 0;
	    }
	}
    }

#if 0
    /* Provide a warning if a node has duplicated fanin */
    for(i = 0; i < node->nin; i++) {
	for(j = i+1; j < node->nin; j++) {
	    if (node->fanin[i] == node->fanin[j]) {
		chk_err("node with duplicated fanin (warning only)", node);
		return 2;
	    }
	}
    }
#endif

    return 1;
}
