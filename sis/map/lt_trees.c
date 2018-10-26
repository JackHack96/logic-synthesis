
/* file @(#)lt_trees.c	1.2 */
/* last modified on 5/1/91 at 15:51:16 */
#include "fanout_delay.h"
#include "fanout_int.h"
#include "sis.h"

static int get_n_fanouts();

static int lt_trees_optimize();

static multidim_t *optimize_one_source();

static void best_one_source();

static void build_selected_tree();

static void extract_merge_info();

static void insert_lt_tree();

/* CONTENTS */

/* LT-Trees based fanout optimization */

static n_gates_t n_gates;

typedef struct lt_tree_struct lt_tree_t;
struct lt_tree_struct {
    int          balanced; /* boolean: whether this node is the root of a two level
                   balanced tree or not */
    two_level_t  *two_level; /* pointer to two_level_t descriptor (if balanced) */
    int          gate_index; /* gate to put at intermediate nodes (if ! balanced) */
    int          sink_index; /* sink_index - 1 is last sink directly under this node */
    delay_time_t required; /* required time at the source, not including source
                            intrinsic */
    double       area;           /* area of subtree rooted here */
};

static lt_tree_t LT_TREE_INIT_VALUE = {-1, 0, -1, -1, {-INFINITY, -INFINITY},
                                       0.0};

static int max_n_gaps = 5;

/* EXTERNAL INTERFACE */

/* ARGSUSED */
void lt_trees_init(network_t *network, fanout_alg_t *alg) { alg->optimize = lt_trees_optimize; }

/* ARGSUSED */
void lt_trees_set_max_n_gaps(fanout_alg_t *alg, alg_property_t *property) { max_n_gaps = property->value; }

/* INTERNAL INTERFACE */

static multidim_t *two_level_table;

static int lt_trees_optimize(opt_array_t *fanout_info, array_t *tree, fanout_cost_t *cost) {
    generic_fanout_optimizer(fanout_info, tree, cost, optimize_one_source,
                             extract_merge_info, build_selected_tree);
    multidim_free(two_level_table);
    return 1;
}

static multidim_t *optimize_one_source(opt_array_t *fanout_info) {
    multidim_t *table;
    int        indices[5];
    int        max_sinks;
    int        source_index, source_polarity, sink_index, sink_polarity, n_gaps;

    n_gates   = fanout_delay_get_n_gates();
    max_sinks = MAX(fanout_info[POLAR_X].n_elts, fanout_info[POLAR_Y].n_elts);
    indices[0] = n_gates.n_gates; /* source index */
    indices[1] = POLAR_MAX;       /* polarity at output of source */
    indices[2] = max_sinks;       /* max sink index */
    indices[3] = POLAR_MAX;       /* sink polarity */
    indices[4] = max_n_gaps;      /* gaps used so far */
    table = multidim_alloc(lt_tree_t, 5, indices);
    multidim_init(lt_tree_t, table, LT_TREE_INIT_VALUE);
    two_level_table = two_level_optimize_for_lt_trees(fanout_info);

    foreach_polarity(sink_polarity) {
        if (fanout_info[sink_polarity].n_elts == 0)
            continue;
        for (sink_index = fanout_info[sink_polarity].n_elts - 1; sink_index >= 0;
             sink_index--) {
            foreach_gate(n_gates, source_index) {
                for (n_gaps = 0; n_gaps < max_n_gaps; n_gaps++) {
                    foreach_polarity(source_polarity) {
                        if (is_source(n_gates, source_index) &&
                            fanout_delay_get_source_polarity(source_index) !=
                            source_polarity)
                            continue;
                        best_one_source(table, two_level_table, &fanout_info[sink_polarity],
                                        source_index, source_polarity, sink_index,
                                        sink_polarity, n_gaps);
                    }
                }
            }
        }
    }
    return table;
}

/* computes the best LT-tree for a given source, a given set of sinks */
/* and a given polarity for both the output of the source and the input of the
 * sinks */
/* Sink interval of the form [sink_index,fanout_info->n_elts[ */

static void best_one_source(multidim_t *table, multidim_t *two_level_table, opt_array_t *fanout_info, int source_index,
                            int source_polarity, int sink_index, int sink_polarity, int n_gaps) {
    int          sink;
    int          buffer_index, buffer_polarity, buffer_gaps;
    lt_tree_t    *buffer_entry;
    int          n_fanouts;
    double       load;
    double       local_area;
    delay_time_t local_required;
    two_level_t  *two_level_entry =
                         INDEX4P(two_level_t, two_level_table, source_index, source_polarity,
                                 sink_index, sink_polarity);
    lt_tree_t    *entry           = INDEX5P(lt_tree_t, table, source_index, source_polarity,
                                            sink_index, sink_polarity, n_gaps);

    /* start with a two level implementation */
    entry->balanced  = 1;
    entry->two_level = two_level_entry;
    entry->required  = two_level_entry->required;
    entry->area      = two_level_entry->area;

    /* choose the best subdecomposition */
    for (sink = sink_index; sink <= fanout_info->n_elts; sink++) {
        if (source_polarity != sink_polarity && sink > sink_index)
            continue;
        foreach_buffer(n_gates, buffer_index) {
            buffer_polarity = (is_inverter(n_gates, buffer_index))
                              ? POLAR_INV(source_polarity)
                              : source_polarity;
            buffer_gaps     = (sink == sink_index && buffer_polarity == sink_polarity)
                              ? n_gaps - 1
                              : n_gaps;
            if (buffer_gaps < 0)
                continue;
            if (sink < fanout_info->n_elts) {
                buffer_entry   = INDEX5P(lt_tree_t, table, buffer_index, buffer_polarity,
                                         sink, sink_polarity, buffer_gaps);
                local_area     = buffer_entry->area;
                n_fanouts      = 1;
                load           = fanout_delay_get_buffer_load(buffer_index);
                local_required = fanout_delay_backward_intrinsic(buffer_entry->required,
                                                                 buffer_index);
            } else {
                local_area     = 0.0;
                n_fanouts      = 0;
                load           = 0.0;
                local_required = PLUS_INFINITY;
            }
            local_area += fanout_delay_get_area(source_index);
            n_fanouts += sink - sink_index;
            load +=
                    fanout_info->cumul_load[sink] - fanout_info->cumul_load[sink_index];
            load += map_compute_wire_load(n_fanouts);
            if (sink > sink_index)
                SETMIN(local_required, local_required,
                       fanout_info->min_required[sink_index]);
            local_required = fanout_delay_backward_load_dependent(local_required,
                                                                  source_index, load);
            if (GETMIN(entry->required) < GETMIN(local_required)) {
                entry->balanced   = 0;
                entry->gate_index = buffer_index;
                entry->sink_index = sink;
                entry->required   = local_required;
                entry->area       = local_area;
            }
        }
    }
}

/* needs corrective computation if source is root of tree: discard effect of
 * drive at the source */

static void extract_merge_info(opt_array_t *fanout_info, multidim_t *table, multidim_t *merge_table) {
    int             source_index;
    delay_time_t    origin;
    double          load;
    lt_tree_t       *from;
    single_source_t *to;
    int             source_polarity, sink_polarity;

    foreach_gate(n_gates, source_index) {
        foreach_polarity(sink_polarity) {
            foreach_polarity(source_polarity) {
                from = INDEX5P(lt_tree_t, table, source_index, source_polarity, 0,
                               sink_polarity, max_n_gaps - 1);
                to   = INDEX3P(single_source_t, merge_table, source_index,
                               source_polarity, sink_polarity);
                if (fanout_info[sink_polarity].n_elts == 0) {
                    to->required = PLUS_INFINITY;
                } else if (is_buffer(n_gates, source_index)) {
                    to->load      = fanout_delay_get_buffer_load(source_index);
                    to->n_fanouts = 1;
                    to->required =
                            fanout_delay_backward_intrinsic(from->required, source_index);
                    to->area = from->area;
                } else if (fanout_delay_get_source_polarity(source_index) ==
                           source_polarity) {
                    if (from->balanced) {
                        to->load =
                                fanout_delay_get_buffer_load(from->two_level->gate_index) *
                                from->two_level->n_gates;
                        to->n_fanouts = from->two_level->n_gates;
                    } else if (from->sink_index < fanout_info[sink_polarity].n_elts) {
                        to->load      = fanout_info[sink_polarity].cumul_load[from->sink_index] +
                                        fanout_delay_get_buffer_load(from->gate_index);
                        to->n_fanouts = from->sink_index + 1;
                    } else {
                        to->load      = fanout_info[sink_polarity].total_load;
                        to->n_fanouts = fanout_info[sink_polarity].n_elts;
                    }
                    load   = to->load + map_compute_wire_load(to->n_fanouts);
                    origin = fanout_delay_backward_load_dependent(ZERO_DELAY,
                                                                  source_index, load);
                    SETSUB(to->required, from->required, origin);
                    to->area = from->area;
                }
            }
        }
    }
}

static void build_selected_tree(opt_array_t *fanout_info, array_t *tree, selected_source_t *source, multidim_t *table) {
    int p, q;
    int n_fanouts;
    int n_gaps = max_n_gaps - 1;
    int source_polarity, buffer_polarity;

    source_polarity = fanout_delay_get_source_polarity(source->main_source);
    p               = source->main_source_sink_polarity;
    q               = POLAR_INV(p);
    if (fanout_info[q].n_elts > 0) {
        if (is_source(n_gates, source->buffer)) {
            assert(source->buffer == source->main_source);
            n_fanouts = get_n_fanouts(&fanout_info[p], table, source->main_source,
                                      source_polarity, 0, p, n_gaps);
            n_fanouts += get_n_fanouts(&fanout_info[q], table, source->main_source,
                                       source_polarity, 0, q, n_gaps);
            fanout_tree_insert_gate(tree, source->main_source, n_fanouts);
            insert_lt_tree(tree, table, &fanout_info[p], source->main_source,
                           source_polarity, 0, p, n_gaps);
            insert_lt_tree(tree, table, &fanout_info[q], source->main_source,
                           source_polarity, 0, q, n_gaps);
        } else {
            n_fanouts = get_n_fanouts(&fanout_info[p], table, source->main_source,
                                      source_polarity, 0, p, n_gaps);
            fanout_tree_insert_gate(tree, source->main_source, n_fanouts + 1);
            insert_lt_tree(tree, table, &fanout_info[p], source->main_source,
                           source_polarity, 0, p, n_gaps);
            buffer_polarity = (is_inverter(n_gates, source->buffer))
                              ? POLAR_INV(source_polarity)
                              : source_polarity;
            n_fanouts       = get_n_fanouts(&fanout_info[q], table, source->buffer,
                                            buffer_polarity, 0, q, n_gaps);
            fanout_tree_insert_gate(tree, source->buffer, n_fanouts);
            insert_lt_tree(tree, table, &fanout_info[q], source->buffer,
                           buffer_polarity, 0, q, n_gaps);
        }
    } else {
        n_fanouts = get_n_fanouts(&fanout_info[p], table, source->main_source,
                                  source_polarity, 0, p, n_gaps);
        fanout_tree_insert_gate(tree, source->main_source, n_fanouts);
        insert_lt_tree(tree, table, &fanout_info[p], source->main_source,
                       source_polarity, 0, p, n_gaps);
    }
}

static int
get_n_fanouts(opt_array_t *fanout_info, multidim_t *table, int source_index, int source_polarity, int sink_index,
              int sink_polarity, int n_gaps) {
    lt_tree_t *entry;

    if (fanout_info->n_elts == sink_index)
        return 0;
    entry = INDEX5P(lt_tree_t, table, source_index, source_polarity, sink_index,
                    sink_polarity, n_gaps);
    if (entry->balanced)
        return entry->two_level->n_gates;
    if (fanout_info->n_elts == entry->sink_index)
        return entry->sink_index - sink_index;
    return entry->sink_index - sink_index + 1;
}

static void
insert_lt_tree(array_t *tree, multidim_t *table, opt_array_t *fanout_info, int source_index, int source_polarity,
               int sink_index, int sink_polarity, int n_gaps) {
    int       n_fanouts;
    lt_tree_t *entry;
    int       buffer_index;
    int       buffer_polarity;
    int       buffer_gaps;

    if (sink_index == fanout_info->n_elts)
        return;
    entry = INDEX5P(lt_tree_t, table, source_index, source_polarity, sink_index,
                    sink_polarity, n_gaps);
    switch (entry->balanced) {
        case 0:noalg_insert_sinks(tree, fanout_info, sink_index, entry->sink_index);
            buffer_index    = entry->gate_index;
            buffer_polarity = (is_inverter(n_gates, buffer_index))
                              ? POLAR_INV(source_polarity)
                              : source_polarity;
            buffer_gaps =
                    (entry->sink_index == sink_index && buffer_polarity == sink_polarity)
                    ? n_gaps - 1
                    : n_gaps;
            if (entry->sink_index == fanout_info->n_elts)
                return;
            sink_index = entry->sink_index;
            n_fanouts  = get_n_fanouts(fanout_info, table, buffer_index, buffer_polarity,
                                       sink_index, sink_polarity, buffer_gaps);
            fanout_tree_insert_gate(tree, buffer_index, n_fanouts);
            insert_lt_tree(tree, table, fanout_info, buffer_index, buffer_polarity,
                           sink_index, sink_polarity, buffer_gaps);
            break;
        case 1:
            insert_two_level_tree(tree, fanout_info, sink_index, source_index,
                                  entry->two_level);
            break;
        default: fail("illegal value of field lt_tree_t.balanced");
    }
}
