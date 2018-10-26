
#ifndef FACTOR_H
#define FACTOR_H

#include "node.h"
#include "array.h"
#include <stdio.h>

void factor(node_t *);

void factor_quick(node_t *);

void factor_good(node_t *);

void factor_free(node_t *);

void factor_dup(node_t *, node_t *);

void factor_alloc(node_t *);

void factor_invalid(node_t *);

void factor_print(FILE *, node_t *);

int node_value(node_t *);

int factor_num_literal(node_t *);

int factor_num_used(node_t *, node_t *);

void eliminate(network_t *, int, int);

array_t *factor_to_nodes(node_t *);

int init_factor();

int end_factor();

#endif
