
#include <stdio.h>	/* for BDD_DEBUG_LIFESPAN */

#include "util.h"
#include "array.h"
#include "st.h"

#include "bdd.h"
#include "bdd_int.h"



static bdd_node *compose();

/*
 *    bdd_compose - compose g into the v slot of f
 *
 *    return the new bdd (external pointer)
 */
bdd_t *
bdd_compose(f, v, g)
bdd_t *f;
bdd_t *v;		/* must be a var */
bdd_t *g;
{
    bdd_safeframe frame;
    bdd_safenode ret;
    bdd_manager *manager;
    bdd_t *h;

    if (f == NIL(bdd_t) || v == NIL(bdd_t) || g == NIL(bdd_t)) {
	fail ("bdd_compose: invalid BDD");
    }

    BDD_ASSERT( ! f->free );
    BDD_ASSERT( ! v->free );
    BDD_ASSERT( ! g->free );

    if (f->bdd != v->bdd || f->bdd != g->bdd) {
	fail("bdd_compose: different bdd managers");
    }

    /*
     *    If v does not represent a variable of the form (varId, 1, 0)
     */
    if (BDD_IS_COMPLEMENT(v->node)) {
	fail("bdd_compose: second argument not a variable");
    }

    manager = f->bdd;		/* either this or v->bdd or g->bdd will do */

    BDD_ASSERT_REGNODE(v->node);
    if (v->node->T != BDD_ONE(manager) ||
	v->node->E != BDD_ZERO(manager)) {
	fail("bdd_compose: second argument not a variable");
    }

    /*
     *    Start the safe frame now that we know the input is correct.
     *    f, v and g are external pointers so they need not be protected
     */
    bdd_safeframe_start(manager, frame);
    bdd_safenode_declare(manager, ret);

    (void) bdd_adhoccache_init(manager);

    BDD_ASSERT_REGNODE(v->node);
    ret.node = compose(manager, f->node, v->node->id, g->node);

    /*
     *    Free the cache, end the safe frame and return the (safe) result
     */
    (void) bdd_adhoccache_uninit(manager);
    h = bdd_make_external_pointer(manager, ret.node, "bdd_compose");
    bdd_safeframe_end(manager);

#if defined(BDD_FLIGHT_RECORDER) /* { */
    (void) fprintf(manager->debug.flight_recorder.log, "%d <- bdd_compose(%d, %d, %d)\n",
	    bdd_index_of_external_pointer(h),
	    bdd_index_of_external_pointer(f),
	    bdd_index_of_external_pointer(v),
	    bdd_index_of_external_pointer(g));
#endif /* } */

    return (h);
}

static bdd_node *
compose(manager, f, varId, g)
bdd_manager *manager;
bdd_node *f;
bdd_variableId varId;
bdd_node *g;
{
	bdd_safeframe frame;
	bdd_safenode safe_f, safe_g,
		ret,
		f_T, f_E,
		var, co_T, co_E;
	bdd_variableId fId;

        bdd_safeframe_start(manager, frame);
        bdd_safenode_link(manager, safe_f, f);
        bdd_safenode_link(manager, safe_g, g);
        bdd_safenode_declare(manager, ret);
        bdd_safenode_declare(manager, f_T);
        bdd_safenode_declare(manager, f_E);
        bdd_safenode_declare(manager, var);
        bdd_safenode_declare(manager, co_T);
        bdd_safenode_declare(manager, co_E);

        manager->heap.stats.adhoc_ops.calls++; 

	fId = BDD_REGULAR(f)->id;

	if (fId > varId) {
	    /*
	     *    The variable in question, varId, is not related to
	     *    the function f, so just return f directly as the compose 
	     */
            manager->heap.stats.adhoc_ops.returns.trivial++; 
	    bdd_safeframe_end(manager)
	    return (f);
	}

        if (bdd_adhoccache_lookup(manager, f, g, (bdd_int) varId, &ret.node)) {
            /*
             *    The answer was already in the cache, so just return it.
             */
            manager->heap.stats.adhoc_ops.returns.cached++; 
            bdd_safeframe_end(manager);
            return (ret.node);
        }

	(void) bdd_get_branches(f, &f_T.node, &f_E.node);

	if (fId == varId) {
	    /*
	     *    ITE(g, f_T, f_E)
	     *
	     *    This is where the real work gets done
	     */
	    ret.node = bdd__ITE_(manager, g, f_T.node, f_E.node);
	} else {
	    /*
	     *    ITE(f, compose(f_T, var, g), compose(f_E, var, g))
	     */
	    co_T.node = compose(manager, f_T.node, varId, g);
	    co_E.node = compose(manager, f_E.node, varId, g);

	    var.node = bdd_find_or_add(manager, fId, BDD_ONE(manager), BDD_ZERO(manager));

	    ret.node = bdd__ITE_(manager, var.node, co_T.node, co_E.node);
	}

	(void) bdd_adhoccache_insert(manager, f, g, (bdd_int) varId, ret.node);
        manager->heap.stats.adhoc_ops.returns.full++; 

	bdd_safeframe_end(manager);
	return (ret.node);
}
