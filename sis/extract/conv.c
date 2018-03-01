/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/extract/conv.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:22 $
 *
 */
#include "sis.h"
#include "extract_int.h"

network_t *global_network;
nodeindex_t *global_node_index;
array_t *global_old_fct;
int use_complement;


/*
 *  Mapping between the sparse matrix and the nodes in the matrix.
 *  Currently this is done with a set of variables global to the extract
 *  package:
 *
 *	global_network -- the network being operated on
 *	global_node_index -- mappnig between column numbers and nodes
 *	global_old_fct -- duplicates of the original nodes, indexed by above
 */

void
ex_setup_globals(network, dup_fcts)
network_t *network;
int dup_fcts;		/* if 1, also save the original functions */
{
    int index;
    node_t *node, *node1;
    lsGen gen;

    global_network = network;
    global_node_index = nodeindex_alloc();
    if (dup_fcts) {
	global_old_fct = array_alloc(node_t *, 0);
    } else {
	global_old_fct = 0;
    }

    foreach_node(network, gen, node) {
	if (node->type != PRIMARY_OUTPUT) {
	    index = nodeindex_insert(global_node_index, node);
	    if (dup_fcts) {
		node1 = node_dup(node);
		array_insert(node_t *, global_old_fct, index, node1);
	    }
	}
    }
}

void
ex_setup_globals_single(node)
node_t *node;
{
    int i;
    node_t *fanin;

    global_network = NIL(network_t);
    global_node_index = nodeindex_alloc();
    foreach_fanin(node, i, fanin) {
	(void) nodeindex_insert(global_node_index, fanin);
    }
    (void) nodeindex_insert(global_node_index, node);
    global_old_fct = NIL(array_t);
}

void
ex_free_globals(dup_fcts)
int dup_fcts;
{
    int i;
    node_t *node1;

    if (dup_fcts) {
	for(i = 0; i < array_n(global_old_fct); i++) {
	    node1 = array_fetch(node_t *, global_old_fct, i);
	    node_free(node1);
	}
	array_free(global_old_fct);
    }
    nodeindex_free(global_node_index);
}


char *
ex_map_index_to_name(i)
int i;
{
    return node_name(nodeindex_nodeof(global_node_index, i));
}

/*
 *  convert a network node into a sparse matrix representation of its
 *  logic function.
 */

void
ex_node_to_sm(node, M)
register node_t *node;
register sm_matrix *M;
{
    register pset last, p;
    register int i, row;
    int *fanin_index;
    node_t *fanin;
    sm_element *pelement;
    value_cell_t *value_cell;

    foreach_set(node->F, last, p) {
	row = M->nrows;

	fanin_index = ALLOC(int, node->nin);
	foreach_fanin(node, i, fanin) {
	    fanin_index[i] = 2 * nodeindex_indexof(global_node_index, fanin);
	    assert(fanin_index[i] >= 0);
	}

	foreach_fanin(node, i, fanin) {
	    switch (GETINPUT(p, i)) {
	    case ZERO:
		pelement = sm_insert(M, row, fanin_index[i] + 1);
		break;
	    case ONE:
		pelement = sm_insert(M, row, fanin_index[i]);
		break;
	    case TWO:
		pelement = NIL(sm_element);
		break;
	    }

	    if (pelement != NIL(sm_element)) {
		value_cell = value_cell_alloc();
		value_cell->sis_index = 
				    nodeindex_indexof(global_node_index, node);
		assert(value_cell->sis_index >= 0);
		value_cell->cube_number = pelement->row_num;
		value_cell->value = 1;
		pelement->user_word = (char *) value_cell; 
	    }
	}

	FREE(fanin_index);
    }
}

sm_matrix *
ex_network_to_sm(network)
network_t *network;
{
    node_t *node;
    sm_matrix *M;
    lsGen gen;

    /* Convert the network into one sparse matrix */
    M = sm_alloc();
    foreach_node(network, gen, node) {
	if (node->type == INTERNAL) {
	    ex_node_to_sm(node, M);
	}
    }
    return M;
}

/*
 *  map a sparse-matrix representation of a function back into a 'node'
 */

node_t *
ex_sm_to_node(func)
sm_matrix *func;
{
    int i, nin;
    node_t *node, *fanin_node, **fanin;
    pset setp, fullset;
    pset_family F;
    sm_col *pcol;
    sm_row *prow;
    sm_element *p;

    /*
     *  Determine the fanin points;
     *  set pcol->flag to be the column number in the set family
     */
    node = node_alloc();
    nin = 0;
    fanin = ALLOC(node_t *, 2*func->ncols);	/* worst case */
    sm_foreach_col(func, pcol) {
	fanin_node = nodeindex_nodeof(global_node_index, pcol->col_num / 2);
	if ((pcol->col_num % 2) == 0) {
	    pcol->flag = 2 * nin;
	    fanin[nin++] = fanin_node;
	} else {
	    if (nin == 0 || fanin[nin-1] != fanin_node) {
		pcol->flag = 2 * nin + 1;
		fanin[nin++] = fanin_node;
	    } else {
		pcol->flag = 2 * (nin-1) + 1;
	    }
	}
    }

    /* Create the set family */
    F = sf_new(func->nrows, nin * 2);
    fullset = set_full(nin * 2);
    sm_foreach_row(func, prow) {
	setp = GETSET(F, F->count++);
	(void) set_clear(setp, nin * 2);
	sm_foreach_row_element(prow, p) {
	    i = sm_get_col(func, p->col_num)->flag;
	    set_insert(setp, i);
	}
	(void) set_diff(setp, fullset, setp);
    }
    set_free(fullset);

    node_replace_internal(node, fanin, nin, F);
    return node;		/* node already minimum base */
}

static int
ex_divide_each_fanout(fanout, cubes, newnode, debug)
array_t *fanout;
array_t *cubes;
node_t *newnode;
{
    int i, changed, count, fanout_index;
    pset_family old_F, dups;
    node_t *node, *old_fct, *t1;
    sm_row *dup_cubes;
    sm_element *p;

    if (debug > 1) {
	(void) fprintf(sisout, "\nnew factor is");
	node_print(sisout, newnode);
    }

    count = 0;
    for(i = 0; i < array_n(fanout); i++) {
	fanout_index = array_fetch(int, fanout, i);
	node = nodeindex_nodeof(global_node_index, fanout_index);
	assert(node != 0);

	if (debug > 1) {
	    (void) fprintf(sisout, "fanout #%d is %s\n", i, node_name(node));
	    (void) fprintf(sisout, "literals = %d\n", node_num_literal(node));
	    if (debug > 2) {
		node_print(sisout, node);
	    }
	}

	if (cubes != NIL(array_t)) {
	    /* duplicate some cubes from the original function */
	    dup_cubes = array_fetch(sm_row *, cubes, i);
	    if (dup_cubes->length > 0) {

		if (debug > 1) {
		    (void) fprintf(sisout, "covers cubes ");
		    sm_row_print(sisout, dup_cubes);
		    (void) fprintf(sisout, "\n");
		}

		old_fct = array_fetch(node_t *, global_old_fct, fanout_index);
		dups = sf_new(10, old_fct->nin * 2);
		sm_foreach_row_element(dup_cubes, p) {
		    dups = sf_addset(dups, GETSET(old_fct->F, p->col_num));
		}

		old_F = old_fct->F;
		old_fct->F = dups;

		if (debug > 1) {
		    (void) fprintf(sisout, "duplicating these cubes ...\n");
		    node_print(sisout, old_fct);
		}

		t1 = node_or(node, old_fct);
		node_replace(node, t1);
		sf_free(dups);
		old_fct->F = old_F;
			
		if (debug > 2) {
		    (void) fprintf(sisout, "after adding redundant cubes\n");
		    node_print(sisout, node);
		}
	    }
	}

	changed = node_substitute(node, newnode, use_complement);
	count += changed;

	if (debug > 1) {
	    if (! changed) {
		(void) fprintf(sisout, "***** DID NOT DIVIDE *****\n");
	    }
	    if (debug > 2) {
		(void) fprintf(sisout, "After weak division\n");
		node_print(sisout, node);
		(void) fprintf(sisout, "literals = %d\n", node_num_literal(node));
	    }
	}
    }
    return count;
}

/*
 *  insert sparse matrix function into the network
 *
 *  fanout is an array giving the indices of the nodes which this
 *	function is expected to divide;
 *  cubes actually gives the cubes of the function that are expected
 *  	to be covered by the division
 */

int
ex_divide_function_into_network(func, fanout, cubes, debug)
sm_matrix *func;
array_t *fanout;
array_t *cubes;
int debug;
{
    node_t *node;
    int index, count;

    /* convert the sparse matrix into a network node */
    node = ex_sm_to_node(func);

    /* perform the division into each node it divides */
    count = ex_divide_each_fanout(fanout, cubes, node, debug);

    /* for now, lets give warning if it divides less than we thought */
    if (debug && (count != array_n(fanout))) {
	(void) fprintf(sisout, 
	    "warning: divided only %d of %d\n", count, array_n(fanout));
    }

    if (count > 0) {
	network_add_node(global_network, node);
	index = nodeindex_insert(global_node_index, node);
    } else {
	node_free(node);
	index = -1;
    }

    /* BUG ! overlapping retangles may need a original_func for this guy ? */
    return index;
}
