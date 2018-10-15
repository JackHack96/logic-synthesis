/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/bdd_ucb/bdd_ite.c,v $
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
 * $Source: /users/pchong/CVS/sis/sis/bdd_ucb/bdd_ite.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:15:01 $
 * $Log: bdd_ite.c,v $
 * Revision 1.1.1.1  2004/02/07 10:15:01  pchong
 * imported
 *
 * Revision 1.3  1992/09/21  23:30:31  sis
 * Updates from Tom Shiple - this is BDD package release 2.4.
 *
 * Revision 1.3  1992/09/21  23:30:31  sis
 * Updates from Tom Shiple - this is BDD package release 2.4.
 *
 * Revision 1.2  1992/09/19  01:46:00  shiple
 * Version 2.4
 * Prefaced compile time debug switches with BDD_.
 *
 * Revision 1.1  1992/07/29  00:26:48  shiple
 * Initial revision
 *
 * Revision 1.2  1992/05/06  18:51:03  sis
 * SIS release 1.1
 *
 * Revision 1.1  92/01/08  17:34:27  sis
 * Initial revision
 * 
 * Revision 1.1  91/03/27  14:35:31  shiple
 * Initial revision
 * 
 *
 */

/*
 *    bdd_ite - returns the ite of three bdd's
 *
 *    ITE Identity: ITE(F, G, H) = ITE(F, G, H)	<---- no kidding
 *
 *    return the new bdd (external pointer)
 */
bdd_t *
bdd_ite(f, g, h, f_phase, g_phase, h_phase)
bdd_t *f;
bdd_t *g;
bdd_t *h;
boolean f_phase;		/* if ! f_phase then negate f */
boolean g_phase;		/* if ! g_phase then negate g */
boolean h_phase;		/* if ! h_phase then negate g */
{
    bdd_safeframe frame;
    bdd_safenode real_f, real_g, real_h, ret;
    bdd_t *i;
    bdd_manager *manager;

    if (f == NIL(bdd_t) || g == NIL(bdd_t) || h == NIL(bdd_t))
	fail("bdd_ite: invalid BDD");

    BDD_ASSERT( ! f->free );
    BDD_ASSERT( ! g->free );
    BDD_ASSERT( ! h->free );

    if (f->bdd != g->bdd || g->bdd != h->bdd)
	fail("bdd_ite: different bdd managers");

    manager = f->bdd;		/* either this or g->bdd or h->bdd will do */

    /*
     *    After the input is checked for correctness, start the safe frame
     *    f and g are already external pointers so they need not be protected
     */
    bdd_safeframe_start(manager, frame);
    bdd_safenode_declare(manager, real_f);
    bdd_safenode_declare(manager, real_g);
    bdd_safenode_declare(manager, real_h);
    bdd_safenode_declare(manager, ret);

    real_f.node = f_phase ? f->node: BDD_NOT(f->node);
    real_g.node = g_phase ? g->node: BDD_NOT(g->node);
    real_h.node = h_phase ? h->node: BDD_NOT(h->node);

    ret.node = bdd__ITE_(manager, real_f.node, real_g.node, real_h.node);

    /*
     *    End the safe frame and return the result
     */
    bdd_safeframe_end(manager);
    i = bdd_make_external_pointer(manager, ret.node, "bdd_ite");

#if defined(BDD_FLIGHT_RECORDER) /* { */
    (void) fprintf(manager->debug.flight_recorder.log, "%d <- bdd_ite(%d, %d, %d, %d, %d, %d)\n",
	    bdd_index_of_external_pointer(i),
	    bdd_index_of_external_pointer(f),
	    bdd_index_of_external_pointer(g),
	    bdd_index_of_external_pointer(h),
	    f_phase, g_phase, h_phase);
#endif /* } */

    return (i);
}
