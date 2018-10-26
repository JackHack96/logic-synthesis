
/* -------------------------------------------------------------------------- *\
   lockgraph.c -- build lock graph for a marked graph STG.

   Lock graph: vertices are astg_signal, edges are stored as adjacency list
   lg_edges of each signal.  A lock graph component is an array of signals.

   Needs more testing for case when do_lock is true.
\* ---------------------------------------------------------------------------*/

#ifdef SIS
#include "astg_int.h"
#include "astg_core.h"

#define ARRAY_DELETE_LAST(a)	(--((a)->num))
#define ARRAY_RESET(a)	(((a)->num) = 0)


typedef struct lock_class {
    array_t *lg_vertices;	/* Array of astg_signal *.		*/
    int connectivity;		/* Used to select class for locking.	*/
} lock_class;


typedef struct lc_rec {
    array_t *classes;		/* Array of lock_class struct		*/
    sm_matrix *lock_matrix;	/* Used to form the lock graph.		*/
    astg_graph *stg;		/* To be available to callbacks.	*/
} lc_rec;


static void print_lock_graph (stg)
astg_graph *stg;
{
    /*	Print adjacency list format of lock graph. */

    int n;
    astg_generator gen;
    astg_signal *sig_p, *edge;

    astg_foreach_signal (stg,gen,sig_p) {
	printf("%s:",sig_p->name);
	for (n=0; n < array_n(sig_p->lg_edges); n++) {
	    edge = array_fetch (astg_signal *, sig_p->lg_edges, n);
	    printf(" %s", edge->name);
	}
	printf("\n");
    }
}

static void print_path (stg)
astg_graph *stg;
{
    astg_vertex *v;
    astg_generator gen;

    astg_foreach_path_vertex (stg,gen,v) {
	if (v->vtype == ASTG_PLACE) continue;
	printf(" %s",astg_v_name(v));
    }
    printf("\n");
}

/* ------------------- Connected Components of Lock Graph ------------------- *\

   Uses a DFS to find the connected components of the lock graph.  Since the
   lock graph is undirected, this is also the strongly connected components.
\* -------------------------------------------------------------------------- */

static void astg_lg_comp (sig_p,component)
astg_signal *sig_p;
array_t *component;
{
    astg_signal *s;
    int n;

    if (sig_p->unprocessed) {
	sig_p->unprocessed = ASTG_FALSE;
	array_insert_last (astg_signal *,component,sig_p);
	for (n=array_n(sig_p->lg_edges); n--; ) {
	    s = array_fetch (astg_signal *,sig_p->lg_edges,n);
	    astg_lg_comp (s,component);
	}
    }
}

int astg_lock_graph_components (stg,f,fdata)
astg_graph *stg;		/*u STG to process.			*/
int (*f)();			/*i What to do with each component.	*/
void *fdata;
{
    astg_generator gen;
    astg_signal *sig_p;
    array_t *component;
    int n_comp = 0;

    astg_foreach_signal (stg,gen,sig_p) {
	sig_p->unprocessed = ASTG_TRUE;
    }

    astg_foreach_signal (stg,gen,sig_p) {
	if (sig_p->unprocessed) {
	    component = array_alloc (astg_signal *,0);
	    astg_lg_comp (sig_p,component);
	    if (f != NULL) (*f) (component,n_comp,fdata);
	    n_comp++;
	    array_free (component);
	}
    }

    return n_comp;
}

/* ----------------------- Shortest Path Algorithm -------------------------- *\

   astg_lock_graph_shortest_path: finding shortest path from single source.
   On exit, all weights for all signals will be set to the minimum
   path to that signal from the source.  From will be NULL if there
   is no path.
\* -------------------------------------------------------------------------- */

static void astg_sp_insert (q,sig_p,new_weight,src)
lsList q;
astg_signal *sig_p;
int new_weight;
astg_signal *src;
{
    astg_signal *old_sig;
    lsGen gen;

    if (sig_p->lg_queue != NULL) lsRemoveItem (sig_p->lg_queue,NULL);
    sig_p->lg_weight = new_weight;
    sig_p->lg_from = src;

    gen = lsStart (q);
    while (lsNext(gen,(lsGeneric*)&old_sig,LS_NH) == LS_OK) {
	if (new_weight <= old_sig->lg_weight) break;
    }

    lsInBefore (gen,(lsGeneric)sig_p,&sig_p->lg_queue);
}

static void astg_sp_adjust (src,dest,src_pqueue)
astg_signal *src, *dest;
lsList src_pqueue;
{
    int new_weight = src->lg_weight + 1;	/* edge weight == 1 */

    if (dest->unprocessed) {
	dest->unprocessed = ASTG_FALSE;
	astg_sp_insert (src_pqueue,dest,new_weight,src);
    }
    else if (new_weight < dest->lg_weight) {
	astg_sp_insert (src_pqueue,dest,new_weight,src);
    }
}

void astg_lock_graph_shortest_path (stg,source)
astg_graph  *stg;	/*u Lock graph path information is updated.	*/
astg_signal *source;	/*u Signal to start with in lock graph.		*/
{
    astg_generator gen;
    lsList  src_pqueue;
    astg_signal *sig_p, *head;
    int n;

    astg_foreach_signal (stg,gen,sig_p) {
	sig_p->unprocessed = ASTG_TRUE;
	sig_p->lg_queue = NULL;
	sig_p->lg_from = NULL;
    }

    src_pqueue = lsCreate ();
    source->unprocessed = ASTG_FALSE;
    astg_sp_insert (src_pqueue,source,0,NIL(astg_signal));

    while (lsDelBegin(src_pqueue,(lsGeneric*)&sig_p) == LS_OK) {
        sig_p->lg_queue = NULL;
	for (n=array_n(sig_p->lg_edges); n--; ) {
	    head = array_fetch (astg_signal *,sig_p->lg_edges,n);
	    astg_sp_adjust (sig_p,head,src_pqueue);
	}
    }

    lsDestroy (src_pqueue,NULL);
}

/* ----------------------------- Misc. Stuff -------------------------------- */

static astg_bool sandwich (t1,t,t2)
int t1, t, t2;
{
    int t1n, t2n;

    t1n = MIN (t1, t2);
    t2n = MAX (t1, t2);

    return (t1n < t && t < t2n);
}

static astg_bool interleaved (s1pos,s1neg,s2pos,s2neg)
int s1pos, s1neg;
int s2pos, s2neg;
{
    return sandwich (s1pos,s2pos,s1neg) ^ sandwich (s1pos,s2neg,s1neg);
}

static astg_bool lg_all_on_path (sig_p,weightp,weightn)
astg_signal *sig_p;
int *weightp, *weightn;
{
    astg_generator gen;
    astg_trans *t;

    astg_foreach_sig_trans (sig_p,gen,t) {
	if (!t->on_path) return ASTG_FALSE;
	if (t->type.trans.trans_type == ASTG_POS_X) {
	    *weightp = t->weight1.i;
	} else if (t->type.trans.trans_type == ASTG_NEG_X) {
	    *weightn = t->weight1.i;
	} else {
	    fail("bad trans");
	}
    }
    return ASTG_TRUE;
}

static int find_interlocks (stg,data)
astg_graph *stg;
void *data;
{
    /* callback for simple cycles to find lock matrix */
    astg_signal *sig1, *sig2;
    astg_generator sgen1, sgen2;
    astg_vertex *v;
    int n = 0;
    lc_rec *lminfo = (lc_rec *) data;
    int weight1p, weight1n, weight2p, weight2n;

    astg_foreach_path_vertex (stg,sgen1,v) {
	v->weight1.i = n++;
    }

    astg_foreach_signal (lminfo->stg,sgen1,sig1) {

	if (!lg_all_on_path(sig1,&weight1p,&weight1n)) continue;

	astg_foreach_signal (lminfo->stg,sgen2,sig2) {

	    if (!lg_all_on_path(sig2,&weight2p,&weight2n)) continue;

	    /* Both signals are on cycle -- check for interleaving. */
	    if (interleaved (weight1p,weight1n,weight2p,weight2n)) {
		if (sm_find (lminfo->lock_matrix,sig1->id,sig2->id) == 0) {
		    if (sig1->id < sig2->id) {
			dbg(3,msg("%s SL %s because:",sig1->name,sig2->name));
			dbg(3,print_path (stg));
		    }
		    sm_insert (lminfo->lock_matrix,sig1->id,sig2->id);
		}
	    }

	} /* end for each sig2 */
    } /* end for each sig1 */

    return 1;
}

static int save_lock_classes (component,comp_n,data)
array_t *component;
int comp_n;		/* ARGSUSED */
void *data;
{
    /* Callback for strong components on lock graph */
    lc_rec *lc_info = (lc_rec *) data;
    lock_class *new_class = ALLOC (lock_class,1);

    new_class->lg_vertices = array_dup (component);
    array_insert_last (lock_class *,lc_info->classes,new_class);
    return 0;
}

static void free_lock_classes (lc_info)
lc_rec *lc_info;
{
    int n;

    if (lc_info->classes != NULL) {
	for (n=array_n(lc_info->classes); n--; ) {
	    array_free (array_fetch(array_t *,lc_info->classes,n));
	}
	array_free (lc_info->classes);
	lc_info->classes = NULL;
    }
}

static int make_lock_graph (stg,lc_info)
astg_graph *stg;
lc_rec *lc_info;
{
    astg_signal *sig_p;
    astg_generator sgen;
    astg_signal *sig1, *sig2;
    array_t *mapping;
    int i,j,n = 0;
    int n_comp;

    lc_info->lock_matrix = sm_alloc ();
    mapping = array_alloc (astg_vertex *,0);
    astg_foreach_signal (stg,sgen,sig_p) {
	sig_p->id = n++;
	array_insert_last (astg_signal *, mapping, sig_p);
    }

    lc_info->stg = stg;
    astg_simple_cycles (stg,ASTG_ALL_CYCLES,find_interlocks,(void *)lc_info,ASTG_ALL);

    for (i=0; i < stg->n_sig; i++) {
	for (j=0; j < i; j++) {
	    assert ((sm_find(lc_info->lock_matrix,i,j) == 0) == (sm_find(lc_info->lock_matrix,j,i) == 0));
	    if (sm_find(lc_info->lock_matrix,i,j)) {
		sig1 = array_fetch (astg_signal *,mapping,i);
		sig2 = array_fetch (astg_signal *,mapping,j);
		/* Add to both lists since lock graph is undirected. */
		array_insert_last (astg_signal *,sig1->lg_edges,sig2);
		array_insert_last (astg_signal *,sig2->lg_edges,sig1);
	    }
	}
    }

    array_free (mapping);
    sm_free (lc_info->lock_matrix);

    free_lock_classes (lc_info);
    lc_info->classes = array_alloc (array_t *,0);
    n_comp = astg_lock_graph_components (stg, save_lock_classes,(void *)lc_info);
    if (n_comp <= 1) {
	free_lock_classes (lc_info);
    }
    dbg(1,msg("lock graph has %d componen%s\n",n_comp,n_comp==1?"t":"ts"));
    return n_comp;
}

/* ----------------------- Rectify Unique State Coding ------------------- */

static astg_bool can_lock_trans (t1,t2)
astg_trans *t1, *t2;
{
    /* Try t1 -> t2 -> t1->opp_trans -> t2->opp_trans 
	This has to bypass places -- not a gr function */
    astg_bool can_lock = ASTG_TRUE;

    if (astg_is_rel (astg_noninput_trigger(t2),t1) ||
	astg_is_rel (astg_noninput_trigger(astg_trx(t1)->opp_trans),t2) ||
	astg_is_rel (astg_noninput_trigger(astg_trx(t2)->opp_trans),astg_trx(t1)->opp_trans) ||
	astg_is_rel (astg_noninput_trigger(t1),astg_trx(t2)->opp_trans)) {
            can_lock = ASTG_FALSE;
    }
    dbg(3,msg("can_lock %s %s = %d\n",astg_trans_name(t1),astg_trans_name(t2),can_lock));
    return can_lock;
}

static astg_do_lock (t1,t2)
astg_trans *t1, *t2;
{
    /* Form lock t1 -> t2 -> t1->opp -> t2->opp */

    dbg(1,msg("locking %s to %s\n",t1->name,t2->name));
    astg_add_constraint (t1,astg_noninput_trigger(t2),ASTG_TRUE);
    astg_add_constraint (t2,astg_noninput_trigger(astg_trx(t1)->opp_trans),ASTG_TRUE);
    astg_add_constraint (astg_trx(t1)->opp_trans,astg_noninput_trigger(astg_trx(t2)->opp_trans),ASTG_TRUE);
    astg_add_constraint (astg_trx(t2)->opp_trans,astg_noninput_trigger(t1),ASTG_TRUE);
}

static astg_bool astg_lock_sig (s1,s2,modify)
astg_signal *s1, *s2;
astg_bool modify;
{
    /* Try either s1+ -> s2+ -> s1- -> s2- or
		  s1+ -> s2- -> s1- -> s2+	*/

    astg_trans *t_s1_plus;
    astg_trans *t_s2_plus;
    astg_generator tgen1, tgen2;

    astg_foreach_pos_trans (s1,tgen1,t_s1_plus) {
	astg_foreach_pos_trans (s2,tgen2,t_s2_plus) {
	    if (can_lock_trans (t_s1_plus,t_s2_plus)) {
		if (modify) astg_do_lock (t_s1_plus,t_s2_plus);
		return ASTG_TRUE;
	    }
	    else if (can_lock_trans (t_s1_plus,astg_trx(t_s2_plus)->opp_trans)) {
		if (modify) astg_do_lock (t_s1_plus,astg_trx(t_s2_plus)->opp_trans);
		return ASTG_TRUE;
	    }
	}
    }

    return ASTG_FALSE;
}

static int astg_lock_sig_cost (stg,sig1,sig2)
astg_graph *stg;
astg_signal *sig1;
astg_signal *sig2;
{
    int cost = 0;
    astg_signal *s1, *s2;
    astg_generator gen1, gen2;

    /* Temporarily add this arc to the lock graph. */
    array_insert_last (astg_signal *, sig1->lg_edges, sig2);
    array_insert_last (astg_signal *, sig2->lg_edges, sig1);

    astg_foreach_signal (stg,gen1,s1) {
	astg_lock_graph_shortest_path (stg,s1);
	astg_foreach_signal (stg,gen2,s2) {
	    if (astg_is_trigger(s1,s2) || astg_is_trigger(s2,s1)) {
		cost += s2->lg_weight;
	    }
	}
    }

    ARRAY_DELETE_LAST (sig1->lg_edges);
    ARRAY_DELETE_LAST (sig2->lg_edges);
    return cost;
}

static astg_bool select_from_lc (stg,lc1,sig1_p,lc2,sig2_p)
astg_graph *stg;
lock_class *lc1, *lc2;
astg_signal **sig1_p, **sig2_p;
{
    astg_bool found_sigs = ASTG_FALSE;
    astg_signal *sig1, *sig2;
    int new_cost, min_cost = 0;
    int n1, n2;

    *sig1_p = *sig2_p = NULL;

    for (n1=array_n(lc1->lg_vertices); n1--; ) {
	sig1 = array_fetch (astg_signal *,lc1->lg_vertices,n1);
	for (n2=array_n(lc2->lg_vertices); n2--; ) {
	    sig2 = array_fetch (astg_signal *,lc2->lg_vertices,n2);

	    if (astg_lock_sig(sig1,sig2,ASTG_FALSE)) {
		new_cost = astg_lock_sig_cost (stg,sig1,sig2);
		dbg(2,msg("  cost = %d (min=%d)\n",new_cost,min_cost));
		if (!found_sigs || new_cost < min_cost) {
		    found_sigs = ASTG_TRUE;
		    min_cost = new_cost;
		    *sig1_p = sig1;  *sig2_p = sig2;
		}
	    }

	} /* end for each sig2 */
    } /* end for each sig2 */

    return found_sigs;
}

static int measure_lc (lc1,lc2)
lock_class *lc1, *lc2;
{
    /* measure the "connectivity" between these two lock classes. */
    return (array_n(lc1->lg_vertices) - array_n(lc2->lg_vertices));
}

static int cmp_lc (obj1,obj2)
char *obj1, *obj2;
{
    /* Callback for array_sort */
    lock_class *lc1 = *(lock_class **) obj1;
    lock_class *lc2 = *(lock_class **) obj2;

    /* Sort in order of decreasing connectivity measures. */
    return (lc2->connectivity - lc1->connectivity);
}

static astg_bool select_two_signals (lc_info,sig1_p,sig2_p)
lc_rec *lc_info;
astg_signal **sig1_p;
astg_signal **sig2_p;
{
    astg_bool found_sig = ASTG_FALSE;
    lock_class *lc1, *lc2;
    int n;

    /* Just take the first lock class arbitrarily. */
    dbg(1,msg("select two signals\n"));
    lc1 = array_fetch (lock_class *,lc_info->classes,0);
    lc1->connectivity = 0;

    for (n=0; n < array_n(lc_info->classes); n++) {
	lc2 = array_fetch (lock_class *,lc_info->classes,n);
	if (lc2 == lc1) continue;
	lc2->connectivity = measure_lc (lc1,lc2);
    }
    array_sort (lc_info->classes,cmp_lc);

    for (n=0; n < array_n(lc_info->classes); n++) {
	lc2 = array_fetch (lock_class *,lc_info->classes,n);
	if (lc2 == lc1) continue;
	dbg(2,msg("checking LC %d\n",n));
	found_sig = select_from_lc (lc_info->stg,lc1,sig1_p,lc2,sig2_p);
	if (found_sig) break;
    }

    dbg(1,msg("exit select two sig %d\n",found_sig));
    return found_sig;
}

static void lock_new_sig (stg)
astg_graph *stg;
{
    astg_signal   *new_sig;
    astg_trans *pos_t, *neg_t;

    new_sig = astg_new_sig (stg,ASTG_INTERNAL_SIG);

    /* Find two existing signals to lock this to. */
    pos_t = astg_find_trans (stg,new_sig->name,ASTG_POS_X, 0,ASTG_TRUE);
    neg_t = astg_find_trans (stg,new_sig->name,ASTG_NEG_X,0,ASTG_TRUE);

    /* Go ahead and lock this up. */
    if (pos_t != NULL && neg_t != NULL) {
	dbg(1,msg("How to lock these?\n"));
    }
}

astg_bool astg_state_coding (stg,do_lock)
astg_graph *stg;
astg_bool do_lock;
{
    /*	Optionally try to form one lock class, and print lock graph. */

    astg_bool rc = ASTG_TRUE;
    astg_signal *sig_p, *sig1, *sig2;
    astg_generator sgen;
    lc_rec lc_info;

    if (!astg_is_marked_graph(stg)) {
	printf("Sorry, lock graphs only work for marked graph STGs.\n");
	return 1;
    }

    if (astg_set_opp (stg,ASTG_ALL) != 0) return 1;

    astg_foreach_signal (stg,sgen,sig_p) {
	ARRAY_RESET (sig_p->lg_edges);
	if (sig_p->n_trans != 2) {
	    printf("%s has %d transitions of signal %s.\n",
		astg_name(stg), sig_p->n_trans, astg_signal_name(sig_p));
	    printf("Lock graph only applies for unique transitions.\n");
	    return 1;
	}
    }

    lc_info.stg = stg;
    lc_info.lock_matrix = NULL;
    lc_info.classes = NULL;

    while (make_lock_graph (stg,&lc_info) > 1 && do_lock) {

	if (select_two_signals (&lc_info,&sig1,&sig2)) {
	    astg_lock_sig (sig1,sig2,ASTG_TRUE);
	}
	else {
	    /* Add a signal and lock to that. */
	    dbg(1,msg("Could not find two signals to lock.\n"));
	    lock_new_sig (stg);
	    rc = ASTG_FALSE;
	    break;
	}

    } /* end while */

    print_lock_graph (stg);
    return rc;
}
#endif /* SIS */
