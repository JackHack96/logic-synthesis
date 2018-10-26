
/* file @(#)library.c	1.14 */
/* last modified on 9/17/91 at 14:30:10 */
/*
 * 01 10-Jun-90 klk 	Changed the procedure choose_nth_smallest_gate from static to global
 */

#include "sis.h"
#include "map_int.h"

static network_t *lib_wire();
static int process_pattern();
static int process_constant();
static prim_t *find_or_create_pattern();
static lib_gate_t *find_gate();
static lib_gate_t *find_or_create_gate();
static lib_class_t *find_class();
static lib_class_t *find_or_create_class();
static void derive_dual_classes();
static network_t *dual_network();
static void lib_reset_default_gates();
static void lib_reset_prim_table();

/* sequential support */
static char **lib_get_pin_delay_info();
static char *node_real_name();
static st_table *clock_table;

#ifdef SIS
typedef struct lib_clock_t lib_clock_t;
struct lib_clock_t {
  char *name;
  delay_pin_t *delay;
};

static int lib_reduce_network();
static int lib_expand_network();
static int lib_get_latch_pin();
static latch_time_t **lib_get_latch_time_info();
static void lib_register_type();
static void lib_save_clock_info();
#endif /* SIS */

int
lib_read_blif(fp, filename, add_inverter, nand_flag, library_p)
FILE *fp;
char *filename;
int add_inverter;
int nand_flag;
library_t **library_p;
{
  int new_gates, c;
  library_t *library;
  network_t *network, *wire;
  st_generator *gen;
  char *key, *value;

  lib_reset_default_gates();
  lib_reset_prim_table();
#ifdef SIS
  clock_table = st_init_table(strcmp, st_strhash);
#endif /* SIS */
  library = *library_p;
  if (library == 0) {
    library = ALLOC(library_t, 1);
    library->gates = lsCreate();
    library->classes = lsCreate();
    library->patterns = lsCreate();
    library->nand_flag = nand_flag;
    library->add_inverter = add_inverter;
  }

  new_gates = 0;
  read_register_filename(filename);
  while ((c = read_blif_first(fp, &network)) != -1) {
    if (! c) return 0;

#ifdef SIS
    lib_register_type(network);
#endif /* SIS */

    if (network_num_pi(network) == 0) {
      /* process constant 0 or 1 cell */
      if (! process_constant(library, network)) {
	return 0;
      }

    } else {
      /* make sure its a 2-input NAND (NOR) gate network */
      if (! map_check_form(network, library->nand_flag)) {
	return 0;
      }

      /* add double-inverter pair to all internal nodes */
      if (library->add_inverter) {
	map_add_inverter(network, /* add_at_pipo */ 0);
      }

      /* add the pattern and gate to the library */
      if (! process_pattern(library, network)) {
	return 0;
      }
    }
    new_gates = 1;
  }

  /* Make sure the library has a wire primitive */
  wire = lib_wire();
  if (! find_gate(library, wire)) {
#ifdef SIS
    lib_register_type(wire);
#endif /* SIS */
    if (! process_pattern(library, wire)) {
      return 0;
    }
    new_gates = 1;
  } else {
    network_free(wire);
  }

  if (new_gates) {
    derive_dual_classes(library);
  }

  *library_p = library;
#ifdef SIS
  st_foreach_item(clock_table, gen, &key, &value) {
    FREE(key);
    FREE(value);
  }
  st_free_table(clock_table);
#endif /* SIS */
  return 1;
}

static int
process_pattern(library, network)
library_t *library;
network_t *network;
{
    lib_class_t *class;
    lib_gate_t *gate;
    prim_t *prim;

    map_setup_network(network);

    prim = find_or_create_pattern(library, network);
    if (! prim) return 0;

    gate = find_or_create_gate(library, network);
    if (! gate) return 0;

    class = find_or_create_class(library, gate);
    if (! class) return 0;

    LS_ASSERT(lsNewEnd(prim->gates, (char *) gate, LS_NH));

    return 1;
}



static int
process_constant(library, network)
library_t *library;
network_t *network;
{
    node_t *node;
    node_function_t func;
    lib_gate_t *gate;
    lib_class_t *class;

    if (network_num_po(network) != 1) {
	error_append("no-input function is not single-output ?\n");
	return 0;
    }
    node = node_get_fanin(network_get_po(network, 0), 0);
    func = node_function(node);
    if (func == NODE_0 || func == NODE_1) {
	gate = find_or_create_gate(library, network);
	if (! gate) return 0;

	class = find_or_create_class(library, gate);
	if (! class) return 0;
    } else {
	error_append("no-input function is not a constant ?\n");
	return 0;
    }
    return 1;
}

static int match_flag;


static int 
found_match(prim)
prim_t *prim;
{
    int i;

    for(i = 0; i < prim->ninputs; i++) {
	if (prim->inputs[i]->binding->type != PRIMARY_INPUT) {
	    return 1;	/* not a match, continue search ... */
	}
    }
    match_flag = 1;
    return 0;		/* stop search now ... */
}


int
lib_pattern_matches(network, prim, ignore_type)
network_t *network;
prim_t *prim;
int ignore_type;
{
  node_t *po;

  if (network_num_pi(network) != prim->ninputs) {
    return 0;
  }
  if (network_num_po(network) != prim->noutputs) {
    return 0;
  }
#ifdef SIS
  if (!ignore_type) {
    if (network_gate_type(network) != prim->latch_type) {
      return 0;
    }
  }
  if (prim->latch_type == COMBINATIONAL) {
    po = node_get_fanin(network_get_po(network, 0), 0);
  } else {
    po = network_get_po(network, 0);
  }
#else
  po = node_get_fanin(network_get_po(network, 0), 0);
#endif /* SIS */
  match_flag = 0;
  gen_all_matches(po, prim, found_match, /* debug */  0, /* allow_internal_fanout */ 0, /* fanout_limit */ INFINITY);
  return match_flag;
}


static prim_t *
find_or_create_pattern(library, network)
library_t *library;
network_t *network;
{
    lsGen gen;
    prim_t *prim;

#ifdef SIS
    /* remove unnecessary nodes for pattern matching */
    if (!lib_reduce_network(network)) {
      return 0;
    }
#endif /* SIS */

    /* see if this is a new pattern, or duplicates some existing pattern */ 
    lsForeachItem(library->patterns, gen, prim) {
	if (lib_pattern_matches(network, prim, /* consider type */ 0)) {
	    LS_ASSERT(lsFinish(gen));
#ifdef SIS
	    if (!lib_expand_network(network)) return 0;
#endif /* SIS */
	    return prim;
	}
    }

    if (! map_network_to_prim(network, &prim)) {
	return 0;
    }

#ifdef SIS
    /* add an internal node if necessary */
    if (!lib_expand_network(network)) {
      return 0;
    }
#endif /* SIS */

    LS_ASSERT(lsNewEnd(library->patterns, (char *) prim, LS_NH));
    return prim;
}

static lib_gate_t *
find_gate(library, network)
library_t *library;
network_t *network;
{
    lsGen gen;
    lib_gate_t *gate;
    char *netname;

    netname = network_name(network);
    lsForeachItem(library->gates, gen, gate) {
	if (strcmp(gate->name, netname) == 0) {
	    /* ### should check that area, delay, function are the same ... */
	    LS_ASSERT(lsFinish(gen));
	    return gate;
	}
    }
    return 0;
}


static lib_gate_t *
find_or_create_gate(library, network)
library_t *library;
network_t *network;
{
  int i;
  lib_gate_t *gate;
  char errmsg[1024];
#ifdef SIS
  lib_clock_t *clock_info;
#endif /* SIS */

  gate = find_gate(library, network);
  if (gate == 0) {
    /* collapse network, re-order fanin of each node */
    node_lib_process(network);
	
    gate = ALLOC(lib_gate_t, 1);
    gate->name = util_strsav(network_name(network));
    gate->network = network;
    gate->area = network->area;
#ifdef SIS
    gate->type = network_gate_type(network);
    gate->latch_pin = lib_get_latch_pin(network);
    gate->latch_time_info = lib_get_latch_time_info(network);
    if (st_lookup(clock_table, network_name(network), (char**) &clock_info)) {
      gate->control_name = clock_info->name;
      gate->clock_delay = clock_info->delay;
    } else {
      gate->control_name = NIL(char);
      gate->clock_delay = NIL(delay_pin_t);
    }
#endif /* SIS */
    gate->delay_info = lib_get_pin_delay_info(network);
    gate->class_p = 0;
    LS_ASSERT(lsNewEnd(library->gates, (char *) gate, LS_NH));

    if (! network->area_given) {
      (void) sprintf(errmsg, "'%s': area not given", gate->name);
      read_error(errmsg);
      return 0;
    }
    if (strcmp(network_name(network), "**wire**") != 0) {
      for(i = network_num_pi(network)-1; i >= 0; i--) {
	if (gate->delay_info[i] == 0) {
	  (void) sprintf(errmsg, 
			 "'%s': missing delay information for pin '%s'", 
			 gate->name, node_name(network_get_pi(network, i)));
	  read_error(errmsg);
	  return 0;
	}
#ifdef SIS
	if (gate->latch_pin == i) continue;
	if (lib_gate_type(gate) != COMBINATIONAL && lib_gate_type(gate) != ASYNCH) {
	  if (gate->latch_time_info[i] == 0) {
	    (void) sprintf(errmsg, 
			   "'%s': missing latch time for pin '%s'", 
			   gate->name, node_name(network_get_pi(network, i)));
	    read_error(errmsg);
	    return 0;
	  }
	}
#endif /* SIS */
      }
    }
  } else {
    network_free(network);
  }
  return gate;
}

/*
 *  compare two networks for equality (by name) of all outputs
 *  the networks should already be collapsed.
 */

static int
compare_network(net1, net2)
network_t *net1, *net2;
{
  lsGen gen;
  node_t *po1, *po2;

  if (network_num_pi(net1) != network_num_pi(net2)) {
    return 1;
  }
  if (network_num_po(net1) != network_num_po(net2)) {
    return 1;
  }

#ifdef SIS
  /* sequential support */
  if (network_gate_type(net1) != network_gate_type(net2)) {
    return 1;
  }
#endif

  foreach_primary_output(net1, gen, po1) {
    po2 = network_find_node(net2, po1->name);
    if (po2 == 0 || node_type(po2) != PRIMARY_OUTPUT) {
      LS_ASSERT(lsFinish(gen));
      return 1;
    }
    po1 = node_get_fanin(po1, 0);
    po2 = node_get_fanin(po2, 0);
    if (! node_equal_by_name(po1, po2)) {
      LS_ASSERT(lsFinish(gen));
      return 1;
    }
  }
  return 0;
}

static lib_class_t *
find_class(library, network)
library_t *library;
network_t *network;
{
    lsGen gen;
    lib_class_t *class;

    lsForeachItem(library->classes, gen, class) {
	if (compare_network(network, class->network) == 0) {
	    LS_ASSERT(lsFinish(gen));
	    return class;
	}
    }
    return 0;
}


static lib_class_t *
find_or_create_class(library, gate)
library_t *library;
lib_gate_t *gate;
{
    lib_class_t *class_p;

    /* see if gate is already assigned to a class */
    if (gate->class_p != 0) {
	return gate->class_p;
    }

    class_p = find_class(library, gate->network);
    if (class_p == NIL(lib_class_t)) {
	class_p = ALLOC(lib_class_t, 1);
	class_p->network = gate->network;
	class_p->gates = lsCreate();
	class_p->dual_class = 0;
	class_p->name = 0;
	LS_ASSERT(lsNewEnd(library->classes, (char *) class_p, LS_NH));
	class_p->library = library;
    } else {
	network_free(gate->network);
	gate->network = class_p->network;
    }

    LS_ASSERT(lsNewEnd(class_p->gates, (char *) gate, LS_NH));
    gate->class_p = class_p;
    return class_p;
}

static void
derive_dual_classes(library)
library_t *library;
{
  lsGen gen;
  lib_class_t *class;
  network_t *dual;

  lsForeachItem(library->classes, gen, class) {
    dual = dual_network(class->network);
    (void) network_collapse(dual);
    class->dual_class = find_class(library, dual);
    network_free(dual);
  }
}


static network_t *
dual_network(network)
network_t *network;
{
    node_t *pi, *po, *node, *inv;
    network_t *new_network;
    lsGen gen, gen1;

    new_network = network_dup(network);

    foreach_primary_input(new_network, gen, pi) {
	inv = node_literal(pi, 0);
	foreach_fanout(pi, gen1, node) {
	    assert(node_patch_fanin(node, pi, inv));
	}
	network_add_node(new_network, inv);
    }

    foreach_primary_output(new_network, gen, po) {
	node = node_get_fanin(po, 0);
	inv = node_literal(node, 0);
	assert(node_patch_fanin(po, node, inv));
	network_add_node(new_network, inv);
    }

    return new_network;
}

static network_t *
lib_wire()
{
    network_t *wire;

    wire = read_eqn_string("NAME = \"**wire**\"; Z = ! o1; o1 = ! a;");
    wire->area_given = 1;
    wire->area = 0.0;
    assert(wire != 0);
    return wire;
}

static double get_gate_area(gate)
lib_gate_t *gate;
{
  if (gate == NIL(lib_gate_t)) return INFINITY;
  return lib_gate_area(gate);
}


/*
 *  search the library for the 'smallest' gate with a given function
 */

lib_gate_t *
choose_smallest_gate(library, string)
library_t *library;
char *string;
{
  return choose_nth_smallest_gate(library, string, 0);
}

 /* only order=0 (smallest) and order=1 (second smallest) currently implemented */

lib_gate_t *choose_nth_smallest_gate(library, string, order)
library_t *library;
char *string;
int order;
{
  network_t *network;
  lib_class_t *class_p;
  lib_gate_t *gate, *best_gate, *second_best_gate;
  lsGen gen;

  network = read_eqn_string(string);
  if (network == 0) return 0;
  class_p = lib_get_class(network, library);
  network_free(network);
  if (class_p == 0) return 0;

  best_gate = NIL(lib_gate_t);
  second_best_gate = NIL(lib_gate_t);
  gen = lib_gen_gates(class_p);
  while (lsNext(gen, (char **) &gate, LS_NH) == LS_OK) {
    if (get_gate_area(gate) < get_gate_area(second_best_gate)) {
      if (get_gate_area(gate) < get_gate_area(best_gate)) {
	second_best_gate = best_gate;
	best_gate = gate;
      } else {
	second_best_gate = gate;
      }
    }
  }
  (void) lsFinish(gen);
  if (order == 0 || (order == 1 && second_best_gate == NIL(lib_gate_t))) {
    return best_gate;
  } else if (order == 1) {
    return second_best_gate;
  } else {
    return NIL(lib_gate_t);
  }
}

 /* default inverter; cached for efficiency but need to be reset */

static lib_gate_t *default_inverter = NIL(lib_gate_t);

static void lib_reset_default_gates()
{
  default_inverter = NIL(lib_gate_t);
}

lib_gate_t *lib_get_default_inverter()
{
  library_t *library;

  if (default_inverter == NIL(lib_gate_t)) {
    assert(library = lib_get_library());
    default_inverter = choose_nth_smallest_gate(library, "f=!a;", 1);
    assert(default_inverter != NIL(lib_gate_t));
  }
  return default_inverter;
}

 /* map from classes to prim_t structures. Cached for efficiency. Need to be reset for every new library */

static st_table *lib_prim_table;

static void lib_reset_prim_table()
{
  st_generator *gen;
  lib_class_t *class_p;
  array_t *prims;

  if (lib_prim_table == NIL(st_table)) return;
  st_foreach_item(lib_prim_table, gen, (char **) &class_p, (char **) &prims) {
    array_free(prims);
  }
  st_free_table(lib_prim_table);
  lib_prim_table = NIL(st_table);
}


 /* computes an array of prim_t *'s corresponding to a class */

array_t *lib_get_prims_from_class(class_p)
lib_class_t *class_p;
{
  array_t **slot;
  library_t *library;
  lsGen gen1, gen2;
  lib_gate_t *gate;
  prim_t *prim;

  if (lib_prim_table == NIL(st_table)) {
    lib_prim_table = st_init_table(st_ptrcmp, st_ptrhash);
  }
  if (st_find_or_add(lib_prim_table, (char *) class_p, (char ***) &slot)) return *slot;
  *slot = array_alloc(prim_t *, 0);
  library = class_p->library;
  lsForeachItem(library->patterns, gen1, prim) {
    lsForeachItem(prim->gates, gen2, gate) {
      if (gate->class_p == class_p) {
	array_insert_last(prim_t *, *slot, prim);
	lsFinish(gen2);
	break;
      }
    }
  }
  return *slot;
}

#ifdef SIS

/* HACK: remove nodes which are not necessary
 * for pattern matching.
 * (e.g., "ANY" node, clock node, etc.)
 */
static int
lib_reduce_network(network)
network_t *network;
{
  node_t *node;
  latch_t *latch;
  node_t *clock, *latch_input;
  char errmsg[1024];

  assert(network_gate_type(network) != (int) UNKNOWN);
  if (network_gate_type(network) != (int) COMBINATIONAL) {
    latch_input = network_latch_input(network);
    if (latch_input == NIL(node_t)) {
      (void)sprintf(errmsg, "latch network '%s' has no latch input", network_name(network));
      read_error(errmsg);
      return 0;
    }
    /* need a true PO name in prim */
    network_swap_names(network, latch_input, node_get_fanin(latch_input, 0));

    /* remove nodes associated with the clock */
    latch = latch_from_node(latch_input);
    if (latch == NIL(latch_t)) {
      (void)sprintf(errmsg, "latch network '%s' has no latch", network_name(network));
      read_error(errmsg);
      return 0;
    }
    clock = latch_get_control(latch);
    if (clock != NIL(node_t)) {
      /* save clock info and delete clock nodes */
      lib_save_clock_info(network, clock);
      latch_set_control(latch, NIL(node_t));
    }

    /* remove "ANY" node */
    node = network_find_node(network, "ANY");
    if (node != NIL(node_t)) {
      node_t *fanin = NIL(node_t);

      network_delete_latch(network, latch);
      /* "ANY" node may have a fanin.
       * In that case, remove the fanin as well.
       */
      if (node_num_fanin(node) != 0) {
	assert(node_num_fanin(node) == 1);
	fanin = node_get_fanin(node, 0);
      }
      network_delete_node(network, node);
      if (fanin != NIL(node_t)) network_delete_node(network, fanin);

      /* hack for D-type latches */
      if (network_num_pi(network) == 1) (void) network_csweep(network);
    }
  }
  return 1;
}
      
/* HACK: Basically, undo a change done by
 * lib_reduce_network.
 * Add an internal node in the network
 * if the network has no internal node.
 * D-type FF is represented by PI->PO
 * after lib_reduce_network.
 */
static int
lib_expand_network(network)
network_t *network;
{
  node_t *pi, *internal, *po;

  if (network_num_internal(network) == 0) {
    assert(network_num_po(network)==1);
    po = network_get_po(network, 0);
    pi = node_get_fanin(po, 0);
    assert(node_type(pi) == PRIMARY_INPUT);
    internal = node_alloc();
    node_replace(internal, node_literal(pi, 1));
    assert(node_patch_fanin(po, pi, internal));
    network_add_node(network, internal);
  }
  return 1;
}
#endif /* SIS */

/*
 * latch output nodes should not affect delay computation.
 * set all the delay parameters to 0 (this can alternatively be fixed
 * inside the delay package) for latch output pins.
 */
static char **
lib_get_pin_delay_info(network)
network_t *network;
{
#ifdef SIS
  lsGen gen;
  node_t *node;

  if (network_gate_type(network) != (int) COMBINATIONAL) {
    foreach_primary_input(network, gen, node) {
      if (IS_LATCH_END(node)) {
	delay_set_parameter(node, DELAY_BLOCK_RISE, 0.0);
	delay_set_parameter(node, DELAY_BLOCK_FALL, 0.0);
	delay_set_parameter(node, DELAY_DRIVE_RISE, 0.0);
	delay_set_parameter(node, DELAY_DRIVE_FALL, 0.0);
	delay_set_parameter(node, DELAY_INPUT_LOAD, 0.0);
	delay_set_parameter(node, DELAY_MAX_INPUT_LOAD, 0.0);
	delay_set_parameter(node, DELAY_PHASE, DELAY_PHASE_NEITHER);
      }
    }
  }
#endif /* SIS */
  return(delay_giveaway_pin_delay(network));
}

#ifdef SIS
/* return the fanin index for the latch output pin
 * if none exists, return -1
 */
static int
lib_get_latch_pin(network)
network_t *network;
{
  lsGen gen;
  node_t *pi, *latch_output;
  int i;

  latch_output = NIL(node_t);
  i = 0;
  foreach_primary_input(network, gen, pi) {
    if (IS_LATCH_END(pi)) {
      assert(latch_output == NIL(node_t)); /* only one latch output */
      latch_output = pi;
      LS_ASSERT(lsFinish(gen));
      break;
    }
    i++;
  }
  if (latch_output == NIL(node_t)) return -1;
  return i;
}

/* return the array of setup/hold times */
static latch_time_t **
lib_get_latch_time_info(network)
network_t *network;
{
  node_t *node;
  latch_time_t **latch_time_array;
  lsGen gen;
  int i;

  switch(network_gate_type(network)) {
  case (int) COMBINATIONAL:
  case (int) ASYNCH:
    return NIL(latch_time_t *);
  default:
    latch_time_array = ALLOC(latch_time_t *, network_num_pi(network));
    i = 0;
    foreach_primary_input(network, gen, node) {
      if (!IS_LATCH_END(node)) {
	latch_time_array[i] = ALLOC(latch_time_t, 1);
	latch_time_array[i]->setup = delay_get_parameter(node, DELAY_ARRIVAL_RISE);
	latch_time_array[i]->hold = delay_get_parameter(node, DELAY_ARRIVAL_FALL);
      } else {
	latch_time_array[i] = NIL(latch_time_t);
      }
      i++;
    }
  }
  return latch_time_array;
}

/* save clock name and delay and then
 * delete the clock nodes
 */
static void
lib_save_clock_info(network, clock)
network_t *network;
node_t *clock;
{
  lib_clock_t *clock_info;
  node_t *clock_fanin;
  double n;

  /* some sanity check */
  assert(node_type(clock) == PRIMARY_OUTPUT && node_num_fanin(clock) == 1);
  clock_fanin = node_get_fanin(clock, 0);
  assert(node_type(clock_fanin) == PRIMARY_INPUT && node_num_fanout(clock_fanin) == 1);

  if (!st_is_member(clock_table, network_name(network))) {
    clock_info = ALLOC(lib_clock_t, 1);
    clock_info->name = node_real_name(clock);
    clock_info->delay = ALLOC(delay_pin_t, 1);
    clock_info->delay->block.rise = delay_get_parameter(clock_fanin, DELAY_BLOCK_RISE);
    clock_info->delay->block.fall = delay_get_parameter(clock_fanin, DELAY_BLOCK_FALL); 
    clock_info->delay->drive.rise = delay_get_parameter(clock_fanin, DELAY_DRIVE_RISE); 
    clock_info->delay->drive.fall = delay_get_parameter(clock_fanin, DELAY_DRIVE_FALL); 
    clock_info->delay->load = delay_get_parameter(clock_fanin, DELAY_INPUT_LOAD); 
    clock_info->delay->max_load = delay_get_parameter(clock_fanin, DELAY_MAX_INPUT_LOAD); 
    n = delay_get_parameter(clock_fanin, DELAY_PHASE); 
    if (n == DELAY_PHASE_INVERTING) {
      clock_info->delay->phase = PHASE_INVERTING;
    } else if (n == DELAY_PHASE_NONINVERTING) {
      clock_info->delay->phase = PHASE_NONINVERTING;
    } else if (n == DELAY_PHASE_NEITHER) {
      clock_info->delay->phase = PHASE_NEITHER;
    } else {
      clock_info->delay->phase = PHASE_NOT_GIVEN;
    }
    assert(st_insert(clock_table, util_strsav(network_name(network)), (char *)clock_info)==0);
  }
  network_delete_node(network, clock);
  network_delete_node(network, clock_fanin);
}

static void
lib_register_type(network)
network_t *network;
{
  lsGen gen;
  node_t *node;
  node_t *latch_input = NIL(node_t);
  int *type;
  latch_t *latch;

  if (!st_is_member(network_type_table, network_name(network))) {
    foreach_primary_output(network, gen, node) {
      if (IS_LATCH_END(node)) {
	assert(latch_input == NIL(node_t)); /* only one latch input per gate */
	latch_input = node;
	LS_ASSERT(lsFinish(gen));
	break;
      }
    }
    type = ALLOC(int, 1);
    if (latch_input == NIL(node_t)) {
      *type = (int) COMBINATIONAL;
    } else {
      latch = latch_from_node(latch_input);
      assert(latch != NIL(latch_t));
      *type = (int) latch_get_type(latch);
    }
    assert(st_insert(network_type_table, util_strsav(network_name(network)), (char*)type)==0);
  }
}

#endif /* SIS */

/* returns the name of the node without {,[,],} */
/* assumes a single node name (no "[23, 43]" type of name) */
static char*
node_real_name(node)
node_t *node;
{
  char *name, *n;
  int len, i;

  name = node_name(node);
  len = strlen(name);
  n = ALLOC(char, len);
  if (name[0] == '{' || name[0] == '[') {
    assert(name[len-1] == '}' || name[len-1] == ']');
    for (i = 1; i < len - 1; i++) {
      n[i-1] = name[i];
    }
    n[i-1] = '\0';
  } else {
    (void) strcpy(n, name);
  }
  return n;
}

/* for debugging purposes */
st_print(table)
st_table *table;
{
  st_generator *gen;
  char *key, *val;

  st_foreach_item(table, gen, &key, &val) {
    printf("key = %s, value = %s\n", key, val);
  }
}
