
#include "sis.h"
#include "extract_int.h"

#define PING_MAXINT	(1 << 30)


static int
row_value(prow, row_cost)
sm_row *prow;
array_t *row_cost;
{
    register sm_element *p;
    register int value;

    value = - array_fetch(int, row_cost, prow->row_num);
    sm_foreach_row_element(prow, p) {
	value = value + VALUE(p);
    }
    return value;
}


static sm_row *
choose_row_seed(A, row_cost)
sm_matrix *A;
array_t *row_cost;
{
    register int value, max_value;
    sm_row *prow, *best_row;

    /* choose row with maximum value */
    max_value = - PING_MAXINT;
    best_row = NIL(sm_row);
    sm_foreach_row(A, prow) {
	value = row_value(prow, row_cost);
	if (value > max_value) {
	    max_value = value;
	    best_row = prow;
	}
    }
    return best_row;
}


static sm_row *
choose_next_row_seed(A, row_cost, rect)
sm_matrix *A;
array_t *row_cost;
rect_t *rect;
{
    register int value, max_value;
    register sm_element *p;
    sm_row *prow, *best_row;

    /* choose row (in rect->rows) with maximum value */
    max_value = - PING_MAXINT;
    best_row = NIL(sm_row);
    sm_foreach_col_element(rect->rows, p) {
	prow = sm_get_row(A, p->row_num);
	value = row_value(prow, row_cost);
	if (value > max_value) {
	    max_value = value;
	    best_row = prow;
	}
    }
    return best_row;
}

static int
col_value(pcol, col_cost)
sm_col *pcol;
array_t *col_cost;
{
    register sm_element *p;
    register int value;

    value = - array_fetch(int, col_cost, pcol->col_num);
    sm_foreach_col_element(pcol, p) {
	value = value + VALUE(p);
    }
    return value;
}


static sm_col *
choose_col_seed(A, col_cost)
sm_matrix *A;
array_t *col_cost;
{
    register int value, max_value;
    sm_col *pcol, *best_col;

    /* choose col with maximum value */
    max_value = - PING_MAXINT;
    best_col = NIL(sm_col);
    sm_foreach_col(A, pcol) {
	value = col_value(pcol, col_cost);
	if (value > max_value) {
	    max_value = value;
	    best_col = pcol;
	}
    }
    return best_col;
}


static sm_col *
choose_next_col_seed(A, col_cost, rect)
sm_matrix *A;
array_t *col_cost;
rect_t *rect;
{
    register int value, max_value;
    register sm_element *p;
    sm_col *pcol, *best_col;

    /* choose col (in rect->cols) with maximum value */
    max_value = - PING_MAXINT;
    best_col = NIL(sm_col);
    sm_foreach_row_element(rect->cols, p) {
	pcol = sm_get_col(A, p->col_num);
	value = col_value(pcol, col_cost);
	if (value > max_value) {
	    max_value = value;
	    best_col = pcol;
	}
    }
    return best_col;
}

static int
update_best(rect1, rect2, best_rect)
rect_t *rect1, *rect2, **best_rect;
{
    int got_new_best;

    got_new_best = 0;
    if (rect1->value >= rect2->value) {
	rect_free(rect2);
	if (rect1->value > (*best_rect)->value) {
	    rect_free(*best_rect);
	    *best_rect = rect1;
	} else {
	    rect_free(rect1);
	}
    } else {
	/* rectangle 2 beats rectangle 1 -- good news */
	rect_free(rect1);
	if (rect2->value > (*best_rect)->value) {
	    rect_free(*best_rect);
	    *best_rect = rect2;
	    got_new_best = 1;
	} else {
	    rect_free(rect2);
	}
    }
    return got_new_best;
}

static rect_t *
ping_pong_improve(A, row_cost, col_cost, best_rect)
sm_matrix *A;
array_t *row_cost, *col_cost;
rect_t *best_rect;
{
    rect_t *rect1, *rect2;
    sm_row *best_row;
    sm_col *best_col;
    int got_new_best;

    rect2 = best_rect;
    do {
	best_row = choose_next_row_seed(A, row_cost, rect2);
	if (best_row == NIL(sm_row)) return best_rect;
	rect1 = greedy_row(A, row_cost, col_cost, best_row);
	if (ping_pong_debug) {
	    (void) printf("ping_pong:    ... pinging rows %d by %d value=%d\n",
		    rect1->rows->length, rect1->cols->length, rect1->value);
	}
	if (rect1->value < 1) {
	    rect_free(rect1);
	    break;
	}

	best_col = choose_next_col_seed(A, col_cost, rect1);
	if (best_col == NIL(sm_col)) return best_rect;
	rect2 = greedy_col(A, col_cost, row_cost, best_col);
	if (ping_pong_debug) {
	    (void) printf("ping_pong:    ... pinging cols %d by %d value=%d\n",
		    rect2->rows->length, rect2->cols->length, rect2->value);
	}

	got_new_best = update_best(rect1, rect2, &best_rect);
    } while (got_new_best);

    return best_rect;
}


static rect_t *
ping_pong_onepass(A, row_cost, col_cost)
sm_matrix *A;
array_t *row_cost, *col_cost;
{
    sm_row *row_seed;
    sm_col *col_seed;
    rect_t *rect1, *rect2;

    row_seed = choose_row_seed(A, row_cost);
    if (row_seed == NIL(sm_row)) return rect_alloc();
    rect1 = greedy_row(A, row_cost, col_cost, row_seed);
    if (ping_pong_debug) {
	(void) printf("ping_pong: row pass gives %d by %d value=%d\n",
			rect1->rows->length, rect1->cols->length, rect1->value);
    }

    col_seed = choose_col_seed(A, col_cost);
    if (col_seed == NIL(sm_col)) return rect1;
    rect2 = greedy_col(A, col_cost, row_cost, col_seed);
    if (ping_pong_debug) {
	(void) printf("ping_pong: col pass gives %d by %d value=%d\n",
			rect2->rows->length, rect2->cols->length, rect2->value);
    }

    if (rect1->value > rect2->value) {
	rect_free(rect2);
	return rect1;
    } else {
	rect_free(rect1);
	return rect2;
    }
}


rect_t *
ping_pong(A, row_cost, col_cost)
sm_matrix *A;
array_t *row_cost, *col_cost;
{
    rect_t *best_rect;

    best_rect = ping_pong_onepass(A, row_cost, col_cost);
    if (best_rect->rows->length > 0 && best_rect->cols->length > 0) {
	best_rect = ping_pong_improve(A, row_cost, col_cost, best_rect);
    }
    return best_rect;
}
