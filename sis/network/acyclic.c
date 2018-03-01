/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/network/acyclic.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:32 $
 *
 */
#include "sis.h"

/*
 *  network_is_acyclic -- detect whether a network contains a cycle
 *
 *  Calls error_append() with any error messages indicating the nodes
 *  on a cycle.
 */

static int do_acyclic_check();
static void extract_path();


int
network_is_acyclic(network)
network_t *network;
{
    st_table *visited;
    node_t *p;
    lsGen gen;

    visited = st_init_table(st_ptrcmp, st_ptrhash);

    /* start a DFS from the outputs */
    foreach_primary_output(network, gen, p) {
	if (! do_acyclic_check(p->fanin[0], visited)) {
	    extract_path(visited);
	    return 0;
	}
    }

    /* we must now also recur from each node we haven't visited yet */
    foreach_node(network, gen, p) {
	if (! st_lookup(visited, (char *) p, NIL(char *))) {

	    /* this node was not reached in DFS from the outputs */
	    if (! do_acyclic_check(p, visited)) {
		extract_path(visited);
		return 0;
	    }
	}
    }

    st_free_table(visited);
    return 1;
}


static int 
do_acyclic_check(node, visited)
node_t *node;
st_table *visited;
{
    char *value;
    int i, status;
    node_t *fanin;

    if (st_lookup(visited, (char *) node, &value)) {
	return ! (int) value;		/* return 0 if already active */

    } else {
	/* add this node to the active path */
	value = (char *) 1;
	(void) st_insert(visited, (char *) node, value);

	foreach_fanin(node, i, fanin) {
	    status = do_acyclic_check(fanin, visited);
	    if (status == 0) {
		return status;
	    }
	}

	/* take this node off of the active path */
	value = (char *) 0;
	(void) st_insert(visited, (char *) node, value);

	return 1;
    }
}

static void
extract_path(visited)
st_table *visited;
{
    st_generator *gen;
    char *key, *value;
    node_t *node;

    error_append("error: network contains a cycle\n");
    st_foreach_item(visited, gen, &key, &value) {
	if ((int) value == 1) {
	    node = (node_t *) key;
	    error_append("node '");
	    error_append(node_name(node));
	    error_append("' is on the cycle\n");
	}
    }
}
