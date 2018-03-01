/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/network/net_seq.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:32 $
 *
 */
#ifdef SIS

#include "sis.h"

node_t *
network_latch_end(node)
node_t *node;
{
    node_type_t type;
    network_t *network;
    latch_t *latch;

    network = node_network(node);
    if (network == NIL(network_t)) {
        fail("latch_end: node not part of a network");
    }

    type = node_type(node);
    if (type != PRIMARY_INPUT && type != PRIMARY_OUTPUT) {
        return NIL(node_t);
    }

    if (st_lookup(network->latch_table, (char *) node, (char **) &latch) != 0) {
	if (latch == NIL(latch_t)) {
	    return(NIL(node_t));
	}
	if (latch_get_input(latch) == node) {
	    return latch_get_output(latch);
	}
	else {
	    return latch_get_input(latch);
	}
    }
    else {
        return(NIL(node_t));
    }
}


/*
 * The latch order is implicitly stored by network_create_latch(), which will
 * add a latch from a po to a pi.  Each call to network_create_latch() will
 * result in appending another latch at the end of the latch order.
 */
void
network_create_latch(network, l, n1, n2)
network_t *network;
latch_t **l;
node_t *n1, *n2;
{
    lsGen gen;
    node_t *fo;
    node_t *po_ptr, *pi_ptr;
    int found = 0;
    st_table *latch_table;
    
    if ((n1->network != network) || (n2->network != network)) {
	fail("network_create_latch: nodes must both belong to the given network");
    }
    foreach_fanout(n1, gen, fo) {
	if (fo == n2) {
	    found = 1;
	    (void) lsFinish(gen);
	    break;
	}
    }
    if (found) {
	network_disconnect(n1, n2, &po_ptr, &pi_ptr);
	*l = latch_alloc();
	latch_set_input(*l, po_ptr);
	latch_set_output(*l, pi_ptr);
	latch_table = network->latch_table;
	(void) st_insert(latch_table, (char *) po_ptr, (char *) *l);
	(void) st_insert(latch_table, (char *) pi_ptr, (char *) *l);
	LS_ASSERT (lsNewEnd(network->latch, (lsGeneric) *l, LS_NH));
	return;
    }
    if (node_type(n1) == PRIMARY_OUTPUT && node_type(n2) == PRIMARY_INPUT) {
	if (network_latch_end(n1) == n2) {
	    *l = latch_from_node(n1);
	    return;
	}
	if (network_latch_end(n1) != NIL(node_t)) {
	    fail("network_create_latch: input node is already part of a latch");
	}
	if (network_latch_end(n2) != NIL(node_t)) {
	    fail("network_create_latch: output node is already part of a latch");
	}
	*l = latch_alloc();
	latch_set_input(*l, n1);
	latch_set_output(*l, n2);
	latch_table = network->latch_table;
	(void) st_insert(latch_table, (char *) n1, (char *) *l);
	(void) st_insert(latch_table, (char *) n2, (char *) *l);
	LS_ASSERT (lsNewEnd(network->latch, (lsGeneric) *l, LS_NH));
	return;
    } else {
	fail("network_create_latch: latch cannot be added between those two node types");
    }
}


void
network_delete_latch(network, l)
network_t *network;
latch_t *l;
{
    lsGen gen;
    char *data = NIL(char);

    if (latch_get_input(l) == NIL(node_t)) {
	fail("network_delete_latch: attempt to delete latch with no input");
    }
    if (latch_get_input(l)->network != network) {
	fail("network_delete_latch: attempt to delete latch not in network");
    }
    for (gen = lsStart(network->latch); (latch_t *) data != l;
	 (void) lsNext(gen, (lsGeneric *) &data, LS_NH)) {
	;
    }
    assert((latch_t *) data == l);
    network_delete_latch_gen(network, gen);
    LS_ASSERT(lsFinish(gen));
}


void
network_delete_latch_gen(network, gen)
network_t *network;
lsGen gen;
{
    node_t *input, *output;
    latch_t *latch;

    LS_ASSERT(lsDelBefore(gen, (lsGeneric *) &latch));
    input = latch_get_input(latch);
    output = latch_get_output(latch);

    if (! st_delete(network->latch_table, (char **) &input, NIL(char *))) {
	fail("network_delete_latch_gen: latch input not in the table");
    }
    assert(input == latch_get_input(latch));

    if (! st_delete(network->latch_table, (char **) &output, NIL(char *))) {
	fail("network_delete_latch_gen: latch_output not in the table");
    }
    assert(output == latch_get_output(latch));

    latch_free(latch);
    return;
}


void
network_disconnect(node1, node2, po_ptr, pi_ptr)
node_t *node1, *node2, **po_ptr, **pi_ptr;
{
    if(node1->network != node2->network){
	fail("network_disconnect: node1 and node2 do not belong to the same network");
    }
    if(node1->network == NIL(network_t)){
	fail("network_disconnect: node1 and node2 do not belong to a network");
    }
    *po_ptr = network_add_fake_primary_output(node1->network, node1);
    *pi_ptr = node_alloc();
    network_add_primary_input(node2->network, *pi_ptr);
    (void)node_patch_fanin(node2, node1, *pi_ptr);
    return;
}


void 
network_connect(node1, node2)
node_t *node1, *node2;
{
    lsGen gen;
    node_t *fanout, *fanin, *n;
    int i, num_nodes;
    array_t *tfo_array;
    network_t *network;

    if(node1->network != node2->network){
	fail("network_connect: node1 and node2 do not belong to the same network");
	}
    if(node1->network == NIL(network_t)){
	fail("network_connect: node1 and node2 do not belong to a network");
    }
    if(node_function(node1) != NODE_PO){
	fail("network_connect: node1 not a PRIMARY OUTPUT");
    }
    if(node_function(node2) != NODE_PI){
	fail("network_connect: node2 not a PRIMARY INPUT");
    }
    fanin = node_get_fanin(node1, 0);
    network = fanin->network;
    num_nodes = network_num_pi(network) + network_num_po(network) +
                network_num_internal(network);
    foreach_fanout(node2, gen, fanout){
        tfo_array = network_tfo(fanout, num_nodes);
        for (i = 0; i < array_n(tfo_array); i++) {
	    n = array_fetch(node_t *, tfo_array, i);
            if (n == fanin) {
		(void) array_free(tfo_array);
		fail("network_connect: connection would result in combinational cycles");
            }
	}
	(void) array_free(tfo_array);
    }
    foreach_fanout(node2, gen, fanout){
	(void) node_patch_fanin(fanout, node2, fanin);
    }
    network_delete_node(node1->network, node1);
    network_delete_node(node2->network, node2);
}


graph_t *
network_stg(network)
network_t *network;
{
    if (network == NIL(network_t)) {
	fail("network_stg: no network");
    }
    return network->stg;
}


void
network_set_stg(network, stg)
network_t *network;
graph_t *stg;
{
    if (network == NIL(network_t)) {
	fail("network_set_stg: no network");
    }
    network->stg = stg;
    if (stg != NIL(graph_t)){
	stg_save_names(network, stg, 1);
    }
    return;
}

int
network_stg_check(network)
network_t *network;
{
    array_t *po_list, *ar;
    bdd_t *f_bdd, *prod_bdd, *new_prod, *bdd_test;
    st_table *leaves;
    bdd_manager *manager;
    char *input, *output;
    char *in_state, *out_state;
    char *save_in, *save_ps;
    lsGen gen, gen2;
    edge_t *e;
    node_t *pi, *po;
    st_generator *sym_table_gen;
    int index, max;

    /* if no network exists (just an stg) then return 1 */

    if (network_num_pi(network) == 0) {
	return 1;
    }
    if (!network_check(network)) {
	(void) fprintf(siserr, "network_stg_check: ");
        (void) fprintf(siserr, "network failed network_check\n");
	return 0;
    }
    if (network->stg == NIL(graph_t)) {
	return 1;
    }

    /* I think this code is not necessary, because if there's an stg, */
    /* it was read in with read_kiss, which calls stg_check. */

/*
    if (!stg_check(network->stg)) {
	(void) fprintf(siserr, "network_stg_check: ");
        (void) fprintf(siserr, "STG failed stg_check\n");
	return 0;
    }
*/

    (void) fprintf(sisout, "Checking to see that the STG ");
    (void) fprintf(sisout, "covers the network...\n");
    if (network_num_latch(network) !=
        (int) strlen(stg_get_state_encoding(stg_get_start(network->stg)))) {
	(void) fprintf(siserr, "network_stg_check: number of latches ");
        (void) fprintf(siserr, "does not match number of encoding bits");
	return 0;
    }

    /*
     * Create an array for the primary outputs.
     */
    po_list = array_alloc(node_t *, 0);
    foreach_primary_output (network, gen, po) {
	if (network_get_control(network,po) == NIL(node_t)) {
            array_insert_last(node_t *, po_list, po);
        }
    }

    /*
     * Create a hash table of the primary inputs, and initialize
     * their values ot -1.
     */
    leaves = st_init_table(st_ptrcmp, st_ptrhash);
    foreach_primary_input(network, gen, pi) {
        if (network_get_control(network,pi) == NIL(node_t)) {
            (void) st_insert(leaves, (char *) pi, (char *) -1);
        }
    }

    /*
     * Get an ordering for the leaves.  The variable id of each PI node
     * will be set as the value in the leaves table.  The third argument
     * says that the nodes in po_list can be visited in any order.
     */
    ar = order_dfs(po_list, leaves, 0);
    (void) array_free(ar);

   /* 
    * We want to elegantly handle the case where a PI does not fanout.
    * Thus, first find the max index assigned to an input (which was reached
    * from a node in po_list).  Then, assign an index to the unassigned PI's.
    */
    max = 0;
    st_foreach_item_int(leaves, sym_table_gen, (char **) &pi, &index) {
        if (index > max) {
	    max = index;
	}
    }
    max++;

    st_foreach_item_int(leaves, sym_table_gen, (char **) &pi, &index) {
      if (index < 0) {
	  index = max++;
	  (void) st_insert(leaves, (char *) pi, (char *) index);
      }
    }

    /* 
     * Create a manager with number of variables equal to number of primary inputs.
     */
    manager = ntbdd_start_manager(st_count(leaves));

    /* 
     * Create single variable BDD's for each primary input.  We do this explicitly, instead 
     * of relying on BDD's to be created for the PI's when the BDD's are recursively
     * built for the PO's, because some PI's may not fanout.
     */
    foreach_primary_input(network, gen, pi) {
        if (network_get_control(network,pi) == NIL(node_t)) {
            (void) ntbdd_node_to_bdd(pi, manager, leaves);
        }
    }

    /* 
     * Create global BDD's for each primary output.  These will be automatically
     * stored in the BDD slot of the node.  This also has the side affect of 
     * creating BDD's for every node in the transitive fanin of the PO's; the BDD's
     * for the primary inputs were built above.  Note: return value of 
     * ntbdd_node_to_bdd not used.
     */
    foreach_primary_output(network, gen, po) {
	if (network_get_control(network,po) == NIL(node_t)) {
            (void) ntbdd_node_to_bdd(po, manager, leaves);
        }
    }
    (void) st_free_table(leaves);

    /* 
     * Checking that the network performs the behavior specified by the stg.
     * Do by simulating each transition in the stg.
     */
    stg_foreach_transition(network->stg, gen, e) {

        /* 
         * Get the input and output vectors for the transition, as well
         * as the current state and next state in state-encoded form.
         */
	input = stg_edge_input_string(e);
	output = stg_edge_output_string(e);
	in_state = stg_get_state_encoding(stg_edge_from_state(e));
	if (in_state == NIL(char)) {
	  (void) fprintf(siserr, "Network_stg_check:  \n");
	  (void) fprintf(siserr, "State %s is missing its encoding\n",
			 stg_get_state_name(stg_edge_from_state(e)));
	  lsFinish(gen);
	  return 0;
	}
        save_in = input; /* save pointers to the beginning of these strings */
	save_ps = in_state; /* so that good error messages can be printed */
	out_state = stg_get_state_encoding(stg_edge_to_state(e));
	if (out_state == NIL(char)) {
	  (void) fprintf(siserr, "Network_stg_check:  ");
	  (void) fprintf(siserr, "State %s is missing its encoding\n",
			 stg_get_state_name(stg_edge_from_state(e)));
	  lsFinish(gen);
	  return 0;
	}

        /* 
         * The product bdd is built incrementally as the product of all the 
         * input bdds and the from-state bdds.  Each bdd is ANDed in 
         * positive or negative form, depending on the value of that 
         * particular state bit or input bit.  If the bit is 2, the bdd 
         * is not ANDed into the product. 
         */
	prod_bdd = bdd_one(manager);
	foreach_primary_input(network, gen2, pi) {
	    if (network_get_control(network,pi) == NIL(node_t)) {
	        f_bdd = ntbdd_at_node(pi);
		if (f_bdd == NIL(bdd_t)) {
		    /* this PI is unused anyway; can skip */
		    continue;
		}
	        if (network_latch_end(pi) == NIL(node_t)) {
		    if (*input == '0') {
			new_prod = bdd_and(prod_bdd, f_bdd, 1, 0);
			(void) bdd_free(prod_bdd);
			prod_bdd = new_prod;
		    } else if (*input == '1') {
			new_prod = bdd_and(prod_bdd, f_bdd, 1, 1);
			(void) bdd_free(prod_bdd);
			prod_bdd = new_prod;
		    }
		    input++;
		} else {
		    if (*in_state == '0') {
			new_prod = bdd_and(prod_bdd, f_bdd, 1, 0);
			(void) bdd_free(prod_bdd);
			prod_bdd = new_prod;
		    } else if (*in_state == '1') {
			new_prod = bdd_and(prod_bdd, f_bdd, 1, 1);
			(void) bdd_free(prod_bdd);
			prod_bdd = new_prod;
		    }
		    in_state++;
		}
	    }
	} /* close: foreach_primary_input(network, gen2, pi) { */

        /* 
         * Each output bdd and to-state bdd is cofactored with respect to
         * product bdd.  The result is checked against the value of that 
         * output bit on the transition, or the value of the encoded bit 
         * in the to-state for that transition. 
         */
	foreach_primary_output(network, gen2, po) {
	    if (network_get_control(network,po) == NIL(node_t)) {
		f_bdd = ntbdd_at_node(po);
		bdd_test = bdd_cofactor(f_bdd, prod_bdd);
		if (network_latch_end(po) == NIL(node_t)) {
		    if (*output == '0') {
			if (!bdd_is_tautology(bdd_test, 0)) {
			    (void) fprintf(siserr, "failed on output node ");
			    (void) fprintf(siserr, "(%s) ", node_name(po));
			    (void) fprintf(siserr, "(should have been 0)\n");
			    (void) fprintf(siserr, "Present state: %s", save_ps);
			    (void) fprintf(siserr, " Input: %s\n", save_in);
			    goto bad_exit;
			}
		    } else if (*output == '1') {
			if (!bdd_is_tautology(bdd_test, 1)) {
			    (void) fprintf(siserr, "failed on output node ");
			    (void) fprintf(siserr, "(%s) ", node_name(po));
			    (void) fprintf(siserr, "(should have been 1)\n");
			    (void) fprintf(siserr, "Present state: %s", save_ps);
			    (void) fprintf(siserr, " Input: %s\n", save_in);
			    goto bad_exit;
			}
		    }
		    output++;
		} else {
		    if (*out_state == '0') {
			if (!bdd_is_tautology(bdd_test, 0)) {
			    (void) fprintf(siserr, "failed on latch output node ");
			    (void) fprintf(siserr, "(%s) ", node_name(po));
			    (void) fprintf(siserr, "(should have been 0)\n");
			    (void) fprintf(siserr, "Present state: %s", save_ps);
			    (void) fprintf(siserr, " Input: %s\n", save_in);
			    goto bad_exit;
			}
		    } else if (*out_state == '1') {
			if (!bdd_is_tautology(bdd_test, 1)) {
			    (void) fprintf(siserr, "failed on latch output node ");
			    (void) fprintf(siserr, "(%s) ", node_name(po));
			    (void) fprintf(siserr, "(should have been 1)\n");
			    (void) fprintf(siserr, "Present state: %s", save_ps);
			    (void) fprintf(siserr, " Input: %s\n", save_in);
			    goto bad_exit;
			}
		    }
		    out_state++;
		}
		(void) bdd_free(bdd_test);
	    }
	} /* close: foreach_primary_output(network, gen2, po) { */
       
        (void) bdd_free(prod_bdd);
    } /* close: stg_foreach_transition(network->stg, gen, e) { */

    /*
     * Free the manager.  This has the side affect of free the bdd_t at every
     * node in the network for which a BDD was created.
     */
    (void) ntbdd_end_manager(manager);
    (void) array_free(po_list);

    return 1;
bad_exit:
    read_error("One of the STG edges did not simulate correctly");
    lsFinish(gen);
    lsFinish(gen2);
    (void) bdd_free(prod_bdd);  /* probably superflous since freeing manager next */
    (void) bdd_free(bdd_test);
    (void) ntbdd_end_manager(manager);
    (void) array_free(po_list);

    return 0;
}


/*
 * Returns whether node is a real primary input or output of the network.  If
 * node is not a PI or PO, it can't possibly be in latch_table, returns 0.  If
 * node is a latch input or latch output, returns 0, sets `latchp' non-nil.
 * If node is a control signal, returns 0, sets `latchp' nil.  If node is a
 * real primary input or output, returns 1.
 *
 * A member of .clock is considered a real PI.
 */
static int
network_is_real_pio(network, node, latchp)
network_t *network;
node_t *node;
latch_t **latchp;
{
    st_table *latch_table;
    network_t *net;

    net = node->network;
    
    if (net != network) {
        abort();
    }
    latch_table = net->latch_table;
    if (st_lookup(latch_table, (char *) node, (char **) latchp) == 0) {
        return(1);
    }
    return(0);
}

int
network_is_real_po(network, node)
network_t *network;
node_t *node;
{
    latch_t *latch;

    if (node_type(node) != PRIMARY_OUTPUT) {
        return(0);
    }
    return(network_is_real_pio(network, node, &latch));
}

int
network_is_real_pi(network, node)
network_t *network;
node_t *node;
{
    latch_t *latch;
    
    if (node_type(node) != PRIMARY_INPUT) {
        return(0);
    }
    return(network_is_real_pio(network, node, &latch));
}

int
network_is_control(network, node)
network_t *network;
node_t *node;
{
    latch_t *latch;
    
    if (network_is_real_pio(network, node, &latch) == 0) {
        return(latch == NIL(latch_t));
    }
    return(0);
}


node_t *
network_get_control(network, control)
network_t *network;
node_t *control;
{
    lsGen gen;
    node_t *fo;

    if (node_type(control) == PRIMARY_OUTPUT) {
        control = node_get_fanin(control, 0);
    }
    foreach_fanout (control, gen, fo) {
        if (node_type(fo) == PRIMARY_OUTPUT) {
	    if (network_is_control(network, fo) != 0) {
	        (void) lsFinish(gen);
		return(fo);
	    }
	}
    }
    return(NIL(node_t));
}	    
    

/*
 * Used for adding primary outputs that will be a latch input or a latch
 * control.  Should be used in the io package for the network to print out
 * correctly.
 */
node_t *
network_add_fake_primary_output(network, node)
network_t *network;
node_t *node;
{
    node_t *out;

    out = network_add_primary_output(network, node);
    network_swap_names(network, node, out);
    return(out);
}

/*
 * Used only in io package.  Problem: when creating the network, need to
 * change the name for a fake primary output (a clock or latch) to be " #"
 * where # is some number, so it cannot possibly conflict with a user
 * specified name to come later.
 *
 * Postprocess: Change all the fake names that start with the " " to some
 * other name.
 */
void
network_replace_io_fake_names(network)
network_t *network;
{
    lsGen gen;
    st_table *name_table;
    st_table *dc_name_table;
    node_t *node;

    name_table = network->name_table;
    if (network->dc_network != NIL(network_t)) {
	dc_name_table = network->dc_network->name_table;
    } else {
	dc_name_table = NIL(st_table);
    }
    foreach_node (network, gen, node) {
        if (node->name[0] == ' ') {
	    (void) st_delete(name_table, &node->name, NIL(char *));
	    do {
	        node_assign_name(node);
	    } while ((st_is_member(name_table, node->name) != 0) ||
		      (dc_name_table != NIL(st_table) &&
                       st_is_member(dc_name_table, node->name) != 0));
	    (void) st_insert(name_table, node->name, (char *) node);
	}
    }
}


/* Visited nodes backwards (towards pi's) */
/* Jump across latches */

static void
snetwork_dfs_recur(node, visited)
node_t *node;
st_table *visited;
{
    char *dummy;
    int i;
    node_t *fanin;

    if (st_lookup(visited, (char *) node, &dummy)) {
	return;
    }
    (void) st_insert(visited, (char *) node, NIL(char));
    foreach_fanin(node, i, fanin) {
	snetwork_dfs_recur(fanin, visited);
    }
    if ((node_type(node) == PRIMARY_INPUT) &&
	((fanin = network_latch_end(node)) != NIL(node_t))) {
	snetwork_dfs_recur(fanin, visited);
    }
    return;
}


/* Return a table of all the nodes that have a PO in their transitive
   fanout.  This is sequential transitive fanout, so latches are traversed */

st_table *
snetwork_tfi_po(network)
network_t *network;
{
    st_table *visited;
    lsGen gen;
    node_t *po;
    st_generator *sgen;
    node_t *node;
    char *dummy;

    visited = st_init_table(st_ptrcmp, st_ptrhash);

    foreach_primary_output(network, gen, po) {
	if (!network_is_real_po(network, po)) continue;
	snetwork_dfs_recur(po, visited);
    }
    return visited;
}
	
#endif /* SIS */
