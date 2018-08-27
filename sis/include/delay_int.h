
#include "delay.h"

#define DELAY_SLOT delay
#define DELAY(node) ((delay_node_t *)node->DELAY_SLOT)

#define DEF_DELAY_SLOT default_delay
#define DEF_DELAY(network) ((delay_network_t *)network->DEF_DELAY_SLOT)

/*
 * Each node is annotated with a 'delay_node_t'; this stores the arrival,
 * required, and slack times for each node.  Also, for PRIMARY_INPUT and
 * PRIMARY_OUTPUT nodes, the pair (time, time_given) allows the specification
 * of input arrival and output required times, respectively.  Finally, a delay
 * model slot is reserved for PRIMARY_INPUT and PRIMARY_OUTPUT nodes; if
 * present, it stores the input-rise factor and output-load factors.
 */

typedef struct delay_node_struct {
  delay_time_t arrival;
  delay_time_t required;
  delay_time_t slack;
  delay_model_t pin_params_valid;
  int num_pin_params;
  delay_pin_t **pin_params;
  delay_pin_t *pin_delay;
  double load;
#ifdef SIS
  char *synch_clock_name;  /* clock_name w.r.t. arr & req times spec. */
  int synch_clock_edge;    /* RISE_TRANSITION or FALL_TRANSITION */
  int synch_relative_flag; /* BEFORE_CLOCK_EDGE or AFTER_CLOCK_EDGE */
#endif                     /* SIS */
} delay_node_t;

typedef struct delay_wire_load_struct {
  double slope;
  int num_pins_set;
  array_t *pins;
} delay_wire_load_table_t;

typedef struct delay_network_struct {
  delay_time_t default_arrival;
  delay_time_t default_required;
  delay_wire_load_table_t wire_load_table;
  delay_pin_t pipo_model;
} delay_network_t;

/* used in com_delay.c */
extern void delay_dup();

extern void delay_free();

extern void delay_alloc();

extern void delay_invalid();

/* Interface to the timing driven cofactoring (tdc) model */
extern void compute_tdc_parms();

extern void delay_print_wire_loads();
