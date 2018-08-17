
#ifndef ORDER_H /* { */
#define ORDER_H


/*
 * Variable ordering methods
 */
extern array_t *order_dfs(array_t *, st_table *, int);

extern array_t *order_random(array_t *, st_table *, int);

typedef enum {
    DFS_ORDER, RANDOM_ORDER
} order_method_t;

#endif /* } */
