
/* file @(#)map_int.h	1.11 */
/* last modified on 8/8/91 at 17:02:06 */
/*
 * Revision 1.4  22/.0/.1  .0:.4:.5  sis
 *  Updates for the Alpha port.
 * 
 * Revision 1.3  1992/05/06  18:55:51  sis
 * SIS release 1.1
 *
 * Revision 1.2  1992/04/17  21:59:58  sis
 * *** empty log message ***
 *
 * Revision 1.1  92/01/08  17:40:48  sis
 * Initial revision
 * 
 * Revision 1.2  91/07/02  11:16:50  touati
 * make copies of MAP(node)->arrival_info instead of pointers: safer.
 * add support for load_limit.
 * add option for gate resizing.
 * 
 * Revision 1.1  91/06/28  22:50:15  touati
 * Initial revision
 * 
 */
#ifndef MAP_INT_H
#define MAP_INT_H

#include "map_defs.h"
#include "lib_int.h"

typedef struct prim_struct prim_t;
typedef struct prim_node_struct prim_node_t;
typedef struct prim_edge_struct prim_edge_t;

typedef enum edge_dir_enum edge_dir_t;
enum edge_dir_enum {DIR_IN, DIR_OUT};

struct prim_struct {
    int ninputs;
    prim_node_t **inputs;	/* direct pointers to primitive inputs */
    int noutputs;
    prim_node_t **outputs;	/* direct pointers to primitive outputs */
    prim_node_t *nodes; 	/* linked list of nodes for the primitive */
    prim_edge_t *edges; 	/* linked list of edge for the primitive */
    lsList gates;
    char *data;			/* just a hook to pass additional data */
#ifdef SIS
    int latch_type;           /* sequential support */
#endif
};


struct prim_node_struct {
    char *name;			/* name of node (inputs and outputs only) */
    unsigned isomorphic_sons:1;
    unsigned nfanin:15;
    unsigned nfanout:15;
    node_type_t type;
    node_t *binding;		/* used during matching: binding for node */
    prim_node_t *next;		/* linked list of nodes */
#ifdef SIS
    unsigned latch_output:1;	/* sequential support */
#endif 
};


struct prim_edge_struct {
    prim_node_t *this_node;
    prim_node_t *connected_node;
    edge_dir_t dir;
    prim_edge_t *next;		/* linked list of edges */
};

typedef enum {
  RAW_NETWORK, 
  ANNOTATED_NETWORK, 
  MAPPED_NETWORK, 
  INCONSISTENT_NETWORK, 
  EMPTY_NETWORK,
  UNKNOWN_TYPE
} network_type_t;

typedef struct {
  node_t *pin_source;		/* if pin source has multiple fanout; otherwise NIL(node_t) */
  int fanout_index;		/* index of fanout of that pin source */
} fanin_fanout_t;

#if defined(_IBMR2) || defined(__osf__)
typedef struct alt_map_struct alt_map_t;
struct alt_map_struct {
#else
typedef struct map_struct map_t;
struct map_struct {
#endif
    lib_gate_t *gate;		/* best gate which matches */
    prim_node_t *binding;	/* used during matching */
    node_t *match_node;		/* used to re-build the graph */
    int ninputs;		/* number of inputs for this gate */
    node_t **save_binding;	/* bindings for the gate inputs */
    delay_time_t *arrival_info; /* copy of arrival times of inputs. used in fanout opt (array of size ninputs) */
    double map_area;		/* current area cost function */
    delay_time_t map_arrival;	/* current arrival time cost function */
    double load;		/* record load on this pin (from last map) */
    int slack_is_critical;	/* slack at node is 'critical' */
    int inverter_paid_for;
    delay_time_t inverter_arrival;
    int index;
    delay_time_t required;
    st_table *gate_link;	/* used in bin_delay_build_network: bindings gate input -> gate root */
    st_generator *gen; 		/* to be used in conjunction with gate_link */
    node_t *node_y; 		/* used in bin_power.c to cache information */
    array_t *fanout_tree;
    node_t *fanout_source;
    fanin_fanout_t *pin_info;	/* only allocated when necessary */
};

 /* to pass options */

typedef struct bin_global_struct bin_global_t;
struct bin_global_struct {
  int new_mode; 	        /* new_mode */
  double new_mode_value;	/* 0.0->area ... 1.0->delay */
  int load_bins_count;		/* for newmap delay and area/delay */
  int delay_bins_count;		/* for newmap area/delay */
  double old_mode;		/* old map; area or delay optimize */
  int inverter_optz;		/* inverter optimization heuristics */
  int allow_internal_fanout;	/* degrees of internal fanouts allowed */
  int print_stat;		/* print area/delay stats at the end */
  int verbose;			/* verbosity level */
  int raw;			/* raw mapping */
  double thresh;		/* threshold used in old tree mapper */
  library_t *library;		/* current library */
  int fanout_optimize;		/* if set, the fanout optimizer is called */
  int area_recover;		/* if set, area is recovered after fanout opt */
  int peephole;			/* if set, call fanout peephole optimizer */
  array_t *fanout_alg;		/* array of fanout algorithm descriptors */
  int remove_inverters;		/* if set, remove redundant inverters after mapping */
  int no_warning;		/* if set, does not print any warning message */
  int ignore_pipo_data;		/* if set, ignore specific values set at PIPO's */
  network_type_t network_type;  /* type of the network when mapping is started */
  int load_estimation;		/* 0-ignore fanout; 1-linear fanout; 2-load of two-level tree */
  int ignore_polarity;		/* 0-add an inverter delay; 1-same arrival time for both polarities */
  int cost_function;		/* 0-MAX(rise,fall); 1-AVER(rise,fall) */
  int n_iterations;		/* 0-do it once, 1-repeat once, etc... */
  int fanout_iterate;		/* 1 if tree covering is going to be called afterwards; 0 otherwise */
  int opt_single_fanout;	/* 0/1 */
  int fanout_log_on;		/* 0/1 */
  int allow_duplication;	/* 0/1 */
  int fanout_limit;		/* limit fanout of internal nodes during matching */
  int check_load_limit;         /* if set, check fanout limit during fanout opt */
  int penalty_factor;		/* if check_load_limit set, load exceeding gate limit is multiplied by penalty_factor */
  int all_gates_area_recover;   /* when set, recover area on all gates, not only buffers */
};

typedef struct match_struct match_t;
struct match_struct {
    int ninputs;
    int noutputs;
    node_t **inputs;
    node_t **outputs;
    prim_t *prim;
    lib_gate_t *gate;
};


#define MAP_SLOT		map
#if defined(_IBMR2) || defined(__osf__)
#define MAP(node)		((alt_map_t *) (node)->MAP_SLOT)
#else
#define MAP(node)		((map_t *) (node)->MAP_SLOT)
#endif

#ifdef SIS
#define IS_LATCH_END(node)      (network_latch_end((node))!=NIL(node_t))
#endif

extern void map_network_dup();
extern int map_network_to_prim();
extern st_table *map_find_isomorphisms();
extern void prim_free();
extern void prim_clear_binding();
extern void map_prim_dump();
extern int tree_map();
extern void map_print_tree_size();
extern node_t *map_prim_get_root();
extern void map_alloc(), map_free(), map_invalid();
extern void map_dup(), map_setup_network();
extern void gen_all_matches();
extern int map_check_form();
extern int do_tree_premap();
extern void map_prep();
extern void patch_constant_cells();
extern network_t *map_build_network ();
extern lib_gate_t *choose_smallest_gate();
extern lib_gate_t *choose_nth_smallest_gate();
extern node_t *map_po_get_fanin();

 /* EXPORTED FROM "com_map.c" */
extern int map_fill_options();

 /* EXPORTED FROM "bin_delay.c" */
extern int bin_delay_tree_match();
extern int bin_delay_area_tree_match();
extern void bin_for_all_nodes_inputs_first(/* network_t *network; VoidFn fn; */);
extern void bin_for_all_nodes_outputs_first(/* network_t *network;VoidFn fn; */);

 /* EXPORTED FROM "treemap.c" */
extern network_t *do_old_tree_match();
#ifdef SIS
extern int seq_filter();
#endif /* SIS */

 /* map_interface.c */
extern network_t *complete_map_interface();
extern network_t *build_mapped_network_interface();
extern network_t *tree_map_interface();
extern network_t *fanout_opt_interface();
extern network_type_t map_get_network_type(/* network_t *network */);
extern void map_report_node_data(/* node_t *node */);
extern network_t *map_premap_network();
extern void map_report_data_mapped();

 /* from replace.c */
extern void replace_2or();

#define is_tree_leaf(node)		\
    (node->type == PRIMARY_INPUT || node_num_fanout(node) > 1)

#define match_is_inverter(node, prim) 	\
    (prim->ninputs == 1 && prim->inputs[0]->binding == node_get_fanin(node, 0))

#define best_match_is_inverter(node) 	\
    (MAP(node)->ninputs == 1 && 	\
	MAP(node)->save_binding[0] == node_get_fanin(node, 0))

#ifdef SIS
extern int network_gate_type();
extern node_t *network_latch_input();
extern st_table *network_type_table; 
#endif /* SIS */

#endif
