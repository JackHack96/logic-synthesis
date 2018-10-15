/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/bdd_ucb/bdd_cofactor.c,v $
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
 * $Source: /users/pchong/CVS/sis/sis/bdd_ucb/bdd_cofactor.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:15:01 $
 * $Log: bdd_cofactor.c,v $
 * Revision 1.1.1.1  2004/02/07 10:15:01  pchong
 * imported
 *
 * Revision 1.4  1993/02/25  01:09:39  sis
 * Shiple updates; 2/24/93.  See Shiple's RCS message.
 *
 * Revision 1.4  1993/02/25  01:09:39  sis
 * Shiple updates; 2/24/93.  See Shiple's RCS message.
 *
 * Revision 1.3  1993/01/20  23:45:49  shiple
 * Moved block of code.  Fixed three errors in comments.
 *
 * Revision 1.2  1992/09/19  02:23:19  shiple
 * Version 2.4
 * > Prefaced compile time debug switches with BDD_. Added typecast to void to some function calls.
 * Added adhoc_ops stats. Restructured _cofactor so that the cases where one branch of g
 * is ZERO are properly hashed and accounted for in the statistics.
 *
 * Revision 1.1  1992/07/29  00:26:44  shiple
 * Initial revision
 *
 * Revision 1.2  1992/05/06  18:51:03  sis
 * SIS release 1.1
 *
 * Revision 1.1  92/01/08  17:34:25  sis
 * Initial revision
 * 
 * Revision 1.1  91/03/27  14:35:28  shiple
 * Initial revision
 * 
 *
 */

/*
 *    ARGH!   The word ``cofactor'' is an exported function in mis,
 *    so we are not allowed to use it here!   ARGH!  These guys don't
 *    believe in package prefixes!  ARGH!
 */
static bdd_node *_cofactor();

/*
 *    bdd_cofactor - cofactor f by g
 *
 *    return the new bdd (external pointer)
 */
bdd_t *
bdd_cofactor(f, g)
bdd_t *f;
bdd_t *g;
{
    bdd_safeframe frame;
    bdd_safenode ret;
    bdd_manager *manager;
    bdd_t *h;

    if (f == NIL(bdd_t) || g == NIL(bdd_t))
	fail("bdd_cofactor: invalid BDD");

    BDD_ASSERT( ! f->free );
    BDD_ASSERT( ! g->free );

    if (g->node == BDD_ZERO(g->bdd))
	fail("bdd_cofactor: cofactor wrt zero not defined");

    if (f->bdd != g->bdd)
	fail("bdd_cofactor: different bdd managers");

    manager = f->bdd;		/* either this or g->bdd will do */

    /*
     *    Start the safe frame now that we know the input is correct.
     *    f and g are external pointers so they need not be protected
     */
    bdd_safeframe_start(manager, frame);
    bdd_safenode_declare(manager, ret);

    (void) bdd_adhoccache_init(manager);

    ret.node = _cofactor(manager, f->node, g->node);

    /*
     *    Free the cache, end the safe frame and return the (safe) result
     */
    (void) bdd_adhoccache_uninit(manager);
    h = bdd_make_external_pointer(manager, ret.node, "bdd_cofactor");
    bdd_safeframe_end(manager);

#if defined(BDD_FLIGHT_RECORDER) /* { */
    (void) fprintf(manager->debug.flight_recorder.log, "%d <- bdd_cofactor(%d, %d)\n",
	    bdd_index_of_external_pointer(h),
	    bdd_index_of_external_pointer(f),
	    bdd_index_of_external_pointer(g));
#endif /* } */

    return (h);
}

/*
 *    _cofactor - recursively perform the cofactoring
 *
 *    ARGH!   The word ``cofactor'' is an exported function in mis,
 *    so we are not allowed to use it here!   ARGH!  These guys don't
 *    believe in package prefixes!  ARGH!
 *
 *    return the result of the reorganization
 */
static bdd_node *
_cofactor(manager, f, g)
bdd_manager *manager;
bdd_node *f;
bdd_node *g;
{
	bdd_safeframe frame;
	bdd_safenode safe_f, safe_g,
		ret,
		f_T, f_E,
		g_T, g_E,
		var, co_T, co_E;
	bdd_variableId fId, gId;

	bdd_safeframe_start(manager, frame);
	bdd_safenode_link(manager, safe_f, f);
	bdd_safenode_link(manager, safe_g, g);
	bdd_safenode_declare(manager, ret);
	bdd_safenode_declare(manager, f_T);
	bdd_safenode_declare(manager, f_E);
	bdd_safenode_declare(manager, g_T);
	bdd_safenode_declare(manager, g_E);
	bdd_safenode_declare(manager, var);
	bdd_safenode_declare(manager, co_T);
	bdd_safenode_declare(manager, co_E);

	BDD_ASSERT(g != BDD_ZERO(manager));

        manager->heap.stats.adhoc_ops.calls++; 

	if (BDD_IS_CONSTANT(manager, f)) {
	    /*
	     *    f is either zero or one
	     */
            manager->heap.stats.adhoc_ops.returns.trivial++; 
	    bdd_safeframe_end(manager);
	    return (f);
	}

	if (g == BDD_ONE(manager)) {
	    /*
	     *    If the thing to cofactor by is the constant one
	     *    then the result is just the function itself
	     */
            manager->heap.stats.adhoc_ops.returns.trivial++; 
	    bdd_safeframe_end(manager);
	    return (f);
	}

	/*
	 *    If there is some possibility that the function may have
	 *    been computed before then look up the function in the cache ...
	 */
	if (bdd_adhoccache_lookup(manager, f, g, /* v */ 0, &ret.node)) {
	    /*
	     *    The answer was already in the cache, so just return it.
	     */
	    manager->heap.stats.adhoc_ops.returns.cached++; 
	    bdd_safeframe_end(manager);
	    return (ret.node);
	}

        /*
         * Dereference the id of the top node, and the then and else branches, 
         * for each of f and g.
         */
	fId = BDD_REGULAR(f)->id;
	gId = BDD_REGULAR(g)->id;
	(void) bdd_get_branches(f, &f_T.node, &f_E.node);
	(void) bdd_get_branches(g, &g_T.node, &g_E.node);

	/*
	 *    In the following code, we must worry about taking a _cofactor
	 *    by the zero function.  So, in each case this is special-cased.
         *    For those cases where we call _cofactor recursively on both branches,
         *    we subsequently call bdd_find_or_add to get the node (var, 1, 0),
         *    where the id of var = (fId > gId) ? gId: fId.
	 */
	if (fId > gId) {
	    if (g_E.node == BDD_ZERO(manager)) {
		/*
		 *    cofactor(f, g_T)
		 */
		ret.node = _cofactor(manager, f, g_T.node);
	    } else if (g_T.node == BDD_ZERO(manager)) {
		/*
		 *    cofactor(f, g_E)
		 */
		ret.node = _cofactor(manager, f, g_E.node);
	    } else {
		/*
		 *    ITE(gid, cofactor(f, g_T), cofactor(f, g_E))
		 */
		co_T.node = _cofactor(manager, f, g_T.node);
		co_E.node = _cofactor(manager, f, g_E.node);
  	        var.node = bdd_find_or_add(manager, gId, BDD_ONE(manager), BDD_ZERO(manager));
	        ret.node = bdd__ITE_(manager, var.node, co_T.node, co_E.node);
	    }	    
	} else if (fId == gId) {
	    if (g_E.node == BDD_ZERO(manager)) {
		/*
		 *    cofactor(f_T, g_T)
		 */
		ret.node = _cofactor(manager, f_T.node, g_T.node);
	    } else if (g_T.node == BDD_ZERO(manager)) {
		/*
		 *    cofactor(f_E, g_E)
		 */
		ret.node = _cofactor(manager, f_E.node, g_E.node);
	    } else {
		/*
		 *    ITE(fId, cofactor(f_T, g_T), cofactor(f_E, g_E))
		 */
		co_T.node = _cofactor(manager, f_T.node, g_T.node);
		co_E.node = _cofactor(manager, f_E.node, g_E.node);
  	        var.node = bdd_find_or_add(manager, fId, BDD_ONE(manager), BDD_ZERO(manager));
	        ret.node = bdd__ITE_(manager, var.node, co_T.node, co_E.node);
	    }
	} else { /* fId < gId */
	    /*
	     *    ITE(fId, cofactor(f_T, g), cofactor(f_E, g))
	     */
	    co_T.node = _cofactor(manager, f_T.node, g);
	    co_E.node = _cofactor(manager, f_E.node, g);
            var.node = bdd_find_or_add(manager, fId, BDD_ONE(manager), BDD_ZERO(manager));
	    ret.node = bdd__ITE_(manager, var.node, co_T.node, co_E.node);
	}

	(void) bdd_adhoccache_insert(manager, f, g, /* v */ 0, ret.node);
        manager->heap.stats.adhoc_ops.returns.full++; 

	bdd_safeframe_end(manager);
	return (ret.node);
}
