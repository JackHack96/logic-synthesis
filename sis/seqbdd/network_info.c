
#ifdef SIS
#include "sis.h"

static array_t *network_extract_extern_pi();
static void allocate_and_rename_primary_inputs();
static void extract_local_pipo();
static void extract_product_pi_info();
static void merge_io_outputs();
static void remember_primary_output_names();
static void rename_internal_nodes();
static void hack_rename_internal_nodes();
static void rename_primary_outputs();

/* network: is the FSM transition + output fn network */
/* if range computation, just that; if verification, product machine */
/* constraints: a simple network with only PI and PO; specifies */
/* the initial state, the list of PS and NS variables, and the correspondance */
/* between PS (present state) and NS (next state). The NS are PO fed by */
/* the corresponding PS, which is represented as a PI. */

void extract_network_info(network, options) network_t *network;
verif_options_t *options;
{
  char *name;
  node_t *node;
  array_t *next_state_po;
  output_info_t *output_info = options->output_info;

  if (options->verbose >= 1)
    report_elapsed_time(options, "extract network info");
  output_info->org_pi = network_extract_pi(network);
  output_info->extern_pi = network_extract_extern_pi(network);
  output_info->init_node = copy_init_state_constraint(network);
  next_state_po = network_extract_next_state_po(network);
  switch (options->type) {
  case BULL_METHOD:
    output_info->po_ordering = get_po_ordering(network, next_state_po, options);
    array_free(next_state_po);
    break;
  case CONSISTENCY_METHOD:
    output_info->po_ordering = get_po_ordering(network, next_state_po, options);
    array_free(next_state_po);
    name = util_strsav("consistency:output");
    output_info->new_pi = create_new_pi(network, output_info->po_ordering);
    if (output_info->is_product_network) { /* verification */
      extract_product_pi_info(network, output_info, name);
    } else { /* range computation */
      node =
          build_equivalence_node(network, output_info->new_pi,
                                 output_info->po_ordering, name, NIL(array_t));
      output_info->main_node = network_add_primary_output(network, node);
    }
    output_info->pi_ordering =
        get_pi_ordering(network, output_info->po_ordering, output_info->new_pi);
    if (options->use_manual_order)
      get_manual_order(output_info->pi_ordering, options);
    break;
  case PRODUCT_METHOD: /* keep the transition relation as independent products;
                          no special case here */
    output_info->po_ordering = get_po_ordering(network, next_state_po, options);
    array_free(next_state_po);
    output_info->new_pi = create_new_pi(network, output_info->po_ordering);
    output_info->transition_nodes = array_alloc(node_t *, 0);
    node = build_equivalence_node(network, output_info->new_pi,
                                  output_info->po_ordering, NIL(char),
                                  output_info->transition_nodes);
    network_delete_node(network, node);
    output_info->main_node = NIL(node_t);
    output_info->pi_ordering =
        get_pi_ordering(network, output_info->po_ordering, output_info->new_pi);
    if (options->use_manual_order)
      get_manual_order(output_info->pi_ordering, options);
    break;
  default:
    fail("unregistered range computation method");
  }
}

void extract_product_network_info(network1, network2,
                                  options) network_t *network1;
network_t *network2;
verif_options_t *options;
{
  if (options->verbose >= 1)
    report_elapsed_time(options, "compute product network");
  compute_product_network(network1, network2, options->output_info);
  extract_network_info(network2, options);
}

/* put the product and the product constraints on network2/constraint2 */
/* return the output created by merging IO_OUTPUTS together and AND and XNOR's
 */
/* all tables work by name; good both for networki and constraintsi */
/* fill the "xnor_nodes" array of external outputs */
/* as well as the "output_node" */

void compute_product_network(network1, network2,
                             output_info) network_t *network1;
network_t *network2;
output_info_t *output_info;
{
  lsGen gen;
  st_generator *sgen;
  char *name;
  node_t *output, *n1, *n2;
  latch_t *l1, *l2;
  int value;
  st_table *node_table = st_init_table(st_ptrcmp, st_ptrhash);
  st_table *input_table = st_init_table(strcmp, st_strhash);
  st_table *output_table = st_init_table(strcmp, st_strhash);

  allocate_and_rename_primary_inputs(network1, network2, input_table, ":0");
  rename_primary_outputs(network2, output_table, ":0");
  rename_internal_nodes(network2, ":0");
  hack_rename_internal_nodes(network1, network2, ":0");
  remember_primary_output_names(network1, output_info, 0);
  remember_primary_output_names(network2, output_info, 1);

  foreach_primary_output(network1, gen, output) {
    (void)network_copy_subnetwork(network2, output, input_table, node_table);
  }
  merge_io_outputs(network1, network2, output_table, output_info);
  st_free_table(input_table);
  st_foreach_item(output_table, sgen, &name, NIL(char *)) { FREE(name); }
  st_free_table(output_table);
  st_free_table(node_table);
  /* copy latches from network1 to network2*/
  foreach_latch(network1, gen, l1) {
    n1 = latch_get_input(l1);
    n2 = latch_get_output(l1);
    n1 = network_find_node(network2, n1->name);
    n2 = network_find_node(network2, n2->name);
    network_create_latch(network2, &l2, n1, n2);
    value = latch_get_initial_value(l1);
    latch_set_initial_value(l2, value);
  }
}

/* for each PI of network1 that does not appear as output of a latch*/
/* find the corresponding PI in network2 and store (name, net2_pi) in
 * input_table  */
/* for each PI of network1 that appears as a PI in constraints1 */
/* rename the corresponding PI in network2 if any and add a new PI in network2
 */
/* again, store the correspondance in input_table */
/* renaming is done by appending the string ":0" to the PI name */

static void allocate_and_rename_primary_inputs(network1, network2, input_table,
                                               postfix) network_t *network1;
network_t *network2;
st_table *input_table;
char *postfix;
{
  lsGen gen;
  int postfix_length = (int)strlen(postfix) + 1;
  node_t *input, *new_input;

  foreach_primary_input(network1, gen, input) {
    if (latch_from_node(input) == NIL(latch_t)) {
      assert(new_input = network_find_node(network2, input->name));
    } else {
      if (new_input = network_find_node(network2, input->name)) {
        char *new_name = ALLOC(char, (int)strlen(input->name) + postfix_length);
        (void)sprintf(new_name, "%s%s", input->name, postfix);
        network_change_node_name(network2, new_input, new_name);
        /*
         * Also ensure that if needed the reference in the dc_network is
         * also changed. This is required to allow the subsequent addition
         * of new_input to retain the name input->name in case the latter is
         * a madeup_name. The routine network_add_node() checks to see if the
         * name being assigned is absent from both the care and dc networks !!!
         */
        if ((network2->dc_network != NIL(network_t)) &&
            (new_input =
                 network_find_node(network2->dc_network, input->name))) {
          network_change_node_name(network2->dc_network, new_input,
                                   util_strsav(new_name));
        }
      }
      new_input = node_alloc();
      new_input->name = util_strsav(input->name);
      network_add_primary_input(network2, new_input);
    }
    st_insert(input_table, input->name, (char *)new_input);
  }
}

/* rename each PO of "network" by appending a ":0" at the end */
/* this is to avoid conflicts with other network */
/* keeps the correspondance between old name and nodes in "output_table" */

static void rename_primary_outputs(network, output_table,
                                   postfix) network_t *network;
st_table *output_table;
char *postfix;
{
  lsGen gen;
  node_t *output;
  int postfix_length = (int)strlen(postfix) + 1;

  foreach_primary_output(network, gen, output) {
    char *old_name = util_strsav(output->name);
    char *new_name = ALLOC(char, (int)strlen(old_name) + postfix_length);
    (void)sprintf(new_name, "%s%s", old_name, postfix);
    network_change_node_name(network, output, new_name);
    st_insert(output_table, old_name, (char *)output);
  }
}

static void rename_internal_nodes(network, postfix) network_t *network;
char *postfix;
{
  char *new_name;
  lsGen gen;
  node_t *node;
  int postfix_length = (int)strlen(postfix) + 1;

  foreach_node(network, gen, node) {
    if (node->type != INTERNAL)
      continue;
    new_name = ALLOC(char, strlen(node->name) + postfix_length);
    (void)sprintf(new_name, "%s%s", node->name, postfix);
    network_change_node_name(network, node, new_name);
  }
}

/* This is a hack.  There is a special case in which verify_fsm
   core dumps.  If n1 has an internal node with the same name as
   a pi in n2 that is a latch output, a core dump occurs.  This fixes
   that problem. */

static void hack_rename_internal_nodes(n1, n2, postfix) network_t *n1, *n2;
char *postfix;
{
  char *new_name;
  lsGen gen;
  node_t *node, *node2;
  int postfix_length = (int)strlen(postfix) + 1;

  foreach_node(n1, gen, node) {
    if (node->type != INTERNAL)
      continue;
    if ((node2 = network_find_node(n2, node->name)) != NIL(node_t)) {
      new_name = ALLOC(char, strlen(node->name) + postfix_length);
      (void)sprintf(new_name, "%s%s", node->name, postfix);
      network_change_node_name(n1, node, new_name);
    }
  }
}

/* only those outputs that do not figure in constraints1 */
/* fill the "xnor_nodes" array of external outputs */
/* as well as the "output_node" */

static void merge_io_outputs(network1, network2, output_table,
                             output_info) network_t *network1;
network_t *network2;
st_table *output_table;
output_info_t *output_info;
{
  int i;
  lsGen gen;
  char *name = EXTERNAL_OUTPUT_NAME;
  node_t *output, *new_output, *matching_output;
  array_t *io_outputs1 = array_alloc(node_t *, 0);
  array_t *io_outputs2 = array_alloc(node_t *, 0);
  array_t *io_fanin1 = array_alloc(node_t *, 0);
  array_t *io_fanin2 = array_alloc(node_t *, 0);

  foreach_primary_output(network1, gen, output) {
    if (!network_is_real_po(network1, output))
      continue;
    assert(new_output = network_find_node(network2, output->name));
    assert(new_output->type == PRIMARY_OUTPUT);
    array_insert_last(node_t *, io_outputs1, new_output);
    new_output = node_get_fanin(new_output, 0);
    assert(st_lookup(output_table, output->name, (char **)&matching_output));
    assert(matching_output->type == PRIMARY_OUTPUT);
    array_insert_last(node_t *, io_outputs2, matching_output);
    matching_output = node_get_fanin(matching_output, 0);
    array_insert_last(node_t *, io_fanin1, new_output);
    array_insert_last(node_t *, io_fanin2, matching_output);
  }
  if (array_n(io_outputs1) == 0)
    return;
  output_info->xnor_nodes = array_alloc(node_t *, 0);
  output = build_equivalence_node(network2, io_fanin1, io_fanin2, name,
                                  output_info->xnor_nodes);
  if (output_info->generate_global_output) {
    output_info->output_node = network_add_primary_output(network2, output);
  } else {
    network_delete_node(network2, output);
    output_info->output_node = NIL(node_t);
    for (i = 0; i < array_n(output_info->xnor_nodes); i++) {
      output = array_fetch(node_t *, output_info->xnor_nodes, i);
      (void)network_add_primary_output(network2, output);
    }
  }
  for (i = 0; i < array_n(io_outputs1); i++) {
    output = array_fetch(node_t *, io_outputs1, i);
    network_delete_node(network2, output);
    output = array_fetch(node_t *, io_outputs2, i);
    network_delete_node(network2, output);
  }
  array_free(io_outputs1);
  array_free(io_outputs2);
  array_free(io_fanin1);
  array_free(io_fanin2);
}

/* convention: all PO of "network" that have the same name as PO of "input" */
/* except for the PO of "input" called INIT_STATE_OUTPUT_NAME are next_state PO
 */

array_t *network_extract_next_state_po(network) network_t *network;
{
  lsGen gen;
  node_t *n1;
  latch_t *l;
  array_t *result;

  result = array_alloc(node_t *, 0);

  foreach_latch(network, gen, l) {
    n1 = latch_get_input(l);
    array_insert_last(node_t *, result, n1);
  }
  return result;
}

/* simply puts the PI of a network in an array */

array_t *network_extract_pi(network) network_t *network;
{
  lsGen gen;
  node_t *input;
  array_t *result = array_alloc(node_t *, 0);

  foreach_primary_input(network, gen, input) {
    array_insert_last(node_t *, result, input);
  }
  return result;
}

/* is extern_pi iff pi and name does not appear in constraint */

static array_t *network_extract_extern_pi(network) network_t *network;
{
  lsGen gen;
  node_t *input;
  array_t *result = array_alloc(node_t *, 0);

  foreach_primary_input(network, gen, input) {
    if (network_is_real_pi(network, input))
      array_insert_last(node_t *, result, input);
  }
  return result;
}

/* copy the init state output and its inputs in network */
/* match the PI's by name */

node_t *copy_init_state_constraint(network) network_t *network;
{
  lsGen gen;
  node_t *init_state;
  node_t *n1, *n2, *n3;
  latch_t *l;
  int value;

  init_state = node_constant(1);
  foreach_latch(network, gen, l) {
    value = latch_get_initial_value(l);
    switch (value) {
    case 0:
    case 1:
      n1 = latch_get_output(l);
      n2 = node_literal(n1, value);
      n3 = node_and(init_state, n2);
      node_free(n2);
      node_free(init_state);
      init_state = n3;
      break;
    case 2:
      break;
    default:
      (void)fprintf(
          sisout,
          "Latch with input: %s and output: %s is not properly initialized. \n",
          node_name(latch_get_input(l)), node_name(latch_get_output(l)));
      node_free(init_state);
      return (NIL(node_t));
    }
  }
  network_add_node(network, init_state);
  network_change_node_name(network, init_state,
                           util_strsav(INIT_STATE_OUTPUT_NAME));
  return network_add_primary_output(network, init_state);
}

/* create one new PI per node in outputs */

array_t *create_new_pi(network, outputs) network_t *network;
array_t *outputs;
{
  int i;
  int n_new_pi = array_n(outputs);
  array_t *new_pi = array_alloc(node_t *, 0);

  for (i = 0; i < n_new_pi; i++) {
    node_t *node = node_alloc();
    node_t *output = array_fetch(node_t *, outputs, i);
    char *buffer = ALLOC(char, (int)strlen(output->name) + 10);

    (void)sprintf(buffer, "%s:y%d", output->name, i);
    node->name = util_strsav(buffer);
    network_add_primary_input(network, node);
    array_insert_last(node_t *, new_pi, node);
    FREE(buffer);
  }
  return new_pi;
}

/* takes the two lists of nodes of "network", merge them together into XNOR
 * gates */
/* connect the results of those XNOR gates into a AND gate, and return the
 * resulting node */
/* the name of the resulting node is given by the output_name argument if non
 * NIL */
/* if xnor_array is non NIL, the intermediate XNOR nodes are saved there */
/* if any node is a PO, it is replaced by its unique fanin */

node_t *build_equivalence_node(network, nodes1, nodes2, output_name,
                               xnor_array) network_t *network;
array_t *nodes1;
array_t *nodes2;
char *output_name;
array_t *xnor_array;
{
  int i;
  int n_nodes = array_n(nodes1);
  node_t *a, *b, *and_node, *new_and_node;
  node_t **xnor_nodes = ALLOC(node_t *, n_nodes);

  assert(array_n(nodes1) == array_n(nodes2));
  for (i = 0; i < n_nodes; i++) {
    a = array_fetch(node_t *, nodes1, i);
    b = array_fetch(node_t *, nodes2, i);
    if (a->type == PRIMARY_OUTPUT)
      a = node_get_fanin(a, 0);
    if (b->type == PRIMARY_OUTPUT)
      b = node_get_fanin(b, 0);
    a = node_literal(a, 1);
    b = node_literal(b, 1);
    xnor_nodes[i] = node_xnor(a, b);
    node_free(a);
    node_free(b);
    network_add_node(network, xnor_nodes[i]);
  }
  if (n_nodes == 0) {
    and_node = node_constant(1);
  } else if (n_nodes == 1) {
    and_node = node_literal(xnor_nodes[0], 1);
    if (xnor_array)
      array_insert_last(node_t *, xnor_array, xnor_nodes[0]);
  } else {
    for (i = 0; i < n_nodes; i++) {
      xnor_nodes[i] = node_literal(xnor_nodes[i], 1);
    }
    and_node = node_and(xnor_nodes[0], xnor_nodes[1]);
    for (i = 2; i < n_nodes; i++) {
      new_and_node = node_and(and_node, xnor_nodes[i]);
      node_free(and_node);
      and_node = new_and_node;
    }
    for (i = 0; i < n_nodes; i++) {
      if (xnor_array != NIL(array_t)) {
        network_add_node(network, xnor_nodes[i]);
        array_insert_last(node_t *, xnor_array, xnor_nodes[i]);
      } else {
        node_free(xnor_nodes[i]);
      }
    }
  }
  network_add_node(network, and_node);
  if (output_name) {
    network_change_node_name(network, and_node, util_strsav(output_name));
  }
  FREE(xnor_nodes);
  return and_node;
}

/* build the consistency output for each of the two networks separately */
/* record the result in output_info, as well as their "AND" */
/* the problem is that we need to extract info on the networks from their
 * product ("network") */

static void extract_product_pi_info(network, output_info,
                                    name) network_t *network;
output_info_t *output_info;
char *name;
{
  int i;
  node_t *node;
  array_t *local_pi, *local_po;

  for (i = 0; i < 2; i++) {
    local_pi = array_alloc(node_t *, 0);
    local_po = array_alloc(node_t *, 0);
    extract_local_pipo(output_info, local_pi, local_po, i);
    output_info->main_nodes[i] = build_equivalence_node(
        network, local_pi, local_po, NIL(char), NIL(array_t));
    array_free(local_pi);
    array_free(local_po);
  }
  node = node_and(output_info->main_nodes[0], output_info->main_nodes[1]);
  network_add_node(network, node);
  network_change_node_name(network, node, name);
  output_info->main_node = network_add_primary_output(network, node);
}

static void remember_primary_output_names(network, info,
                                          value) network_t *network;
output_info_t *info;
int value;
{
  lsGen gen;
  node_t *output;

  if (info->name_table == NIL(st_table)) {
    info->name_table = st_init_table(strcmp, st_strhash);
  }
  foreach_primary_output(network, gen, output) {
    st_insert(info->name_table, output->name, (char *)value);
  }
}

/* given info->po_ordering: i.e. PO of product network in some order */
/* and info->new_pi: matching PI (newly allocated) in same order */
/* and info->name_table: table of (char *) for PO mapping to the netid of their
 * original net */
/* extract in local_pi/local_po those PI-PO pairs for a given netid */
static void extract_local_pipo(info, local_pi, local_po,
                               netid) output_info_t *info;
array_t *local_pi;
array_t *local_po;
int netid;
{
  int i;
  int id;
  node_t *pi, *po;

  for (i = 0; i < array_n(info->po_ordering); i++) {
    po = array_fetch(node_t *, info->po_ordering, i);
    assert(st_lookup_int(info->name_table, po->name, &id));
    if (id != netid)
      continue;
    array_insert_last(node_t *, local_po, po);
    pi = array_fetch(node_t *, info->new_pi, i);
    array_insert_last(node_t *, local_pi, pi);
  }
}
#endif /* SIS */
