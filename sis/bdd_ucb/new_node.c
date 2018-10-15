/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/bdd_ucb/new_node.c,v $
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
 * $Source: /users/pchong/CVS/sis/sis/bdd_ucb/new_node.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:15:02 $
 * $Log: new_node.c,v $
 * Revision 1.1.1.1  2004/02/07 10:15:02  pchong
 * imported
 *
 * Revision 1.4  1993/02/25  01:09:39  sis
 * Shiple updates; 2/24/93.  See Shiple's RCS message.
 *
 * Revision 1.4  1993/02/25  01:09:39  sis
 * Shiple updates; 2/24/93.  See Shiple's RCS message.
 *
 * Revision 1.3  1993/01/16  01:19:23  shiple
 * Added comment noting that the BDD package assumes ALLOC returns NIL when it can't
 * allocate more memory.
 *
 * Revision 1.2  1992/09/19  03:19:08  shiple
 * Version 2.4
 * Prefaced compile time debug switches with BDD_.  Added peak node usage.
 * Fixed bug in function new_block which caused an additional node block
 * to be added to the free list at the start of each garbage collection.
 * Changed name new_block to get_node_block, added function get_block_from_free_list.
 * Added use of manager->heap.init_node_blocks and manager->heap.gc.node_ratio.
 * Add usage of bdd_will_exceed_mem_limit in add_block_to_free_list.
 *
 * Revision 1.1  1992/07/29  00:27:10  shiple
 * Initial revision
 *
 * Revision 1.2  1992/05/06  18:51:03  sis
 * SIS release 1.1
 *
 * Revision 1.1  92/01/08  17:34:40  sis
 * Initial revision
 * 
 * Revision 1.1  91/03/27  14:35:45  shiple
 * Initial revision
 * 
 *
 */

static void get_node_block();
static int add_block_to_free_list();
static void get_block_from_free_list();


/*
 *    bdd_new_node - get a new node and fill in its value
 *
 *    The new node is not put in any hash table or cache
 *
 *    return the new node
 */
bdd_node *
bdd_new_node(manager, variableId, T, E, old)
bdd_manager *manager;
bdd_variableId variableId;
bdd_node *T;
bdd_node *E;
bdd_node *old;	/* NIL for a new node, non-nil during gc (only used to communicate debug info) */
{
	bdd_safeframe frame;
	bdd_safenode safe_T, safe_E, safe_old,
		new;
	int index;

	/*
	 *    WATCHOUT - be sure not to call the version of ``safe frame end''
	 *    which does frame assertion checks.   This routine is called inside
	 *    of the garbage-collector, and so the frames are in an inconsistent
	 *    state by definition.    
	 */
	bdd_safeframe__start(manager, frame);	
	bdd_safenode__link(manager, safe_T, T);
	bdd_safenode__link(manager, safe_E, E);
	bdd_safenode__link(manager, safe_old, old);
	bdd_safenode__declare(manager, new);

	/*
	 *    If this is the first block, or, the block is full,
	 *    then get a new block for the heap.  This means getting a block from
         *    the free list, if it is nonempty, or allocating a new block.  If
         *    this function is called from relocate within bdd_garbage_collect, then
         *    it will always be the case that the free list is nonempty.  That's because
         *    the number of objects collected must be less than or equal to the original
         *    number of objects.
	 */
	if (manager->heap.pointer.block == NIL(bdd_nodeBlock) ||
		manager->heap.pointer.index >= sizeof_el (manager->heap.pointer.block->subheap)) {

            /*
             * It should never occur that we are during gc and the free list is empty.
             */
            BDD_ASSERT(!((old!=NIL(bdd_node))&&(manager->heap.half[manager->heap.gc.halfspace].free==NIL(bdd_nodeBlock))));
	    (void) get_node_block(manager);
	}

	index = manager->heap.pointer.index++;
	manager->heap.pointer.block->used++;
	new.node = &manager->heap.pointer.block->subheap[index];

	new.node->id = variableId;
	new.node->T = T;
	new.node->E = E;
	new.node->next = NIL(bdd_node);

	/*
	 *    Read between the #ifdefs carefully.  What is going on here is
	 *    rather simple, but the ifdefs get too much in the way to make
	 *    it very readable.
	 *
	 *    if (old == nil) {
	 *        - then not during gc so ...
	 *        - assign the current gc generation
	 *        - assign a new uniqueId
	 *        - log the creation of the object
	 *    } else {
	 *        - else it is during a gc
	 *        - copy over the previous uniqueId
	 *        - copy over the previus age (of creation)
	 *        - do not log any creation (b/c it really wasn't)
	 *    }
	 */
	if (old == NIL(bdd_node)) {  /* not during gc */
#if defined(BDD_DEBUG_AGE) || defined(BDD_DEBUG_LIFESPAN) /* { */
	    new.node->age = manager->debug.gc.age;
#endif /* } */
#if defined(BDD_DEBUG_UID) || defined(BDD_DEBUG_LIFESPAN) /* { */
	    new.node->uniqueId = manager->debug.gc.uniqueId++;
#endif /* } */
#if defined(BDD_DEBUG_LIFESPAN) /* { */
	    fprintf(manager->debug.lifespan.trace, "c %d\n", new.node->uniqueId);
#endif /* } */
	} else {  /* during gc */
#if defined(BDD_DEBUG_AGE) || defined(BDD_DEBUG_LIFESPAN) /* { */
	    new.node->age = old->age;
#endif /* } */
#if defined(BDD_DEBUG_UID) || defined(BDD_DEBUG_LIFESPAN) /* { */
	    new.node->uniqueId = old->uniqueId;
#endif /* } */
	}

#if defined(BDD_DEBUG_GC) /* { */
	new.node->halfspace = manager->heap.gc.halfspace;
#endif /* } */

	manager->heap.stats.nodes.used++;
        manager->heap.stats.nodes.peak = MAX(manager->heap.stats.nodes.peak, manager->heap.stats.nodes.used); 
	manager->heap.stats.nodes.unused--;

	/*
	 *    WATCHOUT - be sure not to call the version of ``safe frame end''
	 *    which does frame assertion checks.   This routine is called inside
	 *    of the garbage-collector, and so the frames are in an inconsistent
	 *    state by definition.    
	 */
	bdd_safeframe__end(manager);
	return (new.node);
}

static void
get_node_block(manager)
bdd_manager *manager;
{
    boolean request_status, return_status;
    int i;

    if (manager->heap.half[manager->heap.gc.halfspace].free != NIL(bdd_nodeBlock)) {
	/*
	 * If the free list is not empty, then just take a block off the free list.
         * If we are garbage collecting and relocating objects, then this condition should
         * always be satisfied.
	 */
	(void) get_block_from_free_list(manager);
    } else {
	if (manager->heap.half[manager->heap.gc.halfspace].inuse.top == NIL(bdd_nodeBlock)) {
	    /*
	     * The free list is empty and the inuse list is empty.  Thus, add manager->heap.init_node_blocks to
	     * the free list, and then take one off the free list (a little bit redundant).
             * Use the request_status flag to make sure that we get at least one block.
	     */
            request_status = FALSE;
            for (i = 1; i <= manager->heap.init_node_blocks; i++) {   
 	        return_status = add_block_to_free_list(manager, request_status);              

                /*
                 * If no more blocks can be allocated, then stop asking for more.
                 */
                if (return_status == FALSE) {
                    break;
                }
		request_status = TRUE;		/* getting more becomes optional */
	    }
	    (void) get_block_from_free_list(manager);
	} else {
	    /*
	     * The free list is empty, and the inuse list is full.  Time to garbage collect.
	     */
	    (void) bdd_garbage_collect(manager);

	    /*
	     * We want to provide a buffer for the heap to grow.  Otherwise, the heap
	     * could be, say 98% full, and we will be garbage collecting very frequently.
	     */

            /*
	     * If no garbage was collected, then the inuse list will be filled up and the 
	     * free list will be empty.  In this case, we must get at least one new block,
	     * so we set request_status to FALSE.  Otherwise, getting more blocks is optional.
	     */
	    if ((manager->heap.pointer.index >= sizeof_el (manager->heap.pointer.block->subheap))
		&& (manager->heap.half[manager->heap.gc.halfspace].free == NIL(bdd_nodeBlock))) {
		request_status = FALSE;
	    } else {
		request_status = TRUE;
	    }

            /*
	     * Policy: While the ratio of used to unused nodes is greater than manager->heap.gc.node_ratio, 
             * keep adding blocks to the free list as long as they are available.  This is to provide room for growth.
             */
	    while (manager->heap.stats.nodes.used > (manager->heap.stats.nodes.unused * manager->heap.gc.node_ratio)) {
		return_status = add_block_to_free_list(manager, request_status);

                /*
                 * If no more blocks can be allocated, then stop asking for more.
                 */
                if (return_status == FALSE) {
                    break;
                }
		request_status = TRUE;		/* getting more becomes optional */
	    }

	    /*
	     * If the inuse list is filled up, then at this point, we are guaranteed to have at least 
             * one block on the free list.  In this case, take a block off the free list.  If the inuse
             * list is not filled up, then the next bdd_node will be allocated in the current inuse block.
	     */
	    if (manager->heap.pointer.index >= sizeof_el (manager->heap.pointer.block->subheap)) {
		(void) get_block_from_free_list(manager);
	    }
	}
    } /* close: if (manager->heap.half[manager->heap.gc.halfspace].free != NIL(bdd_nodeBlock)) { */
    return;
}

/*
 * Assume there is at least one block on the free list.  Take a block off the free list
 * and put it onto the inuse list.  Set the manager to allocate next bdd_node in the
 * first slot of the block taken from the free list.
 */
static void
get_block_from_free_list(manager)
bdd_manager *manager;
{
    bdd_nodeBlock *f;

    /*
     * We assume that there is at least one block on the free list.
     */
    BDD_ASSERT(manager->heap.half[manager->heap.gc.halfspace].free != NIL(bdd_nodeBlock));

    /*
     * Take the next block off of the head of the free list, and update the free list pointer.
     */
    f = manager->heap.half[manager->heap.gc.halfspace].free;
    manager->heap.half[manager->heap.gc.halfspace].free = f->next;

    /*
     * Initialize it.
     */
    f->used = 0;
    f->next = NIL(bdd_nodeBlock);

    /*
     * Update the inuse pointers.
     */
    *manager->heap.half[manager->heap.gc.halfspace].inuse.tail = f;
    manager->heap.half[manager->heap.gc.halfspace].inuse.tail = &f->next;

    /*
     * Tell manager where to allocate next bdd_node.
     */
    manager->heap.pointer.block = f;
    manager->heap.pointer.index = 0;

    /*
     *    Do not modify any of stats.blocks.total, stats.nodes.unused or stats.nodes.total
     *    because neither changed when a block on the free list is used; these stats
     *    were changed in add_block_to_free_list().
     */
}

/*
 *    add_block_to_free_list
 *
 *    Make two new blocks and stick one in each halfspace's free list
 *
 */
static int
add_block_to_free_list(manager, optionalp)
bdd_manager *manager;
boolean optionalp;		/* if memory allocation fails, then don't call fail() */
{
	bdd_nodeBlock *b[2];
	int h;

        /*
         * Check to see if allocating 2 more blocks will cause the manager memory limit to be exceeded.
         * If so, and if optionalp is set, then just return FALSE, indicating that we were not
         * successful in allocating more blocks. If optionalp is FALSE, then call bdd_will_exceed_mem_limit
         * again, this time indicating that we want to quit this computation.
         */
        if (bdd_will_exceed_mem_limit(manager, (2 * sizeof(bdd_nodeBlock)), FALSE) == TRUE) { 
            if (optionalp) {
	        return FALSE;
            } else {
                (void) bdd_will_exceed_mem_limit(manager, (2 * sizeof(bdd_nodeBlock)), TRUE);
            }
	}

        /*
         * Allocate the blocks.  Note that the code following this block assumes that ALLOC will 
         * return a NIL pointer if it wasn't able to allocate the requested amount of memory 
         * (actually, all calls to ALLOC in the BDD package make this assumption).  If this 
         * assumption is violated, then, in the following case, bdd_memfail won't be called.
         */
	b[0] = (bdd_nodeBlock *) ALLOC(bdd_nodeBlock, 1);
	b[1] = (bdd_nodeBlock *) ALLOC(bdd_nodeBlock, 1);

	if (b[0] == NIL(bdd_nodeBlock) || b[1] == NIL(bdd_nodeBlock)) {
	    /*
	     *    We've blown the memory limit; back of the greedy approach
	     *    Could be that we've got enough to go on already...
	     */
	    if (optionalp) {
		if (b[0] != NIL(bdd_nodeBlock))
		    FREE(b[0]);
		if (b[1] != NIL(bdd_nodeBlock))
		    FREE(b[1]);
                /*
                 * We were NOT successful in allocating a new pair of blocks, but the optionalp 
                 * flag is set.
                 */
	        return FALSE;
	    } else {
		(void) bdd_memfail(manager, "add_block_to_free_list");
	    }
	}

	for (h=0; h<sizeof_el (manager->heap.half); h++) {
	    b[h]->used = 0;
	    b[h]->next = NIL(bdd_nodeBlock);

	    b[h]->next = manager->heap.half[h].free;
	    manager->heap.half[h].free = b[h];
	}

	manager->heap.stats.nodes.unused += sizeof_el (manager->heap.pointer.block->subheap);
	manager->heap.stats.nodes.total += sizeof_el (manager->heap.pointer.block->subheap);
	manager->heap.stats.blocks.total++;

        /*
         * We were successful in allocating a new pair of blocks.
         */
        return TRUE;
}
