
/* file @(#)fanout_tree.c	1.5 */
/* last modified on 7/1/91 at 22:40:54 */

#include "fanout_delay.h"
#include "fanout_int.h"
#include "gate_link.h"
#include "map_int.h"
#include "map_macros.h"
#include "sis.h"

static n_gates_t n_gates;
typedef enum fanout_tree_enum fanout_tree_t;
enum fanout_tree_enum { SINK_NODE, BUFFER_NODE, SOURCE_NODE };
typedef struct fanout_node_struct fanout_node_t;
struct fanout_node_struct {
  fanout_tree_t type;
  gate_link_t *link;
  int gate_index;
  int arity;
  array_t *children;
  node_t *node;
  int polarity;
  double load;
#ifdef DEBUG
  delay_time_t save_required;
#endif /* DEBUG */
  delay_time_t required;
  delay_time_t arrival; /* including intrinsic delay */
  double area;
  array_t *peephole;
  array_t *remove_opt;
};

static fanout_node_t DEFAULT_FANOUT_NODE = {
    /* type */ SINK_NODE,
    /* link */ NIL(gate_link_t),
    /* gate_index */ -1,
    /* arity */ 0,
    /* children */ NIL(array_t),
    /* node */ NIL(node_t),
    /* polarity */ POLAR_X,
    /* load */ 0.0,
#ifdef DEBUG
    /* save_required */ {-INFINITY, -INFINITY},
#endif
    /* required */ {-INFINITY, -INFINITY},
    /* arrival */ {INFINITY, INFINITY},
    /* area */ INFINITY,
    /* peephole */ NIL(array_t),
    /* remove_opt */ NIL(array_t)};

#include "fanout_tree_static.h"

/* EXTERNAL INTERFACE */

array_t *fanout_tree_alloc() { return array_alloc(fanout_node_t, 0); }

/* if flag free_links_p is 1, should also free the gate_links */

void fanout_tree_free(array, free_links_p) array_t *array;
int free_links_p;
{
  int i;
  fanout_node_t *node;

  for (i = 0; i < array_n(array); i++) {
    node = array_fetch_p(fanout_node_t, array, i);
    array_free(node->children);
    if (free_links_p && node->type == SINK_NODE) {
      FREE(node->link);
    }
  }
  array_free(array);
}

/* necessary if the tree is to survive its fanout environment */
/* also, since the MAP(link->node)->save_binding is going to disappear */
/* we need to replace it by the fanout number of the node */
/* it is ugly to overload like this. See fanout_est_get_pin_fanout_index */
/* a value of -1 for the fanout_index would indicate something was not computed
 * properly */
/* Always initialized to -1 (default value) to catch that kind of problems */

void fanout_tree_save_links(array) array_t *array;
{
  int i;
  gate_link_t *link;
  fanout_node_t *node;

  for (i = 0; i < array_n(array); i++) {
    node = array_fetch_p(fanout_node_t, array, i);
    if (node->type == SINK_NODE) {
      link = node->link;
      node->link = ALLOC(gate_link_t, 1);
      *(node->link) = *link;
      if (link->node == NIL(node_t))
        continue; /* was a dummy tree to start with. Nothing to do */
      assert(MAP(link->node)->pin_info != NIL(fanin_fanout_t));
      node->link->node = NIL(node_t);
      if (link->node->type == PRIMARY_OUTPUT) {
        node->link->pin = MAP(link->node)->pin_info[0].fanout_index;
      } else {
        node->link->pin = MAP(link->node)->pin_info[link->pin].fanout_index;
      }
      assert(node->link->pin != -1);
    }
  }
}

void fanout_tree_copy(to, from) array_t *to;
array_t *from;
{
  int i;
  fanout_node_t *node;

  for (i = 0; i < array_n(from); i++) {
    node = array_fetch_p(fanout_node_t, from, i);
    array_insert(fanout_node_t, to, i, *node);
  }
}

/* exported to avoid having to do it several times */
/* probably should be made internal to that file */

void fanout_tree_add_edges(array) array_t *array;
{
  int i, j, next_i;
  fanout_node_t *node;

  for (i = 0; i < array_n(array); i = next_i) {
    node = array_fetch_p(fanout_node_t, array, i);
    assert(node->type == BUFFER_NODE);
    node->type = SOURCE_NODE;
    node->children = array_alloc(fanout_node_t *, 0);
    next_i = i + 1;
    for (j = 0; j < node->arity; j++) {
      fanout_node_t *child;
      child = array_fetch_p(fanout_node_t, array, next_i);
      array_insert_last(fanout_node_t *, node->children, child);
      next_i = add_edges_rec(array, next_i);
    }
    assert(node->arity == array_n(node->children));
  }
}

/* make sure the tree is consistent and the costs are as announced */

void fanout_tree_check(array, cost, fanout_info) array_t *array;
fanout_cost_t *cost;
opt_array_t *fanout_info;
{
  array_t *trees = fanout_tree_get_roots(array);
  n_gates = fanout_delay_get_n_gates();
  check_polarities(trees, fanout_info);
  check_required(trees, cost->slack);
  check_area(trees, cost->area);
  array_free(trees);
}

/* try to find minimum area tree which meets the requirements */
/* or if none, the one closest to meeting required times */

static int peephole_global_debug;
void fanout_tree_peephole_optimize(array, cost, fanout_info,
                                   debug) array_t *array;
fanout_cost_t *cost;
opt_array_t *fanout_info;
int debug;
{
  array_t *nodes;
  array_t *trees = fanout_tree_get_roots(array);

  peephole_global_debug = debug;
  fanout_tree_alloc_peephole(trees);
  fanout_tree_compute_arrival_times(trees);
  nodes = get_forest_in_bottom_up_order(trees);
  if (peephole_optimize(trees, nodes, cost)) {
    build_peephole_trees(trees);
    fanout_tree_check(array, cost, fanout_info);
  }
  fanout_tree_compute_arrival_times(trees);
  fanout_tree_remove_unnecessary_buffers(trees, cost);
  fanout_tree_check(array, cost, fanout_info);
  fanout_free_peephole(nodes);
  array_free(nodes);
  array_free(trees);
}

/* finally build the tree */

void fanout_tree_build(array) array_t *array;
{
  array_t *trees = fanout_tree_get_roots(array);
  do_build_tree(trees, array);
  free_unused_sources(trees);
  array_free(trees);
}

/* for sink nodes (i.e. leaves) */

void fanout_tree_insert_sink(array, link) array_t *array;
gate_link_t *link;
{
  fanout_node_t node;
  node = DEFAULT_FANOUT_NODE;
  node.link = link;
  array_insert_last(fanout_node_t, array, node);
}

/* to be used for source or intermediate nodes */
/* arity can't be 0 (not a leaf) */

void fanout_tree_insert_gate(array, gate_index, arity) array_t *array;
int gate_index;
int arity;
{
  fanout_node_t node;
  assert(arity > 0);
  node = DEFAULT_FANOUT_NODE;
  node.type = BUFFER_NODE;
  node.gate_index = gate_index;
  node.arity = arity;
  array_insert_last(fanout_node_t, array, node);
}

/* INTERNAL INTERFACE */

static int add_edges_rec(array, i) array_t *array;
int i;
{
  int next_i, j;
  fanout_node_t *node;

  node = array_fetch_p(fanout_node_t, array, i);
  node->children = array_alloc(fanout_node_t *, 0);
  next_i = i + 1;
  switch (node->type) {
  case SINK_NODE:
    break;
  case BUFFER_NODE:
    for (j = 0; j < node->arity; j++) {
      fanout_node_t *child;
      child = array_fetch_p(fanout_node_t, array, next_i);
      array_insert_last(fanout_node_t *, node->children, child);
      next_i = add_edges_rec(array, next_i);
    }
    break;
  default:
    fail("unexpected node type in do_build_rooted_tree");
    break;
  }
  assert(node->arity == array_n(node->children));
  return next_i;
}

static array_t *fanout_tree_get_roots(array) array_t *array;
{
  int i;
  fanout_node_t *node;
  array_t *roots = array_alloc(fanout_node_t *, 0);
  for (i = 0; i < array_n(array); i++) {
    node = array_fetch_p(fanout_node_t, array, i);
    if (node->type == SOURCE_NODE)
      array_insert_last(fanout_node_t *, roots, node);
  }
  return roots;
}

static void do_build_tree(trees, array) array_t *trees;
array_t *array;
{
  int i;
  fanout_node_t *source;

  for (i = 0; i < array_n(trees); i++) {
    source = array_fetch(fanout_node_t *, trees, i);
    source->node = fanout_delay_get_source_node(source->gate_index);
    gate_link_delete_all(source->node);
    do_build_tree_rec(source, array);
  }
}

static void do_build_tree_rec(source, array) fanout_node_t *source;
array_t *array;
{
  int i;
  fanout_node_t *fanout_node;

  assert(array_n(source->children) > 0);
  for (i = 0; i < array_n(source->children); i++) {
    fanout_node = array_fetch(fanout_node_t *, source->children, i);
    switch (fanout_node->type) {
    case SINK_NODE:
      virtual_network_add_to_gate_link(source->node, fanout_node->link);
      break;
    case BUFFER_NODE:
      fanout_node->node = create_buffer(source->node, fanout_node->gate_index);
      do_build_tree_rec(fanout_node, array);
      break;
    default:
      fail("unexpected node type in do_build_rooted_tree");
      break;
    }
  }
  virtual_delay_compute_node_required_time(source->node);
}

static node_t *create_buffer(source, gate_index) node_t *source;
int gate_index;
{
  gate_link_t new_link;
  int phase;
  node_t *new_node = node_alloc();

  /*
    assert(is_inverter(n_gates, gate_index));
    node_replace(new_node, node_literal(source, 0));
  */
  phase = (is_inverter(n_gates, gate_index)) ? 0 : 1;
  node_replace(new_node, node_literal(source, phase));
  network_add_node(node_network(source), new_node);
  fanout_log_register_node(new_node);
  map_alloc(new_node);
  MAP(new_node)->gate = fanout_delay_get_gate(gate_index);
  MAP(new_node)->ninputs = 1;
  MAP(new_node)->save_binding = ALLOC(node_t *, 1);
  MAP(new_node)->save_binding[0] = source;
  MAP(new_node)->map_arrival = PLUS_INFINITY;
  new_link.node = new_node;
  new_link.pin = 0;
  new_link.load = fanout_delay_get_buffer_load(gate_index);
  new_link.required = MINUS_INFINITY;
  gate_link_put(source, &new_link);
  return new_node;
}

/* the rest is to perform consistency checks on the input */

static void check_polarities(trees, fanout_info) array_t *trees;
opt_array_t *fanout_info;
{
  int i, p;
  gate_link_t *link;
  fanout_node_t *source;
  st_table *sink_table = st_init_table(st_ptrcmp, st_ptrhash);

  foreach_polarity(p) {
    for (i = 0; i < fanout_info[p].n_elts; i++) {
      link = array_fetch(gate_link_t *, fanout_info[p].required, i);
      st_insert(sink_table, (char *)link, (char *)p);
    }
  }
  for (i = 0; i < array_n(trees); i++) {
    int source_polarity;
    source = array_fetch(fanout_node_t *, trees, i);
    source_polarity = fanout_delay_get_source_polarity(source->gate_index);
    check_polarity_rec(source, source_polarity, sink_table);
  }
  assert(st_count(sink_table) == 0);
  st_free_table(sink_table);
}

static void check_polarity_rec(source, source_polarity,
                               sink_table) fanout_node_t *source;
int source_polarity;
st_table *sink_table;
{
  int i;
  int sink_polarity;
  int buffer_polarity;
  fanout_node_t *fanout_node;
  char *dummy;

  for (i = 0; i < array_n(source->children); i++) {
    fanout_node = array_fetch(fanout_node_t *, source->children, i);
    switch (fanout_node->type) {
    case SINK_NODE:
      assert(
          st_lookup_int(sink_table, (char *)fanout_node->link, &sink_polarity));
      assert(sink_polarity == source_polarity);
      st_delete(sink_table, (char **)&fanout_node->link, &dummy);
      break;
    case BUFFER_NODE:
      buffer_polarity =
          (fanout_delay_get_buffer_polarity(fanout_node->gate_index) == POLAR_X)
              ? source_polarity
              : POLAR_INV(source_polarity);
      check_polarity_rec(fanout_node, buffer_polarity, sink_table);
      break;
    default:
      fail("unexpected node type in check_polarity_rec");
      break;
    }
  }
}

static void check_required(trees, ref_required) array_t *trees;
delay_time_t ref_required;
{
  int i;
  fanout_node_t *source;
  delay_time_t required;

  required = PLUS_INFINITY;
  for (i = 0; i < array_n(trees); i++) {
    source = array_fetch(fanout_node_t *, trees, i);
    source->required = check_required_rec(source);
    SETMIN(required, required, source->required);
  }
  assert(IS_EQUAL(ref_required, required));
}

/* Exported to compute the delay thru a fanout_tree for the top_down alg. */
delay_time_t map_compute_fanout_tree_req_time(tree) array_t *tree;
{
  fanout_node_t *source;
  delay_time_t required;

  add_edges_rec(tree, 0); /* Build the tree */
  source = array_fetch_p(fanout_node_t, tree, 0);
  required = check_required_rec(source);
  return required;
}

static delay_time_t check_required_rec(source) fanout_node_t *source;
{
  int i;
  delay_time_t required, local_required;
  double load, local_load;
  int n_fanouts;
  fanout_node_t *fanout_node;

  required = PLUS_INFINITY;
  load = 0.0;
  n_fanouts = array_n(source->children);
  for (i = 0; i < array_n(source->children); i++) {
    fanout_node = array_fetch(fanout_node_t *, source->children, i);
    switch (fanout_node->type) {
    case SINK_NODE:
      local_required = fanout_node->required = fanout_node->link->required;
      local_load = fanout_node->link->load;
      break;
    case BUFFER_NODE:
      fanout_node->required = check_required_rec(fanout_node);
      local_required = fanout_delay_backward_intrinsic(fanout_node->required,
                                                       fanout_node->gate_index);
      local_load = fanout_delay_get_buffer_load(fanout_node->gate_index);
      break;
    default:
      fail("unexpected node type in check_polarity_rec");
      break;
    }
    load += local_load;
    SETMIN(required, required, local_required);
  }
  load += map_compute_wire_load(n_fanouts);
#ifdef DEBUG
  source->save_required = required;
#endif
  return fanout_delay_backward_load_dependent(required, source->gate_index,
                                              load);
}

static void check_area(trees, ref_area) array_t *trees;
double ref_area;
{
  int i;
  array_t *nodes;
  fanout_node_t *node;
  double area = 0.0;

  nodes = get_forest_in_bottom_up_order(trees);
  for (i = 0; i < array_n(nodes); i++) {
    node = array_fetch(fanout_node_t *, nodes, i);
    if (node->type == BUFFER_NODE || node->type == SOURCE_NODE)
      area += fanout_delay_get_area(node->gate_index);
  }
  assert(FP_EQUAL(area, ref_area));
  array_free(nodes);
}

/* FOR DEBUGGING: please call "add_edges" first */

void print_fanout_tree(trees, array) array_t *trees;
array_t *array;
{
  int i;
  fanout_node_t *source;

  for (i = 0; i < array_n(trees); i++) {
    int source_polarity;
    source = array_fetch(fanout_node_t *, trees, i);
    source_polarity = fanout_delay_get_source_polarity(source->gate_index);
    source->node = fanout_delay_get_source_node(source->gate_index);
    (void)fprintf(sisout, "source(%s,p(%d),", source->node->name,
                  source_polarity);
#ifdef DEBUG
    (void)fprintf(sisout, "r(%2.3f,%2.3f)", source->save_required.rise,
                  source->save_required.fall);
#endif
    (void)fprintf(sisout, "s(%2.3f,%2.3f))\n", source->required.rise,
                  source->required.fall);
    print_tree_rec(source, source_polarity, array, 4);
  }
}

static void print_tree_rec(source, source_polarity, array,
                           n_tabs) fanout_node_t *source;
int source_polarity;
array_t *array;
int n_tabs;
{
  int i;
  int buffer_polarity;
  lib_gate_t *gate;
  fanout_node_t *fanout_node;
  delay_time_t required;

  for (i = 0; i < array_n(source->children); i++) {
    fanout_node = array_fetch(fanout_node_t *, source->children, i);
    switch (fanout_node->type) {
    case SINK_NODE:
      print_tabs(n_tabs);
      if (fanout_node->link->node == NIL(node_t)) {
        (void)fprintf(sisout, "sink(-%d-,p(%d),", fanout_node->link->pin,
                      source_polarity);
      } else {
        (void)fprintf(sisout, "sink(%s,p(%d),", fanout_node->link->node->name,
                      source_polarity);
      }
      (void)fprintf(sisout, "l(%2.3f),", fanout_node->link->load);
      (void)fprintf(sisout, "r(%2.3f,%2.3f))\n", fanout_node->required.rise,
                    fanout_node->required.fall);
      break;
    case BUFFER_NODE:
      print_tabs(n_tabs);
      gate = fanout_delay_get_gate(fanout_node->gate_index);
      buffer_polarity =
          (fanout_delay_get_buffer_polarity(fanout_node->gate_index) == POLAR_X)
              ? source_polarity
              : POLAR_INV(source_polarity);
      (void)fprintf(sisout, "buffer(%s,p(%d),", gate->name, buffer_polarity);
      (void)fprintf(sisout, "l(%2.3f),",
                    fanout_delay_get_buffer_load(fanout_node->gate_index));
      required = fanout_delay_backward_intrinsic(fanout_node->required,
                                                 fanout_node->gate_index);
      (void)fprintf(sisout, "r(%2.3f,%2.3f))\n", required.rise, required.fall);
      print_tree_rec(fanout_node, buffer_polarity, array, n_tabs + 4);
      break;
    default:
      fail("unexpected node type in check_polarity_rec");
      break;
    }
  }
}

static void print_tabs(n_tabs) int n_tabs;
{
  int i;

  assert(n_tabs >= 0);
  for (i = 0; i < n_tabs; i++)
    (void)fprintf(sisout, " ");
}

static void free_unused_sources(trees) array_t *trees;
{
  int i, source_index;
  node_t *node;
  char *dummy;
  fanout_node_t *source;
  st_table *used_sources = st_init_table(st_ptrcmp, st_ptrhash);

  for (i = 0; i < array_n(trees); i++) {
    source = array_fetch(fanout_node_t *, trees, i);
    source->node = fanout_delay_get_source_node(source->gate_index);
    st_insert(used_sources, (char *)source->node, NIL(char));
  }
  foreach_source(n_gates, source_index) {
    node = fanout_delay_get_source_node(source_index);
    if (!st_lookup(used_sources, (char *)node, &dummy))
      virtual_network_remove_node(node, 1);
  }
  st_free_table(used_sources);
}

static array_t *get_forest_in_bottom_up_order(trees) array_t *trees;
{
  int i;
  fanout_node_t *source;
  array_t *nodes = array_alloc(fanout_node_t *, 0);

  for (i = 0; i < array_n(trees); i++) {
    source = array_fetch(fanout_node_t *, trees, i);
    get_forest_in_bottom_up_order_rec(source, nodes);
    array_insert_last(fanout_node_t *, nodes, source);
  }
  return nodes;
}

static void get_forest_in_bottom_up_order_rec(source,
                                              nodes) fanout_node_t *source;
array_t *nodes;
{
  int i;
  fanout_node_t *fanout_node;

  for (i = 0; i < array_n(source->children); i++) {
    fanout_node = array_fetch(fanout_node_t *, source->children, i);
    switch (fanout_node->type) {
    case SINK_NODE:
      array_insert_last(fanout_node_t *, nodes, fanout_node);
      break;
    case BUFFER_NODE:
      get_forest_in_bottom_up_order_rec(fanout_node, nodes);
      array_insert_last(fanout_node_t *, nodes, fanout_node);
      break;
    default:
      fail("unexpected node type");
      break;
    }
  }
}

typedef struct peep_entry_struct peep_entry_t;
struct peep_entry_struct {
  delay_time_t
      required; /* before intrinsic, after load dependent (except for sinks) */
  delay_time_t
      arrival; /* after intrinsic, before load dependent (except for sinks) */
  double load;
  double area;
  int buffer_index;
  array_t *children_incr;
};

typedef struct peep_seq_struct peep_seq_t;
struct peep_seq_struct {
  delay_time_t required;
  double area;
  int child_index;
};

static int peephole_optimize(trees, nodes, cost) array_t *trees;
array_t *nodes;
fanout_cost_t *cost;
{
  int i, j;
  fanout_node_t *node;
  peep_entry_t *entry;
  fanout_cost_t peephole_cost;

  for (i = 0; i < array_n(nodes); i++) {
    node = array_fetch(fanout_node_t *, nodes, i);
    if (node->type == SINK_NODE)
      continue;
    for (j = 0; j < array_n(node->peephole); j++) {
      entry = array_fetch_p(peep_entry_t, node->peephole, j);
      peephole_optimize_node(node, entry);
    }
  }
  if (peephole_is_better(trees, cost, &peephole_cost)) {
    *cost = peephole_cost;
    return 1;
  }
  return 0;
}

/* BEGIN OPTIMAL SIZING ALGORITHM FOR DELAY MINIMIZATION OF FANOUT TREES */

static void peephole_optimize_node(node, entry) fanout_node_t *node;
peep_entry_t *entry;
{
  int i;
  array_t *log;
  int critical_child;
  int *current_incr;
  fanout_cost_t cost;
  peep_seq_t seq_entry;

  log = array_alloc(peep_seq_t, 0);
  current_incr = ALLOC(int, array_n(node->children));
  for (i = 0; i < array_n(node->children); i++)
    current_incr[i] = 0;

  for (;;) {
    if (!incr_critical_child(node, current_incr, &critical_child))
      break;
    current_incr[critical_child]++;
    seq_entry.child_index = critical_child;
    compute_current_cost(node, entry->buffer_index, current_incr, &cost);
    seq_entry.required = cost.slack;
    seq_entry.area = cost.area;
    array_insert_last(peep_seq_t, log, seq_entry);
  }

  select_best_log_entry(node, log, entry);
  FREE(current_incr);
  array_free(log);
}

static int incr_critical_child(node, current_incr,
                               critical_child_index) fanout_node_t *node;
int *current_incr;
int *critical_child_index;
{
  int i;
  delay_time_t required, local_required;
  fanout_node_t *child, *critical_child;
  peep_entry_t *child_entry;

  *critical_child_index = -1;

  /* find critical child */
  required = PLUS_INFINITY;
  for (i = 0; i < array_n(node->children); i++) {
    child = array_fetch(fanout_node_t *, node->children, i);
    child_entry = array_fetch_p(peep_entry_t, child->peephole, current_incr[i]);
    local_required = child_entry->required;
    if (child_entry->buffer_index != -1)
      local_required = fanout_delay_backward_intrinsic(
          local_required, child_entry->buffer_index);
    if (GETMIN(required) > GETMIN(local_required)) {
      required = local_required;
      *critical_child_index = i;
    }
  }

  /* check whether can increment critical child */
  critical_child =
      array_fetch(fanout_node_t *, node->children, *critical_child_index);
  return (current_incr[*critical_child_index] <
          array_n(critical_child->peephole) - 1);
}

static void compute_current_cost(node, source_index, current_incr, cost)
    fanout_node_t *node; /* some node in the fanout tree */
int source_index;        /* index of a possible gate at that node */
int *current_incr;       /* distribution of size increments to children */
fanout_cost_t *cost;     /* where the results are put: area, required */
{
  int i;
  delay_time_t required, local_required;
  double load, area;
  fanout_node_t *child;
  peep_entry_t *child_entry;

  required = PLUS_INFINITY;
  load = 0.0;
  area = 0.0;
  for (i = 0; i < array_n(node->children); i++) {
    child = array_fetch(fanout_node_t *, node->children, i);
    child_entry = array_fetch_p(peep_entry_t, child->peephole, current_incr[i]);
    local_required = child_entry->required;
    if (child_entry->buffer_index != -1)
      local_required = fanout_delay_backward_intrinsic(
          local_required, child_entry->buffer_index);
    SETMIN(required, required, local_required);
    area += child_entry->area;
    load += child_entry->load;
  }
  load += map_compute_wire_load(array_n(node->children));
  cost->slack =
      fanout_delay_backward_load_dependent(required, source_index, load);
  cost->area = area + fanout_delay_get_area(source_index);
}

static void select_best_log_entry(node, log, entry) fanout_node_t *node;
array_t *log;
peep_entry_t *entry;
{
  int i;
  int best_log_index;
  fanout_cost_t best_cost, cost;
  int *incr;
  peep_seq_t *seq_entry;

  /* first compute cost with all sizes minimum */
  best_log_index = -1;
  incr = ALLOC(int, array_n(node->children));
  for (i = 0; i < array_n(node->children); i++)
    incr[i] = 0;
  compute_current_cost(node, entry->buffer_index, incr, &best_cost);
  SETSUB(best_cost.slack, best_cost.slack, entry->arrival);

  /* then select best cost */
  for (i = 0; i < array_n(log); i++) {
    seq_entry = array_fetch_p(peep_seq_t, log, i);
    SETSUB(cost.slack, seq_entry->required, entry->arrival);
    cost.area = seq_entry->area;
    if (!fanout_opt_is_better_cost(&best_cost, &cost)) {
      best_cost = cost;
      best_log_index = i;
    }
  }

  /* run the log up to the best cost entry */
  for (i = 0; i <= best_log_index; i++) {
    seq_entry = array_fetch_p(peep_seq_t, log, i);
    incr[seq_entry->child_index]++;
  }

  /* save the best size incr assignment to children */
  for (i = 0; i < array_n(node->children); i++)
    array_insert(int, entry->children_incr, i, incr[i]);

  /* compute the best cost */
  compute_current_cost(node, entry->buffer_index, incr, &best_cost);
  FREE(incr);
  entry->required = best_cost.slack;
  entry->area = best_cost.area;
}

/* END OPTIMAL SIZING ALGORITHM FOR DELAY MINIMIZATION OF FANOUT TREES */

static void build_peephole_trees(trees) array_t *trees;
{
  int i;
  fanout_node_t *source;
  peep_entry_t *entry;

  for (i = 0; i < array_n(trees); i++) {
    source = array_fetch(fanout_node_t *, trees, i);
    entry = array_fetch_p(peep_entry_t, source->peephole, 0);
    build_peephole_tree_rec(source, entry);
    source->required = entry->required;
    source->area = entry->area;
  }
}

static void build_peephole_tree_rec(source, entry) fanout_node_t *source;
peep_entry_t *entry;
{
  int i;
  int child_incr;
  peep_entry_t *child_entry;
  fanout_node_t *fanout_node;

  for (i = 0; i < array_n(source->children); i++) {
    fanout_node = array_fetch(fanout_node_t *, source->children, i);
    switch (fanout_node->type) {
    case SINK_NODE:
      break;
    case BUFFER_NODE:
      child_incr = array_fetch(int, entry->children_incr, i);
      child_entry =
          array_fetch_p(peep_entry_t, fanout_node->peephole, child_incr);
      build_peephole_tree_rec(fanout_node, child_entry);
      fanout_node->gate_index = child_entry->buffer_index;
      fanout_node->required = child_entry->required;
      fanout_node->area = child_entry->area;
      break;
    default:
      fail("unexpected node type in do_build_rooted_tree");
      break;
    }
  }
}

static int peephole_is_better(trees, cost, peephole_cost) array_t *trees;
fanout_cost_t *cost;
fanout_cost_t *peephole_cost;
{
  int i;
  fanout_node_t *node;
  peep_entry_t *entry;
  int peephole_is_better_p;

  peephole_cost->area = 0.0;
  peephole_cost->slack = PLUS_INFINITY;
  for (i = 0; i < array_n(trees); i++) {
    node = array_fetch(fanout_node_t *, trees, i);
    entry = array_fetch_p(peep_entry_t, node->peephole, 0);
    peephole_cost->area += entry->area;
    SETMIN(peephole_cost->slack, peephole_cost->slack, entry->required);
  }
  peephole_is_better_p = fanout_opt_is_better_cost(peephole_cost, cost);
  if (peephole_global_debug == -1 ||
      (peephole_global_debug && !peephole_is_better_p)) {
    (void)fprintf(sisout, "before peephole: a(%2.2f) s(%2.4f %2.4f)\t",
                  cost->area, cost->slack.rise, cost->slack.fall);
    (void)fprintf(sisout, "after peephole: a(%2.2f) s(%2.4f %2.4f)\n",
                  peephole_cost->area, peephole_cost->slack.rise,
                  peephole_cost->slack.fall);
  }
  return peephole_is_better_p;
}

/* ROUTINES FOR INITIALIZATION AND DEALLOCATION OF PEEPHOLE AND ARRIVAL FIELDS
 */

static void fanout_tree_alloc_peephole(trees) array_t *trees;
{
  int i;
  fanout_node_t *source;

  for (i = 0; i < array_n(trees); i++) {
    source = array_fetch(fanout_node_t *, trees, i);
    peephole_init_source(source);
    alloc_peephole_rec(source);
  }
}

static void alloc_peephole_rec(source) fanout_node_t *source;
{
  int i;
  fanout_node_t *fanout_node;

  for (i = 0; i < array_n(source->children); i++) {
    fanout_node = array_fetch(fanout_node_t *, source->children, i);
    switch (fanout_node->type) {
    case SINK_NODE:
      peephole_init_sink(fanout_node);
      break;
    case BUFFER_NODE:
      peephole_init_buffer(fanout_node);
      alloc_peephole_rec(fanout_node);
      break;
    default:
      fail("unexpected node type in alloc_peephole_rec");
      break;
    }
  }
}

static void peephole_init_sink(node) fanout_node_t *node;
{
  peep_entry_t entry;

  node->peephole = array_alloc(peep_entry_t, 1);
  entry.required = node->link->required;
  entry.arrival = PLUS_INFINITY;
  entry.load = node->link->load;
  entry.area = 0.0;
  entry.buffer_index = -1;
  entry.children_incr = array_alloc(int, 0);
  array_insert(peep_entry_t, node->peephole, 0, entry);
}

static void peephole_init_source(node) fanout_node_t *node;
{
  peep_entry_t entry;

  node->peephole = array_alloc(peep_entry_t, 1);
  entry.required = MINUS_INFINITY;
  entry.load = 0.0;
  entry.area = INFINITY;
  entry.arrival = ZERO_DELAY;
  entry.buffer_index = node->gate_index;
  entry.children_incr = array_alloc(int, 0);
  array_insert(peep_entry_t, node->peephole, 0, entry);
}

static void peephole_init_buffer(node) fanout_node_t *node;
{
  int i;
  int from, to;
  peep_entry_t entry;

  assert(is_buffer(n_gates, node->gate_index));
  if (is_inverter(n_gates, node->gate_index)) {
    from = n_gates.n_pos_buffers;
    to = n_gates.n_neg_buffers;
  } else {
    from = 0;
    to = n_gates.n_pos_buffers;
  }
  node->peephole = array_alloc(peep_entry_t, to - from);
  for (i = 0; i < to - from; i++) {
    entry.buffer_index = i + from;
    entry.load = fanout_delay_get_buffer_load(entry.buffer_index);
    entry.required = MINUS_INFINITY;
    entry.area = INFINITY;
    entry.arrival = PLUS_INFINITY;
    entry.children_incr = array_alloc(int, 0);
    array_insert(peep_entry_t, node->peephole, i, entry);
  }
}

static void fanout_free_peephole(nodes) array_t *nodes;
{
  int i, j;
  fanout_node_t *node;

  for (i = 0; i < array_n(nodes); i++) {
    node = array_fetch(fanout_node_t *, nodes, i);
    for (j = 0; j < array_n(node->peephole); j++) {
      peep_entry_t *entry = array_fetch_p(peep_entry_t, node->peephole, j);
      array_free(entry->children_incr);
    }
    if (node->remove_opt) {
      cleanup_remove_opt_node(node);
    }
    array_free(node->peephole);
  }
}

/* COMPUTING ARRIVAL TIMES */

static void fanout_tree_compute_arrival_times(trees) array_t *trees;
{
  int i;
  double load;
  fanout_node_t *source;
  peep_entry_t *entry;

  for (i = 0; i < array_n(trees); i++) {
    source = array_fetch(fanout_node_t *, trees, i);
    entry = array_fetch_p(peep_entry_t, source->peephole, 0);
    load = fanout_tree_get_node_load(source);
    source->arrival = entry->arrival = ZERO_DELAY;
    compute_arrival_times_rec(source, load);
  }
}

static void compute_arrival_times_rec(source,
                                      source_load) fanout_node_t *source;
double source_load;
{
  int i, j;
  double fanout_load;
  delay_time_t fanout_arrival, local_arrival;
  fanout_node_t *fanout_node;
  peep_entry_t *entry;

  local_arrival = fanout_delay_forward_load_dependent(
      source->arrival, source->gate_index, source_load);
  for (i = 0; i < array_n(source->children); i++) {
    fanout_node = array_fetch(fanout_node_t *, source->children, i);
    switch (fanout_node->type) {
    case SINK_NODE:
      entry = array_fetch_p(peep_entry_t, fanout_node->peephole, 0);
      fanout_node->arrival = entry->arrival = local_arrival;
      break;
    case BUFFER_NODE:
      for (j = 0; j < array_n(fanout_node->peephole); j++) {
        entry = array_fetch_p(peep_entry_t, fanout_node->peephole, j);
        fanout_load = source_load -
                      fanout_delay_get_buffer_load(fanout_node->gate_index) +
                      entry->load;
        fanout_arrival = fanout_delay_forward_load_dependent(
            source->arrival, source->gate_index, fanout_load);
        fanout_arrival =
            fanout_delay_forward_intrinsic(fanout_arrival, entry->buffer_index);
        entry->arrival = fanout_arrival;
      }
      fanout_node->arrival = fanout_delay_forward_intrinsic(
          local_arrival, fanout_node->gate_index);
      fanout_load = fanout_tree_get_node_load(fanout_node);
      compute_arrival_times_rec(fanout_node, fanout_load);
      break;
    default:
      fail("unexpected node type in alloc_peephole_rec");
      break;
    }
  }
}

static double fanout_tree_get_node_load(node) fanout_node_t *node;
{
  int i;
  double load = 0.0;
  fanout_node_t *fanout_node;

  if (node->type != SOURCE_NODE && node->type != BUFFER_NODE)
    fail("unexpected node type in fanout_tree_get_node_load");

  for (i = 0; i < array_n(node->children); i++) {
    fanout_node = array_fetch(fanout_node_t *, node->children, i);
    switch (fanout_node->type) {
    case SINK_NODE:
      load += fanout_node->link->load;
      break;
    case BUFFER_NODE:
      load += fanout_delay_get_buffer_load(fanout_node->gate_index);
      break;
    default:
      fail("unexpected node type in alloc_peephole_rec");
      break;
    }
  }
  return load + map_compute_wire_load(array_n(node->children));
}

/* visit the tree from sinks to root */
/* each time  */

static void fanout_tree_remove_unnecessary_buffers(trees, cost) array_t *trees;
fanout_cost_t *cost;
{
  int i;
  fanout_node_t *source;

  for (i = 0; i < array_n(trees); i++) {
    source = array_fetch(fanout_node_t *, trees, i);
    source->polarity = fanout_delay_get_source_polarity(source->gate_index);
    remove_unnecessary_buffers_rec(source);
    remove_buffer_process_source(source);
  }
  cost->slack = PLUS_INFINITY;
  cost->area = 0.0;
  for (i = 0; i < array_n(trees); i++) {
    source = array_fetch(fanout_node_t *, trees, i);
    SETMIN(cost->slack, cost->slack, source->required);
    cost->area += source->area;
  }
}

static void remove_unnecessary_buffers_rec(source) fanout_node_t *source;
{
  int i;
  fanout_node_t *fanout_node;

  for (i = 0; i < array_n(source->children); i++) {
    fanout_node = array_fetch(fanout_node_t *, source->children, i);
    switch (fanout_node->type) {
    case SINK_NODE:
      fanout_node->polarity = source->polarity;
      remove_buffer_process_sink(fanout_node);
      break;
    case BUFFER_NODE:
      fanout_node->polarity =
          (fanout_delay_get_buffer_polarity(fanout_node->gate_index) == POLAR_X)
              ? source->polarity
              : POLAR_INV(source->polarity);
      remove_unnecessary_buffers_rec(fanout_node);
      remove_buffer_process_buffer(fanout_node);
      break;
    default:
      fail("unexpected node type in alloc_peephole_rec");
      break;
    }
  }
}

typedef struct remove_opt_struct remove_opt_t;
struct remove_opt_struct {
  delay_time_t required; /* aggregate for node & polarity; after intrinsic */
  double load;           /* aggregate load, not including wire_load */
  st_table *links;       /* list of nodes to be implemented */
};

static void remove_buffer_process_sink(node) fanout_node_t *node;
{
  int p;
  remove_opt_t entry[2];

  foreach_polarity(p) {
    entry[p].load = 0.0;
    entry[p].required = PLUS_INFINITY;
    entry[p].links = st_init_table(st_ptrcmp, st_ptrhash);
  }
  p = node->polarity;
  entry[p].required = node->link->required;
  entry[p].load = node->link->load;
  st_insert(entry[p].links, (char *)node, NIL(char));
  node->remove_opt = array_alloc(remove_opt_t, 2);
  foreach_polarity(p) {
    array_insert(remove_opt_t, node->remove_opt, p, entry[p]);
  }
  node->load = node->link->load;
  node->required = node->link->required;
  node->area = 0.0;
}

/* remove_opt array is only used for flattening, not for merging */
/* entring in this routine, we first check whether any child */
/* does not have a remove_opt entry (in continue_flattening) */
/* if all children have a remove_opt entry, we go ahead and try to flatten */
/* if it succeeds, we remove the remove_opt array of the children */
/* update the cost info of the node, and return with the node */
/* annotated with a new remove_opt array */
/* if we cannot flatten, we remove the remove_opt entries of the children */
/* and try to merge the nodes greedily. If that succeeds, we recompute */
/* the cost info associated with the node; otherwise we give up and return */
/* If we cannot flatten, we do not allocate a remove_opt entry in any case */
/* so there is no need to call cleanup_remove_opt_node */

static void remove_buffer_process_buffer(node) fanout_node_t *node;
{
  int was_flattened;
  compute_buffer_node_cost(node);
  if (continue_flattening(node)) {
    compute_flatten_info(node);
    was_flattened = can_flatten_subtree(node);
    cleanup_remove_opt_children_entries(node);
    if (was_flattened) {
      compute_buffer_node_cost(node);
      return;
    }
    cleanup_remove_opt_node(node);
  }
  cleanup_remove_opt_children_entries(node);
  if (can_merge_subtrees(node)) {
    compute_buffer_node_cost(node);
  }
}

/* a no answer means that one of subtree cannot be */
/* optimized any more; in that case, give up optimizing */

static int continue_flattening(node) fanout_node_t *node;
{
  int i;
  fanout_node_t *child;

  for (i = 0; i < array_n(node->children); i++) {
    child = array_fetch(fanout_node_t *, node->children, i);
    if (child->remove_opt == NIL(array_t))
      return 0;
  }
  return 1;
}

/* gather on the node all the children of its children */
/* those are not necessarily leaves */
/* leaves (or sinks) are set up initially to be children of themselves */
/* so there is no need for special case */

static void compute_flatten_info(node) fanout_node_t *node;
{
  int i, p;
  fanout_node_t *child;
  remove_opt_t entry[2];
  remove_opt_t *child_entry;
  char *key, *value;
  st_generator *gen;

  foreach_polarity(p) {
    entry[p].load = 0.0;
    entry[p].required = PLUS_INFINITY;
    entry[p].links = st_init_table(st_ptrcmp, st_ptrhash);
  }
  for (i = 0; i < array_n(node->children); i++) {
    child = array_fetch(fanout_node_t *, node->children, i);
    foreach_polarity(p) {
      child_entry = array_fetch_p(remove_opt_t, child->remove_opt, p);
      SETMIN(entry[p].required, entry[p].required, child_entry->required);
      entry[p].load += child_entry->load;
      st_foreach_item(child_entry->links, gen, &key, &value) {
        st_insert(entry[p].links, key, value);
      }
    }
  }
  node->remove_opt = array_alloc(remove_opt_t, 2);
  foreach_polarity(p) {
    array_insert(remove_opt_t, node->remove_opt, p, entry[p]);
  }
}

/* check whether by flattening all nodes under "node" we can obtain a better
 * cost */
/* costs are compared on basis of area if meet delay constraint (node->arrival)
 */
/* or on basis of delay otherwise.*/
/* no_opt_cost is simply the cost at the node right now. */
/* the best_new_cost is the cost obtained by collapsing all leaves onto "node"
 */
/* if both polarities are present, one inverter is introduced and optimally
 * sized */

static int can_flatten_subtree(node) fanout_node_t *node;
{
  int best_gate_index;
  delay_time_t slack;
  fanout_cost_t cost, current_cost;

  if (already_flat_subtree(node))
    return 1;
  compute_no_opt_cost(node, &current_cost);
  compute_best_new_cost(node, &cost, &best_gate_index);
  if (fanout_opt_is_better_cost(&current_cost, &cost))
    return 0;
  update_children_entries(node, best_gate_index);
  SETSUB(slack, node->required, node->arrival);
  assert(FP_EQUAL(node->area, cost.area));
  assert(IS_EQUAL(slack, cost.slack));
  return 1;
}

static void compute_no_opt_cost(node, cost) fanout_node_t *node;
fanout_cost_t *cost;
{
  SETSUB(cost->slack, node->required, node->arrival);
  cost->area = node->area;
}

static void compute_best_new_cost(node, cost,
                                  best_gate_index) fanout_node_t *node;
fanout_cost_t *cost;
int *best_gate_index;
{
  int p, q;
  remove_opt_t *entry[2];

  foreach_polarity(p) {
    entry[p] = array_fetch_p(remove_opt_t, node->remove_opt, p);
  }
  p = node->polarity;
  q = POLAR_INV(p);
  if (st_count(entry[q]->links) == 0) {
    delay_time_t required;
    double load = entry[p]->load;
    load += map_compute_wire_load(st_count(entry[p]->links));
    required = fanout_delay_backward_load_dependent(entry[p]->required,
                                                    node->gate_index, load);
    SETSUB(cost->slack, required, node->arrival);
    cost->area = fanout_delay_get_area(node->gate_index);
    *best_gate_index = -1;
  } else {
    fanout_cost_t local_cost;
    int gate_index;
    double local_load;
    delay_time_t local_required;

    cost->slack = MINUS_INFINITY;
    cost->area = INFINITY;
    foreach_inverter(n_gates, gate_index) {
      local_load = entry[q]->load;
      local_load += map_compute_wire_load(st_count(entry[q]->links));
      local_required = fanout_delay_backward_load_dependent(
          entry[q]->required, gate_index, local_load);
      local_required =
          fanout_delay_backward_intrinsic(local_required, gate_index);
      local_load = entry[p]->load + fanout_delay_get_buffer_load(gate_index);
      local_load += map_compute_wire_load(st_count(entry[p]->links) + 1);
      SETMIN(local_required, local_required, entry[p]->required);
      local_required = fanout_delay_backward_load_dependent(
          local_required, node->gate_index, local_load);
      SETSUB(local_cost.slack, local_required, node->arrival);
      local_cost.area = fanout_delay_get_area(gate_index) +
                        fanout_delay_get_area(node->gate_index);
      if (fanout_opt_is_better_cost(&local_cost, cost)) {
        *cost = local_cost;
        *best_gate_index = gate_index;
      }
    }
  }
}

static void update_children_entries(node, best_gate_index) fanout_node_t *node;
int best_gate_index;
{
  int p, q;
  char *key, *value;
  st_generator *gen;
  remove_opt_t *entry[2];

  foreach_polarity(p) {
    entry[p] = array_fetch_p(remove_opt_t, node->remove_opt, p);
  }
  p = node->polarity;
  q = POLAR_INV(p);
  if (st_count(entry[q]->links) == 0) {
    node->arity = st_count(entry[p]->links);
    array_free(node->children);
    node->children = array_alloc(fanout_node_t *, 0);
    st_foreach_item(entry[p]->links, gen, &key, &value) {
      array_insert_last(fanout_node_t *, node->children, (fanout_node_t *)key);
    }
  } else {
    fanout_node_t *child = get_first_buffer_child(node);
    assert(child != NIL(fanout_node_t));
    child->gate_index = best_gate_index;
    child->arity = st_count(entry[q]->links);
    child->polarity = q;
    array_free(child->children);
    child->children = array_alloc(fanout_node_t *, 0);
    st_foreach_item(entry[q]->links, gen, &key, &value) {
      array_insert_last(fanout_node_t *, child->children, (fanout_node_t *)key);
    }
    compute_buffer_node_cost(child);
    node->arity = st_count(entry[p]->links) + 1;
    array_free(node->children);
    node->children = array_alloc(fanout_node_t *, 0);
    array_insert_last(fanout_node_t *, node->children, child);
    st_foreach_item(entry[p]->links, gen, &key, &value) {
      array_insert_last(fanout_node_t *, node->children, (fanout_node_t *)key);
    }
  }
  compute_buffer_node_cost(node);
}

static fanout_node_t *get_first_buffer_child(node) fanout_node_t *node;
{
  int i;

  for (i = 0; i < array_n(node->children); i++) {
    fanout_node_t *local_child =
        array_fetch(fanout_node_t *, node->children, i);
    if (local_child->type == BUFFER_NODE)
      return local_child;
  }
  return NIL(fanout_node_t);
}

static int already_flat_subtree(node) fanout_node_t *node;
{
  int i;
  int n_buffer_children;
  fanout_node_t *child;
  fanout_node_t *last_buffer_child;

  n_buffer_children = 0;
  last_buffer_child = NIL(fanout_node_t);
  for (i = 0; i < array_n(node->children); i++) {
    child = array_fetch(fanout_node_t *, node->children, i);
    if (child->type == BUFFER_NODE) {
      n_buffer_children++;
      last_buffer_child = child;
    }
  }
  if (n_buffer_children == 0)
    return 1;
  if (n_buffer_children > 1)
    return 0;
  node = last_buffer_child;
  if (!is_inverter(n_gates, node->gate_index))
    return 0;
  n_buffer_children = 0;
  for (i = 0; i < array_n(node->children); i++) {
    child = array_fetch(fanout_node_t *, node->children, i);
    if (child->type == BUFFER_NODE)
      n_buffer_children++;
  }
  return (n_buffer_children == 0);
}

/* remove the "remove_opt_t" arrays from the children */
/* so that there is no need for deallocating later on */
/* actually: does not really work: if it is flattened */
/* some nodes may become inaccessible: should be checked */
/* and deallocated at the end */

static void cleanup_remove_opt_children_entries(node) fanout_node_t *node;
{
  int i;
  fanout_node_t *child;

  for (i = 0; i < array_n(node->children); i++) {
    child = array_fetch(fanout_node_t *, node->children, i);
    if (child->remove_opt != NIL(array_t))
      cleanup_remove_opt_node(child);
  }
}

static void cleanup_remove_opt_node(node) fanout_node_t *node;
{
  int p;

  foreach_polarity(p) {
    remove_opt_t *entry = array_fetch_p(remove_opt_t, node->remove_opt, p);
    st_free_table(entry->links);
  }
  array_free(node->remove_opt);
  node->remove_opt = NIL(array_t);
}

static void remove_buffer_process_source(node) fanout_node_t *node;
{
  remove_buffer_process_buffer(node);
  if (node->remove_opt)
    cleanup_remove_opt_node(node);
}

/* updates area and required time entries */

static void compute_buffer_node_cost(node) fanout_node_t *node;
{
  int i;
  double area = 0.0;
  double load = 0.0;
  delay_time_t required, local_required;
  fanout_node_t *child;

  required = PLUS_INFINITY;
  for (i = 0; i < array_n(node->children); i++) {
    child = array_fetch(fanout_node_t *, node->children, i);
    area += child->area;
    load += child->load;
    local_required = child->required;
    if (child->type == BUFFER_NODE)
      local_required =
          fanout_delay_backward_intrinsic(local_required, child->gate_index);
    SETMIN(required, required, local_required);
  }
  load += map_compute_wire_load(array_n(node->children));
  node->required =
      fanout_delay_backward_load_dependent(required, node->gate_index, load);
  node->area = area + fanout_delay_get_area(node->gate_index);
  node->load = (is_buffer(n_gates, node->gate_index))
                   ? fanout_delay_get_buffer_load(node->gate_index)
                   : 0.0;
}

typedef struct merge_struct merge_t;
struct merge_struct {
  fanout_node_t *node;
  int ignored;
  int sink;
};

static int can_merge_subtrees(node) fanout_node_t *node;
{
  int i;
  int current_index, merge_index;
  delay_time_t arrival;
  fanout_node_t *child;
  merge_t *current_node, *merge_node;
  merge_t merge_entry;
  array_t *node_array = array_alloc(merge_t, 0);
  int modified = 0;

  for (i = 0; i < array_n(node->children); i++) {
    child = array_fetch(fanout_node_t *, node->children, i);
    merge_entry.node = child;
    if (child->type == SINK_NODE) {
      merge_entry.ignored = 1;
      merge_entry.sink = 1;
    } else {
      merge_entry.ignored = 0;
      merge_entry.sink = 0;
    }
    array_insert_last(merge_t, node_array, merge_entry);
  }
  arrival = recompute_arrival_time(node, node_array);
  for (current_index = 0; current_index < array_n(node_array);
       current_index++) {
    current_node = array_fetch_p(merge_t, node_array, current_index);
    if (current_node->ignored)
      continue;
    for (merge_index = current_index + 1; merge_index < array_n(node_array);
         merge_index++) {
      merge_node = array_fetch_p(merge_t, node_array, merge_index);
      if (merge_node->ignored)
        continue;
      if (can_merge_node(current_node->node, merge_node->node, arrival)) {
        merge_nodes(current_node->node, merge_node->node);
        merge_node->ignored = 1;
        arrival = recompute_arrival_time(node, node_array);
        modified = 1;
      }
    }
  }
  adjust_node_children(node, node_array);
  array_free(node_array);
  return modified;
}

static delay_time_t recompute_arrival_time(node,
                                           node_array) fanout_node_t *node;
array_t *node_array;
{
  int i;
  merge_t *merge_node;
  double load = 0.0;
  int n_nodes = 0;

  for (i = 0; i < array_n(node_array); i++) {
    merge_node = array_fetch_p(merge_t, node_array, i);
    if (merge_node->ignored && !merge_node->sink)
      continue;
    load += merge_node->node->load;
    n_nodes++;
  }
  load += map_compute_wire_load(n_nodes);
  return fanout_delay_forward_load_dependent(node->arrival, node->gate_index,
                                             load);
}

static int can_merge_node(current_node, merge_node,
                          arrival) fanout_node_t *current_node;
fanout_node_t *merge_node;
delay_time_t arrival;
{
  double load;
  delay_time_t required, slack;
  int n_elts;

  if (current_node->polarity != merge_node->polarity)
    return 0;
  arrival = fanout_delay_forward_intrinsic(arrival, current_node->gate_index);
  load = 0.0;
  required = PLUS_INFINITY;
  add_children_cost(current_node->children, &required, &load);
  add_children_cost(merge_node->children, &required, &load);
  n_elts = array_n(current_node->children) + array_n(merge_node->children);
  load += map_compute_wire_load(n_elts);
  required = fanout_delay_backward_load_dependent(
      required, current_node->gate_index, load);
  SETSUB(slack, required, arrival);
  return (GETMIN(slack) >= 0.0);
}

static void add_children_cost(array, required, load) array_t *array;
delay_time_t *required;
double *load;
{
  double result_load;
  delay_time_t result_required, local_required;
  int i;
  fanout_node_t *child;

  result_load = *load;
  result_required = *required;
  for (i = 0; i < array_n(array); i++) {
    child = array_fetch(fanout_node_t *, array, i);
    result_load += child->load;
    local_required = child->required;
    if (child->type == BUFFER_NODE)
      local_required =
          fanout_delay_backward_intrinsic(local_required, child->gate_index);
    SETMIN(result_required, result_required, local_required);
  }
  *load = result_load;
  *required = result_required;
}

static void merge_nodes(current_node, merge_node) fanout_node_t *current_node;
fanout_node_t *merge_node;
{
  int i;
  fanout_node_t *child;
  array_t *current_children = current_node->children;
  array_t *merge_children = merge_node->children;
  array_t *new_children = array_alloc(fanout_node_t *, 0);

  for (i = 0; i < array_n(current_children); i++) {
    child = array_fetch(fanout_node_t *, current_children, i);
    assert(child->remove_opt == NIL(array_t));
    array_insert_last(fanout_node_t *, new_children, child);
  }
  for (i = 0; i < array_n(merge_children); i++) {
    child = array_fetch(fanout_node_t *, merge_children, i);
    assert(child->remove_opt == NIL(array_t));
    array_insert_last(fanout_node_t *, new_children, child);
  }
  array_free(current_children);
  current_node->children = new_children;
  current_node->arity = array_n(new_children);
  compute_buffer_node_cost(current_node);
}

static void adjust_node_children(node, child_array) fanout_node_t *node;
array_t *child_array;
{
  int i;
  merge_t *child_entry;

  array_t *new_children = array_alloc(fanout_node_t *, 0);

  for (i = 0; i < array_n(child_array); i++) {
    child_entry = array_fetch_p(merge_t, child_array, i);
    if (child_entry->ignored && !child_entry->sink)
      continue;
    array_insert_last(fanout_node_t *, new_children, child_entry->node);
  }
  array_free(node->children);
  node->children = new_children;
  assert(array_n(new_children) <= node->arity);
  node->arity = array_n(new_children);
}

/* INTERFACE to fanout_est.c */

pchong

/* the arrival time at a node includes the intrinsic delay but not the load
 * dependent delay */
/* the arrival time at a sink is the arrival time at pin input (no intrinsic) */

void fanout_tree_extract_fanout_leaves(fanout, array) multi_fanout_t *fanout;
array_t *array;
{
  int i;
  array_t *trees;
  fanout_node_t *source;
  double source_load;
  int source_polarity;

  trees = fanout_tree_get_roots(array);
  for (i = 0; i < array_n(trees); i++) {
    source = array_fetch(fanout_node_t *, trees, i);
    source_load = fanout_tree_get_node_load(source);
    source_polarity = fanout_delay_get_source_polarity(source->gate_index);
    source->arrival = ZERO_DELAY;
    extract_fanout_leaves_rec(fanout, source, source_load, source_polarity);
  }
  array_free(trees);
}

static void extract_fanout_leaves_rec(fanout, source, source_load,
                                      source_polarity) multi_fanout_t *fanout;
fanout_node_t *source;
double source_load;
int source_polarity;
{
  int i;
  gate_link_t *link;
  int gate_polarity;
  int fanout_polarity;
  int fanout_index;
  double fanout_load;
  fanout_bucket_t *bucket;
  fanout_node_t *fanout_node;
  delay_time_t local_arrival;

  local_arrival = fanout_delay_forward_load_dependent(
      source->arrival, source->gate_index, source_load);
  for (i = 0; i < array_n(source->children); i++) {
    fanout_node = array_fetch(fanout_node_t *, source->children, i);
    switch (fanout_node->type) {
    case SINK_NODE:
      link = fanout_node->link;
      fanout_index = link->pin; /* overloaded: when tree saved, link->pin
                                   becomes fanout_index */
      bucket = fanout_est_fanout_bucket_alloc(fanout);
      bucket->load = source_load;
      bucket->pwl = fanout_delay_get_delay_pwl(source->gate_index, source_load,
                                               source->arrival);
      fanout->leaves[source_polarity][fanout_index].bucket = bucket;
      fanout->leaves[source_polarity][fanout_index].load = link->load;
      fanout_node->arrival = local_arrival;
      break;
    case BUFFER_NODE:
      fanout_node->arrival = fanout_delay_forward_intrinsic(
          local_arrival, fanout_node->gate_index);
      fanout_load = fanout_tree_get_node_load(fanout_node);
      gate_polarity = fanout_delay_get_buffer_polarity(fanout_node->gate_index);
      fanout_polarity = (gate_polarity == POLAR_X) ? source_polarity
                                                   : POLAR_INV(source_polarity);
      extract_fanout_leaves_rec(fanout, fanout_node, fanout_load,
                                fanout_polarity);
      break;
    default:
      fail("unexpected node type in extract_fanout_leaves_rec");
      break;
    }
  }
}

/* suppose the tree has edges already */

double fanout_tree_get_source_load(array) array_t *array;
{
  array_t *trees;
  fanout_node_t *source;
  double source_load;

  trees = fanout_tree_get_roots(array);
  assert(array_n(trees) == 1);
  source = array_fetch(fanout_node_t *, trees, 0);
  source_load = fanout_tree_get_node_load(source);
  array_free(trees);
  return source_load;
}

array_t *fanout_tree_get_root_node(array) array_t *array;
{ return (fanout_tree_get_roots(array)); }
