/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/map/bottom_up.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:24 $
 *
 */
/* file @(#)bottom_up.c	1.7 */
/* last modified on 7/2/91 at 19:44:14 */
/**
**  MODIFICATION HISTORY:
**
**  01 27-May-91 klk		New code to create a buffer tree that satisfies the fanout limit
**                              using the bottom_up algorithm.
**  01 10-Jun-91 klk		Fixed a bug in the function build_tree_folim_bottom_up_rec().
**/
/*
 * $Log: bottom_up.c,v $
 * Revision 1.1.1.1  2004/02/07 10:14:24  pchong
 * imported
 *
 * Revision 1.2  1992/05/06  18:55:51  sis
 * SIS release 1.1
 *
 * Revision 1.2  1992/05/06  18:55:51  sis
 * SIS release 1.1
 *
 * Revision 1.1  1992/04/17  21:47:50  sis
 * Initial revision
 *
 * Revision 1.1  1992/04/17  21:47:50  sis
 * Initial revision
 *
 * Revision 1.1  92/01/08  17:40:31  sis
 * Initial revision
 * 
 * Revision 1.2  91/07/02  10:48:32  touati
 * remove Pani's changes. Have been made global to all algorithms.
 * 
 * Revision 1.1  91/06/28  14:48:00  touati
 * Initial revision
 * 
*/



#include "sis.h"
#include <math.h>
#include "fanout_int.h"
#include "fanout_delay.h"


/* A node in the buffer tree data structure for the bottom_up algorithm */
typedef struct bottom_up_struct bottom_up_t;
struct bottom_up_struct {
    delay_time_t required;	/* Required time at the source node of the tree */
    double area;		/* area of the buffer */
    int gate_index;		/* Index of the buffer in the library */
    array_t *children;		/* Children of the node in the buffer tree */
    gate_link_t *link;		/* Pointer to a sink node for a leaf node in the buffer tree */
};

static bottom_up_t *build_tree_bottom_up_rec();
static bottom_up_t *create_tree_node();
static int bottom_up_optimize();
static int compute_sink_index();
static int get_polarity_with_latest_req_time();
static st_table *fanout_info_extract();
static void build_tree_bottom_up();
static void create_node_for_sink();
static void put_tree_in_fanout_tree_order();
static void select_best_subgroup();


 /* CONTENTS */

 /* Bottom up fanout optimization derived from combinational merging */

static n_gates_t n_gates;


 /* EXTERNAL INTERFACE */

 /* ARGSUSED */
void bottom_up_init(network, alg)
network_t *network;
fanout_alg_t *alg;
{
  alg->optimize = bottom_up_optimize;
}


 /* INTERNAL INTERFACE */

static int bottom_up_optimize(fanout_info, tree, cost)
opt_array_t *fanout_info;		 /* fanout_info[POLAR_X] lists the positive polarity sinks; POLAR_Y negative */
array_t *tree;				 /* array in which the result tree is stored: format of fanout_tree.c */
fanout_cost_t *cost;			 /* contains information on the result: required times, area */
{
  int source_index;
  array_t *local_tree, *best_tree;
  fanout_cost_t local_cost;

  n_gates = fanout_delay_get_n_gates();
  cost->slack = MINUS_INFINITY;
  cost->area = INFINITY;
  best_tree = fanout_tree_alloc();
  foreach_source(n_gates, source_index) {
    local_tree = fanout_tree_alloc();
    build_tree_bottom_up(fanout_info, source_index, local_tree, &local_cost);
    if (GETMIN(cost->slack) < GETMIN(local_cost.slack)) {
      array_free(best_tree);
      best_tree = local_tree;
      *cost = local_cost;
    } else {
      array_free(local_tree);
    }
  }
  fanout_tree_copy(tree, best_tree);
  array_free(best_tree);
  return 1;
}


static void build_tree_bottom_up(fanout_info, source_index, tree, cost)
opt_array_t *fanout_info;
int source_index;
array_t *tree;
fanout_cost_t *cost;
{
  int i, p;
  bottom_up_t *root, *node;
  gate_link_t *link;
  st_generator *gen;
  st_table *table = st_init_table(st_numcmp, st_numhash);

  foreach_polarity(p) {
    for (i = 0; i < fanout_info[p].n_elts; i++) {
      link = array_fetch(gate_link_t *, fanout_info[p].required, i);
      create_node_for_sink(link, table);
    }
  }
  root = build_tree_bottom_up_rec(fanout_info, source_index, table);
  cost->slack = root->required;
  cost->area = root->area;
  put_tree_in_fanout_tree_order(tree, root);
  st_foreach_item(table, gen, (char **) &link, (char **) &node) {
    assert(link == node->link);
    if (link->node == NIL(node_t)) FREE(link);
    array_free(node->children);
    FREE(node);
  }
  st_free_table(table);
}

static bottom_up_t *build_tree_bottom_up_rec(fanout_info, source_index, table)
opt_array_t *fanout_info;
int source_index;
st_table *table;
{
  int p, q;
  int source_polarity;
  int buffer_index, sink_index;
  int no_buffer_ok;
  bottom_up_t *node, *result;
  opt_array_t new_fanout_info[2];

 /* if only one output left, put it on the source node and terminate */
  source_polarity = fanout_delay_get_source_polarity(source_index);
  p = source_polarity;
  q = POLAR_INV(p);
  if (fanout_info[p].n_elts == 1 && fanout_info[q].n_elts == 0) {
    return create_tree_node(&fanout_info[p], 0, source_index, table);
  }

 /* otherwise, determine which polarity to use next (latest req. time) */
  p = get_polarity_with_latest_req_time(fanout_info);
  q = POLAR_INV(p);

 /* select the best group of late required and corresponding buffer_index */
  no_buffer_ok = (source_polarity == p && fanout_info[q].n_elts == 0);
  select_best_subgroup(&fanout_info[p], source_index, no_buffer_ok, &buffer_index, &sink_index);
  if (buffer_index == -1) {
    assert(p == source_polarity && fanout_info[q].n_elts == 0);
    return create_tree_node(&fanout_info[p], 0, source_index, table);
  }

 /* if still work to do: create new_fanout_info properly, function of buffer polarity */
  node = create_tree_node(&fanout_info[p], sink_index, buffer_index, table);
  if (is_inverter(n_gates, buffer_index)) {
    new_fanout_info[p].links = fanout_info_extract(&fanout_info[p], 0, sink_index);
    fanout_info_preprocess(&new_fanout_info[p]);
    new_fanout_info[q].links = fanout_info_extract(&fanout_info[q], 0, fanout_info[q].n_elts);
    st_insert(new_fanout_info[q].links, (char *) node->link, NIL(char));
    fanout_info_preprocess(&new_fanout_info[q]);
  } else {
    new_fanout_info[p].links = fanout_info_extract(&fanout_info[p], 0, sink_index);
    st_insert(new_fanout_info[p].links, (char *) node->link, NIL(char));
    fanout_info_preprocess(&new_fanout_info[p]);
    new_fanout_info[q].links = fanout_info_extract(&fanout_info[q], 0, fanout_info[q].n_elts);
    fanout_info_preprocess(&new_fanout_info[q]);
  }
  result = build_tree_bottom_up_rec(new_fanout_info, source_index, table);
  foreach_polarity (p) {  fanout_info_free(&new_fanout_info[p], 0);  }
  return result;
}

 /* best_sink: [best_sink,n_elts-1] are under the new node */

static void select_best_subgroup(fanout_info, source_index, no_buffer_ok, best_buffer, best_sink)
opt_array_t *fanout_info;
int source_index;
int no_buffer_ok;
int *best_buffer;
int *best_sink;
{
  int buffer_index;
  int k, best_k;
  int local_sink;
  double load, local_load;
  delay_time_t best_required, local_required;

  load = fanout_info->total_load + map_compute_wire_load(fanout_info->n_elts);
  *best_buffer = -1;
  best_required = MINUS_INFINITY;
  foreach_buffer(n_gates, buffer_index) {
    if (fanout_info->n_elts == 1 && ! is_inverter(n_gates, buffer_index)) continue;
    k = compute_best_number_of_inverters(source_index, buffer_index, load, fanout_info->n_elts);
    local_sink = compute_sink_index(fanout_info, k);
    local_load = fanout_info->total_load - fanout_info->cumul_load[local_sink];
    local_load += map_compute_wire_load(fanout_info->n_elts - local_sink);
    local_required = fanout_info->min_required[local_sink];
    local_required = fanout_delay_backward_load_dependent(local_required, buffer_index, local_load);
    local_required = fanout_delay_backward_intrinsic(local_required, buffer_index);
    local_load = fanout_delay_get_buffer_load(buffer_index) * k;
    local_load += map_compute_wire_load(k);
    local_required = fanout_delay_backward_load_dependent(local_required, source_index, local_load);
    if (GETMIN(best_required) < GETMIN(local_required)) {
      best_k = k;
      best_required = local_required;
      *best_buffer = buffer_index;
      *best_sink = local_sink;
    }
  }
  if (no_buffer_ok && best_k == 1) {
    assert(*best_sink == 0);
    local_load = load;
    local_required = fanout_info->min_required[0];
    local_required = fanout_delay_backward_load_dependent(local_required, source_index, local_load);
    if (GETMIN(best_required) < GETMIN(local_required)) {
      *best_buffer = -1;
      *best_sink = 0;
    }
  }
}

 /* guarantees a reasonable level of progress */
 /* should at least take two sinks */

static int compute_sink_index(fanout_info, k)
opt_array_t *fanout_info;
int k;
{
  int i;
  double load;
  double load_threshold = fanout_info->total_load / k;

  assert(fanout_info->n_elts > 0);
  if (fanout_info->n_elts == 1) return 0;
  for (i = fanout_info->n_elts - 2; i >= 0; i--) {
    load = fanout_info->total_load - fanout_info->cumul_load[i];
    if (load > load_threshold)
      return i;
  }
  return 0;
}

static st_table *fanout_info_extract(fanout_info, from, to)
opt_array_t *fanout_info;
int from;
int to;
{
  int i;
  gate_link_t *link;
  st_table *result = st_init_table(st_numcmp, st_numhash);

  for (i = from; i < to; i++) {
    link = array_fetch(gate_link_t *, fanout_info->required, i);
    st_insert(result, (char *) link, NIL(char));
  }
  return result;
}

static void put_tree_in_fanout_tree_order(tree, root)
array_t *tree;
bottom_up_t *root;
{
  int i;
  bottom_up_t *node;

  if (array_n(root->children) == 0) {
    fanout_tree_insert_sink(tree, root->link);
  } else {
    fanout_tree_insert_gate(tree, root->gate_index, array_n(root->children));
    for (i = 0; i < array_n(root->children); i++) {
      node = array_fetch(bottom_up_t *, root->children, i);
      put_tree_in_fanout_tree_order(tree, node);
    }
  }
}

static bottom_up_t *create_tree_node(fanout_info, sink_index, source_index, table)
opt_array_t *fanout_info;
int sink_index;
int source_index;
st_table *table;
{
  int i;
  double area;
  double load;
  delay_time_t required;
  gate_link_t *link;
  bottom_up_t *child;
  bottom_up_t *result = ALLOC(bottom_up_t, 1);

 /* first, the gate_link_t */
  link = result->link = ALLOC(gate_link_t, 1);
  link->node = NIL(node_t);
  link->pin = -1;
  required = fanout_info->min_required[sink_index];
  load = fanout_info->total_load - fanout_info->cumul_load[sink_index];
  load += map_compute_wire_load(fanout_info->n_elts - sink_index);
  required = fanout_delay_backward_load_dependent(required, source_index, load);
  if (is_buffer(n_gates, source_index)) {
    link->required = fanout_delay_backward_intrinsic(required, source_index);
    link->load = fanout_delay_get_buffer_load(source_index);
  } else {
    link->required = required;
    link->load = 0.0;
  }

 /* now, the node itself */
  result->required = result->link->required;
  result->children = array_alloc(bottom_up_t *, 0);
  area = 0.0;
  for (i = sink_index; i < fanout_info->n_elts; i++) {
    link = array_fetch(gate_link_t *, fanout_info->required, i);
    assert(st_lookup(table, (char *) link, (char **) &child));
    array_insert_last(bottom_up_t *, result->children, child);
    area += child->area;
  }
  result->gate_index = source_index;
  result->area = area + fanout_delay_get_area(source_index);
  st_insert(table, (char *) result->link, (char *) result);
  return result;
}

static void create_node_for_sink(link, table)
gate_link_t *link;
st_table *table;
{
  bottom_up_t *result = ALLOC(bottom_up_t, 1);

  result->link = link;
  result->required = link->required;
  result->area = 0.0;
  result->children = array_alloc(bottom_up_t *, 0);
  result->gate_index = -1;
  st_insert(table, (char *) link, (char *) result);
}

static int get_polarity_with_latest_req_time(fanout_info)
opt_array_t *fanout_info;
{
  int p, q;
  gate_link_t *link;
  delay_time_t required[2];

  assert(fanout_info[POLAR_X].n_elts != 0 || fanout_info[POLAR_Y].n_elts != 0);
  foreach_polarity(p) {
    q = POLAR_INV(p);
    if (fanout_info[p].n_elts == 0) return q;
    link = array_fetch(gate_link_t *, fanout_info[p].required, fanout_info[p].n_elts - 1);
    required[p] = link->required;
  }
  return (GETMIN(required[POLAR_X]) > GETMIN(required[POLAR_Y])) ? POLAR_X : POLAR_Y;
}
