
#include "sis.h"
#include "../include/extract_int.h"


typedef struct best_subkernel_struct best_subkernel_state_t;
struct best_subkernel_struct {
    rect_t    *best_rect;
    array_t   *row_cost;
    array_t   *col_cost;
    sm_matrix *kernel_cube_matrix;
    int       first_rect;
};

/*
 *  Helper function to sort through all prime rectangles in the kernel-
 *  cube matrix and return the rectangle with the best cost, where the cost
 *  is measured as literals in the SOP form for each function
 *
 *  We compute literals in SOP form by counting the 'value' of each cube
 *  which would be covered by this kernel.  The value reflects the
 *  number of literals in the cube.  Note that some of the values may
 *  be 0 (indicating they have already been covered by another rectangle).
 */


/* ARGSUSED */
static int
best_subkernel_sop(co_rect, rect, state_p)
        sm_matrix *co_rect;
        register rect_t *rect;
        char *state_p;
{
    register int           ks, cks;
    register sm_element    *p, *p1, *p2;
    value_cell_t           *value_cell;
    best_subkernel_state_t *state;
    int                    ncks;

    /* fetch the state */
    state = (best_subkernel_state_t *) state_p;

    /* ignore empty rectangles */
    if (rect->rows->length == 0 || rect->cols->length == 0) {
        rect->value = 0;
        return 1;
    }

    /* count # literals in the co-kernel cubes */
    cks  = 0;
    sm_foreach_col_element(rect->rows, p)
    {
        cks += array_fetch(
        int, state->row_cost, p->row_num) -1;
    }
    ncks = rect->rows->length;

    /* count # literals in the kernel cubes */
    ks = 0;
    sm_foreach_row_element(rect->cols, p)
    {
        ks += array_fetch(
        int, state->col_cost, p->col_num);
    }

    /* compute the real rectangle value */
    rect->value = 0;
    sm_foreach_col_element(rect->rows, p1)
    {
        sm_foreach_row_element(rect->cols, p2)
        {
            p = sm_find(state->kernel_cube_matrix, p1->row_num, p2->col_num);
            assert(p != 0 && p->user_word != 0);
            value_cell = (value_cell_t *) p->user_word;
            rect->value += value_cell->value;
        }
    }
    rect->value -= ks + cks + ncks;

    /* first rectangle becomes the best, otherwise must beat previous */
    if (state->first_rect || rect->value > state->best_rect->value) {
        rect_free(state->best_rect);
        state->best_rect  = rect_dup(rect);
        state->first_rect = 0;
    }

    return 1;
}

/*
 *  find the BEST value rectangle in the kernel-cube matrix by generating
 *  all prime rectangles and choosing the best
 *
 *  cost function is literals in factored form (approx.)
 */


/* ARGSUSED */
static int
best_subkernel_factor(co_rect, rect, state_p)
        sm_matrix *co_rect;
        register rect_t *rect;
        char *state_p;
{
    register sm_element    *p, *p1, *p2;
    st_table               *fanout;
    value_cell_t           *value_cell;
    best_subkernel_state_t *state;
    char                   *key;
    int                    ks;

    state = (best_subkernel_state_t *) state_p;

    /* ignore empty rectangles */
    if (rect->rows->length == 0 || rect->cols->length == 0) {
        rect->value = 0;
        return 1;
    }

    /* count # literals in the kernel cubes */
    ks = 0;
    sm_foreach_row_element(rect->cols, p)
    {
        ks += array_fetch(
        int, state->col_cost, p->col_num);
    }

    /* find all unique functions it feeds */
    fanout = st_init_table(st_numcmp, st_numhash);
    sm_foreach_col_element(rect->rows, p1)
    {
        sm_foreach_row_element(rect->cols, p2)
        {
            p          = sm_find(state->kernel_cube_matrix, p1->row_num, p2->col_num);
            value_cell = (value_cell_t *) p->user_word;
            key        = (char *) value_cell->sis_index;
            (void) st_insert(fanout, key, NIL(
            char));
        }
    }

    rect->value = (st_count(fanout) - 1) * (ks - 1) - 1;

    /* first rectangle becomes the best, otherwise must beat previous */
    if (state->first_rect || rect->value > state->best_rect->value) {
        rect_free(state->best_rect);
        state->best_rect  = rect_dup(rect);
        state->first_rect = 0;
    }

    st_free_table(fanout);
    return 1;
}

static rect_t *
best_subkernel(kernel_cube_matrix, row_cost, col_cost, func)
        sm_matrix *kernel_cube_matrix;
        array_t *row_cost, *col_cost;
        int (*func)();
{
    sm_row                 *prow;
    sm_element             *p;
    rect_t                 *rect;
    best_subkernel_state_t *state;

    state = ALLOC(best_subkernel_state_t, 1);
    state->best_rect          = rect_alloc();
    state->row_cost           = row_cost;
    state->col_cost           = col_cost;
    state->kernel_cube_matrix = kernel_cube_matrix;
    state->first_rect         = 1;
    gen_all_rectangles(kernel_cube_matrix, func, (char *) state);

    /* consider each row as a separate rectangle */
    sm_foreach_row(kernel_cube_matrix, prow)
    {
        rect = rect_alloc();
        (void) sm_col_insert(rect->rows, prow->row_num);
        sm_foreach_row_element(prow, p)
        {
            (void) sm_row_insert(rect->cols, p->col_num);
        }
        (void) (*func)(NIL(sm_matrix), rect, (char *) state);
        rect_free(rect);
    }

    rect = state->best_rect;
    FREE(state);
    return rect;
}

rect_t *
choose_subkernel(A, row_cost, col_cost, option)
        sm_matrix *A;
        array_t *row_cost, *col_cost;
        int option;
{
    rect_t *rect = 0;

    switch (option) {
        case 0: rect = ping_pong(A, row_cost, col_cost);
            break;
        case 1: rect = best_subkernel(A, row_cost, col_cost, best_subkernel_sop);
            break;
        case 2: rect = best_subkernel(A, row_cost, col_cost, best_subkernel_factor);
            break;
        default: fail("bad option in choose_subkernel");
            break;
    }
    return rect;
}
