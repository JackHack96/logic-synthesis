
/*
 * The purpose of this package is the computation of node and network delays
 * in SIS.  Delays are returned as delay_time_t structures, which contain two
 * doubles, rise and fall.
 *
 * This package is designed to manipulate the basic timing package.  In
 * particular, routines are provided to compute the delay of a node for both
 * rising and falling times, to reset the delay model to a specialized
 * function, if desired, or to provide parameters to the default delay model.
 */
#include "delay_int.h"
#include "tdc_int.h"
#include "sis.h"
#include <setjmp.h>

static jmp_buf delay_botch;

static void set_arrival_time();

static void set_required_time();

static void set_fanout_load();

static void compute_mapped_params();

static void delay_error();

static void allocate_pin_params();

static delay_time_t compute_delay();

static network_t *map_node_to_network();

static void delay_force_pininfo();

static void modelcpy();

#if !defined(lint) && !defined(SABER)

static void print_pin_delays();

#endif

static delay_pin_t *io_delay();

static delay_time_t pi_arrival_time();

static delay_time_t po_required_time();

/*
 * These static vars are here just to provide the constants.  No network
 * information is stored into them.
 */
static delay_pin_t unit_delay_model = {
    /* block */ {1.0, 1.0},
    /* drive */ {0.0, 0.0},
    /* phase */ PHASE_NONINVERTING,
    /* load */ 1.0,
    /* maxload */ INFINITY};

static delay_pin_t unit_delay_model_with_fanout = {
    /* block */ {1.0, 1.0},
    /* drive */ {0.2, 0.2},
    /* phase */ PHASE_NONINVERTING,
    /* load */ 1.0,
    /* maxload */ INFINITY};

static delay_pin_t unspecified_pipo_delay_model = {
    /* block */ {DELAY_VALUE_NOT_GIVEN, DELAY_VALUE_NOT_GIVEN},
    /* drive */ {DELAY_VALUE_NOT_GIVEN, DELAY_VALUE_NOT_GIVEN},
    /* phase */ PHASE_NONINVERTING,
    /* load */ DELAY_NOT_SET,
    /* maxload */ DELAY_NOT_SET,
    /* user_time */ {DELAY_VALUE_NOT_GIVEN, DELAY_VALUE_NOT_GIVEN}};

/*
 * "REASONABLE" values to assume for the parameters of a model when nothing
 * is specified by the user
 */
static delay_pin_t backup_default_pipo_model = {
    /* block */ {0.0, 0.0},
    /* drive */ {0.0, 0.0},
    /* phase */ PHASE_NONINVERTING,
    /* load */ 0.0,
    /* maxload */ INFINITY,
    /* user_time */ {0.0, 0.0}};

static delay_time_t time_not_given = {DELAY_VALUE_NOT_GIVEN,
                                      DELAY_VALUE_NOT_GIVEN};

static double delay_value_not_given = DELAY_VALUE_NOT_GIVEN;

#undef DELAY_VALUE_NOT_GIVEN
#define DELAY_VALUE_NOT_GIVEN delay_value_not_given

int delay_trace(network, model) network_t *network;
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
  nodevec = network_dfs(network);
  for (i = 0; i < array_n(nodevec); i++) {
    node = array_fetch(node_t *, nodevec, i);
    set_fanout_load(node, model);
  }

  /* Compute arrival times */
  for (i = 0; i < array_n(nodevec); i++) {
    node = array_fetch(node_t *, nodevec, i);
    set_arrival_time(node, model);
  }
  array_free(nodevec);

  /* Find latest output */
  (void)delay_latest_output(network, &latest);

  /* compute required times */
  nodevec = network_dfs_from_input(network);
  for (i = 0; i < array_n(nodevec); i++) {
    node = array_fetch(node_t *, nodevec, i);
    set_required_time(node, latest, model);
  }
  array_free(nodevec);

  /* finally, compute the slacks */
  foreach_node(network, gen, node) {
    delay = DELAY(node);
    delay->slack.rise = delay->required.rise - delay->arrival.rise;
    delay->slack.fall = delay->required.fall - delay->arrival.fall;
  }

  return 1;
}

delay_model_t delay_get_model_from_name(name) char *name;
{
  delay_model_t model;

  if (strcmp(name, "unit") == 0) {
    model = DELAY_MODEL_UNIT;
  } else if (strcmp(name, "unit-fanout") == 0) {
    model = DELAY_MODEL_UNIT_FANOUT;
  } else if (strcmp(name, "library") == 0) {
    model = DELAY_MODEL_LIBRARY;
  } else if (strcmp(name, "mapped") == 0) {
    model = DELAY_MODEL_MAPPED;
  } else if (strcmp(name, "tdc") == 0) {
    model = DELAY_MODEL_TDC;
  } else {
    model = DELAY_MODEL_UNKNOWN;
  }
  return model;
}

void delay_print_wire_loads(network, fp) network_t *network;
FILE *fp;
{
  int i, n;
  double load;
  delay_wire_load_table_t wire_load_table;

  if (DEF_DELAY(network) == NIL(delay_network_t)) {
    delay_network_alloc(network);
  }
  wire_load_table = DEF_DELAY(network)->wire_load_table;

  (void)fprintf(fp, "\t\twire_load slope= ");
  if (wire_load_table.slope == DELAY_NOT_SET)
    (void)fprintf(sisout, "%s\n", DELAY_NOT_SET_STRING);
  else
    (void)fprintf(sisout, "%5.3f\n", wire_load_table.slope);
  n = wire_load_table.num_pins_set;

  if (n > 0) {
    (void)fprintf(fp, "\t\tfanout(wire_load)=");
    for (i = 0; i < n;) {
      load = array_fetch(double, wire_load_table.pins, i);
      (void)fprintf(fp, "  %d(%5.3f)", ++i, load);
    }
    (void)fputc('\n', fp);
  }
}

static void delay_print_blif_slif_wire_loads(network, fp,
                                             slif_flag) network_t *network;
FILE *fp;
int slif_flag;
{
  int i;
  array_t *pins;
  delay_network_t *delay;
  void io_fputs_break(), io_fputc_break();
#ifdef SIS
  void io_fprintf_break();
#else
  char *buffer[16];
#endif /* SIS */

  delay = DEF_DELAY(network);
  if (delay == NIL(delay_network_t)) {
    return;
  }
  pins = delay->wire_load_table.pins;
  if (pins == NIL(array_t)) {
    return;
  }
  io_fputs_break(fp, (slif_flag == 0) ? ".wire" : ".global_attribute wire");

  for (i = 0; i < array_n(pins); i++) {
#ifdef SIS
    io_fprintf_break(fp, " %4.2f", array_fetch(double, pins, i));
#else
    (void)sprintf(buffer, " %4.2f", array_fetch(double, pins, i));
    io_fputs_break(fp, buffer);
#endif /* SIS */
  }
  io_fputc_break(fp, '\n');
}

void delay_print_blif_wire_loads(network, fp) network_t *network;
FILE *fp;
{ delay_print_blif_slif_wire_loads(network, fp, 0); }

void delay_print_slif_wire_loads(network, fp) network_t *network;
FILE *fp;
{ delay_print_blif_slif_wire_loads(network, fp, 1); }

#define ARRIVAL(time, new_time) _arrival(&(time), (new_time))

static void _arrival(time, new_time) double *time, new_time;
{
  if (new_time > *time) {
    *time = new_time;
  }
}

static void set_arrival_time(node, model) register node_t *node;
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
  } else if (node_type(node) == PRIMARY_OUTPUT) {
    fanin = node_get_fanin(node, 0);
    node_delay->arrival.rise = DELAY(fanin)->arrival.rise;
    node_delay->arrival.fall = DELAY(fanin)->arrival.fall;
    return;
  }

  node_arrival = &node_delay->arrival;
  node_arrival->rise = -INFINITY;
  node_arrival->fall = -INFINITY;

  foreach_fanin(node, i, fanin) {
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

static delay_time_t pi_arrival_time(node) node_t *node;
{
  delay_time_t delay;
  delay_pin_t *pin_delay;

  pin_delay = DELAY(node)->pin_delay;
  delay = DEF_DELAY(node_network(node))->default_arrival;

  if (pin_delay != 0 && pin_delay->user_time.rise != DELAY_VALUE_NOT_GIVEN &&
      pin_delay->user_time.fall != DELAY_VALUE_NOT_GIVEN) {
    return (pin_delay->user_time);
  } else if (delay.rise != DELAY_VALUE_NOT_GIVEN &&
             delay.fall != DELAY_VALUE_NOT_GIVEN) {
    return (delay);
  } else {
    delay.rise = 0;
    delay.fall = 0;
    return (delay);
  }
}

#define REQUIRED(time, new_time) _required(&(time), (new_time))

static void _required(time, new_time) double *time, new_time;
{
  if (new_time < *time) {
    *time = new_time;
  }
}

static void set_required_time(node, latest, model) node_t *node;
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
  node_required->rise = INFINITY;
  node_required->fall = INFINITY;

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

/*
 * After a delay trace, returns the required time at the "fanin_index" of
 * node. Can be different from the required time of the "fanin_index" fanin
 */
int delay_wire_required_time(node, fanin_index, model, req) node_t *node;
int fanin_index;
delay_model_t model;
delay_time_t *req;
{
  delay_pin_t *pin_delay;
  delay_time_t delay, ip_req;
  pin_phase_t phase;

  if (node->type != PRIMARY_INPUT &&
      (fanin_index < 0 || fanin_index >= node_num_fanin(node))) {
    return 0;
  }
  if (node->type == PRIMARY_OUTPUT) {
    *req = delay_required_time(node);
    return 1;
  }

  pin_delay = get_pin_delay(node, fanin_index, model);
  delay = compute_delay(node, pin_delay, model);
  if (node->type == PRIMARY_INPUT) {
    phase = PHASE_NONINVERTING;
  } else {
    phase = pin_delay->phase;
    if (phase == PHASE_NOT_GIVEN) {
      return 0;
    }
  }

  *req = delay_required_time(node);
  ip_req.rise = INFINITY;
  ip_req.fall = INFINITY;

  if (phase == PHASE_INVERTING || phase == PHASE_NEITHER) {
    REQUIRED(ip_req.rise, req->fall - delay.fall);
    REQUIRED(ip_req.fall, req->rise - delay.rise);
  }
  if (phase == PHASE_NONINVERTING || phase == PHASE_NEITHER) {
    REQUIRED(ip_req.rise, req->rise - delay.rise);
    REQUIRED(ip_req.fall, req->fall - delay.fall);
  }

  *req = ip_req;

  return 1;
}
/*
 * After a delay trace, returns the slack time at the "fanin_index" of
 * node. Can be different from the slack time of the "fanin_index" fanin
 */
int delay_wire_slack_time(node, fanin_index, model, slack) node_t *node;
int fanin_index;
delay_model_t model;
delay_time_t *slack;
{
  delay_time_t req, arr;

  if (node->type == PRIMARY_INPUT) {
    arr = delay_arrival_time(node);
  } else {
    arr = delay_arrival_time(node_get_fanin(node, fanin_index));
  }
  if (delay_wire_required_time(node, fanin_index, model, &req)) {
    slack->rise = req.rise - arr.rise;
    slack->fall = req.fall - arr.fall;
    return 1;
  } else {
    return 0;
  }
}

/* Compute the required time at a primary output */

static delay_time_t po_required_time(node, latest) node_t *node;
double latest;
{
  delay_time_t delay;
  delay_pin_t *pin_delay;

  pin_delay = DELAY(node)->pin_delay;
  delay = DEF_DELAY(node_network(node))->default_required;

  if (pin_delay != NIL(delay_pin_t) &&
      pin_delay->user_time.rise != DELAY_VALUE_NOT_GIVEN &&
      pin_delay->user_time.fall != DELAY_VALUE_NOT_GIVEN) {
    return (pin_delay->user_time);
  } else if (delay.rise != DELAY_VALUE_NOT_GIVEN &&
             delay.fall != DELAY_VALUE_NOT_GIVEN) {
    return (delay);
  } else {
    delay.rise = latest;
    delay.fall = latest;
    return (delay);
  }
}

void delay_compute_atime(node, model, load) node_t *node;
delay_model_t model;
double load;
{
  DELAY(node)->load = load;
  DELAY(node)->pin_params_valid = DELAY_MODEL_UNKNOWN;
  set_arrival_time(node, model);
}

static void set_fanout_load(node, model) node_t *node;
delay_model_t model;
{
  int pin;
  lsGen gen;
  double load;
  node_t *fanout;
  delay_pin_t *pin_delay;

  /* Sum the output capacitance (load of each pin we fanout to) */

  DELAY(node)->load =
      compute_wire_load(node_network(node), node_num_fanout(node));
  if (model != DELAY_MODEL_TDC) {
    foreach_fanout_pin(node, gen, fanout, pin) {
      if (fanout->type == PRIMARY_OUTPUT) {
        load = delay_get_default_parameter(node_network(fanout),
                                           DELAY_DEFAULT_OUTPUT_LOAD);
        if (load == DELAY_NOT_SET)
          load = 0.0;
        (void)delay_get_po_load(fanout, &load);
        DELAY(node)->load += load;
      } else {
        pin_delay = get_pin_delay(fanout, pin, model);
        DELAY(node)->load += pin_delay->load;
      }
    }
  } else {
    /*
     * For the TDC model we have no need to compute the fanout load
     */
    DELAY(node)->load = 0.0;
  }
}

/*
 * Non-invasive version of the set_fanout_load() command. It will return
 * the load at the fanouts without storing them with the node
 */
double delay_compute_fo_load(node, model) node_t *node;
delay_model_t model;
{
  int pin;
  lsGen gen;
  double op_load, load;
  node_t *fanout;
  delay_pin_t *pin_delay;

  /* Sum the output capacitance (load of each pin we fanout to) */

  op_load = compute_wire_load(node_network(node), node_num_fanout(node));
  foreach_fanout_pin(node, gen, fanout, pin) {
    if (fanout->type == PRIMARY_OUTPUT) {
      load = delay_get_default_parameter(node_network(fanout),
                                         DELAY_DEFAULT_OUTPUT_LOAD);
      if (load == DELAY_NOT_SET)
        load = 0.0;
      (void)delay_get_po_load(fanout, &load);
      op_load += load;
    } else {
      pin_delay = get_pin_delay(fanout, pin, model);
      op_load += pin_delay->load;
    }
  }
  return op_load;
}

/*
 * The following strategy is used to get the wire load:
 * If the array has a value for that fanout, use it
 * else compute it by a linear model depending on slope
 * else if slope not spec, extrapolate from values given.
 */
double compute_wire_load(network, pins) network_t *network;
int pins;
{
  int n;
  double val;
  delay_network_t *delay;
  delay_wire_load_table_t wire_load_table;

  if (pins == 0 || network == NIL(network_t)) {
    return 0.0;
  }
  if (pins < 0) {
    delay_error("compute_wire_load: node with non-positive fanout");
  }

  delay = DEF_DELAY(network);
  if (delay == NIL(delay_network_t)) {
    delay_network_alloc(network);
    delay = DEF_DELAY(network);
  }
  wire_load_table = delay->wire_load_table;
  n = wire_load_table.num_pins_set;
  if (pins > n) {
    if (wire_load_table.slope > 0.0) {
      return (wire_load_table.slope * pins);
    } else {
      if (n == 0) {
        return (0.0);
      }
      val = array_fetch(double, wire_load_table.pins, n - 1);
      if (n == 1) {
        return (val);
      } else {
        return (val * (1 + (pins - n)) -
                (pins - n) * array_fetch(double, wire_load_table.pins, n - 2));
      }
    }
  } else {
    return (array_fetch(double, wire_load_table.pins, pins - 1));
  }
}

/*
 * Get the pin delay parameters for a node.  This is highly model-dependent
 * code. This returns a delay_pin_t *, and should contain all the necessary
 * information to for compute_delay to compute the delay through this pin.
 * Further, the code in this routine for each model MUST fill in the capacitive
 * load slot (a double) and the phase slot (of type pin_phase_t) in the
 * returned delay_pin_t.  These slots are read by model-independent code.
 */
delay_pin_t *get_pin_delay(node, pin, model) node_t *node;
int pin;
delay_model_t model;
{
  char errmsg[1024];
  delay_pin_t *delay;
  input_phase_t phase;
  /* Need structure to modify the models based on the phase of the gate */
  static delay_pin_t internal_delay_model;

  if (node_type(node) == PRIMARY_INPUT) {
    return (io_delay(node, model));
  }
  if (node_type(node) == PRIMARY_OUTPUT) {
    return (io_delay(node, model));
  }

  switch (model) {
  case DELAY_MODEL_UNIT:
    internal_delay_model.block = unit_delay_model.block;
    internal_delay_model.drive = unit_delay_model.drive;
    internal_delay_model.load = unit_delay_model.load;
    internal_delay_model.max_load = unit_delay_model.max_load;
    /* Set the PHASE based on the function at the node */
    phase = node_input_phase(node, node_get_fanin(node, pin));
    if (phase == POS_UNATE) {
      internal_delay_model.phase = PHASE_NONINVERTING;
    } else if (phase == NEG_UNATE) {
      internal_delay_model.phase = PHASE_INVERTING;
    } else {
      internal_delay_model.phase = PHASE_NEITHER;
    }
    delay = &internal_delay_model;
    break;

  case DELAY_MODEL_UNIT_FANOUT:
    internal_delay_model.block = unit_delay_model_with_fanout.block;
    internal_delay_model.drive = unit_delay_model_with_fanout.drive;
    internal_delay_model.load = unit_delay_model_with_fanout.load;
    internal_delay_model.max_load = unit_delay_model_with_fanout.max_load;
    /* Set the PHASE based on the function at the node */
    phase = node_input_phase(node, node_get_fanin(node, pin));
    if (phase == POS_UNATE) {
      internal_delay_model.phase = PHASE_NONINVERTING;
    } else if (phase == NEG_UNATE) {
      internal_delay_model.phase = PHASE_INVERTING;
    } else {
      internal_delay_model.phase = PHASE_NEITHER;
    }
    delay = &internal_delay_model;
    break;

  case DELAY_MODEL_LIBRARY:
    delay = (delay_pin_t *)lib_get_pin_delay(node, pin);
    if (delay == NIL(delay_pin_t)) {
      (void)sprintf(errmsg, "cannot use library model: node '%s' not mapped\n",
                    node_name(node));
      delay_error(errmsg);
      /* NOTREACHED */
    }
    break;

  case DELAY_MODEL_MAPPED:
    delay = (delay_pin_t *)lib_get_pin_delay(node, pin);
    if (delay == NIL(delay_pin_t)) {
      /*
       * Node is not mapped.  First check to see if delays have been
       * computed.  If not, compute them.  Also checks to see if we can
       * use existing array.
       */
      if (DELAY(node)->pin_params_valid == DELAY_MODEL_UNKNOWN) {

#ifdef DEBUG
        printf("Computing mapped parameters for node %s\n", node_name(node));
#endif
        allocate_pin_params(node);
        compute_mapped_params(node, 0.0);
        DELAY(node)->pin_params_valid = DELAY_MODEL_MAPPED;
      }
      delay = DELAY(node)->pin_params[pin];
    }
    break;
  case DELAY_MODEL_TDC:
    if (DELAY(node)->pin_params_valid != DELAY_MODEL_TDC) {
#ifdef DEBUG
      printf("Computing tdc parameters for node %s\n", node_name(node));
#endif
      allocate_pin_params(node);
      /* Use complex tdc model with fanout */
      compute_tdc_params(node, 2);
      DELAY(node)->pin_params_valid = DELAY_MODEL_TDC;
    }
    delay = DELAY(node)->pin_params[pin];
    break;
  default:
    delay_error("get_pin_delay: bad model type");
    /* NOTREACHED */
  }
  return (delay);
}

/*
 * Compute the delays on the io pins of the network.  Model-dependent
 */
static delay_pin_t *io_delay(node, model) node_t *node;
delay_model_t model;
{
  double val;
  delay_time_t drive;
  static delay_pin_t pipo_delay_model; /* structure to pass data */

  switch (model) {
  case DELAY_MODEL_UNIT:
    return (&unit_delay_model);
  case DELAY_MODEL_UNIT_FANOUT:
    pipo_delay_model.block = unit_delay_model_with_fanout.block;
    pipo_delay_model.drive = unit_delay_model_with_fanout.drive;
    pipo_delay_model.phase = unit_delay_model_with_fanout.phase;
    pipo_delay_model.load = unit_delay_model_with_fanout.load;
    pipo_delay_model.max_load = unit_delay_model_with_fanout.max_load;
    if (delay_get_pi_drive(node, &drive)) {
      pipo_delay_model.drive = drive;
    } else {
      val = delay_get_default_parameter(node_network(node),
                                        DELAY_DEFAULT_DRIVE_RISE);
      if (val != DELAY_NOT_SET)
        pipo_delay_model.drive.rise = val;
      val = delay_get_default_parameter(node_network(node),
                                        DELAY_DEFAULT_DRIVE_FALL);
      if (val != DELAY_NOT_SET)
        pipo_delay_model.drive.fall = val;
    }
    return (&pipo_delay_model);
  case DELAY_MODEL_LIBRARY:
  case DELAY_MODEL_MAPPED:
  case DELAY_MODEL_TDC:
    if (DELAY(node)->pin_delay != NIL(delay_pin_t)) {
      pipo_delay_model = *(DELAY(node)->pin_delay);
      modelcpy(&pipo_delay_model, DELAY(node)->pin_delay, node);
    } else {
      modelcpy(&pipo_delay_model, &unspecified_pipo_delay_model, node);
    }
    return (&pipo_delay_model);
  default:
    fail("io_delay: bad model");
    /*NOTREACHED*/
  }
}

/*
 * Fill in slots with default values where the DELAY_VALUE is NOT_GIVEN.
 * Since the defaults are settable, they too may be NOT_GIVEN, so we need a
 * backup.
 */
static void modelcpy(newpin, pin, node) delay_pin_t *newpin, *pin;
node_t *node;
{
  delay_network_t *net_delay;
  delay_pin_t default_pipo_model;
  double value;
  pin_phase_t phase;

  if ((net_delay = DEF_DELAY(node_network(node))) != NIL(delay_network_t)) {
    default_pipo_model = net_delay->pipo_model;
  } else {
    default_pipo_model = unspecified_pipo_delay_model;
  }

  if (pin->block.rise == DELAY_VALUE_NOT_GIVEN) {
    value = default_pipo_model.block.rise;
    newpin->block.rise = (value == DELAY_VALUE_NOT_GIVEN)
                             ? backup_default_pipo_model.block.rise
                             : value;
  }
  if (pin->block.fall == DELAY_VALUE_NOT_GIVEN) {
    value = default_pipo_model.block.fall;
    newpin->block.fall = (value == DELAY_VALUE_NOT_GIVEN)
                             ? backup_default_pipo_model.block.fall
                             : value;
  }
  if (pin->drive.rise == DELAY_VALUE_NOT_GIVEN) {
    value = default_pipo_model.drive.rise;
    newpin->drive.rise = (value == DELAY_VALUE_NOT_GIVEN)
                             ? backup_default_pipo_model.drive.rise
                             : value;
  }
  if (pin->drive.fall == DELAY_VALUE_NOT_GIVEN) {
    value = default_pipo_model.drive.fall;
    newpin->drive.fall = (value == DELAY_VALUE_NOT_GIVEN)
                             ? backup_default_pipo_model.drive.fall
                             : value;
  }
  if (pin->load == DELAY_VALUE_NOT_GIVEN) {
    value = default_pipo_model.load;
    newpin->load = (value == DELAY_VALUE_NOT_GIVEN)
                       ? backup_default_pipo_model.load
                       : value;
  }
  if (pin->max_load == DELAY_VALUE_NOT_GIVEN) {
    value = default_pipo_model.max_load;
    newpin->max_load = (value == DELAY_VALUE_NOT_GIVEN)
                           ? backup_default_pipo_model.max_load
                           : value;
  }
  if (pin->phase == PHASE_NOT_GIVEN) {
    phase = default_pipo_model.phase;
    newpin->phase =
        (phase == PHASE_NOT_GIVEN) ? backup_default_pipo_model.phase : phase;
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
static void allocate_pin_params(node) node_t *node;
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
  } else if (nparams != n_old_params) {
    old_pins = delay->pin_params;
    new_pins = ALLOC(delay_pin_t *, nparams);

    if (n_old_params < nparams) {
      for (i = 0; i < n_old_params; i++) {
        new_pins[i] = old_pins[i];
      }
      for (i = n_old_params; i < nparams; i++) {
        new_pins[i] = ALLOC(delay_pin_t, 1);
      }
    } else {
      for (i = 0; i < nparams; i++) {
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
 * Compute the block, drive and load parameters for node by mapping it naively
 * into a network and then doing a delay trace to derive the parameters.  This
 * is a very inefficient approach, but it is simple and easy to understand.
 * Must be recoded if it turns out to be very inefficient.
 */
static void compute_mapped_params(node, mode) node_t *node;
double mode;
{
  network_t *network;
  int i, found = 0;
  delay_pin_t *pin, *pin1;
  lsGen gen, gen1;
  node_t *this_node, *out_node, *fanin, *p, *q, *r;
  char *name;
  delay_time_t *user_time;

  network = map_node_to_network(node, mode);
  if (network == NIL(network_t)) {
    delay_error(NIL(char));
  }

  /*
   * Strategy here is to assume that all the delay at reaching the output is
   * due to the ith input pin.  Hence, we begin by setting the arrival times
   * of all inputs to -INFINITY and the output loads driven to be 0.  This
   * means (again, for linear models) that the delay across the node is
   * ENTIRELY due to the set delay for the ith pin.
   */

  foreach_primary_output(network, gen, out_node) { /* only one */
    DELAY(out_node)->load = 0;
  }

  foreach_primary_input(network, gen, this_node) {
    delay_force_pininfo(this_node);
    user_time = &DELAY(this_node)->pin_delay->user_time;
    user_time->rise = -INFINITY;
    user_time->fall = -INFINITY;
  }

  foreach_primary_input(network, gen, this_node) {
    name = node_name(this_node);
    found = 0;

    /*
     * Find the pin which corresponds to input this_node of the mapped
     * network
     */
    foreach_fanin(node, i, fanin) {
      if (strcmp(name, node_long_name(fanin)) == 0) {
        found = 1;
        break;
      }
    }

    if (found == 0) {
      delay_error("Severe internal error in mapped delay model");
    }

    pin = DELAY(node)->pin_params[i];
    /*
     * Compute the phase.  computes node = pf + qf' + r (f == fanin).
     * Node is inverting iff q != 0; both if p != 0, q != 0; noninverting
     * otherwise.
     */
    node_algebraic_cofactor(node, fanin, &p, &q, &r);

    if (node_function(q) == NODE_0) {
      pin->phase = PHASE_NONINVERTING;
    } else if (node_function(p) == NODE_0) {
      pin->phase = PHASE_INVERTING;
    } else {
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
    (void)delay_trace(network, DELAY_MODEL_LIBRARY);

    foreach_primary_output(network, gen1, out_node) {
      /* should only be one */

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

    pin->load = DELAY(this_node)->load;

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
    printf("pin %d br %f bf %f dr %f df %f phase %s load %f alt_load %f\n", i,
           pin->block.rise, pin->block.fall, pin->drive.rise, pin->drive.fall,
           (pin->phase == PHASE_INVERTING
                ? "neg"
                : (pin->phase == PHASE_NONINVERTING ? "pos" : "both")),
           pin->load, DELAY(this_node)->load);
#endif

    /* reset this node's user time given to be -INFINITY for next
       iteration */

    user_time->rise = -INFINITY;
    user_time->fall = -INFINITY;
  }
  network_free(network);
}

/*
 * Essentially similar to compute mapped params  -- but also returns the
 * mapped network. Put on request of the speedup package where a node may
 * be replaced by its decomposition
 */
network_t *delay_generate_decomposition(node, mode) node_t *node;
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
   * of all inputs to -INFINITY and the output loads driven to be 0.  This
   * means (again, for linear models) that the delay across the node is
   * ENTIRELY due to the set delay for the ith pin.
   */

  foreach_primary_output(network, gen, out_node) { /* only one */
    DELAY(out_node)->load = 0;
  }

  foreach_primary_input(network, gen, this_node) {
    delay_force_pininfo(this_node);
    /*
    delay_set_parameter(this_node, DELAY_ARRIVAL_RISE, -INFINITY);
    delay_set_parameter(this_node, DELAY_ARRIVAL_FALL, -INFINITY);
    */
    user_time = &DELAY(this_node)->pin_delay->user_time;
    user_time->rise = -INFINITY;
    user_time->fall = -INFINITY;
  }

  foreach_primary_input(network, gen, this_node) {
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
    } else if (node_function(p) == NODE_0) {
      pin->phase = PHASE_INVERTING;
    } else {
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
    (void)delay_trace(network, DELAY_MODEL_LIBRARY);

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

    pin->load = DELAY(this_node)->load;

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
    printf("pin %d br %f bf %f dr %f df %f phase %s load %f alt_load %f\n", i,
           pin->block.rise, pin->block.fall, pin->drive.rise, pin->drive.fall,
           (pin->phase == PHASE_INVERTING
                ? "neg"
                : (pin->phase == PHASE_NONINVERTING ? "pos" : "both")),
           pin->load, DELAY(this_node)->load);
#endif

    /* reset this node's user time given to be -INFINITY for next
       iteration */

    user_time = &DELAY(this_node)->pin_delay->user_time;
    user_time->rise = -INFINITY;
    user_time->fall = -INFINITY;
  }
  return (network);
}

/* Model dependent code */
static delay_time_t compute_delay(node, pin_delay, model) node_t *node;
delay_pin_t *pin_delay;
delay_model_t model;
{
  delay_time_t delay;

  switch (model) {
  case DELAY_MODEL_UNIT:
  case DELAY_MODEL_UNIT_FANOUT:
  case DELAY_MODEL_LIBRARY:
  case DELAY_MODEL_MAPPED:
  case DELAY_MODEL_TDC:
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

static void delay_error(s) char *s;
{
  if (s != NIL(char)) {
    error_append(s);
  }
  longjmp(delay_botch, 1);
}

delay_time_t delay_node_pin(node, pin_num, model) node_t *node;
int pin_num;
delay_model_t model;
{
  delay_pin_t *pin_delay;
  delay_time_t delay;

  delay.rise = delay.fall = 0.0;

  if (node_type(node) == PRIMARY_OUTPUT) {
    return (delay);
  }
  set_fanout_load(node, model);

  pin_delay = get_pin_delay(node, pin_num, model);
  return (compute_delay(node, pin_delay, model));
}

delay_time_t delay_fanout_contribution(node, pin_num, fanout,
                                       model) node_t *node,
    *fanout;
int pin_num;
delay_model_t model;
{
  delay_pin_t *pin_delay;
  delay_time_t delay, delay1, *delay_ptr;
  double load;
  int i, num;

  delay.rise = delay.fall = 0.0;
  i = node_get_fanin_index(fanout, node);
  delay_ptr = &delay;

  if (i == -1) {
    return (delay);
  }

  /*
  set_fanout_load(node, model);
  */

  num = node_num_fanout(node);
  load = (DELAY(node)->load - get_pin_delay(fanout, i, model)->load) +
         (compute_wire_load(node_network(node), num) -
          compute_wire_load(node_network(node), num - 1));

  pin_delay = get_pin_delay(node, pin_num, model);
  delay1 = compute_delay(node, pin_delay, model);
  delay = delay_map_simulate(1, &delay_ptr, (char **)&pin_delay, load);
  delay1.rise -= delay.rise;
  delay1.fall -= delay.fall;
  return (delay1);
}

#define DELAY_NODE_CHECK(network, node, fname)                                 \
  network = network_of_node(node);                                             \
  if (!network_has_been_traced(network)) {                                     \
    fail("fname called before trace done\n");                                  \
  } else if (network_has_been_modified(network)) {                             \
    (void)fprintf(                                                             \
        siserr, "Warning: network modified between delay_trace and fname\n");  \
  }

delay_time_t delay_arrival_time(node) node_t *node;
{
  /*  Goes in when I get the name of the fns from RR

   network_t *network;

   DELAY_NODE_CHECK(network, node, delay_arrival_time);
    */
  return (DELAY(node)->arrival);
}

delay_time_t delay_required_time(node) node_t *node;
{
  /*  Goes in when I get the name of the fns from RR

  network_t *network;

  DELAY_NODE_CHECK(network, node, delay_required_time);
   */
  return (DELAY(node)->required);
}

delay_time_t delay_slack_time(node) node_t *node;
{
  /*  Goes in when I get the name of the fns from RR

  network_t *network;

  DELAY_NODE_CHECK(network, node, delay_slack_time);
   */
  return (DELAY(node)->slack);
}

double delay_load(node) node_t *node;
{
  /*  Goes in when I get the name of the fns from RR

  network_t *network;

  DELAY_NODE_CHECK(network, node, delay_load);
   */

  return (DELAY(node)->load);
}

node_t *delay_latest_output(network, latest_p) network_t *network;
double *latest_p;
{
  node_t *po, *last_output;
  lsGen gen;
  double latest;
  delay_time_t arrival;

  /*  Goes in when I get the name of the fns from RR
  DELAY_NODE_CHECK(network, node, delay_latest_output);
   */

  /* Find latest output */
  latest = -INFINITY;
  last_output = 0;

  foreach_primary_output(network, gen, po) {
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
  return (last_output);
}

static void delay_force_pininfo(node) node_t *node;
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

void delay_set_default_parameter(network, param, value) network_t *network;
delay_param_t param;
double value;
{
  delay_network_t *delay;
  delay_wire_load_table_t *wire_load_table;

  delay = DEF_DELAY(network);
  if (delay == NIL(delay_network_t)) {
    delay_network_alloc(network);
    delay = DEF_DELAY(network);
  }

  wire_load_table = &delay->wire_load_table;
  switch (param) {
  case DELAY_ADD_WIRE_LOAD:
    if (value < 0) {
      if (wire_load_table->pins != NIL(array_t)) {
        array_free(wire_load_table->pins);
      }
      wire_load_table->pins = NIL(array_t);
      wire_load_table->num_pins_set = 0;
    } else {
      if (wire_load_table->pins == NIL(array_t)) {
        wire_load_table->pins = array_alloc(double, 10);
      }
      array_insert_last(double, wire_load_table->pins, value);
      wire_load_table->num_pins_set++;
    }
    return;
  case DELAY_WIRE_LOAD_SLOPE:
    wire_load_table->slope = value;
    return;
  case DELAY_DEFAULT_DRIVE_RISE:
    delay->pipo_model.drive.rise = value;
    return;
  case DELAY_DEFAULT_DRIVE_FALL:
    delay->pipo_model.drive.fall = value;
    return;

  case DELAY_DEFAULT_OUTPUT_LOAD:
    delay->pipo_model.load = value;
    return;
  case DELAY_DEFAULT_MAX_INPUT_LOAD:
    delay->pipo_model.max_load = value;
    return;

  case DELAY_DEFAULT_ARRIVAL_RISE:
    delay->default_arrival.rise = value;
    return;
  case DELAY_DEFAULT_ARRIVAL_FALL:
    delay->default_arrival.fall = value;
    return;

  case DELAY_DEFAULT_REQUIRED_RISE:
    delay->default_required.rise = value;
    return;
  case DELAY_DEFAULT_REQUIRED_FALL:
    delay->default_required.fall = value;
    return;

  default:
    fail("bad param in delay_set_default_parameter");
  }
}

void delay_set_parameter(node, param, value) node_t *node;
delay_param_t param;
double value;
{
  delay_pin_t *pin_delay;

  delay_force_pininfo(node);
  pin_delay = DELAY(node)->pin_delay;

  switch (param) {
  case DELAY_BLOCK_RISE:
    pin_delay->block.rise = value;
    break;
  case DELAY_BLOCK_FALL:
    pin_delay->block.fall = value;
    break;
  case DELAY_DRIVE_RISE:
    pin_delay->drive.rise = value;
    break;
  case DELAY_DRIVE_FALL:
    pin_delay->drive.fall = value;
    break;
  case DELAY_OUTPUT_LOAD:
  case DELAY_INPUT_LOAD:
    pin_delay->load = value;
    break;
  case DELAY_MAX_INPUT_LOAD:
    pin_delay->max_load = value;
    break;
  case DELAY_PHASE:
    if (value == DELAY_PHASE_INVERTING) {
      pin_delay->phase = PHASE_INVERTING;
    } else if (value == DELAY_PHASE_NONINVERTING) {
      pin_delay->phase = PHASE_NONINVERTING;
    } else if (value == DELAY_PHASE_NEITHER) {
      pin_delay->phase = PHASE_NEITHER;
    } else {
      fail("bad phase in delay_set_parameter");
    }
    break;

  case DELAY_ARRIVAL_RISE:
  case DELAY_REQUIRED_RISE:
    pin_delay->user_time.rise = value;
    break;

  case DELAY_ARRIVAL_FALL:
  case DELAY_REQUIRED_FALL:
    pin_delay->user_time.fall = value;
    break;

  default:
    fail("bad enum value in delay_set_parameter");
    /* NOTREACHED */
  }
}

double delay_get_default_parameter(network, param) network_t *network;
delay_param_t param;
{
  delay_network_t *delay;

  delay = DEF_DELAY(network);
  if (delay == NIL(delay_network_t)) {
    delay_network_alloc(network);
    delay = DEF_DELAY(network);
  }
  switch (param) {
  case DELAY_DEFAULT_ARRIVAL_RISE:
    return delay->default_arrival.rise;
  case DELAY_DEFAULT_ARRIVAL_FALL:
    return delay->default_arrival.fall;
  case DELAY_DEFAULT_REQUIRED_RISE:
    return delay->default_required.rise;
  case DELAY_DEFAULT_REQUIRED_FALL:
    return delay->default_required.fall;
  case DELAY_DEFAULT_OUTPUT_LOAD:
    return delay->pipo_model.load;
  case DELAY_DEFAULT_MAX_INPUT_LOAD:
    return delay->pipo_model.max_load;
  case DELAY_DEFAULT_DRIVE_RISE:
    return delay->pipo_model.drive.rise;
  case DELAY_DEFAULT_DRIVE_FALL:
    return delay->pipo_model.drive.fall;
  case DELAY_WIRE_LOAD_SLOPE:
    return delay->wire_load_table.slope;
  default:
    fail("bad param in delay_get_global_parameter");
  }
  /*NOTREACHED*/
}

double delay_get_parameter(node, param) node_t *node;
delay_param_t param;
{
  pin_phase_t phase;
  delay_pin_t *pin_delay;

  delay_force_pininfo(node);
  pin_delay = DELAY(node)->pin_delay;

  switch (param) {
  case DELAY_BLOCK_RISE:
    return (pin_delay->block.rise);
  case DELAY_BLOCK_FALL:
    return (pin_delay->block.fall);
  case DELAY_DRIVE_RISE:
    return (pin_delay->drive.rise);
  case DELAY_DRIVE_FALL:
    return (pin_delay->drive.fall);
  case DELAY_OUTPUT_LOAD:
  case DELAY_INPUT_LOAD:
    return (pin_delay->load);
  case DELAY_MAX_INPUT_LOAD:
    return (pin_delay->max_load);
  case DELAY_PHASE:
    phase = pin_delay->phase;
    if (phase == PHASE_INVERTING) {
      return (DELAY_PHASE_INVERTING);
    } else if (phase == PHASE_NONINVERTING) {
      return (DELAY_PHASE_NONINVERTING);
    } else if (phase == PHASE_NEITHER) {
      return (DELAY_PHASE_NEITHER);
    } else {
      fail("bad phase value in delay_get_parameter");
      /* NOTREACHED */
    }

  case DELAY_ARRIVAL_RISE:
  case DELAY_REQUIRED_RISE:
    return (pin_delay->user_time.rise);

  case DELAY_ARRIVAL_FALL:
  case DELAY_REQUIRED_FALL:
    return (pin_delay->user_time.fall);

  default:
    fail("bad enum in delay_get_parameter");
    /* NOTREACHED */
  }
}

void delay_alloc(node) node_t *node;
{
  delay_node_t *delay;

  delay = ALLOC(delay_node_t, 1);
  node->DELAY_SLOT = (char *)delay;
  delay->pin_delay = NIL(delay_pin_t);
  delay->pin_params_valid = DELAY_MODEL_UNKNOWN;
  delay->num_pin_params = 0;
  delay->pin_params = NIL(delay_pin_t *);
#ifdef SIS
  delay->synch_clock_name = NIL(char);
#endif /* SIS */
}

void delay_free(node) node_t *node;
{
  int i;
  delay_node_t *delay;

  delay = DELAY(node);
  FREE(delay->pin_delay);
  if (delay->num_pin_params != 0 && delay->pin_params != NIL(delay_pin_t *)) {
    for (i = 0; i < delay->num_pin_params; i++) {
      /*
      if (delay->pin_params[i] != &unspecified_pipo_delay_model) {
      */
      FREE(delay->pin_params[i]);
      /*
      }
      */
    }
    FREE(delay->pin_params);
  }
#ifdef SIS
  FREE(delay->synch_clock_name);
#endif /* SIS */
  FREE(node->DELAY_SLOT);
}

void delay_dup(oldnode, newnode) node_t *oldnode, *newnode;
{
  int nparms, i;
  delay_node_t *old, *new;

  new = DELAY(newnode);
  old = DELAY(oldnode);
  *new = *old;

  if (old->pin_params_valid != DELAY_MODEL_UNKNOWN) {
    nparms = new->num_pin_params;
    new->pin_params = ALLOC(delay_pin_t *, nparms);
    for (i = 0; i < nparms; ++i) {
      new->pin_params[i] = ALLOC(delay_pin_t, 1);
      *new->pin_params[i] = *old->pin_params[i];
    }
  } else {
    new->pin_params = NIL(delay_pin_t *);
    new->num_pin_params = 0;
  }

  if (old->pin_delay != NIL(delay_pin_t)) {
    new->pin_delay = ALLOC(delay_pin_t, 1);
    *new->pin_delay = *old->pin_delay;
  }
#ifdef SIS
  if (old->synch_clock_name != NIL(char)) {
    new->synch_clock_name = util_strsav(old->synch_clock_name);
  }
#endif /* SIS */
}

/*
 * Need to invalidate the mapped parameters for this node
 */
void delay_invalid(node) node_t *node;
{
  /* BUG FIX: by KJ -- earlier this routine did nothing */
  DELAY(node)->pin_params_valid = DELAY_MODEL_UNKNOWN;
}

void delay_network_alloc(network) network_t *network;
{
  delay_network_t *delay;

  delay = ALLOC(delay_network_t, 1);
  network->DEF_DELAY_SLOT = (char *)delay;
  delay->default_arrival = time_not_given;
  delay->default_required = time_not_given;

  delay->wire_load_table.slope = DELAY_VALUE_NOT_GIVEN;
  delay->wire_load_table.num_pins_set = 0;
  delay->wire_load_table.pins = NIL(array_t);

  delay->pipo_model = unspecified_pipo_delay_model;
}

void delay_network_free(network) network_t *network;
{
  array_t *pins;

  if (DEF_DELAY(network) != NIL(delay_network_t)) {
    pins = DEF_DELAY(network)->wire_load_table.pins;
    if (pins != NIL(array_t)) {
      array_free(pins);
    }
    FREE(network->DEF_DELAY_SLOT);
  }
}

void delay_network_dup(new, old) network_t *new, *old;
{
  array_t *oldpins;

  delay_network_free(new);
  if (DEF_DELAY(old) == NIL(delay_network_t)) {
    new->DEF_DELAY_SLOT = NIL(char);
    return;
  }
  new->DEF_DELAY_SLOT = (char *)ALLOC(delay_network_t, 1);
  *DEF_DELAY(new) = *DEF_DELAY(old);

  oldpins = DEF_DELAY(old)->wire_load_table.pins;
  DEF_DELAY(new)->wire_load_table.pins =
      (oldpins != NIL(array_t)) ? array_dup(oldpins) : NIL(array_t);
}

/*
 *  Mild hack: take the pin_delay records from the primary inputs, and
 *  give these away to the library package; the library package will return
 *  these, as requested via lib_get_pin_delay(node, pin);
 *
 *  We don't care if pin_delay == 0 for some pin; but the library package
 *  probably does ...
 */
char **delay_giveaway_pin_delay(network) network_t *network;
{
  lsGen gen;
  node_t *node;
  int i;
  char **delay_info;
  delay_pin_t *pin_delay;

  delay_info = ALLOC(char *, network_num_pi(network));
  i = 0;
  foreach_primary_input(network, gen, node) {
    pin_delay = DELAY(node)->pin_delay;
    if (pin_delay == NIL(delay_pin_t)) {
      delay_info[i++] = NIL(char);
    } else {
      modelcpy(pin_delay, pin_delay, node);
      delay_info[i++] = (char *)pin_delay;
      DELAY(node)->pin_delay = NIL(delay_pin_t);
    }
  }
  return (delay_info);
}

/*
 * Routines to query the pin_delay_t structure.
 * Intended for use by the mapper
 */
double delay_get_load(pin_delay) char *pin_delay;
{ return (((delay_pin_t *)pin_delay)->load); }

delay_time_t delay_get_drive(pin_delay) char *pin_delay;
{ return (((delay_pin_t *)pin_delay)->drive); }

delay_time_t delay_get_block(pin_delay) char *pin_delay;
{ return (((delay_pin_t *)pin_delay)->block); }

double delay_get_load_limit(pin_delay) char *pin_delay;
{ return (((delay_pin_t *)pin_delay)->max_load); }

static network_t *map_node_to_network(node, mode) node_t *node;
double mode;
{
  network_t *net1, *net2;
  library_t *library;

  if ((library = lib_get_library()) == NIL(library_t)) {
    delay_error("Mapped Delay model: no current library\n");
    return (NIL(network_t)); /* Not reached -- in to keep lint happy */
  } else {
    net1 = network_create_from_node(node);
    net2 =
        map_network(net1, library, mode, 1, 0); /* user spec mode & inverters */
    network_free(net1);
    return net2;
  }
}

/*
 * Routines to query the primary outputs/inputs for user-specified data
 *
 * Could be done by delay_get_parameter calls -- packaged for convenience
 * Return 1 if value is set, 0 otherwisw
 */
int delay_get_po_load(node, load) node_t *node;
double *load;
{
  double l;
  if (node_type(node) != PRIMARY_OUTPUT) {
    return (0);
  }

  l = delay_get_parameter(node, DELAY_OUTPUT_LOAD);
  if (l != DELAY_VALUE_NOT_GIVEN) {
    *load = l;
    return (1);
  }
  return (0);
}

int delay_get_pi_drive(node, drive) node_t *node;
delay_time_t *drive;
{
  delay_time_t t;
  if (node_type(node) != PRIMARY_INPUT) {
    return (0);
  }
  t.rise = delay_get_parameter(node, DELAY_DRIVE_RISE);
  t.fall = delay_get_parameter(node, DELAY_DRIVE_FALL);
  if (t.rise != DELAY_VALUE_NOT_GIVEN && t.fall != DELAY_VALUE_NOT_GIVEN) {
    *drive = t;
    return (1);
  }
  return (0);
}

int delay_get_pi_load_limit(node, load_limit) node_t *node;
double *load_limit;
{
  double l;
  if (node_type(node) != PRIMARY_INPUT) {
    return (0);
  }
  l = delay_get_parameter(node, DELAY_MAX_INPUT_LOAD);
  if (l != DELAY_VALUE_NOT_GIVEN) {
    *load_limit = l;
    return (1);
  }
  return (0);
}

int delay_get_pi_arrival_time(node, arrival) node_t *node;
delay_time_t *arrival;
{
  delay_time_t t;
  if (node_type(node) != PRIMARY_INPUT) {
    return (0);
  }
  t.rise = delay_get_parameter(node, DELAY_ARRIVAL_RISE);
  t.fall = delay_get_parameter(node, DELAY_ARRIVAL_FALL);
  if (t.rise != DELAY_VALUE_NOT_GIVEN && t.fall != DELAY_VALUE_NOT_GIVEN) {
    *arrival = t;
    return (1);
  }
  return (0);
}

int delay_get_po_required_time(node, required) node_t *node;
delay_time_t *required;
{
  delay_time_t t;
  if (node_type(node) != PRIMARY_OUTPUT) {
    return (0);
  }
  t.rise = delay_get_parameter(node, DELAY_REQUIRED_RISE);
  t.fall = delay_get_parameter(node, DELAY_REQUIRED_FALL);
  if (t.rise != DELAY_VALUE_NOT_GIVEN && t.fall != DELAY_VALUE_NOT_GIVEN) {
    *required = t;
    return (1);
  }
  return (0);
}

/* Debugging code.  Not currently used. */

#if !defined(lint) && !defined(SABER)
static void print_pin_delays(node, model) node_t *node;
delay_model_t model;
{
  int i;
  int pin;
  double load;
  node_t *fanout;
  delay_pin_t *pin_delay;
  lsGen gen;

  printf("Delay calculation for node %s:\n", node_name(node));

  for (i = 0; i < node_num_fanin(node); ++i) {
    pin_delay = get_pin_delay(node, i, model);
    printf("pin %d br %f bf %f dr %f df %f phase %s load %f\n", i,
           pin_delay->block.rise, pin_delay->block.fall, pin_delay->drive.rise,
           pin_delay->drive.fall,
           (pin_delay->phase == PHASE_INVERTING
                ? "neg"
                : (pin_delay->phase == PHASE_NONINVERTING ? "pos" : "both")),
           pin_delay->load);
  }

  load = compute_wire_load(node_network(node), node_num_fanout(node));
  foreach_fanout_pin(node, gen, fanout, pin) {
    pin_delay = get_pin_delay(fanout, pin, model);
    load += pin_delay->load;
  }

  printf("load calculation for node %s: stored %f calculated %f\n",
         node_name(node), DELAY(node)->load, load);
}
#endif /* lint and saber */

#ifdef SIS
/*
 * Routine to get the synchronization data regarding PO/PI nodes
 * returns 0 if not synchronized, 1 if it is
 * "flag" returns whether the delay-data refers to occurance before or after
 * the clock pulse. The delay data can be obtained from the routines
 * delay_get_pi_arrival_time() and delay_get_po_required_time()
 */
int delay_get_synch_edge(node, edgep, flag) node_t *node;
clock_edge_t *edgep;
int *flag;
{
  char *name;
  sis_clock_t *clock;
  network_t *network;

  if (node_type(node) == INTERNAL) {
    return (0);
  }
  if ((name = DELAY(node)->synch_clock_name) == NIL(char)) {
    return (0);
  }
  if ((network = node_network(node)) == NIL(network_t)) {
    return (0);
  }
  if ((clock = clock_get_by_name(network, name)) == NIL(sis_clock_t)) {
    return (0);
  }
  edgep->clock = clock;
  edgep->transition = DELAY(node)->synch_clock_edge;
  *flag = DELAY(node)->synch_relative_flag;

  return (1);
}

/*
 * Routine to copy the synchronization edge information from the fanin
 * of a po_node to the po_node itself. Meant for use by the io package
 */

void delay_copy_terminal_constraints(po_node) node_t *po_node;
{
  char *name;
  node_t *node;
  double value;
  network_t *network;

  assert(node_type(po_node) == PRIMARY_OUTPUT);
  node = node_get_fanin(po_node, 0);

  /* Copy the loads and the required times at the outputs */
  value = delay_get_parameter(node, DELAY_OUTPUT_LOAD);
  delay_set_parameter(po_node, DELAY_OUTPUT_LOAD, value);
  value = delay_get_parameter(node, DELAY_REQUIRED_RISE);
  delay_set_parameter(po_node, DELAY_REQUIRED_RISE, value);
  value = delay_get_parameter(node, DELAY_REQUIRED_FALL);
  delay_set_parameter(po_node, DELAY_REQUIRED_FALL, value);

  if ((name = DELAY(node)->synch_clock_name) == NIL(char)) {
    return;
  }
  if ((network = node_network(node)) == NIL(network_t)) {
    return;
  }
  if (clock_get_by_name(network, name) == NIL(sis_clock_t)) {
    return;
  }

  /* Copy the synchronization data */
  DELAY(po_node)->synch_clock_name = util_strsav(name);
  DELAY(po_node)->synch_clock_edge = DELAY(node)->synch_clock_edge;
  DELAY(po_node)->synch_relative_flag = DELAY(node)->synch_relative_flag;
}

/*
 * Routine to set the synchronization data regarding PO/PI nodes
 * returns 0 if it cannot set the values ....
 * "flag" is one of BEFORE_CLOCK_EDGE or AFTER_CLOCK_EDGE and indicates
 * whether the delay-data refers to occurance before or after
 * the clock pulse. The delay data can be set by  the routine
 * delay_set_parameter() using the appropriate calls.
 */
int delay_set_synch_edge(node, edge, flag) node_t *node;
clock_edge_t edge;
int flag; /* BEFORE_CLOCK_EDGE or AFTER_CLOCK_EDGE */
{
  DELAY(node)->synch_clock_name = util_strsav(clock_name(edge.clock));
  DELAY(node)->synch_clock_edge = edge.transition;
  DELAY(node)->synch_relative_flag = flag;
  return (1);
}
#endif /* SIS */
