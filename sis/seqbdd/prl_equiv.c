

#ifdef SIS
#include "sis.h"
#include "prl_util.h"

 /* EXTERNAL INTERFACE */

 /* 'equiv_nets' performs the following optimization:
  * 1. for each net in the network, computes the BDD representing
  *    the Boolean function of that net in terms of PIs.
  * 2. use the external don't care (e.g. reachability)
  *    and cofactor the BDD at each net by the EXDC.
  * 3. group the nets by equivalence class, were n1 and n2
  *    are equivalent iff they are always equal or always
  *    the negation of each other in the don't care set.
  * 4. for each equivalence class, select a net of smallest depth
  *    and move the fanouts of all the other nets of that class
  *    to that net, inserting inverters whenever necessary.
  * 5. sweeps the network to remove the dead logic.
  *
  * We take the Boolean and of all output don't cares.
  *
  * Since it may remove latches on which the dc_network may depend
  * this routine removes the dc_network when it is done.
  */

static void   equiv_nets    ARGS((network_t *, seq_info_t *, bdd_t *, prl_options_t *));

void Prl_EquivNets(network, options)
network_t *network;
prl_options_t *options;
{
  seq_info_t *seq_info;
  bdd_t *reachable_states, *seq_dc;

  seq_info = Prl_SeqInitNetwork(network, options);
  seq_dc = Prl_GetSimpleDc(seq_info);
  if (network->dc_network != NIL(network_t) && bdd_is_tautology(seq_dc, 0)) {
      (void) fprintf(siserr, "no reachability information found in don't care network\n");
  }
  equiv_nets(network, seq_info, seq_dc, options);
  bdd_free(seq_dc);
  Prl_SeqInfoFree(seq_info, options);
  Prl_RemoveDcNetwork(network);
}


 /* INTERNAL INTERFACE */

typedef struct net_info_struct net_info_t;

struct net_info_struct {
  node_t *node;			       /* source of the net */
  int index;			       /* index of the net in the array of nets */
  int root_index;		       /* smallest index of an equivalent net */
  array_t *equiv_array;		       /* array of nets equivalent to this one; empty if index > root_index */
  net_info_t *best_member;	       /* best node to be taken as source among all nodes in equiv_array */
  long depth;			       /* depth of source of this node */
  long cost;			       /* cost of using this node as source for all equiv nets */
  bdd_t *fn;			       /* value of the net; after cofactoring */
  bdd_t *inv_fn;		       /* not(value) of the net; after cofactoring */
};

static void initialize_net_entry 	ARGS((seq_info_t *, net_info_t *, node_t *, int, bdd_t *));
static void compute_equivalence_classes ARGS((int, net_info_t *));
static void report_equivalence_classes  ARGS((int, net_info_t *));
static int  skip_this_net 		ARGS((net_info_t *));
static void collapse_equiv_nets 	ARGS((net_info_t *, prl_options_t *));

static void equiv_nets(network, seq_info, dc, options)
network_t *network;
seq_info_t *seq_info;
bdd_t *dc;
prl_options_t *options;
{
    int i;
    lsGen gen;
    int node_count;
    int n_nets;
    node_t *node;
    net_info_t *net_info;
    bdd_t *care_set;

    n_nets = network_num_internal(network) + network_num_pi(network);
    net_info = ALLOC(net_info_t, n_nets);
    node_count = 0;
    care_set = bdd_not(dc);

    if (options->verbose > 0) {
	(void) fprintf(sisout, "build BDDs for equivalence computation ... \n");
    }
    foreach_node(network, gen, node) {
	if (node->type == PRIMARY_OUTPUT) continue;
	initialize_net_entry(seq_info, &(net_info[node_count]), node, node_count, care_set);
	node_count++;
    }
    assert(node_count == n_nets);
    if (options->verbose > 0) {
	(void) fprintf(sisout, "compute equivalence classes ... \n");
    }
    compute_equivalence_classes(n_nets, net_info);
    if (options->verbose > 0) {
	report_equivalence_classes(n_nets, net_info);
    }
    if (options->verbose > 0) {
	(void) fprintf(sisout, "collapse equivalent nets ... \n");
    }
    for (i = 0; i < n_nets; i++) {
	if (skip_this_net(&(net_info[i]))) continue;
	collapse_equiv_nets(&(net_info[i]), options);
    }
    
    bdd_free(care_set);
    for (i = 0; i < n_nets; i++) {
	array_free(net_info[i].equiv_array);
    }
    FREE(net_info);
}

 /* skip all entries that either are not equivalence classes */
 /* or are equivalence classes with only one element */

static int skip_this_net(net_info)
net_info_t *net_info;
{
    if (net_info->root_index < net_info->index) return 1;
    if (array_n(net_info->equiv_array) <= 1) return 1;
    return 0;
}

static void initialize_net_entry(seq_info, net_info, node, index, care_set)
seq_info_t *seq_info;
net_info_t *net_info;
node_t *node;
int index;
bdd_t *care_set;
{
    bdd_t *fn;

    net_info->node = node;
    net_info->index = index;
    net_info->root_index = index;
    net_info->cost = INFINITY;
    net_info->best_member = NIL(net_info_t);
    net_info->equiv_array = array_alloc(net_info_t *, 0);
    array_insert_last(net_info_t *, net_info->equiv_array, net_info);
    fn = ntbdd_node_to_bdd(node, seq_info->manager, seq_info->leaves);
    assert(fn != NIL(bdd_t));
    if (bdd_is_tautology(care_set, 0)) {
	net_info->fn = bdd_dup(care_set);
	net_info->inv_fn = bdd_not(care_set);
    } else {
	net_info->fn = bdd_cofactor(fn, care_set);
	net_info->inv_fn = bdd_not(net_info->fn);
    }
}

 /* 
 * we use a couple of invariants:
 * (1) root_index < index iff visited net belongs to an equiv class
 * with at least one member with smaller index. The smallest index
 * among all members of this class is root_index.
 */

static void compute_equivalence_classes(n_nets, net_info)
int n_nets;
net_info_t *net_info;
{
    int i, j;

    for (i = 0; i < n_nets; i++) {
	assert(net_info[i].index == i);
	if (net_info[i].root_index < net_info[i].index) continue;
	for (j = i + 1; j < n_nets; j++) {
	    if (net_info[j].root_index < net_info[j].index) continue;
	    if (bdd_equal(net_info[i].fn, net_info[j].fn)) {
		array_insert_last(net_info_t *, net_info[i].equiv_array, &(net_info[j]));
		net_info[j].root_index = i;
	    } else if (bdd_equal(net_info[i].inv_fn, net_info[j].fn)) {
		array_insert_last(net_info_t *, net_info[i].equiv_array, &(net_info[j]));
		net_info[j].root_index = i;
	    }
	}
    }
}

/*
 * Prints the equivalence classes on stdout.
 */

static void report_equivalence_classes(n_nets, net_info)
int n_nets;
net_info_t *net_info;
{
    int i;
    int max_size;
    int *stats;
    int n_classes;
    
    max_size = 1;
    for (i = 0; i < n_nets; i++) {
	max_size = MAX(max_size, array_n(net_info[i].equiv_array));
    }

    n_classes = 0;
    stats = ALLOC(int, max_size + 1);
    for (i = 0; i <= max_size; i++) {
	stats[i] = 0;
    }
    for (i = 0; i < n_nets; i++) {
	if (net_info[i].root_index < net_info[i].index) continue;
	stats[array_n(net_info[i].equiv_array)]++;
	n_classes++;
    }
    (void) fprintf(sisout, "Report on net equivalence classes:\n");
    (void) fprintf(sisout, "Total Number of Classes: %d\n", n_classes);
    if (n_classes > 0) {
	for (i = 1; i <= max_size; i++) {
	    (void) fprintf(sisout, "classes of size %d: %d %2.2f%%\n", 
			   i, 
			   stats[i], 
			   100 * (double) stats[i] / n_classes);
	}
    }
    FREE(stats);
}

/* 
 * Work on an equivalence class basis.
 * First: compute the costs: based on depth, then on amount of logic that can be removed. 
 * depth is important: guarantees that we are not creating cycles.
 *
 * For each equivalence class containing more than one element
 * move the fanouts of all members to the best_member of the class.
 * 'best_member' has been computing earlier, as the best choice within the class.
 */

static void compute_node_cost_rec ARGS((node_t *, st_table *, st_table *));
static void MoveFanout ARGS((node_t *, node_t *));

static void collapse_equiv_nets(net_info, options)
net_info_t *net_info;
prl_options_t *options;
{
    int i;
    int is_inverted;
    int best_cost, best_depth;
    node_t *inv_node, *source_node;
    net_info_t *net;
    st_table *cost_table, *depth_table;

    /* compute the best node to be used as root for all of the equiv nets */

    cost_table = st_init_table(st_ptrcmp, st_ptrhash);
    depth_table = st_init_table(st_ptrcmp, st_ptrhash);
    best_cost = INFINITY;
    best_depth = INFINITY;

    for (i = 0; i < array_n(net_info->equiv_array); i++) {
	net = array_fetch(net_info_t *, net_info->equiv_array, i);
	compute_node_cost_rec(net->node, cost_table, depth_table);
	assert(st_lookup(cost_table, (char *) net->node, (char **) &(net->cost))); /* long cost */
	assert(st_lookup(depth_table, (char *) net->node, (char **) &(net->depth))); /* long depth */
	if (options->verbose > 0) {
	    (void) fprintf(sisout, "source %s depth=%d cost=%d\n", net->node->name, net->depth, net->cost);
	}
	if (net->depth < best_depth || (net->depth == best_depth && net->cost < best_cost)) {
	    best_depth = net->depth;
	    best_cost = net->cost;
	    net_info->best_member = net;
	}
    }
    st_free_table(cost_table);
    st_free_table(depth_table);
    

    /* perform the collapsing */

    source_node = net_info->best_member->node;
    inv_node = node_literal(source_node, 0);
    network_add_node(node_network(source_node), inv_node);

    for (i = 0; i < array_n(net_info->equiv_array); i++) {
	net = array_fetch(net_info_t *, net_info->equiv_array, i);
	if (bdd_equal(net->fn, net_info->best_member->fn)) {
	    MoveFanout(net->node, source_node);
	} else {
	    assert(bdd_equal(net->fn, net_info->best_member->inv_fn));
	    MoveFanout(net->node, inv_node);
	}
	if (options->verbose > 0) {
	    (void) fprintf(sisout, "move fanout of node %s to node %s ", net->node->name, source_node->name);
	    if (bdd_equal(net->fn, net_info->best_member->inv_fn)) {
		(void) fprintf(sisout, "(inverted -> %s)", inv_node->name);
	    } 
	    (void) fprintf(sisout, "\n");
	}
    }
}

static void compute_node_cost_rec(node, cost_table, depth_table)
node_t *node;
st_table *cost_table;
st_table *depth_table;
{
    int i;
    long fanin_cost, fanin_depth;
    long cost, depth;
    node_t *fanin;

    if (st_lookup(cost_table, (char *) node, NIL(char *))) return;
    cost = (node_num_fanout(node) == 1) ? node_num_literal(node) : 0;
    depth = 0;
    foreach_fanin(node, i, fanin) {
	compute_node_cost_rec(fanin, cost_table, depth_table);
	assert(st_lookup(cost_table, (char *) fanin, (char **) &fanin_cost)); /* long fanin_cost */
	assert(st_lookup(depth_table, (char *) fanin, (char **) &fanin_depth));	/* long fanin_depth */
	cost += fanin_cost;
	depth = MAX(depth, 1 + fanin_depth);
    }
    st_insert(cost_table, (char *) node, (char *) cost);
    st_insert(depth_table, (char *) node, (char *) depth);
}


/* 
 *----------------------------------------------------------------------
 *
 * MoveFanout -- INTERNAL ROUTINE
 *
 * Takes the fanout of 'from' and move it to 'to'.
 * The copying of the fanouts into an array is necessary:
 * not a good idea to iterate on a list while each iteration
 * removes an element of the list. Safer this way.
 * The call to 'node_scc' is necessary to make sure that
 * there is no duplicated fanin after the optimization.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	1. all the fanouts of 'from' are moved to 'to'.
 *	2. all fanout nodes are minimized using 'node_scc'.
 *
 *----------------------------------------------------------------------
 */

static void MoveFanout(from, to)
node_t *from;
node_t *to;
{
    int i;
    lsGen gen;
    int n_fanouts;
    node_t *fanout;
    node_t **fanouts;

    if (from == to) return;
    n_fanouts = node_num_fanout(from);
    fanouts = ALLOC(node_t *, n_fanouts);
    i = 0;

    foreach_fanout(from, gen, fanout) {
	fanouts[i++] = fanout;
    }

    for (i = 0; i < n_fanouts; i++) {
	(void) node_patch_fanin(fanouts[i], from, to);
	node_scc(fanouts[i]);
    }

    FREE(fanouts);
}
#endif /* SIS */

