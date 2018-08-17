#include "sis.h"
#include "../include/ntbdd_int.h"


/*
 * This file contains the routines to handle bdd_t * stored at network nodes,
 * There is a field (char *) on each node for that to store the bdd_t *'s.
 * Standard sis practice is to define a macro to access this field 
 * that does the type cast.
 *
 * Also standard sis practice is to have node demon that are automatically 
 * called whenever a node is created or freed.
 */

void bdd_alloc_demon(node)
        node_t *node;
{
    BDD_SET(node, NIL(bdd_t));
}

void bdd_free_demon(node)
        node_t *node;
{
    bdd_t *fn;

    fn = ntbdd_at_node(node);
    if (fn == NIL(bdd_t)) {
        return;
    }

    bdd_free(fn);
    BDD_SET(node, NIL(bdd_t));
}


/* 
 * A pair of routines to set and access the BDD field.
 * WATCHOUT: the resulting bdd_node may be NIL(bdd_node).
 */

bdd_t *ntbdd_at_node(node)
        node_t *node;
{
    if (node == NIL(node_t)) {
        fail("ntbdd_at_node: NIL node");
    } else {
        return BDD(node);
    }
}

/*
 * Have to record all the networks where BDD nodes 
 * of the current manager have been stored.
 * The invariant we guarantee is that the BDD(node) 
 * of a node of any network is put to NIL whenever 
 * the corresponding BDD manager is deallocated.
 * The last_network business is just a 1 slot LRU cache 
 * for the network_table to avoid too many hash table accesses.
 */
void ntbdd_set_at_node(node, new_fn)
        node_t *node;
        bdd_t *new_fn;    /* may be NIL */
{
    network_t          *network;
    bdd_manager        *manager;
    bdd_t              *old_fn;
    bdd_external_hooks *hooks;
    ntbdd_t            *ntbdd_data;

    old_fn = ntbdd_at_node(node);

    /*
     * If the fn has already been set at node, just return. Otherwise, free the
     * existing BDD (could be NIL) and set the new one.
     */
    if (old_fn == new_fn) {
        return;
    }

    if (old_fn != NIL(bdd_t)) {
        bdd_free(old_fn);
    }

    BDD_SET(node, new_fn);

    /*
     * Get the network of node. If the node does not have a network, return.
     */
    network = node_network(node);
    if (network == NIL(network_t)) return;

    /*
     * Get the ntbdd_data for the manager of the new BDD.  If the network was the last
     * inserted into the hooks network table, then we don't need to do anything.
     * Otherwise, we insert the network into the hooks network table.
     */
    manager    = bdd_get_manager(new_fn);
    hooks      = bdd_get_external_hooks(manager);
    ntbdd_data = (ntbdd_t *) hooks->network;

    if (ntbdd_data->last_network == network) return;
    ntbdd_data->last_network = network;
    st_insert(ntbdd_data->network_table, (char *) network, NIL(
    char));
}

/*
 * ntbdd_free_at_node - free the bdd associated with this node
 * This is the only function that should be called to free
 * a bdd which is assigned to a node.  Do not call bdd_free
 * on such nodes (or make sure that BDD(node) is set to 0).
 */
void ntbdd_free_at_node(node)
        node_t *node;
{
    bdd_t *fn;

    fn = ntbdd_at_node(node);
    if (fn == NIL(bdd_t)) {
        return;
    }
    bdd_free(fn);
    BDD_SET(node, NIL(bdd_t));
}


