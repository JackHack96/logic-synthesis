/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/bdd_ucb/assert_frame.c,v $
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
 * $Source: /users/pchong/CVS/sis/sis/bdd_ucb/assert_frame.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:15:01 $
 * $Log: assert_frame.c,v $
 * Revision 1.1.1.1  2004/02/07 10:15:01  pchong
 * imported
 *
 * Revision 1.3  1992/09/21  23:30:31  sis
 * Updates from Tom Shiple - this is BDD package release 2.4.
 *
 * Revision 1.3  1992/09/21  23:30:31  sis
 * Updates from Tom Shiple - this is BDD package release 2.4.
 *
 * Revision 1.2  1992/09/19  01:44:15  shiple
 * Version 2.4
 * Prefaced compile time debug switches with BDD_.
 *
 * Revision 1.1  1992/07/29  00:26:38  shiple
 * Initial revision
 *
 * Revision 1.2  1992/05/06  18:51:03  sis
 * SIS release 1.1
 *
 * Revision 1.1  92/01/08  17:34:23  sis
 * Initial revision
 * 
 * Revision 1.1  91/04/11  20:58:22  shiple
 * Initial revision
 * 
 *
 */

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
