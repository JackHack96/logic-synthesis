
#ifndef DELAY_H
#define DELAY_H

#include "network.h"
#include "node.h"
#include "clock.h"
#include <stdio.h>

typedef struct time_struct {
  double rise;
  double fall;
} delay_time_t;

typedef enum pin_phase_enum {
  PHASE_NOT_GIVEN,
  PHASE_INVERTING,
  PHASE_NONINVERTING,
  PHASE_NEITHER
} pin_phase_t;

/*
 * The delay_pin_t structure .....
 * Specifies the delay model between a pair of pins
 */
typedef struct delay_pin_struct {
  delay_time_t block;
  delay_time_t drive;
  pin_phase_t phase;
  double load;
  double max_load;
  delay_time_t user_time;
} delay_pin_t;

typedef enum delay_model_enum {
  DELAY_MODEL_UNIT,
  DELAY_MODEL_LIBRARY,
  DELAY_MODEL_UNIT_FANOUT,
  DELAY_MODEL_MAPPED,
  DELAY_MODEL_UNKNOWN,
  DELAY_MODEL_TDC
} delay_model_t;

int delay_trace(network_t *, delay_model_t);

delay_model_t delay_get_model_from_name(char *);

delay_pin_t *get_pin_delay(node_t *, int, delay_model_t);

node_t *delay_latest_output(network_t *, double *);

network_t *delay_generate_decomposition(node_t *, double);

delay_time_t delay_node_pin(node_t *, int, delay_model_t);

delay_time_t delay_fanout_contribution(node_t *, int, node_t *,
                                              delay_model_t);

int delay_wire_required_time(node_t *, int, delay_model_t,
                                    delay_time_t *);

delay_time_t delay_required_time(node_t *);

delay_time_t delay_arrival_time(node_t *);

delay_time_t delay_slack_time(node_t *);

double delay_load(node_t *);

double delay_compute_fo_load(node_t *, delay_model_t);

double compute_wire_load(network_t *, int);

typedef enum delay_param_enum {
  DELAY_BLOCK_RISE,
  DELAY_BLOCK_FALL,
  DELAY_DRIVE_RISE,
  DELAY_DRIVE_FALL,
  DELAY_PHASE,
  DELAY_OUTPUT_LOAD,
  DELAY_INPUT_LOAD,
  DELAY_MAX_INPUT_LOAD,
  DELAY_ARRIVAL_RISE,
  DELAY_ARRIVAL_FALL,
  DELAY_REQUIRED_RISE,
  DELAY_REQUIRED_FALL,

  DELAY_ADD_WIRE_LOAD,
  DELAY_WIRE_LOAD_SLOPE,
  DELAY_DEFAULT_DRIVE_RISE,
  DELAY_DEFAULT_DRIVE_FALL,
  DELAY_DEFAULT_OUTPUT_LOAD,
  DELAY_DEFAULT_ARRIVAL_RISE,
  DELAY_DEFAULT_ARRIVAL_FALL,
  DELAY_DEFAULT_REQUIRED_RISE,
  DELAY_DEFAULT_REQUIRED_FALL,
  DELAY_DEFAULT_MAX_INPUT_LOAD
} delay_param_t;

#ifdef SIS
/* For modelling the synchronization of the primary inputs/outputs */
#define BEFORE_CLOCK_EDGE 0
#define AFTER_CLOCK_EDGE 1
#endif /* SIS */

#define DELAY_NOT_SET -1234567.0
#define DELAY_VALUE_NOT_GIVEN DELAY_NOT_SET
#define DELAY_NOT_SET_STRING "UNSET "
#define DELAY_PHASE_NOT_GIVEN -1

#define DELAY_PHASE_INVERTING 0.0
#define DELAY_PHASE_NONINVERTING 1.0
#define DELAY_PHASE_NEITHER 2.0

void delay_set_tdc_params(char *);

void delay_set_parameter(node_t *, delay_param_t, double);

double delay_get_parameter(node_t *, delay_param_t);

void delay_set_default_parameter(network_t *, delay_param_t, double);

double delay_get_default_parameter(network_t *, delay_param_t);

/* Routines to interface with the mapper */
delay_time_t delay_map_simulate(); /* special mapping routine */
char **delay_giveaway_pin_delay(network_t *); /* library read-in hack */
double delay_get_load(char *);

double delay_get_load_limit(char *);

delay_time_t delay_get_drive(char *);

delay_time_t delay_get_block(char *);

/* Routines to query user-specified data regarding the environment */
int delay_get_pi_drive(node_t *, delay_time_t *);

int delay_get_pi_load_limit(node_t *, double *);

int delay_get_pi_arrival_time(node_t *, delay_time_t *);

int delay_get_po_load(node_t *, double *);

int delay_get_po_required_time(node_t *, delay_time_t *);

#ifdef SIS
int delay_set_synch_edge(node_t *, clock_edge_t, int);
int delay_get_synch_edge(node_t *, clock_edge_t *, int *);
#endif /* SIS */

/* io package */
void delay_print_blif_wire_loads(network_t *, FILE *);

void delay_print_slif_wire_loads(network_t *, FILE *);

#ifdef SIS
void delay_copy_terminal_constraints(node_t *);
#endif /* SIS */

/* networks */
void delay_network_alloc(network_t *);

void delay_network_free(network_t *);

void delay_network_dup(network_t *, network_t *);

/* For the speed package */
network_t *tdc_factor_network(network_t *);

int init_delay();

int end_delay();

#endif
