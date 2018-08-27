#ifndef NTBDD_INT_H /* { */
#define NTBDD_INT_H

/*
 * Stuff hooked on the bdd manager so that everything is
 * freed at the appropriate point.
 */
typedef struct {
  network_t *last_network;
  st_table *network_table;
} ntbdd_t;

/*
 * Macro to access bdd_t's at nodes.
 */
#define BDD(node) ((bdd_t *)(node)->bdd)
#define BDD_SET(node, value) ((node)->bdd = (char *)(value))

/*
 * Stuff internal to the ntbdd package.
 */
extern void bdd_alloc_demon(/* node_t *node */);

extern void bdd_free_demon(/* node_t *node; */);

/*
 * enum type to denote what type of BDD is being created: LOCAL
 * means just in terms of the node's immediate fanin; GLOBAL
 * means in terms of the nodes in the leaves table.
 */
typedef enum { LOCAL, GLOBAL } ntbdd_type_t;

#endif /* } */
