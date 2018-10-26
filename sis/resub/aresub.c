
/*
 *  aresub: algebraic re-sutstitution. 
 *  routines provided:
 *      resub_alge_node();
 *      resub_alge_network();
 */

#include "sis.h"
#include "resub.h"
#include "resub_int.h"

/*
 *  Substitute one function (nodep) into all other 
 *  functions using algebraic division of the function 
 *  default is to use complement
 *  if use_complement =0 then don't use it
 */
int
resub_alge_node(f,use_complement)
node_t *f;
int use_complement;
{
    array_t *target, *tl1;
    node_t *np;
    int i, status;

    status = 0;
    if (f->type == PRIMARY_INPUT || f->type == PRIMARY_OUTPUT) { 
	return status;
    }

    target = array_alloc(node_t *, 0);
    foreach_fanin(f, i, np) {
	tl1 = network_tfo(np, 1);
	array_append(target, tl1);
	array_free(tl1);
    }
    array_sort(target, node_compare_id);
    array_uniq(target, node_compare_id, (void (*)()) 0);

    for(i = 0; i < array_n(target); i++) {
	np = array_fetch(node_t *, target, i);
	if (node_substitute(f, np, use_complement)) {
	    status = 1;
	}
    }
    array_free(target);

    return status;
}

/*
 *  Substitute each function in the network into all other 
 *  functions using algebraic division of the function and 
 *  its complement.
 */
void
resub_alge_network(network,use_complement)
network_t *network;
int use_complement;
{
    lsGen gen;
    node_t *np;
    bool not_done;

    not_done = TRUE;
    while (not_done) {
	not_done = FALSE;
	foreach_node(network, gen, np) {
	    if (resub_alge_node(np,use_complement)) {
		not_done = TRUE;
	    }
	}
    }
}
