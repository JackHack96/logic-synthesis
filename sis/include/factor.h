
#ifndef FACTOR_H
#define FACTOR_H

#include "node.h"
#include "array.h"
#include <stdio.h>

extern void factor(node_t *);

extern void factor_quick(node_t *);

extern void factor_good(node_t *);

extern void factor_free(node_t *);

extern void factor_dup(node_t *, node_t *);

extern void factor_alloc(node_t *);

extern void factor_invalid(node_t *);

extern void factor_print(FILE *, node_t *);

extern int node_value(node_t *);

extern int factor_num_literal(node_t *);

extern int factor_num_used(node_t *, node_t *);

extern void eliminate(network_t *, int, int);

extern array_t *factor_to_nodes(node_t *);

int init_factor();

int end_factor();

#endif
