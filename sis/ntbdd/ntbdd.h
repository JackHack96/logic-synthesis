#ifndef NTBDD_H /* { */
#define NTBDD_H


/*
 * Verification methods
 */
typedef enum {
    ONE_AT_A_TIME, ALL_TOGETHER
} ntbdd_verify_method_t;

/* 
 * Utilities
 */
extern bdd_t *ntbdd_at_node(node_t *);

extern network_t *ntbdd_bdd_single_to_network(bdd_t *, char *, array_t *);

extern network_t *ntbdd_bdd_array_to_network(array_t *, array_t *, array_t *);

extern void ntbdd_end_manager(bdd_manager *);

extern void ntbdd_free_at_node(node_t *);

extern bdd_t *ntbdd_node_to_bdd(node_t *, bdd_manager *, st_table *);

extern bdd_t *ntbdd_node_to_local_bdd(node_t *, bdd_manager *, st_table *);

extern void ntbdd_set_at_node(node_t *, bdd_t *);

extern bdd_manager *ntbdd_start_manager(int);

extern int ntbdd_verify_network(network_t *, network_t *, order_method_t, ntbdd_verify_method_t);

#endif /* } */
