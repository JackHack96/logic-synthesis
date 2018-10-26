
#ifndef PHASE_H
#define PHASE_H

#include "network.h"

void add_inv_network(network_t *);

int add_inv_node(network_t *, node_t *);

void phase_quick(network_t *);

void phase_good(network_t *);

void phase_random_greedy(network_t *, int);

void phase_trace_set(void);

void phase_trace_unset(void);

int init_phase();

int end_phase();

#endif
