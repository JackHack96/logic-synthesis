
#include <stdio.h>	/* for BDD_DEBUG_LIFESPAN */

#include "util.h"
#include "array.h"
#include "st.h"

#include "bdd.h"
#include "bdd_int.h"



/*
 * Calculate the numerator as a percentage of the denominator.  Assumes inputs are ints. 
 * Return 0.0 if denominator is zero.
 */
float
bdd_get_percentage(numerator, denominator)
int numerator;
int denominator;
{
    if (denominator == 0) {
        return 0.0;
    } else {
        return (((float)numerator/denominator)*100);
    }
}

/*
 *    bdd_print_stats - print the given statistics
 *
 *    return nothing, just do it.
 */
void
bdd_print_stats(stats, file)
bdd_stats stats;
FILE *file;		/* probably misout */
{

    int total_hashtable_queries;
    int total_itetable_lookups;
    int total_consttable_lookups;
    int total_adhoc_lookups;

    /* 
     * Make intermediate calculations.
     */
    total_hashtable_queries = stats.cache.hashtable.hits + stats.cache.hashtable.misses;    
    total_itetable_lookups = stats.cache.itetable.hits + stats.cache.itetable.misses;
    total_consttable_lookups = stats.cache.consttable.hits + stats.cache.consttable.misses;
    total_adhoc_lookups = stats.cache.adhoc.hits + stats.cache.adhoc.misses;

    /* 
     * Print out the stats.
     */
    fprintf(file, "\
BDD Package Statistics\n\
\n\
Blocks (bdd_nodeBlock): %d\n\
\n\
Nodes (bdd_node):\n\
        used   unused    total     peak\n\
    %8d %8d %8d %8d\n\
\n\
Extptr (bdd_t):\n\
        used   unused    total\n\
    %8d %8d %8d\n\
\n\
Hashtable:\n\
    hits:   %8d (%4.1f%%)\n\
    misses: %8d (%4.1f%%)\n\
    total:  %8d (find_or_add calls)\n\
\n\
Caches:              ITE    ITE_const     adhoc\n\
 Total calls:    %8d   %8d   %8d\n\
    trivial:    %9.1f%% %9.1f%% %9.1f%%\n\
    cached:     %9.1f%% %9.1f%% %9.1f%%\n\
    full:       %9.1f%% %9.1f%% %9.1f%%\n\
 Total lookups:  %8d   %8d   %8d\n\
    misses:     %9.1f%% %9.1f%% %9.1f%%\n\
 Total inserts:  %8d   %8d        --\n\
    collisions: %9.1f%% %9.1f%%       --\n\
\n\
Garbage Collections:\n\
    collections: %d\n\
    total nodes collected: %d\n\
    total time:  %.2f sec\n\
\n\
Memory Usage (bytes):\n\
  manager:         %9d\n\
  bdd_nodes:       %9d\n\
  hashtable:       %9d\n\
  extptrs (bdd_t): %9d\n\
  ITE cache:       %9d\n\
  ITE_const cache: %9d\n\
  adhoc cache:     %9d\n\
  total:           %9d\n\
",
    stats.blocks.total,

    stats.nodes.used,
    stats.nodes.unused,
    stats.nodes.total,
    stats.nodes.peak,

    stats.extptrs.used,
    stats.extptrs.unused,
    stats.extptrs.total,

    stats.cache.hashtable.hits, 
    bdd_get_percentage(stats.cache.hashtable.hits, total_hashtable_queries),
    stats.cache.hashtable.misses,
    bdd_get_percentage(stats.cache.hashtable.misses, total_hashtable_queries),
    total_hashtable_queries,

    stats.ITE_ops.calls,
    stats.ITE_constant_ops.calls,
    stats.adhoc_ops.calls,

    bdd_get_percentage(stats.ITE_ops.returns.trivial, stats.ITE_ops.calls),
    bdd_get_percentage(stats.ITE_constant_ops.returns.trivial, stats.ITE_constant_ops.calls),
    bdd_get_percentage(stats.adhoc_ops.returns.trivial, stats.adhoc_ops.calls),

    bdd_get_percentage(stats.ITE_ops.returns.cached, stats.ITE_ops.calls),
    bdd_get_percentage(stats.ITE_constant_ops.returns.cached, stats.ITE_constant_ops.calls),
    bdd_get_percentage(stats.adhoc_ops.returns.cached, stats.adhoc_ops.calls),

    bdd_get_percentage(stats.ITE_ops.returns.full, stats.ITE_ops.calls),
    bdd_get_percentage(stats.ITE_constant_ops.returns.full, stats.ITE_constant_ops.calls),
    bdd_get_percentage(stats.adhoc_ops.returns.full, stats.adhoc_ops.calls),

    total_itetable_lookups,
    total_consttable_lookups,
    total_adhoc_lookups,

    bdd_get_percentage(stats.cache.itetable.misses, total_itetable_lookups),
    bdd_get_percentage(stats.cache.consttable.misses, total_consttable_lookups),
    bdd_get_percentage(stats.cache.adhoc.misses, total_adhoc_lookups),

    stats.cache.itetable.inserts,
    stats.cache.consttable.inserts,

    bdd_get_percentage(stats.cache.itetable.collisions, stats.cache.itetable.inserts),
    bdd_get_percentage(stats.cache.consttable.collisions, stats.cache.consttable.inserts),

    stats.gc.times,
    stats.gc.nodes_collected,
    ((double) stats.gc.runtime / 1000),

    stats.memory.manager,
    stats.memory.nodes,
    stats.memory.hashtable,
    stats.memory.ext_ptrs,
    stats.memory.ITE_cache,
    stats.memory.ITE_const_cache,
    stats.memory.adhoc_cache,
    stats.memory.total); 
}

