
/* file @(#)fanout_opt.c	1.10 */
/* last modified on 7/22/91 at 12:36:25 */
/**
**  MODIFICATION HISTORY:
**
**  01 02-Nov-90 cyt    Changed #include "fanout_alg.macro.h" to "fanout_alg_macro.h"
**                      Added code to solve the fanout limit problem and #include delay_int
**  02 27-May-91 klk    New code to handle fanout limit.
**  02 10-Jun-91 klk	Changed primary input max drive to correspond to the second largest inverter
**			in the library
**/


#include "sis.h"
#include <math.h>
#include "map_macros.h"
#include "map_int.h"
#include "fanout_int.h"
#include "fanout_delay.h"

#include "fanout_opt_static.h"

static bin_global_t global;



 /* EXTERNAL INTERFACE */

void fanout_opt_update_peephole_flag(alg, property)
fanout_alg_t *alg;
alg_property_t *property;
{
  alg->peephole_flag = property->value;
}

void fanout_opt_update_size_threshold(alg, property)
fanout_alg_t *alg;
alg_property_t *property;
{
  alg->min_size = property->value;
}

 /* called by the command line option */

array_t *fanout_opt_get_fanout_alg(n_entries, entries)
int n_entries;
char **entries;
{
  int i, j;
  fanout_alg_descr *descr_p;
  fanout_prop_descr *prop_p;
  fanout_alg_t fanout_alg;
  fanout_alg_t *alg;
  alg_property_t property;
  static array_t *list_of_algs = NIL(array_t);

  if (list_of_algs == NIL(array_t)) {

    list_of_algs = array_alloc(fanout_alg_t, 0);
    prop_p = fanout_properties;

    for (descr_p=fanout_algorithms; descr_p->alg_id != 0; descr_p++) {
	fanout_alg.on_flag = descr_p->on_flag;
	fanout_alg.name = descr_p->name;
	fanout_alg.properties = array_alloc(alg_property_t, 0);
	fanout_alg.init = descr_p->init_fn;
	while (prop_p->alg_id == descr_p->alg_id) {
	    property.name = prop_p->name;
	    property.value = prop_p->value;
	    property.update_fn = prop_p->update_fn;
	    array_insert_last (alg_property_t, fanout_alg.properties, property);
	    prop_p++;
	}
	array_insert_last (fanout_alg_t, list_of_algs, fanout_alg);
    }
  }

  if (n_entries == 0) return list_of_algs;

  for (j = 0; j < array_n(list_of_algs); j++) {
    alg = array_fetch_p(fanout_alg_t, list_of_algs, j);
    alg->on_flag = (strcmp(alg->name, "noalg") == 0);
  }
  for (i = 0; i < n_entries; i++) {
    for (j = 0; j < array_n(list_of_algs); j++) {
      alg = array_fetch_p(fanout_alg_t, list_of_algs, j);
      if (strcmp(alg->name, entries[i]) == 0)
	alg->on_flag = 1;
    }
  }    
  return list_of_algs;
}

void fanout_opt_print_fanout_alg(array)
array_t *array;
{
  int i;
  fanout_alg_t *alg;

  fprintf(sisout, "fanout algs in use: ");
  for (i = 0; i < array_n(array); i++) {
    alg = array_fetch_p(fanout_alg_t, array, i);
    if (alg->on_flag) fprintf(sisout, "\"%s\" ", alg->name);
  }
  fprintf(sisout, "\n");

  fprintf(sisout, "fanout algs not in use: ");
  for (i = 0; i < array_n(array); i++) {
    alg = array_fetch_p(fanout_alg_t, array, i);
    if (! alg->on_flag) fprintf(sisout, "\"%s\" ", alg->name);
  }
  fprintf(sisout, "\n");
}

void fanout_opt_set_fanout_param(array, n_entries, entries, alg_index)
array_t *array;
int n_entries;
char **entries;
int *alg_index;
{
  int i;
  int entry_index;
  fanout_alg_t *alg;
  alg_property_t *property;

  *alg_index = -1;
  if (n_entries == 0) return;
  for (i = 0; i < array_n(array); i++) {
    alg = array_fetch_p(fanout_alg_t, array, i);
    if (strcmp(alg->name, entries[0]) == 0) {
      *alg_index = i;
      n_entries--;
      entries++;
      break;
    }
  }
  if (n_entries == 0 || *alg_index == -1) return;
  entry_index = -1;
  for (i = 0; i < array_n(alg->properties); i++) {
    property = array_fetch_p(alg_property_t, alg->properties, i);
    if (strcmp(property->name, entries[0]) == 0) {
      entry_index = i;
      n_entries--;
      entries++;
      break;
    }
  }
  if (n_entries != 1 || entry_index == -1) {
    *alg_index = -1;
    return;
  }
  property->value = atoi(entries[0]);
}

void fanout_opt_print_fanout_param(array, index)
array_t *array;
int index;
{
  int i;
  alg_property_t *property;
  fanout_alg_t *alg = array_fetch_p(fanout_alg_t, array, index);
  
  fprintf(sisout, "property list of alg \"%s\"\n", alg->name);
  for (i = 0; i < array_n(alg->properties); i++) {
    property = array_fetch_p(alg_property_t, alg->properties, i);
    fprintf(sisout, "%s --> %d\n", property->name, property->value);
  }
}


void fanout_optimization(network, globals)
network_t *network;
bin_global_t globals;
{
  global = globals;
  if (! global.fanout_optimize && ! global.area_recover) return;

  fanout_opt_init(network);
  virtual_network_setup_gate_links(network, &globals);
  virtual_network_remove_wires(network);
  virtual_network_flatten(network);
  virtual_delay_compute_arrival_times(network, globals);
  virtual_delay_set_po_negative_required(network);

  bin_for_all_nodes_outputs_first(network, do_fanout_opt);
  if (global.area_recover) {
    virtual_delay_compute_arrival_times(network, globals);
    virtual_network_flatten(network);
    virtual_delay_set_po_required(network);
    bin_for_all_nodes_outputs_first(network, do_fanout_opt);
    if (global.all_gates_area_recover) {
      virtual_delay_compute_arrival_times(network, globals);
      virtual_delay_set_po_required(network);
      virtual_net_for_all_nodes_outputs_first(network, compute_required_times);
      init_gate_info();
      virtual_net_for_all_nodes_inputs_first(network, do_global_area_recover);
      free_gate_info();
    }
  }
  fanout_opt_end(network);
}


  /* INTERNAL INTERFACE */

static void fanout_opt_init(network)
network_t *network;
{
  int i;
  int j;
  fanout_alg_t *alg;
  alg_property_t *property;

  fanout_log_init(&global);
  for (i = 0; i < array_n(global.fanout_alg); i++) {
    alg = array_fetch_p(fanout_alg_t, global.fanout_alg, i);
    if (alg->on_flag) {
      for (j = 0; j < array_n(alg->properties); j++) {
	property = array_fetch_p(alg_property_t, alg->properties, j);
	(*property->update_fn)(alg, property);
      }
      (*alg->init)(network, alg);
    }
  }
}


 /* ARGSUSED */
static void fanout_opt_end(network)
network_t *network;
{
 /* nothing */
}


static void do_fanout_opt(node_x)
    node_t *node_x;
{
#ifdef SIS
  /* no fanout optimization for latches */
  if (lib_gate_type(lib_gate_of(node_x)) != COMBINATIONAL && lib_gate_type(lib_gate_of(node_x)) != UNKNOWN ) {
    virtual_delay_compute_node_required_time(node_x);
    return;
  }
#endif /* SIS */

  switch (node_function(node_x)) {
  case NODE_UNDEFINED:
    /* former po removed */
    break;
  case NODE_0:
  case NODE_1:
    if (! global.no_warning) (void) fprintf(siserr, "WARNING: constants should have been removed earlier\n");
    break;
  case NODE_PO:
 /* already processed */
    break;
  case NODE_BUF:
    fail("should not be any buffers in underlying network");
    break;
  case NODE_INV:
    if (MAP(node_x)->gate != NIL(lib_gate_t)) 
      virtual_delay_compute_node_required_time(node_x);
    break;
  case NODE_PI:
  case NODE_AND:
  case NODE_OR:
  case NODE_COMPLEX:
    if (fanout_map_optimal(node_x) == 0) {
      if (node_x->type == PRIMARY_INPUT || MAP(node_x)->gate != NIL(lib_gate_t))
	virtual_delay_compute_node_required_time(node_x);
    }
    break;
  default:
    ;
  }
}

#define SOURCE_X_ALONE 0
#define SOURCE_Y_ALONE 1
#define SOURCE_X_ON_X  2
#define SOURCE_X_ON_Y  3
#define SOURCE_MAX     4

typedef struct {
  int type;			     /* any of the above #define, except SOURCE_MAX */
  array_t *trees[POLAR_MAX];	     /* tree for sources[POLAR_X] and sources[POLAR_Y] */
  fanout_cost_t costs[POLAR_MAX];    /* cost of tree for each source */
  fanout_cost_t cost;		     /* aggregate cost */
} fanout_sol_t;

 /* needed for K.J.'s code */
static node_t *current_root_node;

 /* returns 1 iff fanout optimization was performed */

static int fanout_map_optimal(node_x)
node_t *node_x;
{
  int p, assign_x;
  int n_fanouts;
  node_t *node_y;
  node_t *sources[2];
  array_t *best_tree;
  int source_assign[POLAR_MAX];
  opt_array_t fanout_info[2];
  opt_array_t empty_fanout_info;
  opt_array_t fanout_info_arg[2];
  fanout_sol_t fanout_sol[SOURCE_MAX];
  fanout_sol_t *assign_sol;
  int best_sol;

 /* extract sink information */
  if (! extract_fanout_problem(node_x, sources, fanout_info)) return 0;
  current_root_node = node_x;
  
 /* 
  * try the following combinations:
  * (0) sources[POLAR_X] alone
  * (1) sources[POLAR_Y] alone
  * (2) sources[POLAR_X] for sinks of POLAR_X and sources[POLAR_Y] for sinks of POLAR_Y
  * (3) sources[POLAR_X] for sinks of POLAR_Y and sources[POLAR_Y] for sinks of POLAR_X
  * if only one polarity of sources, only (0) or (1) is attempted
  * if only one polarity of sinks, only (0) and (1) are attempted
  * To take tree area into account, we add MAP(source)->gate->area to the cost.
  */

  best_sol = SOURCE_MAX;
  if (sources[POLAR_X] != NIL(node_t)) {
    fanout_single_source_optimal(sources[POLAR_X], fanout_info, POLAR_X, &fanout_sol[SOURCE_X_ALONE]);
    fanout_sol[SOURCE_X_ALONE].trees[POLAR_Y] = NIL(array_t);
    fanout_sol[SOURCE_X_ALONE].cost = fanout_sol[SOURCE_X_ALONE].costs[POLAR_X];
    best_sol = SOURCE_X_ALONE;
  }
  if (sources[POLAR_Y] != NIL(node_t)) {
    fanout_single_source_optimal(sources[POLAR_Y], fanout_info, POLAR_Y, &fanout_sol[SOURCE_Y_ALONE]);
    fanout_sol[SOURCE_Y_ALONE].trees[POLAR_X] = NIL(array_t);
    fanout_sol[SOURCE_Y_ALONE].cost = fanout_sol[SOURCE_Y_ALONE].costs[POLAR_Y];
    if (best_sol == SOURCE_MAX) {
      best_sol = SOURCE_Y_ALONE;
    } else if (fanout_opt_is_better_cost(&fanout_sol[SOURCE_X_ALONE].cost, &fanout_sol[SOURCE_Y_ALONE].cost)) {
      fanout_tree_free(fanout_sol[SOURCE_Y_ALONE].trees[POLAR_Y], 0);
      best_sol = SOURCE_X_ALONE;
    } else {
      fanout_tree_free(fanout_sol[SOURCE_X_ALONE].trees[POLAR_X], 0);
      best_sol = SOURCE_Y_ALONE;
    }
  }

  if (sources[POLAR_X] != NIL(node_t) && sources[POLAR_Y] != NIL(node_t)
      && fanout_info[POLAR_X].n_elts > 0 && fanout_info[POLAR_Y].n_elts > 0) {

/* 
 * The complicated case where all possibilities need to be examined
 * A lot of recomputation could be avoided by sharing the work at a lower
 * level, but that would require major recoding. We play it safe here.
 */

    empty_fanout_info.links = st_init_table(st_ptrcmp, st_ptrhash);
    fanout_info_preprocess(&empty_fanout_info);
    source_assign[POLAR_X] = SOURCE_X_ON_X;
    source_assign[POLAR_Y] = SOURCE_X_ON_Y;

    foreach_polarity(assign_x) {
      assign_sol = &fanout_sol[source_assign[assign_x]];
      fanout_info_arg[POLAR_X] = fanout_info[POLAR_X];
      fanout_info_arg[POLAR_Y] = empty_fanout_info;
      fanout_single_source_optimal(sources[assign_x], fanout_info_arg, assign_x, assign_sol);
      fanout_info_arg[POLAR_X] = empty_fanout_info;
      fanout_info_arg[POLAR_Y] = fanout_info[POLAR_Y];
      fanout_single_source_optimal(sources[POLAR_INV(assign_x)], fanout_info_arg, POLAR_INV(assign_x), assign_sol);
      assign_sol->cost = fanout_opt_add_cost(&(assign_sol->costs[POLAR_X]), &(assign_sol->costs[POLAR_Y]));
      foreach_polarity(p) {
	if (MAP(sources[p])->gate) assign_sol->cost.area += MAP(sources[p])->gate->area;
      }
      if (fanout_opt_is_better_cost(&(fanout_sol[best_sol].cost), &(assign_sol->cost))) {
	fanout_tree_free(assign_sol->trees[POLAR_X], 0);
	fanout_tree_free(assign_sol->trees[POLAR_Y], 0);
      } else {
	foreach_polarity(p) {
	  if (fanout_sol[best_sol].trees[p]) fanout_tree_free(fanout_sol[best_sol].trees[p], 0);
	}
	best_sol = source_assign[assign_x];
      }
    }

    fanout_info_free(&empty_fanout_info, 1);
  }

/*
 * At this point, the best solution has been computed, and all the other trees 
 * have already been deallocated.
 * Remove sources no longer in use. Be careful not to remove the other source
 * in that process though. Do not call virtual_network_remove_node recursively.
 * Not clear why it is done here, nor wht it is not done recursively after
 * fanout tree construction. --> Check that later.
 */  

  foreach_polarity(p) {
    if (fanout_sol[best_sol].trees[p] != NIL(array_t)) {
      fanout_delay_add_source(sources[p], p);
      fanout_tree_build(fanout_sol[best_sol].trees[p]);
      fanout_delay_free_sources();
    }
  }
    
  n_fanouts = fanout_info[POLAR_X].n_elts + fanout_info[POLAR_Y].n_elts;
    if (global.fanout_iterate && n_fanouts > 1) {
    assert(sources[POLAR_X] == NIL(node_t) || sources[POLAR_Y] == NIL(node_t));
/*
 *  assert(SOURCE_X_ALONE == POLAR_X && SOURCE_Y_ALONE == POLAR_Y);
 */
    best_tree = fanout_sol[best_sol].trees[best_sol];
    fanout_tree_save_links(best_tree);
    node_y = MAP(node_x)->node_y;
    if (node_y != NIL(node_t) && node_num_fanout(node_y) > 1) {
      MAP(node_y)->fanout_tree = best_tree;
      MAP(node_y)->fanout_source = (sources[POLAR_X] == NIL(node_t)) ? sources[POLAR_Y] : sources[POLAR_X];
    } else {
      assert(node_x != NIL(node_t) && node_num_fanout(node_x) > 1);
      MAP(node_x)->fanout_tree = best_tree;
      MAP(node_x)->fanout_source = (sources[POLAR_X] == NIL(node_t)) ? sources[POLAR_Y] : sources[POLAR_X];
    }
  }

 /* clean up */

  if (fanout_sol[best_sol].trees[POLAR_X] == NIL(array_t)) {
    virtual_network_remove_node(node_x, 1);
  } 
  if (fanout_sol[best_sol].trees[POLAR_Y] == NIL(array_t) && MAP(node_x)->node_y != NIL(node_t)) {
    virtual_network_remove_node(MAP(node_x)->node_y, 1);
  } 

  foreach_polarity(p) {
    if (! global.fanout_iterate && fanout_sol[best_sol].trees[p]) {
      fanout_tree_free(fanout_sol[best_sol].trees[p], 0);
    }
    fanout_info_free(&fanout_info[p], 1);
  }
  return 1;
}

static void fanout_single_source_optimal(source, fanout_info, source_polarity, fanout_sol)
node_t *source;
opt_array_t *fanout_info;
int source_polarity;
fanout_sol_t *fanout_sol;
{
  int i;
  array_t **trees;
  fanout_cost_t *costs;
  fanout_alg_t *alg;
  int best_sol;
  fanout_cost_t best_cost;
  int n_algs = array_n(global.fanout_alg);

  fanout_delay_add_source(source, source_polarity);
  /* iterate through algorithms */
  trees = ALLOC(array_t *, n_algs);
  costs = ALLOC(fanout_cost_t, n_algs);
 
  for (i = 0; i < n_algs; i++) {
    alg = array_fetch_p(fanout_alg_t, global.fanout_alg, i);
    trees[i] = fanout_tree_alloc();
    costs[i].slack = MINUS_INFINITY;
    costs[i].area = INFINITY;
    if (alg->min_size > fanout_info[POLAR_X].n_elts + fanout_info[POLAR_Y].n_elts) continue;
    if (alg->on_flag && (*alg->optimize)(fanout_info, trees[i], &costs[i])) {
      fanout_tree_add_edges(trees[i]);
      fanout_tree_check(trees[i], &costs[i], fanout_info);
      if (alg->peephole_flag) 
	fanout_tree_peephole_optimize(trees[i], &costs[i], fanout_info, global.verbose);
#ifdef DEBUG
      (void)fprintf(sisout,"Algorithm name %s\n", alg->name);
      (void)fprintf(sisout,"Source node: %s\n", source->name);
      (void)fprintf(sisout,"fanout_info: %6.2f %6.2f\n", fanout_info[0].total_load, fanout_info[1].total_load);
      (void)fprintf(sisout,"fanout buffer tree:\n");
      print_fanout_tree(fanout_tree_get_root_node(trees[i]), trees[i]);
      (void)fprintf(sisout, "\n");
#endif
    }
  }

  /* select best solution and build it */
  best_sol = -1;
  best_cost.slack = MINUS_INFINITY;
  best_cost.area = INFINITY;
  for (i = 0; i < n_algs; i++) {
    if (fanout_opt_is_better_cost(&costs[i], &best_cost)) {
      best_cost = costs[i];
      best_sol = i;
    }
  }
  
  
  assert(best_sol != -1);

#ifdef DEBUG
  alg = array_fetch_p(fanout_alg_t, global.fanout_alg, best_sol);
  (void)fprintf(sisout,"source node: %s; Best algorithm: %s. ", source->name, alg->name);  
#endif

  fanout_sol->trees[source_polarity] = trees[best_sol];
  fanout_sol->costs[source_polarity] = costs[best_sol];

#ifdef DEBUG
  if (MAP(source)->gate != NULL)
    (void)fprintf(sisout,".  Gate type: %s\n\n", MAP(source)->gate->name);
  else (void)fprintf(sisout,"\n\n");
#endif


  /* clean up */
  for (i = 0; i < n_algs; i++) {
    if (i == best_sol) continue;
    fanout_tree_free(trees[i], 0);
  }
  FREE(trees);
  FREE(costs);
  fanout_delay_free_sources();
}


  /* just a routine to call initialization routines */
  /* returns 0 and cleanup if there is not enough to optimize */

static int extract_fanout_problem(root, sources, fanout_info)
node_t *root;
node_t **sources;
opt_array_t *fanout_info;
{
  int p, q;
  int n_fanouts;

  sources[POLAR_X] = root;
  sources[POLAR_Y] = MAP(root)->node_y;
  foreach_polarity(p) {
    q = POLAR_INV(p);
    fanout_info[p].links = extract_links(sources[p], sources[q]);
    fanout_info_preprocess(&fanout_info[p]);
  }
  foreach_polarity(p) {
    sources[p] = keep_external_source_only(sources[p]);
  }
  n_fanouts = fanout_info[POLAR_X].n_elts + fanout_info[POLAR_Y].n_elts;
  if (n_fanouts > 1 || (n_fanouts >= 1 && global.opt_single_fanout)) return 1;
  foreach_polarity(p) { fanout_info_free(&fanout_info[p], 1); }
  return 0;
}

static st_table *extract_links(source, exception)
node_t *source;
node_t *exception;
{
  gate_link_t link, *link_ptr;
  st_table *result = st_init_table(st_ptrcmp, st_ptrhash);

  if (source == NIL(node_t)) return result;
  if (gate_link_is_empty(source)) return result;
  gate_link_first(source, &link);
  do {
    if (link.node == exception) continue;
    link_ptr = ALLOC(gate_link_t, 1);
    *link_ptr = link;
    st_insert(result, (char *) link_ptr, NIL(char));
  } while (gate_link_next(source, &link));
  return result;
}


 /* extract the sinks of same polarity as source */
 /* except if sink is the only possible local_node */

void fanout_info_preprocess(fanout_info)
     opt_array_t *fanout_info;
{
  int n_elts;
  int i = 0;
  gate_link_t *key;
  double acc_load;
  delay_time_t required;
  char *dummy_value = NIL(char);
  st_generator *gen;

  n_elts = fanout_info->n_elts = st_count(fanout_info->links);
  if (n_elts == 0) {
    fanout_info->total_load = 0;
    return;
  }
  fanout_info->required = array_alloc(gate_link_t *, n_elts);
  st_foreach_item(fanout_info->links, gen, (char **) &key, &dummy_value) {
    array_insert(gate_link_t *, fanout_info->required, i, key);
    i++;
  }
  array_sort(fanout_info->required, compare_gate_links);

  fanout_info->cumul_load = ALLOC(double, n_elts + 1);
  acc_load = 0.0;
  for (i = 0; i < n_elts; i++) {
    gate_link_t *link = array_fetch(gate_link_t *, fanout_info->required, i);
    fanout_info->cumul_load[i] = acc_load;
    acc_load += link->load;
  }
  fanout_info->total_load = fanout_info->cumul_load[n_elts] = acc_load;

  fanout_info->min_required = ALLOC(delay_time_t, n_elts);
  required = PLUS_INFINITY;
  for (i = n_elts - 1; i >= 0; i--) {
    gate_link_t *link = array_fetch(gate_link_t *, fanout_info->required, i);
    SETMIN(required, required, link->required);
    fanout_info->min_required[i] = required;
  }
}


 /* keeps node if it is a source external to fanout tree */
static node_t *keep_external_source_only(node)
     node_t *node;
{
  if (node == NIL(node_t)) return node;
  if (node->type == PRIMARY_INPUT) return node;
  if (MAP(node)->gate == NIL(lib_gate_t)) return NIL(node_t);
  if (MAP(node)->ninputs > 1) return node;
  return NIL(node_t);
}


  /* compares two gate links, by required times */

static int compare_gate_links(obj1, obj2)
char *obj1, *obj2;
{
  gate_link_t *g1 = *(gate_link_t **) obj1;
  gate_link_t *g2 = *(gate_link_t **) obj2;
  double cmp = GETMIN(g1->required) - GETMIN(g2->required);
  if (cmp == 0.0) {
    cmp = -(g1->load - g2->load);
    if (cmp == 0.0) {
      cmp = (g1->pin - g2->pin);
    }
  } 
  if (cmp < 0) return -1;
  if (cmp > 0) return 1;
  return 0;
}


 /* the "free_link_flag" is for bottom_up */
 /* it should be 1 if used here in fanout_opt.c */
 /* and 0 if used in bottom_up.c to avoid freeing twice */

void fanout_info_free(fanout_info, free_link_flag)
     opt_array_t *fanout_info;
     int free_link_flag;
{
  gate_link_t *key;
  char *dummy_value = NIL(char);
  st_generator *gen;

  if (free_link_flag) {
    st_foreach_item(fanout_info->links, gen, (char **) &key, &dummy_value) {
      FREE(key);
    }
  }
  st_free_table(fanout_info->links);
  if (fanout_info->n_elts == 0) return;
  array_free(fanout_info->required);
  FREE(fanout_info->cumul_load);
  FREE(fanout_info->min_required);
}

int fanout_opt_is_better_cost(cost1, cost2)
fanout_cost_t *cost1;
fanout_cost_t *cost2;
{
  double slack1 = GETMIN(cost1->slack);
  double slack2 = GETMIN(cost2->slack);
  double diff   = slack1 - slack2;

  if (FP_EQUAL(slack1, 0.0)) slack1 = 0.0;
  if (FP_EQUAL(slack2, 0.0)) slack2 = 0.0;
  if (FP_EQUAL(diff, 0.0))   diff   = 0.0;

  if (slack2 < 0) {
    return (diff > 0) ? 1 : 0;
  } else if (slack1 < 0) {
    return 0;
  } else {
    return (cost1->area < cost2->area) ? 1 : 0;
  }
}

static fanout_cost_t fanout_opt_add_cost(cost1, cost2)
fanout_cost_t *cost1;
fanout_cost_t *cost2;
{
  fanout_cost_t result;

  SETMIN(result.slack, cost1->slack, cost2->slack);
  result.area = cost1->area + cost2->area;
  return result;
}


 /* exported to K.J.'s buffering algorithm */

node_t *fanout_opt_get_root()
{
  int p;
  node_t *source;
  node_t *sources[2];

  sources[POLAR_X] = current_root_node;
  sources[POLAR_Y] = MAP(current_root_node)->node_y;
  foreach_polarity(p) {
    source = keep_external_source_only(sources[p]);
    if (source != NIL(node_t)) return source;
  }
  return NIL(node_t);
}

node_t *fanout_opt_get_root_inv()
{
  int p;
  node_t *source;
  node_t *sources[2];

  sources[POLAR_X] = current_root_node;
  sources[POLAR_Y] = MAP(current_root_node)->node_y;
  source = fanout_opt_get_root();
  foreach_polarity(p) {
    if (source == sources[p]) 
      return sources[POLAR_INV(p)];
  }
  return NIL(node_t);
}



 /* GLOBAL AREA RECOVERY: consider all the gates */

typedef struct {
  int n_equiv_gates;
  lib_gate_t **gates;
} gate_info_t;

static st_table *gate_info;


static void init_gate_info()
{
  int i;
  lib_class_t *class;
  lib_gate_t *gate;
  lsGen class_gen, gate_gen;
  library_t *library;
  gate_info_t *info;

  assert(gate_info == NIL(st_table));
  gate_info = st_init_table(st_ptrcmp, st_ptrhash);
  assert((library = lib_get_library()) != NIL(library_t));
  lsForeachItem(library->classes, class_gen, class) {
    info = ALLOC(gate_info_t, 1);
    info->n_equiv_gates = lsLength(class->gates);
    info->gates = ALLOC(lib_gate_t *, info->n_equiv_gates);
    i = 0;
    lsForeachItem(class->gates, gate_gen, gate) {
      st_insert(gate_info, (char *) gate, (char *) info);
      info->gates[i++] = gate;
    }
  }
}

static void free_gate_info()
{
  int i;
  st_generator *gen;
  char *key;
  gate_info_t *value;
  array_t *values;

  values = array_alloc(gate_info_t *, 0);
  st_foreach_item(gate_info, gen, &key, (char **) &value) {
    if (value->gates != NIL(lib_gate_t *)) {
      FREE(value->gates);
      value->gates = NIL(lib_gate_t *);
      array_insert_last(gate_info_t *, values, value);
    }
  }
  st_free_table(gate_info);
  for (i = 0; i < array_n(values); i++) {
    value = array_fetch(gate_info_t *, values, i);
    FREE(value);
  }
  array_free(values);
  gate_info = NIL(st_table);
}


static void compute_required_times(node)
node_t *node;
{
#ifdef SIS
  /* skip latches */
  if (lib_gate_type(lib_gate_of(node)) != COMBINATIONAL && lib_gate_type(lib_gate_of(node)) != UNKNOWN ) {
    virtual_delay_compute_node_required_time(node);
    return;
  }
#endif /* SIS */

  switch (node_function(node)) {
  case NODE_UNDEFINED:
    /* former po removed */
    break;
  case NODE_0:
  case NODE_1:
    fail("ERROR: constants should have been removed earlier\n");
    break;
  case NODE_PO:
  case NODE_PI:
    /* nothing to do */
    break;
  case NODE_BUF:
  case NODE_INV:
  case NODE_AND:
  case NODE_OR:
  case NODE_COMPLEX:
    if (MAP(node)->gate != NIL(lib_gate_t)) {
      virtual_delay_compute_node_required_time(node);
    }
    break;
  default:
    ;
  }
}

static void do_global_area_recover(node)
node_t *node;
{
  gate_info_t *info;

#ifdef SIS
  /* skip latches */
  if (lib_gate_type(lib_gate_of(node)) != COMBINATIONAL && lib_gate_type(lib_gate_of(node)) != UNKNOWN ) {
    recompute_arrival_times(node);
    return;
  }
#endif /* SIS */
  
  switch (node_function(node)) {
  case NODE_UNDEFINED:
    /* former po removed */
    break;
  case NODE_0:
  case NODE_1:
    if (! global.no_warning) (void) fprintf(siserr, "WARNING: constants should have been removed earlier\n");
    break;
  case NODE_PO:
  case NODE_PI:
    /* nothing to do */
    break;
  case NODE_BUF:
  case NODE_INV:
  case NODE_AND:
  case NODE_OR:
  case NODE_COMPLEX:
    if (MAP(node)->gate != NIL(lib_gate_t)) {
      assert(st_lookup(gate_info, (char *) MAP(node)->gate, (char **) &info));
      if (info->n_equiv_gates > 1) {
	resize_node(node, info);
      }
      recompute_arrival_times(node);
    }
    break;
  default:
    ;
  }
}

static void resize_node(node, info)
node_t *node;
gate_info_t *info;
{
  int i;
  int best_index;
  delay_time_t arrival, required;
  fanout_cost_t *costs;
  fanout_cost_t best_cost;

  required = MAP(node)->required;
  costs = ALLOC(fanout_cost_t, info->n_equiv_gates);
  for (i = 0; i < info->n_equiv_gates; i++) {
    arrival = get_resized_node_arrival_time(node, info->gates[i]);
    SETSUB(costs[i].slack, required, arrival);
    costs[i].area = lib_gate_area(info->gates[i]);
  }
  best_index = -1;
  best_cost.slack = MINUS_INFINITY;
  best_cost.area = INFINITY;
  for (i = 0; i < info->n_equiv_gates; i++) {
    if (fanout_opt_is_better_cost(&costs[i], &best_cost)) {
      best_cost = costs[i];
      best_index = i;
    }
  }
  assert(best_index >= 0);
  replace_node_gate(node, info->gates[best_index]);
  FREE(costs);
}

 /* for a 'new_gate' candidate do: */
 /* recompute the arrival times at each input of 'node' */
 /* by taking into account the difference in load values */
 /* compute the arrival time at the output of new_gate and return that value */

static delay_time_t get_resized_node_arrival_time(node, new_gate)
node_t *node;
lib_gate_t *new_gate;
{
  int i;
  delay_time_t *arrival_times;
  delay_time_t **arrival_times_p;
  double load_diff;
  delay_time_t arrival;
  double load, load_limit;
  
  if (new_gate == MAP(node)->gate) {
    return MAP(node)->map_arrival;
  }
  arrival_times = ALLOC(delay_time_t, MAP(node)->ninputs);
  arrival_times_p = ALLOC(delay_time_t *, MAP(node)->ninputs);
  for (i = 0; i < MAP(node)->ninputs; i++) {
    load_diff = delay_get_load(new_gate->delay_info[i]) - delay_get_load(MAP(node)->gate->delay_info[i]);
    arrival_times[i] = get_reloaded_node_arrival_time(MAP(node)->save_binding[i], load_diff);
    arrival_times_p[i] = &(arrival_times[i]);
  }
  load = MAP(node)->load;
  load_limit = delay_get_load_limit(new_gate->delay_info[0]);
  if (global.check_load_limit && load > load_limit) {
    load *= global.penalty_factor;
  }
  arrival = delay_map_simulate(MAP(node)->ninputs, arrival_times_p, new_gate->delay_info, load);
  FREE(arrival_times);
  FREE(arrival_times_p);
  return arrival;
}


 /* recompute the arrival time at the node with load_diff as load */
 /* the arrival time information is supposed to have been saved in MAP(node)->arrival_info */
 /* when flag is set, new load value is kept in node */

static delay_time_t get_reloaded_node_arrival_time(node, load_diff)
node_t *node;
double load_diff;
{
  double load, load_limit;

  load = MAP(node)->load + load_diff;
  if (node->type == PRIMARY_INPUT) {
    load_limit = pipo_get_pi_load_limit(node);
  } else {
    load_limit = delay_get_load_limit(MAP(node)->gate->delay_info[0]);
  }
  if (global.check_load_limit && load > load_limit) {
    load *= global.penalty_factor;
  }
  return recompute_map_arrival(node, load);
}


 /* have to change the loads on the inputs and the gate_links appropriately for load info */
 /* do not need to compute the required times: done later */
 /* WARNING: may be wrong: what if same gate has several connections to the same node? */

static void replace_node_gate(node, new_gate)
node_t *node;
lib_gate_t *new_gate;
{
  int i;
  node_t *input;
  double load_diff;

  if (new_gate == MAP(node)->gate) return;
  for (i = 0; i < MAP(node)->ninputs; i++) {
    load_diff = delay_get_load(new_gate->delay_info[i]) - delay_get_load(MAP(node)->gate->delay_info[i]);
    input = MAP(node)->save_binding[i];
    MAP(input)->load += load_diff;
    MAP(input)->map_arrival = recompute_map_arrival(input, MAP(input)->load);
  }
  MAP(node)->gate = new_gate;
}

 /* FUNCTION: to be used when the output load has changed to recompute MAP(node)->map_arrival */

static delay_time_t recompute_map_arrival(node, load)
node_t *node;
double load;
{
  int i;
  delay_time_t arrival, drive;
  delay_time_t **arrival_times;

  assert(node->type != PRIMARY_OUTPUT);
  if (node->type == PRIMARY_INPUT) {
    arrival = pipo_get_pi_arrival(node);
    drive =   pipo_get_pi_drive(node);
    arrival.rise += drive.rise * load;
    arrival.fall += drive.fall * load;
  } else {
    arrival_times = ALLOC(delay_time_t *, MAP(node)->ninputs);
    for (i = 0; i < MAP(node)->ninputs; i++) {
      arrival_times[i] = &(MAP(node)->arrival_info[i]);
    }
    arrival = delay_map_simulate(MAP(node)->ninputs, arrival_times, MAP(node)->gate->delay_info, load);
    FREE(arrival_times);
  }
  return arrival;
}


 /* PROCEDURE: to be used when the arrival times of the inputs have changed to recompute */
 /* MAP(node)->arrival_info and MAP(node)->map_arrival */

static void recompute_arrival_times(node)
node_t *node;
{
  int i;
  delay_time_t *arrival_times;
  delay_time_t **arrival_times_p;

  arrival_times = ALLOC(delay_time_t, MAP(node)->ninputs);
  arrival_times_p = ALLOC(delay_time_t *, MAP(node)->ninputs);
  for (i = 0; i < MAP(node)->ninputs; i++) {
    arrival_times[i] = MAP(MAP(node)->save_binding[i])->map_arrival;
    arrival_times_p[i] = &(arrival_times[i]);
  }
  MAP(node)->map_arrival = delay_map_simulate(MAP(node)->ninputs, arrival_times_p, MAP(node)->gate->delay_info, MAP(node)->load);
  FREE(MAP(node)->arrival_info);
  MAP(node)->arrival_info = arrival_times;
  FREE(arrival_times_p);
}


 /* the next two routines are needed to guarantee that the nodes */
 /* are visited in the proper order. 'network_dfs' does not always */
 /* work on the virtual networks after fanout optimization */
 /* moreover, we can skip over nodes this way, so it can be faster */

static void virtual_net_for_all_nodes_outputs_first(network,fn)
network_t *network;
VoidFn fn;
{
  lsGen gen;
  node_t *pi;
  st_table *visited;

  visited = st_init_table(st_ptrcmp, st_ptrhash);
  foreach_primary_input(network, gen, pi) {
    outputs_first_rec(pi, fn, visited);
  }
  st_free_table(visited);
}

static void outputs_first_rec(node, fn, visited)
node_t *node;
VoidFn fn;
st_table *visited;
{
  gate_link_t link;

  if (st_lookup(visited, (char *) node, NIL(char *))) return;
  st_insert(visited, (char *) node, NIL(char));
  if (node->type != PRIMARY_OUTPUT) {
    assert(node->type == PRIMARY_INPUT || MAP(node)->gate != NIL(lib_gate_t));
    if (gate_link_n_elts(node) > 0) {
      gate_link_first(node, &link);
      do {
	outputs_first_rec(link.node, fn, visited);
      } while (gate_link_next(node, &link));
    }
  }
  (*fn)(node);
}

static void virtual_net_for_all_nodes_inputs_first(network,fn)
network_t *network;
VoidFn fn;
{
  lsGen gen;
  node_t *po;
  node_t *node;
  st_table *visited;

  visited = st_init_table(st_ptrcmp, st_ptrhash);
  foreach_primary_output(network, gen, po) {
    node = map_po_get_fanin(po);
    inputs_first_rec(node, fn, visited);
    (*fn)(po);
  }
  st_free_table(visited);
}

static void inputs_first_rec(node, fn, visited)
node_t *node;
VoidFn fn;
st_table *visited;
{
  int i;

  if (st_lookup(visited, (char *) node, NIL(char *))) return;
  st_insert(visited, (char *) node, NIL(char));
  if (node->type != PRIMARY_INPUT) {
    assert(MAP(node)->gate != NIL(lib_gate_t));
    for (i = 0; i < MAP(node)->ninputs; i++) {
      inputs_first_rec(MAP(node)->save_binding[i], fn, visited);
    }
  }
  (*fn)(node);
}
