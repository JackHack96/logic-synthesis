/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/bdd_ucb/assert_heap.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:15:01 $
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
 * $Source: /users/pchong/CVS/sis/sis/bdd_ucb/assert_heap.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:15:01 $
 * $Log: assert_heap.c,v $
 * Revision 1.1.1.1  2004/02/07 10:15:01  pchong
 * imported
 *
 * Revision 1.3  1992/09/21  23:30:31  sis
 * Updates from Tom Shiple - this is BDD package release 2.4.
 *
 * Revision 1.3  1992/09/21  23:30:31  sis
 * Updates from Tom Shiple - this is BDD package release 2.4.
 *
 * Revision 1.2  1992/09/19  02:06:29  shiple
 * Version 2.4
 * Prefaced compile time debug switches with BDD_. Added typecast to void to some function calls.
 * Changed ITE and ITE_const caches to be array of pointers.
 *
 * Revision 1.1  1992/07/29  00:26:39  shiple
 * Initial revision
 *
 * Revision 1.2  1992/05/06  18:51:03  sis
 * SIS release 1.1
 *
 * Revision 1.1  92/01/08  17:34:23  sis
 * Initial revision
 * 
 * Revision 1.1  91/03/27  14:35:25  shiple
 * Initial revision
 * 
 *
 */

static void assert_okay();

/*
 *    bdd_assert_heap_correct - look over the heap and say its okay.
 *
 *    return if okay, fail if bad
 */
void
bdd_assert_heap_correct(manager)
bdd_manager *manager;
{
	bdd_nodeBlock *block;
	bdd_node *node;
	bdd_hashcache_entry *hentry;
	bdd_constcache_entry *centry;
	bdd_adhoccache_key *key;
	bdd_safeframe *sf;
	bdd_safenode *sn;
	bdd_bddBlock *bblock;
	bdd_t *bnode;
	int node_i, i;
	st_generator *gen;

	(void) assert_okay(manager, manager->bdd.one);

	for (block=manager->heap.half[manager->heap.gc.halfspace].inuse.top;
		block != NIL(bdd_nodeBlock); block=block->next) {
	    for (node_i=0; node_i<block->used; node_i++) {
		node = &block->subheap[node_i];
		(void) assert_okay(manager, node->T);
		(void) assert_okay(manager, node->E);
	    }
	}

	for (sf=manager->heap.internal_refs.frames; sf != NIL(bdd_safeframe); sf=sf->prev) {
	    for (sn=sf->nodes; sn != NIL(bdd_safenode); sn=sn->next) {
		if (sn->arg != NIL(bdd_node *))
		    (void) assert_okay(manager, *sn->arg);
		(void) assert_okay(manager, sn->node);
	    }
	}

	for (bblock=manager->heap.external_refs.map;
		bblock != NIL(bdd_bddBlock); bblock=bblock->next) {
	    for (i=0; i<sizeof_el (bblock->subheap); i++) {
		bnode = &bblock->subheap[i];
		if ( ! bnode->free )
		    (void) assert_okay(manager, bnode->node);
	    }
	}

	for (i = 0; i < manager->heap.cache.itetable.nbuckets; i++) {
	    hentry = manager->heap.cache.itetable.buckets[i];
            if (hentry != NIL(bdd_hashcache_entry)) {
  	        (void) assert_okay(manager, hentry->ITE.f);
	        (void) assert_okay(manager, hentry->ITE.g);
	        (void) assert_okay(manager, hentry->ITE.h);
	        (void) assert_okay(manager, hentry->data);
            }
	}

	for (i = 0; i < manager->heap.cache.consttable.nbuckets; i++) {
	    centry = manager->heap.cache.consttable.buckets[i];
            if (centry != NIL(bdd_constcache_entry)) {
	        (void) assert_okay(manager, centry->ITE.f);
	        (void) assert_okay(manager, centry->ITE.g);
	        (void) assert_okay(manager, centry->ITE.h);
	        /* ignore centry->data */
            }
	}

	if (manager->heap.cache.adhoc.table != NIL(st_table)) {
	    gen = st_init_gen(manager->heap.cache.adhoc.table);
	    while (st_gen(gen, (refany*) &key, (refany*) &node)) {
		(void) assert_okay(manager, key->f);
		(void) assert_okay(manager, key->g);
		/* ignore key->v */
		(void) assert_okay(manager, node);
	    }
	    (void) st_free_gen(gen);
	}
}

static void
assert_okay(manager, node)
bdd_manager *manager;
bdd_node *node;		/* may be complemented */
{
	bdd_node *reg_node;

	if (node == NIL(bdd_node))
	    return;	/* okay */

	reg_node = BDD_REGULAR(node);

#if defined(BDD_DEBUG_GC) /* { */
	BDD_ASSERT(reg_node->halfspace == manager->heap.gc.halfspace);
#endif /* } */

	BDD_ASSERT_NOT_BROKEN_HEART(reg_node);	

	return;	/* okay */
}
