
/* -------------------------------------------------------------------------- *\
   astg2.c -- Graph and net algorithms for STG's.
\* ---------------------------------------------------------------------------*/

#ifdef SIS
#include "astg_int.h"
#include "astg_core.h"

/* ---------------------------- ASTG Daemons -------------------------------- */

static astg_daemon_t *daemon_list = NULL;

void astg_do_daemons (stg1,stg2,type)
astg_graph *stg1, *stg2;
astg_daemon_enum type;
{
    /*	Execute all daemons of the given type. */
    astg_daemon_t *d;

    dbg(1,printf("do daemons %lX %lX %d\n",stg1,stg2,type));

    for (d=daemon_list; d != NULL; d=d->next) {
    if (type == d->type) (*d->daemon)(stg1,stg2);
    }
}

extern void astg_register_daemon (type, daemon)
astg_daemon_enum type;
astg_daemon daemon;
{
    /*	Register an ASTG daemon.  A daemon is a function which takes two
    arguments:
      static void daemon (astg_graph *g1, astg_graph *g2);
    The usage of the arguments depends on the daemon type:
      ASTG_DAEMON_ALLOC: g1 is the new astg, and g2 is NULL.
      ASTG_DAEMON_DUP: g1 is the new astg, and g2 is the old astg.
      ASTG_DAEMON_INVALID: g1 is invalidated astg, g2 is NULL.
      ASTG_DAEMON_FREE: g1 is the old astg, and and g2 is NULL.
    The astg g2 must never be modified. */

    astg_daemon_t *info = ALLOC(astg_daemon_t,1);
    info->daemon = daemon;
    info->type = type;
    info->next = daemon_list;
    daemon_list = info;
}

void astg_discard_daemons ()
{
    /*	Called at the very end to free structures for daemons. */

    astg_daemon_t *info = daemon_list, *next;

    for (info=daemon_list; info != NULL; info=next) {
    next = info->next;
    FREE (info);
    }
    daemon_list = NULL;
}

extern void *astg_get_slot (stg,slot_id)
astg_graph *stg;
astg_slot_enum slot_id;
{
    /*	Return the value of the given slot. */
    return stg->slots[(int)slot_id];
}

extern void astg_set_slot (stg,slot_id,value)
astg_graph *stg;
astg_slot_enum slot_id;
void *value;
{
    /*	Set a slot to a new value. */
    stg->slots[(int)slot_id] = value;
}

/* --------------------------- Net Predicates ------------------------------- */

extern astg_bool astg_is_marked_graph (stg)
astg_graph *stg;
{
    /*	Returns true if stg is a marked graph. */

    astg_generator pgen;
    astg_place *p;
    astg_bool is_mg = ASTG_TRUE;

    astg_foreach_place (stg,pgen,p) {
    if (astg_in_degree(p) > 1 || astg_out_degree(p) > 1) {
        is_mg = ASTG_FALSE;
    }
    }
    return is_mg;
}

extern astg_bool astg_is_state_machine (stg)
astg_graph *stg;
{
    /*	Returns true if stg is a state machine. */

    astg_bool is_sm = ASTG_TRUE;
    astg_generator tgen;
    astg_trans *t;

    astg_foreach_trans (stg,tgen,t) {
    if (astg_in_degree(t) > 1 || astg_out_degree(t) > 1) {
        is_sm = ASTG_FALSE;
    }
    }
    return is_sm;
}

extern astg_bool astg_is_free_choice_net (stg)
astg_graph *stg;
{
    /*	Returns true if the stg is a Free Choice net. */

    astg_bool is_fcn = ASTG_TRUE;
    astg_generator pgen, tgen;
    astg_place *p;
    astg_trans *t;

    astg_sel_new (stg,"not free choice",ASTG_FALSE);
    astg_foreach_place (stg,pgen,p) {
    if (astg_out_degree(p) <= 1) continue;
    astg_foreach_output_trans (p,tgen,t) {
        if (astg_in_degree(t) != 1) {
        astg_sel_vertex (p,ASTG_TRUE);
        astg_sel_vertex (t,ASTG_TRUE);
        is_fcn = ASTG_FALSE;
        }
    }
    }
    return is_fcn;
}

astg_bool astg_is_place_simple (g)
astg_graph *g;
{
    astg_generator gen1, gen2, gen3;
    astg_vertex *src_t, *dest_t;
    astg_edge *e, *e2;
    astg_bool place_simple = ASTG_TRUE;

    astg_foreach_trans (g,gen1,src_t) src_t->flag0 = ASTG_FALSE;

    astg_foreach_trans (g,gen1,src_t) {
    astg_foreach_out_edge (src_t,gen2,e) {
        astg_foreach_out_edge (astg_head(e),gen3,e2) {
        dest_t = astg_head (e2);
        if (dest_t->flag0) place_simple = ASTG_FALSE;
        dest_t->flag0 = ASTG_TRUE;
        }
    }
    astg_foreach_out_edge (src_t,gen2,e) {
        astg_foreach_out_edge (astg_head(e),gen3,e2) {
        astg_head(e2)->flag0 = ASTG_FALSE;
        }
    }
    }
    return place_simple;
}

astg_bool astg_is_pure (g)
astg_graph *g;
{
    astg_generator gen1, gen2, gen3;
    astg_vertex *p, *p2;
    astg_edge   *e1, *e2;
    astg_vertex *t;
    astg_bool    rc = ASTG_TRUE;

    astg_foreach_place (g,gen1,p) {
    astg_foreach_out_edge (p,gen2,e1) {
        t = astg_head (e1);
        astg_foreach_out_edge (t,gen3,e2) {
        p2 = astg_head (e2);
        if (p == p2) {
            rc = ASTG_FALSE;
            printf("warning: place %s, trans %s not pure.\n",
            astg_v_name(p), astg_v_name(t));
        }
        }
    }
    }
    return rc;
}

/* --------------------------- Simple Cycles ------------------------- */

static int astg_report_cycle (g)
astg_graph *g;
{
    astg_generator gen;
    astg_vertex *v;
    int rc;

    astg_foreach_path_vertex (g,gen,v) {
    v->on_path = ASTG_TRUE;
    }
    rc = (*g->f) (g,g->f_data);
    astg_foreach_path_vertex (g,gen,v) {
    v->on_path = ASTG_FALSE;
    }
    return rc;
}

static int astg_cycles (g,v)
astg_graph  *g;
astg_vertex *v;
{
    astg_generator gen;
    int rc = 0;
    astg_edge *e;

    if (v->active) {
        if (v->unprocessed) {
        g->path_start = v;
        rc = astg_report_cycle (g);
    }
    }
    else if (v->subset) {
        v->active =  ASTG_TRUE;
        astg_foreach_out_edge (v,gen,e) {
            v->alg.sc.trail = e;
            rc += astg_cycles (g,astg_head(e));
        }
        v->active = v->unprocessed = ASTG_FALSE;
    }
    return rc;
}

int astg_simple_cycles (g,target_v,f,f_data,subset)
astg_graph  *g;
astg_vertex *target_v;
int      (*f)();
void	  *f_data;
astg_bool    subset;
{
    int rc = 0;
    astg_bool x;
    astg_vertex *v;
    astg_generator gen;

    /* Initialize the unprocessed flag to ASTG_TRUE if all cycles are
    being reported, or ASTG_FALSE if a specific target is given. */
    x = (target_v == NULL);

    astg_foreach_vertex (g,gen,v) {
    v->active = ASTG_FALSE;
    if (!subset) v->subset = ASTG_TRUE;
    v->unprocessed = x && v->subset;
    v->on_path = ASTG_FALSE;
    v->alg.sc.trail = NULL;
    }

    /* Save callback and callback data */
    g->f = f;   g->f_data = f_data;

    if (target_v != NULL) {
    target_v->unprocessed = ASTG_TRUE;
        rc = astg_cycles (g,target_v);
    }
    else {
        astg_foreach_vertex (g,gen,v) {
            if (v->unprocessed) rc += astg_cycles (g,v);
        }
    }
    return rc;
}

/* --------------------- Topological Ordering --------------------------- */

static void astg_ts_dfs (g,v,num_p)
astg_graph  *g;
astg_vertex *v;
int *num_p;
{
    astg_generator gen;
    astg_edge *e;

    if (v->unprocessed) {
        v->unprocessed = ASTG_FALSE;
    astg_foreach_out_edge (v,gen,e) {
        astg_ts_dfs (g,astg_head(e),num_p);
    }
    v->alg.ts.index = (*num_p)++;
    }
}

array_t *astg_top_sort (g,subset)
astg_graph *g;		/*u graph to sort vertices in a topological order	*/
astg_bool subset;		/*i only process vertices marked with subset flag	*/
{
    astg_generator gen;
    astg_vertex *v;
    int vertex_n;
    array_t *varray;

    astg_foreach_vertex (g,gen,v) {
    v->alg.ts.index = 0;
    if (subset) {
        v->unprocessed = v->subset;
    }
    else {
        v->subset = v->unprocessed = ASTG_TRUE;
    }
    }

    vertex_n = 0;
    astg_foreach_vertex (g,gen,v) {
        if (v->unprocessed) astg_ts_dfs (g,v,&vertex_n);
    }

    assert (vertex_n <= g->n_vertex);
    varray = array_alloc (astg_vertex *, g->n_vertex);
    astg_foreach_vertex (g,gen,v) {
    v->unprocessed = v->subset;
    if (!v->subset) continue;
    array_insert (astg_vertex *, varray, v->alg.ts.index, v);
    }

    return varray;
}

/* -------------------------- Connected Components --------------------- */

static void astg_cc_dfs (v,vcomp,component_n)
astg_vertex *v;
array_t *vcomp;
int component_n;
{
    astg_generator gen;
    astg_edge *e;

    if (v->unprocessed) {
    v->unprocessed = ASTG_FALSE;
    v->alg.cc.comp_num = component_n;
    array_insert_last (astg_vertex *, vcomp, v);
    astg_foreach_in_edge (v,gen,e) {
        astg_cc_dfs (astg_tail(e),vcomp,component_n);
    }
    astg_foreach_out_edge (v,gen,e) {
        astg_cc_dfs (astg_head(e),vcomp,component_n);
    }
    }
}

astg_connected_comp (g,f,f_data,subset)
astg_graph *g;	/*u graph to find components for			*/
int (*f)();	/*i callback: int f(array_t *, int n, void *);		*/
void *f_data;	/*i arbitrary data to pass to callback			*/
astg_bool subset;	/*i only process vertices marked with subset flag	*/
{
    astg_generator gen;
    astg_vertex *v;
    int component_n;
    array_t *vcomp;

    if (!subset) {
    astg_foreach_vertex (g,gen,v) v->unprocessed = ASTG_TRUE;
    }
    component_n = 0;

    astg_foreach_vertex (g,gen,v) {
    if (v->unprocessed) {
        vcomp = array_alloc (astg_vertex *,0);
        astg_cc_dfs (v,vcomp,component_n);
        if (f != NULL) (*f) (vcomp,component_n,f_data);
        component_n++;
        array_free (vcomp);
    }
    }
    return component_n;
}

static void astg_scc_dfs (v,vcomp,component_n)
astg_vertex *v;
array_t *vcomp;
int component_n;
{
    astg_generator gen;
    astg_edge *e;

    if (v->unprocessed) {
    v->unprocessed = ASTG_FALSE;
    /* Add to component array, and number it. */
    v->alg.scc.comp_num = component_n;
    array_insert_last (astg_vertex *, vcomp, v);
    astg_foreach_in_edge (v,gen,e) {
        astg_scc_dfs (astg_tail(e),vcomp,component_n);
    }
    }
}

int astg_strong_comp (g,f,f_data,subset)
astg_graph *g;	/*u find strong components of g				*/
int (*f)();	/*i callback: int f(array_t *, int n, void *);		*/
void *f_data;	/*i arbitrary data to pass to callback			*/
astg_bool subset;	/*i only process vertices marked with subset flag	*/
{
    array_t *varray;
    array_t *vcomp;
    astg_vertex *v;
    int n, component_n;

    varray = astg_top_sort (g,subset);
    /* unprocessed is set appropriately. */
    component_n = 0;

    for (n=array_n(varray); n--; /*empty*/) {
    v = array_fetch (astg_vertex *,varray,n);
    if (v->unprocessed) {
        vcomp = array_alloc (astg_vertex *,0);
        astg_scc_dfs (v,vcomp,component_n);
        if (f != NULL) (*f) (vcomp,component_n,f_data);
        component_n++;
        array_free (vcomp);
    }
    }

    array_free (varray);
    return component_n;
}

/* --------------------------- Cycles -------------------------- */

static int astg_find_cycles (v)
astg_vertex *v;
{
    astg_generator gen;
    astg_edge *e;
    int rc = 0;

    if (v->active) {
        rc = 1;
    }
    else if (v->unprocessed) {
        v->active =  ASTG_TRUE;
        astg_foreach_out_edge (v,gen,e) {
            rc = astg_find_cycles (astg_head(e));
            if (rc != 0) break;
        }
        v->active = v->unprocessed = ASTG_FALSE;
    }
    return rc;
}

int astg_has_cycles (g)
astg_graph *g;
{
    astg_generator gen;
    astg_vertex *v;
    int n_cycles;

    astg_foreach_vertex (g,gen,v) {
    v->active = ASTG_FALSE;
    v->unprocessed = ASTG_TRUE;
    }

    astg_foreach_vertex (g,gen,v) {
        n_cycles = astg_find_cycles (v);
        if (n_cycles != 0) break;
    }

    return (n_cycles != 0);
}

/* --------------------------- Misc. Stuff ------------------------------------ */

static astg_bool astg_cp_dfs (v,dest)
astg_vertex *v;		/*u vertex to continue DFS trace from	*/
astg_vertex *dest;	/*i target vertex for the path		*/
{
    astg_edge *e;
    astg_bool path_found = ASTG_FALSE;
    astg_generator gen;

    if (v->unprocessed) {
    v->unprocessed = ASTG_FALSE;
    astg_foreach_out_edge (v,gen,e) {
        if (!path_found) path_found = astg_cp_dfs (astg_head(e),dest);
    }
    }
    return path_found;
}

astg_bool astg_choose_path (g,start,dest)
astg_graph *g;		/*u find any path from start to dest	*/
astg_vertex *start;	/*u vertex to start path at		*/
astg_vertex *dest;	/*i vertex to try to reach		*/
{
    astg_generator gen;
    astg_vertex *v;

    astg_foreach_vertex (g,gen,v) v->unprocessed = ASTG_TRUE;
    return astg_cp_dfs (start,dest);
}

void astg_print_path (stg,stream)
astg_graph *stg;
FILE *stream;
{
    astg_generator gen;
    astg_vertex *v;

    fprintf (stream,"Path:");
    astg_foreach_path_vertex (stg,gen,v) {
    fprintf(stream," %s",astg_v_name(v));
    }
    fputs("\n",stream);
}

void astg_print (g,stream)
astg_graph *g;
FILE *stream;
{
    astg_generator gen;
    astg_vertex *v;
    astg_edge *e;

    fprintf(stream,"Graph '%s':\n",g->name);
    astg_foreach_vertex (g,gen,v) {
        fprintf(stream,"  %s",astg_v_name(v));
        astg_foreach_out_edge (v,gen,e) {
            fprintf(stream," %s",astg_v_name(astg_head(e)));
        }
        fputs("\n",stream);
    }
}
#endif /* SIS */
