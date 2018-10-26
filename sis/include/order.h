
#ifndef ORDER_H /* { */
#define ORDER_H

#include "array.h"
#include "st.h"

/*
 * Variable ordering methods
 */
array_t *order_dfs(array_t *, st_table *, int);

array_t *order_random(array_t *, st_table *, int);

typedef enum { DFS_ORDER, RANDOM_ORDER } order_method_t;

#endif /* } */
