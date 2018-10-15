/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/simplify/dc_filter.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:49 $
 *
 */
#include "sis.h"
#include "simp_int.h"

static void fobsdc_sm_bp_1();
void node_d2merge();
static void filter_exact();
static void filter_disj_support();
static void filter_size();
static void filter_fdist();
static void filter_sdist();

/* filter the dont care set according to the filter type indicated 
 */
node_t *
simp_dc_filter(f, dc, filter)
node_t *f,*dc;
sim_filter_t filter;
{

    sm_matrix *Mfdc;
    node_t *new_dc;
    
    /* constant functions ignored for filtering */
    if (node_num_fanin(dc) == 0) {
        return(dc);
    }

    (void) node_d1merge(dc);

    /* create sparse matrix for function and its dont care */
    Mfdc = simp_node_to_sm(f, dc);

    if (simp_debug) {
        simp_sm_print(Mfdc);
    }

    switch (filter) {
    case SIM_FILTER_EXACT :
        (void) filter_exact(Mfdc);
        break;
    case SIM_FILTER_DISJ_SUPPORT :
        (void) filter_exact(Mfdc);
        (void) filter_disj_support(Mfdc);
        break;
    case SIM_FILTER_SIZE :
        (void) filter_exact(Mfdc);
        (void) filter_size(Mfdc);
        break;
    case SIM_FILTER_FDIST :
	(void) filter_exact(Mfdc);
	(void) filter_fdist(Mfdc, DIST_BOUND);
        break;
    case SIM_FILTER_SDIST :
	(void) filter_exact(Mfdc);
	(void) filter_sdist(Mfdc, DIST_BOUND);
        break;
    case SIM_FILTER_ALL :
        (void) filter_exact(Mfdc);
        (void) filter_disj_support(Mfdc);
        (void) filter_size(Mfdc);
        break;
    case SIM_FILTER_NONE:
        break;
    default:
        fail("Wrong filter type");
        exit(-1);
    }

    if (simp_debug) {
        simp_sm_print(Mfdc);
    }

    new_dc = simp_sm_to_node(Mfdc);
    node_free(dc);
    sm_free(Mfdc);

    return(new_dc);
}

/* attempt to find a partition (iteratively) of the form :
 *
 *     xy-
 *     -yz
 *
 * where y is a single variable.
 */
static void
filter_exact(Mfdc)
sm_matrix *Mfdc;
{
    array_t *chosen;
    sm_col *col;

    chosen = sm_col_count_init(sm_ncols(Mfdc));
    while ((col = sm_get_long_col(Mfdc, chosen)) != NULL) {
        (void) fdc_sm_bp_1(Mfdc, col);
    }
}

static void
filter_disj_support(Mfdc)
sm_matrix *Mfdc;
{
    (void) fdc_sm_bp_2(Mfdc);
}

static void
filter_size(Mfdc)
sm_matrix *Mfdc;
{
    (void) fdc_sm_bp_4(Mfdc);
}

static void
filter_fdist(Mfdc, distance)
sm_matrix *Mfdc;
int distance;
{
    (void) fdc_sm_bp_6(Mfdc, distance);
}

static void
filter_sdist(Mfdc, distance)
sm_matrix *Mfdc;
int distance;
{
    (void) fdc_sm_bp_7(Mfdc, distance);
}

/* removes the cubes in the observability don't care set that are unlikely
 * to be used for simplification.
 */
node_t *
simp_obsdc_filter(dc, node_table, var, nodepi)
node_t *dc; 
st_table *node_table; /* a list of variables allowed in each cube. */
int	var;          /* indicates how many variables that are not in the 
		       * table are allowed in each cube. */
node_t *nodepi;
{

    sm_matrix *Mfdc;
    node_t *new_dc;
    node_t *n1, *n2, *n3;
    st_generator *gen;
    char *dummy;
    node_t *np;

    /* if nothing to filter. */
    if (node_num_fanin(dc) == 0) {
        return(dc);
    }

    /* AND all the nodes in the table and all Primary Inputs. */
    n1= node_constant(1);
    st_foreach_item(node_table, gen, &dummy, NIL(char *)) {
    np = (node_t *) dummy;
    	n3= node_literal(np,1);
    	n2= node_and(n3, n1);
    	node_free(n1);
    	node_free(n3);
    	n1= n2;
    }
    n3= node_literal(nodepi,1);
    n2= node_and(n3, n1);
    node_free(n1);
    node_free(n3);
    n1= n2;


    node_d2merge(dc);
    if (node_num_fanin(dc) == 0) {
        return(dc);
    }


    /* create sparse matrix for the AND function and its dont care */
    Mfdc = simp_node_to_sm(n1, dc);

    fobsdc_sm_bp_1(Mfdc, var);

    new_dc = simp_sm_to_node(Mfdc);

    node_free(dc);
    node_free(n1);
    sm_free(Mfdc);
    return(new_dc);
}


/* remove any cube from observability don't care set that has more than
 * "var" literals that are not in any cubes of the onset function.
 */
static void
fobsdc_sm_bp_1(M, var)
sm_matrix *M;
int    var;
{
    sm_row *prow;
    sm_col *pcol;
    sm_element *p;
    int i, init_rows;

    if (M->nrows == 0)
        return;

    /* mark every row and column as unvisited */
    for (prow = M->first_row; prow != NULL; prow = prow->next_row)
        prow->flag = 0;
    for (pcol = M->first_col; pcol != NULL; pcol = pcol->next_col)
        pcol->flag = 0;
    
    /* for each row in the on set mark the columns in them as visited */
    for (prow = M->first_row; prow != NULL; prow = prow->next_row) {
        if (((int) prow->user_word) == F_SET) {
            prow->flag = 1;
            for (p = prow->first_col; p != NULL; p = p->next_col) {
                pcol = sm_get_col(M, p->col_num);
                pcol->flag = 1;
            }
        }
    }
    
    for (prow = M->first_row; prow != NULL; prow = prow->next_row) {
        if (!(((int) prow->user_word) == F_SET)) {
            for (p = prow->first_col; p != NULL; p = p->next_col) {
                pcol = sm_get_col(M, p->col_num);
    			if (!pcol->flag)
    				prow->flag++;
            }
        }
    }


    /* any unmarked rows can now be deleted => inessential dont cares */
    init_rows = M->rows_size;
    if (simp_debug)
        (void) fprintf(sisout, "rows deleted : ");
    for (i = 0; i < init_rows; i ++) {
        prow = sm_get_row(M, i);
    	if (prow != NULL){
        if (!(((int) prow->user_word) == F_SET)) {
            if ( prow->flag > var) {
               	 sm_delrow(M, i);
            }
    	}
    	}
    }
}

node_t *
simp_obssatdc_filter(dc, f, var)
node_t *dc; /*obs. don't care to be filtered. */
node_t *f;
int    var;
{
    sm_matrix *Mfdc;
    node_t *new_dc;

    /* if nothing to filter. */
    if (node_num_fanin(dc) == 0) {
        return(dc);
    }


    /* create sparse matrix for the AND function and its dont care */
    Mfdc = simp_node_to_sm(f, dc);

    fobsdc_sm_bp_1(Mfdc, var);

    new_dc = simp_sm_to_node(Mfdc);

    node_free(dc);
    sm_free(Mfdc);
    return(new_dc);
}


/* repeated distance 1 merge 
 *
 */
void node_d2merge(f)
node_t *f;
{

    int nin, i, lit_count;
    int old_litcount;

    nin = node_num_fanin(f);
    define_cube_size(nin);

    old_litcount = node_num_literal(f);
    do {
    lit_count = node_num_literal(f);
    for (i=0; i< nin; i++){
    	f->F = d1merge(f->F, i);
    	}
    } while (lit_count > node_num_literal(f));

    if (old_litcount > node_num_literal(f))
    	node_scc(f);

    f->is_dup_free = 1;
    f->is_scc_minimal = 1;
    node_minimum_base(f);
}
