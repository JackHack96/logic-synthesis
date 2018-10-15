#include <stdio.h>	/* for BDD_DEBUG_LIFESPAN */

#include "util.h"
#include "array.h"
#include "st.h"

#include "bdd.h"
#include "bdd_int.h"

static bdd_t *bdd_quantify();

/*
 *    bdd_smooth - the smoothing function
 */
bdd_t *
bdd_smooth(f, var_array)
bdd_t *f;
array_t *var_array;	/* of bdd_t* */
{
    return bdd_quantify(f, var_array, BDD_EXISTS);
}

/*
 *    bdd_consensus - the consensus function
 */
bdd_t *
bdd_consensus(f, var_array)
bdd_t *f;
array_t *var_array;	/* of bdd_t* */
{
    return bdd_quantify(f, var_array, BDD_FORALL);
}

/*
 *    bdd_quantify - the quantifying function
 *
 *    return the new bdd (external pointer)
 */
static bdd_t *
bdd_quantify(f, var_array, quant_type)
bdd_t *f;
array_t *var_array;	/* of bdd_t* */
bdd_quantify_type_t quant_type;
{
    bdd_manager *manager;
    bdd_safeframe frame;
    bdd_safenode real_f, ret;
    array_t *vars;
    bdd_t *h;

    if (f == NIL(bdd_t)) {
	fail ("bdd_quantify: invalid BDD");
    }

    BDD_ASSERT( ! f->free );

    manager = f->bdd;

    /*
     *    After the input is checked for correctness, start the safe frame
     *    f and g are already external pointers so they need not be protected
     */
    bdd_safeframe_start(manager, frame);
    bdd_safenode_declare(manager, real_f);
    bdd_safenode_declare(manager, ret);

    real_f.node = f->node;

    vars = bdd_get_sorted_varids(var_array);
    (void) bdd_adhoccache_init(manager);

    ret.node = bdd_internal_quantify(manager, f->node, 0, vars, quant_type);

    /*
     *    End the safe frame and return the result
     */
    (void) bdd_adhoccache_uninit(manager);
    (void) array_free(vars);
    bdd_safeframe_end(manager);
    h = bdd_make_external_pointer(manager, ret.node, "bdd_quantify");

#if defined(BDD_FLIGHT_RECORDER) /* { */
    (void) fprintf(manager->debug.flight_recorder.log, "%d <- bdd_%s(%d, { ",
	    bdd_index_of_external_pointer(h),
	    ((quant_type == BDD_EXISTS) ? "smooth" : "consensus"), 
	    bdd_index_of_external_pointer(f));
    {
	int i;

	for (i=0; i<array_n(var_array); i++) {
	    bdd_t *v;
	    
	    v = array_fetch(bdd_t *, var_array, i);
	    (void) fprintf(manager->debug.flight_recorder.log, "%d%s",
		    bdd_index_of_external_pointer(v),
		    i+1 == array_n(var_array) ? " } )\n": ", ");
	}
    }
#endif /* } */

    return h;
}

/*
 *    bdd_internal_quantify - recursively perform the quantifying
 *    This function name is prefaced by "bdd_internal_" because other routines in the
 *    BDD package call this function.
 *
 *    return the result of the reorganization
 */
bdd_node *
bdd_internal_quantify(manager, f, index, vars, quant_type)
bdd_manager *manager;
bdd_node *f;
int index;
array_t *vars;
bdd_quantify_type_t quant_type;
{
    bdd_safeframe frame;
    bdd_safenode safe_f;
    bdd_safenode f_T, f_E;
    bdd_safenode qnt_T, qnt_E;
    bdd_safenode ret, var;
    bdd_variableId fId, top_varId, last_varId;

    bdd_safeframe_start(manager, frame);
    bdd_safenode_link(manager, safe_f, f);
    bdd_safenode_declare(manager, ret);
    bdd_safenode_declare(manager, f_T);
    bdd_safenode_declare(manager, f_E);
    bdd_safenode_declare(manager, qnt_T);
    bdd_safenode_declare(manager, qnt_E);
    bdd_safenode_declare(manager, var);

    manager->heap.stats.adhoc_ops.calls++; 

    if (index >= array_n(vars)) {
	/*
	 *    no more variables to quantify
	 */
	manager->heap.stats.adhoc_ops.returns.trivial++; 
	bdd_safeframe_end(manager);
	return f;
    }

    fId = BDD_REGULAR(f)->id;
    last_varId = array_fetch(int, vars, array_n(vars) - 1);

    if (fId > last_varId) {
	/* 
	 * If f's id above the last possible quantifying variable, return f.
	 * Also takes care of the case f == CONSTANT.
	 */
	manager->heap.stats.adhoc_ops.returns.trivial++; 
	bdd_safeframe_end(manager);
	return f;
    }

    if (bdd_adhoccache_lookup(manager, f, NIL(bdd_node), index, &ret.node)) {
	/*
	 *    The answer was already in the cache, so just return it.
	 */
	manager->heap.stats.adhoc_ops.returns.cached++; 
	bdd_safeframe_end(manager);
	return (ret.node);
    }

    (void) bdd_get_branches(f, &f_T.node, &f_E.node);

    /* from now on, everything has to be protected */
    top_varId = array_fetch(int, vars, index);
    if (fId > top_varId) {
	ret.node = bdd_internal_quantify(manager, f, index + 1, vars, quant_type);
    } else if (fId == top_varId) {
	/*
	 * If EXISTS:  quantify(f_T, vars + 1) OR  quantify(f_E, vars + 1) 
	 * If FORALL:  quantify(f_T, vars + 1) AND quantify(f_E, vars + 1) 
	 */
	qnt_T.node = bdd_internal_quantify(manager, f_T.node, index + 1, vars, quant_type);
	qnt_E.node = bdd_internal_quantify(manager, f_E.node, index + 1, vars, quant_type);
	if (quant_type == BDD_EXISTS) {
	    ret.node = bdd__ITE_(manager, qnt_T.node, BDD_ONE(manager), qnt_E.node);
	} else if (quant_type == BDD_FORALL) {
	    ret.node = bdd__ITE_(manager, qnt_T.node, qnt_E.node, BDD_ZERO(manager));
	} else {
	    fail("bdd_internal_smooth: unknown quantify method");
	}
    } else {			/* fId < top_varId */
	/*
	 *    ITE(fId, quantify(f_T, vars), quantify(f_E, vars))
	 */
	qnt_T.node = bdd_internal_quantify(manager, f_T.node, index, vars, quant_type);
	qnt_E.node = bdd_internal_quantify(manager, f_E.node, index, vars, quant_type);
	var.node = bdd_find_or_add(manager, fId, BDD_ONE(manager), BDD_ZERO(manager));
	ret.node = bdd__ITE_(manager, var.node, qnt_T.node, qnt_E.node);
    }

    (void) bdd_adhoccache_insert(manager, f, NIL(bdd_node), index, ret.node);
    manager->heap.stats.adhoc_ops.returns.full++; 
    bdd_safeframe_end(manager);
    return (ret.node);
}
