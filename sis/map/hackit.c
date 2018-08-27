
/* file @(#)hackit.c	1.2 */
/* last modified on 5/1/91 at 15:51:06 */
#include "lib_int.h"
#include "map_int.h"
#include "sis.h"

static lib_gate_t *choose_largest_gate(library, string) library_t *library;
char *string;
{
  network_t *network;
  lib_class_t *class;
  lib_gate_t *gate, *best_gate;
  lsGen gen;
  double best_area;

  network = read_eqn_string(string);
  if (network == 0)
    return 0;
  class = lib_get_class(network, library);
  if (class == 0)
    return 0;

  best_area = -1;
  best_gate = 0;
  gen = lib_gen_gates(class);
  while (lsNext(gen, (char **)&gate, LS_NH) == LS_OK) {
    if (lib_gate_area(gate) > best_area) {
      best_area = lib_gate_area(gate);
      best_gate = gate;
    }
  }
  LS_ASSERT(lsFinish(gen));
  network_free(network);
  return best_gate;
}

static node_t *create_inverter(nodelist, fanin, inv_gate) lsList nodelist;
node_t *fanin;
lib_gate_t *inv_gate;
{
  node_t *inv;
  char *formals[1];
  node_t *actuals[1];

  inv = node_alloc();
  actuals[0] = fanin;
  formals[0] = lib_gate_pin_name(inv_gate, 0, 1);
  assert(lib_set_gate(inv, inv_gate, formals, actuals, 1));
  LS_ASSERT(lsNewEnd(nodelist, (char *)inv, LS_NH));
  return inv;
}

static void add_inverters(nodelist, node, inv_gate) lsList nodelist;
node_t *node;
lib_gate_t *inv_gate;
{
  int new_inv;
  node_t *inv1, *inv, *fanout;
  lsGen gen;

  /* Find if the node already has an inverter at the output */
  inv = NIL(node_t);
  foreach_fanout(node, gen, fanout) {
    if (node_function(fanout) == NODE_INV) {
      if (inv != NIL(node_t)) {
        /*
        (void) fprintf(stderr,
            "Warning: node %s has fanout to >1 inverter\n",
            node_name(node));
         */
      } else {
        inv = fanout;
      }
    }
  }

  /* Create an inverter if none found so far */
  new_inv = 0;
  if (inv == NIL(node_t)) {
    inv = create_inverter(nodelist, node, inv_gate);
    new_inv = 1;
  }

  /* Handle the positive-phase fanouts (i.e., fanouts of 'node') */
  inv1 = create_inverter(nodelist, inv, inv_gate);
  foreach_fanout(node, gen, fanout) {
    if (node_function(fanout) != NODE_INV) {
      assert(node_patch_fanin(fanout, node, inv1));
    }
  }

  /* Handle the positive-phase fanouts (i.e., fanouts of 'node') */
  if (!new_inv) {
    inv1 = create_inverter(nodelist, inv1, inv_gate);
    foreach_fanout(inv, gen, fanout) {
      if (node_function(fanout) != NODE_INV) {
        assert(node_patch_fanin(fanout, inv, inv1));
      }
    }
  }
}

buffer_inputs(network, library) network_t *network;
library_t *library;
{
  lib_gate_t *inv_gate;
  lsList nodelist;
  lsGen gen;
  node_t *pi, *node;

  inv_gate = choose_largest_gate(library, "f = !a;");

  nodelist = lsCreate();
  foreach_primary_input(network, gen, pi) {
    add_inverters(nodelist, pi, inv_gate);
  }
  lsForeachItem(nodelist, gen, node) { network_add_node(network, node); }

  LS_ASSERT(lsDestroy(nodelist, (void (*)())

                                    0));

  (void)network_cleanup(network);
}
