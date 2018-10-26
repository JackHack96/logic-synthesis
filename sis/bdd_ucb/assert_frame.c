
#include <stdio.h>	/* for BDD_DEBUG_LIFESPAN */

#include "util.h"
#include "array.h"
#include "st.h"

#include "bdd.h"
#include "bdd_int.h"



static void assert_okay();

/*
 *    bdd_assert_frames_correct - assert that all of the safe frames are okay
 *
 *    Traverse all of the internal references and touch them
 *
 *    return if correct or don't
 */
void
bdd_assert_frames_correct(manager)
bdd_manager *manager;
{
	bdd_safeframe *sf;
	bdd_safenode *sn;

	for (sf=manager->heap.internal_refs.frames; sf != NIL(bdd_safeframe); sf=sf->prev) {
	    for (sn=sf->nodes; sn != NIL(bdd_safenode); sn=sn->next) {
		if (sn->arg != NIL(bdd_node *))
		    assert_okay(manager, *sn->arg);
		assert_okay(manager, sn->node);
	    }
	}
}

/*
 *    assert_okay - assert that the pointer is okay
 *
 *    return nothing, just do it.
 */
static void
assert_okay(manager, node)
bdd_manager *manager;
bdd_node *node;
{
	bdd_node *reg_node;

	if (node == NIL(bdd_node))
	    return;	/* okay */

	reg_node = BDD_REGULAR(node);

#if defined(BDD_DEBUG_GC) /* { */
	BDD_ASSERT(reg_node->halfspace == manager->heap.gc.halfspace);
#endif /* } */

	BDD_ASSERT_NOT_BROKEN_HEART(reg_node);	

	/* okay */
}
