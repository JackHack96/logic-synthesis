
#include "sis.h"
#include "../include/extract_int.h"


static int
check_rect_value(A, col_cost, row_cost, rect)
        sm_matrix *A;
        array_t *col_cost, *row_cost;
        rect_t *rect;
{
    register sm_element *p1, *p2, *p3;
    register int        value;

    value = 0;
    sm_foreach_row_element(rect->cols, p1)
    {
        sm_foreach_col_element(rect->rows, p2)
        {
            p3 = sm_find(A, p2->row_num, p1->col_num);
            assert(p3 != NIL(sm_element));
            value += VALUE(p3);
        }
    }
    sm_foreach_row_element(rect->cols, p1)
    {
        value -= array_fetch(
        int, col_cost, p1->col_num);
    }
    sm_foreach_col_element(rect->rows, p1)
    {
        value -= array_fetch(
        int, row_cost, p1->row_num);
    }
    return value;
}

static sm_col *
find_max_col_intersection(A, col_cost, rect, row_values)
        sm_matrix *A;
        array_t *col_cost;
        rect_t *rect;
        int *row_values;
{
    register sm_element *p, *p1;
    register sm_row     *prow;
    register int        *col_values;
    sm_col              *best_col, *seed;
    int                 value, max_value;

    if (A->ncols == 0) return 0;

    seed       = rect->rows;
    col_values = ALLOC(
    int, A->last_col->col_num + 1);

    /*  Set each col 'value' to be its cost */
    sm_foreach_col_element(seed, p)
    {
        prow = sm_get_row(A, p->row_num);
        sm_foreach_row_element(prow, p1)
        {
            col_values[p1->col_num] = -array_fetch(
            int, col_cost, p1->col_num);
        }
    }

    /*  Compute for each col the final value if it is added to rectangle */
    sm_foreach_col_element(seed, p)
    {
        prow = sm_get_row(A, p->row_num);
        sm_foreach_row_element(prow, p1)
        {
            col_values[p1->col_num] += VALUE(p1) + row_values[prow->row_num];
        }
    }


    /*
     *  now find the best (which is not also in rect_cols)
     */
    max_value = 0;
    best_col  = NIL(sm_col);
    sm_foreach_col_element(seed, p)
    {
        prow = sm_get_row(A, p->row_num);
        sm_foreach_row_element(prow, p1)
        {
            value = col_values[p1->col_num];
            if (value > max_value) {
                if (!sm_row_find(rect->cols, p1->col_num)) {
                    max_value = value;
                    best_col  = sm_get_col(A, p1->col_num);
                }
            }
        }
    }

    FREE(col_values);
    return best_col;
}

/*
 *  greedy algorithm to find a 'good' rectangle which contains at least
 *  one rowumn of 'seed'
 *
 *  The cost of a rectangle is computed incrementally.
 */


rect_t *
greedy_col(A, col_cost, row_cost, seed)
        sm_matrix *A;
        array_t *col_cost, *row_cost;
        sm_col *seed;
{
    sm_element *p;
    sm_col     *save, *pcol;
    rect_t     *best_rect, *rect;
    int        total_col_cost, *row_values;

    best_rect = rect_alloc();
    if (A->ncols == 0) return best_rect;

    /* make 'seed' the initial rectangle */
    rect = rect_alloc();
    sm_foreach_col_element(seed, p)
    {
        (void) sm_col_insert(rect->rows, p->row_num);
    }
    (void) sm_row_insert(rect->cols, seed->col_num);

    /* Setup the initial value for each rowumn */
    row_values = ALLOC(
    int, A->last_row->row_num + 1);
    sm_foreach_col_element(seed, p)
    {
        row_values[p->row_num] = VALUE(p) -
                                 array_fetch(
        int, row_cost, p->row_num);
    }

    /* Setup the initial rectangle value */
    total_col_cost = array_fetch(
    int, col_cost, seed->col_num);
    rect->value = -total_col_cost;
    sm_foreach_col_element(rect->rows, p)
    {
        rect->value += row_values[p->row_num];
    }


    /*
     *  loop until the rectangle is only a single rowumn (or until
     *  no cols intersect the current rectangle)
     */

    pcol = find_max_col_intersection(A, col_cost, rect, row_values);

    while (pcol != 0 && rect->rows->length > 1) {

        if (greedy_col_debug) {
            (void) printf("greedy_col: rectangle is %d by %d, value=%d\n",
                          rect->cols->length, rect->rows->length, rect->value);
            assert(check_rect_value(A, col_cost, row_cost, rect) == rect->value);
        }

        /* update which cols are in the rectangle */
        (void) sm_row_insert(rect->cols, pcol->col_num);

        /* update the cost of the selected cols */
        total_col_cost += array_fetch(
        int, col_cost, pcol->col_num);

        /* update which rowumns are in the rectangle */
        save = rect->rows;
        rect->rows = sm_col_and(rect->rows, pcol);
        sm_col_free(save);

        /* update the 'cost/value' for each rowumn */
        sm_foreach_col_element(pcol, p)
        {
            row_values[p->row_num] += VALUE(p);
        }

        /* update the rectangle value */
        rect->value = -total_col_cost;
        sm_foreach_col_element(rect->rows, p)
        {
            rect->value += row_values[p->row_num];
        }

        /* See if this is the best rectangle so far ... */
        if (rect->value > best_rect->value) {
            rect_free(best_rect);
            best_rect = rect_dup(rect);
        }

        /* Find the next best col */
        pcol = find_max_col_intersection(A, col_cost, rect, row_values);
    }

    rect_free(rect);

    if (greedy_col_debug) {
        (void) printf("greedy_col: best rectangle is %d by %d value=%d\n",
                      best_rect->cols->length, best_rect->rows->length, best_rect->value);
    }

    FREE(row_values);
    return best_rect;
}
