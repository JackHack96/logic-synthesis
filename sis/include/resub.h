
#ifndef RESUB_H
#define RESUB_H

#include "node.h"

int resub_alge_node(node_t *, int);

void resub_alge_network(network_t *, int);

void resub_bool_node(node_t *);

void resub_bool_network(network_t *);

int init_resub();

int end_resub();

#endif
