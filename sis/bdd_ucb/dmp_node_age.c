
#include <stdio.h>	/* for BDD_DEBUG_LIFESPAN */

#include "util.h"
#include "array.h"
#include "st.h"

#include "bdd.h"
#include "bdd_int.h"



/*
 *    bdd_dump_node_ages - dump the ages of all the nodes (summary)
 *
 *    For debugging, we must be able to determine the relative frequency
 *    of node ages in the heap.   This is used to validate the generational
 *    strategy which says that old things stay and young things die young.
 *
 *    return nothing, just do it.
 */
void
bdd_dump_node_ages(manager, file)
bdd_manager *manager;
FILE *file;
{
	int ages[1000];		/* TODO - big enuf? */
	bdd_nodeBlock *block;
	bdd_node *node;
	int i;

#ifndef BDD_DEBUG_AGE /* { */
	(void) fprintf(stderr, "\
bdd_dump_node_ages: the bdd package is not compiled with DEBUG_AGE\n\
\tso calling this function cannot produce any results\n\
");
#else /* } else { */
	for (i=0; i<sizeof_el (ages); i++)
	    ages[i] = 0;

	/*
	 *    Foreach subheap in the heap (the inuse part only),
	 *    collect its age statistics for dumping out later.
	 */
	for (block=manager->heap.half[manager->heap.gc.halfspace].inuse.top;
		block != NIL(bdd_nodeBlock); block=block->next) {
	    /*
	     *    Foreach member of the subheap
	     */
	    for (i=0; i<block->used; i++) {
		node = &block->subheap[i];

		/*
		 *    If its free, then print it out
		 */
		ages[node->age]++;
	    }
	}

#if defined(BDD_AUTOMATED_STATISTICS_GATHERING) /* { */
	(void) fprintf(file, "age-summary: start\n");
	for (i=0; i<sizeof_el (ages); i++) {
	    if (ages[i] != 0) {
		(void) fprintf(file, "age-summary: %d\t%d\n", i, ages[i]);
	    }
	}	
	(void) fprintf(file, "age-summary: end\n");
#else /* } else { */
	(void) fprintf(file, "\
Age Distribution in bdd_nodes\n\
Age\tCount\n\
");
	for (i=0; i<sizeof_el (ages); i++) {
	    if (ages[i] != 0) {
		(void) fprintf(file, "%d\t%d\n", i, ages[i]);
	    }
	}
#endif /* } */
#endif /* } */
}
