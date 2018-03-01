/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/astg/astg_persist.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:59 $
 *
 */
/* -------------------------------------------------------------------------- *\
   persist.c -- algorithm for making persistent MG STGs.
\* ---------------------------------------------------------------------------*/

#ifdef SIS
#include "astg_int.h"
#include "astg_core.h"

typedef struct {
    astg_vertex *per1;
    astg_vertex *per2;
} per_two_trans;


static int find_per (stg,data)
astg_graph *stg;	/* ARGSUSED */
void *data;		/*i persistency information structure.	*/
{
    per_two_trans *per = (per_two_trans *) data;

    /* Simple cycles callback: check if two transitions are on the cycle. */
    return (per->per1->on_path && per->per2->on_path);
}

static int find_opp (stg,data)
astg_graph *stg;	/*u Try to match +/- transition pairs.	*/
void *data;		/*  ARGSUSED */
{
    /* Simple cycles callback to locate transition matchings. */

    astg_generator gen;
    astg_trans *t;
    astg_signal *sig_p;

    astg_foreach_path_vertex (stg,gen,t) {
	if (t->vtype != ASTG_TRANS) continue;
	sig_p = astg_trx(t)->sig_p;
	sig_p->mark = ASTG_FALSE;
	sig_p->n_found++;
	if (sig_p->n_found == sig_p->n_trans) {
	    sig_p->last_trans = t;
	    sig_p->mark = ASTG_TRUE;
	}
    }

    astg_foreach_path_vertex (stg,gen,t) {
	if (t->vtype != ASTG_TRANS) continue;
	sig_p = astg_trx(t)->sig_p;
	sig_p->n_found = 0;
	if (sig_p->mark) {
	    assert (astg_trx(t)->trans_type != astg_trx(sig_p->last_trans)->trans_type);
	    astg_trx(sig_p->last_trans)->opp_trans = t;
	    sig_p->last_trans = t;
	}
    }

    return 0;
}

int astg_set_opp (stg,subset)
astg_graph *stg;
astg_bool subset;
{
    int result = 0;
    astg_signal *sig_p;
    astg_trans *t;
    astg_generator tgen, sgen;

    astg_foreach_signal (stg,sgen,sig_p) {
	sig_p->n_trans = 0;
	sig_p->n_found = 0;
    }

    astg_foreach_trans (stg,tgen,t) {
	if (subset && !t->subset) continue;
	astg_trx(t)->sig_p->n_trans++;
	astg_trx(t)->opp_trans = NULL;
    }

    astg_simple_cycles (stg,ASTG_ALL_CYCLES,find_opp,ASTG_NO_DATA,subset);

    astg_foreach_trans (stg,tgen,t) {
	if (subset && !t->subset) continue;
	if (astg_trx(t)->opp_trans == NULL) {
	    printf("No opposite found for %s.\n",t->name);
	    result = -1;
	}
    }

    return result;
}

static int make_mgc_persistent (stg,modify)
astg_graph  *stg;
astg_bool modify;		/*i ASTG_FALSE=only identify nonpersistent transitions	*/
{
    astg_vertex *dest;
    astg_graph *g = stg;
    lsList vertex_stack;
    int n_added = 0;
    per_two_trans per_rec;
    astg_generator tgen, pgen;
    astg_trans *t;
    astg_place *p;

    if (astg_set_opp (stg,ASTG_SUBSET) != 0) return -1;
    vertex_stack = lsCreate();

    astg_foreach_trans (stg,tgen,t) {
        if (t->subset && astg_out_degree(t) > 1) {
	    lsNewBegin (vertex_stack, (lsGeneric)t, LS_NH);
	}
    }

    while (lsDelBegin(vertex_stack, (lsGeneric*) &t) == LS_OK) {
	assert (t->vtype == ASTG_TRANS);
	astg_foreach_output_place (t,pgen,p) {
	    per_rec.per1 = p->out_edges->head;
	    per_rec.per2 = astg_trx(t)->opp_trans;
	    assert (per_rec.per1->vtype == ASTG_TRANS);
	    if (astg_simple_cycles (g,ASTG_ALL_CYCLES,find_per,(void *)&per_rec,ASTG_SUBSET) == 0) {
		dbg(1,msg("  %s -> %s non-persistent transition\n",
			astg_v_name(t),astg_v_name(per_rec.per1)));
		astg_sel_vertex (t,ASTG_TRUE);
		if (modify) {		/* Add a persistency constraint. */
		    dest = astg_noninput_trigger (per_rec.per2);
		    if (dest == NULL) {
			printf("Error: cannot make %s -> %s persistent\n",
			    astg_trans_name(t),astg_trans_name(per_rec.per1));
			printf("because no noninput transition enables %s.\n",
			    astg_trans_name(per_rec.per2));
		    } else {
			n_added++;
			astg_add_constraint (per_rec.per1,dest,ASTG_FALSE);
			/* Now per1 needs to be checked. */
			lsNewBegin (vertex_stack, (lsGeneric)per_rec.per1, LS_NH);
		    }
		}
	    }
	}
    }

    lsDestroy (vertex_stack,NULL);
    return n_added;
}

int make_persistent (stg,modify)
astg_graph  *stg;
astg_bool modify;
{
    int status = 0, n_added = 0;
    array_t *mgc;
    astg_vertex *v;
    astg_generator gen;
    int mgc_n, n, n_scc;

    n_scc = astg_connected_comp (stg,ASTG_NO_CALLBACK,ASTG_NO_DATA,ASTG_ALL);
    if (n_scc != 1) {
	printf("error: %s is not connected (it has %d components).\n",
	    astg_name(stg), n_scc);
	status = -1;
    }
    else {
	dbg(1,msg("checking each MGC for nonpersistent transitions\n"));
	get_mg_comp (stg,ASTG_FALSE);

	astg_sel_new (stg,"nonpersistent transitions",ASTG_FALSE);
	for (mgc_n=0; mgc_n < array_n(stg->mg_comp); mgc_n++) {
	    mgc = array_fetch (array_t *,stg->mg_comp,mgc_n);
	    astg_foreach_vertex (stg,gen,v) v->subset = ASTG_FALSE;
	    for (n=0; n < array_n(mgc); n++) {
		v = array_fetch (astg_vertex *,mgc,n);
		v->subset = ASTG_TRUE;
	    }
	    dbg(1,msg("MG component %d...\n",mgc_n+1));
	    status = make_mgc_persistent (stg,modify);
	    if (status < 0) break;
	    n_added += status;
	}
    }

    dbg(1,msg("added %d persistency constraints\n",n_added));
    return status;
}
#endif /* SIS */
