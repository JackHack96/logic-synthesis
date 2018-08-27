
/* file @(#)fanout_delay.h	1.2 */
/* last modified on 5/1/91 at 15:50:30 */
#include "map_delay.h"
#include "pwl.h"

typedef delay_time_t (*DelayFn)();

typedef delay_pwl_t (*PwlFn)();

typedef struct n_gates_struct n_gates_t;
struct n_gates_struct {
  int n_pos_buffers; /* indices for pos_buffers are: [0, n_pos_buffers( */
  int n_neg_buffers; /* indices for neg_buffers are:
                        [n_pos_buffers,n_neg_buffers( */
  int n_buffers;     /* n_buffers always == to n_neg_buffers */
  int n_pos_sources; /* indices for pos_sources are: [n_buffers,n_pos_sources(
                      */
  int n_neg_sources; /* indices for neg_sources are:
                        [n_pos_sources,n_neg_sources( */
  int n_gates;       /* n_gates always == to n_neg_sources */
};

extern void fanout_delay_init(/* */);

extern void fanout_delay_end(/* */);

extern void fanout_delay_add_source(/* node_t *source, int source_polarity */);

extern void fanout_delay_init_sources(/* */);

extern void fanout_delay_free_sources(/* */);

extern double fanout_delay_get_buffer_load(/* int gate_index */);

extern delay_time_t fanout_delay_backward_intrinsic(
    /* delay_time_t required, int gate_index */);

extern delay_time_t fanout_delay_backward_load_dependent(
    /* delay_time_t required, int gate_index, double load */);

extern delay_time_t
    fanout_delay_forward_intrinsic(/* delay_time_t arrival, int gate_index */);

extern delay_time_t fanout_delay_forward_load_dependent(
    /* delay_time_t arrival, int gate_index, double load */);

extern int compute_best_number_of_inverters(
    /* int source_index, int buffer_index, double load, int max_int_nodes */);

extern n_gates_t fanout_delay_get_n_gates(/* */);

extern double fanout_delay_get_area(/* int gate_index */);

extern node_t *fanout_delay_get_source_node(/* int gate_index */);

extern lib_gate_t *fanout_delay_get_gate(/* int gate_index */);

extern int fanout_delay_get_source_polarity(/* int source_index */);

extern int fanout_delay_get_buffer_polarity(/* int buffer_index */);

extern int fanout_delay_get_buffer_index(/* lib_gate_t *buffer */);

extern int fanout_delay_get_source_index(/* node_t *node */);

extern void
    fanout_delay_add_pwl_source(/* node_t *source, int source_polarity */);

extern delay_pwl_t
    fanout_delay_get_delay_pwl(/* int gate_index, delay_time_t arrival */);

#define foreach_buffer(n_gates, gate)                                          \
  for ((gate) = 0; (gate) < (n_gates).n_buffers; (gate)++)
#define foreach_inverter(n_gates, gate)                                        \
  for ((gate) = (n_gates).n_pos_buffers; (gate) < (n_gates).n_buffers; (gate)++)
#define foreach_source(n_gates, gate)                                          \
  for ((gate) = (n_gates).n_buffers; (gate) < (n_gates).n_gates; (gate)++)
#define foreach_gate(n_gates, gate)                                            \
  for ((gate) = 0; (gate) < (n_gates).n_gates; (gate)++)

#define is_source(n_gates, gate)                                               \
  ((gate) >= (n_gates).n_buffers && (gate) < (n_gates).n_gates)
#define is_buffer(n_gates, gate) ((gate) >= 0 && (gate) < (n_gates).n_buffers)
#define is_inverter(n_gates, gate)                                             \
  ((gate) >= (n_gates).n_pos_buffers && (gate) < (n_gates).n_buffers)
