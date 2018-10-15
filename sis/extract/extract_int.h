/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/extract/extract_int.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:22 $
 *
 */
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
    int value;			/* # literals in this cube */
    int sis_index;		/* which sis function */
    int cube_number;		/* which cube of that function */ 
    int ref_count;		/* number of times referenced */
};
#define VALUE(p)		((value_cell_t *) (p)->user_word)->value


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
extern rect_t *greedy_row();
extern rect_t *greedy_col();
extern rect_t *ping_pong();

/* cube_extract */
extern int sparse_cube_extract();

/* kernel.c */
extern void kernel_extract_init();
extern void kernel_extract();
extern void kernel_extract_end();
extern void kernel_extract_free();
extern int sparse_kernel_extract();
extern int overlapping_kernel_extract();
extern int ex_is_level0_kernel();
extern sm_matrix *ex_rect_to_kernel();
extern sm_matrix *ex_rect_to_cokernel();

extern sm_matrix *kernel_cube_matrix;		/* hack */
extern array_t *co_kernel_table;		/* hack */
extern array_t *global_row_cost;		/* hack */
extern array_t *global_col_cost;		/* hack */

/* best_subk.c */
extern rect_t *choose_subkernel();

/* rect.c */
extern void gen_all_rectangles();
extern void rect_free();
extern rect_t *rect_alloc();
extern rect_t *rect_dup();

/* value.c */
extern value_cell_t *value_cell_alloc();
extern void value_cell_free();
extern void free_value_cells();

/* sisc.c */
extern array_t *find_rectangle_fanout();
extern array_t *find_rectangle_fanout_cubes();

/* cubeindex.c */
extern cubeindex_t *cubeindex_alloc();
extern void cubeindex_free();
extern int cubeindex_getindex();
extern sm_row *cubeindex_getcube();

/* conv.c */
extern int ex_divide_function_into_network();
extern void ex_node_to_sm();
extern sm_matrix *ex_network_to_sm();
extern node_t *ex_sm_to_node();
extern void ex_setup_globals();
extern void ex_setup_globals_single();
extern void ex_free_globals();
extern char *ex_map_index_to_name();

extern int greedy_row_debug;
extern int greedy_col_debug;
extern int ping_pong_debug;
extern int cube_extract_debug;
extern int kernel_extract_debug;

extern network_t *global_network;
extern nodeindex_t *global_node_index;
extern array_t *global_old_fct;
extern int use_complement;
