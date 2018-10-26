
#ifndef EXTRACT_INT_H
#define EXTRACT_INT_H

#include "sparse.h"
#include "st.h"
#include "array.h"
#include "node.h"
#include "nodeindex.h"

/*
 *  a rectangle is a collection of rows, columns and has a value
 */
typedef struct rect_struct rect_t;
struct rect_struct {
  sm_col *rows;
  sm_row *cols;
  int value;
};

/*
 *  each element of the sparse matrix contains a 'value' and identifies
 *  where it came from
 */
typedef struct value_cell_struct value_cell_t;
struct value_cell_struct {
  int value;       /* # literals in this cube */
  int sis_index;   /* which sis function */
  int cube_number; /* which cube of that function */
  int ref_count;   /* number of times referenced */
};
#define VALUE(p) ((value_cell_t *)(p)->user_word)->value

/*
 *  a quick way to associate cubes (i.e., sm_row *) to small integers
 *  and back.  Used to build the kernel_cube matrix
 */
typedef struct cubeindex_struct cubeindex_t;
struct cubeindex_struct {
  st_table *cube_to_integer;
  array_t *integer_to_cube;
};

/* greedyrow.c, greedycol.c, pingpong.c */
rect_t *greedy_row();

rect_t *greedy_col();

rect_t *ping_pong();

/* cube_extract */
int sparse_cube_extract();

/* kernel.c */
void kernel_extract_init();

void kernel_extract();

void kernel_extract_end();

void kernel_extract_free();

int sparse_kernel_extract();

int overlapping_kernel_extract();

int ex_is_level0_kernel();

sm_matrix *ex_rect_to_kernel();

sm_matrix *ex_rect_to_cokernel();

sm_matrix *kernel_cube_matrix; /* hack */
array_t *co_kernel_table;      /* hack */
array_t *global_row_cost;      /* hack */
array_t *global_col_cost;      /* hack */

/* best_subk.c */
rect_t *choose_subkernel();

/* rect.c */
void gen_all_rectangles();

void rect_free();

rect_t *rect_alloc();

rect_t *rect_dup();

/* value.c */
value_cell_t *value_cell_alloc();

void value_cell_free();

void free_value_cells();

/* sisc.c */
array_t *find_rectangle_fanout();

array_t *find_rectangle_fanout_cubes();

/* cubeindex.c */
cubeindex_t *cubeindex_alloc();

void cubeindex_free();

int cubeindex_getindex();

sm_row *cubeindex_getcube();

/* conv.c */
int ex_divide_function_into_network();

void ex_node_to_sm();

sm_matrix *ex_network_to_sm();

node_t *ex_sm_to_node();

void ex_setup_globals();

void ex_setup_globals_single();

void ex_free_globals();

char *ex_map_index_to_name();

int greedy_row_debug;
int greedy_col_debug;
int ping_pong_debug;
int cube_extract_debug;
int kernel_extract_debug;

network_t *global_network;
nodeindex_t *global_node_index;
array_t *global_old_fct;
int use_complement;

#endif