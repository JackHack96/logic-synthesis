
#include "sis.h"

/*
 *  network_cleanup -- remove internal nodes with no fanout; this is iterated
 *  until every node has fanout > 0.
 */

int network_cleanup(network) network_t *network;
{
  int status, latch_removed;

  status = network_cleanup_util(network, 1, &latch_removed);
  if ((network->dc_network != NIL(network_t)) && latch_removed) {
    network_free(network->dc_network);
    network->dc_network = NIL(network_t);
  }
  return status;
}

int network_ccleanup(network) network_t *network;
{ return network_cleanup_util(network, 0, NIL(int)); }

int network_cleanup_util(network, sweep_latch,
                         latch_removed) network_t *network;
int sweep_latch;
int *latch_removed;
{
  node_t *p;
  int some_change, changed;
  lsGen gen;
#ifdef SIS
  latch_t *l;
  int i;
  node_t *out;
  node_t *in;
  node_t *dc_pi, *dc_po, *cnode, *fanout;
  lsGen gen2;
  node_t **fo_array;
#endif /* SIS */

  some_change = 0;
  if (latch_removed != NIL(int)) {
    *latch_removed = 0;
  }
  do {
    changed = 0;
    foreach_node(network, gen, p) {
      if (p->type == INTERNAL && node_num_fanout(p) == 0) {
        network_delete_node_gen(network, gen);
        changed = 1;
        some_change = 1;
      }
    }
#ifdef SIS
    if (sweep_latch) {
      foreach_latch(network, gen, l) {
        out = latch_get_output(l);
        in = latch_get_input(l);
        if (node_num_fanout(out) == 0) {
          /* Update the DC network.  If no nodes depend on this latch
                     output, then we can set it to an arbitrary constant in
                     the DC network. */
          if (network->dc_network != NIL(network_t)) {
            dc_pi = network_find_node(network->dc_network, out->name);
            if (dc_pi != NIL(node_t)) {
              cnode = node_constant(0);
              network_add_node(network->dc_network, cnode);
              fo_array = ALLOC(node_t *, node_num_fanout(dc_pi));
              i = 0;
              foreach_fanout(dc_pi, gen2, fanout) { fo_array[i++] = fanout; }
              for (i = node_num_fanout(dc_pi); i-- > 0;) {
                node_patch_fanin(fo_array[i], dc_pi, cnode);
              }
              FREE(fo_array);
              network_delete_node(network->dc_network, dc_pi);
            }
            dc_po = network_find_node(network->dc_network, in->name);
            if (dc_po != NIL(node_t)) {
              network_delete_node(network->dc_network, dc_po);
            }
          }
          network_delete_latch_gen(network, gen);
          network_delete_node(network, in);
          network_delete_node(network, out);
          changed = 1;
          some_change = 1;
          if (latch_removed != NIL(int)) {
            *latch_removed = 1;
          }
        }
      }
    }
#endif /* SIS */

  } while (changed);
  if (network->dc_network != NIL(network_t)) {
    (void)network_sweep(network->dc_network);
  }

  return some_change;
}
