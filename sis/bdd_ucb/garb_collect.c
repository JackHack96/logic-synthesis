
#include <stdio.h>	/* for BDD_DEBUG_LIFESPAN */

#include "util.h"
#include "array.h"
#include "st.h"

#include "bdd.h"
#include "bdd_int.h"



static void flip_spaces();
static void scan_manager();
static void scan_newhalf();
static void scan_cache();
static bdd_node *relocate();
static void scan_adhoccache();
static void scan_hashcache();
static void scan_constcache();

void
bdd_set_gc_mode(bdd, no_gc)
bdd_manager *bdd;
boolean no_gc;		/* this is inverted for historical reasons */
{
	bdd->heap.gc.on = ! no_gc;

#if defined(BDD_DEBUG) /* { */
	fprintf(stderr, "set_gc_mode: garbage-collection is now %s\n", bdd->heap.gc.on ? "on": "off");
#endif /* } */
}

#if defined(BDD_DEBUG_LIFESPAN) /* { */
static void discover_terminations();
#endif /* } */

/*
 *    bdd_garbage_collect - perform a garbage-collection
 *
 *    A simple stop-and-copy garbage-collection is performed.
 *    There is only a single generation at this point, so the
 *    stop-and-copy is rather straightforward.
 *
 *    return nothing, just do it.
 */
void
bdd_garbage_collect(manager)
bdd_manager *manager;
{
	static boolean busyp = FALSE;
	int i;
        long time;
        boolean itetable_on, consttable_on, adhoc_on;
        unsigned int prev_nodes_used;

	if ( ! manager->heap.gc.on ) {
#if defined(BDD_DEBUG_GC) /* { */
	    printf("garbage_collect: the garbage-collector is not enabled (skipping)\n");
#endif /* } */
	    return;
	}

        /* 
         * Mark the amount of time spent garbage collecting.
         */
        time = util_cpu_time();

        /*
         * Note the number of nodes currently in use.
         */
        prev_nodes_used = manager->heap.stats.nodes.used;

        /* 
         * Defensive programming.
         */
	if (busyp) {
	    fail("bdd_garbage_collect: the garbage-collector is already running");
        }

        /* 
         * Not handling this case now.
         */
	if (manager->heap.gc.status.open_generators > 0) {
	    fail("bdd_garbage_collect: there are open generators during the garbage collection");
        }

#if defined(BDD_DEBUG_GC) /* { */
	(void) printf("before garbage collection #%d ...\n", manager->heap.stats.gc.times+1);
	(void) bdd_assert_heap_correct(manager);
#endif /* } */

	busyp = TRUE;

	/*
	 * Defensive progamming.  Turn off caching while garbage collection; prevents
         * any attempt to insert or lookup in the caches.  Remember the current values.
	 */
        itetable_on = manager->heap.cache.itetable.on;
        consttable_on = manager->heap.cache.consttable.on;
        adhoc_on = manager->heap.cache.adhoc.on;
	manager->heap.cache.itetable.on = FALSE;
	manager->heap.cache.consttable.on = FALSE;
	manager->heap.cache.adhoc.on = FALSE;

        /*
         * Flip sense of old space and new space.
         */
	(void) flip_spaces(manager);

        /*
         * Relocate all bdd_nodes directly pointed to by root pointers, into new space.
         */
	(void) scan_manager(manager);

	manager->heap.gc.during.start.block = manager->heap.half[manager->heap.gc.halfspace].inuse.top;
	manager->heap.gc.during.start.index = 0;

	/*
	 * Reset the hashtable.  The function scan_newhalf rehashes all the nodes into the table.
	 * Note that since garbage collection possibly reduces the number of bdd_nodes, maybe the
	 * size of this table should be reduced.
	 */
	manager->heap.hashtable.nkeys = 0;
	for (i = 0; i < manager->heap.hashtable.nbuckets; i++) {
	    manager->heap.hashtable.buckets[i] = NIL(bdd_node);
	}

	(void) scan_newhalf(manager);
	(void) scan_cache(manager);

        /* 
         * In the current implementation, scan_cache does not relocate any nodes.  Therefore, there
         * are no new additional nodes in new space to scan.  Thus, the following call to scan_newhalf
         * is superflous.
         */
	(void) scan_newhalf(manager);	      

	manager->heap.stats.gc.times++;
	manager->heap.cache.itetable.on = itetable_on;
	manager->heap.cache.consttable.on = consttable_on;
	manager->heap.cache.adhoc.on = adhoc_on;
        manager->heap.stats.gc.nodes_collected += (prev_nodes_used - manager->heap.stats.nodes.used);       
	busyp = FALSE;

#if defined(BDD_DEBUG) /* { */
#if defined(BDD_DEBUG_GC) || defined(BDD_DEBUG_GC_STATS) /* { */
	(void) printf("... after garbage collection #%d\n", manager->heap.stats.gc.times);
	(void) bdd_dump_manager_stats(manager);
#if defined(BDD_DEBUG_GC) /* { */
	(void) bdd_assert_heap_correct(manager);
#endif /* } */
#endif /* } */
#if defined(BDD_DEBUG_AGE) || defined(BDD_DEBUG_LIFESPAN) /* { */
	manager->debug.gc.age++;		/* one gc older now ... */
#endif /* } */
#if defined(BDD_DEBUG_LIFESPAN) /* { */
	/*
	 *    Process terminations and then log the fact that it
	 *    is one age older now (for the newly created ones next time)
	 */
	(void) discover_terminations(manager);
	(void) fprintf(manager->debug.lifespan.trace, "g %d\n", manager->debug.gc.age);
#endif /* } */
#endif /* } */

        /*
         * Accumulate the CPU time.
         */
        manager->heap.stats.gc.runtime += util_cpu_time() - time;

}

/*
 *    flip_spaces - do exactly that, invert the sense of oldspace and newspace
 *
 *    return nothing, just do it.
 */
static void
flip_spaces(manager)
bdd_manager *manager;
{
	/*
	 *    Flip the halfspace
	 */
	manager->heap.gc.halfspace = ! manager->heap.gc.halfspace;

	/*
	 *    NOTE: The whole point of having unique id's is that they are in
	 *    fact unique over the life of the (conceptual) object.  So you can
	 *    identify an object by its uniqueId at all times.   Note that with
	 *    each (copying) garbage-collection the data is copied into a new
	 *    bdd_node, BUT the object is not given a new uniqueId.  The uniqueId's
	 *    thus are a denotation of the value in a node.  Thus, there is no need
         *    to reset manager->debug.gc.uniqueId at this point.
	 */

	/*
	 *    Concatenate the free list onto the tail of the inuse list
	 *    Then flip the sense of the inuse list and the free list
	 *    thereby making everything on the free list.
	 */
	*manager->heap.half[manager->heap.gc.halfspace].inuse.tail =
		manager->heap.half[manager->heap.gc.halfspace].free;
	manager->heap.half[manager->heap.gc.halfspace].free =
		manager->heap.half[manager->heap.gc.halfspace].inuse.top;

	manager->heap.half[manager->heap.gc.halfspace].inuse.top = NIL(bdd_nodeBlock);
	manager->heap.half[manager->heap.gc.halfspace].inuse.tail = 
		&manager->heap.half[manager->heap.gc.halfspace].inuse.top;

	manager->heap.pointer.block = NIL(bdd_nodeBlock);
	manager->heap.pointer.index = 0;

	manager->heap.stats.nodes.used = 0;
	manager->heap.stats.nodes.unused = manager->heap.stats.nodes.total;

	/*
	 *    The hashtable and caches are not touched until
	 *    AFTER all of the garbage-collection is done.
	 */
}


/*
 *    scan_manager - scan all root pointers in the manager
 *
 *    The old buckets must be destroyed and ALL bdd_node objects
 *    must be rehashed into a new hashtable.  This is because the
 *    position in the hashtable is a hashfunction of the object
 *    address.   The address moves in a copying garbage-collector
 *    so objects must be rehashed into the new table.
 *
 *    Take all the stuff which needs to be saved and put it in the
 *    flipped stuffd.  This way the garbage collector can operate
 *    using bdd_find_or_add using a clean hashtable; the old hashtable
 *    is saved in the flipped stuff.
 *
 *    Root pointers:
 *        bdd.one		- the one
 *        heap.external_refs	- external references held by the user
 *        heap.internal_refs	- internal references on the stack
 *
 *    return nothing, just do it.
 */
static void
scan_manager(manager)
bdd_manager *manager;
{
	int i;
	bdd_safeframe *sf;
	bdd_safenode *sn;
	bdd_bddBlock *b;

	/*
	 *    The one one
	 */
	manager->bdd.one = relocate(manager, manager->bdd.one);

	/*
	 *    DO NOT use the hash table as a set of root pointers.  Think about it.
	 *    You would be using the data structure which contains ALL cells in the
	 *    heap as the set of root pointers into the heap.   There would be no
	 *    garbage generated at all.
	 *
	 *    The hash table therefore is NOT a root pointer
	 *
	 *    Hash Table
	 */

	/*
	 *    External references
	 */
	for (b=manager->heap.external_refs.map; b != NIL(bdd_bddBlock); b=b->next) {
	    for (i=0; i<sizeof_el (b->subheap); i++) {
		if ( ! b->subheap[i].free )
		    b->subheap[i].node = relocate(manager, b->subheap[i].node);
	    }
	}

	/*
	 *    Internal references
	 */
	for (sf=manager->heap.internal_refs.frames; sf != NIL(bdd_safeframe); sf=sf->prev) {
	    for (sn=sf->nodes; sn != NIL(bdd_safenode); sn=sn->next) {
		/* get them both without worry (one or the other will be nil */
		if (sn->arg != NIL(bdd_node *))
		    *sn->arg = relocate(manager, *sn->arg);
		sn->node = relocate(manager, sn->node);
	    }
	}

	/* 
	 *    DO NOT use the ITE, ITE_const, or adhoc caches as a root pointers right now.
	 *    Wait until after the scan of the newhalf and then
	 *    search for forwarded pointers in the cache.  Wherever
	 *    there are such, then keep those nodes; others are
	 *    nilled out because the objects referenced are dead.
	 *
	 *    See scan_cache() below and how it is used on conjunction
	 *    with a second pass of scan_newhalf above.
	 *
	 */
}

/*
 *    scan_newhalf - scan the new half of memory faulting in stuff
 *
 *    The transitive fanin of objects is faulted in.
 *
 *    NOTE (and note it well): this function must change drastically if
 *    any of a number of preconditions are violated:
 *        1) there are multiple types of objects on multiple page types
 *        2) multiple generations are used
 *
 *    There is only one object type now (the bdd_node*), and so the relocate
 *    and scan functions are effectively trivial; the scan function for 
 *    example is completely inlined here and is only a figment of the paradigm.
 *
 *    If multiple pages types are used, then multiple passes over the page
 *    frontier must be made in order to get the transitive closure on the space
 *
 *    If multiple generations are used, then much needs to be rethought.
 *
 *    return nothing, just do it.
 */
static void
scan_newhalf(manager)
bdd_manager *manager;
{
	bdd_nodeBlock *block;
	bdd_node *node;
	boolean pastp;
	int i, limit, pos;

	BDD_ASSERT(manager->heap.gc.during.start.block != NIL(bdd_nodeBlock));

	/*
	 *    The newhalf scan
	 */
	pastp = FALSE;		/* get inside the loop on the 1st go-around */ 

	for (block=manager->heap.gc.during.start.block; ! pastp; block=block->next) {
	    pastp = block == manager->heap.pointer.block ? TRUE: FALSE;
	    limit = pastp ? manager->heap.pointer.index: sizeof_el (block->subheap);

	    for (i=manager->heap.gc.during.start.index; i<limit; i++) {
		node = &block->subheap[i];

#if defined(BDD_DEBUG_GC) /* { */
		BDD_ASSERT(node->halfspace == manager->heap.gc.halfspace);
#endif /* } */

		/*
		 *    Relocate T and E
		 */
		node->T = relocate(manager, node->T);
		node->E = relocate(manager, node->E);

		/*
		 *    Relink the node (which is now completely defined)
		 *    into the hashtable; this gets all nodes in the newspace
		 */
		pos = bdd_node_hash(manager, node);
		node->next = manager->heap.hashtable.buckets[pos];
		manager->heap.hashtable.buckets[pos] = node;
		manager->heap.hashtable.nkeys++;

		/*
		 *    We must evaluate this again because 
		 *    relocate() changed pointer.{block,index}
		 */
		pastp = block == manager->heap.pointer.block ? TRUE: FALSE;
		limit = pastp ? manager->heap.pointer.index: sizeof_el (block->subheap);
	    }
	    manager->heap.gc.during.start.index = 0;
	}

	manager->heap.gc.during.start.block = manager->heap.pointer.block;
	manager->heap.gc.during.start.index = manager->heap.pointer.index;
}

/*
 *    scan_cache - scan the cache to kill dead entries
 *
 *    WATCHOUT - The cache must be scanned passively to ensure that
 *    it does not become a source of root pointers (besides this phase
 *    occurs a bit late for that).
 *
 *    The deal is that we want to invalidate cache entries which do
 *    not consist entirely of forwarded pointers.   These entries were obviously
 *    not forwarded by being needed elsewhere; they are dead.
 *
 *    return nothing, just do it.
 */
static void
scan_cache(manager)
bdd_manager *manager;
{
	/*
	 * Scan all the caches.
	 */
	(void) scan_hashcache(manager);
	(void) scan_constcache(manager);
	(void) scan_adhoccache(manager);  
}

static void
scan_hashcache(manager)
bdd_manager *manager;
{
    boolean unused;
    int i, pos;
    bdd_hashcache_entry tmp;
    bdd_hashcache_entry *e, *old_e, **old_buckets;
    bdd_node *reg_old_f, *reg_old_g, *reg_old_h, *reg_old_data;

    /*
     * If there are no entries in the cache, then there is nothing to scan.
     */
    if (manager->heap.cache.itetable.nentries == 0) {
        return;
    }

    if (manager->heap.cache.itetable.invalidate_on_gc) {
	/*
	 *    If just invalidating, then invalidate (only)
	 */
#if defined(BDD_DEBUG_GC) /* { */
	(void) printf("... invalidating itetable cache\n");
#endif /* } */

	for (i = 0; i < manager->heap.cache.itetable.nbuckets; i++) {
	    e = manager->heap.cache.itetable.buckets[i];	
	    if (e != NIL(bdd_hashcache_entry)) {
		FREE(e);
		manager->heap.cache.itetable.buckets[i] = NIL(bdd_hashcache_entry);
	    }
	}
    } else {
	/*
	 *    Otherwise the cache must be updated through a rehashing
	 *    because all of the keys in the itetable have been changed.
	 */

	/*
	 *    Save the old one
	 */
	old_buckets = manager->heap.cache.itetable.buckets;

	/*
	 *    Make a new clean one so we can rehash into it
	 */
	manager->heap.cache.itetable.buckets = ALLOC(bdd_hashcache_entry *, manager->heap.cache.itetable.nbuckets);
	if (manager->heap.cache.itetable.buckets == NIL(bdd_hashcache_entry *))
	    (void) bdd_memfail(manager, "scan_hashcache");
	for (i = 0; i < manager->heap.cache.itetable.nbuckets; i++) {
	    manager->heap.cache.itetable.buckets[i] = NIL(bdd_hashcache_entry);
	}

	/*
	 *    ... then try to keep as many of the old cache entries as possible
	 */
	manager->heap.cache.itetable.nentries = 0;
	for (i = 0; i < manager->heap.cache.itetable.nbuckets; i++) {
	    old_e = old_buckets[i];	/* to make it textually easier to read */
	    if (old_e != NIL(bdd_hashcache_entry)) {

		/*
		 *    If ALL of the four pointers were forwarded, then we will keep this entry.
		 *    (Note: An alternate policy would be to keep the entry if ANY of the four pointers
		 *    were forwarded.  In this case, non-forwarded pointers would be relocated,
		 *    thus necessitating an additional call to scan_newhalf after the call to scan_cache.)
		 *
		 *    We must regularize the node pointers before we check for BROKEN_HEART.
		 */
		reg_old_f = BDD_REGULAR(old_e->ITE.f);
		reg_old_g = BDD_REGULAR(old_e->ITE.g);
		reg_old_h = BDD_REGULAR(old_e->ITE.h);
		reg_old_data = BDD_REGULAR(old_e->data);

		if (reg_old_data != NIL(bdd_node) &&
			(BDD_BROKEN_HEART(reg_old_f) && BDD_BROKEN_HEART(reg_old_g) &&
			 BDD_BROKEN_HEART(reg_old_h) && BDD_BROKEN_HEART(reg_old_data))) {
		    /*
		     * Since broken heart is set on all 4 nodes, relocate will just
		     * use the forward pointer, and set the proper phase of the pointer.
		     */
		    tmp.ITE.f = relocate(manager, old_e->ITE.f);
		    tmp.ITE.g = relocate(manager, old_e->ITE.g);
		    tmp.ITE.h = relocate(manager, old_e->ITE.h);
		    tmp.data =  relocate(manager, old_e->data);

		    /*
		     *    Canonicalize the ITE with the same algorithm that was used
		     *    to canonicalize the ITE inputs when the cache was created.
		     */
		    (void) bdd_ite_canonicalize_ite_inputs(manager, &tmp.ITE.f, &tmp.ITE.g, &tmp.ITE.h, &unused);

		    /*
		     * Get the hash position for the entry in the new cache.  If the position is already
		     * occupied, then old_e will not be rehashed; it's lost;  If the position is not
		     * occupied, then we can simply reuse the memory of the old entry.  We cannot use
                     * bdd_hashcache_insert because it doesn't work when the cache is turned off.
		     */
		    pos = bdd_ITE_hash(manager, tmp.ITE.f, tmp.ITE.g, tmp.ITE.h);
		    if (manager->heap.cache.itetable.buckets[pos] != NIL(bdd_hashcache_entry)) {
			FREE(old_e);
		    } else {
			*old_e = tmp;
			manager->heap.cache.itetable.buckets[pos] = old_e;
			manager->heap.cache.itetable.nentries++;
		    }
		} else {
		    /*
		     * old_e points to some stale data, and thus cannot be kept.  Free memory.
		     */
		    FREE(old_e);
		}
	    } /* close: if (old_e != NIL(bdd_hashcache_entry)) { */
	} /* close: for (i = 0; i < manager->heap.cache.itetable.nbuckets; i++) { */

	/*
	 * We are done rehashing. Get rid of the old_buckets.
	 */
	FREE(old_buckets);
    }
}

static void
scan_constcache(manager)
bdd_manager *manager;
{
    boolean unused;
    int i, pos;
    bdd_constcache_entry tmp;
    bdd_constcache_entry *e, *old_e, **old_buckets;
    bdd_node *reg_old_f, *reg_old_g, *reg_old_h;

    /*
     * If there are no entries in the cache, then there is nothing to scan.
     */
    if (manager->heap.cache.consttable.nentries == 0) {
        return;
    }

    if (manager->heap.cache.consttable.invalidate_on_gc) {
	/*
	 *    If just invalidating, then invalidate (only)
	 */
#if defined(BDD_DEBUG_GC) /* { */
	(void) printf("... invalidating consttable cache\n");
#endif /* } */

	for (i = 0; i < manager->heap.cache.consttable.nbuckets; i++) {
	    e = manager->heap.cache.consttable.buckets[i];	
	    if (e != NIL(bdd_constcache_entry)) {
		FREE(e);
		manager->heap.cache.consttable.buckets[i] = NIL(bdd_constcache_entry);
	    }
	}
    } else {
	/*
	 *    Otherwise the cache must be updated through a rehashing
	 *    because all of the keys in the consttable have been changed.
	 */

	/*
	 *    Save the old one
	 */
	old_buckets = manager->heap.cache.consttable.buckets;

	/*
	 *    Make a new clean one so we can rehash into it
	 */
	manager->heap.cache.consttable.buckets = ALLOC(bdd_constcache_entry *, manager->heap.cache.consttable.nbuckets);
	if (manager->heap.cache.consttable.buckets == NIL(bdd_constcache_entry *)) {
	    (void) bdd_memfail(manager, "scan_constcache");
        }
	for (i = 0; i < manager->heap.cache.consttable.nbuckets; i++) {
	    manager->heap.cache.consttable.buckets[i] = NIL(bdd_constcache_entry);
	}

	/*
	 *    ... then try to keep as many of the old cache entries as possible
	 */
        manager->heap.cache.consttable.nentries = 0;
	for (i = 0; i < manager->heap.cache.consttable.nbuckets; i++) {
	    old_e = old_buckets[i];	/* to make it textually easier to read */
            if (old_e != NIL(bdd_constcache_entry)) {

		/*
		 *    If ALL of the three pointers were forwarded, then we will keep this entry.
		 *    (Note: An alternate policy would be to keep the entry if ANY of the three pointers
		 *    were forwarded.  In this case, non-forwarded pointers would be relocated,
		 *    thus necessitating an additional call to scan_newhalf after the call to scan_cache.)
		 *
		 *    We must regularize the node pointers before we check for BROKEN_HEART.
		 */
		reg_old_f = BDD_REGULAR(old_e->ITE.f);
		reg_old_g = BDD_REGULAR(old_e->ITE.g);
		reg_old_h = BDD_REGULAR(old_e->ITE.h);

		if (old_e->data != bdd_status_unknown &&
			(BDD_BROKEN_HEART(reg_old_f) && BDD_BROKEN_HEART(reg_old_g) && BDD_BROKEN_HEART(reg_old_h) )) {
		    /*
		     * Since broken heart is set on all 3 nodes, relocate will just
		     * use the forward pointer, and set the proper phase of the pointer.
		     */
		    tmp.ITE.f = relocate(manager, old_e->ITE.f);
		    tmp.ITE.g = relocate(manager, old_e->ITE.g);
		    tmp.ITE.h = relocate(manager, old_e->ITE.h);
		    tmp.data = old_e->data;

		    /*
		     *    Canonicalize the ITE with the same algorithm that was used
		     *    to canonicalize the ITE inputs when the cache was created.
		     */
		    (void) bdd_ite_canonicalize_ite_inputs(manager, &tmp.ITE.f, &tmp.ITE.g, &tmp.ITE.h, &unused);

		    /*
		     * Get the hash position for the entry in the new cache.  If the position is already
		     * occupied, then old_e will not be rehashed; it's lost;  If the position is not
		     * occupied, then we can simply reuse the memory of the old entry.
		     */
		    pos = bdd_const_hash(manager, tmp.ITE.f, tmp.ITE.g, tmp.ITE.h);
		    if (manager->heap.cache.consttable.buckets[pos] != NIL(bdd_constcache_entry)) {
			FREE(old_e);
		    } else {
			*old_e = tmp;
			manager->heap.cache.consttable.buckets[pos] = old_e;
			manager->heap.cache.consttable.nentries++;
		    }
		} else {
		    /*
		     * Entry cannot be kept.  Free memory.
		     */
		    FREE(old_e);
		}
	    } /* close: if (old_e != NIL(bdd_constcache_entry)) { */
	} /* close: for (i = 0; i < manager->heap.cache.consttable.nbuckets; i++) { */

	/*
	 * We are done rehashing. Get rid of the old_buckets.
	 */
	FREE(old_buckets);
    }
}

static void
scan_adhoccache(manager)
bdd_manager *manager;		/* adhoc.table is not NIL */
{
	st_table *old;
	st_generator *gen;
	bdd_adhoccache_key *k;
	bdd_node *v;
        bdd_node *reg_old_f, *reg_old_g, *reg_old_v;


	if (manager->heap.cache.adhoc.table == NIL(st_table))
	    return;	/* nothing to do */

	/*
	 *    Save the current table (for later traversal) and get a new adhoc
	 *    cache.  Then stick all the entries in the old adhoc cache in to
	 *    the new adhoc cache.  Destroy the old table (but not its keys
	 *    (which are now still in use).
	 */
	old = manager->heap.cache.adhoc.table;
	manager->heap.cache.adhoc.table = NIL(st_table);

	(void) bdd_adhoccache_init(manager);	/* get a new one */

	gen = st_init_gen(old);

	while (st_gen(gen, (refany*) &k, (refany*) &v)) {
            /*
             * We must regularize the node pointers before we check for BROKEN_HEART.
             */
            reg_old_f = BDD_REGULAR(k->f);
            reg_old_g = BDD_REGULAR(k->g);
            reg_old_v = BDD_REGULAR(v);

	    if (BDD_BROKEN_HEART(reg_old_f) && BDD_BROKEN_HEART(reg_old_g) &&
		    (reg_old_v == NIL(bdd_node) || BDD_BROKEN_HEART(reg_old_v))) {
		/*
		 *    Policy: if ALL of the entries in this cache have been
		 *    forwarded, then keep the cache entry in whole.
		 */
                k->f = relocate(manager, k->f);
                k->g = relocate(manager, k->g);
                v = relocate(manager, v);
	        (void) st_insert(manager->heap.cache.adhoc.table, (refany) k, (refany) v);
	    } else {
		/*
		 *    Else, simply don't copy the cache entry over to the new table.
		 */
                FREE(k);
	    }
	}

	(void) st_free_gen(gen);
	(void) st_free_table(old);	/* but DON'T free its keys or values */
}

/*
 *    relocate - the all-important copy function copying from oldspace to newspace
 *
 *    This function can take either regular or complemented pointers as argument.
 *    The result pointer is returned in the same phase as the caller.  The
 *    object referred to is copied to the new space and a forwarding pointer is
 *    created; a broken heart.  This broken heart is always in the positive phase
 *    and it is up to the caller to refer to the new object in whatever phase
 *    is desired.
 *
 *    return the new object in the new space
 */
static bdd_node *
relocate(manager, old)
bdd_manager *manager;
bdd_node *old;		/* may be NIL(bdd_node), a regular or complemented pointer */
{
	bdd_node *new, *reg_old;
	boolean complemented;

	if (old == NIL(bdd_node)) {
	    /*
	     *    This isn't really an object, so it must be special-cased
	     */
	    new = NIL(bdd_node);
	    complemented = FALSE;
	} else {
	    /*
	     *    The broken hearts must deal with regular pointers
	     */
	    reg_old = BDD_REGULAR(old);
	    complemented = BDD_IS_COMPLEMENT(old);

	    if (BDD_BROKEN_HEART(reg_old)) {
		/*
		 *    A forwarded object; take the forwarded value
		 */
		new = BDD_FORWARD_POINTER(reg_old);
	    } else {
		/*
		 *    Must copy over to the new space with the (re)allocate function
		 *    So, use a special find_or_add operation that always copies.
		 */
		new = bdd_new_node(manager, reg_old->id, reg_old->T, reg_old->E, reg_old);
		BDD_SET_FORWARD_POINTER(reg_old, new);
	    }
	}

	BDD_ASSERT_NOT_BROKEN_HEART(new);
	BDD_ASSERT_REGNODE(new);

	new = ! complemented ? new: BDD_NOT(new);

	return (new);
}

#if defined(BDD_DEBUG_LIFESPAN) /* { */
/*
 *    discover_terminations - discover and log lifespans which terminated
 *    
 *    return nothing, just do it.
 */
static void
discover_terminations(manager)
bdd_manager *manager;
{
	bdd_nodeBlock *block;
	bdd_node *node;
	int old_halfspace, i;

	old_halfspace = ! manager->heap.gc.halfspace;

	for (block=manager->heap.half[old_halfspace].inuse.top;
		block != NIL(bdd_nodeBlock); block=block->next) {
	    for (i=0; i<block->used; i++) {
		node = &block->subheap[i];

		/*
		 *    If its not a broken heart, then it died (end of story).
		 */
		if ( ! BDD_BROKEN_HEART(node) ) {
		    fprintf(manager->debug.lifespan.trace, "d %d %d\n", node->uniqueId, node->age);
		}
	    }
	}
}
#endif /* } */
