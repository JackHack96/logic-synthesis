
#include <stdio.h>    /* for BDD_DEBUG_LIFESPAN */

#include "util.h"
#include "array.h"
#include "st.h"

#include "bdd.h"
#include "bdd_int.h"


/*
 *    bdd_end - terminate a bdd manager
 *
 *    return nothing, just do it.
 */
void
bdd_end(manager)
        bdd_manager *manager;
{
    int                  h;
    bdd_nodeBlock        *b, *next_b;
    bdd_bddBlock         *bb, *next_bb;
    int                  i;
    bdd_hashcache_entry  *hentry;
    bdd_constcache_entry *centry;

    if (manager == NIL(bdd_manager))
        return;        /* for compatibility with v2.2 */

#if defined(BDD_DEBUG) /* { */
    (void) fprintf(stderr, "\n\nFINAL BDD MANAGER STATS:\n");
    (void) bdd_dump_manager_stats(manager);
#if defined(BDD_FLIGHT_RECORDER) /* { */
fclose(manager->debug.flight_recorder.log);
#endif /* } */
#if defined(BDD_DEBUG_LIFESPAN) /* { */
fclose(manager->debug.lifespan.trace);
#endif /* } */
#if defined(BDD_DEBUG_AGE) /* { */
(void) bdd_dump_node_ages(manager, stderr);
#endif /* } */
#if defined(BDD_DEBUG_EXT) /* { */
(void) bdd_dump_external_pointers(manager, stderr);
#endif /* } */
#endif /* } */

    /* Destruct in order in the structure declaration */

    /*
     *    Free the hashtable
     */
    FREE(manager->heap.hashtable.buckets);

    /*
     *    Free the ITE cache, and any entries it contains.
     */
    for (i = 0; i < manager->heap.cache.itetable.nbuckets; i++) {
        hentry = manager->heap.cache.itetable.buckets[i];
        if (hentry != NIL(bdd_hashcache_entry)) {
            FREE(hentry);
        }
    }
    FREE(manager->heap.cache.itetable.buckets);

    /*
     *    Free the ITE constant cache, and any entries it contains.
     */
    for (i = 0; i < manager->heap.cache.consttable.nbuckets; i++) {
        centry = manager->heap.cache.consttable.buckets[i];
        if (centry != NIL(bdd_constcache_entry)) {
            FREE(centry);
        }
    }
    FREE(manager->heap.cache.consttable.buckets);

    /*
     *    Free the adhoc cache.
     */
    if (manager->heap.cache.adhoc.table != NIL(st_table)) {
        bdd_adhoccache_uninit(manager);
    }

    /*
     *    Free the external references
     */
    for (bb = manager->heap.external_refs.map; bb != NIL(bdd_bddBlock); bb = next_bb) {
        next_bb = bb->next;
        FREE(bb);
    }

    /*
     *    Free the nodeBlocks in the heap halves
     */
    for (h = 0; h < sizeof_el (manager->heap.half); h++) {
        for (b = manager->heap.half[h].inuse.top; b != NIL(bdd_nodeBlock); b = next_b) {
            next_b = b->next;
            FREE(b);
        }
        for (b = manager->heap.half[h].free; b != NIL(bdd_nodeBlock); b = next_b) {
            next_b = b->next;
            FREE(b);
        }
    }

    /*
     *    DO NOT touch the mis-specific info that is freed in bdd_quit.
     *    This routine is designed to be called from network_free
     */

    FREE(manager);
}
