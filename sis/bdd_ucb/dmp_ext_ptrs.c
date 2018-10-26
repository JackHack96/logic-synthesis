
#include <stdio.h>	/* for BDD_DEBUG_LIFESPAN */

#include "util.h"
#include "array.h"
#include "st.h"

#include "bdd.h"
#include "bdd_int.h"



/*
 *    bdd_dump_external_pointers - dump all outstanding external pointers
 *
 *    For debugging, we must be able to dump all of the external pointers
 *    which are live and give some indication as to where they came from
 *    and who allocated them.   Also, it is good if an indication of the size
 *    of the bdd (partially) held down by the pointer. 
 *
 *    return nothing, just do it.
 */
void
bdd_dump_external_pointers(manager, file)
bdd_manager *manager;
FILE *file;
{
	char value_buf[1000];	/* big enuf? */
	string origin;
	bdd_bddBlock *bblock;
	bdd_t *bnode;
	int bb_i, i, create_bdd, other_bdd;

#if defined(BDD_DEBUG_EXT_ALL) && defined(BDD_AUTOMATED_STATISTICS_GATHERING) /* { */
	(void) fprintf(file, "all-external-pointers: start\n");
#endif /* } */

	/*
	 *    Foreach subheap in the map
	 */
	create_bdd = 0;
	other_bdd = 0;
	for (bblock=manager->heap.external_refs.map, bb_i=0;
		bblock != NIL(bdd_bddBlock); bblock=bblock->next, bb_i++) {
	    /*
	     *    Foreach member of the subheap
	     */
	    for (i=0; i<sizeof_el (bblock->subheap); i++) {
		bnode = &bblock->subheap[i];

		/*
		 *    If its not free, then print it out
		 */
		if (!bnode->free) {
		    /*
		     *    Get some idea of the size of the thing held
		     */
		    if (bnode->node == BDD_ZERO(manager)) {
			strcpy(value_buf, "the zero");
		    } else if (bnode->node == BDD_ONE(manager)) {
			strcpy(value_buf, "the one");
		    } else {
			int size;

			size = bdd_size(bnode);
			sprintf(value_buf, "bdd of %d node%s", size, size == 1 ? "": "s");
		    }

		    /*
		     *    Get some idea of WHO allocated this
		     */
#if defined(BDD_DEBUG_EXT) /* { */
		    /* if debugging external pointers, then bnode-> origin exists */
		    origin = bnode->origin;
#else /* } else { */
		    /* otherwise it doesn't exist, and we have to just guess */
		    origin = "unknown";	/* because it was never tracked */
#endif /* } */

		    if (strcmp(origin, "bdd_create_bdd") == 0) {
			/*
			 *    Don't print these b/c there are usually alot
			 */
			create_bdd++;
		    } else {
			/*
			 *    The actual printing ...
			 */
			other_bdd++;

#if defined(BDD_DEBUG_EXT_ALL) /* { */
#if defined(BDD_AUTOMATED_STATISTICS_GATHERING) /* { */
			(void) fprintf(file, "\
all-external-pointers: bdd_t: %d free: %s node: %s origin: %s\n\
", i + bb_i * sizeof_el (bblock->subheap),
	bnode->free ? "true": "false",
	value_buf, origin);
#else /* } else { */
			(void) fprintf(file, "\
bdd_t[%d] = { free: %s, node: %s, origin: %s }\n\
", i + bb_i * sizeof_el (bblock->subheap),
	bnode->free ? "true": "false",
	value_buf, origin);
#endif /* } */
#endif /* } */
		    }
		}
	    }
	}

#if defined(BDD_DEBUG_EXT_ALL) && defined(BDD_AUTOMATED_STATISTICS_GATHERING) /* { */
	(void) fprintf(file, "all-external-pointers: end\n");
#endif /* } */

#if defined(BDD_AUTOMATED_STATISTICS_GATHERING) /* { */
	(void) fprintf(file, "\
external-pointers: bdd_create_bdd: %d: other: %d\n\
", create_bdd, other_bdd);
#else /* } else { */
	(void) fprintf(file, "\
Outstanding External Pointers\n\
    due to bdd_create_bdd   %d\n\
    due to other operations %d\n\
", create_bdd, other_bdd);
#endif /* } */
}
