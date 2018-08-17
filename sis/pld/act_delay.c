
#include "sis.h"
#include "pld_int.h"

array_t *
act_order_for_delay(node, cost_table)
        node_t *node;
        st_table *cost_table;
{
    array_t     *result, *fanin_array;
    int         nsize, i, j;
    char        *name, *dummy;
    node_t      *fanin;
    COST_STRUCT *cost_node;


    switch (node_function(node)) {
        case NODE_PI : return NIL(array_t);
        case NODE_PO : return NIL(array_t);
        case NODE_0 : return NIL(array_t);
        case NODE_1 : return NIL(array_t);
        default:;
    }

    result      = array_alloc(node_t * , 0);
    fanin_array = array_alloc(COST_STRUCT * , 0);

    foreach_fanin(node, j, fanin)
    {
        name = node_long_name(fanin);
        assert(st_lookup(cost_table, name, &dummy));
        cost_node = (COST_STRUCT *) dummy;
        array_insert_last(COST_STRUCT * , fanin_array, cost_node);
    }
    /* sort the fanin_array by the arrival times */
    /*-------------------------------------------*/
    array_sort(fanin_array, arrival_compare_fn);

    /* put the sorted fanins into the result array */
    /*---------------------------------------------*/
    if (ACT_DEBUG) (void) printf("printing delay order_list for node %s\n", node_long_name(node));
    nsize  = array_n(fanin_array);
    for (i = 0; i < nsize; i++) {
        cost_node = array_fetch(COST_STRUCT * , fanin_array, i);
        array_insert_last(node_t * , result, cost_node->node);
        if (ACT_DEBUG) (void) printf("order_list[%d] = %s\n", i, node_long_name(cost_node->node));
    }

    array_free(fanin_array);
    return result;

}

int arrival_compare_fn(c1, c2)
        COST_STRUCT **c1, **c2; /*  check this */
{
    double arr1, arr2;

    arr1 = (*c1)->arrival_time;
    arr2 = (*c2)->arrival_time;

    if (arr1 > arr2) return (-1);
    if (arr1 < arr2) return 1;
    return 0;
}
  

