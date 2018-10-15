/*
 * Revision Control Information
 *
 * /projects/hsis/CVS/utilities/bdd_cmu/bdd_port/bddport.c,v
 * serdar
 * 1.7
 * 1994/04/28 19:28:08
 *
 */
/*
 * UCB BDD package to CMU BDD package interface. 
 *
 * Translate each exported UCB BDD function described in bdd.doc into an
 * equivalent CMU BDD package call.
 * 
 * Author: Thomas R. Shiple
 * Date: 7 June 93
 *
 */

#include "util.h"     /* includes math.h */
#include "array.h"
#include "st.h"

#include "bdd.h"      /* UCB interface to CMU */
#include "memuser.h"
#include "bdduser.h"  /* CMU exported routines */
#include "bddint.h"   /* CMU internal routines; for use in bdd_get_node() */

/*
 * Function to construct a bdd_t.
 */
static bdd_t *
bdd_construct_bdd_t(mgr, fn)
struct bdd_manager_ *mgr;
struct bdd_ *fn;
{
    bdd_t *result;

    if (fn == (struct bdd_ *) 0) {
	cmu_bdd_fatal("bdd_construct_bdd_t: possible memory overflow");
    }

    result = ALLOC(bdd_t, 1);
    result->mgr = mgr;
    result->node = fn;
    result->free = FALSE;
    return result;
}

/*
BDD Manager Allocation And Destruction ----------------------------------------
*/
void
bdd_end(mgr)
struct bdd_manager_ *mgr;
{
    bdd_external_hooks *hooks;

    hooks = (bdd_external_hooks *) mgr->hooks;
    FREE(hooks); 
    cmu_bdd_quit(mgr);
}

void
bdd_set_mgr_init_dflts(mgr_init)
bdd_mgr_init *mgr_init;
{
    cmu_bdd_warning("bdd_set_mgr_init_dflts: translated to no-op in CMU package");
    return;
}

struct bdd_manager_ *
bdd_start(nvariables)
int nvariables; 
{
    struct bdd_manager_ *mgr;
    int i;
    bdd_external_hooks *hooks;
   
    mgr = cmu_bdd_init();    /*no args*/

    /*
     * Calls to UCB bdd_get_variable are translated into cmu_bdd_var_with_id calls.  However,
     * cmu_bdd_var_with_id assumes that single variable BDDs have already been created for 
     * all the variables that we wish to access.  Thus, following, we explicitly create n
     * variables.  We do not care about the return value of cmu_bdd_new_var_last; in the 
     * CMU package, the single variable BDDs are NEVER garbage collected.
     */
    for (i = 0; i < nvariables; i++) {
	(void) cmu_bdd_new_var_last(mgr);
    }

    hooks = ALLOC(bdd_external_hooks, 1);
    hooks->mdd = hooks->network = hooks->undef1 = (char *) 0;
    mgr->hooks = (char *) hooks;  /* new field added to CMU manager */

    return mgr;
}

struct bdd_manager_ *
bdd_start_with_params(nvariables, mgr_init)
int nvariables; 
bdd_mgr_init *mgr_init;
{
    cmu_bdd_warning("bdd_start_with_params: translated to bdd_start(nvariables) in CMU package");
    return bdd_start(nvariables);
}

/*
BDD Variable Allocation -------------------------------------------------------
*/

bdd_t *
bdd_create_variable(mgr)
struct bdd_manager_ *mgr;
{
    return bdd_construct_bdd_t(mgr, cmu_bdd_new_var_last(mgr));
}

bdd_t *
bdd_create_variable_after(mgr, after_id)
struct bdd_manager_ *mgr;
bdd_variableId after_id;
{
    struct bdd_ *after_var;
    bdd_t 	*result;

    after_var = cmu_bdd_var_with_id(mgr, (long)after_id + 1);

    result =  bdd_construct_bdd_t(mgr, cmu_bdd_new_var_after(mgr, after_var));

    /* No need to free after_var, since single variable BDDs are never garbage collected */

    return result;
}



bdd_t *
bdd_get_variable(mgr, variable_ID)
struct bdd_manager_ *mgr;
bdd_variableId variable_ID;	       /* unsigned int */
{
    struct bdd_ *fn;

    fn = cmu_bdd_var_with_id(mgr, (long) (variable_ID + 1));

    if (fn == (struct bdd_ *) 0) {
	/* variable should always be found, since they are created at bdd_start */
	cmu_bdd_fatal("bdd_get_variable: assumption violated");
    }

    return bdd_construct_bdd_t(mgr, fn);
}

/*
BDD Formula Management --------------------------------------------------------
*/

bdd_t *
bdd_dup(f)
bdd_t *f;
{
    return bdd_construct_bdd_t(f->mgr, cmu_bdd_identity(f->mgr, f->node));
}

void
bdd_free(f)
bdd_t *f;
{
    if (f == NIL(bdd_t)) {
	fail("bdd_free: trying to free a NIL bdd_t");			
    }

    if (f->free == TRUE) {
	fail("bdd_free: trying to free a freed bdd_t");			
    }	

    cmu_bdd_free(f->mgr, f->node);

    /*
     * In case the user tries to free this bdd_t again, set the free field to TRUE, 
     * and NIL out the other fields.  Then free the bdd_t structure itself.
     */
    f->free = TRUE;
    f->node = NIL(struct bdd_);
    f->mgr = NIL(struct bdd_manager_);
    FREE(f);  
}

/*
Operations on BDD Formulas ----------------------------------------------------
*/

bdd_t *
bdd_and(f, g, f_phase, g_phase)
bdd_t *f;
bdd_t *g;
boolean f_phase;
boolean g_phase;
{
    struct bdd_ *temp1, *temp2;
    bdd_t *result;
    struct bdd_manager_ *mgr;

    mgr = f->mgr;
    temp1 = ( (f_phase == TRUE) ? cmu_bdd_identity(mgr, f->node) : cmu_bdd_not(mgr, f->node));
    temp2 = ( (g_phase == TRUE) ? cmu_bdd_identity(mgr, g->node) : cmu_bdd_not(mgr, g->node));
    result = bdd_construct_bdd_t(mgr, cmu_bdd_and(mgr, temp1, temp2));
    cmu_bdd_free(mgr, temp1);
    cmu_bdd_free(mgr, temp2);
    return result;
}

bdd_t *
bdd_and_smooth(f, g, smoothing_vars)
bdd_t *f;
bdd_t *g;
array_t *smoothing_vars;	/* of bdd_t *'s */
{
    int num_vars, i;
    bdd_t *fn, *result;
    struct bdd_ **assoc;
    struct bdd_manager_ *mgr;

    num_vars = array_n(smoothing_vars);
    if (num_vars <= 0) {
	cmu_bdd_fatal("bdd_and_smooth: no smoothing variables");
    }

    assoc = ALLOC(struct bdd_ *, num_vars+1);

    for (i = 0; i < num_vars; i++) {
	fn = array_fetch(bdd_t *, smoothing_vars, i);
	assoc[i] = fn->node;
    }
    assoc[num_vars] = (struct bdd_ *) 0;

    mgr = f->mgr;
    cmu_bdd_temp_assoc(mgr, assoc, 0);
    (void) cmu_bdd_assoc(mgr, -1);  /* set the temp association as the current association */

    result = bdd_construct_bdd_t(mgr, cmu_bdd_rel_prod(mgr, f->node, g->node));
    FREE(assoc);
    return result;
}

bdd_t *
bdd_between(f_min, f_max)
bdd_t *f_min;
bdd_t *f_max;
{
    bdd_t *temp, *ret;
    
    temp = bdd_or(f_min, f_max, 1, 0);
    ret = bdd_minimize(f_min, temp);
    bdd_free(temp);
    return ret;
}

bdd_t *
bdd_cofactor(f, g)
bdd_t *f;
bdd_t *g;
{
    return bdd_construct_bdd_t(f->mgr, cmu_bdd_cofactor(f->mgr, f->node, g->node));
}

bdd_t *
bdd_compose(f, v, g)
bdd_t *f;
bdd_t *v;
bdd_t *g;
{
    return bdd_construct_bdd_t(f->mgr, cmu_bdd_compose(f->mgr, f->node, v->node, g->node));
}

bdd_t *
bdd_consensus(f, quantifying_vars)
bdd_t *f;
array_t *quantifying_vars;	/* of bdd_t *'s */
{
    int num_vars, i;
    bdd_t *fn, *result;
    struct bdd_ **assoc;
    struct bdd_manager_ *mgr;

    num_vars = array_n(quantifying_vars);
    if (num_vars <= 0) {
	cmu_bdd_fatal("bdd_consensus: no quantifying variables");
    }

    assoc = ALLOC(struct bdd_ *, num_vars+1);

    for (i = 0; i < num_vars; i++) {
	fn = array_fetch(bdd_t *, quantifying_vars, i);
	assoc[i] = fn->node;
    }
    assoc[num_vars] = (struct bdd_ *) 0;

    mgr = f->mgr;
    cmu_bdd_temp_assoc(mgr, assoc, 0);
    (void) cmu_bdd_assoc(mgr, -1);  /* set the temp association as the current association */

    result = bdd_construct_bdd_t(mgr, cmu_bdd_forall(mgr, f->node));
    FREE(assoc);
    return result;
}

extern bdd  cmu_bdd_project();

/*
 *    bdd_cproject - the compatible projection function
 *
 *    return the new bdd (external pointer)
 */


bdd_t *
bdd_cproject(f, quantifying_vars)
bdd_t *f;
array_t *quantifying_vars;	/* of bdd_t* */
{
    int num_vars, i;
    bdd_t *fn, *result;
    struct bdd_ **assoc;
    struct bdd_manager_ *mgr;



    if (f == NIL(bdd_t))
	fail ("bdd_cproject: invalid BDD");

    num_vars = array_n(quantifying_vars);
    if (num_vars <= 0) {
        cmu_bdd_fatal("bdd_cproject: no projection variables");
    }

    assoc = ALLOC(struct bdd_ *, num_vars+1);

    for (i = 0; i < num_vars; i++) {
        fn = array_fetch(bdd_t *, quantifying_vars, i);
        assoc[i] = fn->node;
    }
    assoc[num_vars] = (struct bdd_ *) 0;

    mgr = f->mgr;
    cmu_bdd_temp_assoc(mgr, assoc, 0);
    (void) cmu_bdd_assoc(mgr, -1);  /* set the temp association as the current a
                                       ssociation */

    result = bdd_construct_bdd_t(mgr, cmu_bdd_project(mgr, f->node));
    FREE(assoc);
    return result;
    
}


bdd_t *
bdd_else(f)
bdd_t *f;
{
    return bdd_construct_bdd_t(f->mgr, cmu_bdd_else(f->mgr, f->node));
}


bdd_t *
bdd_ite(i, t, e, i_phase, t_phase, e_phase)
bdd_t *i;
bdd_t *t;
bdd_t *e;
boolean i_phase;
boolean t_phase;
boolean e_phase;
{
    struct bdd_ *temp1, *temp2, *temp3;
    bdd_t *result;
    struct bdd_manager_ *mgr;

    mgr = i->mgr;
    temp1 = ( (i_phase == TRUE) ? cmu_bdd_identity(mgr, i->node) : cmu_bdd_not(mgr, i->node));
    temp2 = ( (t_phase == TRUE) ? cmu_bdd_identity(mgr, t->node) : cmu_bdd_not(mgr, t->node));
    temp3 = ( (e_phase == TRUE) ? cmu_bdd_identity(mgr, e->node) : cmu_bdd_not(mgr, e->node));
    result = bdd_construct_bdd_t(mgr, cmu_bdd_ite(mgr, temp1, temp2, temp3));
    cmu_bdd_free(mgr, temp1);
    cmu_bdd_free(mgr, temp2);
    cmu_bdd_free(mgr, temp3);
    return result;
}

bdd_t *
bdd_minimize(f, c)
bdd_t *f;
bdd_t *c;
{
    return bdd_construct_bdd_t(f->mgr, cmu_bdd_reduce(f->mgr, f->node, c->node));
}

bdd_t *
bdd_minimize_with_params(f, c, match_type, compl, no_new_vars, return_min)
bdd_t *f;
bdd_t *c;
bdd_min_match_type_t match_type;
boolean compl;
boolean no_new_vars;
boolean return_min;
{
    return bdd_minimize(f, c);
}

bdd_t *
bdd_not(f)
bdd_t *f;
{
    return bdd_construct_bdd_t(f->mgr, cmu_bdd_not(f->mgr, f->node));
}

bdd_t *
bdd_one(mgr)
struct bdd_manager_ *mgr;
{
    return bdd_construct_bdd_t(mgr, cmu_bdd_one(mgr));
}

bdd_t *
bdd_or(f, g, f_phase, g_phase)
bdd_t *f;
bdd_t *g;
boolean f_phase;
boolean g_phase;
{
    struct bdd_ *temp1, *temp2;
    bdd_t *result;
    struct bdd_manager_ *mgr;

    mgr = f->mgr;
    temp1 = ( (f_phase == TRUE) ? cmu_bdd_identity(mgr, f->node) : cmu_bdd_not(mgr, f->node));
    temp2 = ( (g_phase == TRUE) ? cmu_bdd_identity(mgr, g->node) : cmu_bdd_not(mgr, g->node));
    result = bdd_construct_bdd_t(mgr, cmu_bdd_or(mgr, temp1, temp2));
    cmu_bdd_free(mgr, temp1);
    cmu_bdd_free(mgr, temp2);
    return result;
}

bdd_t *
bdd_smooth(f, smoothing_vars)
bdd_t *f;
array_t *smoothing_vars;	/* of bdd_t *'s */
{
    int num_vars, i;
    bdd_t *fn, *result;
    struct bdd_ **assoc;
    struct bdd_manager_ *mgr;

    num_vars = array_n(smoothing_vars);
    if (num_vars <= 0) {
	cmu_bdd_fatal("bdd_smooth: no smoothing variables");
    }

    assoc = ALLOC(struct bdd_ *, num_vars+1);

    for (i = 0; i < num_vars; i++) {
	fn = array_fetch(bdd_t *, smoothing_vars, i);
	assoc[i] = fn->node;
    }
    assoc[num_vars] = (struct bdd_ *) 0;

    mgr = f->mgr;
    cmu_bdd_temp_assoc(mgr, assoc, 0);
    (void) cmu_bdd_assoc(mgr, -1);  /* set the temp association as the current association */

    result = bdd_construct_bdd_t(mgr, cmu_bdd_exists(mgr, f->node));
    FREE(assoc);
    return result;
}

bdd_t *
bdd_substitute(f, old_array, new_array)
bdd_t *f;
array_t *old_array;	/* of bdd_t *'s */
array_t *new_array;	/* of bdd_t *'s */
{
    int num_old_vars, num_new_vars, i;
    bdd_t *fn_old, *fn_new, *result;
    struct bdd_ **assoc;
    struct bdd_manager_ *mgr;

    num_old_vars = array_n(old_array);
    num_new_vars = array_n(new_array);
    if (num_old_vars != num_new_vars) {
	cmu_bdd_fatal("bdd_substitute: mismatch of number of new and old variables");
    }

    assoc = ALLOC(struct bdd_ *, 2*(num_old_vars+1));

    for (i = 0; i < num_old_vars; i++) {
	fn_old = array_fetch(bdd_t *, old_array, i);
	fn_new = array_fetch(bdd_t *, new_array, i);
	assoc[2*i]   = fn_old->node;
	assoc[2*i+1] = fn_new->node;
    }
    assoc[2*num_old_vars]   = (struct bdd_ *) 0;
    assoc[2*num_old_vars+1] = (struct bdd_ *) 0;  /* not sure if we need this second 0 */

    mgr = f->mgr;
    cmu_bdd_temp_assoc(mgr, assoc, 1);
    (void) cmu_bdd_assoc(mgr, -1);  /* set the temp association as the current association */

    result = bdd_construct_bdd_t(mgr, cmu_bdd_substitute(mgr, f->node));
    FREE(assoc);
    return result;
}

bdd_t *
bdd_then(f)
bdd_t *f;
{
    return bdd_construct_bdd_t(f->mgr, cmu_bdd_then(f->mgr, f->node));
}

bdd_t *
bdd_top_var(f)
bdd_t *f;
{
    return bdd_construct_bdd_t(f->mgr, cmu_bdd_if(f->mgr, f->node));
}

bdd_t *
bdd_xnor(f, g)
bdd_t *f;
bdd_t *g;
{
    return bdd_construct_bdd_t(f->mgr, cmu_bdd_xnor(f->mgr, f->node, g->node));
}

bdd_t *
bdd_xor(f, g)
bdd_t *f;
bdd_t *g;
{
    return bdd_construct_bdd_t(f->mgr, cmu_bdd_xor(f->mgr, f->node, g->node));
}

bdd_t *
bdd_zero(mgr)
struct bdd_manager_ *mgr;
{
    return bdd_construct_bdd_t(mgr, cmu_bdd_zero(mgr));
}

/*
Queries about BDD Formulas ----------------------------------------------------
*/

boolean
bdd_equal(f, g)
bdd_t *f;
bdd_t *g;
{
    return (f->node == g->node);
}

bdd_t *
bdd_intersects(f, g)
bdd_t *f;
bdd_t *g;
{
    return bdd_construct_bdd_t(f->mgr, cmu_bdd_intersects(f->mgr, f->node, g->node));
}

boolean
bdd_is_tautology(f, phase)
bdd_t *f;
boolean phase;
{
    return ((phase == TRUE) ? (f->node == cmu_bdd_one(f->mgr)) : (f->node == cmu_bdd_zero(f->mgr)));
}

boolean
bdd_leq(f, g, f_phase, g_phase)
bdd_t *f;
bdd_t *g;
boolean f_phase;
boolean g_phase;
{
    struct bdd_ *temp1, *temp2, *implies_fn;
    struct bdd_manager_ *mgr;
    boolean result_value;

    mgr = f->mgr;
    temp1 = ( (f_phase == TRUE) ? cmu_bdd_identity(mgr, f->node) : cmu_bdd_not(mgr, f->node));
    temp2 = ( (g_phase == TRUE) ? cmu_bdd_identity(mgr, g->node) : cmu_bdd_not(mgr, g->node));
    implies_fn = cmu_bdd_implies(mgr, temp1, temp2); /* returns a minterm of temp1*!temp2 */
    result_value = (implies_fn == cmu_bdd_zero(mgr));
    cmu_bdd_free(mgr, temp1);
    cmu_bdd_free(mgr, temp2);
    cmu_bdd_free(mgr, implies_fn);
    return result_value;
}

/*
Statistics and Other Queries --------------------------------------------------
*/

double 
bdd_count_onset(f, var_array)
bdd_t *f;
array_t *var_array;  	/* of bdd_t *'s */
{
    int num_vars;
    double fraction;

    num_vars = array_n(var_array);
    fraction = cmu_bdd_satisfying_fraction(f->mgr, f->node); /* cannot give support vars */
    return (fraction * pow((double) 2, (double) num_vars));
}

struct bdd_manager_ *
bdd_get_manager(f)
bdd_t *f;
{
    return (f->mgr);
}

bdd_node *
bdd_get_node(f, is_complemented)
bdd_t *f;
boolean *is_complemented;    /* return */
{
    *is_complemented = (boolean) TAG0(f->node);  /* using bddint.h */
    return ((bdd_node *) BDD_POINTER(f->node));  /* using bddint.h */
}

void
bdd_get_stats(mgr, stats)
struct bdd_manager_ *mgr;
bdd_stats *stats;	/* return */
{
    stats->gc.runtime = (long) mgr;  /* for use in bdd_print_stats; this is a hack */
    /* SIS simplify package looks at these fields */
    stats->nodes.total = stats->nodes.used = (unsigned int) cmu_bdd_total_size(mgr);    
    return;
}

var_set_t *
bdd_get_support(f)
bdd_t *f;
{
    struct bdd_ **support, *var;
    struct bdd_manager_ *mgr;
    long num_vars;
    var_set_t *result;
    int id, i;

    mgr = f->mgr;
    num_vars = cmu_bdd_vars(mgr);

    result = var_set_new((int) num_vars);
    support = (struct bdd_ **) mem_get_block((num_vars+1) * sizeof(struct bdd_ *));
    (void) cmu_bdd_support(mgr, f->node, support);

    for (i = 0; i < num_vars; ++i) {  /* can never have more than num_var non-zero entries in array */
	var = support[i]; 
	if (var == (struct bdd_ *) 0) {
	    break;  /* have reach end of null-terminated array */
	}
	id = (int) (cmu_bdd_if_id(mgr, var) - 1);  /* a variable is never garbage collected, so no need to free */
	var_set_set_elt(result, id);
    }

    mem_free_block((pointer)support);

    return result;
}

array_t *
bdd_get_varids(var_array)
array_t *var_array;
{
  int i;
  bdd_t *var;
  array_t *result;
 
  result = array_alloc(bdd_variableId, 0);
  for (i = 0; i < array_n(var_array); i++) {
    var = array_fetch(bdd_t *, var_array, i);
    array_insert_last(bdd_variableId, result, bdd_top_var_id(var));
  }
  return result;
}

unsigned int 
bdd_num_vars(mgr)
struct bdd_manager_ *mgr;
{
    return (cmu_bdd_vars(mgr));
}

void
bdd_print(f)
bdd_t *f;
{
    cmu_bdd_print_bdd(f->mgr, f->node, bdd_naming_fn_none, bdd_terminal_id_fn_none, (pointer) 0, stdout);
}

void
bdd_print_stats(stats, file)
bdd_stats stats;
FILE *file;
{
    struct bdd_manager_ *mgr;

    mgr = (struct bdd_manager_ *) stats.gc.runtime;
    cmu_bdd_stats(mgr, file);
}

int
bdd_size(f)
bdd_t *f;
{
    return ((int) cmu_bdd_size(f->mgr, f->node, 1));
}

bdd_variableId
bdd_top_var_id(f)
bdd_t *f;
{
    return ((bdd_variableId) (cmu_bdd_if_id(f->mgr, f->node) - 1));
}

/*
Miscellaneous -----------------------------------------------------------------
*/

bdd_external_hooks *
bdd_get_external_hooks(mgr)
struct bdd_manager_ *mgr;
{
    return ((bdd_external_hooks *) mgr->hooks);
}

void
bdd_register_daemon(mgr, daemon)
struct bdd_manager_ *mgr;
void (*daemon)();
{
    cmu_bdd_warning("bdd_register_daemon: translated to no-op in CMU package");
}

void
bdd_set_gc_mode(mgr, no_gc)
struct bdd_manager_ *mgr;
boolean no_gc;		
{
    cmu_bdd_warning("bdd_set_gc_mode: translated to no-op in CMU package");
}

void 
bdd_dynamic_reordering(mgr, algorithm_type)
struct bdd_manager_ *mgr;
bdd_reorder_type_t algorithm_type;
{
    switch(algorithm_type) {
    case BDD_REORDER_SIFT:
	cmu_bdd_dynamic_reordering(mgr, cmu_bdd_reorder_sift);
	break;
    case BDD_REORDER_WINDOW:
	cmu_bdd_dynamic_reordering(mgr, cmu_bdd_reorder_stable_window3);
	break;
    case BDD_REORDER_NONE:
	cmu_bdd_dynamic_reordering(mgr, cmu_bdd_reorder_none);
	break;
    default:
	cmu_bdd_fatal("bdd_dynamic_reordering: unknown algorithm type");
    }
}

void 
bdd_reorder(mgr)
struct bdd_manager_ *mgr;
{
    cmu_bdd_reorder(mgr);
}

bdd_variableId
bdd_get_id_from_level(mgr, level)
struct bdd_manager_ *mgr;
long level;
{
    struct bdd_ *fn;

    fn = cmu_bdd_var_with_index(mgr, level);

    if (fn == (struct bdd_ *) 0) {
	    /* variable should always be found, since they are created at bdd_start */
	    cmu_bdd_fatal("bdd_get_id_from_level: assumption violated");
    }

    return ((bdd_variableId)(cmu_bdd_if_id(mgr, fn) - 1 ));

}

long
bdd_top_var_level(mgr, fn)
struct bdd_manager_ *mgr;
bdd_t *fn;
{

	return cmu_bdd_if_index(mgr, fn->node);
}


