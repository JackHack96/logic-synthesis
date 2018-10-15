/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/bdd_ucb/bdd_start.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:15:01 $
 *
 */
#include <sys/param.h>	/* for MAXPATHLEN */
#include <stdio.h>	/* for BDD_DEBUG_LIFESPAN */

#include "util.h"
#include "array.h"
#include "st.h"

#include "bdd.h"
#include "bdd_int.h"

/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/bdd_ucb/bdd_start.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:15:01 $
 * $Log: bdd_start.c,v $
 * Revision 1.1.1.1  2004/02/07 10:15:01  pchong
 * imported
 *
 * Revision 1.3  1992/09/21  23:30:31  sis
 * Updates from Tom Shiple - this is BDD package release 2.4.
 *
 * Revision 1.3  1992/09/21  23:30:31  sis
 * Updates from Tom Shiple - this is BDD package release 2.4.
 *
 * Revision 1.2  1992/09/19  02:46:41  shiple
 * Version 2.4
 * Prefaced compile time debug switches with BDD_. Added typecast to void to some function calls.
 * Initialized new fields added to manager and stats data structures.  Added use of bdd_mgr_init.
 * Changed ITE and ITE_const caches to be arrays of pointers.  Added bdd_register_daemon.
 *
 * Revision 1.1  1992/07/29  00:26:53  shiple
 * Initial revision
 *
 * Revision 1.2  1992/05/06  18:51:03  sis
 * SIS release 1.1
 *
 * Revision 1.1  92/01/08  17:34:29  sis
 * Initial revision
 * 
 * Revision 1.1  91/03/27  14:35:34  shiple
 * Initial revision
 * 
 *
 */

/*
 * bdd_set_mgr_init_dflts - initialize the input bdd_mgr_init with the default settings
 */
void 
bdd_set_mgr_init_dflts(mgr_init)
bdd_mgr_init *mgr_init;
{
    mgr_init->ITE_cache.on = BDD_DFLT_ITE_ON;
    mgr_init->ITE_cache.resize_at = BDD_DFLT_ITE_RESIZE_AT;
    mgr_init->ITE_cache.max_size = BDD_DFLT_ITE_MAX_SIZE;
    mgr_init->ITE_const_cache.on = BDD_DFLT_ITE_CONST_ON;
    mgr_init->ITE_const_cache.resize_at = BDD_DFLT_ITE_CONST_RESIZE_AT;
    mgr_init->ITE_const_cache.max_size = BDD_DFLT_ITE_CONST_MAX_SIZE;
    mgr_init->adhoc_cache.on = BDD_DFLT_ADHOC_ON;
    mgr_init->adhoc_cache.resize_at = BDD_DFLT_ADHOC_RESIZE_AT;
    mgr_init->adhoc_cache.max_size = BDD_DFLT_ADHOC_MAX_SIZE;
    mgr_init->garbage_collector.on = BDD_DFLT_GARB_COLLECT_ON;
    mgr_init->memory.daemon = BDD_DFLT_DAEMON; 
    mgr_init->memory.limit = BDD_DFLT_MEMORY_LIMIT;
    mgr_init->nodes.ratio = BDD_DFLT_NODE_RATIO;
    mgr_init->nodes.init_blocks = BDD_DFLT_INIT_BLOCKS;
}

/*
 *    bdd_start_with_params - initialize a bdd manager with the supplied parameters
 *
 *    return the new bdd manager
 */
bdd_manager *
bdd_start_with_params(nvariables, mgr_init)
int nvariables;		/* the maximum number of variables allowable in this bdd manager */
bdd_mgr_init *mgr_init;
{
    bdd_manager *manager;
    int i, h;

    manager = ALLOC(bdd_manager, 1);
    if (manager == NIL(bdd_manager))
	(void) bdd_memfail(NIL(bdd_manager), "bdd_start");

    manager->undef1 = 0;   /* for debugging purposes */

    /*
     *    Initialize the heap part of the bdd_manager
     */

    manager->heap.hashtable.nkeys = 0;
    manager->heap.hashtable.nbuckets = BDD_HASHTABLE_INITIAL_SIZE;
    manager->heap.hashtable.buckets = ALLOC(bdd_node *, manager->heap.hashtable.nbuckets);
    if (manager->heap.hashtable.buckets == NIL(bdd_node *))
	(void) bdd_memfail(NIL(bdd_manager), "bdd_start");
    manager->heap.hashtable.rehash_at_nkeys = manager->heap.hashtable.nbuckets * BDD_HASHTABLE_MAXCHAINLEN;
    for (i=0; i<manager->heap.hashtable.nbuckets; i++) {
	manager->heap.hashtable.buckets[i] = NIL(bdd_node);
    }

    manager->heap.external_refs.pointer.block = NIL(bdd_bddBlock);
    manager->heap.external_refs.pointer.index = 0;
    manager->heap.external_refs.nmap = 0;
    manager->heap.external_refs.free = 0;
    manager->heap.external_refs.map = NIL(bdd_bddBlock);

    manager->heap.internal_refs.frames = NIL(bdd_safeframe);

    manager->heap.cache.itetable.on = mgr_init->ITE_cache.on;
    manager->heap.cache.itetable.invalidate_on_gc = FALSE;
    manager->heap.cache.itetable.resize_at = mgr_init->ITE_cache.resize_at;
    manager->heap.cache.itetable.max_size = mgr_init->ITE_cache.max_size;
    manager->heap.cache.itetable.nbuckets = BDD_CACHE_INITIAL_SIZE;
    manager->heap.cache.itetable.nentries = 0;
    manager->heap.cache.itetable.buckets = ALLOC(bdd_hashcache_entry *, manager->heap.cache.itetable.nbuckets);
    if (manager->heap.cache.itetable.buckets == NIL(bdd_hashcache_entry *)) {
	(void) bdd_memfail(NIL(bdd_manager), "bdd_start");
    }
    for (i = 0; i < manager->heap.cache.itetable.nbuckets; i++) {
	manager->heap.cache.itetable.buckets[i] = NIL(bdd_hashcache_entry);
    }

    manager->heap.cache.consttable.on = mgr_init->ITE_const_cache.on; 
    manager->heap.cache.consttable.invalidate_on_gc = FALSE;
    manager->heap.cache.consttable.resize_at = mgr_init->ITE_const_cache.resize_at;
    manager->heap.cache.consttable.max_size = mgr_init->ITE_const_cache.max_size;
    manager->heap.cache.consttable.nbuckets = BDD_CACHE_INITIAL_SIZE;
    manager->heap.cache.consttable.nentries = 0;
    manager->heap.cache.consttable.buckets = ALLOC(bdd_constcache_entry *, manager->heap.cache.consttable.nbuckets); 
    if (manager->heap.cache.consttable.buckets == NIL(bdd_constcache_entry *)) {  
	(void) bdd_memfail(NIL(bdd_manager), "bdd_start");
    }
    for (i = 0; i < manager->heap.cache.consttable.nbuckets; i++) {
	manager->heap.cache.consttable.buckets[i] = NIL(bdd_constcache_entry);
    }

    manager->heap.cache.adhoc.on = mgr_init->adhoc_cache.on; /* if defined, caching is on */
    manager->heap.cache.adhoc.max_size = mgr_init->adhoc_cache.max_size; 
    manager->heap.cache.adhoc.table = NIL(st_table); /* initially undefined */

    for (h=0; h<sizeof_el(manager->heap.half); h++) {
        manager->heap.half[h].inuse.top = NIL(bdd_nodeBlock);
        manager->heap.half[h].inuse.tail = &manager->heap.half[h].inuse.top;
        manager->heap.half[h].free = NIL(bdd_nodeBlock);
    }

    manager->heap.init_node_blocks = mgr_init->nodes.init_blocks;
    manager->heap.pointer.block = NIL(bdd_nodeBlock);
    manager->heap.pointer.index = 0;

    manager->heap.gc.on = mgr_init->garbage_collector.on;
    manager->heap.gc.halfspace = 0; /* start at halfspace 0 */
    manager->heap.gc.node_ratio = mgr_init->nodes.ratio;
    manager->heap.gc.status.open_generators = 0;

#if defined(BDD_DEBUG)		/* { */
    /*
     *    Debug stuff ...
     */
#if defined(BDD_DEBUG_AGE) || defined(BDD_DEBUG_LIFESPAN) /* { */
    manager->debug.gc.age = 0;
#endif				/* } */
#if defined(BDD_DEBUG_UID) || defined(BDD_DEBUG_LIFESPAN) /* { */
    manager->debug.gc.uniqueId = 0;
#endif				/* } */
#endif				/* } */

    manager->heap.gc.during.start.index = 0;
    manager->heap.gc.during.start.block = NIL(bdd_nodeBlock);
    manager->memory.daemon = mgr_init->memory.daemon; 
    manager->memory.limit = mgr_init->memory.limit;

    /*
     *    Clear out the stats
     */
    manager->heap.stats.cache.hashtable.hits = 0;
    manager->heap.stats.cache.hashtable.misses = 0;
    manager->heap.stats.cache.hashtable.collisions = 0;
    manager->heap.stats.cache.hashtable.inserts = 0;

    manager->heap.stats.cache.itetable = manager->heap.stats.cache.hashtable;
    manager->heap.stats.cache.consttable = manager->heap.stats.cache.hashtable;
    manager->heap.stats.cache.adhoc = manager->heap.stats.cache.hashtable;

    manager->heap.stats.ITE_ops.calls = 0;
    manager->heap.stats.ITE_ops.returns.trivial = 0;
    manager->heap.stats.ITE_ops.returns.cached = 0;
    manager->heap.stats.ITE_ops.returns.full = 0;

    manager->heap.stats.ITE_constant_ops = manager->heap.stats.ITE_ops;
    manager->heap.stats.adhoc_ops = manager->heap.stats.ITE_ops;

    manager->heap.stats.blocks.total = 0;

    manager->heap.stats.nodes.used = 0;
    manager->heap.stats.nodes.unused = 0;
    manager->heap.stats.nodes.total = 0;
    manager->heap.stats.nodes.peak = 0;

    manager->heap.stats.extptrs.used = 0;
    manager->heap.stats.extptrs.total = manager->heap.external_refs.nmap;
    manager->heap.stats.extptrs.unused = manager->heap.stats.extptrs.total;
    manager->heap.stats.extptrs.blocks = 0;

    manager->heap.stats.gc.times = 0;
    manager->heap.stats.gc.nodes_collected = 0;
    manager->heap.stats.gc.runtime = 0;
 
    manager->heap.stats.memory.first_sbrk = manager->heap.stats.memory.last_sbrk = (int) sbrk(0);
    manager->heap.stats.memory.manager = 0;
    manager->heap.stats.memory.nodes = 0;
    manager->heap.stats.memory.hashtable = 0;
    manager->heap.stats.memory.ext_ptrs = 0;
    manager->heap.stats.memory.ITE_cache = 0;
    manager->heap.stats.memory.ITE_const_cache = 0;
    manager->heap.stats.memory.adhoc_cache = 0;
    manager->heap.stats.memory.total = 0;

#if defined(BDD_DEBUG)		/* { */
#if defined(BDD_DEBUG_LIFESPAN)	/* { */
    /* this must be before any calls to find_or_add or (transitively) new_node */
    {
	char tracefile[MAXPATHLEN];

	(void) sprintf(tracefile, DEBUG_LIFESPAN_TRACEFILE, getpid());
	manager->debug.lifespan.trace = fopen(tracefile, "w");
	(void) fprintf(stderr, "tracefile is %s\n", tracefile);
    }
    BDD_ASSERT(manager->debug.lifespan.trace != NIL(FILE));

    /*
     *    Start the recording by indicating that we're at generation 0
     */
    (void) fprintf(manager->debug.lifespan.trace, "g %d\n", manager->debug.gc.age);
#endif				/* } */
#if defined(BDD_FLIGHT_RECORDER)	/* { */
    /* this must be before any bdd-manipulating calls are made */
    {
	char logfile[MAXPATHLEN];

	(void) sprintf(logfile, FLIGHT_RECORDER_LOGFILE, getpid());
	manager->debug.flight_recorder.log = fopen(logfile, "w");
	(void) fprintf(stderr, "logfile is %s\n", logfile);
    }
    BDD_ASSERT(manager->debug.flight_recorder.log != NIL(FILE));
#endif				/* } */
#endif				/* } */

    /*
     *    Now initialize all of the bdd-algorithm stuff. This first call to find_or_add causes 
     *    manager->heap.init_node_blocks to be allocated. 
     */
    manager->bdd.one = bdd_find_or_add(manager, /* variableId */ BDD_ONE_ID, NIL(bdd_node), NIL(bdd_node));
    manager->bdd.nvariables = nvariables;

    /*
     *    With the understanding that zeroing out the structure will
     *    mean niling out all of the pointers inside the structure.
     */
    bzero(&manager->hooks, sizeof (manager->hooks));

#if defined(BDD_DEBUG)		/* { */
#if defined(BDD_NO_GC)		/* { */
    /* this is independent of defined(DEBUG) */
    manager->heap.gc.on = FALSE;
#endif				/* } */
#endif				/* } */
    
    return (manager);
}

/*
 *    bdd_start - initialize a bdd manager
 *
 *    Given the number of variables, create and initialize a bdd_manager with
 *    the default settings.
 *
 *    return the new bdd manager
 */
bdd_manager *
bdd_start(nvariables)
int nvariables;		/* the maximum number of variables allowable in this bdd manager */
{
    bdd_mgr_init mgr_init;

    (void) bdd_set_mgr_init_dflts(&mgr_init);

    return (bdd_start_with_params(nvariables, &mgr_init));
}

/*
 * bdd_register_daemon
 *
 * Register the function to be called when the memory limit is exceeded.  
 */
void
bdd_register_daemon(manager, daemon)
bdd_manager *manager;
void (*daemon)();
{
    manager->memory.daemon = daemon; 
    return;
}

