
/* file @(#)tree.c	1.2 */
/* last modified on 5/1/91 at 15:51:57 */
#include "sis.h"
#include "../include/map_int.h"


static int is_binary_tree();

static int tree_equal();

static void mark_isomorphic_nodes();


st_table *
map_find_isomorphisms(network)
        network_t *network;
{
    st_table *visited, *isomorphic_sons;
    node_t   *node;
    lsGen    gen;

    isomorphic_sons = st_init_table(st_ptrcmp, st_ptrhash);

    if (network_num_po(network) == 1) {
        visited = st_init_table(st_ptrcmp, st_ptrhash);

        /* actually, there is only 1 output */
        foreach_primary_output(network, gen, node)
        {
            if (is_binary_tree(node, visited)) {
                mark_isomorphic_nodes(node, isomorphic_sons);
            }
        }
        st_free_table(visited);
    }

    return isomorphic_sons;
}


static int
is_binary_tree(node, visited)
        node_t *node;
        st_table *visited;
{
    int    i;
    node_t *fanin;

    if (st_lookup(visited, (char *) node, NIL(char *))) {
        return 0;            /* is not a tree */
    } else {
        (void) st_insert(visited, (char *) node, NIL(
        char));
        if (node_num_fanin(node) > 2) {
            return 0;            /* is not unary or binary */
        }
        foreach_fanin(node, i, fanin)
        {
            if (!is_binary_tree(fanin, visited)) {
                return 0;
            }
        }
        return 1;        /* passes as a unary/binary tree */
    }
}


static int
tree_equal(node1, node2)
        node_t *node1, *node2;
{
    int eql;

    if (node_num_fanin(node1) != node_num_fanin(node2)) {
        return 0;
    }

    switch (node_num_fanin(node1)) {
        case 0: eql = 1;
            break;
        case 1: eql = tree_equal(node_get_fanin(node1, 0), node_get_fanin(node2, 0));
            break;
        case 2:
            eql = (tree_equal(node_get_fanin(node1, 0), node_get_fanin(node2, 0)) &&
                   tree_equal(node_get_fanin(node1, 1), node_get_fanin(node2, 1)))
                  ||
                  (tree_equal(node_get_fanin(node1, 0), node_get_fanin(node2, 1)) &&
                   tree_equal(node_get_fanin(node1, 1), node_get_fanin(node2, 0)));
            break;
        default: fail("bad node degree in tree_equal");
            /* NOTREACHED */
    }
    return eql;
}


static void
mark_isomorphic_nodes(node, isomorphic_sons)
        node_t *node;
        st_table *isomorphic_sons;
{
    int    i;
    node_t *fanin;

    foreach_fanin(node, i, fanin)
    {
        mark_isomorphic_nodes(fanin, isomorphic_sons);
    }

    if (node_num_fanin(node) == 2) {
        if (tree_equal(node_get_fanin(node, 0), node_get_fanin(node, 1))) {
            (void) st_insert(isomorphic_sons, (char *) node, NIL(
            char));
        }
    }
}
