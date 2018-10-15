/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/delay/delay.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:19 $
 *
 */
#ifndef DELAY_H
#define DELAY_H

typedef struct time_struct {
    double rise;
    double fall;
} delay_time_t;

typedef enum pin_phase_enum {
    PHASE_NOT_GIVEN, PHASE_INVERTING, PHASE_NONINVERTING, PHASE_NEITHER
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
    DELAY_MODEL_UNIT, DELAY_MODEL_LIBRARY, DELAY_MODEL_UNIT_FANOUT,
    DELAY_MODEL_MAPPED, DELAY_MODEL_UNKNOWN, DELAY_MODEL_TDC
} delay_model_t;

EXTERN int delay_trace ARGS((network_t *, delay_model_t));
EXTERN delay_model_t delay_get_model_from_name ARGS((char *));
EXTERN delay_pin_t *get_pin_delay ARGS((node_t *, int, delay_model_t));
EXTERN node_t *delay_latest_output ARGS((network_t *, double *));
EXTERN network_t *delay_generate_decomposition ARGS((node_t *, double));

EXTERN delay_time_t delay_node_pin ARGS((node_t *, int, delay_model_t));
EXTERN delay_time_t delay_fanout_contribution ARGS((node_t *, int, node_t *, delay_model_t));
EXTERN int delay_wire_required_time ARGS((node_t *, int, delay_model_t, delay_time_t *));
EXTERN delay_time_t delay_required_time ARGS((node_t *));
EXTERN delay_time_t delay_arrival_time ARGS((node_t *));
EXTERN delay_time_t delay_slack_time ARGS((node_t *));
EXTERN double delay_load ARGS((node_t *));
EXTERN double delay_compute_fo_load ARGS((node_t *, delay_model_t));
EXTERN double compute_wire_load ARGS((network_t *, int));

typedef enum delay_param_enum {
    DELAY_BLOCK_RISE, DELAY_BLOCK_FALL,
    DELAY_DRIVE_RISE, DELAY_DRIVE_FALL,
    DELAY_PHASE,
    DELAY_OUTPUT_LOAD,
    DELAY_INPUT_LOAD, DELAY_MAX_INPUT_LOAD,
    DELAY_ARRIVAL_RISE, DELAY_ARRIVAL_FALL,
    DELAY_REQUIRED_RISE, DELAY_REQUIRED_FALL,

    DELAY_ADD_WIRE_LOAD, DELAY_WIRE_LOAD_SLOPE,
    DELAY_DEFAULT_DRIVE_RISE, DELAY_DEFAULT_DRIVE_FALL, 
    DELAY_DEFAULT_OUTPUT_LOAD,
    DELAY_DEFAULT_ARRIVAL_RISE, DELAY_DEFAULT_ARRIVAL_FALL, 
    DELAY_DEFAULT_REQUIRED_RISE, DELAY_DEFAULT_REQUIRED_FALL,
    DELAY_DEFAULT_MAX_INPUT_LOAD
} delay_param_t;

#ifdef SIS
/* For modelling the synchronization of the primary inputs/outputs */
#define BEFORE_CLOCK_EDGE 0
#define AFTER_CLOCK_EDGE  1
#endif /* SIS */

#define DELAY_NOT_SET -1234567.0
#define DELAY_VALUE_NOT_GIVEN DELAY_NOT_SET
#define DELAY_NOT_SET_STRING "UNSET "
#define DELAY_PHASE_NOT_GIVEN -1

#define DELAY_PHASE_INVERTING 0.0
#define DELAY_PHASE_NONINVERTING 1.0
#define DELAY_PHASE_NEITHER 2.0

EXTERN void delay_set_tdc_params ARGS((char *));
EXTERN void delay_set_parameter ARGS((node_t *, delay_param_t, double));
EXTERN double delay_get_parameter ARGS((node_t *, delay_param_t));
EXTERN void delay_set_default_parameter ARGS((network_t *, delay_param_t, double));
EXTERN double delay_get_default_parameter ARGS((network_t *, delay_param_t));

/* Routines to interface with the mapper */
EXTERN delay_time_t delay_map_simulate();	/* special mapping routine */
EXTERN char **delay_giveaway_pin_delay ARGS((network_t *));	/* library read-in hack */
EXTERN double delay_get_load ARGS((char *));
EXTERN double delay_get_load_limit ARGS((char *));
EXTERN delay_time_t delay_get_drive ARGS((char *));
EXTERN delay_time_t delay_get_block ARGS((char *));

/* Routines to query user-specified data regarding the environment */
EXTERN int delay_get_pi_drive ARGS((node_t *, delay_time_t *));
EXTERN int delay_get_pi_load_limit ARGS((node_t *, double *));
EXTERN int delay_get_pi_arrival_time ARGS((node_t *, delay_time_t *));
EXTERN int delay_get_po_load ARGS((node_t *, double *));
EXTERN int delay_get_po_required_time ARGS((node_t *, delay_time_t *));
#ifdef SIS
EXTERN int delay_set_synch_edge ARGS((node_t *, clock_edge_t, int));
EXTERN int delay_get_synch_edge ARGS((node_t *, clock_edge_t *, int *));
#endif /* SIS */

/* io package */
EXTERN void delay_print_blif_wire_loads ARGS((network_t *, FILE *));
EXTERN void delay_print_slif_wire_loads ARGS((network_t *, FILE *));

#ifdef SIS
EXTERN void delay_copy_terminal_constraints ARGS((node_t *));
#endif /* SIS */

/* networks */
EXTERN void delay_network_alloc ARGS((network_t *));
EXTERN void delay_network_free ARGS((network_t *));
EXTERN void delay_network_dup ARGS((network_t *, network_t *));

/* For the speed package */
EXTERN network_t * tdc_factor_network ARGS((network_t *));

#endif
