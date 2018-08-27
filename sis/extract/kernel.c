
#include "extract_int.h"
#include "sis.h"

int cube_extract_debug;
int kernel_extract_debug;
int greedy_col_debug;
int greedy_row_debug;
int ping_pong_debug;

static void dump_fct();

static void dump_cube();

static void dump_literal();

static void dump_subkernel();

static void dump_kernel_table();

static void dump_kernel();

static int record_kernel();

static void clear_kernel_rect();

/*
 * kernel_cube_table -- maps between kernel cubes and columns of the
 * kernel-cube matrix.
 */
static cubeindex_t *kernel_cube_table;

/*
 *  co_kernel_table -- maps between the co-kernel cubes and the rows of
 *  the kernel-cube matrix.  (It is simply an array because we never need
 *  to find the index given a cube)
 */
array_t *co_kernel_table;

/*
 *  kernel_cube_matrix -- a sparse matrix with 1 row for each kernel of
 *  each function.  Each row has an associated co-kernel, and each column
 *  has an associated kernel cube.  Each entry contains a 'value_cell'
 *  which gives the value of the cube covered by that entry, and which
 *  gives the function and cube number.  Each cube of a function is represented
 *  by only 1 value_cell even though it may appear multiple times in the
 *  sparse matrix (that is, it may be covered by many different kernels).
 *  Thus, the aliasing effect of covering a cube is easily detected.
 */
sm_matrix *kernel_cube_matrix;

/*
 *  costs for each kernel cube (row_cost) and each co-kernel cube (col_cost)
 */
array_t *global_row_cost;
array_t *global_col_cost;

static value_cell_t **function_cubes;

void kernel_extract_init() {
  kernel_cube_table = cubeindex_alloc();
  co_kernel_table = array_alloc(sm_row *, 0);
  kernel_cube_matrix = sm_alloc();
}

void kernel_extract(A, sis_index, use_all_kernels) sm_matrix *A;
int sis_index;
int use_all_kernels;
{
  sm_row *prow;

  if (A->nrows == 0)
    return; /* ignore empty functions */

  /* allocate 1 value_cell for each cube in the function */
  function_cubes = ALLOC(value_cell_t *, A->last_row->row_num + 1);
  sm_foreach_row(A, prow) {
    function_cubes[prow->row_num] = value_cell_alloc();
    function_cubes[prow->row_num]->sis_index = sis_index;
    function_cubes[prow->row_num]->cube_number = prow->row_num;
    function_cubes[prow->row_num]->value = prow->length;
  }

  /* generate the kernels of the function (including A itself) */
  gen_all_rectangles(A, record_kernel, (char *)use_all_kernels);

  /* Free the values cells -- remember, they are reference counted */
  sm_foreach_row(A, prow) { value_cell_free(function_cubes[prow->row_num]); }
  FREE(function_cubes);
}

void kernel_extract_end() {
  sm_row *prow;
  sm_col *pcol;
  sm_row *kernel_cube, *co_kernel;
  int k;

  /* setup the row cost array (the cost of each co-kernel) */
  global_row_cost = array_alloc(int, 0);
  sm_foreach_row(kernel_cube_matrix, prow) {
    co_kernel = array_fetch(sm_row *, co_kernel_table, prow->row_num);
    k = co_kernel->length + 1;
    array_insert(int, global_row_cost, prow->row_num, k);
  }

  /* setup the column cost array (the cost of each kernel-cube) */
  global_col_cost = array_alloc(int, 0);
  sm_foreach_col(kernel_cube_matrix, pcol) {
    kernel_cube = cubeindex_getcube(kernel_cube_table, pcol->col_num);
    array_insert(int, global_col_cost, pcol->col_num, kernel_cube->length);
  }
}

void kernel_extract_free() {
  int i;

  cubeindex_free(kernel_cube_table);

  for (i = 0; i < array_n(co_kernel_table); i++) {
    sm_row_free(array_fetch(sm_row *, co_kernel_table, i));
  }
  array_free(co_kernel_table);
  free_value_cells(kernel_cube_matrix);
  sm_free(kernel_cube_matrix);

  array_free(global_row_cost);
  array_free(global_col_cost);
}

static int record_kernel(co_rect, rect, state) sm_matrix *co_rect;
rect_t *rect;
char *state;
{
  register int rownum, colnum;
  register sm_row *prow;
  sm_row *row;
  sm_element *p;
  int use_all_kernels = (int)state;

  if (use_all_kernels || ex_is_level0_kernel(co_rect)) {

    /* Record the kernel as a row in the kernel_cube matrix */
    rownum = kernel_cube_matrix->nrows;
    sm_foreach_row(co_rect, prow) {

      /* find the column for this kernel cube */
      colnum = cubeindex_getindex(kernel_cube_table, prow);
      p = sm_insert(kernel_cube_matrix, rownum, colnum);

      /* set the 'value_cell' for this entry -- bump reference count */
      function_cubes[prow->row_num]->ref_count++;
      p->user_word = (char *)function_cubes[prow->row_num];
    }

    /* Save the co_kernel */
    row = sm_row_dup(rect->cols);
    array_insert(sm_row *, co_kernel_table, rownum, row);
  }
  return 1;
}

/*
 *  sparse_kernel_extract
 *
 *  use_best_subkernel
 *	== 0 uses pingpong to select a good value rectangle
 *      == 1 uses best_subkernel to select the best value rectangle
 *      == 2 uses best_subkernel (and # literals in factored form)
 *
 *  nonoverlapping: all entries of the kernel_cube_matrix which contain
 *  a particular value cell are removed before selecting the next subkernel
 */

int sparse_kernel_extract(value_threshold,
                          use_best_subkernel) int value_threshold,
    use_best_subkernel;
{
  array_t *fanout;
  sm_row *kernel_cube;
  sm_matrix *func;
  sm_element *p;
  rect_t *rect;
  int nonzero, total, total_value;
  double sparsity;

  if (kernel_extract_debug) {
    if (kernel_extract_debug == 99)
      dump_kernel_table();
    nonzero = sm_num_elements(kernel_cube_matrix);
    total = kernel_cube_matrix->nrows * kernel_cube_matrix->ncols;
    sparsity = (double)nonzero / (double)total * 100.0;
    (void)fprintf(
        sisout, "kernel-cube matrix is %d by %d (%d nonzero, %4.3f%% sparse)\n",
        kernel_cube_matrix->nrows, kernel_cube_matrix->ncols, nonzero,
        sparsity);
  }

  kernel_extract_end();

  /* Extract the kernels */
  total_value = 0;
  do {
    rect = choose_subkernel(kernel_cube_matrix, global_row_cost,
                            global_col_cost, use_best_subkernel);
    if (rect->rows->length == 0 || rect->cols->length == 0 ||
        rect->value <= value_threshold) {
      rect_free(rect);
      break;
    }

    /* construct a sparse-matrix representing the function */
    func = sm_alloc();
    sm_foreach_row_element(rect->cols, p) {
      kernel_cube = cubeindex_getcube(kernel_cube_table, p->col_num);
      sm_copy_row(func, func->nrows, kernel_cube);
    }

    /* add it to the network */
#if 1
    fanout = find_rectangle_fanout(kernel_cube_matrix, rect);
    (void)ex_divide_function_into_network(func, fanout, NIL(array_t),
                                          kernel_extract_debug);
#else
        sometimes for debugging thsi is better
            fanout = find_rectangle_fanout_cubes(kernel_cube_matrix, rect, &cubes);
        (void)ex_divide_function_into_network(func, fanout, cubes);
        array_free(cubes);
#endif
    array_free(fanout);
    sm_free(func);

    if (kernel_extract_debug) {
      (void)fprintf(sisout, "subkernel rectangle is %d by %d with value %d\n",
                    rect->rows->length, rect->cols->length, rect->value);
      if (kernel_extract_debug > 1) {
        dump_subkernel(rect);
      }
    }

    clear_kernel_rect(kernel_cube_matrix, rect);
    total_value += rect->value;
    rect_free(rect);
  } while (kernel_cube_matrix->nrows > 0); /* make lint happy */

  return total_value;
}

int ex_is_level0_kernel(A) sm_matrix *A;
{
  register sm_col *pcol;

  if (A->nrows <= 1) {
    return 0;
  } else {
    sm_foreach_col(A, pcol) {
      if (pcol->length > 1) {
        return 0;
      }
    }
    return 1;
  }
}

static void clear_kernel_rect(A, rect) sm_matrix *A;
rect_t *rect;
{
  register sm_element *p1, *p2, *p;
  register sm_row *prow;
  sm_row *next_row;
  st_table *value_table;

  value_table = st_init_table(st_ptrcmp, st_ptrhash);

  sm_foreach_col_element(rect->rows, p1) {
    sm_foreach_row_element(rect->cols, p2) {
      p = sm_find(A, p1->row_num, p2->col_num);
      assert(p != NIL(sm_element));
      (void)st_insert(value_table, p->user_word, NIL(char));
      value_cell_free((value_cell_t *)p->user_word);
      sm_remove_element(A, p);
    }
  }

  for (prow = A->first_row; prow != 0; prow = next_row) {
    next_row = prow->next_row;
    for (p = prow->first_col; p != 0; p = p1) {
      p1 = p->next_col;
      if (st_lookup(value_table, p->user_word, NIL(char *))) {
        value_cell_free((value_cell_t *)p->user_word);
        sm_remove_element(A, p);
      }
    }
  }
  st_free_table(value_table);
}

/*
 *  overlapping kernel extraction
 */

static void overlapping_clear_rect();

int overlapping_kernel_extract(value_threshold,
                               use_best_subkernel) int value_threshold,
    use_best_subkernel;
{
  array_t *fanout, *cubes;
  sm_row *kernel_cube;
  sm_matrix *func;
  sm_element *p;
  rect_t *rect;
  int nonzero, total, total_value;
  double sparsity;

  if (kernel_extract_debug) {
    nonzero = sm_num_elements(kernel_cube_matrix);
    total = kernel_cube_matrix->nrows * kernel_cube_matrix->ncols;
    sparsity = (double)nonzero / (double)total * 100.0;
    (void)fprintf(
        sisout, "kernel-cube matrix is %d by %d (%d nonzero, %4.3f%% sparse)\n",
        kernel_cube_matrix->nrows, kernel_cube_matrix->ncols, nonzero,
        sparsity);
  }

  kernel_extract_end();

  /*  Extract the kernels  */
  total_value = 0;
  do {
    rect = choose_subkernel(kernel_cube_matrix, global_row_cost,
                            global_col_cost, use_best_subkernel);
    if (rect->rows->length == 0 || rect->cols->length == 0 ||
        rect->value <= value_threshold) {
      rect_free(rect);
      break;
    }

    /* construct a sparse-matrix representing the function */
    func = sm_alloc();
    sm_foreach_row_element(rect->cols, p) {
      kernel_cube = cubeindex_getcube(kernel_cube_table, p->col_num);
      sm_copy_row(func, func->nrows, kernel_cube);
    }

    /* add it to the network */
    fanout = find_rectangle_fanout_cubes(kernel_cube_matrix, rect, &cubes);
    (void)ex_divide_function_into_network(func, fanout, cubes,
                                          kernel_extract_debug);
    array_free(fanout);
    sm_free(func);

    if (kernel_extract_debug) {
      (void)fprintf(sisout, "subkernel rectangle is %d by %d with value %d\n",
                    rect->rows->length, rect->cols->length, rect->value);
      if (kernel_extract_debug > 1) {
        dump_subkernel(rect);
      }
    }

    overlapping_clear_rect(kernel_cube_matrix, rect);
    total_value += rect->value;
    rect_free(rect);
  } while (kernel_cube_matrix->nrows > 0);

  return total_value;
}

static void overlapping_clear_rect(A, rect) sm_matrix *A;
rect_t *rect;
{
  register sm_element *p1, *p2, *p;

  sm_foreach_col_element(rect->rows, p1) {
    sm_foreach_row_element(rect->cols, p2) {
      p = sm_find(A, p1->row_num, p2->col_num);
      assert(p != NIL(sm_element));
      VALUE(p) = 0;
    }
  }
}

static void dump_literal(fp, index) FILE *fp;
int index;
{
  (void)fputs(ex_map_index_to_name(index / 2), fp);
  if ((index % 2) == 1)
    putc('\'', fp);
}

static void dump_cube(fp, cube) FILE *fp;
sm_row *cube;
{
  sm_element *p;
  int first;

  first = 1;
  sm_foreach_row_element(cube, p) {
    if (!first)
      putc(' ', fp);
    dump_literal(fp, p->col_num);
    first = 0;
  }
}

static void dump_fct(fp, fct) FILE *fp;
sm_matrix *fct;
{
  sm_row *cube;
  int first;

  first = 1;
  sm_foreach_row(fct, cube) {
    if (!first)
      (void)fputs(" + ", fp);
    dump_cube(fp, cube);
    first = 0;
  }
}

static void dump_kernel(fp, kernel) FILE *fp;
sm_row *kernel;
{
  sm_row *kernel_cube;
  sm_element *p;
  int first;

  first = 1;
  sm_foreach_row_element(kernel, p) {
    if (!first)
      (void)fputs(" + ", fp);
    kernel_cube = cubeindex_getcube(kernel_cube_table, p->col_num);
    dump_cube(fp, kernel_cube);
    first = 0;
  }
}

static void dump_subkernel(rect) rect_t *rect;
{
  sm_matrix *func, *func1;

  func = ex_rect_to_kernel(rect);
  func1 = ex_rect_to_cokernel(rect);

  (void)fprintf(sisout, "subkernel: ");
  dump_fct(sisout, func);
  (void)fprintf(sisout, "\n");
  (void)fprintf(sisout, "co-kernel: ");
  dump_fct(sisout, func1);
  (void)fprintf(sisout, "\n");

  sm_free(func);
  sm_free(func1);
}

static void dump_kernel_table() {
  sm_row *kernel, *co_kernel, *kernel_cube;
  sm_element *p;
  value_cell_t *value_cell;

  (void)fprintf(sisout, "Kernel cube matrix is\n");
  sm_print(sisout, kernel_cube_matrix);

  (void)fprintf(sisout, "\nKernels are\n");
  sm_foreach_row(kernel_cube_matrix, kernel) {
    (void)fprintf(sisout, "kernel %3d: ", kernel->row_num);
    dump_kernel(sisout, kernel);
    (void)fprintf(sisout, "  (co-kernel=");
    co_kernel = array_fetch(sm_row *, co_kernel_table, kernel->row_num);
    dump_cube(sisout, co_kernel);
    (void)fprintf(sisout, ")\n");
  }

  (void)fprintf(sisout, "\nkernels are (gross detail)\n");
  sm_foreach_row(kernel_cube_matrix, kernel) {
    (void)fprintf(sisout, "kernel %d: co-kernel:  ", kernel->row_num);
    co_kernel = array_fetch(sm_row *, co_kernel_table, kernel->row_num);
    dump_cube(sisout, co_kernel);
    (void)fprintf(sisout, "\n");

    sm_foreach_row_element(kernel, p) {
      value_cell = (value_cell_t *)p->user_word;
      (void)fprintf(sisout,
                    "    col=%3d sis-fct=%-10s cube=%2d literals=%2d:  ",
                    p->col_num, ex_map_index_to_name(value_cell->sis_index),
                    value_cell->cube_number, value_cell->value);
      kernel_cube = cubeindex_getcube(kernel_cube_table, p->col_num);
      dump_cube(sisout, kernel_cube);
      (void)fprintf(sisout, "\n");
    }
  }
  (void)fprintf(sisout, "\n");
}

sm_matrix *ex_rect_to_kernel(rect) rect_t *rect;
{
  sm_matrix *func;
  sm_row *cube;
  sm_element *p;

  /* construct a sparse-matrix representing the kernel */
  func = sm_alloc();
  sm_foreach_row_element(rect->cols, p) {
    cube = cubeindex_getcube(kernel_cube_table, p->col_num);
    sm_copy_row(func, func->nrows, cube);
  }
  return func;
}

sm_matrix *ex_rect_to_cokernel(rect) rect_t *rect;
{
  sm_matrix *func;
  sm_row *cube;
  sm_element *p;

  /* construct a sparse-matrix representing the co-kernel */
  func = sm_alloc();
  sm_foreach_col_element(rect->rows, p) {
    cube = array_fetch(sm_row *, co_kernel_table, p->row_num);
    sm_copy_row(func, func->nrows, cube);
  }
  return func;
}
