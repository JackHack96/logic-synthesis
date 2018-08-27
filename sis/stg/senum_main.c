
#ifdef SIS
/**********************************************************************/
/*           This is the main routine for senum                       */
/**********************************************************************/

#include "sis.h"
#include "stg_int.h"

/*
 * Global variables for this package ---
 */
network_t *copy;
ndata **stg_pstate, **stg_nstate, **real_po;
int *stg_estate;
int nlatch, npi, npo;
int stg_longs_per_state, stg_bits_per_long;
int total_no_of_states;
long total_no_of_edges;
unsigned *unfinish_head, *hashed_state;
st_table *slist;
st_table *state_table;
st_table *node_to_ndata_table;
int n_varying_nodes;
ndata **varying_node;

extern sis_clock_t *clock_get_transitive_clock(); /* Should be in clock.h */
static stg_clock_t *stg_get_clock_info();
static int stg_valid_network();
static void stg_remove_control_logic();
/*
 * Routines needed to replace the use of the undef1 field of node
 */

ndata *nptr(node) node_t *node;
{
  char *dummy;
  if (st_lookup(node_to_ndata_table, (char *)node, &dummy)) {
    return ((ndata *)dummy);
  } else {
    return NIL(ndata);
  }
}

void setnptr(node, value) node_t *node;
ndata *value;
{ (void)st_insert(node_to_ndata_table, (char *)node, (char *)value); }

graph_t *stg_extract(network, ctable) network_t *network;
int ctable;
{
  unsigned *unfinish_next, *compact_state;
  int index;
  graph_t *stg;
  st_generator *generator;
  char *key, *value, *clk_name;
  node_t *control_node;
  sis_clock_t *clock;
  stg_clock_t *clk_data; /* Storage for the clocking information */
  lsGen gen;
  latch_t *l;
  int init_val;
  char *init_state;
  vertex_t *i_state;
  int nl, i, edge_index;

  /*    msec = util_cpu_time(); */
  if (!network_check(network)) {
    (void)fprintf(siserr, "Error in network\n");
    (void)fprintf(siserr, "%s\n", error_string());
    return NIL(graph_t);
  }

  copy = network_dup(network);
  if (!network_check(copy)) {
    (void)fprintf(siserr, "Error in copy\n");
    (void)fprintf(siserr, "%s\n", error_string());
    goto error_exit;
  }
  /*
   * first do a network sweep.  This will get rid of latches that
   * fanout nowhere and parallel latches.
   */
  (void)network_sweep(copy);

  /*
   * Check if the network is one we can handle
   */
  if (!stg_valid_network(copy, &control_node, &clk_name, &edge_index)) {
    (void)fprintf(siserr, "STG cannot be extracted for this topology\n");
    goto error_exit;
  }

  /* Now remove the clock signal and control logic from the network */
  stg_remove_control_logic(copy);

  /*
   * Decompose the network into AND gates with as most 16 inputs
   */
  decomp_tech_network(copy, 16, 0);
  if (!ctable) {
    foreach_latch(copy, gen, l) {
      init_val = latch_get_initial_value(l);
      if (init_val != 0 && init_val != 1) {
        (void)fprintf(
            siserr,
            "Network must have a single initial state to extract the STG\n");
        lsFinish(gen);
        goto error_exit;
      }
    }
  } else {
    /* Save the encoding string of the initial state. */
    /* The reason for this is that stg_extract -a looks at all */
    /* states, not just the initial state.  Later we need to set */
    /* the initial state in the new stg to what was given in the
       /* blif file. */
    nl = network_num_latch(copy);
    init_state = ALLOC(char, nl + 1);
    i = -1;
    foreach_latch(copy, gen, l) {
      i++;
      init_val = latch_get_initial_value(l);
      switch (init_val) {
      case 0:
        init_state[i] = '0';
        break;
      case 1:
        init_state[i] = '1';
        break;
      case 2:
        init_state[i] = '2';
        break;
      case 3:
        init_state[i] = '3';
        break;
      default:
        (void)fprintf(siserr, "Unknown latch value %i\n", init_val);
        goto error_exit;
      }
    }
    i++;
    init_state[i] = '\0';
  }

  total_no_of_edges = total_no_of_states = 0;

  stg_free(network_stg(network));
  network_set_stg(network, NIL(graph_t));
  stg = stg_alloc();
  slist = st_init_table(stg_statecmp, stg_statehash);
  state_table = st_init_table(strcmp, st_strhash);
  node_to_ndata_table = st_init_table(st_ptrcmp, st_ptrhash);

  nlatch = network_num_latch(copy);
  npi = network_num_pi(copy) - nlatch;
  npo = network_num_po(copy) - nlatch;

  stg_bits_per_long = 8 * sizeof(unsigned);
  stg_longs_per_state = 1 + ((nlatch - 1) / stg_bits_per_long);
  hashed_state = ALLOC(unsigned, stg_longs_per_state);
  stg_init_state_hash();

  n_varying_nodes = 0;
  varying_node = ALLOC(ndata *, network_num_internal(copy) + npi);

  stg_estate = SENUM_ALLOC(int, nlatch);
  stg_pstate = ALLOC(ndata *, nlatch);
  stg_nstate = ALLOC(ndata *, nlatch);
  real_po = ALLOC(ndata *, npo);

  level_circuit();
  rearrange_gate_inputs();
  if (ctable) {
    ctable_enum(stg);
    i_state = stg_get_state_by_name(stg, init_state);
    if (i_state == NIL(vertex_t)) {
      fail("Initial state not found in extracted stg");
    }
    stg_set_start(stg, i_state);
    stg_set_current(stg, i_state);
    FREE(init_state);
  } else {
    unfinish_head = shashcode(stg_estate);
    (void)st_insert(slist, (char *)unfinish_head, (char *)(NIL(unsigned *)));
    index = 1;
    do {
      /*
        (void) fprintf(sisout,"Enumeration number %d starting at ",index);
        stg_print_hashed_state(sisout, unfinish_head);
        (void) fprintf(sisout,"\n");
        */
      if (index > 1) {
        compact_state = unfinish_head;
        stg_translate_hashed_code(compact_state, stg_estate);
      }
      unfinish_next = unfinish_head;
      (void)st_lookup(slist, (char *)unfinish_next, (char **)&unfinish_head);
      enumerate(0, stg_estate, stg);
      index++;
    } while (unfinish_head != NIL(unsigned));
  }
  FREE(stg_estate);
  FREE(stg_pstate);
  FREE(stg_nstate);
  FREE(varying_node);
  FREE(real_po);
  stg_end_state_hash();
  st_free_table(slist);
  st_free_table(state_table);
  st_foreach_item(node_to_ndata_table, generator, &key, &value) { FREE(value); }
  st_free_table(node_to_ndata_table);
  /*
    foreach_node(copy, gen, node){
    FREE(nptr(node));
    }
    */
  network_free(copy);

  stg_set_num_inputs(stg, npi);
  stg_set_num_outputs(stg, npo);
  stg_set_num_products(stg, total_no_of_edges);
  stg_set_num_states(stg, total_no_of_states);
  clk_data = stg_get_clock_info(network);
  stg_set_clock_data(stg, clk_data);
  stg_set_edge_index(stg, edge_index);
  network_set_stg(network, stg);

  /*
    (void) fprintf(sisout,"Total number of states = %d\n",total_no_of_states);
    (void) fprintf(sisout,"Total number of edges = %d\n",total_no_of_edges);
    (void) fprintf(sisout,"Total time = %g\n",
    (double) (util_cpu_time() - msec) / 1000);
    */
#ifdef STG_DEBUG
  if (!stg_check(stg)) {
    stg_free(network_stg(network));
    stg = NIL(graph_t);
    network_set_stg(network, NIL(graph_t));
  }
#endif
  return (stg);

error_exit:
  network_free(copy);
  return NIL(graph_t);
}

/*
 * Check if the network is one we can handle: The following are checked.
 * --- Only a single clock should be present
 * --- This clock should not be gated. No inputs common to clocking & logic
 * --- The latches controlled by this clock should be edge triggered
 * --- The latches should all be controlled by the same signal
 * --- No real PO or latch input depends on the clock signal
 */
static int stg_valid_network(network, control_node, clock_namep,
                             edge_indexp) network_t *network;
node_t **control_node;
char **clock_namep;
int *edge_indexp;
{
  int i;
  lsGen gen;
  char *name;
  latch_synch_t type, new_type;
  node_t *node, *control, *new_control;
  array_t *nodevec;
  sis_clock_t *clock;
  delay_time_t delay;
  input_phase_t phase;
  latch_t *latch;

  /* Initializations */
  *clock_namep = NIL(char);
  *edge_indexp = -1;
  *control_node = NIL(node_t);

  /* Check for a single clock */
  if (network_num_clock(network) > 1) {
    (void)fprintf(siserr, "Multiple clocks found int the design\n");
    return 0;
  }
  /*
   * Now check to see if all the latches are single phase and
   * clocked by the same control node
   */
  type = UNKNOWN;
  control = NIL(node_t);
  foreach_latch(network, gen, latch) {
    new_control = latch_get_control(latch);
    new_type = latch_get_type(latch);
    if (new_control != NIL(node_t)) {
      if (control == NIL(node_t)) {
        control = new_control;
      } else if (control != new_control) {
        (void)fprintf(siserr, "Different signals control latches\n");
        return 0;
      }
    }
    /* Now check the sytnchronization types */
    if (new_type != UNKNOWN && new_type != RISING_EDGE &&
        new_type != FALLING_EDGE) {
      (void)fprintf(siserr, "Latch type other than edge-triggered found\n");
      return 0;
    } else if (type != UNKNOWN && type != new_type) {
      (void)fprintf(siserr, "Latches of different types present in design\n");
      return 0;
    } else if (new_type == RISING_EDGE || new_type == FALLING_EDGE) {
      type = new_type;
    }
  }
  /*
   * At this stage we have a unique signal type and a unique type of latch
   * Now see if there is any interaction between the logic and the control
   */
  if (control != NIL(node_t)) {
    nodevec = network_tfi(control, INFINITY);
    for (i = array_n(nodevec); i-- > 0;) {
      node = array_fetch(node_t *, nodevec, i);
      if (network_is_real_pi(network, node) &&
          clock_get_by_name(network, node_long_name(node)) == 0) {
        (void)fprintf(siserr, "Gated clocks found in the design\n");
        array_free(nodevec);
        return 0;
      }
    }
    array_free(nodevec);
    clock = clock_get_transitive_clock(network, control, DELAY_MODEL_UNIT,
                                       &delay, &phase);
  }

  /*
   * It could be the case that the latches are not controlled by anything
   * yet the clock signals fan out (transitively) to some PO or latch...
   */
  if (control == NIL(node_t) && network_num_clock(network) == 1) {
    foreach_clock(network, gen, clock) { name = clock_name(clock); }
    node = network_find_node(network, name);
    nodevec = network_tfo(node, INFINITY);
    for (i = array_n(nodevec); i-- > 0;) {
      node = array_fetch(node_t *, nodevec, i);
      if (node->type == PRIMARY_OUTPUT && !network_is_control(network, node)) {
        (void)fprintf(siserr, "Path from Clock to latch input or PO found\n");
        array_free(nodevec);
        return 0;
      }
    }
    array_free(nodevec);
  }
  if (control != NIL(node_t)) {
    /* Set latch type based on the phase of the clock relationship */
    if (phase == NEG_UNATE) {
      if (type == RISING_EDGE) {
        type = FALLING_EDGE;
      } else {
        type = RISING_EDGE;
      }
    } else if (phase == BINATE) {
      (void)fprintf(
          siserr,
          "Phase relationship between clock and control is not unique\n");
      return 0;
    }
  }
  /*
   * Now save the names of the clocks and an index that is indicative
   * of the clocking edge (unknown = -1, rise = 0; fall = 1)
   */
  if (control != NIL(node_t)) {
    *clock_namep = util_strsav(clock_name(clock));
    if (type == RISING_EDGE) {
      *edge_indexp = 0;
    } else if (type == FALLING_EDGE) {
      *edge_indexp = 1;
    } else {
      *edge_indexp = -1;
      (void)fprintf(sisout, "This should never happen\n");
    }
  }
  *control_node = control;

  return 1;
}

/*
 * Based on a valid network structure, simply remove the control logic
 * Guarantees that the deleted nodes do not fanout to logic....
 */
static void stg_remove_control_logic(network) network_t *network;
{
  int i;
  lsGen gen;
  latch_t *latch;
  sis_clock_t *clock;
  array_t *nodevec;
  node_t *node, *control_node;

  foreach_latch(network, gen, latch) { latch_set_control(latch, NIL(node_t)); }
  control_node = NIL(node_t);
  foreach_clock(network, gen, clock) {
    control_node = network_find_node(network, clock_name(clock));
  }
  if (control_node == NIL(node_t))
    return;

  /* Traverse transitive fanin  --- deleting nodes closer to PO first */
  nodevec = network_tfo(control_node, INFINITY);
  for (i = 0; i < array_n(nodevec); i++) {
    node = array_fetch(node_t *, nodevec, i);
    network_delete_node(network, node);
  }
  network_delete_node(network, control_node);
  return;
}

static stg_clock_t *stg_get_clock_info(network) network_t *network;
{
  lsGen gen;
  clock_edge_t edge;
  stg_clock_t *clk_data;
  sis_clock_t *clock;

  if (network_num_clock(network) != 1)
    return NIL(stg_clock_t);

  foreach_clock(network, gen, clock) {
    lsFinish(gen);
    break;
    /* NOT REACHED */
  }

  clk_data = ALLOC(stg_clock_t, 1);
  clk_data->name = util_strsav(clock_name(clock));
  clk_data->cycle_time = clock_get_cycletime(network);
  edge.clock = clock;

  edge.transition = RISE_TRANSITION;
  clk_data->nominal_rise = clock_get_parameter(edge, CLOCK_NOMINAL_POSITION);
  clk_data->min_rise = clock_get_parameter(edge, CLOCK_LOWER_RANGE);
  clk_data->max_rise = clock_get_parameter(edge, CLOCK_UPPER_RANGE);

  edge.transition = FALL_TRANSITION;
  clk_data->nominal_fall = clock_get_parameter(edge, CLOCK_NOMINAL_POSITION);
  clk_data->min_fall = clock_get_parameter(edge, CLOCK_LOWER_RANGE);
  clk_data->max_fall = clock_get_parameter(edge, CLOCK_UPPER_RANGE);

  return clk_data;
}

#endif /* SIS */
