
/* file @(#)mix_lt_trees.c	1.3 */
/* last modified on 5/1/91 at 15:51:41 */
#include "fanout_delay.h"
#include "fanout_int.h"
#include "sis.h"
#include <math.h>

static int get_n_fanouts();

static int mixed_lt_trees_optimize();

static multidim_t *optimize_one_source();

static void best_one_source();

static void build_selected_tree();

static void insert_mixed_lt_tree();

static void select_best_source();

/* CONTENTS */

/* LT-Trees based fanout optimization */

static n_gates_t n_gates;

typedef struct lt_tree_struct lt_tree_t;
struct lt_tree_struct {
  int balanced; /* boolean: whether this node is the root of a two level
                   balanced tree or not */
  two_level_t *two_level; /* pointer to two_level_t descriptor (if balanced) */
  int gate_index; /* gate to put at intermediate nodes (if ! balanced) */
  int x_index; /* x_index - 1 is last sink directly under this node, if any */
  int y_index; /* y_index - 1 is last sink directly under this node, if any */
  delay_time_t required; /* required time at the source, not including source
                            intrinsic */
  double area;           /* area of subtree rooted here */
};

static lt_tree_t LT_TREE_INIT_VALUE = {
    -1, 0, -1, -1, -1, {-INFINITY, -INFINITY}, 0.0};

static int max_n_gaps = 5;
static int max_x_index = 15;
static int max_y_index = 15;

/* EXTERNAL INTERFACE */

/* ARGSUSED */
void mixed_lt_trees_init(network, alg) network_t *network;
fanout_alg_t *alg;
{ alg->optimize = mixed_lt_trees_optimize; }

/* ARGSUSED */
void mixed_lt_trees_set_max_n_gaps(alg, property) fanout_alg_t *alg;
alg_property_t *property;
{ max_n_gaps = property->value; }

/* ARGSUSED */
void mixed_lt_trees_set_max_x_index(alg, property) fanout_alg_t *alg;
alg_property_t *property;
{ max_x_index = property->value; }

/* ARGSUSED */
void mixed_lt_trees_set_max_y_index(alg, property) fanout_alg_t *alg;
alg_property_t *property;
{ max_y_index = property->value; }

/* INTERNAL INTERFACE */

static multidim_t *two_level_table;

static int mixed_lt_trees_optimize(fanout_info, tree, cost)
    opt_array_t *fanout_info; /* fanout_info[POLAR_X] lists the positive
                                 polarity sinks; POLAR_Y negative */
array_t *tree; /* array in which the result tree is stored: format of
                  fanout_tree.c */
fanout_cost_t
    *cost; /* contains information on the result: required times, area */
{
  multidim_t *table;
  selected_source_t source;

  cost->slack = MINUS_INFINITY;
  cost->area = INFINITY;
  if (fanout_info[POLAR_X].n_elts == 0)
    return 0;
  if (fanout_info[POLAR_Y].n_elts == 0)
    return 0;
  if (fanout_info[POLAR_X].n_elts > max_x_index &&
      fanout_info[POLAR_Y].n_elts > max_y_index)
    return 0;
  table = optimize_one_source(fanout_info);
  select_best_source(table, &source, cost);
  build_selected_tree(fanout_info, tree, &source, table);
  multidim_free(two_level_table);
  multidim_free(table);
  return 1;
}

static multidim_t *optimize_one_source(fanout_info) opt_array_t *fanout_info;
{
  multidim_t *table;
  int indices[5];
  int n_gaps;
  int x_index, y_index;
  int source_index, source_polarity;

  n_gates = fanout_delay_get_n_gates();
  indices[0] = n_gates.n_gates; /* source index */
  indices[1] = POLAR_MAX;       /* polarity at output of source */
  indices[2] = fanout_info[POLAR_X].n_elts + 1;
  indices[3] = fanout_info[POLAR_Y].n_elts + 1;
  indices[4] = max_n_gaps; /* gaps used so far */
  table = multidim_alloc(lt_tree_t, 5, indices);
  multidim_init(lt_tree_t, table, LT_TREE_INIT_VALUE);
  two_level_table = two_level_optimize_for_lt_trees(fanout_info);

  assert(fanout_info[POLAR_X].n_elts > 0 && fanout_info[POLAR_Y].n_elts > 0);
  for (x_index = fanout_info[POLAR_X].n_elts; x_index >= 0; x_index--) {
    for (y_index = fanout_info[POLAR_Y].n_elts; y_index >= 0; y_index--) {
      foreach_gate(n_gates, source_index) {
        for (n_gaps = 0; n_gaps < max_n_gaps; n_gaps++) {
          foreach_polarity(source_polarity) {
            if (is_source(n_gates, source_index) &&
                fanout_delay_get_source_polarity(source_index) !=
                    source_polarity)
              continue;
            best_one_source(table, two_level_table, fanout_info, source_index,
                            source_polarity, x_index, y_index, n_gaps);
          }
        }
      }
    }
  }
  return table;
}

/* computes the best LT-tree for a given source, a given set of sinks */

static void best_one_source(table, two_level_table, fanout_info, source_index,
                            source_polarity, x_index, y_index,
                            n_gaps) multidim_t *table;
multidim_t *two_level_table;
opt_array_t *fanout_info;
int source_index;
int source_polarity;
int x_index;
int y_index;
int n_gaps;
{
  int p, q, p_sink, q_sink, sink, next_x, next_y;
  int buffer_index, buffer_polarity, buffer_gaps;
  lt_tree_t *buffer_entry;
  int n_fanouts;
  double load;
  double local_area;
  delay_time_t local_required;
  two_level_t *two_level_entry;
  lt_tree_t *entry = INDEX5P(lt_tree_t, table, source_index, source_polarity,
                             x_index, y_index, n_gaps);

  if (x_index == fanout_info[POLAR_X].n_elts &&
      y_index == fanout_info[POLAR_Y].n_elts) {
    entry->balanced = 0;
    entry->gate_index = -1;
    entry->x_index = x_index;
    entry->y_index = y_index;
    entry->required = PLUS_INFINITY;
    entry->area = 0.0;
    return;
  }
  if (x_index == fanout_info[POLAR_X].n_elts) {
    two_level_entry = INDEX4P(two_level_t, two_level_table, source_index,
                              source_polarity, y_index, POLAR_Y);
  } else if (y_index == fanout_info[POLAR_Y].n_elts) {
    two_level_entry = INDEX4P(two_level_t, two_level_table, source_index,
                              source_polarity, x_index, POLAR_X);
  } else {
    two_level_entry = NIL(two_level_t);
  }
  if (two_level_entry != NIL(two_level_t)) {
    entry->balanced = 1;
    entry->two_level = two_level_entry;
    entry->required = two_level_entry->required;
    entry->area = two_level_entry->area;
  }

  /* choose the best subdecomposition */
  p = source_polarity;
  q = POLAR_INV(source_polarity);
  p_sink = (p == POLAR_X) ? x_index : y_index;
  q_sink = (q == POLAR_X) ? x_index : y_index;
  for (sink = p_sink; sink <= fanout_info[p].n_elts; sink++) {
    foreach_buffer(n_gates, buffer_index) {
      buffer_polarity = (is_inverter(n_gates, buffer_index)) ? q : p;
      buffer_gaps = (sink == p_sink && sink < fanout_info[p].n_elts)
                        ? n_gaps - 1
                        : n_gaps;
      if (buffer_gaps < 0)
        continue;
      next_x = (p == POLAR_X) ? sink : q_sink;
      next_y = (p == POLAR_Y) ? sink : q_sink;
      if (sink == fanout_info[p].n_elts && q_sink == fanout_info[q].n_elts) {
        local_area = 0.0;
        n_fanouts = 0;
        load = 0.0;
        local_required = PLUS_INFINITY;
      } else {
        buffer_entry = INDEX5P(lt_tree_t, table, buffer_index, buffer_polarity,
                               next_x, next_y, buffer_gaps);
        local_area = buffer_entry->area;
        n_fanouts = 1;
        load = fanout_delay_get_buffer_load(buffer_index);
        local_required = fanout_delay_backward_intrinsic(buffer_entry->required,
                                                         buffer_index);
      }
      local_area += fanout_delay_get_area(source_index);
      n_fanouts += sink - p_sink;
      load +=
          fanout_info[p].cumul_load[sink] - fanout_info[p].cumul_load[p_sink];
      load += map_compute_wire_load(n_fanouts);
      if (sink > p_sink)
        SETMIN(local_required, local_required,
               fanout_info[p].min_required[p_sink]);
      local_required = fanout_delay_backward_load_dependent(local_required,
                                                            source_index, load);
      if (GETMIN(entry->required) < GETMIN(local_required)) {
        entry->balanced = 0;
        entry->gate_index = buffer_index;
        entry->x_index = next_x;
        entry->y_index = next_y;
        entry->required = local_required;
        entry->area = local_area;
      }
    }
  }
}

static void select_best_source(table, best_source, best_cost) multidim_t *table;
selected_source_t *best_source;
fanout_cost_t *best_cost;
{
  int source_index;
  int source_polarity;
  lt_tree_t *entry;

  best_cost->slack = MINUS_INFINITY;
  best_cost->area = INFINITY;
  foreach_source(n_gates, source_index) {
    source_polarity = fanout_delay_get_source_polarity(source_index);
    entry = INDEX5P(lt_tree_t, table, source_index, source_polarity, 0, 0,
                    max_n_gaps - 1);
    if (GETMIN(best_cost->slack) < GETMIN(entry->required)) {
      best_cost->slack = entry->required;
      best_cost->area = entry->area;
      best_source->main_source = source_index;
    }
  }
  assert(GETMIN(best_cost->slack) > -INFINITY);
}

static void build_selected_tree(fanout_info, tree, source,
                                table) opt_array_t *fanout_info;
array_t *tree;
selected_source_t *source;
multidim_t *table;
{
  int n_fanouts;
  int source_polarity = fanout_delay_get_source_polarity(source->main_source);

  n_fanouts = get_n_fanouts(fanout_info, table, source->main_source,
                            source_polarity, 0, 0, max_n_gaps - 1);
  fanout_tree_insert_gate(tree, source->main_source, n_fanouts);
  insert_mixed_lt_tree(tree, table, fanout_info, source->main_source,
                       source_polarity, 0, 0, max_n_gaps - 1);
}

static void insert_mixed_lt_tree(tree, table, fanout_info, source_index,
                                 source_polarity, x_index, y_index,
                                 n_gaps) array_t *tree;
multidim_t *table;
opt_array_t *fanout_info;
int source_index;
int source_polarity;
int x_index;
int y_index;
int n_gaps;
{
  int p, q, p_sink, q_sink, sink;
  int n_fanouts;
  lt_tree_t *entry;
  int buffer_index;
  int buffer_polarity;
  int buffer_gaps;
  int buffer_x, buffer_y;

  if (x_index == fanout_info[POLAR_X].n_elts &&
      y_index == fanout_info[POLAR_Y].n_elts)
    return;
  entry = INDEX5P(lt_tree_t, table, source_index, source_polarity, x_index,
                  y_index, n_gaps);
  p = source_polarity;
  q = POLAR_INV(p);
  p_sink = (p == POLAR_X) ? x_index : y_index;
  q_sink = (q == POLAR_X) ? x_index : y_index;
  switch (entry->balanced) {
  case 0:
    sink = (p == POLAR_X) ? entry->x_index : entry->y_index;
    assert(q_sink == ((q == POLAR_X) ? entry->x_index : entry->y_index));
    noalg_insert_sinks(tree, &fanout_info[p], p_sink, sink);
    if (sink == fanout_info[p].n_elts && q_sink == fanout_info[q].n_elts)
      return;
    buffer_index = entry->gate_index;
    buffer_polarity = (is_inverter(n_gates, buffer_index)) ? q : p;
    buffer_gaps =
        (sink == p_sink && sink < fanout_info[p].n_elts) ? n_gaps - 1 : n_gaps;
    buffer_x = (p == POLAR_X) ? sink : q_sink;
    buffer_y = (p == POLAR_Y) ? sink : q_sink;
    n_fanouts = get_n_fanouts(fanout_info, table, buffer_index, buffer_polarity,
                              buffer_x, buffer_y, buffer_gaps);
    fanout_tree_insert_gate(tree, buffer_index, n_fanouts);
    insert_mixed_lt_tree(tree, table, fanout_info, buffer_index,
                         buffer_polarity, buffer_x, buffer_y, buffer_gaps);
    break;
  case 1:
    if (p_sink == fanout_info[p].n_elts) {
      assert(q_sink < fanout_info[q].n_elts);
      insert_two_level_tree(tree, &fanout_info[q], q_sink, source_index,
                            entry->two_level);
    } else if (q_sink == fanout_info[q].n_elts) {
      assert(p_sink < fanout_info[p].n_elts);
      insert_two_level_tree(tree, &fanout_info[p], p_sink, source_index,
                            entry->two_level);
    } else {
      fail("can't use two level when sinks of both polarities remain");
    }
    break;
  default:
    fail("illegal value of field lt_tree_t.balanced");
  }
}

static int get_n_fanouts(fanout_info, table, source_index, source_polarity,
                         x_index, y_index, n_gaps) opt_array_t *fanout_info;
multidim_t *table;
int source_index;
int source_polarity;
int x_index;
int y_index;
int n_gaps;
{
  int p, q, p_sink, q_sink, sink;
  lt_tree_t *entry;

  if (x_index == fanout_info[POLAR_X].n_elts &&
      y_index == fanout_info[POLAR_Y].n_elts)
    return 0;
  entry = INDEX5P(lt_tree_t, table, source_index, source_polarity, x_index,
                  y_index, n_gaps);
  if (entry->balanced)
    return entry->two_level->n_gates;
  p = source_polarity;
  q = POLAR_INV(p);
  p_sink = (p == POLAR_X) ? x_index : y_index;
  q_sink = (q == POLAR_X) ? x_index : y_index;
  sink = (p == POLAR_X) ? entry->x_index : entry->y_index;
  if (sink == fanout_info[p].n_elts && q_sink == fanout_info[q].n_elts)
    return (sink - p_sink);
  return sink - p_sink + 1;
}
