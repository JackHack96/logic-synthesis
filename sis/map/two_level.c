/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/map/two_level.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:26 $
 *
 */
/* Last modified on Fri Feb  7 21:59:46 1992 by touati */
/**
**  MODIFICATION HISTORY:
**
** 02 7-Feb-1992 touati		Fixed the bug introduced by klk's fix.
**				Factorized the common code between compute_balanced_required_time()
**				and compute_best_fit_required_time().
**
** 01 10-June-1991 klk		Fixed a bug in the functions compute_balanced_required_time() and
**				compute_best_fit_required_time().
**/

#include "sis.h"
#include <math.h>
#include "fanout_int.h"
#include "fanout_delay.h"

static delay_time_t compute_balanced_required_time();
static delay_time_t compute_best_fit_required_time();
static delay_time_t from_buffer_assignment_to_required_time();
static int balanced_optimize();
static int two_level_optimize();
static multidim_t *optimize_balanced();
static multidim_t *optimize_one_source_all_sets_of_sinks();
static multidim_t *optimize_two_level();
static void best_one_source();
static void build_selected_tree();
static void extract_merge_info();
static void insert_one_level_tree();



 /* CONTENTS */

 /* this file contains the routines to compute fanout trees */
 /* of depth 2 with one level of identical buffers */
 /* This file can be used as an independent fanout optimizer */
 /* though it was originally intended to be a subroutine for LT-trees */


static n_gates_t n_gates;
static two_level_t  TWO_LEVEL_INIT_VALUE    = {-1, -1, {-INFINITY, -INFINITY}};
static DelayFn two_level_best_fit_fn;


 /* EXTERNAL INTERFACE */

 /* ARGSUSED */
void two_level_init(network, alg)
network_t *network;
fanout_alg_t *alg;
{
  alg->optimize = two_level_optimize;
}

 /* ARGSUSED */
void balanced_init(network, alg)
network_t *network;
fanout_alg_t *alg;
{
  alg->optimize = balanced_optimize;
}


 /* INTERNAL INTERFACE */

static int two_level_optimize(fanout_info, tree, cost)
opt_array_t *fanout_info;		 /* fanout_info[POLAR_X] lists the positive polarity sinks; POLAR_Y negative */
array_t *tree;				 /* array in which the result tree is stored: format of fanout_tree.c */
fanout_cost_t *cost;			 /* contains information on the result: required times, area */
{
  generic_fanout_optimizer(fanout_info, tree, cost, optimize_two_level, extract_merge_info, build_selected_tree);
  return 1;
}

static int balanced_optimize(fanout_info, tree, cost)
opt_array_t *fanout_info;		 /* fanout_info[POLAR_X] lists the positive polarity sinks; POLAR_Y negative */
array_t *tree;				 /* array in which the result tree is stored: format of fanout_tree.c */
fanout_cost_t *cost;			 /* contains information on the result: required times, area */
{
  generic_fanout_optimizer(fanout_info, tree, cost, optimize_balanced, extract_merge_info, build_selected_tree);
  return 1;
}


 /* this routine computes all table entries for all possible sources */
 /* and all possible intermediate buffers */
 /* but only for one polarity at a time */
 /* which polarity is used is automatically infered from the polarity */
 /* of the source and intermediate buffers */
 /* if the source is an actual source of the fanout problem, only */
 /* one polarity is possible */

static multidim_t *optimize_two_level(fanout_info)
opt_array_t *fanout_info;
{
  two_level_best_fit_fn = compute_best_fit_required_time;
  return optimize_one_source_all_sets_of_sinks(fanout_info, 0 /* top level only */);
}

static multidim_t *optimize_balanced(fanout_info)
opt_array_t *fanout_info;
{
  two_level_best_fit_fn = compute_balanced_required_time;
  return optimize_one_source_all_sets_of_sinks(fanout_info, 0 /* top level only */);
}


 /* exported for "lt_trees.c" */

multidim_t *two_level_optimize_for_lt_trees(fanout_info)
opt_array_t *fanout_info;
{
  two_level_best_fit_fn = compute_best_fit_required_time;
  return optimize_one_source_all_sets_of_sinks(fanout_info, 1 /* all sets */);
}

static multidim_t *optimize_one_source_all_sets_of_sinks(fanout_info, all_sets_of_sinks)
opt_array_t *fanout_info;
int all_sets_of_sinks;
{
  multidim_t *table;
  int indices[4];
  int max_sinks;
  int source_index, source_polarity, sink_index, sink_polarity;
  two_level_t *entry;

  n_gates = fanout_delay_get_n_gates();
  max_sinks = MAX(fanout_info[POLAR_X].n_elts, fanout_info[POLAR_Y].n_elts);
  indices[0] = n_gates.n_gates;		 /* source index */
  indices[1] = POLAR_MAX;		 /* polarity at output of source */
  indices[2] = max_sinks;		 /* max sink index */
  indices[3] = POLAR_MAX;		 /* sink polarity */
  table = multidim_alloc(two_level_t, 4, indices);
  multidim_init(two_level_t, table, TWO_LEVEL_INIT_VALUE);

  foreach_gate(n_gates, source_index) {
    foreach_polarity(source_polarity) {
      if (is_source(n_gates, source_index) && fanout_delay_get_source_polarity(source_index) != source_polarity) continue;
      foreach_polarity(sink_polarity) {
	if (fanout_info[sink_polarity].n_elts == 0) continue;
	for (sink_index = 0; sink_index < fanout_info[sink_polarity].n_elts; sink_index++) {
	  if(sink_index > 0 && all_sets_of_sinks == 0) break;
	  entry = INDEX4P(two_level_t, table, source_index, source_polarity, sink_index, sink_polarity);
	  best_one_source(&fanout_info[sink_polarity], entry, source_index, sink_index, (source_polarity != sink_polarity));
	}
      }
    }
  }
  return table;
}


 /* computes the best two level tree for a given source, and a given set of sinks */
 /* of the form [sink_index,fanout_info->n_elts[ */

static void best_one_source(fanout_info, entry, source_index, sink_index, diff_polarity)
opt_array_t *fanout_info;
two_level_t *entry;
int source_index;
int sink_index;
int diff_polarity;			 /* 1 iff source and sinks have different polarity */
{
  int g, k;
  int from, to;
  int n_sinks;
  double load;
  delay_time_t required;

  from = (diff_polarity) ? n_gates.n_pos_buffers : 0;
  to   = (diff_polarity) ? n_gates.n_neg_buffers : n_gates.n_pos_buffers;
  n_sinks = fanout_info->n_elts - sink_index;
  load = fanout_info->total_load - fanout_info->cumul_load[sink_index] + map_compute_wire_load(n_sinks);

  for (g = from; g < to; g++) {
    k = compute_best_number_of_inverters(source_index, g, load, n_sinks);
    required = (two_level_best_fit_fn)(fanout_info, source_index, sink_index, g, &k, NIL(int));
    if (GETMIN(entry->required) < GETMIN(required)) {
      entry->gate_index = g;
      entry->n_gates = k;
      entry->required = required;
      entry->area = k * fanout_delay_get_area(g) + fanout_delay_get_area(source_index);
    }
  }
}


typedef struct best_fit_struct best_fit_t;
struct best_fit_struct {
  delay_time_t required;
  double load;
  int n_fanouts;
};

static best_fit_t BEST_FIT_INIT_VALUE = {{INFINITY, INFINITY}, 0.0, 0};

 /* greedy algorithm: */
 /* assign the sink to the intermediate node that would provide the largest overall */
 /* required time after the sink is inserted */

static delay_time_t compute_best_fit_required_time(fanout_info, source_index, sink_index, buffer_index, n_buffers, sink_to_buffer_map)
opt_array_t *fanout_info;
int source_index;
int sink_index;
int buffer_index;
int *n_buffers;
int *sink_to_buffer_map;	      /* records mapping sink->buffer; if NIL(int), does nothing */
{
    best_fit_t *best_fit_info;
    delay_time_t required_so_far;
    delay_time_t local_required;
    double load, local_load;
    int sink;			      /* index on remaining sinks, from sink_index to fanout_info->n_elts */
    int buffer;			      /* index on intermediate buffers, from 0 to n_buffers */
    int num_assigned_buffers;         /* Number of buffers that are actually assigned sinks */
    int n_elts = fanout_info->n_elts;
    int n_sinks = n_elts - sink_index;

    best_fit_info = ALLOC(best_fit_t, *n_buffers);
    for (buffer = 0; buffer < *n_buffers; buffer++)
      best_fit_info[buffer] = BEST_FIT_INIT_VALUE;

    required_so_far = PLUS_INFINITY;
    for (sink = sink_index; sink < n_elts; sink++) {
	gate_link_t *link;
	int best_fit;
	delay_time_t best_required;

	link = array_fetch(gate_link_t *, fanout_info->required, sink);
	best_fit = -1;
	best_required = MINUS_INFINITY;

	for (buffer = 0; buffer < *n_buffers; buffer++) {
	    SETMIN(local_required, link->required, best_fit_info[buffer].required);
	    local_load = best_fit_info[buffer].load + link->load;
	    local_load += map_compute_wire_load(best_fit_info[buffer].n_fanouts + 1);
	    local_required = fanout_delay_backward_load_dependent(local_required, buffer_index, local_load);
	    if (GETMIN(best_required) < GETMIN(local_required)) {
		best_required = local_required;
		best_fit = buffer;
	    }
	}
	SETMIN(best_fit_info[best_fit].required, best_fit_info[best_fit].required, link->required);
	best_fit_info[best_fit].load    += link->load;
	best_fit_info[best_fit].n_fanouts++;
	if (sink_to_buffer_map != NIL(int)) sink_to_buffer_map[sink] = best_fit;
	SETMIN(required_so_far, required_so_far, best_required);
    }
    return from_buffer_assignment_to_required_time(best_fit_info, n_buffers, required_so_far, source_index, buffer_index, n_sinks);
}

 /* balanced algorithm: ignores the required times to perform the allocation */

static delay_time_t compute_balanced_required_time(fanout_info, source_index, sink_index, buffer_index, n_buffers, sink_to_buffer_map)
opt_array_t *fanout_info;
int source_index;
int sink_index;
int buffer_index;
int *n_buffers;
int *sink_to_buffer_map;       /* records mapping sink->buffer; if NIL(int), does nothing */
{
    best_fit_t *best_fit_info;
    delay_time_t required_so_far;
    delay_time_t local_required;
    double load, local_load;
    int sink;			      /* index on remaining sinks, from sink_index to fanout_info->n_elts */
    int buffer;			      /* index on intermediate buffers, from 0 to n_buffers */
    int n_elts = fanout_info->n_elts;
    int n_sinks = n_elts - sink_index;

    best_fit_info = ALLOC(best_fit_t, *n_buffers);

    for (buffer = 0; buffer < *n_buffers; buffer++)
      best_fit_info[buffer] = BEST_FIT_INIT_VALUE;

    for (sink = sink_index; sink < n_elts; sink++) {
	gate_link_t *link;
	int best_fit;

	load = INFINITY;
	for (buffer = 0; buffer < *n_buffers; buffer++) {
	    if (load > best_fit_info[buffer].load) {
		load = best_fit_info[buffer].load;
		best_fit = buffer;
	    }
	}
	link = array_fetch(gate_link_t *, fanout_info->required, sink);
	best_fit_info[best_fit].load += link->load;
	best_fit_info[best_fit].n_fanouts++;
	SETMIN(best_fit_info[best_fit].required, best_fit_info[best_fit].required, link->required);
	if (sink_to_buffer_map != NIL(int)) sink_to_buffer_map[sink] = best_fit;
    }
    required_so_far = PLUS_INFINITY;
    for (buffer = 0; buffer < *n_buffers; buffer++) {
	local_required = best_fit_info[buffer].required;
	local_load = best_fit_info[buffer].load;
	local_load += map_compute_wire_load(best_fit_info[buffer].n_fanouts);
	local_required = fanout_delay_backward_load_dependent(local_required, buffer_index, local_load);
	SETMIN(required_so_far, required_so_far, local_required);
    }
    return from_buffer_assignment_to_required_time(best_fit_info, n_buffers, required_so_far, source_index, buffer_index, n_sinks);
}

/* 
 * takes a buffer assignment computed by either 'compute_balanced_required_time'
 * or 'compute_best_fit_required_time', performs some consistency checks
 * and returns the required time.
 * NOTE: should deallocate 'best_fit_info'
 */

static delay_time_t from_buffer_assignment_to_required_time(best_fit_info, n_buffers, required_so_far, source_index, buffer_index, n_sinks)
best_fit_t *best_fit_info;
int *n_buffers;
delay_time_t required_so_far;
int source_index;
int buffer_index;
int n_sinks;
{
    int buffer;			      /* index on intermediate buffers, from 0 to n_buffers */
    int num_assigned_buffers;	      /* number of buffers that are actually assigned sinks */
    int num_assigned_sinks;	      /* number of sinks that are actually assigned to a buffer */
    double load;		      /* load at the main buffer */

    /*
     * Make sure all the buffers are assigned some sink.
     * If the number of assigned buffers is not the same as the value of n_buffers, change
     * n_buffers to number of actually assigned buffers.
     */
    num_assigned_buffers = 0;
    num_assigned_sinks = 0;
    for (buffer = 0; buffer < *n_buffers; buffer++) {
	if (best_fit_info[buffer].n_fanouts > 0) {
	    num_assigned_sinks += best_fit_info[buffer].n_fanouts;
	    num_assigned_buffers++;
	}
    }
    FREE(best_fit_info);
    assert(num_assigned_sinks == n_sinks);
    if(num_assigned_buffers != *n_buffers) {
	(void) fprintf(sisout, "balanced: Some buffers in the two-level tree are not assigned sinks\n");
	*n_buffers = num_assigned_buffers;
    }

    required_so_far = fanout_delay_backward_intrinsic(required_so_far, buffer_index);
    load = fanout_delay_get_buffer_load(buffer_index) * *n_buffers + map_compute_wire_load(*n_buffers);
    return fanout_delay_backward_load_dependent(required_so_far, source_index, load);
}


 /* needs corrective computation if source is root of tree: discard effect of drive at the source */

static void extract_merge_info(fanout_info, table, merge_table)
opt_array_t *fanout_info;
multidim_t *table;
multidim_t *merge_table;
{
  int source_index;
  delay_time_t origin;
  double load;
  two_level_t *from;
  single_source_t *to;
  int source_polarity, sink_polarity;
  
  foreach_gate(n_gates, source_index) {
    foreach_polarity(sink_polarity) {
      foreach_polarity(source_polarity) {
	from = INDEX4P(two_level_t, table, source_index, source_polarity, 0, sink_polarity);
	to = INDEX3P(single_source_t, merge_table, source_index, source_polarity, sink_polarity);
	if (fanout_info[sink_polarity].n_elts == 0) {
	  to->required = PLUS_INFINITY;
	} else if (from->gate_index == -1) {
	  continue;
	} else if (is_buffer(n_gates,source_index)) {
	  to->load = fanout_delay_get_buffer_load(source_index);
	  to->n_fanouts = 1;
	  to->required = fanout_delay_backward_intrinsic(from->required, source_index);
	  to->area = from->area;
	} else if (fanout_delay_get_source_polarity(source_index) == source_polarity) {
	  to->load = fanout_delay_get_buffer_load(from->gate_index) * from->n_gates;
	  to->n_fanouts = from->n_gates;
	  load = to->load + map_compute_wire_load(to->n_fanouts);
	  origin = fanout_delay_backward_load_dependent(ZERO_DELAY, source_index, load);
	  SETSUB(to->required, from->required, origin);
	  to->area = from->area;
	}
      }
    }
  }
}

static void build_selected_tree(fanout_info, tree, source, table)
opt_array_t *fanout_info;
array_t *tree;
selected_source_t *source;
multidim_t *table;
{
  int p, q;
  int n_fanouts;
  int source_polarity, buffer_polarity;
  two_level_t *entry_p, *entry_q;
  
  source_polarity = fanout_delay_get_source_polarity(source->main_source);
  p = source->main_source_sink_polarity;
  q = POLAR_INV(p);
  entry_p = INDEX4P(two_level_t, table, source->main_source, source_polarity, 0, p);
  if (fanout_info[p].n_elts == 0) entry_p->n_gates = 0;

  if (fanout_info[q].n_elts > 0) {
    if (is_source(n_gates, source->buffer)) {
      assert(source->buffer == source->main_source);
      entry_q = INDEX4P(two_level_t, table, source->main_source, source_polarity, 0, q);
      n_fanouts = entry_p->n_gates + entry_q->n_gates;
      fanout_tree_insert_gate(tree, source->main_source, n_fanouts);
      insert_two_level_tree(tree, &fanout_info[p], 0, source->main_source, entry_p);
      insert_two_level_tree(tree, &fanout_info[q], 0, source->main_source, entry_q);
    } else {
      n_fanouts = entry_p->n_gates + 1;
      fanout_tree_insert_gate(tree, source->main_source, n_fanouts);
      insert_two_level_tree(tree, &fanout_info[p], 0, source->main_source, entry_p);
      buffer_polarity = (is_inverter(n_gates, source->buffer)) ? POLAR_INV(source_polarity) : source_polarity;
      entry_q = INDEX4P(two_level_t, table, source->buffer, buffer_polarity, 0, q);
      n_fanouts = entry_q->n_gates;
      fanout_tree_insert_gate(tree, source->buffer, n_fanouts);
      insert_two_level_tree(tree, &fanout_info[q], 0, source->buffer, entry_q);
    }
  } else {
    fanout_tree_insert_gate(tree, source->main_source, entry_p->n_gates);
    insert_two_level_tree(tree, &fanout_info[p], 0, source->main_source, entry_p);
  }
}

void insert_two_level_tree(tree, fanout_info, sink_index, source_index, entry)
array_t *tree;
opt_array_t *fanout_info;
int sink_index;
int source_index;
two_level_t *entry;
{
  int i, j;
  int *sink_to_buffer_map;
  st_table **fanouts;

  if (entry->n_gates == 0) return;
  sink_to_buffer_map = ALLOC(int, fanout_info->n_elts);
  fanouts = ALLOC(st_table *, entry->n_gates);
  (void) (*two_level_best_fit_fn)(fanout_info, source_index, sink_index, entry->gate_index, &(entry->n_gates), sink_to_buffer_map);
  for (i = 0; i < entry->n_gates; i++)
    fanouts[i] = st_init_table(st_numcmp, st_numhash);
  for (j = sink_index; j < fanout_info->n_elts; j++)
    (void) st_insert(fanouts[sink_to_buffer_map[j]], (char *) j, NIL(char));
  for (i = 0; i < entry->n_gates; i++) {
    fanout_tree_insert_gate(tree, entry->gate_index, st_count(fanouts[i]));
    insert_one_level_tree(tree, fanout_info, fanouts[i]);
  }
  for (i = 0; i < entry->n_gates; i++)
    st_free_table(fanouts[i]);
  FREE(fanouts);
  FREE(sink_to_buffer_map);
}

static void insert_one_level_tree(tree, fanout_info, fanout_table)
array_t *tree;
opt_array_t *fanout_info;
st_table *fanout_table;
{
  char *key, *value;
  st_generator *gen;
  st_foreach_item(fanout_table, gen, &key, &value) {
    int sink_index = (int) key;
    gate_link_t *link = array_fetch(gate_link_t *, fanout_info->required, sink_index);
    fanout_tree_insert_sink(tree, link);
  }
}
