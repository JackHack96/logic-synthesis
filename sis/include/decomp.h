
#ifndef DECOMP_H
#define DECOMP_H

extern void decomp_quick_network(network_t *);

extern void decomp_quick_node(network_t *, node_t *);

extern array_t *decomp_quick(node_t *);

extern void decomp_good_network(network_t *);

extern void decomp_good_node(network_t *, node_t *);

extern array_t *decomp_good(node_t *);

extern void decomp_disj_network(network_t *);

extern void decomp_disj_node(network_t *, node_t *);

extern array_t *decomp_disj(node_t *);

extern void decomp_tech_network(network_t *, int, int);

#endif
