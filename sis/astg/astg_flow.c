
/* -------------------------------------------------------------------------- *\
   astg_flow.c -- do exhaustive token flow on an STG.

   int astg_token_flow (astg_graph *stg, astg_bool verbose);

   input: The initial marking is given by the initial_token bit of each
	  place in the STG.
\* ---------------------------------------------------------------------------*/

#ifdef SIS
#include "astg_int.h"
#include "astg_core.h"


array_t *astg_check_new_state (finfo,new_state)
astg_flow_t *finfo;
astg_scode new_state;
{
    astg_place *p;
    astg_trans *sv;
    astg_state *state_p;
    astg_marking *one_marking;
    unsigned n_disabled;
    array_t *enabled;
    astg_generator tgen, pgen, mgen;

    if (finfo->status != 0) return NULL;

    state_p = astg_find_state (finfo->stg,
		    astg_adjusted_code(finfo->stg,new_state),ASTG_TRUE);

    /* Check if this marking was reached already. */
    astg_foreach_marking (state_p,mgen,one_marking) {
	if (astg_cmp_marking(finfo->marking,one_marking) == 0) {
	    return NULL;	/* Marking was reached already. */
	}
    }

    enabled = array_alloc (astg_vertex *,0);

    astg_foreach_trans (finfo->stg,tgen,sv) {
	/* All input places are marked => transition enabled. */
	n_disabled = astg_disabled_count (sv,finfo->marking);
	if (n_disabled == 0) {
	    if (finfo->force_safe) {
		/* Special handling of unsafe condition. */
		astg_foreach_output_place (sv,pgen,p) {
		    if (astg_get_marked(finfo->marking,p)) n_disabled++;
		}
	    }
	    if (n_disabled == 0) {
		array_insert_last (astg_vertex *, enabled, sv);
		astg_set_useful(sv,ASTG_TRUE);
	    }
	} 
	else if (n_disabled == 1) {
	    /* Exactly one place is unmarked. */
	    astg_foreach_input_place (sv,pgen,p) {
		if (!astg_get_marked(finfo->marking,p)) astg_set_useful(p,ASTG_TRUE);
	    }
	}
    } 

    /* Add the new marking to this state. */
    one_marking = astg_dup_marking (finfo->marking);
    astg_add_marking (state_p, one_marking);
    if (astg_debug_flag >= 2) astg_print_marking (0,finfo->stg,one_marking);
    return enabled;
}

static int flow_state (flow_info,new_state)
astg_flow_t *flow_info;
astg_scode new_state;
{
    array_t *enabled = astg_check_new_state (flow_info,new_state);
    astg_trans *t;
    int i;

    if (enabled != NULL) {
	/* Recursively generate each succeeding marking. */
	for (i=array_n(enabled); flow_info->status == 0 && i--; ) {
	    t = array_fetch (astg_trans *, enabled, i);
	    new_state = astg_fire (flow_info->marking,t);
	    flow_state (flow_info,new_state); 
	    astg_unfire (flow_info->marking,t);
	} 
	array_free (enabled);
    }
}

static astg_scode astg_flow_outmask (stg)
astg_graph *stg;	/*i return output state bit mask for this stg.	*/
{
    /*	Compute a mask of the state bits of all output signals. */

    astg_trans *t;
    astg_generator gen;
    astg_signal *sig_p;
    astg_scode out_mask = 0;

    astg_foreach_trans (stg,gen,t) {
	sig_p = astg_trans_sig(t);
	if (astg_is_noninput(sig_p)) {
	    out_mask |= astg_state_bit (sig_p);
	}
    }

    return out_mask;
}


static astg_bool astg_flow_check_csc (stg,verbose)
astg_graph *stg;
astg_bool verbose;
{
    /*	Check for state coding problems in the STG. */

    astg_bool has_usc = ASTG_TRUE;
    astg_generator sgen, mgen;
    int n;
    astg_state *state_p;
    astg_marking *marking_p;
    astg_scode min_enabled, max_enabled, out_mask;

    if (astg_get_flow_status(stg) != ASTG_OK) return ASTG_FALSE;

    out_mask = astg_flow_outmask (stg);

    astg_foreach_state (stg,sgen,state_p) {
	min_enabled = out_mask;
	max_enabled = 0;
	astg_foreach_marking (state_p,mgen,marking_p) {
	    if (astg_is_dummy_marking (marking_p)) continue;
	    min_enabled &= astg_get_enabled(marking_p);
	    max_enabled |= astg_get_enabled(marking_p);
	}
	max_enabled &= out_mask;

	if (min_enabled != max_enabled) {
	    has_usc = ASTG_FALSE;
	    astg_set_flow_status (stg, ASTG_NOT_USC);
	    if (verbose) {
		printf("\nerror: state assignment problem for state");
		astg_print_state (stg,astg_state_code(state_p));
		n = 0;
		astg_foreach_marking (state_p,mgen,marking_p) {
		    astg_print_marking (++n,stg,marking_p);
		}
	    }
	}
    } /* end for each state code */

    return has_usc;
}

static void astg_flow_check_redundant (stg)
astg_graph *stg;
{
    astg_generator gen;
    astg_place *p;
    astg_trans *t;

    if (astg_get_flow_status(stg) != ASTG_OK) return;
    astg_sel_new (stg,"unfired transitions",ASTG_FALSE);
    astg_foreach_trans (stg,gen,t) {
	if (!astg_get_useful(t)) {
	    astg_sel_vertex (t,ASTG_TRUE);
	    astg_set_flow_status (stg,ASTG_NOT_LIVE);
	}
    }

    if (astg_get_flow_status(stg) != ASTG_OK) return;
    astg_sel_new (stg,"redundant places",ASTG_FALSE);
    astg_foreach_place (stg,gen,p) {
	if (!astg_get_useful(p)) {
	    astg_sel_vertex (p,ASTG_TRUE);
	}
    }
}

astg_retval astg_token_flow (stg,verbose)
astg_graph *stg;	/*u stg to do marking flow			*/
astg_bool verbose;	/*i ASTG_TRUE=print messages for flow problems	*/
{
    /*	If the previous flow status was ASTG_OK, and the graph has not
	changed structurally since the last flow, then just print messages.
	Otherwise, do exhaustive token flow on the astg. */

    astg_flow_t flow_rec;
    astg_retval status;
    astg_scode initial_state = 0;
    astg_bool redo_flow = astg_reset_state_graph (stg);

    astg_sel_clear (stg);

    if (redo_flow) {
	flow_rec.force_safe = ASTG_FALSE;
	flow_rec.status = 0;
	flow_rec.stg = stg;
	flow_rec.marking = astg_initial_marking (stg);
	flow_state (&flow_rec,initial_state);
	astg_delete_marking (flow_rec.marking);
    }

    astg_flow_check_csc (stg,verbose);
    astg_flow_check_redundant (stg);
    if (verbose) astg_sel_show (stg);

    status = astg_get_flow_status (stg);
    if (astg_debug_flag >= 2) {
	printf("flow: redo=%d, status=%d\n",redo_flow,status);
    }
    return status;
}
#endif /* SIS */
