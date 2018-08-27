
/* file @(#)treemap.c	1.9 */
/* last modified on 7/22/91 at 12:36:35 */
#include "fanout_int.h"
#include "lib_int.h"
#include "map_delay.h"
#include "map_int.h"
#include "map_macros.h"
#include "sis.h"
#include <math.h>

static int do_tree_match();

static int find_best_match();

static void find_match_cost();

static void match_cost();

static node_t *build_network_recur();

static int inverter_optimize();

static int global_debug, global_inverter_optz;
static double global_area_mode, global_thresh;
static int g_allow_internal_fanout;
static bin_global_t global;

/* debugger support ... till we get something better than DBX */
/* typedef char * STRING;
 * static char myname[512];
 * static char *mystr;
 * static lib_gate_t *mygate;
 * static lsGen mygen;
 * static node_t *mynode;
 */

static double max_arrival_estimate;
static double area_estimate;

#ifdef SIS
/* sequential support */
static int is_buffer();
static char *prim_gate_name();
static int is_transitive_fanin();
#if !defined(lint) && !defined(SABER)
static void map_print_network();
#endif
static void hack_po();
#endif

/*
 *  exported interface ...
 */

network_t *map_network(network, library, mode, inv_optz,
                       allow_internal_fanout) network_t *network;
library_t *library;
double mode;
int inv_optz, allow_internal_fanout;
{
  bin_global_t globals;
  int argc;
  char **argv;

  argc = 0;
  argv = NIL(char *);
  (void)map_fill_options(&globals, &argc, &argv);
  globals.old_mode = mode;
  globals.remove_inverters = 1;
  globals.inverter_optz = inv_optz;
  globals.allow_internal_fanout = allow_internal_fanout;
  globals.library = library;
  globals.no_warning = 1;

  return complete_map_interface(network, &globals);
}

/*
 *  tree_map -- implement 'network' using the tree-matching algorithm
 */
int tree_map(network, library, globals) network_t *network;
library_t *library;
bin_global_t globals;
{
  global = globals;
  global.library = library;
  global_debug = globals.verbose;      /* needed for find_best_match() ! */
  global_area_mode = globals.old_mode; /* needed for find_best_match() ! */
  global_thresh = globals.thresh;
  global_inverter_optz =
      globals.inverter_optz; /* needed for find_best_match() ! */
  g_allow_internal_fanout = globals.allow_internal_fanout;

  area_estimate = 0.0;        /* for final heuristics checking */
  max_arrival_estimate = 0.0; /* for final heuristics checking */

  return do_tree_match(network, library, &globals);
}

/*
 *  premap: set each node to be a nand gate or an inverter
 *  we do this regardless of whether we are using 'nand' gates or 'nor'
 *  gates in order to time the circuit based on nand gates; note that
 *  this may change the logic function (if we stopped now); however,
 *  all pattern matching, etc. is strictly topological based.
 */

int do_tree_premap(network, library) network_t *network;
library_t *library;
{
  lsGen gen;
  network_t *libnet;
  node_t *pi, *node;
  lib_gate_t *gate, *zero, *one, *inv, *nand2;
  char **inv_formal, **nand2_formal;
  delay_time_t slack;
  double real_slack;
  int i;

  /* Give each node a 'map' record */
  map_setup_network(network);

  /* find standard gates */
  nand2 = choose_smallest_gate(library, "f=!(a*b);");
  if (nand2 == NIL(lib_gate_t)) {
    nand2 = choose_smallest_gate(library, "f=!(a+b);");
    if (nand2 == NIL(lib_gate_t)) {
      nand2 = choose_smallest_gate(library, "f=a*b;"); /* hack */
      if (nand2 == NIL(lib_gate_t)) {
        nand2 = choose_smallest_gate(library, "f=a+b;"); /* hack */
        if (nand2 == NIL(lib_gate_t)) {
          error_append("library error: cannot find 2-input nand-gate\n");
          return 0;
        }
      }
    }
  }
  inv = choose_smallest_gate(library, "f=!a;");
  if (inv == NIL(lib_gate_t)) {
    error_append("library error: cannot find inverter\n");
    return 0;
  }
  zero = choose_smallest_gate(library, "f=0;");
  if (zero == NIL(lib_gate_t)) {
    error_append("library error: cannot find constant 0 cell\n");
    return 0;
  }
  one = choose_smallest_gate(library, "f=1;");
  if (one == NIL(lib_gate_t)) {
    error_append("library error: cannot find constant 1 cell\n");
    return 0;
  }

  /* create the formal arglist for inv and nand2 */
  inv_formal = ALLOC(char *, 1);
  i = 0;
  libnet = lib_class_network(lib_gate_class(inv));
  foreach_primary_input(libnet, gen, pi) { inv_formal[i++] = pi->name; }

  /* create the formal arglist for inv and nand2 */
  nand2_formal = ALLOC(char *, 2);
  i = 0;
  libnet = lib_class_network(lib_gate_class(nand2));
  foreach_primary_input(libnet, gen, pi) { nand2_formal[i++] = pi->name; }

  /* set the default mapping for each node */
  foreach_node(network, gen, node) {
    if (node->type == INTERNAL) {
      switch (node_num_fanin(node)) {
      case 0:
        gate = node_function(node) == NODE_0 ? zero : one;
        assert(lib_set_gate(node, gate, NIL(char *), NIL(node_t *), 0));
        break;
      case 1:
        assert(lib_set_gate(node, inv, inv_formal, node->fanin, 1));
        break;
      case 2:
        assert(lib_set_gate(node, nand2, nand2_formal, node->fanin, 2));
        break;
      default:
        fail("bad node degree; should have been caught before here");
        break;
      }
    }
  }
  FREE(inv_formal);
  FREE(nand2_formal);

  /* perform delay trace using unit-delay model */
  assert(delay_trace(network, DELAY_MODEL_LIBRARY));

  /* assign the initial area and arrival times for each node */
  foreach_node(network, gen, node) {
    if (node->type == PRIMARY_INPUT) {
      MAP(node)->map_area = 0;
      MAP(node)->map_arrival = delay_arrival_time(node);
    } else {
      MAP(node)->map_area = -1;
      MAP(node)->map_arrival.rise = INFINITY;
      MAP(node)->map_arrival.fall = INFINITY;
    }
    MAP(node)->load = delay_load(node);
    slack = delay_slack_time(node);
    real_slack = MIN(slack.rise, slack.fall);
    MAP(node)->slack_is_critical = real_slack < global_thresh;
  }

  return 1;
}

static int do_tree_match(network, library, options) network_t *network;
library_t *library;
bin_global_t *options;
{
  int i, some_output;
  node_t *node, *fanout;
  prim_t *prim;
  array_t *nodevec;
  lsGen gen, gen1;
  char errmsg[1024];
  int debug = options->verbose;
#ifdef SIS
  latch_t *latch;
#endif

  /* clear all bindings for all patterns */
  lsForeachItem(library->patterns, gen, prim) { prim_clear_binding(prim); }

  /* Match each node in a depth-first order */
  nodevec = network_dfs(network);
  for (i = 0; i < array_n(nodevec); i++) {
    node = array_fetch(node_t *, nodevec, i);

#ifdef SIS
    /* sequential support */
    if (node->type == PRIMARY_INPUT)
      continue;
    if (node->type == PRIMARY_OUTPUT && !IS_LATCH_END(node))
      continue;
#else
    if (node->type != INTERNAL)
      continue;
#endif
    if (node_num_fanin(node) == 0)
      continue; /* avoid constants */

#ifdef SIS
    /* set latch type to rising-edge if it is not already set */
    latch = NIL(latch_t);
    if (IS_LATCH_END(node)) {
      latch = latch_from_node(node);
      if (latch == NIL(latch_t)) {
        (void)sprintf(errmsg, "latch node '%s' has no latch structure\n",
                      node_name(node));
        error_append(errmsg);
        return 0;
      }
    }
#endif

    /*   Try each primitive rooted at this node */
    MAP(node)->gate = NIL(lib_gate_t);
    lsForeachItem(library->patterns, gen, prim) {
      gen_all_matches(node, prim, find_best_match, debug > 5,
                      g_allow_internal_fanout, options->fanout_limit);
    }
    if (MAP(node)->map_area < 0.0 || MAP(node)->gate == 0) {
#ifdef SIS
      if (latch == NIL(latch_t) || latch_get_type(latch) == ASYNCH) {
#endif
        (void)sprintf(errmsg, "library error: no gate matches at %s\n",
                      node_name(node));
        error_append(errmsg);
        return 0;
#ifdef SIS
      } else {
        latch_synch_t latch_type;

        /* Synchronous latch - try an opposite polarity latch */
        latch_type = latch_get_type(latch);
        switch ((int)latch_type) {
        case (int)RISING_EDGE:
          latch_set_type(latch, FALLING_EDGE);
          break;
        case (int)FALLING_EDGE:
          latch_set_type(latch, RISING_EDGE);
          break;
        case (int)ACTIVE_HIGH:
          latch_set_type(latch, ACTIVE_LOW);
          break;
        case (int)ACTIVE_LOW:
          latch_set_type(latch, ACTIVE_HIGH);
          break;
        default:
          fail("do_tree_match: bad latch type");
          break;
        }
        lsForeachItem(library->patterns, gen, prim) {
          gen_all_matches(node, prim, find_best_match, debug > 5,
                          g_allow_internal_fanout, options->fanout_limit);
        }
        if (MAP(node)->map_area < 0.0 || MAP(node)->gate == 0) {
          (void)sprintf(errmsg, "library error: no latch matches at %s\n",
                        node_name(node));
          error_append(errmsg);
          return 0;
        } else {
          node_t *clock, *new_node, *fanin;
          lib_gate_t *inv;

          if (!global.no_warning)
            (void)fprintf(siserr,
                          "warning: no %s type latch matches at %s (opposite "
                          "polarity latch used)\n",
                          (latch_type == RISING_EDGE)
                              ? "RISING_EDGE"
                              : (latch_type == FALLING_EDGE)
                                    ? "FALLING_EDGE"
                                    : (latch_type == ACTIVE_HIGH)
                                          ? "ACTIVE_HIGH"
                                          : "ACTIVE_LOW",
                          node_name(node));
          /* Insert a small inverter at the clock */
          inv = choose_smallest_gate(library, "f=!a;");
          clock = latch_get_control(latch);
          if (clock != NIL(node_t)) {
            assert(node_type(clock) == PRIMARY_OUTPUT);
            fanin = node_get_fanin(clock, 0);
            new_node = node_alloc();
            node_replace(new_node, node_literal(fanin, 0));
            assert(node_patch_fanin(clock, fanin, new_node));
            network_add_node(network, new_node);
            map_alloc(new_node);
            MAP(new_node)->gate = inv;
            MAP(new_node)->map_area = inv->area;
            MAP(new_node)->map_arrival.rise = MAP(fanin)->map_arrival.rise;
            MAP(new_node)->map_arrival.fall = MAP(fanin)->map_arrival.fall;
            MAP(new_node)->ninputs = 1;
            MAP(new_node)->save_binding = ALLOC(node_t *, 1);
            MAP(new_node)->save_binding[0] = fanin;
          }
        }
      }
#endif
    }

    /* apply inverter optimization at the root of a tree */
    if (global_inverter_optz && is_tree_leaf(node)) {
      inverter_optimize(node);
    }

    /* compute the area and timing estimates */
    some_output = 0;
    foreach_fanout(node, gen1, fanout) {
      if (fanout->type == PRIMARY_OUTPUT) {
        some_output = 1;
      }
    }
    if (some_output) {
      area_estimate += MAP(node)->map_area / node_num_fanout(node);
      max_arrival_estimate =
          MAX(max_arrival_estimate, MAP(node)->map_arrival.rise);
      max_arrival_estimate =
          MAX(max_arrival_estimate, MAP(node)->map_arrival.fall);
    }

    if (debug > 0) {
      (void)fprintf(sisout,
                    "Best match at %s is %s area=%5.2f arrival=(%5.2f %5.2f)\n",
                    node_name(node), MAP(node)->gate->name, MAP(node)->map_area,
                    MAP(node)->map_arrival.rise, MAP(node)->map_arrival.fall);
    }
  }
  array_free(nodevec);

  /* Derive the optimal cover with a single walk over the network */
#ifdef SIS
  hack_po(network);
#endif
  return 1;
}

/*
 *  find_best_match: simulate all gates which match a given pattern;
 *  maintain at the node the best match seen to this point
 */
static int find_best_match(prim) prim_t *prim;
{
  lsGen gen;
  lib_gate_t *gate;

#ifdef SIS
  /* Make sure that we distinguish between combinational gates and latches. */
  if (seq_filter(prim, global_debug)) {
    /* simulate each gate which matches this pattern */
#endif /* SIS */
    lsForeachItem(prim->gates, gen, gate) { find_match_cost(prim, gate); }
#ifdef SIS
  }
#endif /* SIS */
  return 1;
}

/* compare_cost contains the cost function: it returns 1 if THIS is better than
 * MATCH.
 */

int compare_cost(this_area, this_arrival, match_area, match_arrival,
                 slack_is_critical)

    double match_area,
    this_area, match_arrival, this_arrival;
int slack_is_critical;

{
  int optimize_area, optimize_delay;
  double this_cost, match_cost;

  /* old cost function for 0.0, 1.0 and 2.0 */
  if (FP_EQUAL(global_area_mode, 2)) {
    optimize_area = !slack_is_critical;
    optimize_delay = slack_is_critical;
  } else {
    optimize_area = FP_EQUAL(global_area_mode, 0.0);
    optimize_delay = FP_EQUAL(global_area_mode, 1.0);
  }

  if (optimize_area) {
    if (match_area < this_area ||
        (FP_EQUAL(match_area, this_area) && match_arrival < this_arrival)) {
      return 1;
    }
  } else if (optimize_delay) {
    if (match_arrival < this_arrival ||
        (FP_EQUAL(match_arrival, this_arrival) && match_area < this_area)) {
      return 1;
    }
  } else {
    /* cost = (1 - mode) * area + mode * delay
     * use area then delay to break ties
     */
    this_cost =
        (1.0 - global_area_mode) * this_area + global_area_mode * this_arrival;
    match_cost = (1.0 - global_area_mode) * match_area +
                 global_area_mode * match_arrival;
    if (match_cost < this_cost ||
        (FP_EQUAL(match_cost, this_cost) && match_area < this_area) ||
        (FP_EQUAL(match_cost, this_cost) && FP_EQUAL(match_area, this_area) &&
         match_arrival < this_arrival)) {
      return 1;
    }
  }

  return 0;
}

/*
 *  find_match_cost: simulate a single gate; keep it if its the best
 */

static void find_match_cost(prim, gate) prim_t *prim;
lib_gate_t *gate;
{
  int i, new_best;
  double match_area, this_area, match_arrival, this_arrival;
  delay_time_t arrival;
  node_t *node;

  /* get the root in the subject network */
  node = map_prim_get_root(prim);

  /* Determine the value of this match */
  match_cost(node, prim, gate, &match_area, &arrival);
  match_arrival = MAX(arrival.rise, arrival.fall);

  if (global_debug > 1) {
    (void)fprintf(sisout, "    match of %s at %s (area=%5.2f arrival=%5.2f)\n",
                  gate->name, node_name(node), match_area, match_arrival);
    if (global_debug > 2) {
      for (i = 0; i < prim->ninputs; i++) {
        (void)fprintf(sisout,
                      "        Graph node=%10s <---> (in)  prim_node=%10s\n",
                      prim->inputs[i]->binding->name, prim->inputs[i]->name);
      }
      for (i = 0; i < prim->noutputs; i++) {
        (void)fprintf(sisout,
                      "        Graph node=%10s <---> (out) prim_node=%10s\n",
                      prim->outputs[i]->binding->name, prim->outputs[i]->name);
      }
    }
  }

  if (MAP(node)->gate == 0) {
    /* first match is always the best */
    new_best = 1;

  } else {
    this_area = MAP(node)->map_area;
    this_arrival =
        MAX(MAP(node)->map_arrival.rise, MAP(node)->map_arrival.fall);
    new_best = compare_cost(this_area, this_arrival, match_area, match_arrival,
                            MAP(node)->slack_is_critical);
  }

  if (new_best) {
    /* save this match as minimum cost for this node */
    MAP(node)->gate = gate;
    MAP(node)->map_area = match_area;
    MAP(node)->map_arrival.rise = arrival.rise;
    MAP(node)->map_arrival.fall = arrival.fall;

    FREE(MAP(node)->save_binding);
    MAP(node)->ninputs = prim->ninputs;
    MAP(node)->save_binding = ALLOC(node_t *, prim->ninputs);
    for (i = 0; i < prim->ninputs; i++) {
      MAP(node)->save_binding[i] = prim->inputs[i]->binding;
    }
  }
}

static void match_cost(node, prim, gate, area, arrival) node_t *node;
prim_t *prim;
lib_gate_t *gate;
double *area;
delay_time_t *arrival;
{
  register int i;
  register node_t *fanin;
  register delay_time_t **arrival_times;
  register prim_node_t **leafs;

  leafs = prim->inputs;

  /* check -- is the match an inverter which is already paid for ? */
  if (global_inverter_optz) {
    if (match_is_inverter(node, prim)) {
      fanin = leafs[0]->binding;
      if (MAP(fanin)->inverter_paid_for) {
        *area = 0.0;
        *arrival = MAP(fanin)->inverter_arrival;
        return;
      }
    }
  }

  /*
   *  Compute area cost of this match
   */
  *area = gate->area;
  for (i = prim->ninputs - 1; i >= 0; i--) {
    fanin = leafs[i]->binding;
    if (g_allow_internal_fanout & 1) {
      if (fanin->type != PRIMARY_INPUT) {
        *area += MAP(fanin)->map_area / node_num_fanout(fanin);
      }
    } else {
      if (!is_tree_leaf(fanin)) {
        *area += MAP(fanin)->map_area;
      }
    }
  }

  /*
   *  Compute arrival times (if this match chosen)
   */
  if (strcmp(gate->name, "**wire**") == 0) {
    *arrival = MAP(prim->inputs[0]->binding)->map_arrival;

  } else {
    arrival_times = ALLOC(delay_time_t *, prim->ninputs);
    for (i = prim->ninputs - 1; i >= 0; i--) {
      fanin = leafs[i]->binding;
      arrival_times[i] = &(MAP(fanin)->map_arrival);
    }
    *arrival = delay_map_simulate(prim->ninputs, arrival_times,
                                  gate->delay_info, MAP(node)->load);
    FREE(arrival_times);
  }
}

network_t *map_build_network(network) network_t *network;
{
  node_t *node, *new_node, **fanin;
  network_t *new_network;
  lsGen gen;

  /* clear the 'matching' node for each node in the old network */
  foreach_node(network, gen, node) { MAP(node)->match_node = NIL(node_t); }

  /* allocate a new network */
  new_network = network_alloc();
  network_set_name(new_network, network_name(network));
  delay_network_dup(new_network, network);
#ifdef SIS
  new_network->astg = astg_dup(network->astg);
#endif /* SIS */

  /* copy the PI's first */
  foreach_primary_input(network, gen, node) {
    delay_time_t drive;
    delay_time_t arrival;

    new_node = node_dup(node);
    MAP(new_node)->gate = NIL(lib_gate_t);
    if (MAP(new_node)->ninputs > 0) {
      MAP(new_node)->ninputs = 0;
      FREE(MAP(new_node)->save_binding);
    }
    network_add_primary_input(new_network, new_node);
    MAP(node)->match_node = new_node;
    if (delay_get_pi_drive(node, &drive)) {
      delay_set_parameter(new_node, DELAY_DRIVE_RISE, drive.rise);
      delay_set_parameter(new_node, DELAY_DRIVE_FALL, drive.fall);
    }
    if (delay_get_pi_arrival_time(node, &arrival)) {
      delay_set_parameter(new_node, DELAY_ARRIVAL_RISE, arrival.rise);
      delay_set_parameter(new_node, DELAY_ARRIVAL_FALL, arrival.fall);
    }
  }

  /* copy the PO's and recursively from the PO's */
  foreach_primary_output(network, gen, node) {
    delay_time_t required;
    double load;

    new_node = node_dup(node);
    MAP(new_node)->gate = NIL(lib_gate_t);
    if (MAP(new_node)->ninputs > 0) {
      MAP(new_node)->ninputs = 0;
      FREE(MAP(new_node)->save_binding);
    }
    fanin = ALLOC(node_t *, 1);
    fanin[0] = build_network_recur(new_network, map_po_get_fanin(node));
    node_replace_internal(new_node, fanin, 1, NIL(set_family_t));
    network_add_node(new_network, new_node);
    MAP(node)->match_node = new_node;
    if (delay_get_po_required_time(node, &required)) {
      delay_set_parameter(new_node, DELAY_REQUIRED_RISE, required.rise);
      delay_set_parameter(new_node, DELAY_REQUIRED_FALL, required.fall);
    }
    if (delay_get_po_load(node, &load)) {
      delay_set_parameter(new_node, DELAY_OUTPUT_LOAD, load);
    }
  }
  return new_network;
}

static node_t *build_network_recur(new_network, node) network_t *new_network;
node_t *node;
{
  int i, nin;
  node_t *pi, *new_node, **fanin;
  char **formals;
  lsGen gen;

  if (MAP(node)->match_node == NIL(node_t)) {
    assert(node->type == INTERNAL);

    /* minor hack -- remove 'wires' when building the output graph */

    assert(MAP(node)->gate != NIL(lib_gate_t));
    if (strcmp(MAP(node)->gate->name, "**wire**") == 0) {
      new_node = build_network_recur(new_network, MAP(node)->save_binding[0]);
      MAP(node)->match_node = new_node;

    } else {
      new_node = node_alloc();
      new_node->name = util_strsav(node->name);
      nin = network_num_pi(MAP(node)->gate->network);

      fanin = ALLOC(node_t *, nin);
      for (i = 0; i < nin; i++) {
        fanin[i] = build_network_recur(new_network, MAP(node)->save_binding[i]);
      }
      formals = ALLOC(char *, nin);
      i = 0;
      foreach_primary_input(MAP(node)->gate->network, gen, pi) {
        formals[i++] = pi->name;
      }

      if (!lib_set_gate(new_node, MAP(node)->gate, formals, fanin, nin)) {
        fail("lib_set_gate cannot fail");
      }
      network_add_node(new_network, new_node);
      MAP(node)->match_node = new_node;
      FREE(formals);
      FREE(fanin);
    }
  }
  return MAP(node)->match_node;
}

static void set_leaf_inverter_flag(node) node_t *node;
{
  int i;
  node_t *fanin;

  if (is_tree_leaf(node)) {
    /* found leaf -- exit recursion */
    return;
  }

  /* see if an inverter matched at the leaf */
  if (best_match_is_inverter(node)) {
    fanin = MAP(node)->save_binding[0];
    if (is_tree_leaf(fanin) && !MAP(fanin)->inverter_paid_for) {
      MAP(fanin)->inverter_paid_for = 1;
      MAP(fanin)->inverter_arrival = MAP(node)->map_arrival;
    }
  }

  for (i = MAP(node)->ninputs - 1; i >= 0; i--) {
    set_leaf_inverter_flag(MAP(node)->save_binding[i]);
  }
}

static int inverter_optimize(node) register node_t *node;
{
  int i;

  /* see if an inverter matched at the root of the tree */
  if (best_match_is_inverter(node)) {
    MAP(node)->inverter_paid_for = 1;
    MAP(node)->inverter_arrival = MAP(node)->map_arrival;
  }

  /* walk back to the leaves, seeing if inverters matched at the inputs */
  for (i = MAP(node)->ninputs - 1; i >= 0; i--) {
    set_leaf_inverter_flag(MAP(node)->save_binding[i]);
  }
}

/*
 *  patch the netlist so that all 0,1 cells are driven by a single node
 */

void patch_constant_cells(network) network_t *network;
{
  lsGen gen, gen1;
  node_t *node, *fanout, *const0_node, *const1_node;

  const0_node = const1_node = NIL(node_t);

  foreach_node(network, gen, node) {
    switch (node_function(node)) {
    case NODE_0:
      if (const0_node == 0) {
        const0_node = node;
      } else {
        /* patch all other 0 nodes to be driven by the 0-cell */
        foreach_fanout(node, gen1, fanout) {
          assert(node_patch_fanin(fanout, node, const0_node));
        }
        /* delete this redundant 0-cell */
        network_delete_node_gen(network, gen);
      }
      break;

    case NODE_1:
      if (const1_node == 0) {
        const1_node = node;
      } else {
        /* patch all other 1 nodes to be driven by the 1-cell */
        foreach_fanout(node, gen1, fanout) {
          assert(node_patch_fanin(fanout, node, const1_node));
        }
        /* delete this redundant 1-cell */
        network_delete_node_gen(network, gen);
      }
      break;
    default:;
    }
  }
}

#ifdef SIS
/* sequential support */
/* distinguish between latches and
 * combinational gates.
 */
int seq_filter(prim, debug) prim_t *prim;
int debug;
{
  prim_node_t *prim_latch_end;
  node_t *latch_end, *node;
  int i;
  latch_t *latch;

  node = map_prim_get_root(prim);
  latch_end = network_latch_end(node);
  if (latch_end == NIL(node_t)) {
    /* the subject graph is a combintaional logic */
    if (prim->latch_type != COMBINATIONAL) {
      if (debug <= -4) {
        (void)fprintf(sisout, "\t--REJECT %s: Need combinational gate at %s\n",
                      prim_gate_name(prim), node_name(node));
      }
      return 0;
    } else {
      return 1;
    }
  } else {
    /* the subject graph is a latch */
    latch = latch_from_node(node);
    if (prim->latch_type != latch_get_type(latch)) {
      if (debug <= -4) {
        (void)fprintf(sisout, "\t--REJECT %s: Gate type mismatch at %s\n",
                      prim_gate_name(prim), node_name(node));
      }
      return 0;
    } else {
      /* make sure that the latch outputs match */
      /* first find the latch output node on the pattern graph */
      prim_latch_end = NIL(prim_node_t);
      for (i = 0; i < prim->ninputs; i++) {
        if (prim->inputs[i]->latch_output) {
          prim_latch_end = prim->inputs[i];
          break;
        }
      }
      if (prim_latch_end == NIL(prim_node_t)) {
        /* no latch output found - must be a d-type latch */
        return 1;

        /* check if both latch output nodes match.
         * because of the inverter pairs which are added
         * everywhere there may exist a buffer connection
         * between the two nodes.
         */
      } else if (prim_latch_end->binding == latch_end ||
                 is_buffer(latch_end, prim_latch_end->binding)) {
        /* make sure that this latch output node is unique */
        for (i = 0; i < prim->ninputs; i++) {
          if (prim->inputs[i] == prim_latch_end)
            continue;
          if (is_transitive_fanin(prim->inputs[i]->binding, latch_end)) {
            if (debug <= -4) {
              (void)fprintf(sisout,
                            "\t--REJECT %s at %s: Latch output is not unique\n",
                            prim_gate_name(prim), node_name(node));
              (void)fprintf(sisout,
                            "\t\t[[Multiple latch ouputs are %s and %s]]\n",
                            node_name(prim_latch_end->binding),
                            node_name(prim->inputs[i]->binding));
            }
            return 0;
          }
        }
        return 1;
      }
      if (debug <= -4) {
        (void)fprintf(sisout,
                      "\t--REJECT %s at %s: Latch outputs don't match\n",
                      prim_gate_name(prim), node_name(node));
        (void)fprintf(
            sisout, "\t\t[[Graph latch output = %s, Prim latch output = %s]]\n",
            latch_end->name, prim_latch_end->binding->name);
      }
    }
  }
  return 0;
}

static char *prim_gate_name(prim) prim_t *prim;
{
  lsGen gen;
  lib_gate_t *gate;

  lsForeachItem(prim->gates, gen, gate) {
    LS_ASSERT(lsFinish(gen));
    break;
    /* NOTREACHED */
  }
  assert(gate != NIL(lib_gate_t));
  return (gate->name);
}

/* Check if node2 is a buffer of node1 */
static int is_buffer(node1, node2) node_t *node1, *node2;
{
  lsGen gen1, gen2;
  node_t *fanout1, *fanout2;

  foreach_fanout(node1, gen1, fanout1) {
    if (node_function(fanout1) == NODE_INV) {
      foreach_fanout(fanout1, gen2, fanout2) {
        if (node_function(fanout2) == NODE_INV && fanout2 == node2) {
          LS_ASSERT(lsFinish(gen1));
          LS_ASSERT(lsFinish(gen2));
          return 1;
        }
      }
    }
  }
  return 0;
}

/* check if node2 is in the transitive fanin of node1 */
static int is_transitive_fanin(node1, node2) node_t *node1;
register node_t *node2;
{
  node_t *fanin;
  int result, i;

  if (node1 == node2)
    return 1;
  if (node1->type == PRIMARY_INPUT)
    return 0;
  result = 0;
  foreach_fanin(node1, i, fanin) {
    result += is_transitive_fanin(fanin, node2);
    if (result > 0) {
      break;
    }
  }
  return result;
}

/* After initial mapping, the function associated with
 * latches are temporarily stored at PO/latch-input nodes.
 * Now create an extra buffer node to move the func info to.
 * Also modify the fanout of the new node
 * such that it points to the real PO.
 * Before:  (internal node) -> real PO (no func)
 *                          -> PO/latch-input (has latch func)
 *                          -> PO/latch-input (has latch func)
 *                                 ...
 * After:   (internal node) -> real PO (no func)
 *                          -> (internal node - has latch func)     ->
 * PO/latch-input (no func)
 *                          -> (internal node - has latch func)     ->
 * PO/latch-input (no func)
 *                                 ...
 */
static void hack_po(network) network_t *network;
{
  lsGen gen;
  node_t *po, *fanin, *new_node;
  int i;
  latch_t *latch;

  foreach_latch(network, gen, latch) {
    po = latch_get_input(latch);
    fanin = node_get_fanin(po, 0);
    new_node = node_alloc();
    node_replace(new_node, node_literal(fanin, 1));
    assert(node_patch_fanin(po, fanin, new_node));
    network_add_node(network, new_node);
    /* PO derives its output from its fanin node */
    /* Swap names to preserve the IO names*/
    network_swap_names(network, new_node, fanin);
    map_alloc(new_node);
    MAP(new_node)->gate = MAP(po)->gate;
    MAP(new_node)->map_area = MAP(po)->map_area;
    MAP(new_node)->map_arrival = MAP(po)->map_arrival;
    MAP(new_node)->ninputs = MAP(po)->ninputs;
    MAP(new_node)->save_binding = ALLOC(node_t *, MAP(po)->ninputs);
    for (i = 0; i < MAP(po)->ninputs; i++) {
      MAP(new_node)->save_binding[i] = MAP(po)->save_binding[i];
    }
    MAP(po)->gate = NIL(lib_gate_t);
  }
}

#if !defined(lint) && !defined(SABER)

/* debug routine */
static void map_print_network(network) network_t *network;
{
  array_t *list;
  node_t *node;
  int i;

  list = network_dfs(network);
  for (i = 0; i < array_n(list); i++) {
    node = array_fetch(node_t *, list, i);
    node_print(sisout, node);
  }
  array_free(list);
}
#endif
#endif
