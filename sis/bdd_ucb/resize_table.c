/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/bdd_ucb/resize_table.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:15:02 $
 *
 */
#include <stdio.h>	/* for BDD_DEBUG_LIFESPAN */

#include "util.h"
#include "array.h"
#include "st.h"

#include "bdd.h"
#include "bdd_int.h"

/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/bdd_ucb/resize_table.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:15:02 $
 * $Log: resize_table.c,v $
 * Revision 1.1.1.1  2004/02/07 10:15:02  pchong
 * imported
 *
 * Revision 1.3  1992/09/21  23:30:31  sis
 * Updates from Tom Shiple - this is BDD package release 2.4.
 *
 * Revision 1.3  1992/09/21  23:30:31  sis
 * Updates from Tom Shiple - this is BDD package release 2.4.
 *
 * Revision 1.2  1992/09/19  03:26:02  shiple
 * Version 2.4
 * Prefaced compile time debug switches with BDD_. Added typecast to void to some function calls.
 * Added new prime calculation for hash table sizes.  Relocated the functionality of resizing of ITE
 * and ITE constant caches.  Renamed bdd_resize_tables to bdd_resize_hashtable, and cleaned it up.
 * Added usage of bdd_will_exceed_mem_limit in bdd_resize_hashtable.
 *
 * Revision 1.1  1992/07/29  00:27:11  shiple
 * Initial revision
 *
 * Revision 1.2  1992/05/06  18:51:03  sis
 * SIS release 1.1
 *
 * Revision 1.1  92/01/08  17:34:41  sis
 * Initial revision
 * 
 * Revision 1.1  91/04/11  21:00:10  shiple
 * Initial revision
 * 
 *
 */

#define BDD_NUM_HASH_PRIMES 28

/*
 *    bdd_resize_hashtable - resize the hashtable and make it bigger
 *
 *    Grow the hashtable by its grow factor and grow the cache by its
 *    grow factor also.  Perform a rehash/recache to map all of the old
 *    stuff into this new larger world.
 *
 *    return nothing, just do it.
 */
void
bdd_resize_hashtable(manager)
bdd_manager *manager;
{
    unsigned int old_nbuckets, next_hash_prime;
    bdd_node **old_buckets;
    int i, pos;
    bdd_node *n, *next_n;


    /*
     * Get the next hash prime.  If allocating a new table will cause the manager memory limit to 
     * be exceeded, then just return.  We are trading off time in favor of space; the collision chains
     * could become very long.
     */
    next_hash_prime = bdd_get_next_hash_prime(manager->heap.hashtable.nbuckets);
    if (bdd_will_exceed_mem_limit(manager, (next_hash_prime * sizeof(bdd_node *)), FALSE) == TRUE) { 
        return;
    }

    /*
     * Enlarge and clear out the new hashtable.
     */
    old_nbuckets = manager->heap.hashtable.nbuckets;
    old_buckets = manager->heap.hashtable.buckets;
    manager->heap.hashtable.nbuckets = next_hash_prime;
    manager->heap.hashtable.rehash_at_nkeys = manager->heap.hashtable.nbuckets * BDD_HASHTABLE_MAXCHAINLEN;
    manager->heap.hashtable.buckets = ALLOC(bdd_node *, manager->heap.hashtable.nbuckets);
    if (manager->heap.hashtable.buckets == NIL(bdd_node *)) {
	(void) bdd_memfail(manager, "bdd_resize_hashtable");
    }

    /*
     * Set all the buckets to NIL (so that the gc doesn't traverse them needlessly).
     */
    for (i = 0; i < manager->heap.hashtable.nbuckets; i++) {
	manager->heap.hashtable.buckets[i] = NIL(bdd_node);
    }

    /*
     * Remap all of the hashtable entries to the new larger hashtable.
     */
    for (i = 0; i < old_nbuckets; i++) {
	for (n = old_buckets[i]; n != NIL(bdd_node); n = next_n) {
	    next_n = n->next;

	    pos = bdd_node_hash(manager, n);

	    /* link the old data it onto the new bucket list */
	    n->next = manager->heap.hashtable.buckets[pos];
	    manager->heap.hashtable.buckets[pos] = n;
	}
    }

    /*
     * Free the array of buckets.  Note that the entries of the table were simply 
     * moved over to the new table.
     */
    FREE(old_buckets);
}

/* 
 * Get the next size for the hashtable.  Here, size is defined as the number of buckets.
 * This routine makes several assumptions:
 *  1) The initial table size is one of the numbers in the array primes, and all
 *     subsequent sizes are found using this routine.
 *  2) A bucket is at least 4 bytes.
 *  3) A grow factor of roughly two is desired.
 *
 * The primes listed are the greatest primes less than successive powers of two, with a safty
 * buffer of 4 to account for the malloc overhead.  Thus, for 2^7, the largest prime less than
 * 2^7-4, which is 113, is used.
 */

unsigned int bdd_get_next_hash_prime(current_size)
unsigned int current_size;
{
    int i;
    static unsigned int primes[BDD_NUM_HASH_PRIMES] = {
                 3, 
                11, 
                23, 
                59, 
               113, 
               251, 
               503, 
              1019, 
              2039, 
              4091, 
              8179, 
             16369, 
             32749, 
             65521, 
            131063, 
            262139, 
            524269, 
           1048571, 
           2097143, 
           4194287, 
           8388593, 
          16777199, 
          33554393, 
          67108859, 
         134217689, 
         268435399, 
         536870879, 
        1073741789};

    for (i = 0; i < BDD_NUM_HASH_PRIMES - 1; i++) {
        if (current_size == primes[i]) {
#if defined(BDD_MEMORY_USAGE) /* { */
            (void) fprintf(stdout, "hashtable size = %d\n", primes[i+1]);
#endif
            return primes[i+1];
        }
    }

    fail("bdd_get_next_hash_prime: current size not found");
}


