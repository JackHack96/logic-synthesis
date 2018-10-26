
/* file @(#)map_delay.c	1.6 */
/* last modified on 7/8/91 at 14:47:03 */
#include "sis.h"
#include <math.h>
#include "map_int.h"
#include "lib_int.h"
#include "map_macros.h"
#include "map_delay.h"
#include "fanout_delay.h"

static double compute_fanout_load_correction();
static void map_set_current_network();
static void precompute_fanout_load_correction();

#define ARRIVAL(time, new_time)	if (new_time > time) time = new_time;

void delay_map_compute_required_times(nin, model, node_required, load, required)
int nin;
char **model;
delay_time_t node_required;
double load;
delay_time_t *required;
{
  register int i;
  register delay_pin_t *pin_delay, **lib_delay_model;
  delay_time_t delay;

  lib_delay_model = (delay_pin_t **) model;
  for(i = nin-1; i >= 0; i--) {
    pin_delay = lib_delay_model[i];
    delay.rise = pin_delay->block.rise + pin_delay->drive.rise * load;
    delay.fall = pin_delay->block.fall + pin_delay->drive.fall * load;

    switch(pin_delay->phase) {
    case PHASE_INVERTING:
      required[i].rise = node_required.fall - delay.fall;
      required[i].fall = node_required.rise - delay.rise; 
      break;
    case PHASE_NONINVERTING:
      required[i].rise = node_required.rise - delay.rise;
      required[i].fall = node_required.fall - delay.fall;
      break;
    case PHASE_NEITHER:
      required[i].rise = MIN(node_required.rise - delay.rise, node_required.fall - delay.fall);
      required[i].fall = required[i].rise;
      break;
    default:
      ;
    }
  }
}

 /* TREATMENT OF PRIMARY INPUTS / OUTPUTS */

#define MAX_PRECOMPUTED_FANOUT_LOADS 20

typedef struct {
  lib_gate_t *default_inv;
  delay_time_t arrival;
  delay_time_t drive;
  double load;
  double load_limit;
  delay_time_t required;
  int required_not_set;
  double fanout_loads[MAX_PRECOMPUTED_FANOUT_LOADS];
} pipo_t;

static pipo_t DEFAULT_PIPO_SETTINGS;
static pipo_t pipo_settings;
static network_t *map_current_network;
static bin_global_t global;

static void map_set_current_network(network)
network_t *network;
{
  assert(network != NIL(network_t));
  map_current_network = network;
}

 /* if ignore_pipo_data is set, remove all delay info from the network except wire load */

void map_alloc_delay_info(network, options)
network_t *network;
bin_global_t *options;
{
  lsGen gen;
  node_t *pi, *po;

  global = *options;
  map_set_current_network(network);
  pipo_settings = DEFAULT_PIPO_SETTINGS;
  pipo_settings.default_inv = lib_get_default_inverter();
  pipo_settings.drive = delay_get_drive(pipo_settings.default_inv->delay_info[0]);
  pipo_settings.load = delay_get_load(pipo_settings.default_inv->delay_info[0]);
  pipo_settings.load_limit = delay_get_load_limit(pipo_settings.default_inv->delay_info[0]);
/* which one is better?
  pipo_settings.load_limit = INFINITY;
*/
  if (global.ignore_pipo_data) {
    delay_set_default_parameter(network, DELAY_DEFAULT_DRIVE_RISE, DELAY_NOT_SET);
    delay_set_default_parameter(network, DELAY_DEFAULT_DRIVE_FALL, DELAY_NOT_SET);
    delay_set_default_parameter(network, DELAY_DEFAULT_ARRIVAL_RISE, DELAY_NOT_SET);
    delay_set_default_parameter(network, DELAY_DEFAULT_ARRIVAL_FALL, DELAY_NOT_SET);
    delay_set_default_parameter(network, DELAY_DEFAULT_REQUIRED_RISE, DELAY_NOT_SET);
    delay_set_default_parameter(network, DELAY_DEFAULT_REQUIRED_FALL, DELAY_NOT_SET);
    delay_set_default_parameter(network, DELAY_DEFAULT_OUTPUT_LOAD, DELAY_NOT_SET);
    delay_set_default_parameter(network, DELAY_ADD_WIRE_LOAD, DELAY_NOT_SET);
    delay_set_default_parameter(network, DELAY_WIRE_LOAD_SLOPE, DELAY_NOT_SET);
    foreach_primary_input(network, gen, pi) {
      delay_set_parameter(pi, DELAY_DRIVE_RISE, DELAY_NOT_SET);
      delay_set_parameter(pi, DELAY_DRIVE_FALL, DELAY_NOT_SET);
      delay_set_parameter(pi, DELAY_ARRIVAL_RISE, DELAY_NOT_SET);
      delay_set_parameter(pi, DELAY_ARRIVAL_FALL, DELAY_NOT_SET);
      delay_set_parameter(pi, DELAY_MAX_INPUT_LOAD, DELAY_NOT_SET);
    }
    foreach_primary_output(network, gen, po) {
      delay_set_parameter(po, DELAY_REQUIRED_RISE, DELAY_NOT_SET);
      delay_set_parameter(po, DELAY_REQUIRED_FALL, DELAY_NOT_SET);
      delay_set_parameter(po, DELAY_OUTPUT_LOAD, DELAY_NOT_SET);
    }
  }

  if (delay_get_default_parameter(network, DELAY_DEFAULT_DRIVE_RISE) == DELAY_NOT_SET) {
    if (! global.no_warning) {
      (void) fprintf(siserr, "WARNING: uses as primary %s the value ", "input drive");
      (void) fprintf(siserr, "(%2.2f,%2.2f)\n", pipo_settings.drive.rise, pipo_settings.drive.fall);
    }
    delay_set_default_parameter(network, DELAY_DEFAULT_DRIVE_RISE, pipo_settings.drive.rise); 
    delay_set_default_parameter(network, DELAY_DEFAULT_DRIVE_FALL, pipo_settings.drive.fall); 
  } else {
    pipo_settings.drive.rise = delay_get_default_parameter(network, DELAY_DEFAULT_DRIVE_RISE);
    pipo_settings.drive.fall = delay_get_default_parameter(network, DELAY_DEFAULT_DRIVE_FALL);
  }
  if (delay_get_default_parameter(network, DELAY_DEFAULT_ARRIVAL_RISE) == DELAY_NOT_SET) {
    if (! global.no_warning) {
      (void) fprintf(siserr, "WARNING: uses as primary %s the value ", "input arrival");
      (void) fprintf(siserr, "(%2.2f,%2.2f)\n", pipo_settings.arrival.rise, pipo_settings.arrival.fall);
    }
    delay_set_default_parameter(network, DELAY_DEFAULT_ARRIVAL_RISE, pipo_settings.arrival.rise); 
    delay_set_default_parameter(network, DELAY_DEFAULT_ARRIVAL_FALL, pipo_settings.arrival.fall); 
  } else {
    pipo_settings.arrival.rise = delay_get_default_parameter(network, DELAY_DEFAULT_ARRIVAL_RISE);
    pipo_settings.arrival.fall = delay_get_default_parameter(network, DELAY_DEFAULT_ARRIVAL_FALL);
  }
  if (delay_get_default_parameter(network, DELAY_DEFAULT_MAX_INPUT_LOAD) == DELAY_NOT_SET) {
    if (! global.no_warning) {
      (void) fprintf(siserr, "WARNING: uses as primary %s the value ", "input max load limit");
      (void) fprintf(siserr, "(%2.2f)\n", pipo_settings.load_limit);
    }
    delay_set_default_parameter(network, DELAY_DEFAULT_MAX_INPUT_LOAD, pipo_settings.load_limit);
  } else {
    pipo_settings.load_limit = delay_get_default_parameter(network, DELAY_DEFAULT_MAX_INPUT_LOAD);
  }
  if (delay_get_default_parameter(network, DELAY_DEFAULT_REQUIRED_RISE) == DELAY_NOT_SET) {
    pipo_settings.required_not_set = 1;
    if (! global.no_warning) {
      (void) fprintf(siserr, "WARNING: uses as primary %s the value ", "output required");
      (void) fprintf(siserr, "(%2.2f,%2.2f)\n", pipo_settings.required.rise, pipo_settings.required.fall);
    }
    delay_set_default_parameter(network, DELAY_DEFAULT_REQUIRED_RISE, pipo_settings.required.rise); 
    delay_set_default_parameter(network, DELAY_DEFAULT_REQUIRED_FALL, pipo_settings.required.fall); 
  } else {
    pipo_settings.required.rise = delay_get_default_parameter(network, DELAY_DEFAULT_REQUIRED_RISE);
    pipo_settings.required.fall = delay_get_default_parameter(network, DELAY_DEFAULT_REQUIRED_FALL);
  }
  if (delay_get_default_parameter(network, DELAY_DEFAULT_OUTPUT_LOAD) == DELAY_NOT_SET) {
    if (! global.no_warning) {
      (void) fprintf(siserr, "WARNING: uses as primary %s the value ", "output load");
      (void) fprintf(siserr, "%2.2f\n", pipo_settings.load);
    }
    delay_set_default_parameter(network, DELAY_DEFAULT_OUTPUT_LOAD, pipo_settings.load);
  } else {
    pipo_settings.load = delay_get_default_parameter(network, DELAY_DEFAULT_OUTPUT_LOAD);
  }
  fanout_delay_init(options);
  precompute_fanout_load_correction();
}

void map_free_delay_info()
{
  fanout_delay_end();
}

void pipo_set_default_po_required(default_value)
delay_time_t default_value;
{
  if (pipo_settings.required_not_set) {
    pipo_settings.required = default_value;
    if (! global.no_warning) {
      (void) fprintf(siserr, "WARNING: uses as primary %s the value ", "output required");
      (void) fprintf(siserr, "(%2.2f,%2.2f)\n", pipo_settings.required.rise, pipo_settings.required.fall);
    }
    delay_set_default_parameter(map_current_network, DELAY_DEFAULT_REQUIRED_RISE, pipo_settings.required.rise); 
    delay_set_default_parameter(map_current_network, DELAY_DEFAULT_REQUIRED_FALL, pipo_settings.required.fall); 
  }
}

delay_time_t pipo_get_pi_arrival(node)
node_t *node;
{
  delay_time_t arrival;
  if (delay_get_pi_arrival_time(node, &arrival)) return arrival;
  return pipo_settings.arrival;
}

delay_time_t pipo_get_pi_drive(node)
node_t *node;
{
  delay_time_t drive;
  if (delay_get_pi_drive(node, &drive)) return drive;
  return pipo_settings.drive;
}

double pipo_get_pi_load_limit(node)
node_t *node;
{
  double load_limit;
  if (delay_get_pi_load_limit(node, &load_limit)) return load_limit;
  return pipo_settings.load_limit;
}

delay_time_t pipo_get_po_required(node)
node_t *node;
{
  delay_time_t required;
  if (delay_get_po_required_time(node, &required)) return required;
  return pipo_settings.required;
}

double pipo_get_po_load(node)
node_t *node;
{
  double load;
  if (delay_get_po_load(node, &load)) return load;
  return pipo_settings.load;
}

 /* should be in delay/delay.c really */

pin_phase_t delay_get_polarity(pin_delay)
char *pin_delay;
{
  return ((delay_pin_t *) pin_delay)->phase;
}

double map_compute_wire_load(n_fanouts)
int n_fanouts;
{
  return compute_wire_load(map_current_network, n_fanouts);
}


 /* used in bin_delay.c as a heuristic to estimate the load */
 /* at a multiple fanout point */

double map_compute_fanout_load_correction(n_fanouts)
int n_fanouts;
{
  assert(n_fanouts > 0);
  if (n_fanouts < MAX_PRECOMPUTED_FANOUT_LOADS) return pipo_settings.fanout_loads[n_fanouts];
  return compute_fanout_load_correction(n_fanouts);
}

static void precompute_fanout_load_correction()
{
  int i;

  for (i = 1; i < MAX_PRECOMPUTED_FANOUT_LOADS; i++) {
    pipo_settings.fanout_loads[i] = compute_fanout_load_correction(i);
  }
}

 /* ignore, for the moment, wire loads. Because the entire tree mapper ignore them */

static double compute_fanout_load_correction(n_fanouts)
int n_fanouts;
{
  int buffer_index;
  double buffer_load;
  double total_load;
  int n_buffers;
  double buffered_load;

  buffer_index = fanout_delay_get_buffer_index(pipo_settings.default_inv);
  buffer_load = fanout_delay_get_buffer_load(buffer_index);
  switch (global.load_estimation) {
  case 0:
    total_load = 0.0;
    break;
  case 1:
    total_load = n_fanouts * buffer_load;
    break;
  case 2:
    total_load = n_fanouts * buffer_load;
    n_buffers = compute_best_number_of_inverters(buffer_index, buffer_index, total_load, n_fanouts);
    buffered_load = n_buffers * buffer_load;
    total_load = MIN(total_load, buffered_load);
    break;
  default:
    (void) fprintf(siserr, "unrecognized value of global.load_estimation: %d\n", global.load_estimation);
    fail("");
    break;
  }
  return total_load;
}
