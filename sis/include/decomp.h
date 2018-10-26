
#ifndef DECOMP_H
#define DECOMP_H

#include "network.h"
#include "array.h"
#include "node.h"

void decomp_quick_network(network_t *);

void decomp_quick_node(network_t *, node_t *);

array_t *decomp_quick(node_t *);

void decomp_good_network(network_t *);

void decomp_good_node(network_t *, node_t *);

array_t *decomp_good(node_t *);

void decomp_disj_network(network_t *);

void decomp_disj_node(network_t *, node_t *);

array_t *decomp_disj(node_t *);

void decomp_tech_network(network_t *, int, int);

int init_decomp();

int end_decomp();

#endif
