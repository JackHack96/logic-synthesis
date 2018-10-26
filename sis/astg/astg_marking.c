
/* -------------------------------------------------------------------------- *\
   marking.c -- conversion between markings and state codings.
\* ---------------------------------------------------------------------------*/

#ifdef SIS
#include "astg_int.h"
#include "astg_core.h"

#define NOT_IN_SM_FLAG		flag1

astg_bool valid_marking (stg)
astg_graph *stg;
{
    int i,j;
    int n_token, token_count;
    array_t *smc;
    astg_vertex *v;
    int x = 0;
    astg_bool good_marking = ASTG_TRUE;
    astg_generator pgen;

    n_token = 0;
    astg_foreach_place (stg,pgen,v) {
	if (astg_plx(v)->initial_token) n_token++;
    }
    dbg(1,msg("checking marking of %d tokens\n",n_token));

    for (i=0; i < array_n(stg->sm_comp); i++) {
	smc = array_fetch (array_t *, stg->sm_comp, i);
	token_count = 0;
	for (j=0; j < array_n(smc); j++) {
	    v = array_fetch (astg_vertex *, smc, j);
	    if (v->vtype != ASTG_PLACE) continue;
	    if (astg_plx(v)->initial_token) token_count++;
	}
	if (token_count != 1) {
	    good_marking = ASTG_FALSE;
	    printf("warning: SM component %d has %d tokens:\n",
		i+1,token_count);
	    astg_print_component (x++,smc,stdout);
	}
    }

    return good_marking;
}

static lsStatus remove_marked_smc (data,arg)
lsGeneric data;
lsGeneric arg;		/*  ARGSUSED */
{
    array_t *smc = (array_t *) data;
    astg_vertex *place;
    astg_vertex *v;
    lsStatus   rc = LS_OK;
    int i;

    for (i=0; i < array_n(smc); i++) {
	place = array_fetch (astg_vertex *, smc, i);
	if (place->vtype != ASTG_PLACE) continue;
	if (astg_plx(place)->initial_token) {
	    rc = LS_DELETE;
	    break;
	}
    }

    if (rc == LS_DELETE) {
	/* This component has a token, so mark all its
	   vertices as invalid for further marking. */
	dbg(2,msg("deleting smc\n"));
	for (i=0; i < array_n(smc); i++) {
	    v = array_fetch (astg_vertex *, smc, i);
	    v->NOT_IN_SM_FLAG = ASTG_FALSE;
	}
    }

    return rc;
}

static astg_bool find_marking (stg)
astg_graph *stg;
{
    lsList   unmarked_smc;
    array_t *smc;
    astg_vertex *place, *new_marked;
    astg_generator gen;
    astg_vertex *v;
    astg_bool unmarked;
    astg_generator pgen;
    astg_place *p;
    int i;

    astg_foreach_vertex (stg,gen,v) {
	v->NOT_IN_SM_FLAG = ASTG_TRUE;		/* is not in marked SM	*/
    }

    stg->has_marking = ASTG_FALSE;
    stg->change_count++;
    astg_foreach_place (stg,pgen,p) {
	astg_plx(p)->initial_token = ASTG_FALSE;
    }

    unmarked_smc = lsCreate ();
    for (i=0; i < array_n(stg->sm_comp); i++) {
	smc = array_fetch (array_t *, stg->sm_comp, i);
	lsNewBegin (unmarked_smc,(char *)smc,(lsHandle *)NULL);
    }

    while (lsDelBegin (unmarked_smc,(lsGeneric *)&smc) != LS_NOMORE) {

	/* Attempt to place token in this SM. */
	unmarked = ASTG_TRUE;
	for (i=0; i < array_n(smc); i++) {
	    place = array_fetch (astg_vertex *, smc, i);
	    if (place->vtype != ASTG_PLACE) continue;
	    if (unmarked && place->NOT_IN_SM_FLAG) {
		astg_plx(place)->initial_token = ASTG_TRUE;
		new_marked = place;
		unmarked = ASTG_FALSE;
	    }
	    place->NOT_IN_SM_FLAG = ASTG_FALSE;
	}

	if (unmarked) break;
	dbg(1,msg("marking %s\n",astg_place_name(new_marked)));

	lsForeach (unmarked_smc, remove_marked_smc,   (lsGeneric)new_marked);

    } /* end while */

    lsDestroy (unmarked_smc,(void (*)())NULL);

    if (unmarked) {
	printf("error: no 1-token state machine marking.\n");
    }
    else {
	stg->has_marking = ASTG_TRUE;
	if (astg_debug_flag >= 2) {
	    printf("Valid initial marking:\n");
	    astg_foreach_place (stg,pgen,p) {
		if (astg_plx(p)->initial_token) printf(" %s",p->name);
	    }
	    printf("\n");
	}
    }

    return stg->has_marking;
}

astg_bool astg_one_sm_token (stg)
astg_graph *stg;
{
    astg_bool found_marking = ASTG_FALSE;

    if (get_sm_comp (stg,ASTG_TRUE) != ASTG_OK ||
	     get_mg_comp (stg,ASTG_TRUE) != ASTG_OK) {
	printf("No initial marking; STG is not even live/safe.\n");
    }
    else if (stg->has_marking) {
	found_marking = valid_marking (stg);
    }
    else {
	found_marking = find_marking (stg);
    }

    return found_marking;
}

extern astg_retval astg_initial_state (stg,state_p)
astg_graph *stg;	/*u do token flow for this STG			*/
astg_scode *state_p;	/*o state code if result OK or NOT_USC		*/
{
    /*	Returns the state code and corresponding marking for some initial state
	of the STG.  If a marking has been set with the initial_token flags of
	each place, then that is used, otherwise an arbitrary live-safe marking
	is selected.  Returns:
	  ASTG_OK:      state code of initial marking is returned.
	  ASTG_NO_MARKING: no live-safe initial marking.
	  ASTG_NOT_SAFE: the initial marking was not safe. */

    astg_retval status = ASTG_NO_MARKING;

    if (stg->has_marking || astg_one_sm_token(stg)) {
	status = astg_token_flow (stg,ASTG_FALSE);
	if (status == ASTG_OK || status == ASTG_NOT_USC) {
	    *state_p = astg_adjusted_code(stg,stg->flow_info.initial_state);
	    status = ASTG_OK;
	}
    }

    return status;
}

extern astg_retval astg_unique_state (stg,state_code)
astg_graph *stg;
astg_scode *state_code;
{
    /*	Returns an arbitrary unique state code for this STG.  Even if the STG
	has a state assignment problem, a state code will be returned which
	corresponds to a unique marking.  Possible return values:
	    ASTG_OK: a unique state code is returned
	    ASTG_NO_MARKING: no state codes were unique
	otherwise some other token flow error occured. */

    astg_retval status;
    astg_generator sgen;
    astg_state *state_p;

    status = astg_token_flow (stg,ASTG_FALSE);
    if (status == ASTG_OK) {
	/* Any state including initial state is okay. */
	*state_code = astg_adjusted_code(stg,stg->flow_info.initial_state);
    }
    else if (status == ASTG_NOT_USC) {
	/* Try to find some unique state. */
	status = ASTG_NO_MARKING;
	astg_foreach_state (stg,sgen,state_p) {
	    if (status == ASTG_OK) continue;
	    if (astg_state_n_marking(state_p) == 1) {
		status = ASTG_OK;
		*state_code = astg_state_code (state_p);
	    }
	}

	if (status != ASTG_OK) {
	    printf("No unique state code was found.\n");
	}
    }

    return status;
}

astg_retval astg_set_marking_by_code (stg,state_code,sig_mask)
astg_graph *stg;	/*u Net to set an initial marking for.		*/
astg_scode state_code;	/*i State code for desired initial marking.	*/
astg_scode sig_mask;	/*i Mask of signals to ignore in state code.	*/
{
    astg_retval status;
    astg_generator sgen;
    astg_state *state_p;
    astg_marking *marking_p;
    astg_scode mismatches;

    status = astg_token_flow (stg,ASTG_FALSE);
    if (status != ASTG_OK && status != ASTG_NOT_USC) return status;
    status = ASTG_NO_MARKING;

    astg_foreach_state (stg,sgen,state_p) {
	if (status == ASTG_OK) continue;
	mismatches = (~sig_mask) & (state_code ^ astg_state_code(state_p));
	if (mismatches == 0) {
	    status = ASTG_OK;
	    marking_p = array_fetch (astg_marking *,state_p->states,0);
	    astg_set_marking (stg,marking_p);
	}
    }

    dbg(1,msg("set_marking %d\n",status));
    return status;
}

extern astg_retval astg_set_marking_by_name (stg,sig_values)
astg_graph *stg;
st_table *sig_values;
{
    /*	Set the marking using pairs of signal name/value.  The key for the hash
	table is signal name strings, the data is an int which nonzero means
	the signal should have level 1, otherwise 0.  These values are combined
	into a astg_scode value and then astg_set_marking is called.
	Return values:
	    ASTG_OK: marking found and set
	    ASTG_BAD_SIGNAME: unrecognized signal name, message is printed
	    ASTG_NO_MARKING: no compatible marking could be found
	Other various flow errors can occur (not safe, etc.).  Also see
	astg_set_marking. */

    astg_retval status;
    st_generator *sig_gen;
    astg_signal *sig_p;
    char *sig_name;
    astg_scode state_code = 0, sig_mask = 0;
    int   value;

    status = astg_token_flow (stg,ASTG_FALSE);
    /* State assignment problem does not phaze us here. */
    if (status == ASTG_NOT_USC) status = ASTG_OK;
    if (status != ASTG_OK) return status;

    st_foreach_item_int (sig_values,sig_gen,(char **)&sig_name, &value) {
	sig_p = astg_find_named_signal (stg,sig_name);
	if (sig_p == NULL) {
	    printf("No signal '%s'\n",sig_name);
	    status = ASTG_BAD_SIGNAME;
	}
	else {
	    sig_mask |= astg_signal_bit (sig_p);
	    if (value) state_code |= astg_signal_bit (sig_p);
	}
    }

    if (status == ASTG_OK) {
	status = astg_set_marking_by_code (stg,state_code,sig_mask);
    }

    return status;
}
#endif /* SIS */
