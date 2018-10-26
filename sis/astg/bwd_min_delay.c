
#ifdef SIS
/*
 * The purpose of this package is the computation of node and network delays
 * in MIS.  Delays are returned as delay_time_t structures, which contain two
 * doubles, rise and fall.
 *
 * This package is designed to manipulate the basic timing package.  In
 * particular, routines are provided to compute the delay of a node for both
 * rising and falling times, to reset the delay model to a specialized
 * function, if desired, or to provide parameters to the default delay model.
 *
 * The delay package that comes with mis/sis computes MAXIMUM delays: this is a
 * copy that computes MINIMUM delays. Eventually we should use simulation to 
 * compute both.
 */
#include <setjmp.h>
#include <math.h>
#include "sis.h"
#include "min_delay_int.h"

node_t * min_delay_latest_output();
static jmp_buf delay_botch;

static void set_arrival_time();
static void set_required_time();
static void set_fanout_load();
static void delay_error();
static void allocate_pin_params();
static delay_time_t compute_delay();
static network_t *map_node_to_network();
static void delay_force_pininfo();
#ifndef lint
static void print_pin_delays();
#endif
static delay_time_t pi_arrival_time();
static delay_time_t po_required_time();

#define node_type(n) ((n)->type)

static delay_time_t time_not_given = {
    DELAY_VALUE_NOT_GIVEN, DELAY_VALUE_NOT_GIVEN
};

static double delay_value_not_given = DELAY_VALUE_NOT_GIVEN;

#undef DELAY_VALUE_NOT_GIVEN
#define DELAY_VALUE_NOT_GIVEN delay_value_not_given

int 
bwd_min_delay_trace(network, model)
network_t *network;
delay_model_t model;
{
  int i;
  node_t *node;
  array_t *nodevec;
  double latest;
  lsGen gen;
  delay_node_t *delay;

  error_init();
  if (setjmp(delay_botch)) {
    return 0;
  }

  if (DEF_DELAY(network) == NIL(delay_network_t)) {
    delay_network_alloc(network);
  }

  /* compute the load on each driver */
  foreach_node (network, gen, node) {
    set_fanout_load(node, model);
  }

  /* Compute arrival times */
  nodevec = network_dfs(network);
  for (i = 0; i < array_n(nodevec); i++) {
    node = array_fetch(node_t *, nodevec, i);
    set_arrival_time(node, model);
  }
  array_free(nodevec);

  /* Find latest output */
  (void) min_delay_latest_output(network, &latest);

  /* compute required times */
  nodevec = network_dfs_from_input(network);
  for (i = 0; i < array_n(nodevec); i++) {
    node = array_fetch(node_t *, nodevec, i);
    set_required_time(node, latest, model);
  }
  array_free(nodevec);

  /* finally, compute the slacks */
  foreach_node (network, gen, node) {
    delay = DELAY(node);
    delay->slack.rise = delay->required.rise - delay->arrival.rise;
    delay->slack.fall = delay->required.fall - delay->arrival.fall;
  }

  return 1;
}

#define ARRIVAL(time, new_time) 	_arrival(&(time), (new_time))

static void
_arrival(time, new_time)
double *time, new_time;
{
  if (new_time < *time) {
    *time = new_time;
  }
}

static void
set_arrival_time(node, model)
register node_t *node;
delay_model_t model;
{
  register int i;
  register node_t *fanin;
  delay_pin_t *pin_delay;
  delay_time_t delay, *node_arrival, fanin_arrival;
  delay_node_t *node_delay;
  pin_phase_t phase;

  node_delay = DELAY(node);

  if (node_type(node) == PRIMARY_INPUT) {
    node_delay->arrival = pi_arrival_time(node);
	
    pin_delay = get_pin_delay(node, /* pin */ 0, model);
    delay = compute_delay(node, pin_delay, model);
	
    node_delay->arrival.rise += delay.rise;
    node_delay->arrival.fall += delay.fall;
    return;
  }
  else if (node_type(node) == PRIMARY_OUTPUT) {
    fanin = node_get_fanin(node, 0);
    node_delay->arrival.rise = DELAY(fanin)->arrival.rise;
    node_delay->arrival.fall = DELAY(fanin)->arrival.fall;
    return;
  } 

  node_arrival = &node_delay->arrival;
  node_arrival->rise = INFINITY;
  node_arrival->fall = INFINITY;

  foreach_fanin (node, i, fanin) {
    pin_delay = get_pin_delay(node, i, model);
    delay = compute_delay(node, pin_delay, model);
	
    fanin_arrival = DELAY(fanin)->arrival;
    phase = pin_delay->phase;
    if (phase == PHASE_NOT_GIVEN) {
      delay_error("set_arrival_time: bad pin phase\n");
    }
    if (phase == PHASE_INVERTING || phase == PHASE_NEITHER) {
      ARRIVAL(node_arrival->rise, fanin_arrival.fall + delay.rise);
      ARRIVAL(node_arrival->fall, fanin_arrival.rise + delay.fall);
    }
    if (phase == PHASE_NONINVERTING || phase == PHASE_NEITHER) {
      ARRIVAL(node_arrival->rise, fanin_arrival.rise + delay.rise);
      ARRIVAL(node_arrival->fall, fanin_arrival.fall + delay.fall);
    }
  }
}

/* Compute the arrival time for a primary input. */

static delay_time_t
pi_arrival_time(node)
node_t *node;
{
  delay_time_t delay;
  delay_pin_t *pin_delay;

  pin_delay = DELAY(node)->pin_delay;
  delay = DEF_DELAY(node_network(node))->default_arrival;

  if (pin_delay != 0 && 
      pin_delay->user_time.rise != DELAY_VALUE_NOT_GIVEN &&
      pin_delay->user_time.fall != DELAY_VALUE_NOT_GIVEN) {
    return(pin_delay->user_time);
  }
  else if (delay.rise != DELAY_VALUE_NOT_GIVEN     &&
	   delay.fall != DELAY_VALUE_NOT_GIVEN) {
    return(delay);
  }
  else {
    delay.rise = 0;
    delay.fall = 0;
    return(delay);
  }
}


#define REQUIRED(time, new_time)	_required(&(time), (new_time))

static void
_required(time, new_time)
double *time, new_time;
{
  if (new_time < *time) {
    *time = new_time;
  }
}

static void
set_required_time(node, latest, model)
node_t *node;
double latest;
delay_model_t model;
{
  int pin;
  node_t *fanout;
  delay_pin_t *pin_delay;
  delay_time_t delay, *node_required, fanout_required;
  delay_node_t *node_delay;
  pin_phase_t phase;
  lsGen gen;

  node_delay = DELAY(node);
  if (node_type(node) == PRIMARY_OUTPUT) {
    node_delay->required = po_required_time(node, latest);
    return;
  }

  node_required = &node_delay->required;
  node_required->rise = -INFINITY;
  node_required->fall = -INFINITY;

  foreach_fanout_pin(node, gen, fanout, pin) {
    fanout_required = DELAY(fanout)->required;
    if (node_type(fanout) == PRIMARY_OUTPUT) {
      REQUIRED(node_required->rise, fanout_required.rise);
      REQUIRED(node_required->fall, fanout_required.fall);
      continue;
    }

    pin_delay = get_pin_delay(fanout, pin, model);
    delay = compute_delay(fanout, pin_delay, model);
    phase = pin_delay->phase;
    if (phase == PHASE_NOT_GIVEN) {
      delay_error("set_required_time: bad pin phase\n");
    }
    if (phase == PHASE_INVERTING || phase == PHASE_NEITHER) {
      REQUIRED(node_required->rise, fanout_required.fall - delay.fall);
      REQUIRED(node_required->fall, fanout_required.rise - delay.rise);
    }
    if (phase == PHASE_NONINVERTING || phase == PHASE_NEITHER) {
      REQUIRED(node_required->rise, fanout_required.rise - delay.rise);
      REQUIRED(node_required->fall, fanout_required.fall - delay.fall);
    }
  }
}

/* Compute the required time at a primary output */

static delay_time_t
po_required_time(node,  latest)
node_t *node;
double latest;
{
  delay_time_t delay;
  delay_pin_t *pin_delay;

  pin_delay = DELAY(node)->pin_delay;
  delay = DEF_DELAY(node_network(node))->default_required;

  if (pin_delay != NIL(delay_pin_t) 				   &&
      pin_delay->user_time.rise != DELAY_VALUE_NOT_GIVEN &&
      pin_delay->user_time.fall != DELAY_VALUE_NOT_GIVEN) {
    return(pin_delay->user_time);
  }
  else if (delay.rise != DELAY_VALUE_NOT_GIVEN     &&
	   delay.fall != DELAY_VALUE_NOT_GIVEN) {
    return(delay);
  }
  else {
    delay.rise = latest;
    delay.fall = latest;
    return(delay);
  }
}


static void
set_fanout_load(node, model)
node_t *node;
delay_model_t model;
{
  int pin;
  node_t *fanout;
  delay_pin_t *pin_delay;
  lsGen gen;

  /* Sum the output capacitance (load of each pin we fanout to) */
    
  DELAY(node)->load = compute_wire_load(node_network(node), node_num_fanout(node));
  foreach_fanout_pin (node, gen, fanout, pin) {
    pin_delay = get_pin_delay(fanout, pin, model);
    DELAY(node)->load += pin_delay->load;
  }
}

/*
 * Allocate storage for new pin params, attempting to reuse storage where
 * possible.  The number of fanins is the number of delays we need, number of
 * allocated parameters is the number we have.  If we have allocated none,
 * then just do a standard storage allocation.  If we have too few, copy the
 * storage we do have and allocate the rest.  If we have too many, copy what
 * we need and free the rest.  If we have just the right number (I suspect,
 * the usual case), then do nothing.
 */
static void
allocate_pin_params(node)
node_t *node;
{
  int nparams, n_old_params, i;
  delay_pin_t **old_pins, **new_pins;
  delay_node_t *delay;

  nparams = node_num_fanin(node);
  delay = DELAY(node);
  n_old_params = delay->num_pin_params;

  if (delay->pin_params == NIL(delay_pin_t *)) {
    delay->num_pin_params = nparams;
    new_pins = ALLOC(delay_pin_t *, nparams);
    for (i = 0; i < nparams; i++) {
      new_pins[i] = ALLOC(delay_pin_t, 1);
    }
    delay->pin_params = new_pins;
  }
  else if (nparams != n_old_params) {
    old_pins = delay->pin_params;
    new_pins = ALLOC(delay_pin_t *, nparams);
	
    if (n_old_params < nparams) {
      for (i = 0; i < n_old_params; i++) {
	new_pins[i] = old_pins[i];
      }
      for (i = n_old_params; i < nparams; i++) {
	new_pins[i] = ALLOC(delay_pin_t, 1);
      }
    }
    else {
      for (i = 0; i < n_old_params; i++) {
	new_pins[i] = old_pins[i];
      }
      for (i = nparams; i < n_old_params; ++i) {
	FREE(old_pins[i]);
      }
    }
    FREE(old_pins);
    delay->pin_params = new_pins;
    /* BUG FIX: by KJ --- Set the field for the no. of parameters */
    delay->num_pin_params = nparams;
  }
}

/*
 * Essentially similar to compute mapped params  -- but also returns the
 * mapped network. Put on request of the speedup package where a node may
 * be replaced by its decomposition
 */
network_t *
min_delay_generate_decomposition(node, mode)
node_t *node;
double mode;
{
  network_t *network;
  int i, found = 0;
  delay_pin_t *pin, *pin1;
  lsGen gen, gen1;
  node_t *this_node, *out_node, *fanin, *p, *q, *r;
  char *name;
  delay_time_t *user_time;

  /* Just in case one did not allocate the storage for the mapped model */
  allocate_pin_params(node);
  if ((network = map_node_to_network(node, mode)) == NIL(network_t))
    delay_error(NIL(char));

  /*
   * Strategy here is to assume that all the delay at reaching the output is
   * due to the ith input pin.  Hence, we begin by setting the arrival times
   * of all inputs to INFINITY and the output loads driven to be 0.  This
   * means (again, for linear models) that the delay across the node is
   * ENTIRELY due to the set delay for the ith pin.
   */

  foreach_primary_output(network, gen, out_node) { /* only one */
    DELAY(out_node)->load = 0;
  }

  foreach_primary_input(network, gen, this_node) {
    delay_force_pininfo(this_node);
    /*
      delay_set_parameter(this_node, DELAY_ARRIVAL_RISE, INFINITY);
      delay_set_parameter(this_node, DELAY_ARRIVAL_FALL, INFINITY); 
      */
    user_time = &DELAY(this_node)->pin_delay->user_time;
    user_time->rise = INFINITY;
    user_time->fall = INFINITY;
  }
    
  foreach_primary_input(network, gen, this_node){
    name = node_name(this_node);
    found = 0;

    /*
     * Find the pin which corresponds to input this_node of the mapped
     * network
     */
    foreach_fanin(node, i, fanin) {
      if (strcmp(name, node_name(fanin)) == 0) {
	found = 1;
	break;
      }
    }
    if (found == 0) {
      delay_error("Severe internal error in mapped delay model");
    }
		
    pin = DELAY(node)->pin_params[i];

    /*
     * Compute the phase.  Computes node = pf + qf' + r (f == fanin).
     * Node is inverting iff q != 0; both if p != 0, q != 0; noninverting
     * otherwise
     */
    node_algebraic_cofactor(node, fanin, &p, &q, &r);

    if (node_function(q) == NODE_0) {
      pin->phase = PHASE_NONINVERTING;
    }
    else if (node_function(p) == NODE_0) {
      pin->phase = PHASE_INVERTING;
    }
    else {
      pin->phase = PHASE_NEITHER;
    }

    node_free(p);
    node_free(q);
    node_free(r);

    user_time = &DELAY(this_node)->pin_delay->user_time;
    user_time->rise = 0;
    user_time->fall = 0;
	
    /*
     * Compute block parameter.  This is the delay result when the network
     * output is driving a load of 0.  The drive parameter, similarly, is
     * equal to the drive parameter of the node driving the output.
     */
    (void) bwd_min_delay_trace(network, DELAY_MODEL_LIBRARY);

    foreach_primary_output(network, gen1, out_node) { /* only one */
      pin->block = delay_arrival_time(out_node);
      /*
       * It turns out that the drive parameter for each pin of node is
       * equal to the drive parameter of the (single) fanin of the
       * output node of the mapped network.  Get this fanin.  We do it
       * through a foreach loop because that's the easiest way to do it,
       * but we ought to go through this loop only once.  Ditto for the
       * outer loop
       */
      pin1 = get_pin_delay(out_node, 0, DELAY_MODEL_LIBRARY);
      pin->drive = pin1->drive;

    }

    /* Compute the load.  This is a little tricky, so watch the comment */
	
    pin -> load = DELAY(this_node) -> load;

    /* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
       Naive and WRONG.  The load on this_node (a PI) is merely the
       load on the buffer it drives.  What we want is the load driven by
       that buffer.  The following code gets it: */
	   

    /*
      foreach_fanout_pin(this_node, gen1, p, i) {
      pin -> load = DELAY(p) -> load;
      }
      */

	
#ifdef DEBUG
    printf("Pin parameters for node %s, pin %s\n", node_name(node),
	   node_name(fanin));
    printf("pin %d br %f bf %f dr %f df %f phase %s load %f alt_load %f\n",
	   i, pin -> block.rise, pin -> block.fall,
	   pin -> drive.rise, pin -> drive.fall,
	   (pin -> phase == PHASE_INVERTING?"neg":
	    (pin -> phase == PHASE_NONINVERTING?"pos":"both")),
	   pin -> load, DELAY(this_node) -> load);
#endif
	

    /* reset this node's user time given to be INFINITY for next
       iteration */

    user_time = &DELAY(this_node)->pin_delay->user_time;
    user_time->rise = INFINITY;
    user_time->fall = INFINITY;
  }
  return(network);
}

/* Model dependent code */
static delay_time_t
compute_delay(node, pin_delay, model)
node_t *node;
delay_pin_t *pin_delay;
delay_model_t model;
{
  delay_time_t delay;
    
  switch(model) {
  case DELAY_MODEL_UNIT:
  case DELAY_MODEL_UNIT_FANOUT:
  case DELAY_MODEL_LIBRARY:
  case DELAY_MODEL_MAPPED:
    delay.rise = pin_delay->drive.rise * DELAY(node)->load;
    delay.fall = pin_delay->drive.fall * DELAY(node)->load;
    if (node_type(node) != PRIMARY_INPUT) {
      delay.rise += pin_delay->block.rise;
      delay.fall += pin_delay->block.fall;
    }
    break;
  default:
    delay_error("compute_delay: bad model type");
    /* NOTREACHED */
  }
  return delay;
}
	    
static void
delay_error(string)
char *string;
{
  if (string != NIL(char)) {
    error_append(string);
  }
  longjmp(delay_botch, 1);
}

#define DELAY_NODE_CHECK(network, node, fname) \
    network = network_of_node(node);\
    if(!network_has_been_traced(network)) {\
	fail("fname called before trace done\n");\
    } else if (network_has_been_modified(network)) {\
	(void)fprintf(siserr,\
		"Warning: network modified between delay_trace and fname\n");\
    }
    

node_t *
min_delay_latest_output(network, latest_p)
network_t *network;
double *latest_p;
{
  node_t *po, *last_output;
  lsGen gen;
  double latest;
  delay_time_t arrival;

  /*  Goes in when I get the name of the fns from RR 
      DELAY_NODE_CHECK(network, node, min_delay_latest_output);
      */
     
  /* Find latest output */
  latest = INFINITY;
  last_output = 0;

  foreach_primary_output (network, gen, po) {
    arrival = DELAY(po)->arrival;
    if (arrival.rise > latest) {
      latest = arrival.rise;
      last_output = po;
    }
    if (arrival.fall > latest) {
      latest = arrival.fall;
      last_output = po;
    }
  }

  *latest_p = latest;
  return(last_output);
}


static void
delay_force_pininfo(node)
node_t *node;
{
  delay_pin_t *pin_delay;
    
  assert(node != NIL(node_t));
  if (DELAY(node) == NIL(delay_node_t)) {
    delay_alloc(node);
  }
  if (DELAY(node)->pin_delay == NIL(delay_pin_t)) {
    DELAY(node)->pin_delay = pin_delay = ALLOC(delay_pin_t, 1);
    pin_delay->block = time_not_given;
    pin_delay->drive = time_not_given;
    /* BUG FIX: by KJ --- Earlier the phase was not initialized */
    pin_delay->phase = PHASE_NOT_GIVEN;
    pin_delay->load = DELAY_VALUE_NOT_GIVEN;
    pin_delay->max_load = DELAY_VALUE_NOT_GIVEN;
    pin_delay->user_time = time_not_given;
  }
}
    
static network_t *
map_node_to_network(node, mode)
node_t *node;
double mode;
{
  network_t *net1, *net2;
  library_t *library;

  if ((library = lib_get_library()) == NIL(library_t)) {
    delay_error("Mapped Delay model: no current library\n");
    return(NIL(network_t)); /* Not reached -- in to keep lint happy */
  } else {
    net1 = network_create_from_node(node);
    net2 = map_network(net1, library, mode, 1, 0); /* user spec mode & inverters */
    network_free(net1);
    return net2;
  }
}

#undef  node_type
#endif /* SIS */
