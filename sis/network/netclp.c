/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/network/netclp.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:33 $
 *
 */
#include "sis.h"


/*
 *  collapse boolean network to two-level functions.
 */

int
network_collapse(network)
network_t *network;
{
    int i, changed;
    node_t *node;
    array_t *node_vec;

    /* order the nodes from a depth-first search */ 
    node_vec = network_dfs(network);

    /* collapse all of the nodes in dfs order */
    changed = 0;
    for(i = 0; i < array_n(node_vec); i++) {
	node = array_fetch(node_t *, node_vec, i);
	if (node->type == INTERNAL) {
	    if (network_collapse_single(node)) {
		changed = 1;
	    }
	}
    } 

    /* final cleanup */
    if (network_cleanup(network)) {
	changed = 1;
    }

    array_free(node_vec);
    return changed;
}


/*
 *  collapse a single node until it depends on primary inputs only
 */
int
network_collapse_single(node)
node_t *node;
{
    int j, changed, some_change;
    node_t *fanin;

    changed = 0;
    do {
	some_change = 0;
	foreach_fanin(node, j, fanin) {
	    if (node_collapse(node, fanin)) {
		some_change = 1;
		changed = 1;
		break;
	    }
	}
    } while (some_change);
    return changed;
}
