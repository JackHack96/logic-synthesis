/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/extract/greedyrow.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:22 $
 *
 */
#include "sis.h"
#include "extract_int.h"


static int
check_rect_value(A, row_cost, col_cost, rect)
sm_matrix *A;
array_t *row_cost, *col_cost;
rect_t *rect;
{
    register sm_element *p1, *p2, *p3;
    register int value;

    value = 0;
    sm_foreach_col_element(rect->rows, p1) {
	sm_foreach_row_element(rect->cols, p2) {
	    p3 = sm_find(A, p1->row_num, p2->col_num);
	    assert(p3 != NIL(sm_element));
	    value += VALUE(p3);
	}
    }
    sm_foreach_col_element(rect->rows, p1) {
	value -= array_fetch(int, row_cost, p1->row_num);
    }
    sm_foreach_row_element(rect->cols, p1) {
	value -= array_fetch(int, col_cost, p1->col_num);
    }
    return value;
}

static sm_row *
find_max_row_intersection(A, row_cost, rect, col_values)
sm_matrix *A;
array_t *row_cost;
rect_t *rect;
int *col_values;
{
    register sm_element *p, *p1;
    register sm_col *pcol;
    register int *row_values;
    sm_row *best_row, *seed;
    int value, max_value;

    if (A->nrows == 0) return 0;

    seed = rect->cols;
    row_values = ALLOC(int, A->last_row->row_num + 1);

    /*  Set each row 'value' to be its cost */
    sm_foreach_row_element(seed, p) {
	pcol = sm_get_col(A, p->col_num);
	sm_foreach_col_element(pcol, p1) {
	    row_values[p1->row_num] = - array_fetch(int, row_cost, p1->row_num);
	}
    }

    /*  Compute for each row the final value if it is added to rectangle */
    sm_foreach_row_element(seed, p) {
	pcol = sm_get_col(A, p->col_num);
	sm_foreach_col_element(pcol, p1) {
	    row_values[p1->row_num] += VALUE(p1) + col_values[pcol->col_num];
	}
    }


    /*
     *  now find the best (which is not also in rect_rows)
     */
    max_value = 0;
    best_row = NIL(sm_row);
    sm_foreach_row_element(seed, p) {
	pcol = sm_get_col(A, p->col_num);
	sm_foreach_col_element(pcol, p1) {
	    value = row_values[p1->row_num];
	    if (value > max_value) {
		if (! sm_col_find(rect->rows, p1->row_num)) {
		    max_value = value;
		    best_row = sm_get_row(A, p1->row_num);
		}
	    }
	}
    }

    FREE(row_values);
    return best_row;
}

/*
 *  greedy algorithm to find a 'good' rectangle which contains at least
 *  one column of 'seed'
 *
 *  The cost of a rectangle is computed incrementally.
 */


rect_t *
greedy_row(A, row_cost, col_cost, seed)
sm_matrix *A;
array_t *row_cost, *col_cost;
sm_row *seed;
{
    sm_element *p;
    sm_row *save, *prow;
    rect_t *best_rect, *rect;
    int total_row_cost, *col_values;

    best_rect = rect_alloc();
    if (A->nrows == 0) return best_rect;

    /* make 'seed' the initial rectangle */
    rect = rect_alloc();
    sm_foreach_row_element(seed, p) {
	(void) sm_row_insert(rect->cols, p->col_num);
    }
    (void) sm_col_insert(rect->rows, seed->row_num);

    /* Setup the initial value for each column */
    col_values = ALLOC(int, A->last_col->col_num + 1);
    sm_foreach_row_element(seed, p) {
	col_values[p->col_num] = VALUE(p) - 
					array_fetch(int, col_cost, p->col_num);
    }

    /* Setup the initial rectangle value */
    total_row_cost = array_fetch(int, row_cost, seed->row_num);
    rect->value = - total_row_cost;
    sm_foreach_row_element(rect->cols, p) {
	rect->value += col_values[p->col_num];
    }


    /*
     *  loop until the rectangle is only a single column (or until
     *  no rows intersect the current rectangle)
     */

    prow = find_max_row_intersection(A, row_cost, rect, col_values);

    while (prow != 0 && rect->cols->length > 1) {

	if (greedy_row_debug) {
	    (void) printf("greedy_row: rectangle is %d by %d, value=%d\n",
		rect->rows->length, rect->cols->length, rect->value);
	    assert(check_rect_value(A, row_cost, col_cost, rect)==rect->value);
	}

	/* update which rows are in the rectangle */
	(void) sm_col_insert(rect->rows, prow->row_num);

	/* update the cost of the selected rows */
	total_row_cost += array_fetch(int, row_cost, prow->row_num);

	/* update which columns are in the rectangle */
	save = rect->cols;
	rect->cols = sm_row_and(rect->cols, prow);
	sm_row_free(save);

	/* update the 'cost/value' for each column */
	sm_foreach_row_element(prow, p) {
	    col_values[p->col_num] += VALUE(p);
	}

	/* update the rectangle value */
	rect->value = - total_row_cost;
	sm_foreach_row_element(rect->cols, p) {
	    rect->value += col_values[p->col_num];
	}

	/* See if this is the best rectangle so far ... */
	if (rect->value > best_rect->value) {
	    rect_free(best_rect);
	    best_rect = rect_dup(rect);
	}

	/* Find the next best row */
	prow = find_max_row_intersection(A, row_cost, rect, col_values);
    }

    rect_free(rect);

    if (greedy_row_debug) {
	(void) printf("greedy_row: best rectangle is %d by %d value=%d\n",
	    best_rect->rows->length, best_rect->cols->length, best_rect->value);
    }

    FREE(col_values);
    return best_rect;
}
