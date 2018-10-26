
#include <stdio.h>	/* for BDD_DEBUG_LIFESPAN */

#include "util.h"
#include "array.h"
#include "st.h"

#include "bdd.h"
#include "bdd_int.h"



static void resize_consttable();

/*
 *    bdd_constcache_insert - perform an insertion into the constcache
 *
 *    return nothing, just do it.
 */
void
bdd_constcache_insert(manager, ITE_f, ITE_g, ITE_h, data)
bdd_manager *manager;
bdd_node *ITE_f;
bdd_node *ITE_g;
bdd_node *ITE_h;
bdd_constant_status data;
{
    int pos;
    bdd_constcache_entry *entry;

    BDD_ASSERT_NOTNIL(ITE_f);
    BDD_ASSERT_NOTNIL(ITE_g);
    BDD_ASSERT_NOTNIL(ITE_h);

    if ( ! manager->heap.cache.consttable.on ) {
        /*
         * Has no effect if caching is off.
         */
	return;		
    }

    pos = bdd_const_hash(manager, ITE_f, ITE_g, ITE_h);
    entry = manager->heap.cache.consttable.buckets[pos];

    if (entry != NIL(bdd_constcache_entry)) {
	/*
	 * An entry already exists at pos.  Since we are using an open-addressing
	 * scheme, we will just reuse the memory of this entry.  Number of entries
	 * stays constant.
	 */
	manager->heap.stats.cache.consttable.collisions++;
    } else {
	/*
	 * Must allocate a new entry, since one does not already exist at pos. It's possible
	 * that allocating a bdd_hashcache_entry will cause the manager's 
         * memory limit to be exceeded. However, we allow this to happen, because the alternative
	 * is that we don't cache this call.  Not caching new calls could be deadly for runtime.
	 */
	entry = ALLOC(bdd_constcache_entry, 1);
        if (entry == NIL(bdd_constcache_entry)) {  
	    (void) bdd_memfail(manager, "bdd_constcache_insert");
        }
        manager->heap.cache.consttable.buckets[pos] = entry;
	manager->heap.cache.consttable.nentries++;
    }

    entry->ITE.f = ITE_f;
    entry->ITE.g = ITE_g;
    entry->ITE.h = ITE_h;
    entry->data = data;

    manager->heap.stats.cache.consttable.inserts++;

    /*
     * Check to see if table needs resizing.  We base the decision on the ratio of the number of entries to the number
     * of buckets.
     */
    if ( (unsigned int) (((float) manager->heap.cache.consttable.nentries 
                          / (float) manager->heap.cache.consttable.nbuckets) * 100) 
                         > manager->heap.cache.consttable.resize_at) {
        (void) resize_consttable(manager);
    }
}

/*
 *    bdd_constcache_lookup - perform a lookup in the cache
 *
 *    return {TRUE, FALSE} on {found, not found}.
 */
boolean
bdd_constcache_lookup(manager, f, g, h, value)
bdd_manager *manager;
bdd_node *f;
bdd_node *g;
bdd_node *h;
bdd_constant_status *value;		/* return */
{

    int pos;
    bdd_constcache_entry *entry;

    if ( ! manager->heap.cache.consttable.on ) {
        /*
         * Always fails if caching is off.
         */
	return (FALSE);		
    }

    pos = bdd_const_hash(manager, f, g, h);
    entry = manager->heap.cache.consttable.buckets[pos];

    if (entry != NIL(bdd_constcache_entry)) {
	/*
	 * Entry exists at pos.  See if it matches.  If not (a miss) return.  If it does match
	 * (a hit), then will drop out of IF and set value.
	 */
        if (entry->data == bdd_status_unknown ||(entry->ITE.f != f || entry->ITE.g != g || entry->ITE.h != h)) {
	    manager->heap.stats.cache.consttable.misses++;
	    return (FALSE);
        }
    } else {
	/*
	 * No entry exists at pos.  Definitely a miss.
	 */
	manager->heap.stats.cache.consttable.misses++;
	return (FALSE);
    }  

    manager->heap.stats.cache.consttable.hits++;

    *value = entry->data;
    return (TRUE);
}

/*
 * resize_consttable 
 *
 * Make the ITE constant cache larger.  Perform a rehash to map all of the old
 * stuff into this new larger world. Note that since we don't maintain collision chains,
 * some stuff may be lost.
 */
static void
resize_consttable(manager)
bdd_manager *manager;
{
    unsigned int old_nbuckets, next_hash_prime;
    bdd_constcache_entry **old_buckets, *old_entry;
    int i, pos;

    /*
     * If the max_size has already been exceeded, then just return.
     */
    if (manager->heap.cache.consttable.nbuckets > manager->heap.cache.consttable.max_size) {
        return;
    }

    /*
     * Get the next hash prime.  If allocating a new table will cause the manager memory limit to 
     * be exceeded, then just return.  Trading off time in favor of space.
     */
    next_hash_prime = bdd_get_next_hash_prime(manager->heap.cache.consttable.nbuckets);
    if (bdd_will_exceed_mem_limit(manager, (next_hash_prime * sizeof(bdd_constcache_entry *)), FALSE) == TRUE) { 
        return;
    }

    /*
     * Save the old buckets.
     */
    old_nbuckets = manager->heap.cache.consttable.nbuckets;
    old_buckets = manager->heap.cache.consttable.buckets;

    /*
     * Get the new size and allocate the new table.  We are growing by roughly a factor of two.
     * If there isn't sufficient memory, don't fail: the cache
     * is meant to enhance performance, but we can still continue even if we can't resize.
     */
    manager->heap.cache.consttable.nbuckets = next_hash_prime;
    manager->heap.cache.consttable.buckets = ALLOC(bdd_constcache_entry *, manager->heap.cache.consttable.nbuckets); 
    if (manager->heap.cache.consttable.buckets == NIL(bdd_constcache_entry *)) {
        manager->heap.cache.consttable.nbuckets = old_nbuckets;
        manager->heap.cache.consttable.buckets = old_buckets;
        return;
    }

    /* 
     * Initialize all the entries to NIL.
     */
    for (i = 0; i < manager->heap.cache.consttable.nbuckets; i++) {
	manager->heap.cache.consttable.buckets[i] = NIL(bdd_constcache_entry);
    }

    /*
     * Remap all of the ITE cache entries to the new larger cache.
     */
    manager->heap.cache.consttable.nentries = 0;
    for (i = 0; i < old_nbuckets; i++) {
	old_entry = old_buckets[i];
	if (old_entry != NIL(bdd_constcache_entry)) {

	    /*
	     * Get the hash position for the entry in the new cache.  If the position is already
	     * occupied, then old_entry will not be rehashed; it's lost;  If the position is not
	     * occupied, then simply set the new cache bucket pointer to old_entry.
	     */
	    pos = bdd_constcache_entry_hash(manager, old_entry);
	    if (manager->heap.cache.consttable.buckets[pos] != NIL(bdd_constcache_entry)) {
		FREE(old_entry);
	    } else {
		manager->heap.cache.consttable.buckets[pos] = old_entry;
		manager->heap.cache.consttable.nentries++;
	    }
	}
    }

    /*
     * Free the array of buckets.  Note that the entries of the table were simply 
     * moved over to the new table.
     */
    FREE(old_buckets);

}

