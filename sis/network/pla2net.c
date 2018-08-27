
#include "sis.h"

network_t *pla_to_network(PLA) pPLA PLA;
{
  network_t *network;
  node_t *node, *node1, **inputs, **pterms;
  int out, ninputs, npterms;
  register pset p, pnew;
  register int var, i, k;

  /* we can only handle binary-valued PLAs */
  if (cube.num_binary_vars + 1 != cube.num_vars)
    return 0;

  network = network_alloc();

  /* Create a node for each primary input */
  ninputs = 0;
  inputs = ALLOC(node_t *, cube.num_binary_vars);
  for (var = 0; var < cube.num_binary_vars; var++) {
    if (network_find_node(network, PLA->label[2 * var + 1]) != NIL(node_t)) {
      (void)fprintf(siserr,
                    "Error reading pla: node %s is already in the network.\n",
                    PLA->label[2 * var + 1]);
      network_free(network);
      FREE(inputs);
      return NIL(network_t);
    }
    node = node_alloc();
    node->name = util_strsav(PLA->label[2 * var + 1]);
    inputs[ninputs++] = node;
    network_add_primary_input(network, node);
  }

  /* Create a node for each first-level NOR-gate */
  npterms = 0;
  pterms = ALLOC(node_t *, PLA->F->count);
  foreachi_set(PLA->F, i, p) {

    /* Allocate the node */
    node = node_create(sf_new(1, 2 * ninputs), nodevec_dup(inputs, ninputs),
                       ninputs);
    network_add_node(network, node);
    pterms[npterms++] = node;

    /* Insert the (inverted inputs) NOR-logic function into the node */
    pnew = GETSET(node->F, node->F->count++);
    (void)set_fill(pnew, 2 * ninputs);
    for (var = ninputs - 1; var >= 0; var--) {
      switch (GETINPUT(p, var)) {
      case ONE:
        set_remove(pnew, 2 * var);
        break;
      case ZERO:
        set_remove(pnew, 2 * var + 1);
        break;
      }
    }
    node_scc(node);
  }

  /* Create a node (and an output) for each second-level NAND-gate */
  for (out = 0; out < cube.part_size[cube.output]; out++) {
    i = cube.first_part[cube.num_vars - 1] + out;

    /* Allocate the node */
    node = node_create(sf_new(1, 2 * npterms), nodevec_dup(pterms, npterms),
                       npterms);
    network_add_node(network, node);

    /* Allocate and insert an extra inverter node */
    if (network_find_node(network, PLA->label[i]) != NIL(node_t)) {
      (void)fprintf(siserr,
                    "Error reading pla: node %s is already in the network.\n",
                    PLA->label[i]);
      network_free(network);
      FREE(inputs);
      FREE(pterms);
      return NIL(network_t);
    }
    node1 = node_literal(node, 0);
    node1->name = util_strsav(PLA->label[i]);
    network_add_node(network, node1);

    /* allocate and insert the PO node */
    (void)network_add_primary_output(network, node1);

    /* Insert the NOR-logic function into the node */
    pnew = GETSET(node->F, node->F->count++);
    (void)set_fill(pnew, 2 * npterms);
    foreachi_set(PLA->F, k, p) {
      if (is_in_set(p, i)) {
        set_remove(pnew, 2 * k + 1);
      }
    }
    node_scc(node);
  }

  FREE(inputs);
  FREE(pterms);

  return network;
}

network_t *pla_to_network_single(PLA) pPLA PLA;
{
  network_t *network;
  node_t *node, **inputs;
  int out, ninputs;
  register pset last, p, pnew;
  register int var, i;

  /* we can only handle binary-valued PLAs */
  if (cube.num_binary_vars + 1 != cube.num_vars)
    return 0;

  network = network_alloc();

  /* Create a node for each primary input */
  ninputs = 0;
  inputs = ALLOC(node_t *, cube.num_binary_vars);
  for (var = 0; var < cube.num_binary_vars; var++) {
    if (network_find_node(network, PLA->label[2 * var + 1]) != NIL(node_t)) {
      (void)fprintf(siserr,
                    "Error reading pla: node %s is already in the network.\n",
                    PLA->label[2 * var + 1]);
      network_free(network);
      FREE(inputs);
      return NIL(network_t);
    }
    node = node_alloc();
    node->name = util_strsav(PLA->label[2 * var + 1]);
    inputs[ninputs++] = node;
    network_add_primary_input(network, node);
  }

  /* Create a node for each function */
  for (out = 0; out < cube.part_size[cube.output]; out++) {
    i = cube.first_part[cube.num_vars - 1] + out;

    /* Allocate the node */
    if (network_find_node(network, PLA->label[i]) != NIL(node_t)) {
      (void)fprintf(siserr,
                    "Error reading pla: node %s is already in the network.\n",
                    PLA->label[i]);
      network_free(network);
      FREE(inputs);
      return NIL(network_t);
    }
    node = node_create(sf_new(20, 2 * ninputs), nodevec_dup(inputs, ninputs),
                       ninputs);
    node->name = util_strsav(PLA->label[i]);
    network_add_node(network, node);

    /* allocate and insert the PO node */
    (void)network_add_primary_output(network, node);

    foreach_set(PLA->F, last, p) {
      if (is_in_set(p, i)) {

        /* copy this cube to this node */
        pnew = set_fill(set_new(2 * ninputs), 2 * ninputs);
        for (var = ninputs - 1; var >= 0; var--) {
          switch (GETINPUT(p, var)) {
          case ONE:
            set_remove(pnew, 2 * var);
            break;
          case ZERO:
            set_remove(pnew, 2 * var + 1);
            break;
          }
        }
        node->F = sf_addset(node->F, pnew);
        set_free(pnew);
      }
    }
    node_scc(node); /* make scc and min-base */
  }

  FREE(inputs);

  return network;
}

pPLA espresso_read_pla(fp) FILE *fp;
{
  pPLA PLA;

  undefine_cube_size();
  if (read_pla(fp, TRUE, FALSE, FD_type, &PLA) <= 0) {
    return 0;
  }
  makeup_labels(PLA);

  return PLA;
}

network_t *pla_to_dcnetwork_single(PLA) pPLA PLA;
{
  network_t *network;
  node_t *node, **inputs;
  int out, ninputs;
  register pset last, p, pnew;
  register int var, i;

  /* we can only handle binary-valued PLAs */
  if (cube.num_binary_vars + 1 != cube.num_vars)
    return 0;
  if (PLA->D->count == 0) {
    return (NIL(network_t));
  }

  network = network_alloc();

  /* Create a node for each primary input */
  ninputs = 0;
  inputs = ALLOC(node_t *, cube.num_binary_vars);
  for (var = 0; var < cube.num_binary_vars; var++) {
    node = node_alloc();
    node->name = util_strsav(PLA->label[2 * var + 1]);
    inputs[ninputs++] = node;
    network_add_primary_input(network, node);
  }

  /* Create a node for each function */
  for (out = 0; out < cube.part_size[cube.output]; out++) {
    i = cube.first_part[cube.num_vars - 1] + out;

    /* Allocate the node */
    node = node_create(sf_new(20, 2 * ninputs), nodevec_dup(inputs, ninputs),
                       ninputs);
    node->name = util_strsav(PLA->label[i]);
    network_add_node(network, node);

    /* allocate and insert the PO node */
    (void)network_add_primary_output(network, node);

    foreach_set(PLA->D, last, p) {
      if (is_in_set(p, i)) {

        /* copy this cube to this node */
        pnew = set_fill(set_new(2 * ninputs), 2 * ninputs);
        for (var = ninputs - 1; var >= 0; var--) {
          switch (GETINPUT(p, var)) {
          case ONE:
            set_remove(pnew, 2 * var);
            break;
          case ZERO:
            set_remove(pnew, 2 * var + 1);
            break;
          }
        }
        node->F = sf_addset(node->F, pnew);
        set_free(pnew);
      }
    }
    node_scc(node); /* make scc and min-base */
  }

  FREE(inputs);

  return network;
}
