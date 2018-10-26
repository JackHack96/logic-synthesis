#include <stdio.h>	/* for BDD_DEBUG_LIFESPAN */

#include "util.h"
#include "array.h"
#include "st.h"

#include "bdd.h"
#include "bdd_int.h"



static int cmp();
static int hash();

/*
 *    bdd_adhoccache_init - initialize the adhoc cache for the manager
 *
 *    return nothing, just do it.
 */
void
bdd_adhoccache_init(manager)
bdd_manager *manager;
{
	manager->heap.cache.adhoc.table = st_init_table(cmp, hash);
}

/*
 *    bdd_adhoccache_insert - insert an entry into the adhoc cache
 *
 *    return nothing, just do it.
 */
void
bdd_adhoccache_insert(manager, f, g, v, value)
bdd_manager *manager;
bdd_node *f;		/* may be NIL(bdd_node) */
bdd_node *g;		/* may be NIL(bdd_node) */
bdd_int v;		/* may be any integer */
bdd_node *value;
{
	bdd_adhoccache_key *key;
        int status;

	if ( ! manager->heap.cache.adhoc.on ) {
	    return;		/* has no effect if caching is off */
        }

        /* 
         * If we have already reached the max number of entries, then kill the current
	 * adhoc_cache and start a new one.  We must do this, because the st package 
	 * doesn't provide a convenient way to just overwrite an old entry.
         */ 
        if (st_count(manager->heap.cache.adhoc.table) > manager->heap.cache.adhoc.max_size) { 
	    (void) bdd_adhoccache_uninit(manager);
	    (void) bdd_adhoccache_init(manager);
        }

        /*
         * Allocate a new key (even if it would cause the memory limit to be exceeded).
         */
	key = ALLOC(bdd_adhoccache_key, 1);
	if (key == NIL(bdd_adhoccache_key)) {
	    (void) bdd_memfail(manager, "bdd_adhoccache_insert");
        }

        /*
         * Set the values of the key.
         */
	key->f = f;
	key->g = g;
	key->v = v;

        /*
         * Insert the new key.  Since we previous lookup on this entry was not successful, there should
         * not be another entry already under the key.  st_insert returns 1 if there was an entry already under
         * the key, and 0 otherwise.  Since the table has collision chains, there should not be any collisions.
         */
	status = st_insert(manager->heap.cache.adhoc.table, (refany) key, (refany) value);
        BDD_ASSERT(!status); 
	manager->heap.stats.cache.adhoc.inserts++;
}

/*
 *    bdd_adhoccache_lookup - lookup an entry in the adhoc cache
 *
 *    return {TRUE, FALSE} on {found, not found}.
 */
boolean
bdd_adhoccache_lookup(manager, f, g, v, value)
bdd_manager *manager;
bdd_node *f;		/* may be NIL(bdd_node) */
bdd_node *g;		/* may be NIL(bdd_node) */
bdd_int v;		/* may be any integer */
bdd_node **value;	/* return */
{
	bdd_adhoccache_key key;


	if ( ! manager->heap.cache.adhoc.on ) {
	    /*
	     * Always fails if caching is off.
	     */
	    return (FALSE);		
	}

	key.f = f;
	key.g = g;
	key.v = v;

	if (st_lookup(manager->heap.cache.adhoc.table, (refany) &key, (refany*) value)) {
	    manager->heap.stats.cache.adhoc.hits++;
	    return (TRUE);
	}

	manager->heap.stats.cache.adhoc.misses++;
	return (FALSE);
}

/*
 *    bdd_adhoccache_uninit - destroy a adhoc cache
 *
 *    delete all the keys
 *
 *    return nothing, just do it.
 */
void
bdd_adhoccache_uninit(manager)
bdd_manager *manager;
{
	st_generator *gen;
	bdd_adhoccache_key *key;
	bdd_node *value;
	
	st_foreach_item(manager->heap.cache.adhoc.table, gen, (refany*) &key, (refany*) &value) {
	    FREE(key);
	}
	st_free_table(manager->heap.cache.adhoc.table);
	manager->heap.cache.adhoc.table = NIL(st_table);
}

/*
 *    cmp - a st_table comparison function
 *
 *    return {0, 1} on {same, different}.
 */
static int
cmp(n1, n2)
bdd_adhoccache_key *n1;
bdd_adhoccache_key *n2;
{
	boolean samep;

	samep = n1->f == n2->f && n1->g == n2->g && n1->v == n2->v;
	return (samep ? /* equal */ 0: /* different */ 1);
}

/* 
 *    hash - a st_table hash function
 *
 *    return an integer in the range 0 .. modulus-1
 */
static int
hash(key, modulus)
bdd_adhoccache_key *key;
int modulus;
{
	unsigned int hval;
	
	hval = bdd_generic_hash(key->f, key->g, key->v, modulus);
	return (hval);
}
