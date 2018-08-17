
/* file @(#)replace.c	1.2 */
/* last modified on 5/1/91 at 15:51:53 */

#include "sis.h"

/* simple optimization */
/* goes thru the network, and looks at */
/* all the 2-input OR gate nodes */
/* for each of them, remember their function in a simple */
/* structure. Only consider the nodes with fanout == 1 */
/* Then visit the network from inputs to outputs */
/* for each of these nodes, put them in a hash table */
/* using their function as key. If something is already in the hash table */
/* replace one node by the other. Then both of these nodes are no longer of interest */
/* (one is of fanout 2, the other one of fanout 0) so remove them. */
/* continue until no more substitution is possible */

static int fncmp();

static int get_literal();

static int node_skip();

static unsigned int fnhash();

static void extract_node_fn();

static void replace_node();

static void smaller_pointer_first();

typedef struct {
    node_t *fanin[2];
    int    phase[2];
} node_fn_t;


void replace_2or(network, verbose)
        network_t *network;
        int verbose;
{
    int          i;
    int          has_changed;
    st_generator *gen;
    node_t       *node, *replaced_node;
    node_fn_t    *fn;
    array_t      *nodevec    = network_dfs(network);
    st_table     *node_table = st_init_table(st_numcmp, st_numhash);
    st_table     *visited    = st_init_table(fncmp, (ST_PFICPI) fnhash);

    /* remove unnecessary buffers that may blur the computation of n_fanouts */
    network_csweep(network);

    for (i = 0; i < array_n(nodevec); i++) {
        node = array_fetch(node_t * , nodevec, i);
        /* store an easy-to-manipulate fn from the node */
        /* do it only if node is 2-input OR gate */
        /* and the fanout of the node is 1 */
        if (node_skip(node)) continue;
        extract_node_fn(node_table, node);
    }
    do {
        has_changed = 0;
        for (i      = 0; i < array_n(nodevec); i++) {
            node = array_fetch(node_t * , nodevec, i);
            if (node_skip(node)) continue;
            if (!st_lookup(node_table, (char *) node, (char **) &fn)) continue;
            if (!st_lookup(visited, (char *) fn, (char **) &replaced_node)) {
                st_insert(visited, (char *) fn, (char *) node);
                continue;
            }
            if (node == replaced_node) continue;
            if (verbose) (void) fprintf(sisout, "replace node %s by %s\n", replaced_node->name, node->name);
            replace_node(node_table, visited, node, replaced_node);
            /* the node "replaced_node" has been swept away at this point */
            has_changed = 1;
            st_delete(visited, (char **) &fn, NIL(
            char *));
            st_delete(node_table, (char **) &node, (char **) &fn);
            FREE(fn);
            st_delete(node_table, (char **) &replaced_node, (char **) &fn);
            FREE(fn);
        }
    } while (has_changed == 1);
    st_foreach_item(node_table, gen, (char **) &node, (char **) &fn)
    {
        FREE(fn);
    }
    st_free_table(node_table);
    st_free_table(visited);
    array_free(nodevec);
}

/* returns 1 iff node is not a 2-input OR gate */
/* with fanout of 1 */

static int node_skip(node)
        node_t *node;
{
    if (node_function(node) != NODE_OR) return 1;
    if (node_num_fanin(node) != 2) return 1;
    if (node_num_fanout(node) != 1) return 1;
    return 0;
}

static void extract_node_fn(table, node)
        st_table *table;
        node_t *node;
{
    int       i, j;
    int       n_cubes = node_num_cube(node);
    int       fn_array[2][2];
    node_fn_t *fn;

    assert(n_cubes == 2);
    assert(node_num_fanin(node) == 2);
    assert(node_function(node) == NODE_OR);
    fn = ALLOC(node_fn_t, 1);
    fn->fanin[0] = node_get_fanin(node, 0);
    fn->fanin[1] = node_get_fanin(node, 1);
    for (i = 0; i < 2; i++) {
        for (j = 0; j < 2; j++) {
            fn_array[i][j] = get_literal(node, i, j);    /* i->cube, j->literal */
        }
    }
    for (j = 0; j < 2; j++) {
        assert(fn_array[0][j] == 2 || fn_array[1][j] == 2);
        assert(fn_array[0][j] != 2 || fn_array[1][j] != 2);
        fn->phase[j] = (fn_array[0][j] == 2) ? fn_array[1][j] : fn_array[0][j];
    }
    smaller_pointer_first(fn);
    st_insert(table, (char *) node, (char *) fn);
}

static int get_literal(node, row, literal)
        node_t *node;
        int row;
        int literal;
{
    node_cube_t cube = node_get_cube(node, row);
    switch (node_get_literal(cube, literal)) {
        case ZERO:return 0;
        case ONE:return 1;
        default:return 2;
    }
}

static unsigned int fnhash(obj, modulus)
        char *obj;
        unsigned int modulus;
{
    node_fn_t    *fn = (node_fn_t *) obj;
    unsigned int n;

    n = (unsigned int) ((int) fn->fanin[0] + (int) fn->fanin[1] + fn->phase[0] * 2 + fn->phase[1]);
    return n % modulus;
}

static int fncmp(obj1, obj2)
        char *obj1;
        char *obj2;
{
    node_fn_t *fn1 = (node_fn_t *) obj1;
    node_fn_t *fn2 = (node_fn_t *) obj2;

    if (fn1->fanin[0] != fn2->fanin[0]) return 1;
    if (fn1->fanin[1] != fn2->fanin[1]) return 1;
    if (fn1->phase[0] != fn2->phase[0]) return 1;
    if (fn1->phase[1] != fn2->phase[1]) return 1;
    return 0;
}

/* a little bit convoluted */
/* have to make sure that the node "fanout", in which "replaced" */
/* is now replaced by "node" is correctly stored in node_table */
/* and visited table. To do so, need to change its function */
/* in node_table to make sure it reflects the change. */
/* if it was visited, we also need to remove that entry in visited */
/* and rehash it with the new function */

static void replace_node(node_table, visited, node, replaced)
        st_table *node_table;
        st_table *visited;
        node_t *node;
        node_t *replaced;
{
    node_fn_t *old_fn, *tmp, *new_fn;
    node_t    *visited_node;
    node_t    *fanout;
    int       was_visited;

    assert(node_num_fanout(replaced) == 1);
    fanout = node_get_fanout(replaced, 0);
    assert(node_substitute(replaced, node, 0));
    if (!st_lookup(node_table, (char *) fanout, (char **) &old_fn)) {
        network_sweep_node(fanout);
        return;
    }
    was_visited = 0;
    if (st_lookup(visited, (char *) old_fn, (char **) &visited_node)) {
        if (fanout == visited_node) {
            was_visited = 1;
            tmp         = old_fn;
            st_delete(visited, (char **) &old_fn, NIL(
            char *));
            assert(tmp == old_fn);
        }
    }
    if (old_fn->fanin[0] == replaced) {
        old_fn->fanin[0] = node;
    } else {
        assert(old_fn->fanin[1] == replaced);
        old_fn->fanin[1] = node;
    }
    smaller_pointer_first(old_fn);
    new_fn = old_fn;
    if (was_visited) st_insert(visited, (char *) new_fn, (char *) fanout);
    network_sweep_node(fanout);
}

/* makes sure that the smaller node pointer */
/* is stored first (for commutativity of OR) */
/* if not, swap */
static void smaller_pointer_first(fn)
        node_fn_t *fn;
{
    node_t *tmp_node;
    int    tmp_int;

    if ((int) fn->fanin[0] < (int) fn->fanin[1]) return;
    tmp_node = fn->fanin[1];
    fn->fanin[1] = fn->fanin[0];
    fn->fanin[0] = tmp_node;
    tmp_int = fn->phase[1];
    fn->phase[1] = fn->phase[0];
    fn->phase[0] = tmp_int;
}
