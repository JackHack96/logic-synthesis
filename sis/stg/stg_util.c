
#ifdef SIS
#include "sis.h"
#include "stg_int.h"

void stg_dump_graph(graph, network) graph_t *graph;
network_t *network;
{
  lsGen gen, gen2;
  vertex_t *re_no;
  node_t *node;
  edge_t *re_ed;

  if (graph == NIL(graph_t)) {
    (void)fprintf(sisout, "NIL stg\n");
    return;
  }
#ifdef STG_DEBUG
  if (!stg_check(graph)) {
    (void)fprintf(sisout, "Graph failed stg_check\n");
    return;
  }
#endif
  (void)fprintf(sisout, "\nInitial state %s\n",
                stg_get_state_name(stg_get_start(graph)));
  if (network != NIL(network_t)) {
    (void)fprintf(sisout, "PI list: ");
    foreach_primary_input(network, gen, node) {
      if (network_latch_end(node) == NIL(node_t))
        (void)fprintf(sisout, "%s  ", node_name(node));
    }
    (void)fprintf(sisout, "\nPO list: ");
    foreach_primary_output(network, gen, node) {
      if (network_latch_end(node) == NIL(node_t))
        (void)fprintf(sisout, "%s  ", node_name(node));
    }
    (void)fprintf(sisout, "\n");
  }
  (void)fprintf(sisout, "PresentState input  output NextState\n");
  stg_foreach_state(graph, gen,
                    re_no){foreach_state_outedge(re_no, gen2, re_ed){
      (void)fprintf(sisout, "%s %s %s %s\n", stg_get_state_name(re_no),
                    stg_edge_input_string(re_ed), stg_edge_output_string(re_ed),
                    stg_get_state_name(stg_edge_to_state(re_ed)));
}
}
(void)fprintf(sisout, "\n");
}

static array_t *
    stg_assign_names(flag, n) int flag; /* == 1 => inputs , otherwise outputs */
int n; /* Number of names that need to be assigned */
{
  int i;
  char *name, name_buf[16];
  array_t *name_array;

  if (n <= 0)
    return NIL(array_t);
  name_array = array_alloc(char *, n);
  for (i = 0; i < n; i++) {
    (void)sprintf(name_buf, "%s_%d", (flag ? "IN" : "OUT"), i);
    name = util_strsav(name_buf);
    array_insert_last(char *, name_array, name);
  }
  return name_array;
}

int stg_save_names(network, stg, print_error) network_t *network;
graph_t *stg;
int print_error;
{
  char *name;
  lsGen gen;
  node_t *node;
  array_t *name_array;
  int netw_pi, stg_pi, netw_po, stg_po;
  stg_clock_t *clock_data;
  sis_clock_t *clock;
  clock_edge_t edge;

  if (stg == NIL(graph_t) || network == NIL(network_t))
    return;

  /* Save the output names */
  if ((name_array = stg_get_names(stg, 0)) == NIL(array_t)) {
    /* Unfortunately, this is the only way to find number of true PO's */
    netw_po = 0;
    foreach_primary_output(network, gen, node) {
      if (network_is_real_po(network, node)) {
        netw_po++;
      }
    }
    stg_po = stg_get_num_outputs(stg);
    if (netw_po > 0) {
      if (netw_po != stg_po) {
        (void)fprintf(
            siserr,
            "Number of outputs in the STG and the BLIF file do not match.\n");
        (void)fprintf(siserr, "Output names from the BLIF file are ignored.\n");
        return 1;
      }
      name_array = array_alloc(char *, 0);
      foreach_primary_output(network, gen, node) {
        if (network_is_real_po(network, node)) {
          name = util_strsav(node_long_name(node));
          array_insert_last(char *, name_array, name);
        }
      }
    } else {
      /* Make up names for the STG output */
      name_array = stg_assign_names(0, stg_po);
    }
    stg_set_names(stg, name_array, 0);
  }

  /* Save the input names now */
  if ((name_array = stg_get_names(stg, 1)) == NIL(array_t)) {
    netw_pi = network_num_pi(network) - network_num_latch(network) -
              network_num_clock(network);
    stg_pi = stg_get_num_inputs(stg);
    if (netw_pi > 0) {
      if (netw_pi != stg_pi) {
        if (print_error) {
          /* This error message gets printed twice during read_blif,
             this switch prevents that.  The same is not true of the
                     outputs because they aren't present the first time through
           */
          (void)fprintf(
              siserr,
              "Number of inputs in the STG and the BLIF file do not match.\n");
          (void)fprintf(siserr,
                        "Input names from the BLIF file are ignored.\n");
        }
        return 1;
      }
      name_array = array_alloc(char *, 0);
      foreach_primary_input(network, gen, node) {
        /* Avoid latch inputs and clocks */
        if (network_is_real_pi(network, node) &&
            clock_get_by_name(network, node_long_name(node)) == 0) {
          name = util_strsav(node_long_name(node));
          array_insert_last(char *, name_array, name);
        }
      }
    } else {
      /* Make up names for the STG inputs */
      name_array = stg_assign_names(1, stg_pi);
    }
    stg_set_names(stg, name_array, 1);
  }
  /* One should also save the clock data -- KJ 7/22/93
   * else we will lose track of the clocking info. supplied with the
   * .clock construct when the kiss file is specified with the .inputs
   * and .outputs ... When more thna 1 clock ignore the clock setting
   */
  if (network_num_clock(network) == 1) {
    if ((clock_data = stg_get_clock_data(stg)) != NIL(stg_clock_t)) {
      if (clock_data->name != NIL(char))
        FREE(clock_data->name);
      FREE(clock_data);
    }
    clock_data = ALLOC(stg_clock_t, 1);
    clock_data->cycle_time = clock_get_cycletime(network);
    foreach_clock(network, gen, clock) {
      clock_data->name = util_strsav(clock_name(clock));
      edge.clock = clock;
      edge.transition = RISE_TRANSITION;
      clock_data->nominal_rise =
          clock_get_parameter(edge, CLOCK_NOMINAL_POSITION);
      clock_data->min_rise = clock_get_parameter(edge, CLOCK_LOWER_RANGE);
      clock_data->max_rise = clock_get_parameter(edge, CLOCK_UPPER_RANGE);
      edge.transition = FALL_TRANSITION;
      clock_data->nominal_fall =
          clock_get_parameter(edge, CLOCK_NOMINAL_POSITION);
      clock_data->min_fall = clock_get_parameter(edge, CLOCK_LOWER_RANGE);
      clock_data->max_fall = clock_get_parameter(edge, CLOCK_UPPER_RANGE);
    }
    stg_set_clock_data(stg, clock_data);
  }
  return 0;
}

/*
 * Routine that will find a PI/PO node from the dont-care network (dcnet)
 * having the same name as the node "node" in the care-network. Then will
 * change the name of the dont-care node to be "name"
 */
static void stg_change_dc_node_name(dcnet, node, name) network_t *dcnet;
node_t *node;
char *name;
{
  node_t *dc_node;

  if (dcnet != NIL(network_t)) {
    dc_node = network_find_node(dcnet, node_long_name(node));
    if (dc_node != NIL(node_t)) {
      network_change_node_name(dcnet, dc_node, util_strsav(name));
    }
  }
}
/*
 * Use the PI/PO names stored with the STG as the names of network PI/PO
 * Assume that the order of PI's and PO's is unchanged....
 * In case the network has aSTG associated ith it, the names for this STG are
 * also made compatible with the true network names....
 */
void stg_set_network_pipo_names(network, stg) network_t *network;
graph_t *stg;
{
  int i;
  int modify_names;
  lsGen gen;
  graph_t *stg1;
  char *name, *name1;
  network_t *dcnet;
  node_t *node;
  array_t *name_array, *name_array1;

  if (stg == NIL(graph_t)) {
    /* This should never happen... just a safety feature */
    return;
  }
  stg1 = network_stg(network);
  dcnet = network_dc_network(network);
  modify_names = (stg1 != NIL(graph_t) && stg1 != stg);

  /* Get the names of the PI's and change names of the network PI */
  /*
   * We have to be careful here... In an example, the PO name was the same
   * as that of a next_state output name assigned by "nova". To be sure that
   * things are always correct, we will first invalidate the names of the
   * primary inputs and outputs by appending them. Then, we will add new
   * corect names for PI's/PO's that were stored along with the STG.
   */
  foreach_primary_input(network, gen, node) {
    name = ALLOC(char, strlen(node_long_name(node)) + 10);
    (void)sprintf(name, "LatchOut_%s", node_long_name(node));
    stg_change_dc_node_name(dcnet, node, name);
    network_change_node_name(network, node, name);
  }

  assert((name_array = stg_get_names(stg, 1)) != NIL(array_t));
  i = 0;
  if (modify_names)
    name_array1 = stg_get_names(stg1, 1);
  foreach_primary_input(network, gen, node) {
    if (network_is_real_pi(network, node) &&
        clock_get_by_name(network, node_long_name(node)) == 0) {
      name = array_fetch(char *, name_array, i);
      stg_change_dc_node_name(dcnet, node, name);
      network_change_node_name(network, node, util_strsav(name));
      if (modify_names) {
        name1 = array_fetch(char *, name_array1, i);
        FREE(name1);
        array_insert(char *, name_array1, i, util_strsav(name));
      }
      i++;
    }
  }
  if (network_num_pi(network) == 0) {
    /* This is a special case - this routine is being called from
       com_state_minimize.  The network is empty, and there is no
       dc network. */
    for (i = 0; i < array_n(name_array); i++) {
      name = array_fetch(char *, name_array, i);
      node = node_alloc();
      network_add_primary_input(network, node);
      network_change_node_name(network, node, util_strsav(name));
    }
  }

  /* Change the PO names too --- append prefix to the names first */
  assert((name_array = stg_get_names(stg, 0)) != NIL(array_t));
  i = 0;
  if (modify_names)
    name_array1 = stg_get_names(stg1, 0);
  foreach_primary_output(network, gen, node) {
    name = ALLOC(char, strlen(node_long_name(node)) + 10);
    (void)sprintf(name, "LatchIn_%s", node_long_name(node));
    stg_change_dc_node_name(dcnet, node, name);
    network_change_node_name(network, node, name);
  }
  foreach_primary_output(network, gen, node) {
    if (network_is_real_po(network, node)) {
      name = array_fetch(char *, name_array, i);
      stg_change_dc_node_name(dcnet, node, name);
      network_change_node_name(network, node, util_strsav(name));
      if (modify_names) {
        name1 = array_fetch(char *, name_array1, i);
        FREE(name1);
        array_insert(char *, name_array1, i, util_strsav(name));
      }
      i++;
    }
  }
  if (network_num_po(network) == 0) {
    /* This is a special case - this routine is being called from
       com_state_minimize.  The network is empty, and there is no
       dc network. */
    for (i = 0; i < array_n(name_array); i++) {
      name = array_fetch(char *, name_array, i);
      node = node_constant(0);
      node->name = util_strsav(name);
      network_add_node(network, node);
      (void)network_add_primary_output(network, node);
    }
  }
}
#endif /* SIS */
