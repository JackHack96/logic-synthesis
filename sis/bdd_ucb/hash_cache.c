
#include <stdio.h>    /* for BDD_DEBUG_LIFESPAN */

#include "util.h"
#include "array.h"
#include "st.h"

#include "bdd.h"
#include "bdd_int.h"


/*
 * NOTE: The "hashcache" should really have been named the "itecache", since it caches 
 * results of ITE operations.  The name "hashtable" has been changed to "itetable" in two 
 * places: bdd_stats->cache.itetable and manager->heap.cache.itetable.  However, many instances
 * of the term hashcache, such as the functions names below, persist.  Also, note that "hashtable"
 * is sometimes used to refer to the primary table for bdd_nodes that maintains uniqueness.
 */

static void resize_itetable();

/*
 *    bdd_hashcache_insert - perform an insertion into the cache
 *
 *    return nothing, just do it.
 */
void
bdd_hashcache_insert(manager, ITE_f, ITE_g, ITE_h, data)
        bdd_manager *manager;
        bdd_node *ITE_f;
        bdd_node *ITE_g;
        bdd_node *ITE_h;
        bdd_node *data;
{
    int                 pos;
    bdd_hashcache_entry *entry;

    BDD_ASSERT_NOTNIL(ITE_f);
    BDD_ASSERT_NOTNIL(ITE_g);
    BDD_ASSERT_NOTNIL(ITE_h);
    BDD_ASSERT_NOTNIL(data);

    if (!manager->heap.cache.itetable.on) {
        /*
         * Has no effect if caching is off.
         */
        return;
    }

    pos   = bdd_ITE_hash(manager, ITE_f, ITE_g, ITE_h);
    entry = manager->heap.cache.itetable.buckets[pos];

    if (entry != NIL(bdd_hashcache_entry)) {
        /*
         * An entry already exists at pos.  Since we are using an open-addressing
         * scheme, we will just reuse the memory of this entry.  Number of entries
         * stays constant.
         */
        manager->heap.stats.cache.itetable.collisions++;
    } else {
        /*
         * Must allocate a new entry, since one does not already exist at pos. It's possible
         * that allocating a bdd_hashcache_entry will cause the manager's
             * memory limit to be exceeded. However, we allow this to happen, because the alternative
         * is that we don't cache this call.  Not caching new calls could be deadly for runtime.
         */
        entry = ALLOC(bdd_hashcache_entry, 1);
        if (entry == NIL(bdd_hashcache_entry)) {
            (void) bdd_memfail(manager, "bdd_hashcache_insert");
        }
        manager->heap.cache.itetable.buckets[pos] = entry;
        manager->heap.cache.itetable.nentries++;
    }

    entry->ITE.f = ITE_f;
    entry->ITE.g = ITE_g;
    entry->ITE.h = ITE_h;
    entry->data  = data;

    manager->heap.stats.cache.itetable.inserts++;

    /*
     * Check to see if table needs resizing.  We base the decision on the ratio of the number of entries to the number
     * of buckets.
     */
    if ((unsigned int) (((float) manager->heap.cache.itetable.nentries
                         / (float) manager->heap.cache.itetable.nbuckets) * 100)
        > manager->heap.cache.itetable.resize_at) {
        (void) resize_itetable(manager);
    }
}

/*
 *    bdd_hashcache_lookup - perform a lookup in the cache
 *
 *    return {TRUE, FALSE} on {found, not found}.
 */
boolean
bdd_hashcache_lookup(manager, f, g, h, value)
        bdd_manager *manager;
        bdd_node *f;
        bdd_node *g;
        bdd_node *h;
        bdd_node **value;        /* return */
{
    int                 pos;
    bdd_hashcache_entry *entry;

    if (!manager->heap.cache.itetable.on) {
        /*
         * Always fails if caching is off.
         */
        return (FALSE);
    }

    pos   = bdd_ITE_hash(manager, f, g, h);
    entry = manager->heap.cache.itetable.buckets[pos];

    if (entry != NIL(bdd_hashcache_entry)) {
        /*
         * Entry exists at pos.  See if it matches.  If not (a miss) return.  If it does match
         * (a hit), then will drop out of IF and set value.
         */
        if (entry->data == NIL(bdd_node) || (entry->ITE.f != f || entry->ITE.g != g || entry->ITE.h != h)) {
            manager->heap.stats.cache.itetable.misses++;
            return (FALSE);
        }
    } else {
        /*
         * No entry exists at pos.  Definitely a miss.
         */
        manager->heap.stats.cache.itetable.misses++;
        return (FALSE);
    }

    manager->heap.stats.cache.itetable.hits++;

    *value = entry->data;
    return (TRUE);
}

/*
 * resize_itetable 
 *
 * Make the ITE cache larger.  Perform a rehash to map all of the old
 * stuff into this new larger world. Note that since we don't maintain collision chains,
 * some stuff may be lost.
 */
static void
resize_itetable(manager)
        bdd_manager *manager;
{
    unsigned int        old_nbuckets, next_hash_prime;
    bdd_hashcache_entry **old_buckets, *old_entry;
    int                 i, pos;

    /*
     * If the max_size has already been exceeded, then just return.
     */
    if (manager->heap.cache.itetable.nbuckets > manager->heap.cache.itetable.max_size) {
        return;
    }

    /*
     * Get the next hash prime.  If allocating a new table will cause the manager memory limit to 
     * be exceeded, then just return.  Trading off time in favor of space.
     */
    next_hash_prime = bdd_get_next_hash_prime(manager->heap.cache.itetable.nbuckets);
    if (bdd_will_exceed_mem_limit(manager, (next_hash_prime * sizeof(bdd_hashcache_entry *)), FALSE) == TRUE) {
        return;
    }

    /*
     * Save the old buckets.
     */
    old_nbuckets = manager->heap.cache.itetable.nbuckets;
    old_buckets  = manager->heap.cache.itetable.buckets;

    /*
     * Get the new size and allocate the new table.  We are growing by roughly a factor of two.
     * If there isn't sufficient memory, don't fail: the cache
     * is meant to enhance performance, but we can still continue even if we can't resize.
     */
    manager->heap.cache.itetable.nbuckets = next_hash_prime;
    manager->heap.cache.itetable.buckets  = ALLOC(bdd_hashcache_entry * , manager->heap.cache.itetable.nbuckets);
    if (manager->heap.cache.itetable.buckets == NIL(bdd_hashcache_entry *)) {
        manager->heap.cache.itetable.nbuckets = old_nbuckets;
        manager->heap.cache.itetable.buckets  = old_buckets;
        return;
    }

    /* 
     * Initialize all the entries to NIL.
     */
    for (i = 0; i < manager->heap.cache.itetable.nbuckets; i++) {
        manager->heap.cache.itetable.buckets[i] = NIL(bdd_hashcache_entry);
    }

    /*
     * Remap all of the ITE cache entries to the new larger cache.
     */
    manager->heap.cache.itetable.nentries = 0;
    for (i = 0; i < old_nbuckets; i++) {
        old_entry = old_buckets[i];
        if (old_entry != NIL(bdd_hashcache_entry)) {

            /*
             * Get the hash position for the entry in the new cache.  If the position is already
             * occupied, then old_entry will not be rehashed; it's lost;  If the position is not
             * occupied, then simply set the new cache bucket pointer to old_entry.
             */
            pos = bdd_hashcache_entry_hash(manager, old_entry);
            if (manager->heap.cache.itetable.buckets[pos] != NIL(bdd_hashcache_entry)) {
                FREE(old_entry);
            } else {
                manager->heap.cache.itetable.buckets[pos] = old_entry;
                manager->heap.cache.itetable.nentries++;
            }
        }
    }

    /*
     * Free the array of buckets.  Note that the entries of the table were simply 
     * moved over to the new table.
     */
    FREE(old_buckets);

}

