
#include <stdio.h>	/* for BDD_DEBUG_LIFESPAN */

#include "util.h"
#include "array.h"
#include "st.h"

#include "bdd.h"
#include "bdd_int.h"



static bdd_variableId get_next_variable();
static bdd_node *and_smooth();

/*
 *    bdd_and_smooth - smooth and and at the same time
 *
 *    smoothing_vars: array of bdd_t *'s, input variables to be smoothed out
 *
 *    return the smoothed and anded result
 */
bdd_t *
bdd_and_smooth(f, g, smoothing_vars)
bdd_t *f;
bdd_t *g;
array_t *smoothing_vars;	/* of bdd_t* */
{
    bdd_safeframe frame;
    bdd_safenode real_f, real_g, ret;
    bdd_manager *manager;
    array_t *vars;
    bdd_t *h;

    if (f == NIL(bdd_t) || g == NIL(bdd_t))
	fail("bdd_and_smooth: invalid BDD");

    BDD_ASSERT( ! f->free );
    BDD_ASSERT( ! g->free );

    if (f->bdd != g->bdd)
	fail("bdd_and_smooth: different bdd managers");

    manager = f->bdd;		/* either this or g->bdd will do */

    /*
     *    After the input is checked for correctness, start the safe frame
     *    f and g are already external pointers so they need not be protected
     */
    bdd_safeframe_start(manager, frame);
    bdd_safenode_declare(manager, real_f);
    bdd_safenode_declare(manager, real_g);
    bdd_safenode_declare(manager, ret);

    real_f.node = f->node;
    real_g.node = g->node;

    vars = bdd_get_sorted_varids(smoothing_vars);
    (void) bdd_adhoccache_init(manager);

    ret.node = and_smooth(manager, real_f.node, real_g.node, /* index */ 0, vars);

    /*
     *    Free the cache, end the safe frame and return the (safe) result
     */
    (void) bdd_adhoccache_uninit(manager);
    (void) array_free(vars);
    bdd_safeframe_end(manager);
    h = bdd_make_external_pointer(manager, ret.node, "bdd_and_smooth");

#if defined(BDD_FLIGHT_RECORDER) /* { */
    (void) fprintf(manager->debug.flight_recorder.log, "%d <- bdd_and_smooth(%d, %d, { ",
	    bdd_index_of_external_pointer(h),
	    bdd_index_of_external_pointer(f),
	    bdd_index_of_external_pointer(g));
    {
	int i;

	for (i=0; i<array_n(smoothing_vars); i++) {
	    bdd_t *v;
	    
	    v = array_fetch(bdd_t *, smoothing_vars, i);
	    (void) fprintf(manager->debug.flight_recorder.log, "%d%s",
		    bdd_index_of_external_pointer(v),
		    i+1 == array_n(smoothing_vars) ? " } )\n": ", ");
	}
    }
#endif /* } */

    return (h);
}

/*
 *    and_smooth - and and smooth at the same time
 *
 *    Can't share with the other smooth because of caching!!!
 *    Should we put the index in the cache?
 *
 *    return the anding and the smoothing ...
 */
static bdd_node *
and_smooth(manager, f, g, index, vars)
bdd_manager *manager;
bdd_node *f;
bdd_node *g;
int index;
array_t *vars;
{
    bdd_safeframe frame;
    bdd_safenode safe_f, safe_g;
    bdd_safenode f_T, f_E, g_T, g_E;
    bdd_safenode T, E;
    bdd_safenode h;
    bdd_safenode ret, var;
    bdd_variableId fId, gId, varId;
    int next_index;

    bdd_safeframe_start(manager, frame);
    bdd_safenode_link(manager, safe_f, f);
    bdd_safenode_link(manager, safe_g, g);
    bdd_safenode_declare(manager, ret);
    bdd_safenode_declare(manager, var);
    bdd_safenode_declare(manager, f_T);
    bdd_safenode_declare(manager, f_E);
    bdd_safenode_declare(manager, g_T);
    bdd_safenode_declare(manager, g_E);
    bdd_safenode_declare(manager, T);
    bdd_safenode_declare(manager, E);
    bdd_safenode_declare(manager, h);
  
    manager->heap.stats.adhoc_ops.calls++; 

    if (f == BDD_ZERO(manager) || g == BDD_ZERO(manager)) {
	manager->heap.stats.adhoc_ops.returns.trivial++; 
	bdd_safeframe_end(manager);
	return BDD_ZERO(manager);
    } 
    if (f == BDD_ONE(manager) && g == BDD_ONE(manager)) {
	manager->heap.stats.adhoc_ops.returns.trivial++; 
	bdd_safeframe_end(manager);
	return BDD_ONE(manager);
    }
    if (bdd_adhoccache_lookup(manager, f, g, /* unused */ 0, &ret.node)) {
	manager->heap.stats.adhoc_ops.returns.cached++; 
	bdd_safeframe_end(manager);
	return (ret.node);
    }

    fId = BDD_REGULAR(f)->id;
    gId = BDD_REGULAR(g)->id;

    /*
     *    if f or g constant, only one is a constant, and that constant is ONE
     *    in that case, the var ID should put the constant infinitely far and
     *    everything should work fine.
     */
    (void) bdd_get_branches(f, &f_T.node, &f_E.node);
    (void) bdd_get_branches(g, &g_T.node, &g_E.node);

    /*
     *    skip over uninteresting variables
     */
    varId = get_next_variable(MIN(fId,gId), index, vars, &next_index);
    if (fId > gId) {
	if (varId == gId) {
	    /*
	     *    should smooth
	     *    not clear to me whether faster to smooth g first,
	     *    and then recur or the other way around
	     */
	    h.node = bdd__ITE_(manager, g_T.node, BDD_ONE(manager), g_E.node);
	    ret.node = and_smooth(manager, f, h.node, next_index, vars);
	} else {
	    /*
	     *    should not smooth
	     */
	    T.node = and_smooth(manager, f, g_T.node, next_index, vars);
	    E.node = and_smooth(manager, f, g_E.node, next_index, vars);
	    var.node = bdd_find_or_add(manager, gId, BDD_ONE(manager), BDD_ZERO(manager));
	    ret.node = bdd__ITE_(manager, var.node, T.node, E.node);
	}
    } else if (fId == gId) {
	T.node = and_smooth(manager, f_T.node, g_T.node, next_index, vars);
	E.node = and_smooth(manager, f_E.node, g_E.node, next_index, vars);
	if (varId == fId) {
	    /*
	     *    should smooth
	     */
	    ret.node = bdd__ITE_(manager, T.node, BDD_ONE(manager), E.node);
	} else {
	    /*
	     *    should not smooth
	     */
	    var.node = bdd_find_or_add(manager, fId, BDD_ONE(manager), BDD_ZERO(manager));
	    ret.node = bdd__ITE_(manager, var.node, T.node, E.node);
	}
    } else {			/* fId < gId */
	if (varId == fId) {
	    /*
	     *    should smooth
	     *    should we or f first, or recur first?
	     */
	    T.node = and_smooth(manager, f_T.node, g, next_index, vars);
	    E.node = and_smooth(manager, f_E.node, g, next_index, vars);
	    ret.node = bdd__ITE_(manager, T.node, BDD_ONE(manager), E.node);
	} else {
	    /*
	     *    should not smooth
	     */
	    T.node = and_smooth(manager, f_T.node, g, next_index, vars);
	    E.node = and_smooth(manager, f_E.node, g, next_index, vars);
	    var.node = bdd_find_or_add(manager, fId, BDD_ONE(manager), BDD_ZERO(manager));
	    ret.node = bdd__ITE_(manager, var.node, T.node, E.node);
	}
    }
    (void) bdd_adhoccache_insert(manager, f, g, /* unused */ 0, ret.node);
    manager->heap.stats.adhoc_ops.returns.full++; 
  
    bdd_safeframe_end(manager);
    return ret.node;
}

/*
 *    get_next_variable - get the next variable
 *
 *    The ID of BDD_ONE if the bound is exceeded
 *    otherwise the first variable in the array that is
 *    no smaller than top_var as side effect, increment
 *    index to next position.
 *
 *    return the variableId
 */
static bdd_variableId
get_next_variable(top_var, index, smoothing_vars, next_index)
bdd_variableId top_var;
int index;
array_t *smoothing_vars;
int *next_index;
{
    int i;
    bdd_variableId value;

    for (i = index; i < array_n(smoothing_vars); i++) {
	value = array_fetch(bdd_variableId, smoothing_vars, i);
	if (value >= top_var) break;
    }
    if (i >= array_n(smoothing_vars)) {
	*next_index = array_n(smoothing_vars);
	return BDD_ONE_ID;
    } else if (value == top_var) {
	*next_index = i + 1;
	return value;
    } else {
	*next_index = i;
	return value;
    }
}
