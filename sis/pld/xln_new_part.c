
#include "sis.h"
#include "../include/pld_int.h"

extern int                XLN_DEBUG;

struct sol_struct {
    int    value;
    node_t *in, *out;
};
typedef struct sol_struct sol_t;

static int cost_fanin();

static int composite_fanin();

/* new improved routine: hopefully better, faster and smoother */
imp_part_network(network, size, MOVE_FANINS, MAX_FANINS
)
network_t *network;
int       size;
int       MOVE_FANINS;
int       MAX_FANINS;
{
int      no_nodes;
int      i, xln_comp(), l;
node_t   *node, *fi, *fo;
sol_t    *sol;
lsGen    gen, gen1;
st_table *table;
array_t  *cover, *array_disjoint();
array_t  *values;
int      changed;

changed = 1;
while (changed){
changed = 0;
if (
xln_do_trivial_collapse_network(network, size, MOVE_FANINS, MAX_FANINS
))
changed  = 1;
no_nodes = network_num_internal(network);
table    = st_init_table(st_ptrcmp, st_ptrhash);
l        = 0;
values   = array_alloc(sol_t * , 0);
foreach_node(network, gen, node
){
if ((
node_function(node)
== NODE_PI)||(
node_function(node)
== NODE_PO)) continue;
st_insert(table,
(char *)node, (char *)l);
l++;
foreach_fanin(node, i, fi
){
if (
node_function(fi)
== NODE_PI) continue;
if (
composite_fanin(node, fi
) <= size){
sol = ALLOC(sol_t, 1);
sol->
in = fi;
sol->
out = node;
sol->
value = 0;
foreach_fanout(fi, gen1, fo
){
sol->value +=
cost_fanin(fo, fi
);
}
(void)
array_insert_last(sol_t
*, values, sol);
}
}
}

if (
array_n(values)
== 0) {
array_free(values);
continue;
}
array_sort(values, xln_comp
);
cover   = array_disjoint(values, no_nodes, table);
changed = 1;
if (
array_n(cover)
== 0){
(void)

error_init();

(void)error_append("array values was not zero, disjoint array is\n");
assert(0);
}
for (
i = 0;
i <
array_n(cover);
i++){
sol  = array_fetch(sol_t * , cover, i);
fi   = sol->in;
node = sol->out;
if (
composite_fanin(node, fi
) <= size){
if (
node_collapse(node, fi
)){
if (XLN_DEBUG > 1){
(void)
fprintf(sisout,
"collapsed %s into %s\n",
node_long_name(fi), node_long_name(node)
);
}
}
else {
(void) printf("could not collapse %s into %s\n",
node_long_name(fi), node_long_name(node)
);
exit(1);
}
}
else {
(void) printf("composite fanin of (%s, %s) > %d\n",
node_long_name(fi), node_long_name(node), size
);
exit(1);
}
}
st_free_table(table);
for (
i = 0;
i <
array_n(cover);
i++){
sol = array_fetch(sol_t * , cover, i);
FREE(sol);
}
array_free   (cover);
array_free   (values);
network_sweep(network);
}
}


array_t *
array_disjoint(a, s, t)
        array_t *a;
        int s;
        st_table *t;
{
    int     i, *n_out;
    int     j, k;
    array_t *new_a;
    sol_t * sol;
    char *dummy;

    n_out = ALLOC(
    int, s);
    for (i = 0; i < s; i++) {
        n_out[i] = 0;
    }

    new_a  = array_alloc(sol_t * , 0);
    for (i = 0; i < array_n(a); i++) {
        sol = array_fetch(sol_t * , a, i);
        if (st_lookup(t, (char *) sol->in, &dummy)) {
            (void) error_init();
            (void) error_append("Hello node number not found\n");
        }
        j = (int) dummy;
        if (st_lookup(t, (char *) sol->out, &dummy)) {
            (void) error_init();
            (void) error_append("Hello node number not found\n");
        }
        k = (int) dummy;
        (void) error_init();
        (void) error_append("index size out of array\n");
        assert(j < s);
        assert(k < s);
        if ((n_out[j] == 0) && (n_out[k] == 0)) {
            array_insert_last(sol_t * , new_a, sol);
            n_out[k] = 1;
        } else {
            FREE(sol);
        }
    }
    FREE(n_out);
    return new_a;
}




int
xln_comp(o1, o2)
        sol_t **o1, **o2;
{
    sol_t * sol1, *sol2;
    sol1 = *o1;
    sol2 = *o2;

    if (sol1->value == sol2->value) return 0;
    if (sol1->value > sol2->value) return 1;
    return -1;
}

/*------------------------------------------------------------------------------
  Collapsing nodes which can be collapsed into all their fanouts without 
  getting any infeasibilities. The nodes collapsed are deleted from the network.
  Returns the number of collapsed nodes.
-------------------------------------------------------------------------------*/
int
xln_do_trivial_collapse_network(network, size, MOVE_FANINS, MAX_FANINS)
        network_t *network;
        int size;
        int MOVE_FANINS; /* if want to move fanins */
        int MAX_FANINS;
{
    int num_collapsed;
    int changed;
    int num_collapsed_iter;

    num_collapsed = 0;
    changed       = 1;
    while (changed) {
        num_collapsed_iter =
                xln_do_trivial_collapse_network_one_iter(network, size,
                                                         MOVE_FANINS, MAX_FANINS);
        num_collapsed += num_collapsed_iter;
        changed            = num_collapsed_iter;
    }
    if (XLN_DEBUG > 1) {
        (void) printf("---collapsed %d nodes in trivial collapse\n", num_collapsed);
    }
    return num_collapsed;
}

int
xln_do_trivial_collapse_network_one_iter(network, size, MOVE_FANINS, MAX_FANINS)
        network_t *network;
        int size;
        int MOVE_FANINS; /* if want to move fanins */
        int MAX_FANINS;
{
    int     num_collapsed;
    int     is_collapsed;
    int     i;
    array_t *nodevec;
    node_t  *node;

    num_collapsed = 0;
    nodevec       = network_dfs(network);

    for (i = 0; i < array_n(nodevec); i++) {
        node         = array_fetch(node_t * , nodevec, i);
        is_collapsed = xln_do_trivial_collapse_node(node, size, MOVE_FANINS, MAX_FANINS);
        if (is_collapsed == 1) {                   /* can collapse */
            network_delete_node(network, node);
            num_collapsed++;
        }
    }
    array_free(nodevec);
    if (XLN_DEBUG > 1) {
        (void) printf("---collapsed %d nodes in trivial collapse one iter\n",
                      num_collapsed);
    }
    return num_collapsed;
}

/*----------------------------------------------------------------------------
  If the node can be collapsed into its fanouts, maintaining feasibility,
  returns 1. If node is not INTERNAL, or has a fanout that is PO, returns 0.
  If MOVE_FANINS = 1, then it tries to move fanins of the node around, so that
  feasibility may be achieved. If some fanins are moved (using Roth-Karp 
  decomp.), then it tries to do collapse this node again into its fanouts.
  If it succeeds, it is fine (return 1), else return 0.
  Note: If collapse is possible, the fanout nodes are changed.
-------------------------------------------------------------------------------*/
int
xln_do_trivial_collapse_node(node, size, MOVE_FANINS, MAX_FANINS)
        node_t *node;
        int size;
        int MOVE_FANINS, MAX_FANINS;
{
    int is_collapsed;

    if (MOVE_FANINS) {
        /* returns a flag telling if it moved some fanins. */
        /*-------------------------------------------------*/
        (void) xln_node_move_fanins(node, size, MAX_FANINS, MAXINT);
    }
    is_collapsed = xln_do_trivial_collapse_node_without_moving(node, size);
    if (is_collapsed <= 0) return 0; /* either not INTERNAL, or some fo PO, or not
                                      collapsible */
    return 1; /* can collapse */
}

/*--------------------------------------------------------------------------
  Returns 1 if the node can be collapsed into all the fanouts, 
  maintaining feasibility of the fanout nodes. It collapses the node into
  all the fanouts, so the functions of fanout nodes change. However, does
  not delete the node from the network. Otherwise returns 
    0: either the node is not INTERNAL, or some fanout is PO.
    -ve number diff: diff tells by how much was the feasibility missed.
  In last two cases, there is no change in the network. 
---------------------------------------------------------------------------*/
int
xln_do_trivial_collapse_node_without_moving(node, size)
        node_t *node;
        int size;
{
    int     j;
    array_t *fo_array;
    node_t  *fo, *fo_simpl;
    lsGen   gen;
    int     diff, diff_fo;
    int     flag;

    if (node->type != INTERNAL) return 0;
    fo_array = array_alloc(node_t * , 0);
    flag     = 1;
    diff     = MAXINT;
    foreach_fanout(node, gen, fo)
    {
        if (fo->type == PRIMARY_OUTPUT) {
            lsFinish(gen);
            array_free(fo_array);
            return 0;
        }
        diff_fo = size - composite_fanin(fo, node);
        if (diff_fo < 0) {
            diff = min(diff_fo, diff);
            flag = 0;
        } else {
            array_insert_last(node_t * , fo_array, fo);
        }
    }
    /* flag = 0 means the node cannot be collapsed because of infeasibility */
    /*----------------------------------------------------------------------*/
    if (flag == 0) {
        array_free(fo_array);
        return diff;
    }
    /* number of fanouts = 0 */
    /*-----------------------*/
    if (array_n(fo_array) == 0) {
        if (XLN_DEBUG) {
            (void) printf("warning-- node %s does not fanout anywhere\n",
                          node_long_name(node));
        }
        array_free(fo_array);
        return 0;
    }
    assert(array_n(fo_array) > 0);
    for (j = 0; j < array_n(fo_array); j++) {
        fo = array_fetch(node_t * , fo_array, j);
        node_collapse(fo, node);
        fo_simpl = node_simplify(fo, NIL(node_t), NODE_SIM_ESPRESSO);
        node_replace(fo, fo_simpl);
    }
    array_free(fo_array);
    return 1;
}


/* Finds number of distinct fanins of fanin- fanin is a fanin to po
 * ie should fanin be merged into po then the number of wires that 
 * would have to be routed to po in addition to the fanins of po*/

static int
cost_fanin(po, fanin)
        node_t *po, *fanin;
{
    node_t *fi;
    int    j, cost;

    cost = node_num_fanin(fanin);
    foreach_fanin(po, j, fi)
    {
        if (node_get_fanin_index(fanin, fi) >= 0) cost--;
    }
    return (cost);
}


static int
composite_fanin(node1, node2)
        node_t *node1, *node2;
{
    node_t *fi;
    int    inp, j;

    inp = node_num_fanin(node1) - 1;
    foreach_fanin(node2, j, fi)
    {
        if (node_get_fanin_index(node1, fi) < 0) inp++;
    }
    return (inp);
}
 	
