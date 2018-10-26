
/* file @(#)fanout_util.c	1.2 */
/* last modified on 5/1/91 at 15:51:01 */
#include "fanout_delay.h"
#include "fanout_int.h"
#include "sis.h"

static void compute_merge_cost();

static void get_best_one_source();

static void select_best_source();

static n_gates_t n_gates;

/* SOME USEFUL CONSTANTS */

delay_time_t MINUS_INFINITY = {-INFINITY, -INFINITY};
delay_time_t PLUS_INFINITY  = {INFINITY, INFINITY};
delay_time_t ZERO_DELAY     = {0.0, 0.0};

single_source_t SINGLE_SOURCE_INIT_VALUE = {
        {-INFINITY, -INFINITY}, 0.0, 0, 0.0};

/* UTILITIES */

int find_minimum_index(delay_time_t *array, int n) {
    int          i;
    int          result;
    delay_time_t best_so_far;

    best_so_far = MINUS_INFINITY;
    for (i      = 0; i < n; i++) {
        if (GETMIN(best_so_far) < GETMIN(array[i])) {
            best_so_far = array[i];
            result      = i;
        }
    }
    return result;
}

void
generic_fanout_optimizer(opt_array_t *fanout_info, array_t *tree, fanout_cost_t *cost, MultidimFn optimize_one_source,
                         VoidFn extract_merge_info, VoidFn build_tree) {
    multidim_t        *table;
    multidim_t        *merge_table;
    selected_source_t source;
    int               indices[3];

    n_gates = fanout_delay_get_n_gates();
    table   = (*optimize_one_source)(fanout_info);
    indices[0] = n_gates.n_gates;
    indices[1] = POLAR_MAX;
    indices[2] = POLAR_MAX;
    merge_table = multidim_alloc(single_source_t, 3, indices);
    multidim_init(single_source_t, merge_table, SINGLE_SOURCE_INIT_VALUE);
    (*extract_merge_info)(fanout_info, table, merge_table);
    select_best_source(merge_table, &source, cost);
    (*build_tree)(fanout_info, tree, &source, table);
    multidim_free(merge_table);
    multidim_free(table);
}

/* INTERNAL INTERFACE */

/* FORMAT OF THE MERGE_TABLE */
/* (n_gates.n_gates * POLAR_MAX * POLAR_MAX) -> (source_index, source_polarity,
 * sink_polarity) */
/* 2 cases: */
/* (1) source_index is a source: in that case, source_polarity = polarity of the
 * source */
/* and required time and load returned by the entry correspond to the data at
 * the output of the source */
/* (2) source_index is a buffer: in that case, source_polarity is arbitrary */
/* and required time and load returned correspond to the data at input of the
 * buffer */
/* this allows for an easy merge of different solutions */

/* FORMAT OF SOURCE */
/* first entry: real source (not a buffer) */
/* second entry: polarity of sinks directly connected to that source */
/* third entry: a buffer (or inverter) that feeds the sinks of opposite polarity
 */

/* FORMAT OF COST */
/* slack at the source and total area of solution */

static void select_best_source(multidim_t *merge_table, selected_source_t *best_source, fanout_cost_t *best_cost) {
    int               source_index;
    selected_source_t source;
    fanout_cost_t     cost;
    n_gates = fanout_delay_get_n_gates();

    best_cost->slack = MINUS_INFINITY;
    best_cost->area  = INFINITY;
    foreach_source(n_gates, source_index) {
        get_best_one_source(merge_table, source_index, &source, &cost);
        if (GETMIN(best_cost->slack) < GETMIN(cost.slack)) {
            *best_cost   = cost;
            *best_source = source;
        }
    }
    assert(GETMIN(best_cost->slack) > -INFINITY);
}

static void get_best_one_source(multidim_t *merge_table, int source_index, selected_source_t *best_source,
                                fanout_cost_t *best_cost) {
    int               sink_polarity;
    int               source_polarity, buffer_polarity;
    int               buffer_index;
    selected_source_t source;
    fanout_cost_t     cost;
    single_source_t   *source_entry, *buffer_entry;

    best_cost->slack   = MINUS_INFINITY;
    best_cost->area    = INFINITY;
    source.main_source = source_index;
    source_polarity = fanout_delay_get_source_polarity(source_index);
    foreach_polarity(sink_polarity) {
        source.main_source_sink_polarity = sink_polarity;
        source_entry = INDEX3P(single_source_t, merge_table, source_index,
                               source_polarity, sink_polarity);
        foreach_gate(n_gates, buffer_index) {
            if (is_source(n_gates, buffer_index)) {
                if (buffer_index != source_index)
                    continue;
                buffer_polarity = source_polarity;
            } else {
                buffer_polarity = (is_inverter(n_gates, buffer_index))
                                  ? POLAR_INV(source_polarity)
                                  : source_polarity;
            }
            source.buffer = buffer_index;
            buffer_entry = INDEX3P(single_source_t, merge_table, buffer_index,
                                   buffer_polarity, POLAR_INV(sink_polarity));
            compute_merge_cost(source_index, source_entry, buffer_entry, &cost);
            if (GETMIN(best_cost->slack) < GETMIN(cost.slack)) {
                *best_cost   = cost;
                *best_source = source;
            }
        }
    }
}

static void
compute_merge_cost(int source_index, single_source_t *entry_x, single_source_t *entry_y, fanout_cost_t *cost) {
    double       load;
    delay_time_t required;

    SETMIN(required, entry_x->required, entry_y->required);
    load = entry_x->load + entry_y->load;
    load += map_compute_wire_load(entry_x->n_fanouts + entry_y->n_fanouts);
    cost->slack =
            fanout_delay_backward_load_dependent(required, source_index, load);
    cost->area = entry_x->area + entry_y->area;
}
