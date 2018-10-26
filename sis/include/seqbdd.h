
/* file %M% release %I% */
/* last modified: %G% at %U% */

#ifndef SEQBDD_H
#define SEQBDD_H

#include "node.h"
#include "array.h"
#include "st.h"
#include "bdd.h"

typedef enum { CONSISTENCY_METHOD, BULL_METHOD, PRODUCT_METHOD } range_method_t;

typedef struct {
  int is_product_network;     /* 0/1 */
  int generate_global_output; /* 0/1 */
  node_t
      *main_node; /* main consistency output node: AND of the two net_nodes */
  node_t *output_node; /* the main output node: AND of the xnor_nodes */
  node_t *init_node;   /* initial state */
  array_t *new_pi;
  array_t *org_pi;       /* the PI's before we introduce the consistency PI's */
  array_t *extern_pi;    /* the external PI's (not present states) */
  st_table *pi_ordering; /* some good order of the PI's */
  array_t *po_ordering;  /* next_state_po, in some good order */
  /* the remaining is (so far) only used by "decoupled.c" */
  node_t *main_nodes[2]; /* net nodes: consistency output for each net (for
                            verification only) */
  array_t *xnor_nodes; /* array of all xnor_nodes  (xnor of external outputs) */
  st_table
      *name_table; /* PIPO name is mapped to 0 or 1 (network 0 or network 1) */
  array_t *transition_nodes; /* nodes corresponding to (y_i == f_i(x)) for each
                                i; product is trans relation */
} output_info_t;

typedef struct {
  bdd_t *total_set;      /* should be computed by general stg traversal routine
                            (read-only) */
  range_method_t type;   /* should always be computed */
  bdd_manager *manager;  /* should always be computed */
  bdd_t *output_fn;      /* should always be computed */
  bdd_t *init_state_fn;  /* should always be computed */
  array_t *pi_inputs;    /* should always be computed: array of BDD's for PI
                            current state */
  bdd_t *consistency_fn; /* for CONSISTENCY_METHOD */
  array_t *smoothing_inputs; /* for CONSISTENCY_METHOD */
  array_t *output_fns;       /* for BULL_METHOD  */
  array_t *input_vars;       /* for CONSISTENCY2_METHOD (should be merged with
                                pi_inputs) */
  array_t *output_vars;      /* for CONSISTENCY2_METHOD */
  array_t *external_outputs;
  array_t
      *transition_outputs; /* for CONSISTENCY2 (one bdd_t per (y_i==f_i(x))) */
} range_data_t;

typedef struct verif_options_t verif_options_t;

/* Each method is implemented using 5 functions of these types. */
/* The typedefs use _f suffix to remind they are for functions. */

typedef range_data_t *seqbdd_range_f(network_t *, verif_options_t *);

typedef bdd_t *seqbdd_next_f(bdd_t *, range_data_t *, verif_options_t *);

typedef bdd_t *seqbdd_reverse_f(bdd_t *, range_data_t *, verif_options_t *);

typedef void seqbdd_free_f(range_data_t *, verif_options_t *);

typedef int seqbdd_check_f(bdd_t *, range_data_t *, int *, verif_options_t *);

typedef void seqbdd_sizes_f(range_data_t *, int *, int *);

typedef int (*IntFn)();

typedef bdd_t *(*BddFn)();

#define INIT_STATE_OUTPUT_NAME "initial_state"
#define EXTERNAL_OUTPUT_NAME "equiv:output"

struct verif_options_t {
  /* interface options */
  int timeout;
  int keep_old_network; /* 0/1 */
  output_info_t *output_info;
  /* algorithm options */
  int does_verification; /* 0/1 */
  int n_iter;            /* used for range_computation */
  int stop_if_verify;    /* if set, flip the return status of verify_fsm */
  range_method_t type;
  seqbdd_range_f *alloc_range_data;
  seqbdd_next_f *compute_next_states;
  seqbdd_reverse_f *compute_reverse_image;
  seqbdd_free_f *free_range_data;
  seqbdd_check_f *check_output;
  seqbdd_sizes_f *bdd_sizes;
  int verbose;
  char *sim_file; /* file to save the simulation vectors in */
  /* for machines that do not verify */
  int ordering_depth; /* use to limit the search in good ordering heuristic */
  /* depth of 0 means greedy algorithm */
  int use_manual_order; /* 0/1 */
  char *order_network_name;
  network_t
      *order_network; /* network that specifies the order of PIPO to be used */
  int last_time;      /* last time the time was asked for */
  int total_time;     /* total time since beginning of command */
  int n_partitions;
};

/* Three sets of functions to implement 3 different methods.  */
/* Because of bug in DECstation cc, can't use typedefs above. */

range_data_t *consistency_alloc_range_data();

bdd_t *consistency_compute_next_states();

bdd_t *consistency_compute_reverse_image();

void consistency_free_range_data();

int consistency_check_output();

void consistency_bdd_sizes();

range_data_t *bull_alloc_range_data();

bdd_t *bull_compute_next_states();

bdd_t *bull_compute_reverse_image();

void bull_free_range_data();

int bull_check_output();

void bull_bdd_sizes();

range_data_t *product_alloc_range_data();

bdd_t *product_compute_next_states();

bdd_t *product_compute_reverse_image();

void product_free_range_data();

int product_check_output();

void product_bdd_sizes();

int seqbdd_extract_input_sequence();

int bdd_range_fill_options();

int input_network_check_pi(/* net1, net2 */);

int check_input_networks(/* net1, constraints1, net2, constraints2 */);

void bdd_print_any_minterm(/* bdd_t *fn */);

int seq_verify_interface();

network_t *range_computation_interface();

node_t *copy_init_state_constraint();

node_t *build_equivalence_node();

/* from verif_util.c */
array_t *order_nodes();

array_t *create_new_pi();

array_t *get_po_ordering();

st_table *get_pi_ordering();

array_t *get_remaining_po();

array_t *network_extract_next_state_po(
    /* network_t *network, network_t *constraints */);

array_t *network_extract_pi(/* network_t *network */);

void print_node_array(/* array_t *array */);

void print_node_table(/* st_table *table */);

node_t *network_copy_subnetwork();

void report_inconsistency();

st_table *from_array_to_table(/* array_t *array */);

void output_info_free();

/* from ordering.c */
typedef struct {
  int n_vars;
  int n_sets;
  var_set_t **sets;
} set_info_t;

array_t *find_best_set_order(/* set_info_t *info, int verbose */);

array_t *find_greedy_set_order(/* set_info_t *info, int verbose */);

/* from verif_util.c */
array_t *bdd_extract_var_array(/* array_t *node_list */);

/* bdd_tovar.c */
array_t *bdd_get_varids(/* array_t *var_array */);

array_t *bdd_get_sorted_varids(/* array_t *var_array */);

int bdd_varid_cmp(/* char *obj1, char *obj2 */);

array_t *
    extract_state_input_vars(/* st_table *pi_ordering, st_table *ito_table */);

array_t *
    extract_state_output_vars(/* st_table *pi_ordering, st_table *ito_table */);

st_table *extract_input_to_output_table(
    /* array_t *org_pi, *new_pi, *po_ordering, network_t *network */);

void report_elapsed_time(/* verif_options_t *options, char *string */);

void compute_product_network();

void output_info_init();

typedef struct {
  array_t *fns;
} bull_key_t;

typedef struct {
  array_t *fns;
  array_t *ins;
  bdd_t *range;
} bull_value_t;

bdd_t *input_cofactor();

array_t *disjoint_support_functions();

bdd_t *range_2_compute();

bdd_t *bull_cofactor();

void get_manual_order(/* st_table *order, verif_options_t *options */);

int breadth_first_stg_traversal(/* network_t **network, network_t
                                          *constraints, */
                                       /* verif_options_t *options */);

/* from product.c */
bdd_t *bdd_incr_and_smooth();

int init_seqbdd();

int end_seqbdd();

#endif /* SEQBDD_H */
