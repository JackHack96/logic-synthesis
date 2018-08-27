
/*---------------------------------------------------------------------------
|      This file contains routines that evaluate the power dissipation
|  in various types of static sequential circuits.
|
|        power_seq_static_zero()
|        power_seq_static_unit()
|        power_seq_static_arbit()
|        power_add_fsm_state_logic()
|
| Copyright (c) 1991 - Abhijit Ghosh. University of California, Berkeley
|
| Jose' Monteiro, MIT, Jan/93            jcm@rle-vlsi.mit.edu
|  - Modified to be included in SIS (instead of Flames)
|  - Added exact and approximate methods, calculating respectively
|    state probabilities and PS lines probabilities
+--------------------------------------------------------------------------*/

#ifdef SIS

#include "power_int.h"
#include "sis.h"

static void power_place_PS_lines_first();
static int power_network_concatenate();

/* This is the main routine for the evaluation of power in sequential circuits
 */
int power_seq_static_arbit(network, info_table, pslOption,
                           total_power) network_t *network;
st_table *info_table;
int pslOption;
double *total_power;
{
  st_table *leaves = st_init_table(st_ptrcmp, st_ptrhash), *stateIndex,
           *stateLineIndex;
  array_t *piOrder, *psOrder, *poArray;
  bdd_manager *manager;
  bdd_t *bdd;
  network_t *symbolic;
  node_t *po, *pi, *node, *orig_node;
  power_info_t *power_info;
  node_info_t *node_info;
  power_pi_t *PIInfo;
  int i, index;
  double prob_node_one, *stateProb, *psLineProb;
  char *key, *value;
  st_generator *sgen;
  lsGen gen;

  /* First thing to do is to get the symbolic network */
  symbolic = power_symbolic_simulate(network, info_table);

  /* Generate PI_ttt through next state logic */
  if (power_add_fsm_state_logic(network, symbolic))
    return 1;

  /* Calculate state/PSlines probability */
  switch (pslOption) {
  case EXACT_PS:
    stateProb = power_exact_state_prob(network, info_table, &stateIndex,
                                       &stateLineIndex);
    break;
  case STATELINE_PS: /* Line prob. are changed inside */
    power_PS_lines_from_state(network, info_table);
    break;
  case APPROXIMATION_PS: /* Line prob. are changed inside if set size = 1 */
    psLineProb =
        power_direct_PS_lines_prob(network, info_table, &psOrder, symbolic);
    break;
  case UNIFORM_PS:
    break;
    /* Do nothing: default value! */
  }

  /* Now the probability calculation for the symbolic circuit */
  poArray = array_alloc(node_t *, 0);
  foreach_primary_output(symbolic, gen, po) {
    array_insert_last(node_t *, poArray, po);
  }
  piOrder = order_nodes(poArray, /* PI's only */ 1);
  if (piOrder == NIL(array_t))
    piOrder = array_alloc(node_t *, 0);
  array_free(poArray);

  switch (pslOption) {
  case EXACT_PS:
    power_place_PS_lines_first(&piOrder, stateLineIndex);
    break;
  case APPROXIMATION_PS:
    if (power_setSize != 1)
      power_place_PIs_last(&piOrder, psOrder);
  }

  manager = ntbdd_start_manager(3 * network_num_pi(symbolic));

  PIInfo = ALLOC(power_pi_t, network_num_pi(symbolic));
  for (i = 0; i < array_n(piOrder); i++) {
    pi = array_fetch(node_t *, piOrder, i);
    st_insert(leaves, (char *)pi, (char *)i);
    orig_node = pi->copy; /* Link to the original network */
    assert(st_lookup(info_table, (char *)orig_node, (char **)&node_info));
    PIInfo[i].probOne = node_info->prob_one;
    if (pslOption == EXACT_PS)
      if (st_lookup(stateLineIndex, (char *)pi->copy, (char **)&index))
        PIInfo[i].PSLineIndex = index;
      else
        PIInfo[i].PSLineIndex = -1;
  }
  array_free(piOrder);

  *total_power = 0;
  foreach_primary_output(symbolic, gen, po) {
    node = po->fanin[0]; /* The actual node we are looking at */
    bdd = ntbdd_node_to_bdd(node, manager, leaves);
    switch (pslOption) {
    case EXACT_PS:
      prob_node_one = power_calc_func_prob_w_stateProb(
          bdd, PIInfo, stateProb, stateIndex, st_count(stateLineIndex));
      break;
    case APPROXIMATION_PS:
      if (power_setSize == 1)
        prob_node_one = power_calc_func_prob(bdd, PIInfo);
      else
        prob_node_one = power_calc_func_prob_w_sets(bdd, PIInfo, psLineProb,
                                                    array_n(psOrder));
      break;
    default:
      prob_node_one = power_calc_func_prob(bdd, PIInfo);
    }
    orig_node = node->copy; /* Correspondence to the orig. network */
    assert(
        st_lookup(power_info_table, (char *)orig_node, (char **)&power_info));
    *total_power += power_info->cap_factor * prob_node_one * CAPACITANCE;
    power_info->switching_prob += prob_node_one;
    if (power_verbose > 50)
      fprintf(sisout, "Node %s Probability %f\n", node_name(node),
              prob_node_one);
  }
  *total_power *= 250.0; /* The 0.5 Vdd ^2 factor for a 5V Vdd */

  ntbdd_end_manager(manager);
  st_free_table(leaves);
  FREE(PIInfo);
  network_free(symbolic);
  switch (pslOption) {
  case EXACT_PS:
    FREE(stateProb);
    st_foreach_item(stateIndex, sgen, &key, &value) { FREE(key); }
    st_free_table(stateIndex);
    st_free_table(stateLineIndex);
    break;
  case APPROXIMATION_PS:
    FREE(psLineProb);
    array_free(psOrder);
  }

  return 0;
}

int power_add_fsm_state_logic(network, symbolic) network_t *network;
network_t *symbolic;
{
  st_table *po_table, *pi_table, *ps_table;
  array_t *poArray;
  network_t *ns_logic;
  node_t *node, *pi, *po;
  int i;
  char buffer[100];
  lsGen gen;

  /* Get the network corresponding to the NS logic */
  ns_logic = network_dup(network);

  /* Now delete the real PO gates */
  poArray = array_alloc(node_t *, 0);
  foreach_primary_output(ns_logic, gen, po) {
    array_insert_last(node_t *, poArray, po);
  }
  for (i = 0; i < array_n(poArray); i++) {
    node = array_fetch(node_t *, poArray, i);
    if (network_latch_end(node) == NIL(node_t)) {
      /* This is a pure PO */
      network_delete_node(ns_logic, node);
    }
  }
  array_free(poArray);

  /* Delete all the nodes that don't go anywhere */
  network_csweep(ns_logic);

  /* Now have to concatenate the networks */
  /* However, we need to fill up the tables first */
  po_table = st_init_table(st_ptrcmp, st_ptrhash);
  pi_table = st_init_table(st_ptrcmp, st_ptrhash);
  ps_table = st_init_table(st_ptrcmp, st_ptrhash);

  foreach_primary_output(ns_logic, gen, po) {
    node = network_latch_end(po);
    sprintf(buffer, "%s_ttt", node->name);
    pi = network_find_node(symbolic, buffer);
    st_insert(po_table, (char *)po, (char *)pi);
    sprintf(buffer, "%s_000", node->name);
    pi = network_find_node(symbolic, buffer);
    st_insert(ps_table, (char *)node, (char *)pi);
  }
  foreach_primary_input(ns_logic, gen, pi) {
    if (network_latch_end(pi) == NIL(node_t)) {
      /* It is definitely a PI */
      sprintf(buffer, "%s_000", pi->name);
      node = network_find_node(symbolic, buffer);
      if (node != NIL(node_t))
        st_insert(pi_table, (char *)pi, (char *)node);
    }
  }

  power_network_concatenate(symbolic, ns_logic, po_table, ps_table, pi_table);

  st_free_table(po_table);
  st_free_table(pi_table);
  st_free_table(ps_table);
  network_free(ns_logic);

  /* Delete all the PI's that don't go anywhere */
  poArray = array_alloc(node_t *, 0);
  foreach_primary_input(symbolic, gen, pi) {
    if (node_num_fanout(pi) == 0) {
      array_insert_last(node_t *, poArray, pi);
    }
  }
  for (i = 0; i < array_n(poArray); i++) {
    node = array_fetch(node_t *, poArray, i);
    network_delete_node(symbolic, node);
  }
  array_free(poArray);

  return 0;
}

static void power_place_PS_lines_first(piOrder,
                                       stateLineIndex) array_t **piOrder;
st_table *stateLineIndex;
{
  array_t *PSlines, *PIlines;
  node_t *node;
  int i;
  char *dummy;

  PSlines = array_alloc(node_t *, 0);
  PIlines = array_alloc(node_t *, 0);

  for (i = 0; i < array_n(*piOrder); i++) {
    node = array_fetch(node_t *, *piOrder, i);
    if (st_lookup(stateLineIndex, (char *)node->copy, &dummy))
      array_insert_last(node_t *, PSlines, node);
    else
      array_insert_last(node_t *, PIlines, node);
  }
  array_append(PSlines, PIlines);

  array_free(PIlines);
  array_free(*piOrder);
  *piOrder = PSlines;
}

static int power_network_concatenate(
    symbolic_net, ns_logic, po_link_table, ps_link_table,
    pi_link_table) network_t *symbolic_net; /* Will be changed */
network_t *ns_logic;                        /* Will not be freed */
st_table *po_link_table;                    /* Will not be freed */
st_table *ps_link_table;                    /* Will not be freed */
st_table *pi_link_table;                    /* Will not be freed */
{
  st_table *correspondance = st_init_table(st_ptrcmp, st_ptrhash);
  array_t *nodeArray;
  node_t *fanin, *node, *node2, *new_node, *fanout;
  int i, j;
  char *value, *key, *buffer;
  st_generator *sgen;
  lsGen gen;

  /* There is no guarantee that the names in each net are unique. Therefore
     add the prefix ns_ to each name in the ns_logic network */
  nodeArray = power_network_dfs(ns_logic);
  for (i = 0; i < array_n(nodeArray); i++) {

    node = array_fetch(node_t *, nodeArray, i);

    /* First change the name of the node */
    buffer = ALLOC(char, (int)strlen(node->name) + 5);
    sprintf(buffer, "%s_nsl", node->name);
    network_change_node_name(ns_logic, node, buffer);

    new_node = node_dup(node);
    network_add_node(symbolic_net, new_node);
    st_insert(correspondance, (char *)node, (char *)new_node);

    if (node_function(node) == NODE_PI)
      continue;

    if (node_function(node) == NODE_PO) {
      /* Patch the fanin right */
      fanin = node->fanin[0];
      st_lookup(correspondance, (char *)fanin, (char **)&node);
      node_patch_fanin(new_node, fanin, node);
      continue;
    }

    /* Came here => internal node */
    /*Patch its fanins */
    foreach_fanin(new_node, j, fanin) {
      st_lookup(correspondance, (char *)fanin, (char **)&node);
      node_patch_fanin(new_node, fanin, node);
    }
  }
  array_free(nodeArray);

  /* Now we have copied the NS logic network into the symbolic network */
  /* The next thing to do is to link them up right using the link tables */

  if (po_link_table != NIL(st_table)) {
    /* In the link table, the key is a PO node in the ns_logic net and
       the value is a PI node in the symbolic net */
    /* Link the PO nodes of NS Logic with some inputs of the symbolic net */
    st_foreach_item(po_link_table, sgen, (char **)&key, (char **)&value) {
      node = (node_t *)key;       /* PO node in NS logic */
      new_node = (node_t *)value; /* PI node in symbolic */
      st_lookup(correspondance, (char *)node, &value);
      node = (node_t *)value;
      if ((node_function(new_node) != NODE_PI) ||
          (node_function(node) != NODE_PO)) {
        fprintf(siserr, "error in performing linkup of PO node's \n");
        return 1;
      }
      node2 = node->fanin[0];
      foreach_fanout(new_node, gen, fanout) {
        node_patch_fanin(fanout, new_node, node2);
      }
      network_delete_node(symbolic_net, new_node);
      network_delete_node(symbolic_net, node);
    }
  }

  if (ps_link_table != NIL(st_table)) {
    /* In the link table, the key is a PI in the ns_logic net, and the
       value is a PI in the symbolic net */
    /* Link the PS nodes of NS Logic with some PS nodes of Symbolic net. */
    st_foreach_item(ps_link_table, sgen, (char **)&key, (char **)&value) {
      node = (node_t *)key;       /* PI node in NS logic */
      new_node = (node_t *)value; /* PI node in symbolic net */
      st_lookup(correspondance, (char *)node, &value);
      node = (node_t *)value;
      if ((node_function(new_node) != NODE_PI) ||
          (node_function(node) != NODE_PI)) {
        fprintf(siserr, "error in performing linkup of PS node's \n");
        return 1;
      }
      node2 = node;
      foreach_fanout(new_node, gen, fanout) {
        node_patch_fanin(fanout, new_node, node2);
      }
      node->copy = new_node->copy; /* Keep link to original network */
      network_delete_node(symbolic_net, new_node);
    }
  }

  if (pi_link_table != NIL(st_table)) {
    /* In the link table, the key is a PI in the ns_logic net, and the
       value is a PI in the symbolic net */
    /* Link the PS nodes of NS Logic with some PS nodes of Symbolic net. */
    st_foreach_item(pi_link_table, sgen, (char **)&key, (char **)&value) {
      node = (node_t *)key;       /* PI node in NS Logic */
      new_node = (node_t *)value; /* PI node in symbolic net */
      st_lookup(correspondance, (char *)node, &value);
      node = (node_t *)value;
      if ((node_function(new_node) != NODE_PI) ||
          (node_function(node) != NODE_PI)) {
        fprintf(siserr, "error in performing linkup of PI node's \n");
        return 1;
      }
      node2 = node;
      foreach_fanout(new_node, gen, fanout) {
        node_patch_fanin(fanout, new_node, node2);
      }
      node->copy = new_node->copy; /* Keep link to original network */
      network_delete_node(symbolic_net, new_node);
    }
  }

  /* Check to make sure that network thing is valid */
  assert(network_check(symbolic_net));

  st_free_table(correspondance);

  return 0;
}

#endif /* SIS */
