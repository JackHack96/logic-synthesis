
#include <stdio.h>	/* for BDD_DEBUG_LIFESPAN */

#include "util.h"
#include "array.h"
#include "st.h"

#include "bdd.h"
#include "bdd_int.h"



static bdd_node *substitute();
static int cmp();			/* like strcmp, but for integers */

/*
 *    bdd_substitute - substitute all old_array vars into with new_array vars
 *
 *    given two arrays of variables a and b consisting of member values
 *    (a1 .. an) and (b1 .. bn), replace all occurrences of ai by bi.
 *
 *    This could be done iteratively with bdd_compose but would require n
 *    passes instead of one.   Thus this algorithm is only a performance optimization.
 *
 *    first extract the var_id's and sort them then call the recursive routine
 *
 *    return the new bdd (external pointer)
 */
bdd_t *
bdd_substitute(f, old_array, new_array)
bdd_t *f;
array_t *old_array;	/* of bdd_t */
array_t *new_array;	/* of bdd_t */
{
    bdd_safeframe frame;
    bdd_safenode ret;
    bdd_manager *manager;
    bdd_t *e;
    int i, old_var, new_var;
    array_t *old_vars;
    array_t *new_vars;
    st_table *map;

    if (f == NIL(bdd_t))
	fail("bdd_substitute: invalid BDD");

    BDD_ASSERT( ! f->free );

    manager = f->bdd;

    /*
     *    Start the safe frame now that we know the input is correct.
     *    f and g are external pointers so they need not be protected
     */
    bdd_safeframe_start(manager, frame);
    bdd_safenode_declare(manager, ret);

    /* sort the old_vars / keep the correspondance with new_vars */
    BDD_ASSERT(array_n(old_array) == array_n(new_array));

    map = st_init_table(st_ptrcmp, st_ptrhash);

    old_vars = bdd_get_varids(old_array);
    new_vars = bdd_get_varids(new_array);

    for (i=0; i<array_n(old_vars); i++) {
	old_var = array_fetch(int, old_vars, i);
	new_var = array_fetch(int, new_vars, i);
	st_insert(map, (refany) old_var, (refany) new_var);
    }

    (void) array_free(new_vars);
    (void) array_sort(old_vars, cmp);

    new_vars = array_alloc(int, (unsigned int) array_n(old_vars));
    for (i=0; i<array_n(old_vars); i++) {
	old_var = array_fetch(int, old_vars, i);
	st_lookup_int(map, (refany) old_var, &new_var);
	(void) array_insert(int, new_vars, i, new_var);
    }
    (void) st_free_table(map);

    (void) bdd_adhoccache_init(manager);

    ret.node = substitute(manager, f->node, 0, old_vars, new_vars);

    /*
     *    Free the cache, end the safe frame and return the (safe) result
     */
    (void) bdd_adhoccache_uninit(manager);
    e = bdd_make_external_pointer(manager, ret.node, "bdd_substitute");
    bdd_safeframe_end(manager);
    (void) array_free(new_vars);
    (void) array_free(old_vars);

#if defined(BDD_FLIGHT_RECORDER) /* { */
    (void) fprintf(manager->debug.flight_recorder.log, "%d <- bdd_substitute(%d, { ",
	    bdd_index_of_external_pointer(e),
	    bdd_index_of_external_pointer(f));
    {
	int i;

	for (i=0; i<array_n(old_array); i++) {
	    bdd_t *v;
	    
	    v = array_fetch(bdd_t *, old_array, i);
	    (void) fprintf(manager->debug.flight_recorder.log, "%d%s",
		    bdd_index_of_external_pointer(v),
		    i+1 == array_n(old_array) ? " }, ": ", ");
	}
    }
    {
	int i;

	for (i=0; i<array_n(new_array); i++) {
	    bdd_t *v;
	    
	    v = array_fetch(bdd_t *, new_array, i);
	    (void) fprintf(manager->debug.flight_recorder.log, "%d%s",
		    bdd_index_of_external_pointer(v),
		    i+1 == array_n(new_array) ? " } )\n": ", ");
	}
    }
#endif /* } */

    return (e);
}

/*
 *    substitute - recursively perform the substitution
 *
 *    cache f and the index
 *    returns the node subsitute(f, old_index, old_vars, new_vars)
 *
 *    return the bdd_node*
 */
static bdd_node *
substitute(manager, f, old_index, old_vars, new_vars)
bdd_manager *manager;
bdd_node *f;
int old_index;
array_t *old_vars;
array_t *new_vars;
{
    bdd_safeframe frame;
    bdd_safenode safe_f, ret, g, f_T, f_E, new_var, su_T, su_E;
    bdd_variableId fId, top_varId, last_varId, new_varId;
    bdd_int val;
    int next_index;

    bdd_safeframe_start(manager, frame);
    bdd_safenode_link(manager, safe_f, f);
    bdd_safenode_declare(manager, ret);
    bdd_safenode_declare(manager, g);
    bdd_safenode_declare(manager, f_T);
    bdd_safenode_declare(manager, f_E);
    bdd_safenode_declare(manager, new_var);
    bdd_safenode_declare(manager, su_T);
    bdd_safenode_declare(manager, su_E);

    manager->heap.stats.adhoc_ops.calls++; 

    if (old_index >= array_n(old_vars)) {
	/*
	 *    If the old index is beyond the substitution
	 *    range, then return f it trivially.
	 */    
	manager->heap.stats.adhoc_ops.returns.trivial++; 
	bdd_safeframe_end(manager);
	return (f);
    }

    fId = BDD_REGULAR(f)->id;

    last_varId = array_fetch(int, old_vars, array_n(old_vars) - 1);
    if (fId > last_varId) {
	/*
	 *    If f's id is above the last possible substitution
	 *    then just return f trivially. Takes care of f == CONSTANT
	 */
	manager->heap.stats.adhoc_ops.returns.trivial++; 
	bdd_safeframe_end(manager);
	return (f);
    }

    /*
     *    key = {f, nil, old_index}
     */
    g.node = NIL(bdd_node);
    val = old_index;

    if (bdd_adhoccache_lookup(manager, f, g.node, val, &ret.node)) {
	/*
	 *    The answer was already in the cache, so just return it.
	 */
	manager->heap.stats.adhoc_ops.returns.cached++; 
	bdd_safeframe_end(manager);
	return (ret.node);
    }

    /* from now on, everything has to be protected */
    top_varId = array_fetch(int, old_vars, old_index);
    if (fId > top_varId) {
	ret.node = substitute(manager, f, old_index + 1, old_vars, new_vars);
    } else {
	if (fId < top_varId) {
	    next_index = old_index;
	    new_varId = fId;
	} else {		/* fId == top_varId */
	    next_index = old_index + 1;
	    new_varId = array_fetch(int, new_vars, old_index);
	}
	(void) bdd_get_branches(f, &f_T.node, &f_E.node);

	su_T.node = substitute(manager, f_T.node, next_index, old_vars, new_vars);
	su_E.node = substitute(manager, f_E.node, next_index, old_vars, new_vars);

	/*
	 *    varId is the value (v, 1, 0)
	 */
	new_var.node = bdd_find_or_add(manager, new_varId, BDD_ONE(manager), BDD_ZERO(manager));
	ret.node = bdd__ITE_(manager, new_var.node, su_T.node, su_E.node);
    }

    (void) bdd_adhoccache_insert(manager, f, g.node, val, ret.node);
    manager->heap.stats.adhoc_ops.returns.full++; 

    bdd_safeframe_end(manager);
    return (ret.node);
}

/*
 *    cmp - like strcmp, but for integers
 *
 *    return {-1, 0, 1} on {<, =, >}
 */
static int
cmp(val1, val2)
int *val1;	/* by reference */
int *val2;	/* by reference */
{
    return (*val1 - *val2);
}
