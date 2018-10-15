/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/seqbdd/seqbdd.h,v $
 * $Author: pchong $
 * $Revision: 1.2 $
 * $Date: 2004/02/07 11:03:22 $
 *
 */
 /* file %M% release %I% */
 /* last modified: %G% at %U% */

#ifndef VERIF_INT_H
#define VERIF_INT_H

typedef enum {
    CONSISTENCY_METHOD,
    BULL_METHOD,
    PRODUCT_METHOD
} range_method_t;

typedef struct {
  int is_product_network;		/* 0/1 */
  int generate_global_output;		/* 0/1 */
  node_t *main_node;			/* main consistency output node: AND of the two net_nodes */
  node_t *output_node;			/* the main output node: AND of the xnor_nodes */
  node_t *init_node;			/* initial state */
  array_t *new_pi;			
  array_t *org_pi;			/* the PI's before we introduce the consistency PI's */
  array_t *extern_pi;			/* the external PI's (not present states) */
  st_table *pi_ordering;		/* some good order of the PI's */
  array_t *po_ordering;			/* next_state_po, in some good order */
					/* the remaining is (so far) only used by "decoupled.c" */
  node_t *main_nodes[2];		/* net nodes: consistency output for each net (for verification only) */
  array_t *xnor_nodes;			/* array of all xnor_nodes  (xnor of external outputs) */
  st_table *name_table;			/* PIPO name is mapped to 0 or 1 (network 0 or network 1) */
  array_t *transition_nodes;		/* nodes corresponding to (y_i == f_i(x)) for each i; product is trans relation */
} output_info_t;

typedef struct {
  bdd_t *total_set;			/* should be computed by general stg traversal routine (read-only) */
  range_method_t type;			/* should always be computed */
  bdd_manager *manager;			/* should always be computed */
  bdd_t *output_fn;			/* should always be computed */
  bdd_t *init_state_fn;			/* should always be computed */
  array_t *pi_inputs;		/* should always be computed: array of BDD's for PI current state */
  bdd_t *consistency_fn;		/* for CONSISTENCY_METHOD */
  array_t *smoothing_inputs;		/* for CONSISTENCY_METHOD */
  array_t *output_fns;			/* for BULL_METHOD  */
  array_t *input_vars;			/* for CONSISTENCY2_METHOD (should be merged with pi_inputs) */
  array_t *output_vars;			/* for CONSISTENCY2_METHOD */
  array_t *external_outputs;		
  array_t *transition_outputs;		/* for CONSISTENCY2 (one bdd_t per (y_i==f_i(x))) */
} range_data_t;

typedef struct verif_options_t verif_options_t;

		/* Each method is implemented using 5 functions of these types. */
		/* The typedefs use _f suffix to remind they are for functions. */

typedef range_data_t *seqbdd_range_f ARGS((network_t *, verif_options_t *));
typedef bdd_t *seqbdd_next_f ARGS((bdd_t *, range_data_t *, verif_options_t *));
typedef bdd_t *seqbdd_reverse_f ARGS((bdd_t *, range_data_t *, verif_options_t *));
typedef void seqbdd_free_f ARGS((range_data_t *, verif_options_t *));
typedef int seqbdd_check_f ARGS((bdd_t *, range_data_t *, int *, verif_options_t *));
typedef void seqbdd_sizes_f ARGS((range_data_t *, int *, int *));

typedef int (*IntFn)();
typedef bdd_t *(*BddFn)();


#define INIT_STATE_OUTPUT_NAME "initial_state"
#define EXTERNAL_OUTPUT_NAME "equiv:output"

struct verif_options_t {
 /* interface options */
  int timeout;
  int keep_old_network;		 /* 0/1 */
  output_info_t *output_info;
 /* algorithm options */
  int does_verification;	 /* 0/1 */
  int n_iter;			 /* used for range_computation */
  int stop_if_verify;		 /* if set, flip the return status of verify_fsm */
  range_method_t   type;
  seqbdd_range_f  *alloc_range_data;
  seqbdd_next_f	  *compute_next_states;
  seqbdd_reverse_f  *compute_reverse_image;
  seqbdd_free_f	  *free_range_data;
  seqbdd_check_f  *check_output;
  seqbdd_sizes_f  *bdd_sizes;
  int verbose;
  char *sim_file;                /* file to save the simulation vectors in */
                                 /* for machines that do not verify */
  int ordering_depth;		 /* use to limit the search in good ordering heuristic */
				 /* depth of 0 means greedy algorithm */
  int use_manual_order;		 /* 0/1 */
  char *order_network_name;
  network_t *order_network;	 /* network that specifies the order of PIPO to be used */
  int last_time;		 /* last time the time was asked for */
  int total_time;		 /* total time since beginning of command */
  int n_partitions;
};


		/* Three sets of functions to implement 3 different methods.  */
		/* Because of bug in DECstation cc, can't use typedefs above. */

extern range_data_t *consistency_alloc_range_data();
extern bdd_t	*consistency_compute_next_states();
extern bdd_t	*consistency_compute_reverse_image();
extern void	 consistency_free_range_data();
extern int 	 consistency_check_output();
extern void 	 consistency_bdd_sizes();

extern range_data_t *bull_alloc_range_data();
extern bdd_t	*bull_compute_next_states();
extern bdd_t	*bull_compute_reverse_image();
extern void 	 bull_free_range_data();
extern int 	 bull_check_output();
extern void 	 bull_bdd_sizes();

extern range_data_t *product_alloc_range_data();
extern bdd_t	*product_compute_next_states();
extern bdd_t	*product_compute_reverse_image();
extern void 	 product_free_range_data();
extern int 	 product_check_output();
extern void 	 product_bdd_sizes();


extern int seqbdd_extract_input_sequence();
extern int bdd_range_fill_options();
extern int input_network_check_pi(/* net1, net2 */);
extern int check_input_networks(/* net1, constraints1, net2, constraints2 */);
extern void bdd_print_any_minterm(/* bdd_t *fn */);
extern int seq_verify_interface();
extern network_t *range_computation_interface();
extern node_t *copy_init_state_constraint();
extern node_t *build_equivalence_node();

 /* from verif_util.c */
extern array_t *order_nodes();
extern array_t *create_new_pi();
extern array_t *get_po_ordering();
extern st_table *get_pi_ordering();
extern array_t *get_remaining_po();
extern array_t *network_extract_next_state_po(/* network_t *network, network_t *constraints */);
extern array_t *network_extract_pi(/* network_t *network */);
extern void print_node_array(/* array_t *array */);
extern void print_node_table(/* st_table *table */);
extern node_t *network_copy_subnetwork();
extern void report_inconsistency();
extern st_table *from_array_to_table(/* array_t *array */);
extern void output_info_free();

 /* from ordering.c */
typedef struct {
  int n_vars;
  int n_sets;
  var_set_t **sets;
} set_info_t;

extern array_t *find_best_set_order(/* set_info_t *info, int verbose */);
extern array_t *find_greedy_set_order(/* set_info_t *info, int verbose */);

 /* from verif_util.c */
extern array_t *bdd_extract_var_array(/* array_t *node_list */);

 /* bdd_tovar.c */
extern array_t *bdd_get_varids(/* array_t *var_array */);
extern array_t *bdd_get_sorted_varids(/* array_t *var_array */);
extern int bdd_varid_cmp(/* char *obj1, char *obj2 */);

extern array_t *extract_state_input_vars(/* st_table *pi_ordering, st_table *ito_table */);
extern array_t *extract_state_output_vars(/* st_table *pi_ordering, st_table *ito_table */);
extern st_table *extract_input_to_output_table(/* array_t *org_pi, *new_pi, *po_ordering, network_t *network */);

extern void report_elapsed_time(/* verif_options_t *options, char *string */);

extern void compute_product_network();
extern void output_info_init();

typedef struct {
  array_t *fns;
} bull_key_t;

typedef struct {
  array_t *fns;
  array_t *ins;
  bdd_t *range;
} bull_value_t;

extern bdd_t *input_cofactor();
extern array_t *disjoint_support_functions();
extern bdd_t *range_2_compute();
extern bdd_t *bull_cofactor();

extern void get_manual_order(/* st_table *order, verif_options_t *options */);

extern int breadth_first_stg_traversal(/* network_t **network, network_t *constraints, */
				       /* verif_options_t *options */);

 /* from product.c */
extern bdd_t *bdd_incr_and_smooth();
#endif /* VERIF_INT_H */
