
/* file @(#)maputil.c	1.8 */
/* last modified on 8/8/91 at 17:02:09 */
/*
 * $Log: maputil.c,v $
 * Revision 1.1.1.1  2004/02/07 10:14:25  pchong
 * imported
 *
 * Revision 1.5  1993/08/05  16:14:19  sis
 * Added default case to switch statement so gcc version will work.
 *
 * Revision 1.4  22/.0/.1  .0:.4:.5  sis
 *  Updates for the Alpha port.
 *
 * Revision 1.3  1992/05/06  18:55:51  sis
 * SIS release 1.1
 *
 * Revision 1.2  1992/04/17  21:59:58  sis
 * *** empty log message ***
 *
 * Revision 1.1  92/01/08  17:40:50  sis
 * Initial revision
 *
 * Revision 1.2  91/07/02  11:18:01  touati
 * replace MAP(node)->arrival_info from pointers to copies.
 * Slower but safer.
 *
 * Revision 1.1  91/06/28  22:50:25  touati
 * Initial revision
 *
 */
#include "gate_link.h"
#include "map_int.h"
#include "sis.h"

void map_alloc(node) node_t *node;
{
  if (MAP(node) != 0)
    map_free(node);
#if defined(_IBMR2) || defined(__osf__)
  node->MAP_SLOT = (char *)ALLOC(alt_map_t, 1);
#else
  node->MAP_SLOT = (char *)ALLOC(map_t, 1);
#endif
  MAP(node)->gate = 0;
  MAP(node)->ninputs = 0;
  MAP(node)->save_binding = 0;
  MAP(node)->arrival_info = NIL(delay_time_t);
  MAP(node)->binding = 0;
  MAP(node)->match_node = 0;
  MAP(node)->inverter_paid_for = 0;
  MAP(node)->slack_is_critical = 0;
  MAP(node)->index = 0;
  MAP(node)->load = 0.0;
  MAP(node)->required.fall = INFINITY;
  MAP(node)->required.rise = INFINITY;
  MAP(node)->node_y = NIL(node_t);
  MAP(node)->gate_link = NIL(st_table);
  gate_link_init(node);
  MAP(node)->fanout_tree = NIL(array_t);
  MAP(node)->fanout_source = NIL(node_t);
  MAP(node)->pin_info = NIL(fanin_fanout_t);
}

void map_free(node) node_t *node;
{
  if (MAP(node) != 0) {
    gate_link_free(node);
    if (MAP(node)->ninputs) {
      FREE(MAP(node)->save_binding);
    }
    if (MAP(node)->pin_info) {
      FREE(MAP(node)->pin_info);
    }
    if (MAP(node)->arrival_info) {
      FREE(MAP(node)->arrival_info);
      MAP(node)->arrival_info = NIL(delay_time_t);
    }
    FREE(node->MAP_SLOT);
    node->MAP_SLOT = NIL(char);
  }
}

void map_dup(old, new) node_t *old, *new;
{
  int i;

  if (MAP(old) == 0)
    return;
  map_alloc(new);
  gate_link_free(new);
  *MAP(new) = *MAP(old);
  if (MAP(old)->ninputs) {
    MAP(new)->save_binding = ALLOC(node_t *, MAP(old)->ninputs);
    for (i = 0; i < MAP(old)->ninputs; i++) {
      MAP(new)->save_binding[i] = MAP(old)->save_binding[i];
    }
  }
  MAP(new)->pin_info = NIL(fanin_fanout_t);
  MAP(new)->arrival_info = NIL(delay_time_t);
  MAP(new)->gate_link = NIL(st_table);
  gate_link_init(new);
}

void map_invalid(node) node_t *node;
{
  if (MAP(node) != 0) {
    MAP(node)->gate = 0;
  }
}

void map_setup_network(network) network_t *network;
{
  lsGen gen;
  node_t *node;

  foreach_node(network, gen, node) { map_alloc(node); }
}

void map_prep(network, library) network_t *network;
library_t *library;
{
  decomp_quick_network(network);
  if (library->nand_flag) {
    decomp_tech_network(network, /* and_limit */ 0, /* or_limit */ 2);
  } else {
    decomp_tech_network(network, /* and_limit */ 2, /* or_limit */ 0);
  }
  add_inv_network(network);
}

/* this should be the new network */

void map_network_dup(network) network_t *network;
{
  int i;
  lsGen gen;
  node_t *node;

  foreach_node(network, gen, node) {
    if (MAP(node) == 0)
      continue;
    if (MAP(node)->ninputs == 0)
      continue;
    for (i = 0; i < MAP(node)->ninputs; i++) {
      MAP(node)->save_binding[i] = (MAP(node)->save_binding[i])->copy;
    }
  }
}

/* to take into account the new way of handling the input of a PO */

node_t *map_po_get_fanin(node) node_t *node;
{
  assert(node->type == PRIMARY_OUTPUT);
#ifdef SIS
  if ((MAP(node)->ninputs > 0) && !(IS_LATCH_END(node))) {
#else
  if (MAP(node)->ninputs > 0) {
#endif /* SIS */
    return MAP(node)->save_binding[0];
  } else {
    return node_get_fanin(node, 0);
  }
}

#ifdef SIS
/* sequential support */

/* obtain the gate type information of the network */
int network_gate_type(network) network_t *network;
{
  int *type;

  if (st_lookup(network_type_table, network_name(network), (char **)&type)) {
    return (*type);
  }
  return ((int)UNKNOWN);
}

node_t *network_latch_input(network) network_t *network;
{
  lsGen gen;
  node_t *po, *latch_input;

  latch_input = NIL(node_t);
  foreach_primary_output(network, gen, po) {
    if (IS_LATCH_END(po)) {
      assert(latch_input == NIL(node_t)); /* only one latch input */
      latch_input = po;
      LS_ASSERT(lsFinish(gen));
      break;
    }
  }
  return latch_input;
}
#endif
