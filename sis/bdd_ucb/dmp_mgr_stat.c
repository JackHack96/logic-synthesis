
#include <stdio.h>	/* for BDD_DEBUG_LIFESPAN */

#include "util.h"
#include "array.h"
#include "st.h"

#include "bdd.h"
#include "bdd_int.h"



/*
 *    bdd_dump_manager_stats - dump the manager statistics out
 *
 *    return nothing, just do it.
 */
void
bdd_dump_manager_stats(manager)
bdd_manager *manager;
{
    bdd_stats stats;
    int total_hashtable_queries;
    int total_itetable_lookups;
    int total_consttable_lookups;
    int total_adhoc_lookups;
    int num_frames;
    int num_nodes;
    int tot_frame_mem;
    bdd_safeframe *sf;
    bdd_safenode *sn;
    int overall_memory;
    int total_sbrk;

    /*
     * Get the manager's stats.
     */
    (void) bdd_get_stats(manager, &stats);

    /* 
     * Make intermediate calculations.
     */
    total_hashtable_queries = stats.cache.hashtable.hits + stats.cache.hashtable.misses;    
    total_itetable_lookups = stats.cache.itetable.hits + stats.cache.itetable.misses;
    total_consttable_lookups = stats.cache.consttable.hits + stats.cache.consttable.misses;
    total_adhoc_lookups = stats.cache.adhoc.hits + stats.cache.adhoc.misses;

    /*
     * Memory used by safeframes.
     */
    num_frames = 0;
    num_nodes = 0;
    for (sf = manager->heap.internal_refs.frames; sf != NIL(bdd_safeframe); sf = sf->prev) {
        num_frames++;
	for (sn = sf->nodes; sn != NIL(bdd_safenode); sn = sn->next) {
            num_nodes++;
	}
    }
    tot_frame_mem = num_frames * sizeof(bdd_safeframe) + num_nodes * sizeof(bdd_safenode);

    /*
     * Total memory usage.
     */
    overall_memory = stats.memory.total + tot_frame_mem;

    /*
     * Calculate the total sbrk.
     */
    total_sbrk = stats.memory.last_sbrk - stats.memory.first_sbrk;

#if defined(BDD_AUTOMATED_STATISTICS_GATHERING) /* { */
	/*
	 *    Write out the stuff in a way which is easy to parse. Also, include some extra info.
	 */
    (void) fprintf(stderr, "\
stats: start\n\
stats: bdd_nodeBlock %d\n\
stats: bdd_node used: %d, unused: %d, total: %d, peak: %d\n\
stats: bdd_t used: %d, unused: %d, total: %d\n\
stats: hash table total: %d, hits: %d (%4.1f%%), misses: %d (%4.1f%%)\n\
stats: ITE ops total: %d, trivial: %d (%4.1f%%), cached: %d (%4.1f%%), full: %d (%4.1f%%)\n\
stats: ITE table lookups: %d, misses: %d (%4.1f%%)\n\
stats: ITE table insertions: %d, collisions: %d (%4.1f%%)\n\
stats: ITE table entries: %d, percent of buckets: %4.1f%%\n\
stats: ITE_const ops total: %d, trivial: %d (%4.1f%%), cached: %d (%4.1f%%), full: %d (%4.1f%%)\n\
stats: ITE_const table lookups: %d, misses: %d (%4.1f%%)\n\
stats: ITE_const table insertions: %d, collisions: %d (%4.1f%%)\n\
stats: ITE_const table entries: %d, percent of buckets: %4.1f%%\n\
stats: adhoc ops total: %d, trivial: %d (%4.1f%%), cached: %d (%4.1f%%), full: %d (%4.1f%%)\n\
stats: adhoc table lookups: %d, misses: %d (%4.1f%%)\n\
stats: garbage-collections: %d\n\
stats: nodes collected: %d\n\
stats: gc runtime: %.2f sec\n\
stats: end\n\n\
mem: start (all figures in bytes)\n\
mem: manager            = %9d\n\
mem: bdd_nodes          = %9d\n\
mem: unique table bckts = %9d\n\
mem: external ptrs      = %9d\n\
mem: ITE buckets        = %9d\n\
mem: ITE entries        = %9d\n\
mem: consttable buckets = %9d\n\
mem: consttable entries = %9d\n\
mem: adhoc table        = %9d\n\
mem: safe frames        = %9d\n\
mem: overall            = %9d\n\
mem: total sbrk         = %9d\n\
mem: total overhead     = %9d (%d%%)\n\
mem: end\n\
",
    stats.blocks.total,
    stats.nodes.used, stats.nodes.unused, stats.nodes.total, stats.nodes.peak,
    stats.extptrs.used, stats.extptrs.unused, stats.extptrs.total,

    total_hashtable_queries,
    stats.cache.hashtable.hits, 
    bdd_get_percentage(stats.cache.hashtable.hits, total_hashtable_queries),
    stats.cache.hashtable.misses,
    bdd_get_percentage(stats.cache.hashtable.misses, total_hashtable_queries),

    stats.ITE_ops.calls, 
    stats.ITE_ops.returns.trivial,
    bdd_get_percentage(stats.ITE_ops.returns.trivial, stats.ITE_ops.calls),
    stats.ITE_ops.returns.cached, 
    bdd_get_percentage(stats.ITE_ops.returns.cached, stats.ITE_ops.calls),
    stats.ITE_ops.returns.full,
    bdd_get_percentage(stats.ITE_ops.returns.full, stats.ITE_ops.calls),

    total_itetable_lookups,
    stats.cache.itetable.misses,
    bdd_get_percentage(stats.cache.itetable.misses, total_itetable_lookups),

    stats.cache.itetable.inserts,
    stats.cache.itetable.collisions, 
    bdd_get_percentage(stats.cache.itetable.collisions, stats.cache.itetable.inserts),
    manager->heap.cache.itetable.nentries, 
    bdd_get_percentage(manager->heap.cache.itetable.nentries, manager->heap.cache.itetable.nbuckets),
    
    stats.ITE_constant_ops.calls, 
    stats.ITE_constant_ops.returns.trivial,
    bdd_get_percentage(stats.ITE_constant_ops.returns.trivial, stats.ITE_constant_ops.calls),
    stats.ITE_constant_ops.returns.cached, 
    bdd_get_percentage(stats.ITE_constant_ops.returns.cached, stats.ITE_constant_ops.calls),
    stats.ITE_constant_ops.returns.full,
    bdd_get_percentage(stats.ITE_constant_ops.returns.full, stats.ITE_constant_ops.calls),

    total_consttable_lookups,
    stats.cache.consttable.misses,
    bdd_get_percentage(stats.cache.consttable.misses, total_consttable_lookups),

    stats.cache.consttable.inserts,
    stats.cache.consttable.collisions, 
    bdd_get_percentage(stats.cache.consttable.collisions, stats.cache.consttable.inserts),
    manager->heap.cache.consttable.nentries, 
    bdd_get_percentage(manager->heap.cache.consttable.nentries, manager->heap.cache.consttable.nbuckets),
    
    stats.adhoc_ops.calls, 
    stats.adhoc_ops.returns.trivial,
    bdd_get_percentage(stats.adhoc_ops.returns.trivial, stats.adhoc_ops.calls),
    stats.adhoc_ops.returns.cached, 
    bdd_get_percentage(stats.adhoc_ops.returns.cached, stats.adhoc_ops.calls),
    stats.adhoc_ops.returns.full,
    bdd_get_percentage(stats.adhoc_ops.returns.full, stats.adhoc_ops.calls),

    total_adhoc_lookups,
    stats.cache.adhoc.misses,
    bdd_get_percentage(stats.cache.adhoc.misses, total_adhoc_lookups),

    stats.gc.times,
    stats.gc.nodes_collected,
    ((double) stats.gc.runtime / 1000),

    stats.memory.manager,
    stats.memory.nodes,
    stats.memory.hashtable,
    stats.memory.ext_ptrs,
    (manager->heap.cache.itetable.nbuckets * sizeof(bdd_hashcache_entry *)),
    (manager->heap.cache.itetable.nentries * sizeof(bdd_hashcache_entry)),
    (manager->heap.cache.consttable.nbuckets * sizeof(bdd_constcache_entry *)),
    (manager->heap.cache.consttable.nentries * sizeof(bdd_constcache_entry)),
    stats.memory.adhoc_cache,
    tot_frame_mem,
    overall_memory,
    total_sbrk,
    (total_sbrk - overall_memory), 
    (int) (((total_sbrk - overall_memory) * 100) / overall_memory));

#if defined(BDD_MEMORY_USAGE) /* { */
    /* 
     * Get info from ps.  
     */
    system("ps -v | egrep 'TIME|sis'");   /* header line and the process in which we are interested */
#endif /* } */

#else /* } else { */
    /*
     *    Just do the pretty thing of printing out all the 
     *    statistics in a nice pretty (e.g. human readable) way.
     */
    (void) bdd_print_stats(stats, stdout);
#endif /* } */
}



