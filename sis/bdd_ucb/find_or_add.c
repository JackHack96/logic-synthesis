
#include <stdio.h>    /* for BDD_DEBUG_LIFESPAN */

#include "util.h"
#include "array.h"
#include "st.h"

#include "bdd.h"
#include "bdd_int.h"


/*
 *    bdd_find_or_add - find a node in the hashtable or add it
 *
 *    Find a node whose id is given and which has T, E for its data.
 *    If no such node is in the table, then make a new one.
 *
 *    return the (possibly new) node in the hashtable
 */
bdd_node *
bdd_find_or_add(manager, variableId, T, E)
        bdd_manager *manager;
        bdd_variableId variableId;
        bdd_node *T;
        bdd_node *E;
{
    bdd_safeframe frame;
    bdd_safenode  safe_T, safe_E,
                  new, chain, ret;
    int           pos;

    BDD_ASSERT(T != NIL(bdd_node) || variableId == BDD_ONE_ID);
    BDD_ASSERT_NOT_BROKEN_HEART(T);
    BDD_ASSERT(!BDD_IS_COMPLEMENT(T));

    BDD_ASSERT(E != NIL(bdd_node) || variableId == BDD_ONE_ID);
    BDD_ASSERT_NOT_BROKEN_HEART(E);
    /* ASSERT: BDD_IS_COMPLEMENT(E) || ! BDD_IS_COMPLEMENT(E) */

    bdd_safeframe_start(manager, frame);
    bdd_safenode_link(manager, safe_T, T);
    bdd_safenode_link(manager, safe_E, E);
    bdd_safenode_declare(manager, new);
    bdd_safenode_declare(manager, chain);
    bdd_safenode_declare(manager, ret);

    /*
     *    Search the chain (short, its length is supposed
     *    to be less than BDD_HASHTABLE_MAXCHAINLEN).
     */

    pos = bdd_raw_node_hash(manager, variableId, T, E);

    for (chain.node = manager->heap.hashtable.buckets[pos];
         chain.node != NIL(bdd_node); chain.node = chain.node->next) {

        if (chain.node->id == variableId && chain.node->T == T && chain.node->E == E) {
            /*
             *    Found it, blow the frame and return the thing found.
                     *    Update the stats.
             */
            bdd_safeframe_end(manager);
            manager->heap.stats.cache.hashtable.hits++;
            return (chain.node);
        }
    }

    /*
     *    Its not there, so add it, but recall that adding a new node to
     *    the hashtable may cause a garbage-collection.   So, we must ensure
     *    that all values that we want preserved across the gc are tied down.
     *
     *    If one more entry will cause the table to explode,
     *    then make the table bigger before going on.
     */
    if (manager->heap.hashtable.nkeys + 1 >= manager->heap.hashtable.rehash_at_nkeys) {
        (void) bdd_resize_hashtable(manager);        /* definitely don't do THIS if gc is occurring */
    }

    ret.node = bdd_new_node(manager, variableId, T, E, NIL(bdd_node));

    /*
     *    Link in the new node. Recompute pos just in case a gc happenned (which would screw up the pos).
     */
    pos = bdd_raw_node_hash(manager, variableId, T, E);

    ret.node->next = manager->heap.hashtable.buckets[pos];
    manager->heap.hashtable.buckets[pos] = ret.node;
    manager->heap.hashtable.nkeys++;
    manager->heap.stats.cache.hashtable.misses++;

    bdd_safeframe_end(manager);
    return (ret.node);
}
