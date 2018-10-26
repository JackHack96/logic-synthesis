#ifndef DECOMP_INT_H
#define DECOMP_INT_H

#include "node.h"
#include "array.h"
#include "sparse.h"

node_t *decomp_quick_kernel();

node_t *decomp_good_kernel();

array_t *decomp_recur();

array_t *my_array_append();

array_t *decomp_tech_recur();

node_t *dec_node_cube();

int dec_block_partition();

sm_matrix *dec_node_to_sm();

node_t *dec_sm_to_node();

#endif