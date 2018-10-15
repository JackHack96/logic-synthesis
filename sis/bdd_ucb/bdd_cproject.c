#include "util.h"
#include "array.h"
#include "st.h"
#include "bdd.h"
#include "bdd_int.h"

static bdd_node *cproject();

/* 
 * This CPROJ_OFFSET is used to effectively partition the adhoc cache into two pieces: one used for
 * cproject, the other for smooth.  It is assumed that the size of the var_array will never 
 * exceed this value.
 */
#define CPROJ_OFFSET 1000000 


/*
 *    bdd_cproject - the compatible projection function
 *    
 *
 *    Return the new bdd (external pointer) corresponding to the compatible
 *    projection of f onto the variables in var_array.  The "reference vertex"
 *    mentioned in Bill Lin's work is here hardcoded to all 1's.
 */
bdd_t *
bdd_cproject(f, var_array)
bdd_t *f;
array_t *var_array;	/* of bdd_t* */
{
    bdd_manager *manager;
    bdd_safeframe frame;
    bdd_safenode real_f, ret;
    array_t *vars;		/* of int */
    bdd_t *h;

    if (f == NIL(bdd_t)) {
	fail ("bdd_cproject: invalid BDD");
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

    ret.node = cproject(manager, f->node, 0, vars);

    /*
     *    End the safe frame and return the result
     */
    (void) bdd_adhoccache_uninit(manager);
    h = bdd_make_external_pointer(manager, ret.node, "bdd_cproject");
    bdd_safeframe_end(manager);

#if defined(BDD_FLIGHT_RECORDER) /* { */
    fprintf(manager->debug.flight_recorder.log, "%d <- bdd_cproject(%d, { ",
	    bdd_index_of_external_pointer(h),
	    bdd_index_of_external_pointer(f));
    {
	int i;

	for (i=0; i<array_n(var_array); i++) {
	    bdd_t *v;
	    
	    v = array_fetch(bdd_t *, var_array, i);
	    fprintf(manager->debug.flight_recorder.log, "%d%s",
		    bdd_index_of_external_pointer(v),
		    i+1 == array_n(var_array) ? " } )\n": ", ");
	}
    }
#endif /* } */

    return h;
}

/*
 *    cproject - recursively perform compatible projection
 *
 *    return the result of the reorganization
 */
static bdd_node *
cproject(manager, f, index, vars)
bdd_manager *manager;
bdd_node *f;
int index;
array_t *vars;
{
    bdd_safeframe frame;
    bdd_safenode safe_f;
    bdd_safenode f_T, f_E;
    bdd_safenode pr_T, pr_E;
    bdd_safenode sm, pr;
    bdd_safenode ret, var, local;
    bdd_variableId fId, top_varId;

    bdd_safeframe_start(manager, frame);
    bdd_safenode_link(manager, safe_f, f);
    bdd_safenode_declare(manager, ret);
    bdd_safenode_declare(manager, f_T);
    bdd_safenode_declare(manager, f_E);
    bdd_safenode_declare(manager, pr_T);
    bdd_safenode_declare(manager, pr_E);
    bdd_safenode_declare(manager, sm);
    bdd_safenode_declare(manager, pr);
    bdd_safenode_declare(manager, var);
    bdd_safenode_declare(manager, local);

    manager->heap.stats.adhoc_ops.calls++; 

    if (index >= array_n(vars)) { 
	/* no more variables to project */
	manager->heap.stats.adhoc_ops.returns.trivial++; 
	bdd_safeframe_end(manager);
	return f;
    }
    if (f == BDD_ZERO(manager)) { 
	manager->heap.stats.adhoc_ops.returns.trivial++; 
	bdd_safeframe_end(manager); 
	return f; 
    } 

    if (bdd_adhoccache_lookup(manager, f, NIL(bdd_node), index+CPROJ_OFFSET, &ret.node)) {
	/*
	 *    The answer was already in the cache, so just return it.
	 */
	manager->heap.stats.adhoc_ops.returns.cached++; 
	bdd_safeframe_end(manager);
	return (ret.node);
    }

    /*
     * Get the single variable BDD corresponding to the next variable to be processed.
     */
    top_varId = array_fetch(int, vars, index);
    var.node = bdd_find_or_add(manager, top_varId, BDD_ONE(manager), BDD_ZERO(manager));

    (void) bdd_get_branches(f, &f_T.node, &f_E.node);
    fId = BDD_REGULAR(f)->id;

    if (fId > top_varId || f == BDD_ONE(manager)) {
	/* f does not depend on y_i,  then set it to constant */
	pr.node = cproject(manager, f, index + 1, vars); 
	ret.node = bdd__ITE_(manager, var.node, pr.node, BDD_ZERO(manager));
    } else if (fId == top_varId) {
        /* smooth out from f_T all remaining variables in vars, after top_varId */
	sm.node = bdd_internal_quantify(manager, f_T.node, index + 1, vars, BDD_EXISTS); 
	if (sm.node == BDD_ONE(manager)) {
	    pr.node = cproject(manager, f_T.node, index + 1, vars);
	    ret.node = bdd__ITE_(manager, var.node, pr.node, BDD_ZERO(manager));
	} else if (sm.node == BDD_ZERO(manager)) {
	    pr.node = cproject(manager, f_E.node, index + 1, vars);
	    ret.node = bdd__ITE_(manager, var.node, BDD_ZERO(manager), pr.node);
	} else {
	    pr_T.node = cproject(manager, f_T.node, index + 1, vars);
	    pr_E.node = cproject(manager, f_E.node, index + 1, vars);
	    pr.node = bdd__ITE_(manager, sm.node, BDD_ZERO(manager), pr_E.node); 
	    ret.node = bdd__ITE_(manager, var.node, pr_T.node, pr.node); 
	}
    } else { /* fId < top_varId */
	pr_T.node = cproject(manager, f_T.node, index, vars);
	pr_E.node = cproject(manager, f_E.node, index, vars);
	local.node = bdd_find_or_add(manager, fId, BDD_ONE(manager), BDD_ZERO(manager));
	ret.node = bdd__ITE_(manager, local.node, pr_T.node, pr_E.node);
    }

    (void) bdd_adhoccache_insert(manager, f, NIL(bdd_node), index+CPROJ_OFFSET, ret.node);
    manager->heap.stats.adhoc_ops.returns.full++; 

    bdd_safeframe_end(manager);
    return (ret.node);
}

