
#include "sis.h"
#include "extract_int.h"


node_t *
ex_find_divisor(node, level, method)
node_t *node;
int level;
int method;
{
    sm_matrix *node_func, *kernel_sm;
    sm_row *co_kernel;
    node_t *divisor;
    rect_t *rect;
    int rownum;

    /* setup */
    ex_setup_globals_single(node);
    kernel_extract_init();
    node_func = sm_alloc();
    ex_node_to_sm(node, node_func);
    kernel_extract(node_func, /* sis_index */ 0, level);
    free_value_cells(node_func);
    sm_free(node_func);
    kernel_extract_end();

    /* check if the function is kernel-free */
    if (kernel_cube_matrix->nrows == 0) {
	/* shouldn't ever happen ? */
	divisor = NIL(node_t);
	goto cleanup;
    } else if (kernel_cube_matrix->nrows == 1) {
	rownum = kernel_cube_matrix->first_row->row_num;
	co_kernel = array_fetch(sm_row *, co_kernel_table, rownum);
	if (co_kernel->length == 0) {
	    divisor = NIL(node_t);
	    goto cleanup;
	}
    }

    rect = choose_subkernel(kernel_cube_matrix, 
		    global_row_cost, global_col_cost, method);
    divisor = 0;
    if (rect->cols->length > 0 && rect->rows->length > 0) {
	kernel_sm = ex_rect_to_kernel(rect);
	divisor = ex_sm_to_node(kernel_sm);
	sm_free(kernel_sm);
    }
    rect_free(rect);

    /* cleanup */
cleanup:
    kernel_extract_free();
    ex_free_globals(0);

    return divisor;
}
