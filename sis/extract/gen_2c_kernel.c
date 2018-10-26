#include "sis.h"
#include "extract_int.h"
#include "heap.h"
#include "fast_extract_int.h"


/* generate all 2-cube kernels for a node */
static ddset_t *
fx_node_2c_kernels(node_func, index)
sm_matrix *node_func;
int index;
{
    lsGen gen;
    ddset_t *ddset;
    ddivisor_t *divisor;
   
    ddset = ddset_alloc_init();  
    (void) extract_ddivisor(node_func, index, 0, ddset);
    lsForeachItem(ddset->DDset, gen, divisor) {
        WEIGHT(divisor) = fx_compute_divisor_weight(node_func, divisor); 
    }

    return ddset;
}


/* tranform a 2-cube divisor to a node */
static node_t *
fx_ddivisor_to_node(divisor)
ddivisor_t *divisor;
{
    sm_matrix *func;
    node_t *node;
    
    func = sm_alloc();
    (void) sm_copy_row(func, 0, divisor->cube1);
    (void) sm_copy_row(func, 1, divisor->cube2);
    node = fx_sm_to_node(func);
    sm_free(func);

    return node;
}


/* select the best 2-cube divisor based on the provided weighting function */ 
node_t *
fx_2c_kernel_best(node, function, state)
node_t *node;
int (*function)();
char *state;
{
    sm_matrix *node_func;
    ddset_t *ddset;
    ddivisor_t *divisor;
    int status, node_index;
    lsGen gen;
    lsHandle *handle;
    sm_row *prow;
    node_t *node_2c;

    /* setup */
    ex_setup_globals_single(node);

    switch(node_function(node)) {
    case NODE_PI:
    case NODE_PO:
    case NODE_0:
    case NODE_1:
    case NODE_INV:
    case NODE_BUF:
        return NIL(node_t);
    case NODE_AND:
    case NODE_OR:
    case NODE_COMPLEX:
        node_func = sm_alloc();
        (void) fx_node_to_sm(node, node_func);
        node_index = nodeindex_indexof(global_node_index, node);
        ddset = fx_node_2c_kernels(node_func, node_index);
    }

    /* evaluate every 2-cube divisor */
    handle = LS_NH;
    lsForeachItem(ddset->DDset, gen, divisor) {
        node_2c = fx_ddivisor_to_node(divisor);
        status = (*function)(divisor, node_2c, &handle, state);
        (void) node_free(node_2c);
	if (status == 0){
/*
	    fprintf(sisout,"?");
	    fflush(sisout);
*/
	    handle = LS_NH;
	    lsFinish(gen);
	    break;
	}
    }

    /* get the lsHandle from state data structure */
     if (handle != LS_NH){
         divisor = (ddivisor_t *)lsFetchHandle(*handle);
         node_2c = fx_ddivisor_to_node(divisor);
     } else {
         node_2c = NIL(node_t);
     }

    /* cleanup */
    (void) ddset_free(ddset);
    sm_foreach_row(node_func, prow) {
        (void) sm_cube_free((sm_cube_t *) prow->user_word);
    }
    (void) sm_free(node_func);
    ex_free_globals(0);

    return node_2c;
}
