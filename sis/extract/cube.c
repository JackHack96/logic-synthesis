
#include "extract_int.h"
#include "sis.h"

static void clear_rect();

static void update_cube_literal_matrix();

static rect_t *choose_subcube();

static rect_t *best_rect;
static sm_matrix *cube_literal_matrix;

#define SOP_VALUE(x, y) (((x)-1) * ((y)-1) - 1)

int sparse_cube_extract(A, value_threshold, use_best_subcube) sm_matrix *A;
int value_threshold;
int use_best_subcube;
{
  sm_row *prow;
  sm_col *pcol;
  rect_t *rect;
  array_t *row_cost, *col_cost;
  int total_value;

  /* cost for each row is 1 */
  row_cost = array_alloc(int, 0);
  sm_foreach_row(A, prow) { array_insert(int, row_cost, prow->row_num, 1); }

  /* cost for each column is 1 */
  col_cost = array_alloc(int, 0);
  sm_foreach_col(A, pcol) { array_insert(int, col_cost, pcol->col_num, 1); }

  if (cube_extract_debug) {
    (void)fprintf(
        sisout,
        "cube_extract: cube literal matrix is %d by %d cols, %d literals\n",
        A->nrows, A->ncols, sm_num_elements(A));
  }

  total_value = 0;
  do {
    rect = choose_subcube(A, row_cost, col_cost, use_best_subcube);
    if (rect->rows->length < 2 || rect->cols->length < 2 ||
        rect->value <= value_threshold) {
      rect_free(rect);
      break;
    }

    update_cube_literal_matrix(A, row_cost, col_cost, rect);
    clear_rect(A, rect);
    total_value += rect->value;

    if (cube_extract_debug) {
      (void)fprintf(sisout, "%d by %d value=%d literals=%d\n",
                    rect->rows->length, rect->cols->length, rect->value,
                    sm_num_elements(A));
    }

    rect_free(rect);
  } while (A->nrows > 0);

  array_free(row_cost);
  array_free(col_cost);
  return total_value;
}

static void update_cube_literal_matrix(A, row_cost, col_cost,
                                       rect) sm_matrix *A;
array_t *row_cost, *col_cost;
rect_t *rect;
{
  sm_element *p, *p1, *p2;
  sm_matrix *func;
  array_t *fanout;
  int new_colnum, new_rownum, new_index;
  value_cell_t *value_cell, *old_value_cell;

  /* Insert the new cube into the network and divide it out */
  func = sm_alloc();
  sm_copy_row(func, 0, rect->cols);
  fanout = find_rectangle_fanout(A, rect);
  new_index = ex_divide_function_into_network(func, fanout, NIL(array_t),
                                              cube_extract_debug);
  array_free(fanout);
  sm_free(func);

  /* add a new column */
  new_colnum = new_index * 2; /* assume positive phase ? */
  array_insert(int, col_cost, new_colnum, 1);
  sm_foreach_col_element(rect->rows, p) {
    p2 = sm_get_row(A, p->row_num)->first_col;
    old_value_cell = (value_cell_t *)p2->user_word;
    p1 = sm_insert(A, p->row_num, new_colnum);
    value_cell = value_cell_alloc();
    value_cell->value = 1;
    value_cell->sis_index = old_value_cell->sis_index;
    value_cell->cube_number = old_value_cell->cube_number;
    p1->user_word = (char *)value_cell;
  }

  /* add a new row */
  new_rownum = A->last_row->row_num + 1;
  array_insert(int, row_cost, new_rownum, 1);
  sm_foreach_row_element(rect->cols, p) {
    p1 = sm_insert(A, new_rownum, p->col_num);
    value_cell = value_cell_alloc();
    value_cell->value = 1;
    value_cell->sis_index = new_index;
    value_cell->cube_number = 0;
    p1->user_word = (char *)value_cell;
  }
}

#if 0
static int
int_cmp(x, y)
char **x, **y;
{
    int ix = *(int *) x, iy = *(int *) y;
    return iy - ix;
}


static int
bound_rect_size_fancy(co_rect, rect)
sm_matrix *co_rect;
rect_t *rect;
{
    register int i, n, *col_length, *row_length;
    int bound, nbound, new_rows, new_cols;
    sm_row *prow;
    sm_col *pcol;

    bound = 0;
    if (co_rect->nrows > 0) {

    /* get the row cardinalities and sort them */
    row_length = ALLOC(int, co_rect->nrows);
    n = 0;
    sm_foreach_row(co_rect, prow) {
        row_length[n++] = prow->length;
    }
    qsort((char *) row_length, co_rect->nrows, sizeof(int), int_cmp);

    /* get the column cardinalities and sort them */
    col_length = ALLOC(int, co_rect->ncols);
    n = 0;
    sm_foreach_col(co_rect, pcol) {
        col_length[n++] = pcol->length;
    }
    qsort((char *) col_length, co_rect->ncols, sizeof(int), int_cmp);

    for(i = 0; i < co_rect->ncols; i++) {
        /* check the array reference -- should not fail */
        assert(col_length[i] <= co_rect->nrows);

        new_rows = col_length[i];
        new_cols = row_length[col_length[i]-1] + rect->cols->length;
        nbound = SOP_VALUE(new_rows, new_cols);
        if (nbound > bound) bound = nbound;
    }

    FREE(row_length);
    FREE(col_length);
    }
    return bound;
}
#endif

static int bound_rect_size_simple(co_rect, rect) sm_matrix *co_rect;
rect_t *rect;
{
  sm_row *prow;
  sm_col *pcol;
  int bound;

  /* compute a bound for remaining part */
  if (co_rect->nrows > 0) {
    prow = sm_longest_row(co_rect);
    pcol = sm_longest_col(co_rect);
    bound = SOP_VALUE(rect->cols->length + prow->length, pcol->length);
  } else {
    bound = 0;
  }

  return bound;
}

/* ARGSUSED */
static int best_subcube_helper(co_rect, rect, state) sm_matrix *co_rect;
rect_t *rect;
char *state;
{
  int bound;

  rect->value = SOP_VALUE(rect->rows->length, rect->cols->length);
  if (best_rect->rows->length == 0 || rect->value > best_rect->value) {
    rect_free(best_rect);
    best_rect = rect_dup(rect);
  }
  bound = bound_rect_size_simple(co_rect, rect);
  return bound > best_rect->value;
}

static rect_t *best_subcube(A) sm_matrix *A;
{
  best_rect = rect_alloc();
  gen_all_rectangles(A, best_subcube_helper, NIL(char));
  return best_rect;
}

/* ARGSUSED */
static int best_subcube_factored_helper(co_rect, rect,
                                        state) sm_matrix *co_rect;
rect_t *rect;
char *state;
{
  sm_element *p1;
  sm_row *prow;
  value_cell_t *value_cell;
  char *key;
  st_table *func_table;

  /* Find which functions are contained by the rectangle */
  func_table = st_init_table(st_numcmp, st_numhash);
  sm_foreach_col_element(rect->rows, p1) {

    /* the first element in the row identifies the row's function */
    prow = sm_get_row(cube_literal_matrix, p1->row_num);
    value_cell = (value_cell_t *)prow->first_col->user_word;
    key = (char *)value_cell->sis_index;
    (void)st_insert(func_table, key, NIL(char));
  }

  rect->value = (rect->cols->length - 1) * (st_count(func_table) - 1) - 1;
  st_free_table(func_table);

  if (best_rect->rows->length == 0 || rect->value > best_rect->value) {
    rect_free(best_rect);
    best_rect = rect_dup(rect);
  }

  return 1;
}

static rect_t *best_subcube_factored(A) sm_matrix *A;
{
  cube_literal_matrix = A;
  best_rect = rect_alloc();
  gen_all_rectangles(A, best_subcube_factored_helper, NIL(char));
  return best_rect;
}

static rect_t *choose_subcube(A, row_cost, col_cost, option) sm_matrix *A;
array_t *row_cost, *col_cost;
int option;
{
  rect_t *rect;

  switch (option) {
  case 0:
    rect = ping_pong(A, row_cost, col_cost);
    break;

  case 1:
    rect = best_subcube(A);
    break;

  case 2:
    rect = best_subcube_factored(A);
    break;

  default:
    fail("bad option in choose_subcube");
    break;
  }

  return rect;
}

static void clear_rect(A, rect) sm_matrix *A;
rect_t *rect;
{
  register sm_element *p1, *p2, *p;

  sm_foreach_col_element(rect->rows, p1) {
    sm_foreach_row_element(rect->cols, p2) {
      p = sm_find(A, p1->row_num, p2->col_num);
      value_cell_free((value_cell_t *)p->user_word);
      sm_remove_element(A, p);
    }
  }
}
