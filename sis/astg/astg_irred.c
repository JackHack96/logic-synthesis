
/* -------------------------------------------------------------------- *\
   irred.c -- check for and optionally remove redundant constraints.
\* ---------------------------------------------------------------------*/

#ifdef SIS
#include "astg_int.h"
#include "astg_core.h"

static void pr_path (stg,s)
astg_graph *stg;
char *s;
{
    astg_generator gen;
    astg_vertex *v;

    msg("%s:",s);
    astg_foreach_path_vertex (stg,gen,v) {
	msg(" %s",astg_v_name(v));
    }
    msg("\n");
}

typedef struct irred_rec {
    astg_vertex *red_tail;
    astg_vertex *red_head;
    astg_vertex *red_place;
} irred_rec;


static int check_redundant (stg,data)
astg_graph *stg;
void *data;
{
    /* Callback for simple cycles to see if this is an alternate cycle. */
    irred_rec *info = (irred_rec *) data;

    if (info->red_tail->on_path && info->red_head->on_path &&
		!info->red_place->on_path) {
        /* Found another loop through these two. */
	dbg(3,pr_path(stg,"alt"));
        return 1;
    }
    return 0;
}

extern int astg_irred (stg,modify)
astg_graph *stg;
astg_bool modify;
{
    /*	Count the number of redundant arcs in the STG, and if modify is
	true, delete them as well. */

    astg_generator gen;
    int n_redundant = 0;
    astg_place *p;
    irred_rec info;

    astg_foreach_place (stg,gen,p) {
	p->subset = ASTG_TRUE;
    }

    astg_foreach_place (stg,gen,p) {
	if (astg_in_degree(p) != 1 || astg_out_degree(p) != 1) continue;
	info.red_tail  = p->in_edges->tail;
	info.red_head  = p->out_edges->head;
	info.red_place = p;
	if (astg_simple_cycles (stg,info.red_tail,
		check_redundant,(void *)&info,ASTG_SUBSET)) {
	    n_redundant++;
	    dbg(1,msg("  %s -> ", astg_v_name(info.red_tail)));
	    dbg(1,msg("%s is redundant\n", astg_v_name(info.red_head)));
	    p->subset = ASTG_FALSE;
	    if (modify) astg_delete_place (p);
	}
    }

    dbg (1,msg("%s %d redundant constraints\n",modify?"Deleted":"Found",n_redundant));
    return n_redundant;
}

/* -------------------------- Calculate Worst Delay --------------------- */

typedef struct cycle_info {
    astg_bool	 find_longest;
    int		 which_cycle;
    int		 cycle_n;
    array_t	*longest_cycle;
    float	 its_delay;
} cycle_info;

static float astg_calc_delay (stg)
astg_graph *stg;
{
    float rc = 0.0;
    astg_vertex *v;
    astg_generator gen;

    astg_foreach_path_vertex (stg,gen,v) {
	if (v->vtype == ASTG_TRANS) {
	    rc += astg_trx(v)->delay;
	}
    }
    return rc;
}

static int check_cycles (stg,data)
astg_graph *stg;
void *data;
{
    cycle_info *cinfo = (cycle_info *) data;
    astg_generator gen;
    int n_found = 0;
    float cycle_delay;
    astg_vertex *v;

    cinfo->cycle_n++;

    if (cinfo->find_longest) {
	cycle_delay = astg_calc_delay (stg);
	n_found = 1;
	if (cinfo->longest_cycle == NULL || cycle_delay > cinfo->its_delay) {
	    if (cinfo->longest_cycle != NULL) array_free (cinfo->longest_cycle);
	    cinfo->longest_cycle = array_alloc (astg_vertex *,0);
	    astg_foreach_path_vertex (stg,gen,v) {
		array_insert_last (astg_vertex *,cinfo->longest_cycle,v);
	    }
	    cinfo->its_delay = cycle_delay;
	}
    }
    else if (cinfo->cycle_n == cinfo->which_cycle || cinfo->which_cycle <= 0) {
	n_found = 1;
	astg_foreach_path_vertex (stg,gen,v) {
	    astg_sel_vertex (v,ASTG_TRUE);
	}
    }

    return n_found;
}

float astg_longest_cycle (stg)
astg_graph *stg;
{
    cycle_info cinfo_rec;

    cinfo_rec.find_longest = ASTG_TRUE;
    cinfo_rec.longest_cycle = NULL;
    astg_simple_cycles (stg,NIL(astg_vertex),check_cycles,(void *)&cinfo_rec,ASTG_ALL);
    if (cinfo_rec.longest_cycle != NULL) array_free (cinfo_rec.longest_cycle);
    return cinfo_rec.its_delay;
}

int astg_select_cycle (stg,thru_trans,cycle_n,find_longest,add_to_set)
astg_graph *stg;
astg_trans *thru_trans;
int cycle_n;			/*i index, 0=all, of which cycle	*/
astg_bool find_longest;		/*i 1=longest delay cycle, 0=any cycles	*/
astg_bool add_to_set;		/*i 1=add to existing set, 0=new set	*/
{
    cycle_info cinfo_rec;
    char set_name[180];
    astg_vertex *v;
    int n, n_cycle;

    cinfo_rec.find_longest = find_longest;
    cinfo_rec.cycle_n = 0;
    cinfo_rec.which_cycle = cycle_n;

    if (find_longest) {
	cinfo_rec.longest_cycle = NULL;
    }
    else if (cycle_n == 0 && !add_to_set) {
	astg_sel_new (stg,"simple cycles",ASTG_FALSE);
    }
    else if (!add_to_set) {
	(void) sprintf(set_name,"simple cycle %d",cycle_n);
	astg_sel_new (stg,set_name,ASTG_FALSE);
    }

    v = (thru_trans == NULL) ? NULL : thru_trans;

    n_cycle = astg_simple_cycles (stg,v,check_cycles,(void *)&cinfo_rec,ASTG_ALL);

    if (find_longest && cinfo_rec.longest_cycle != NULL) {
	(void) sprintf(set_name,"longest cycle %.1f",cinfo_rec.its_delay);
	astg_sel_new (stg,set_name,ASTG_FALSE);
	for (n=0; n < array_n(cinfo_rec.longest_cycle); n++) {
	    v = array_fetch (astg_vertex *,cinfo_rec.longest_cycle,n);
	    astg_sel_vertex (v,ASTG_TRUE);
	}
	array_free (cinfo_rec.longest_cycle);
    }

    dbg(1,msg("Selected %d %s.\n",n_cycle,n_cycle==1?"cycle":"cycles"));
    astg_sel_show (stg);
}
#endif /* SIS */
