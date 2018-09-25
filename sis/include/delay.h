
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

extern int delay_trace(network_t *, delay_model_t);

extern delay_model_t delay_get_model_from_name(char *);

extern delay_pin_t *get_pin_delay(node_t *, int, delay_model_t);

extern node_t *delay_latest_output(network_t *, double *);

extern network_t *delay_generate_decomposition(node_t *, double);

extern delay_time_t delay_node_pin(node_t *, int, delay_model_t);

extern delay_time_t delay_fanout_contribution(node_t *, int, node_t *,
                                              delay_model_t);

extern int delay_wire_required_time(node_t *, int, delay_model_t,
                                    delay_time_t *);

extern delay_time_t delay_required_time(node_t *);

extern delay_time_t delay_arrival_time(node_t *);

extern delay_time_t delay_slack_time(node_t *);

extern double delay_load(node_t *);

extern double delay_compute_fo_load(node_t *, delay_model_t);

extern double compute_wire_load(network_t *, int);

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

extern void delay_set_tdc_params(char *);

extern void delay_set_parameter(node_t *, delay_param_t, double);

extern double delay_get_parameter(node_t *, delay_param_t);

extern void delay_set_default_parameter(network_t *, delay_param_t, double);

extern double delay_get_default_parameter(network_t *, delay_param_t);

/* Routines to interface with the mapper */
extern delay_time_t delay_map_simulate(); /* special mapping routine */
extern char **delay_giveaway_pin_delay(network_t *); /* library read-in hack */
extern double delay_get_load(char *);

extern double delay_get_load_limit(char *);

extern delay_time_t delay_get_drive(char *);

extern delay_time_t delay_get_block(char *);

/* Routines to query user-specified data regarding the environment */
extern int delay_get_pi_drive(node_t *, delay_time_t *);

extern int delay_get_pi_load_limit(node_t *, double *);

extern int delay_get_pi_arrival_time(node_t *, delay_time_t *);

extern int delay_get_po_load(node_t *, double *);

extern int delay_get_po_required_time(node_t *, delay_time_t *);

#ifdef SIS
extern int delay_set_synch_edge(node_t *, clock_edge_t, int);
extern int delay_get_synch_edge(node_t *, clock_edge_t *, int *);
#endif /* SIS */

/* io package */
extern void delay_print_blif_wire_loads(network_t *, FILE *);

extern void delay_print_slif_wire_loads(network_t *, FILE *);

#ifdef SIS
extern void delay_copy_terminal_constraints(node_t *);
#endif /* SIS */

/* networks */
extern void delay_network_alloc(network_t *);

extern void delay_network_free(network_t *);

extern void delay_network_dup(network_t *, network_t *);

/* For the speed package */
extern network_t *tdc_factor_network(network_t *);

int init_delay();

int end_delay();

#endif
