
#include <stdio.h>	/* for BDD_DEBUG_LIFESPAN */

#include "util.h"
#include "array.h"
#include "st.h"

#include "bdd.h"
#include "bdd_int.h"



static void new_block();

/*
 *    bdd_make_external_pointer - make an external pointer
 *
 *    return the external pointer
 */
bdd_t *
bdd_make_external_pointer(manager, node, origin)
bdd_manager *manager;
bdd_node *node;		/* may be NIL(bdd_node) */
string origin;		/* a static string - neither allocated nor freed (must be persistent across _all_ time) */
{
    int i;
    bdd_t *new;

    /*
     *    If there are known not to be any blocks then get one.
     *    This way the following initialization condition will
     *    always be known to set the pointer.block != NIL
     */
    if (manager->heap.external_refs.free == 0) {
	(void) new_block(manager);
    }

    BDD_ASSERT(manager->heap.external_refs.free > 0);

    /*
     * If the pointer is not defined, then start at the beginning
     */
    if (manager->heap.external_refs.pointer.block == NIL(bdd_bddBlock)) {
	manager->heap.external_refs.pointer.block = manager->heap.external_refs.map;
	manager->heap.external_refs.pointer.index = 0;
    }

    BDD_ASSERT(manager->heap.external_refs.pointer.block != NIL(bdd_bddBlock));
    BDD_ASSERT(manager->heap.external_refs.free > 0);

    /*
     * Run through the external references using the roving pointer.
     * Do not go around more than once (this is controlled by stopping at the total number of
     * bdd_t in the map, namely manager->heap.external_refs.nmap).  If you find a free one, use it.
     * If a free one is not found, then new_block is called after the for loop.
     * Note that the index i is never explicitly used in the for loop.  It simply keeps track of how
     * many bdd_t entries we have examined thus far.
     */
    for (i=0; i<manager->heap.external_refs.nmap; i++) {
        /*
         * If the pointer index is at the end of the current block, then move to the next block.
         */
	if (manager->heap.external_refs.pointer.index == 
                         sizeof_el (manager->heap.external_refs.pointer.block->subheap)) {

	    manager->heap.external_refs.pointer.block = manager->heap.external_refs.pointer.block->next;
	    manager->heap.external_refs.pointer.index = 0;
	    
            /*
             * If the pointer has fallen off the end of the list, then start over.  Remember, the list
             * of block is not circularly linked.
             */
	    if (manager->heap.external_refs.pointer.block == NIL(bdd_bddBlock)) {
		manager->heap.external_refs.pointer.block = manager->heap.external_refs.map;
            }
	}

	if ( ! manager->heap.external_refs.pointer.block->subheap[manager->heap.external_refs.pointer.index].free ) {
	    /*
	     *    It is not free, so go on to the next one
	     */
	    manager->heap.external_refs.pointer.index++;
	} else {
	    /*
	     *    It is free
	     */
	    new = &manager->heap.external_refs.pointer.block->subheap[manager->heap.external_refs.pointer.index++];

	    /* set the values in the external pointer */
	    new->free = FALSE;
	    new->node = node;
	    new->bdd = manager;
#if defined(BDD_DEBUG_EXT)		/* { */
	    new->origin = origin;
#endif				/* } */

	    /* decrement the number of free external pointers */
	    manager->heap.external_refs.free--;

	    manager->heap.stats.extptrs.used++;
	    manager->heap.stats.extptrs.unused--;

	    return (new);
	}
    }

    /*
     * No free bdd_ts are available.  Allocate a new block.
     */
    (void) new_block(manager);
    BDD_ASSERT(manager->heap.external_refs.free > 0);

    /*
     *    Just make a tail-recursive call
     *    You know it will succeed b/c of the above assertion that free > 0
     */
    return (bdd_make_external_pointer(manager, node, origin));
}

/*
 *    bdd_destroy_external_pointer - destroy an external pointer
 *
 *    return nothing, just do it.
 */
void
bdd_destroy_external_pointer(ext_f)
bdd_t *ext_f;
{
    BDD_ASSERT(ext_f != NIL(bdd_t));
    if (ext_f->free)
	return;			/* the guy shouldn't be doing this, but no point in failing on it */

    /* boost the number of free external pointers */
    ext_f->bdd->heap.external_refs.free++;

    /*
     * Update the stats.
     */
    ext_f->bdd->heap.stats.extptrs.used--;
    ext_f->bdd->heap.stats.extptrs.unused++;

    /* destroy the values in the external pointer */
    ext_f->free = TRUE;
    ext_f->node = NIL(bdd_node);
    ext_f->bdd = NIL(bdd_manager);
}

/*
 *    new_block - add a new block to the map
 *
 *    Side-effect the new block onto the beginning of the list
 *
 *    return nothing, just do it.
 */
static void
new_block(manager)
bdd_manager *manager;
{
	bdd_bddBlock *block;
	int i;

	/*
	 * Make a new block.
	 */
	block = ALLOC(bdd_bddBlock, 1);
	if (block == NIL(bdd_bddBlock)) {
	    (void) bdd_memfail(manager, "new_block");
	}
        manager->heap.stats.extptrs.blocks++;

        /*
         * Initialize all the entries of the block.
         */
	for (i=0; i<sizeof_el (block->subheap); i++) {
	    block->subheap[i].free = TRUE;
	    block->subheap[i].node = NIL(bdd_node);
	    block->subheap[i].bdd = NIL(bdd_manager);
	}

	/* 
         * Add the new block to the beginning of the list.
         */
	block->next = manager->heap.external_refs.map;
	manager->heap.external_refs.map = block;

	/* increment the counts */
	manager->heap.external_refs.free += sizeof_el (block->subheap);
	manager->heap.external_refs.nmap += sizeof_el (block->subheap);

	/* make the pointer point to the beginning */
	manager->heap.external_refs.pointer.block = block;
	manager->heap.external_refs.pointer.index = 0;

	/* increment the stats */
	manager->heap.stats.extptrs.unused += sizeof_el (block->subheap);
	manager->heap.stats.extptrs.total += sizeof_el (block->subheap);
}

#if defined(BDD_FLIGHT_RECORDER) /* { */
/*
 *    bdd_index_of_external_pointer - get the index of the thing
 *
 *    return the index of the thing
 */
int
bdd_index_of_external_pointer(f)
bdd_t *f;
{
    bdd_manager *manager;
    bdd_bddBlock *block;

    BDD_ASSERT( ! f->free );
    BDD_ASSERT(f->bdd != NIL(bdd_manager));

    manager = f->bdd;

    /*
     * Find the block which contains f.  Then return f's position within the block.
     */
    for (block=manager->heap.external_refs.map; block != NIL(bdd_bddBlock); block=block->next) {
	if (&block->subheap[0] <= f && f < &block->subheap[sizeof_el (block->subheap)])
	    return (((int) f - (int) &block->subheap[0]) / sizeof (block->subheap[0]));
    }

    BDD_FAIL("bdd_index_of_external_pointer could not find the bdd_t!");
    /*NOTREACHED*/
}
#endif /* } */
