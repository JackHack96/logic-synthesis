
#include "node_int.h"
#include "sis.h"

/*
 *  this is a hack for the library package
 *
 *	1. collapse network so all nodes are SOP over the primary inputs
 *	2. simplify each node so its all primes
 *	3. re-adjust the fanin list of each node so that it is defined
 *	   over the full support of all primary inputs; also, order this
 *	   fanin so that it is in the same order as the primary input list
 */

void node_lib_process(network) network_t *network;
{
  node_t *node, *pi, **new_fanin;
  int i, new_nin;
  pset_family func;
  lsGen gen, gen1;

  (void)network_collapse(network);

  foreach_node(network, gen, node) {
    if (node->type == INTERNAL) {
      (void)node_simplify_replace(node, NIL(node_t), NODE_SIM_ESPRESSO);
    }
  }

  foreach_node(network, gen, node) {
    if (node->type == INTERNAL) {

      i = 0;
      new_nin = network_num_pi(network);
      new_fanin = ALLOC(node_t *, new_nin);
      foreach_primary_input(network, gen1, pi) { new_fanin[i++] = pi; }

      func = node_sf_adjust(node, new_fanin, new_nin);
      node_replace_internal(node, new_fanin, new_nin, func);
    }
  }
}
