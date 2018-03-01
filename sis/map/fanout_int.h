/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/map/fanout_int.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:24 $
 *
 */
/* file @(#)fanout_int.h	1.5 */
/* last modified on 7/1/91 at 22:37:04 */
#ifndef FANOUT_INT_H
#define FANOUT_INT_H

#include "multi_array.h"
#include "gate_link.h"
#include "map_defs.h"

typedef multidim_t *(*MultidimFn)();

#define POLAR_X 0
#define POLAR_Y 1
#define POLAR_MAX 2
#define POLAR_INV(polarity) (1 - (polarity))
#define foreach_polarity(x)for((x)=POLAR_X;(x)<POLAR_MAX;(x)++)

typedef struct single_source_struct single_source_t;
struct single_source_struct {
  delay_time_t required;
  double load;
  int n_fanouts;
  double area;
};

typedef struct selected_source_struct selected_source_t;
struct selected_source_struct {
  int main_source;
  int main_source_sink_polarity;
  int buffer;
};

typedef struct opt_array_struct opt_array_t;
struct opt_array_struct {
  int n_elts;
  st_table *links;
  array_t *required;		/* ptrs to gate links, in increasing required time order */
  double *cumul_load;		/* cumul_load[i] = load[0] + ... + load[i-1] in required time order */
  delay_time_t *min_required;	/* min_required[i] = MIN(required[i],...,required[last]) in required time order */
  double total_load;            /* always equal to cumul_load[n_elts - 1] when n_elts > 0 */
};

typedef struct alg_property_struct alg_property_t;
struct alg_property_struct {
  char *name;
  int value;
  VoidFn update_fn;
};

typedef struct fanout_alg_struct fanout_alg_t;
struct fanout_alg_struct {
  char *name;
  int on_flag;
  int peephole_flag;
  int min_size;			/* under that size, not even called */
  array_t *properties;		/* array of properties (alg_property_t) */
  VoidFn init;
  IntFunc optimize;		/* optimize the fanout problem, returns an array */
};

typedef struct fanout_cost_struct fanout_cost_t;
struct fanout_cost_struct {
  delay_time_t slack;
  double area;
};


 /* exported from each fanout algorithm */

#include "map_macros.h"
#include "fanout_alg_macro.h"

 /* exported from FANOUT_UTIL.C */

extern delay_time_t MINUS_INFINITY;
extern delay_time_t PLUS_INFINITY;
extern delay_time_t ZERO_DELAY;

extern single_source_t SINGLE_SOURCE_INIT_VALUE;

extern int find_minimum_index(/* delay_time_t *array, int n */);
extern void generic_fanout_optimizer(/* opt_array_t *fanout_info, array_t *tree, fanout_cost_t *cost,
					VoidFn optimize_one_source, VoidFn extract_merge_info,
					VoidFn build_tree */);


 /* exported from VIRTUAL_NETWORK.C */

extern void virtual_network_setup_gate_links(/* network_t *network */);
extern void virtual_network_remove_wires(/* network_t *network */);
extern void virtual_network_flatten(/* network_t *network */);
extern void virtual_network_remove_node(/* node_t *node */);
extern void virtual_network_add_to_gate_link(/* node_t *source, gate_link_t *link */);
extern void  virtual_network_update_link_required_times(/* node_t *node, delay_time_t *required */);


 /* exported from VIRTUAL_DELAY.C */

extern void virtual_delay_compute_arrival_times(/* network_t *network */);
extern void virtual_delay_arrival_times_rec(/* st_table *visited, node_t *node */);
extern void virtual_delay_set_po_negative_required(/* network_t *network */);
extern void virtual_delay_set_po_required(/* network_t *network */);
extern void virtual_delay_compute_node_required_time(/* node_t *node */);

 /* exported from FANOUT_OPT.C */

extern array_t *fanout_opt_get_fanout_alg(/* int n_entries, char **entries */);
extern void     fanout_opt_print_fanout_alg(/* array_t *fanout_alg */);
extern void     fanout_opt_set_fanout_param(/* array_t *fanout_alg, int n_entries, char **entries, int *alg_index */);
extern void     fanout_opt_print_fanout_param(/* array_t *fanout_alg, int alg_index */);
extern void     fanout_optimization(/* network_t *network, bin_global_t globals */);
extern void     fanout_info_preprocess(/* opt_array_t *fanout_info (only one polarity at a time) */);
extern void     fanout_info_free(/* opt_array_t *fanout_info (only one polarity at a time) */);
extern int      fanout_opt_is_better_cost(/* fanout_cost_t *cost1, fanout_cost_t *cost2 */);
extern node_t  *fanout_opt_get_root(/* */);
extern node_t  *fanout_opt_get_root_inv(/* */);
extern double   get_genlib_maxload (/* lib_gate_t* */); /* find the max load driven by a gate*/
extern unsigned exceed_fanout_limit (/* node_t *, double*/); /* determine if the specified load exceeds the max. allowable load */
extern double   fanout_opt_get_max_load(/* node_t* */); /* find the max. allowable load on a node */
extern double   get_nth_smallest_inv_load(/* int (inverter type) */); /* find the max. allowable on an 'int'th smallest inverter*/

 /* exported from FANOUT_TREE.C */

extern void     fanout_tree_build(/* array_t *array, fanout_cost_t *cost, opt_array_t *fanout_info */);
extern array_t *fanout_tree_alloc(/* */);
extern void     fanout_tree_free(/* array_t *array */);
extern void     fanout_tree_save_links(/* array_t *array */);
extern void     fanout_tree_add_edges(/* array_t *array */);
extern void     fanout_tree_check(/* array_t *array, fanout_cost_t *cost */);
extern void     fanout_tree_peephole_optimize(/* array_t *array, fanout_cost_t *cost */);
extern void     fanout_tree_copy(/* array_t *to, array_t *from */);
extern void     fanout_tree_insert_sink(/* array_t *array, gate_link_t *link */);
extern void     fanout_tree_insert_gate(/* array_t *array, int gate_index, int arity */);


 /* exported from NOALG.C */

extern void noalg_insert_sinks(/* tree, fanout_info, from, to */);

 /* exported from TWO_LEVEL.C */

typedef struct two_level_struct two_level_t;
struct two_level_struct {
  int gate_index;	       /* gate to put at intermediate nodes (index into the buffer table) */
  int n_gates;		       /* number of intermediate nodes */
  delay_time_t required;       /* required time at the source, not including source intrinsic */
  double area;
};

extern multidim_t *two_level_optimize_for_lt_trees(/* opt_array_t *fanout_info */);
extern void insert_two_level_tree(/* array_t *tree, opt_array_t *fanout_info, int sink_index, int source_index, two_level_t *entry */);

 /* exported from fanout_tree.c for fanout_est.c */

extern void fanout_tree_extract_fanout_leaves(/* multi_fanout_t *fanout; opt_array_t *fanout_info; array_t *array; */);
extern double fanout_tree_get_source_load(/* array_t *array */);


 /* exported from fanout_log.c */

extern void fanout_log_init(/* bin_global_t *options; */);
extern void fanout_log_cleanup_network(/* network_t *network; */);
extern void fanout_log_register_node(/* node_t *node; */);

#endif
