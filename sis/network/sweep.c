
#include "sis.h"


#ifdef SIS
int
network_sweep_latch(latch, reached_table)
latch_t *latch;
st_table *reached_table;
{
    node_t *lin, *lout, *fanin, *fanout, *cnode;
    node_function_t func;
    node_t **fo_array;
    int changed = 0;
    int i, init;
    lsGen gen;
    network_t *network;
    
    lin = latch_get_input(latch);
    lout = latch_get_output(latch);
    fanin = node_get_fanin(lin, 0);
    func = node_function(fanin);
    init = latch_get_initial_value(latch);
    network = node_network(lin);
    if ((func == NODE_0 && (init == 0 || init == 2)) ||
	func == NODE_1 && (init == 1 || init == 2)) {
	fo_array = ALLOC(node_t *, node_num_fanout(lout));
        i = 0;
	foreach_fanout(lout, gen, fanout) {
	    fo_array[i++] = fanout;
	}
	for (i = node_num_fanout(lout); i-- > 0;) {
	    node_patch_fanin(fo_array[i], lout, fanin);
	}
	FREE(fo_array);
	changed = 1;
	network_delete_latch(network, latch);
	network_delete_node(network, lin);
	network_delete_node(network, lout);
	return changed;
    }
    if (!st_lookup(reached_table, (char *) lout, NIL(char *))) {
	changed = 1;
	cnode = node_constant(0);
	network_add_node(network, cnode);
        fo_array = ALLOC(node_t *, node_num_fanout(lout));
        i = 0;
        foreach_fanout(lout, gen, fanout) {
            fo_array[i++] = fanout;
        }
        for (i = node_num_fanout(lout); i-- > 0;) {
            node_patch_fanin(fo_array[i], lout, cnode);
        }
        FREE(fo_array);
	network_delete_latch(network, latch);
	network_delete_node(network, lin);
	network_delete_node(network, lout);
    }
    return changed;
}
#endif /* SIS */


int
network_sweep_node(node)
node_t *node;
{
    int i, some_change, changed;
    node_t *fanin;
    node_function_t func;

    changed = 0;

    if (node->type == PRIMARY_OUTPUT) {
	/* For a primary output, only sweep away a buffer node */
	fanin = node_get_fanin(node, 0);
	if (node_function(fanin) == NODE_BUF) {
	    assert(node_patch_fanin(node, fanin, node_get_fanin(fanin, 0)));
	    changed = 1;
	}
    } else {
	do {
	    some_change = 0;
	    foreach_fanin(node, i, fanin) {
		func = node_function(fanin);
		if (func == NODE_0 || func == NODE_1 || func == NODE_BUF 
							|| func == NODE_INV) {
		    (void) node_collapse(node, fanin);
		    some_change = 1;
		    changed = 1;
		    break;
		}
	    }
	} while (some_change);
    }
    return changed;
}


int
network_sweep(network)
network_t *network;
{
    int status, latch_removed;

    status = network_sweep_util(network, 1, &latch_removed);
    if ((network->dc_network != NIL(network_t)) && latch_removed) {
	network_free(network->dc_network);
	network->dc_network = NIL(network_t);
    }
    return status;
}


/* only sweep combinational logic, not latches */

int 
network_csweep(network)
network_t *network;
{
    return network_sweep_util(network, 0, NIL(int));
}


int
network_sweep_util(network, sweep_latch, latch_removed)
network_t *network;
int sweep_latch;
int *latch_removed;
{
    int i, j, changed, some_change;
    array_t *node_vec;
    node_t *node, *pi, *po;
#ifdef SIS
    int latch_change = 0;
    array_t *latch_array;
    latch_t *l, *l2;
    node_t *input, *output, *fanin, *fanout;
    node_t *dc_pi1, *dc_pi2, *dc_po1, *dc_po2;
    int num_fanout;
    st_table *lfanin_table, *delete_latch, *reached_table;
    st_generator *sgen;
    node_t **fo_array;
    node_t *latch_patch[3];
    int ival;
#endif /* SIS */
    lsGen gen;
    network_t *dc_net;
    
    changed = 0;
    some_change = 1;

    while (some_change) {
	some_change = 0;
        node_vec = network_dfs(network);
        for(i = 0; i < array_n(node_vec); i++) {
            node = array_fetch(node_t *, node_vec, i);
            if (network_sweep_node(node)) {
                some_change = changed = 1;
            }
        }
        array_free(node_vec);
#ifdef SIS
	if (sweep_latch) {
	    reached_table = snetwork_tfi_po(network);
            latch_array = array_alloc(latch_t *, network_num_latch(network));
    	    foreach_latch(network, gen, l) {
    	        array_insert_last(latch_t *, latch_array, l);
	    }
	    for (i = 0; i < array_n(latch_array); i++) {
	        l = array_fetch(latch_t *, latch_array, i);
	        if (network_sweep_latch(l, reached_table)) {
		    some_change = changed = latch_change = 1;
	        }
	    }
            array_free(latch_array);
	    st_free_table(reached_table);
	}
#endif /* SIS */
    }
    if (network->dc_network != NIL(network_t)) {
        (void) network_sweep(network->dc_network);
    }
    
#ifdef SIS
    some_change = TRUE;
    while(some_change){
	some_change = FALSE;
        if (sweep_latch) {
	/* delete latches that are redundant */
	
	lfanin_table = st_init_table(st_ptrcmp, st_ptrhash);
	delete_latch = st_init_table(st_ptrcmp, st_ptrhash);
	foreach_latch(network, gen, l) {
	    fanin = node_get_fanin(latch_get_input(l), 0);
	    (void) st_insert(lfanin_table, (char *) fanin, NIL(char));
	}
	
	/* put all redundant latches in the delete_latch table */
	
	st_foreach_item(lfanin_table, sgen, (char **) &fanin, NIL(char *)) {
	    latch_array = array_alloc(latch_t *, node_num_fanout(fanin));
	    foreach_fanout(fanin, gen, fanout) {
		if ((l = latch_from_node(fanout)) != NIL(latch_t)) {
		    array_insert_last(latch_t *, latch_array, l); 
		}
	    }
	    /* Compare each latch against all the others for equivalence */
	    for (i = 0; i < array_n(latch_array); i++) {
		l = array_fetch(latch_t *, latch_array, i);
		if (l == NIL(latch_t)) continue;
		for (j = i+1; j < array_n(latch_array); j++) {
		    l2 = array_fetch(latch_t *, latch_array, j);
		    if (l2 == NIL(latch_t)) continue;
		    if (latch_equal(l, l2)) {
			st_insert(delete_latch, (char *) l2,
                                  (char *) latch_get_output(l));
			array_insert(latch_t *, latch_array, j, NIL(latch_t));
                    }
		}
	    }
	    array_free(latch_array);
	}
	st_foreach_item(delete_latch, sgen, (char **) &l, (char **) &output) {
	    node = latch_get_output(l);
	    input = latch_get_input(l);
	    fo_array = ALLOC(node_t *, node_num_fanout(node));
	    i = 0;
	    foreach_fanout(node, gen, fanout) {
		fo_array[i++] = fanout;
	    }
	    for (i = node_num_fanout(node); i-- > 0;) {
		node_patch_fanin(fo_array[i], node, output);
	    }
            some_change = TRUE;
	    changed = latch_change = 1;
		    
	    network_delete_latch(network, l);
	    network_delete_node(network, input);
	    network_delete_node(network, node);
	    FREE(fo_array);
	}
	
	st_free_table(lfanin_table);
	st_free_table(delete_latch);
	}
    }
    if (latch_removed != NIL(int)) {
	*latch_removed = latch_change;
    }
#endif /* SIS */

    /* delete nodes with no fanout */
#ifdef SIS
    if (network_cleanup_util(network, sweep_latch, &latch_change)) {
#else
    if (network_cleanup_util(network, sweep_latch, NIL(int))) {
#endif
	changed = 1;
    }
#ifdef SIS
    if (latch_change) *latch_removed = 1;
#endif
    return changed;
}

