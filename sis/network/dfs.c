
#include "sis.h"

/*
 *  network_dfs -- order the nodes for a depth-first search from
 *  the outputs (all fanin's appear in the list before each node)
 *
 *  Crash and burn if a cycle is detected in the network.
 */

static int network_dfs_recur();

array_t *network_dfs(network_t *network) {
    int      i;
    st_table *visited;
    array_t  *roots;
    array_t  *node_vec;
    node_t   *node;
    lsGen    gen;

    visited  = st_init_table(st_ptrcmp, st_ptrhash);
    node_vec = array_alloc(node_t *, 0);
    roots    = array_alloc(node_t *, 0);

    foreach_primary_output(network, gen, node) {
        array_insert_last(node_t *, roots, node);
    }
    /* handle floating nodes */
    foreach_node(network, gen, node) {
        if (node_num_fanout(node) == 0 && node->type != PRIMARY_OUTPUT) {
            array_insert_last(node_t *, roots, node);
        }
    }
    for (i = 0; i < array_n(roots); i++) {
        node = array_fetch(node_t *, roots, i);
        if (!network_dfs_recur(node, node_vec, visited, 1, INFINITY)) {
            fail("network_dfs: network contains a cycle\n");
        }
    }
    st_free_table(visited);
    array_free(roots);
    return node_vec;
}

#ifdef SIS

/* Make sure that the vector returned has all the control po nodes
   BEFORE the latch output nodes.  This is so that the arrivals of the
   latch outputs can be computed based on the arrival of the clock and
   the delay through the latch of the clock. */

array_t *network_special_dfs(network_t *network) {
    int      i;
    st_table *visited;
    array_t  *roots;
    array_t  *node_vec;
    node_t   *node;
    lsGen    gen;

    visited  = st_init_table(st_ptrcmp, st_ptrhash);
    node_vec = array_alloc(node_t *, 0);
    roots    = array_alloc(node_t *, 0);

    foreach_primary_output(network, gen, node) {
        if (network_is_control(network, node)) {
            array_insert_last(node_t *, roots, node);
        }
    }
    foreach_primary_output(network, gen, node) {
        if (!network_is_control(network, node)) {
            array_insert_last(node_t *, roots, node);
        }
    }
    /* handle floating nodes */
    foreach_node(network, gen, node) {
        if (node_num_fanout(node) == 0 && node->type != PRIMARY_OUTPUT) {
            array_insert_last(node_t *, roots, node);
        }
    }
    for (i = 0; i < array_n(roots); i++) {
        node = array_fetch(node_t *, roots, i);
        if (!network_dfs_recur(node, node_vec, visited, 1, INFINITY)) {
            fail("network_dfs: network contains a cycle\n");
        }
    }
    st_free_table(visited);
    array_free(roots);
    return node_vec;
}

#endif /* SIS */

array_t *network_dfs_from_input(network_t *network) {
    int      i;
    st_table *visited;
    array_t  *node_vec;
    array_t  *roots;
    node_t   *node;
    lsGen    gen;

    visited  = st_init_table(st_ptrcmp, st_ptrhash);
    node_vec = array_alloc(node_t *, 0);
    roots    = array_alloc(node_t *, 0);

    foreach_primary_input(network, gen, node) {
        array_insert_last(node_t *, roots, node);
    }
    /* handle floating nodes */
    foreach_node(network, gen, node) {
        if (node_num_fanin(node) == 0 && node->type != PRIMARY_INPUT) {
            array_insert_last(node_t *, roots, node);
        }
    }
    for (i = 0; i < array_n(roots); i++) {
        node = array_fetch(node_t *, roots, i);
        if (!network_dfs_recur(node, node_vec, visited, 0, INFINITY)) {
            fail("network_dfs_from_input: network contains a cycle\n");
        }
    }
    st_free_table(visited);
    array_free(roots);
    return node_vec;
}

array_t *network_tfi(node_t *node, int level) {
    st_table *visited;
    array_t  *node_vec;
    node_t   *fanin;
    int      i;

    visited  = st_init_table(st_ptrcmp, st_ptrhash);
    node_vec = array_alloc(node_t *, 0);

    foreach_fanin(node, i, fanin) {
        if (!network_dfs_recur(fanin, node_vec, visited, 1, level)) {
            fail("network_tfi: network contains a cycle\n");
        }
    }

    st_free_table(visited);
    return node_vec;
}

array_t *network_tfo(node_t *node, int level) {
    st_table *visited;
    array_t  *node_vec;
    node_t   *fanout;
    lsGen    gen;

    visited  = st_init_table(st_ptrcmp, st_ptrhash);
    node_vec = array_alloc(node_t *, 0);

    foreach_fanout(node, gen, fanout) {
        if (!network_dfs_recur(fanout, node_vec, visited, 0, level)) {
            fail("network_tfo: network contains a cycle\n");
        }
    }

    st_free_table(visited);
    return node_vec;
}

static int network_dfs_recur(node_t *node, array_t *node_vec, st_table *visited, int dir, int level) {
    int    i;
    char   *value;
    node_t *fanin, *fanout;
    lsGen  gen;

    if (level > 0) {

        if (st_lookup(visited, (char *) node, &value)) {
            return value == 0; /* if value is 1, then a cycle */

        } else {
            /* add this node to the active path */
            value = (char *) 1;
            (void) st_insert(visited, (char *) node, value);

            /* avoid recursion if level-1 wouldn't add anything anyways */
            if (level > 1) {
                if (dir) {
                    foreach_fanin(node, i, fanin) {
                        if (!network_dfs_recur(fanin, node_vec, visited, dir, level - 1)) {
                            return 0;
                        }
                    }
                } else {
                    foreach_fanout(node, gen, fanout) {
                        if (!network_dfs_recur(fanout, node_vec, visited, dir, level - 1)) {
                            return 0;
                        }
                    }
                }
            }

            /* take this node off of the active path */
            value = (char *) 0;
            (void) st_insert(visited, (char *) node, value);

            /* add node to list */
            array_insert_last(node_t *, node_vec, node);
        }
    }
    return 1;
}
