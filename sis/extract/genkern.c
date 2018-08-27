
#include "extract_int.h"
#include "sis.h"

static int gen_kernels_got_one();

static int gen_subkernels_got_one();

typedef struct gen_kernel_state_struct gen_kernel_state_t;

struct gen_kernel_state_struct {
  int (*user_func)();

  char *user_state;
};

void ex_kernel_gen(node, func, state) node_t *node;
int (*func)();
char *state;
{
  sm_matrix *node_func;
  gen_kernel_state_t *my_state;

  my_state = ALLOC(gen_kernel_state_t, 1);
  my_state->user_func = func;
  my_state->user_state = state;

  /* setup */
  ex_setup_globals_single(node);
  node_func = sm_alloc();
  ex_node_to_sm(node, node_func);

  gen_all_rectangles(node_func, gen_kernels_got_one, (char *)my_state);

  /* cleanup */
  free_value_cells(node_func);
  sm_free(node_func);
  ex_free_globals(0);
  FREE(my_state);
}

static int gen_kernels_got_one(co_rect, rect, state_p) sm_matrix *co_rect;
register rect_t *rect;
char *state_p;
{
  gen_kernel_state_t *state;
  sm_matrix *dummy;
  node_t *kernel, *cokernel;

  state = (gen_kernel_state_t *)state_p;

  /* map the kernel ('co_rect') into a real node */
  kernel = ex_sm_to_node(co_rect);

  /* map the co-kernel ('rect') into a real node */
  dummy = sm_alloc();
  sm_copy_row(dummy, 0, rect->cols);
  cokernel = ex_sm_to_node(dummy);
  sm_free(dummy);

  /* hack for the fact that sparse matrix can't represent 1 */
  if (node_function(cokernel) == NODE_0) {
    node_free(cokernel);
    cokernel = node_constant(1);
  }

  return (*state->user_func)(kernel, cokernel, state->user_state);
}

void ex_subkernel_gen(node, func, level, state) node_t *node;
int (*func)();
int level;
char *state;
{
  sm_matrix *node_func;
  gen_kernel_state_t *my_state;

  my_state = ALLOC(gen_kernel_state_t, 1);
  my_state->user_func = func;
  my_state->user_state = state;

  /* setup */
  ex_setup_globals_single(node);
  kernel_extract_init();
  node_func = sm_alloc();
  ex_node_to_sm(node, node_func);
  kernel_extract(node_func, /* sis_index */ 0, level);
  free_value_cells(node_func);
  sm_free(node_func);
  kernel_extract_end();

  gen_all_rectangles(kernel_cube_matrix, gen_subkernels_got_one,
                     (char *)my_state);

  /* cleanup */
  kernel_extract_free();
  ex_free_globals(0);
  FREE(my_state);
}

/* ARGSUSED */
static int gen_subkernels_got_one(co_rect, rect, state_p) sm_matrix *co_rect;
register rect_t *rect;
char *state_p;
{
  gen_kernel_state_t *state;
  node_t *kernel, *cokernel;
  sm_matrix *kernel_sm, *cokernel_sm;

  state = (gen_kernel_state_t *)state_p;

  if (rect->cols->length == 0 || rect->rows->length == 0)
    return 1;

  kernel_sm = ex_rect_to_kernel(rect);
  kernel = ex_sm_to_node(kernel_sm);

  cokernel_sm = ex_rect_to_cokernel(rect);
  cokernel = ex_sm_to_node(cokernel_sm);

  sm_free(kernel_sm);
  sm_free(cokernel_sm);

  return (*state->user_func)(kernel, cokernel, state->user_state);
}
