/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/simplify/filter_util.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:49 $
 *
 */
#include "sis.h"
#include "simp_int.h"

static void bd_comp_exact();

/* Three flags are used to  mark rows and columns 
 * 0 - indicates not visited
 * 1 - indicates visited and to be considered
 * 2 - indicates visited but not to be considered
 */

/* 
 * find a block decomposition of a sparse matrix - suitable only for
 * dont care filtering (type 1). second parameter indicates which column to
 * ignore in the decoposition - use NULL to supress.
 */
void
fdc_sm_bp_1(M, ex_col)
sm_matrix *M;
sm_col *ex_col;
{
    sm_row *prow;
    sm_col *pcol;
    int i, init_rows;

    if (M->nrows == 0)
        return;

    /* mark dc set rows and all columns as unvisited */
    for (prow = M->first_row; prow != NULL; prow = prow->next_row)
	if (((int) prow->user_word) == F_SET)
	    prow->flag = 1;
	else
            prow->flag = 0;
    for (pcol = M->first_col; pcol != NULL; pcol = pcol->next_col)
        pcol->flag = 0;

    /* call block decomposition recursively */
    (void) bd_comp_exact(M, ex_col);
    
    /* any unmarked rows can now be deleted => inessential dont cares */
    init_rows = M->rows_size;
    if (simp_debug) {
        (void) fprintf(sisout, "excluded column is %d\n", ex_col->col_num);
        (void) fprintf(sisout, "rows deleted : ");
    }
    for (i = 0; i < init_rows; i ++) {
        prow = sm_get_row(M, i);
        if ((prow != NULL) && (prow->flag == 0)) {
            if (simp_debug) {
                (void) fprintf(sisout, "%d ", prow->row_num);
            }
            sm_delrow(M, i);
        }
    }
    if (simp_debug)
	(void) fprintf(sisout, "\n");
}

/* attempt to partition the sparse matrix for the exact filter recursively */
static void
bd_comp_exact(M, ex_col)
sm_matrix *M;
sm_col *ex_col;
{

    sm_row *prow;
    sm_col *pcol;
    sm_element *p;
    bool new_found;

    new_found = FALSE;

    /* for all marked rows mark columns as visited */
    for (prow = M->first_row; prow != NULL; prow = prow->next_row) {
        if (prow->flag == 1) {
            prow->flag = 2;
            for (p = prow->first_col; p != NULL; p = p->next_col) {
                pcol = sm_get_col(M, p->col_num);
                if ((pcol != ex_col) && (pcol->flag == 0)) {
		    new_found = TRUE;
		    pcol->flag = 1;
		}
            }
        }
    }

    if (!new_found)
	return; /* no more to do */

    new_found = FALSE;

    /* for each column marked as visited mark the rows in them as visited */
    for (pcol = M->first_col; pcol != NULL; pcol = pcol->next_col) {
	if ((pcol != ex_col) && (pcol->flag == 1)) {
	    pcol->flag = 2;
            for (p = pcol->first_row; p != NULL; p = p->next_row) {
                prow = sm_get_row(M, p->row_num);
                if (prow->flag == 0) {
		    new_found = TRUE;
		    prow->flag = 1;
		}
            }
        }
    }

    if (!new_found)
	return; /* no more to do */
    else
	(void) bd_comp_exact(M, ex_col); /* recurse */
}

/* 
 * heuristic filter - remove the dont care cubes that do not share any 
 * variables with the on set. this is an approximation and may delete 
 * dont cares that are useful in simplification. 
 */
void
fdc_sm_bp_2(M)
sm_matrix *M;
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
    
    /* for each column visited mark the rows in them as visited */
    for (pcol = M->first_col; pcol != NULL; pcol = pcol->next_col) {
        if (pcol->flag) {
            for (p = pcol->first_row; p != NULL; p = p->next_row) {
                prow = sm_get_row(M, p->row_num);
                prow->flag = 1;
            }
        }
    }

    /* any unmarked rows can now be deleted => inessential dont cares */
    init_rows = M->rows_size;
    if (simp_debug)
        (void) fprintf(sisout, "rows deleted : ");
    for (i = 0; i < init_rows; i ++) {
        prow = sm_get_row(M, i);
        if ((prow != NULL) && (! prow->flag)) {
            if (simp_debug)
                (void) fprintf(sisout, "%d ", i);
            sm_delrow(M, i);
        }
    }
    if (simp_debug)
        (void) fprintf(sisout, "\n");
}

/* 
 * heuristic filter - remove the smaller dont care cubes that have some part
 * of their support outside the on set. this is an approximation and may delete 
 * dont cares that are useful in simplification. 
 */
void
fdc_sm_bp_4(M)
sm_matrix *M;
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
            for (p = prow->first_col; p != NULL; p = p->next_col) {
                pcol = sm_get_col(M, p->col_num);
                pcol->flag = 1;
            }
        }
    }
    
    /* for each column unvisited, mark the rows in them as visited */
    for (pcol = M->first_col; pcol != NULL; pcol = pcol->next_col) {
        if (!pcol->flag) {
            for (p = pcol->first_row; p != NULL; p = p->next_row) {
                prow = sm_get_row(M, p->row_num);
                prow->flag = 1;
            }
        }
    }

    /* unmark any rows with large dont care cubes */
    for (prow = M->first_row; prow != NULL; prow = prow->next_row)
        if (prow->length <= SIZE_BOUND)
            prow->flag = 0;

    /* any marked rows can now be deleted => inessential dont cares */
    init_rows = M->rows_size;
    if (simp_debug)
        (void) fprintf(sisout, "rows deleted : ");
    for (i = 0; i < init_rows; i ++) {
        prow = sm_get_row(M, i);
        if ((prow != NULL) && (prow->flag)) {
            if (simp_debug)
                (void) fprintf(sisout, "%d ", i);
            sm_delrow(M, i);
        }
    }
    if (simp_debug)
        (void) fprintf(sisout, "\n");
}

/* 
 * heuristic filter - remove the dont care cubes that have more than
 * a certain distance from any cube in the on set
 * this is an approximation and may delete dont cares that are useful 
 * in simplification. 
 */
void
fdc_sm_bp_6(M, distance)
sm_matrix *M;
int distance;
{
    sm_row *prow;
    sm_element *p;
    sm_col *pcol;
    int i, init_rows, cdist;

    if (M->nrows == 0)
        return;

    /* mark every row of dc-set as unvisited. 
     * mark on set columns, unmark others */
    for (pcol = M->first_col; pcol != NULL; pcol = pcol->next_col)
        pcol->flag = 0;
    for (prow = M->first_row; prow != NULL; prow = prow->next_row) {
	if ((int) (prow->user_word) == F_SET) {
	    prow->flag = 1;
            for (p = prow->first_col; p != NULL; p = p->next_col) {
                pcol = sm_get_col(M, p->col_num);
                pcol->flag = 1;
            }
	}
	else {
            prow->flag = 0;
	}
    }

    /* for each dc set cube, mark as visited those cubes less than certain
     * distance (only support outside on set is considered */
    for (prow = M->first_row; prow != NULL; prow = prow->next_row) {
	if (((int) prow->user_word) == DC_SET) {
	    cdist = 0;
	    for (p = prow->first_col; p != NULL; p = p->next_col) {
		pcol = sm_get_col(M, p->col_num);
		if ((!pcol->flag) && (sm_get(int, p) != 2))
		    cdist ++;
		if (cdist > distance)
		    break;
	    }
	    if (cdist <= distance)
	        prow->flag = 1; /* less than distance bound */
	}
    }

    /* any unmarked rows can now be deleted => inessential dont cares */
    init_rows = M->rows_size;
    for (i = 0; i < init_rows; i ++) {
        prow = sm_get_row(M, i);
        if ((prow != NULL) && (!prow->flag)) {
            sm_delrow(M, i);
        }
    }
}

/* 
 * heuristic filter - remove the dont care cubes that have more than
 * a certain distance from any cube in the on set
 * this is an approximation and may delete dont cares that are useful 
 * in simplification. 
 */
void
fdc_sm_bp_7(M, distance)
sm_matrix *M;
int distance;
{
    sm_row *prow, *pr;
    sm_element *p;
    sm_col *pcol;
    int i, init_rows;

    if (M->nrows == 0)
        return;

    /* mark every row of dc-set as unvisited. 
       mark on set columns, unmark others */
    for (pcol = M->first_col; pcol != NULL; pcol = pcol->next_col)
        pcol->flag = 0;
    for (prow = M->first_row; prow != NULL; prow = prow->next_row) {
	if ((int) (prow->user_word) == F_SET) {
	    prow->flag = 1;
            for (p = prow->first_col; p != NULL; p = p->next_col) {
                pcol = sm_get_col(M, p->col_num);
                pcol->flag = 1;
            }
	}
	else {
            prow->flag = 0;
	}
    }

    /* for each on set cube mark all dont care cubes less than max_distance
     * as visited */
    for (prow = M->first_row; prow != NULL; prow = prow->next_row) {
	if (((int) prow->user_word) == F_SET) {
            for (pr= M->first_row; pr!= NULL; pr= pr->next_row) {
		if (pr->flag)
		    continue;
	        if (((int) pr->user_word) != F_SET) {
		    if (dist_check(M, prow, pr, distance))
			pr->flag = 1;
		}
            }
	}
    }

    /* any unmarked rows can now be deleted => inessential dont cares */
    init_rows = M->rows_size;
    for (i = 0; i < init_rows; i ++) {
        prow = sm_get_row(M, i);
        if ((prow != NULL) && (!prow->flag)) {
            sm_delrow(M, i);
        }
    }
}

array_t *
sm_col_count_init(size)
int size;
{

    array_t *chosen;
    int i;

    chosen = array_alloc(bool, size);
    for (i = 0; i < size; i ++)
        (void) array_insert(bool, chosen, i, FALSE);
    return(chosen);
}
    
sm_col *
sm_get_long_col(M, chosen)
sm_matrix *M;
array_t *chosen;
{

    sm_col *col, *max;

    /* find the longest column not already chosen */
    max = NULL;
    sm_foreach_col(M, col) {
        if (array_fetch(bool, chosen, col->col_num) == TRUE)
            continue;
        else {
            if ((max == NULL) || (col->length > max->length))
                max = col;
        }
    }

    if (max == NULL) {
        array_free(chosen);
        return(NULL);
    }
    else {
        array_insert(bool, chosen, max->col_num, TRUE);
        return(max);
    }
}

/* return true if distance between "p" and "r" is not greater than "distance"
 * distance is measured a little differently than the standard definition
 * (see espresso book for example). for the on set support distance is the
 * same as the usual definition. for the variables outside the on set 
 * support, if the on set has a 2 and the dc set cube does not then
 * distance is 1. of course if the on set cube has a 0 (1) and the dc set 
 * cube has a 1 (0) the distance is 1. 
 * "p" is an on set cube
 * "r" is a dc set cube */
bool
dist_check(M, p, r, distance)
sm_matrix *M;
sm_row *p, *r;
int distance;
{

    sm_element *pele, *rele;
    sm_col *pcol;
    int cdist = 0;
    int pval, rval;

    pele = p->first_col;
    rele = r->first_col;
    while ((pele != NULL) && (rele != NULL)) {
	if (pele->col_num < rele->col_num)
	    pele = pele->next_col;
	else if (pele->col_num > rele->col_num)  {
	    pcol = sm_get_col(M, rele->col_num);
	    if (!pcol->flag && (sm_get(int, rele) != 2))
		cdist ++;
     	    rele = rele->next_col;
	}
	else if (pele->col_num == rele->col_num) {
            pcol = sm_get_col(M, pele->col_num);
	    pval = sm_get(int, pele);
	    rval = sm_get(int, rele);
	    if (pcol->flag == 1) {
		if ((pval != 2) && (rval != 2) && (pval != rval)) 
		    cdist ++;
	    }
	    else if ((rval != 2) && (pval != rval))
		cdist ++;
	    pele = pele->next_col;
	    rele = rele->next_col;
	}
	if (cdist > distance)
	    return(FALSE);
    }

    /* distance for variables outside support of on set cube */
    while (rele != NULL) {
	cdist ++;
	if (cdist > distance)
	    return(FALSE);
        rele = rele->next_col;
    }

    return(TRUE);
}
