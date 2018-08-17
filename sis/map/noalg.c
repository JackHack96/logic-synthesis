
/* file @(#)noalg.c	1.2 */
/* last modified on 5/1/91 at 15:51:43 */
#include "sis.h"
#include "../include/fanout_int.h"
#include "../include/fanout_delay.h"

static int noalg_optimize();

static multidim_t *optimize_one_source();

static void build_selected_tree();

static void compute_best_one_source_one_polarity();

static void extract_merge_info();

static n_gates_t n_gates;

typedef struct noalg_struct noalg_t;
struct noalg_struct {
    delay_time_t required;
    double       area;
};

static noalg_t NOALG_INIT_VALUE = {{-INFINITY, -INFINITY}, 0.0};


/* CONTENTS */

/* this file contains the routines to compute fanout trees */
/* composed of at most one inverter */

/* ARGSUSED */
void noalg_init(network, alg)
        network_t *network;
        fanout_alg_t *alg;
{
    alg->optimize = noalg_optimize;
}

/* INTERNAL INTERFACE */

static int noalg_optimize(fanout_info, tree, cost)
        opt_array_t *fanout_info;         /* fanout_info[POLAR_X] lists the positive polarity sinks; POLAR_Y negative */
        array_t *tree;                 /* array in which the result tree is stored: format of fanout_tree.c */
        fanout_cost_t *cost;             /* contains information on the result: required times, area */
{
    generic_fanout_optimizer(fanout_info, tree, cost, optimize_one_source, extract_merge_info, build_selected_tree);
    return 1;
}

/* computes the best solution for each (source,sink_polarity) separately */

static multidim_t *optimize_one_source(fanout_info)
        opt_array_t *fanout_info;
{
    int        source_index, source_polarity;
    noalg_t    *entry;
    int        indices[2];
    multidim_t *table;

    n_gates = fanout_delay_get_n_gates();
    indices[0] = n_gates.n_gates;
    indices[1] = POLAR_MAX;
    table = multidim_alloc(noalg_t, 2, indices);
    multidim_init(noalg_t, table, NOALG_INIT_VALUE);

    foreach_gate(n_gates, source_index) {
        foreach_polarity(source_polarity) {
            if (is_source(n_gates, source_index) &&
                fanout_delay_get_source_polarity(source_index) != source_polarity)
                continue;
            entry = INDEX2P(noalg_t, table, source_index, source_polarity);
            compute_best_one_source_one_polarity(&fanout_info[source_polarity], entry, source_index);
        }
    }
    return table;
}

static void compute_best_one_source_one_polarity(fanout_info, entry, source_index)
        opt_array_t *fanout_info;
        noalg_t *entry;
        int source_index;
{
    delay_time_t required;
    double       load;

    if (fanout_info->n_elts == 0) {
        entry->required = PLUS_INFINITY;
        entry->area     = 0.0;
        return;
    }
    required = fanout_info->min_required[0];
    load     = fanout_info->total_load + map_compute_wire_load(fanout_info->n_elts);
    entry->required = fanout_delay_backward_load_dependent(required, source_index, load);
    entry->area     = fanout_delay_get_area(source_index);
}


/* extract info to interface with fanout_select_best_source */
/* merge_table should be such that its entries are required times */
/* at the output of a outside source */

static void extract_merge_info(fanout_info, table, merge_table)
        opt_array_t *fanout_info;
        multidim_t *table;
        multidim_t *merge_table;
{
    int             source_index;
    noalg_t         *from;
    single_source_t *to;
    int             p;

    foreach_buffer(n_gates, source_index) {
        foreach_polarity(p) {
            from = INDEX2P(noalg_t, table, source_index, p);
            to   = INDEX3P(single_source_t, merge_table, source_index, p, p);
            if (fanout_info[p].n_elts > 0) {
                to->required  = fanout_delay_backward_intrinsic(from->required, source_index);
                to->load      = fanout_delay_get_buffer_load(source_index);
                to->n_fanouts = 1;
                to->area      = from->area;
            } else {
                to->required = PLUS_INFINITY;
            }
        }
    }

    foreach_source(n_gates, source_index) {
        p  = fanout_delay_get_source_polarity(source_index);
        to = INDEX3P(single_source_t, merge_table, source_index, p, p);
        if (fanout_info[p].n_elts > 0) {
            to->required  = fanout_info[p].min_required[0];
            to->load      = fanout_info[p].total_load;
            to->n_fanouts = fanout_info[p].n_elts;
        } else {
            to->required = PLUS_INFINITY;
        }
    }
}


/* ARGSUSED */

static void build_selected_tree(fanout_info, tree, source, table)
        opt_array_t *fanout_info;
        array_t *tree;
        selected_source_t *source;
        multidim_t *table;
{
    int p, q;

    p = source->main_source_sink_polarity;
    q = POLAR_INV(p);
    if (fanout_info[q].n_elts > 0) {
        if (is_source(n_gates, source->buffer)) {
            fail("can't service two polarities from same output");
        } else {
            fanout_tree_insert_gate(tree, source->main_source, fanout_info[p].n_elts + 1);
            noalg_insert_sinks(tree, &fanout_info[p], 0, fanout_info[p].n_elts);
            fanout_tree_insert_gate(tree, source->buffer, fanout_info[q].n_elts);
            noalg_insert_sinks(tree, &fanout_info[q], 0, fanout_info[q].n_elts);
        }
    } else {
        fanout_tree_insert_gate(tree, source->main_source, fanout_info[p].n_elts);
        noalg_insert_sinks(tree, &fanout_info[p], 0, fanout_info[p].n_elts);
    }
}

void noalg_insert_sinks(tree, fanout_info, from, to)
        array_t *tree;
        opt_array_t *fanout_info;
        int from;
        int to;
{
    int sink;

    for (sink = from; sink < to; sink++) {
        gate_link_t *leaf = array_fetch(gate_link_t * , fanout_info->required, sink);
        fanout_tree_insert_sink(tree, leaf);
    }
}
