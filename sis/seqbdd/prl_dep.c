

#ifdef SIS
#include "sis.h"
#include "prl_seqbdd.h"

static int      CheckInput   	     ARGS((network_t *, array_t *));
static array_t *ExtractPiTfo 	     ARGS((network_t *, node_t *));
static void     BlowAwayDependencies ARGS((array_t *, st_table *));

/*
 *----------------------------------------------------------------------
 *
 * Prl_RemoveDependencies -- EXPORTED ROUTINE
 *
 * Given an external PI and a list of external PO
 * remove the structural dependencies between the POs and the PI.
 * When the 'perform_check' flag is on, check that the dependencies
 * to be removed are structural but not logical.
 *
 * Results:
 *	1 iff the check fails.
 *
 * Side effects:
 * 	The network is modified in place.
 *
 *----------------------------------------------------------------------
 */

static prl_removedep_t prl_dep_options;

int Prl_RemoveDependencies(network, nodevec, options)
network_t *network;
array_t *nodevec;
prl_removedep_t *options;
{
    int i;
    node_t *pi, *po;
    array_t *pi_tfo;
    st_table *po_tfi;

    if (CheckInput(network, nodevec)) return 1;	   /* wrong input */
    if (array_n(nodevec) == 1) return 0;	   /* nothing to do */
    prl_dep_options = *options;

 /* 
  * Allocate:
  * 1. an array of nodes containing the TFI of 'pi'
  * 2. a hash table, that will eventually contain the TFO of the 'po'.
  *
  */

    pi = array_fetch(node_t *, nodevec, 0);
    pi_tfo = ExtractPiTfo(network, pi);
    po_tfi = st_init_table(st_ptrcmp, st_ptrhash);
    for (i = 1; i < array_n(nodevec); i++) {
	po = array_fetch(node_t *, nodevec, i);
	st_insert(po_tfi, (char *) po, NIL(char));
	if (prl_dep_options.verbosity) {
	    (void) fprintf(sisout, "PO \"%s\" is a root\n", io_name(po, 0));
	}
    }
    BlowAwayDependencies(pi_tfo, po_tfi);
    st_free_table(po_tfi);
    array_free(pi_tfo);

    /* 
     * Finally, sweep the network 
     */

    network_sweep(network);
    return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * CheckInput -- INTERNAL ROUTINE
 *
 * Given a list of nodes, make sure that the first node is an external PI
 * and the remaining nodes are external POs.
 *
 * Results:
 *	1 iff the check fails.
 *
 *----------------------------------------------------------------------
 */


static int CheckInput(network, nodevec)
network_t *network;
array_t *nodevec;
{
    int i;
    node_t *pi, *po;

    if (array_n(nodevec) == 0) return 1;
    pi = array_fetch(node_t *, nodevec, 0);
    if (! network_is_real_pi(network, pi)) return 1;
    for (i = 1; i < array_n(nodevec); i++) {
	po = array_fetch(node_t *, nodevec, i);
	if (! network_is_real_po(network, po)) return 1;
    }
    return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * ExtractPiTfo -- INTERNAL ROUTINE
 *
 * Given a primary input, returns an array of nodes containing
 * exactly those nodes in the transitive fanout of the PI.
 * The nodes are sorted in topological order.
 *
 * Results:
 *	An array of nodes.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static array_t *ExtractPiTfo(network, pi)
network_t *network;
node_t *pi;
{
    int       i, j;
    node_t   *node, *fanin;
    st_table *roots = st_init_table(st_ptrcmp, st_ptrhash);
    array_t  *pi_tfo = array_alloc(node_t *, 0);
    array_t  *nodevec = network_dfs(network);

    st_insert(roots, (char *) pi, NIL(char));
    array_insert_last(node_t *, pi_tfo, pi);
    for (i = 0; i < array_n(nodevec); i++) {
	node = array_fetch(node_t *, nodevec, i);
	foreach_fanin(node, j, fanin) {
	    if (! st_lookup(roots, (char *) fanin, NIL(char*))) continue;
	    st_insert(roots, (char *) node, NIL(char));
	    array_insert_last(node_t *, pi_tfo, node);
	    break;
	}
    }
    st_free_table(roots);
    array_free(nodevec);
    return pi_tfo;
}


static array_t *GetRootFanouts      ARGS((node_t *, st_table *));
static node_t  *ProcessInternalNode ARGS((node_t *, array_t *));
static void     ProcessPrimaryInput ARGS((node_t *, array_t *));

/*
 *----------------------------------------------------------------------
 *
 * BlowAwayDependencies -- INTERNAL ROUTINE
 *
 * Given an array of nodes in topological order,
 * and a set of roots at their outputs,
 * apply the following algorithm:
 * 1. visit the nodes from outputs to inputs (reverse order)
 * 1.a if the node is a primary output, skip it
 * 1.b examine the fanouts of the node. 
 * 1.c If none of the fanouts belong to the set 'root', skip the node.
 * 1.d If some of the fanouts belong to the set 'root', add the node
 *     to the set root. 
 * 1.e If all the fanouts belong to the set 'root', skip the node.
 * 1.f If some but not all of the fanouts belong to the set 'root',
 *     and the node is not a primary input,
 *     make a copy of the node.
 *     Move the fanouts of the node that do not belong to the set 'root'
 *     to the newly created node.
 * 1.g If some but not all of the fanouts belong to the set 'root',
 *     and the node is a primary input, create a constant node,
 *     move the fanouts of the node that belong to the set 'root'
 *     to the newly created node.
 * 2. sweep the network.   
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The network is modified in place.a
 *
 *----------------------------------------------------------------------
 */

static void BlowAwayDependencies(nodes, roots)
array_t *nodes;
st_table *roots;
{
    int i;
    int all_roots;
    int none_roots;
    node_t *node;
    node_t *root;
    array_t *root_fanouts;

    for (i = array_n(nodes) - 1; i >= 0; i--) {
	node = array_fetch(node_t *, nodes, i);
	if (prl_dep_options.verbosity) {
	    (void) fprintf(sisout, "process node \"%s\"\n", node->name);
	}
	if (node->type == PRIMARY_OUTPUT) continue;
	root_fanouts = GetRootFanouts(node, roots);
	if (node->type == INTERNAL) {
	    root = ProcessInternalNode(node, root_fanouts);
	    st_insert(roots, (char *) root, NIL(char));
	    if (prl_dep_options.verbosity) {
		(void) fprintf(sisout, "node \"%s\" is a new root\n", node->name);
	    }
	} else if (node->type == PRIMARY_INPUT) {
	    ProcessPrimaryInput(node, root_fanouts);
	}
	array_free(root_fanouts);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * GetRootFanouts -- INTERNAL ROUTINE
 *
 * Results:
 *     An array of the non root fanouts of 'node'.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
	
static array_t *GetRootFanouts(node, roots)
node_t *node;
st_table *roots;
{	
    lsGen gen;
    node_t *fanout;
    array_t *root_fanouts = array_alloc(node_t *, 0);

    foreach_fanout(node, gen, fanout) {
	if (st_lookup(roots, (char *) fanout, NIL(char *))) {
	    array_insert_last(node_t *, root_fanouts, fanout);
	}
    }
    return root_fanouts;
}


static void PatchFanin ARGS((array_t *, node_t *, node_t *));

/*
 *----------------------------------------------------------------------
 *
 * ProcessInternalNode -- INTERNAL ROUTINE
 *
 * Results:
 *     The node that supports the nodes in 'root_fanouts', if any.
 *
 * Side effects:
 *	The node may be duplicated.
 *
 *----------------------------------------------------------------------
 */

static node_t *ProcessInternalNode(node, root_fanouts)
node_t *node;
array_t *root_fanouts;
{
    node_t *copy;

    if (array_n(root_fanouts) == 0) return NIL(node_t);
    if (array_n(root_fanouts) == node_num_fanout(node)) return node;
    copy = node_dup(node);
    if (copy->name != NIL(char)) {
	FREE(copy->name);
	copy->name = NIL(char);
    }
    network_add_node(node->network, copy);
    PatchFanin(root_fanouts, node, copy);
    return copy;
}


/*
 *----------------------------------------------------------------------
 *
 * ProcessPrimaryInput -- INTERNAL ROUTINE
 *
 * Results:
 *     None.
 *
 * Side effects:
 *	A constant is inserted to feed the nodes in 'root_fanouts'.
 *
 *----------------------------------------------------------------------
 */

static void ProcessPrimaryInput(node, root_fanouts)
node_t *node;
array_t *root_fanouts;
{
    node_t *copy;

    if (array_n(root_fanouts) == 0) return;
    copy = node_constant(prl_dep_options.insert_a_one);
    network_add_node(node->network, copy);
    PatchFanin(root_fanouts, node, copy);
}

/*
 *----------------------------------------------------------------------
 *
 * PatchFanin -- INTERNAL ROUTINE
 *
 * Results:
 *     None.
 *
 * Side effects:
 *	Replaces 'old' by 'new' as an input for all nodes in 'nodevec'.
 *
 *----------------------------------------------------------------------
 */

static void PatchFanin(nodevec, old, new)
array_t *nodevec;
node_t *old;
node_t *new;
{
    int i;
    node_t *node;

    for (i = 0; i < array_n(nodevec); i++) {
	node = array_fetch(node_t *, nodevec, i);
	(void) node_patch_fanin(node, old, new);
	node_scc(node);
	if (prl_dep_options.verbosity) {
	    (void) fprintf(sisout, 
			   "move input of \"%s\" from \"%s\" to \"%s\"\n", 
			   node->name, 
			   old->name, 
			   new->name);
	}
    }
}

#endif /* SIS */
