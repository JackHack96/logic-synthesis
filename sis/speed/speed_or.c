
/*
 * Routine to decompose a sum-of-products representation
 * into a tree of and(nand) gates.
 */

#include "speed_int.h"
#include "sis.h"
#include <stdio.h>

int speed_and_or_decomp(network, node, speed_param) network_t *network;
node_t *node;
speed_global_t *speed_param;
{
  int i;
  array_t *collapse_array;
  node_t *cube, *temp, *new_node, *nlit;

  if (node_num_cube(node) > 1) {
    new_node = node_constant(1);
    for (i = 0; i < node_num_cube(node); i++) {
      cube = speed_dec_node_cube(node, i);
      network_add_node(network, cube);

      nlit = node_literal(cube, 0);
      temp = node_and(new_node, nlit);
      node_free(new_node);
      node_free(nlit);
      new_node = temp;
    }
    node_replace(node, new_node);

    foreach_fanin(node, i, cube) {
      /*
       * speed decompose the cubes
       */
      if (!speed_and_decomp(network, cube, speed_param, 0)) {
        error_append("Failed to decompose cube");
        fail(error_string());
      }
    }

    if (!speed_param->add_inv) {
      collapse_array = array_alloc(node_t *, 0);
      foreach_fanin(node, i, cube) {
        if (node_num_fanin(cube) <= 1) {
          array_insert_last(node_t *, collapse_array, cube);
        }
      }
      for (i = 0; i < array_n(collapse_array); i++) {
        cube = array_fetch(node_t *, collapse_array, i);
        (void)node_collapse(node, cube);
        if (node_num_fanout(cube) == 0)
          network_delete_node(network, cube);
      }
      array_free(collapse_array);
    }
    /*
     * Decompose the cube that combines all the cubes.
     * The arrival time at the cube o/p will be set.
     */

    if (!speed_and_decomp(network, node, speed_param, 1)) {
      error_append("Failed to decompose node combining cubes ");
      fail(error_string());
    }
  } else if (!speed_and_decomp(network, node, speed_param, 0)) {
    error_append("Failed to decompose single cube");
    fail(error_string());
  }

  return 1;
}
