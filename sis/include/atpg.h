#ifndef ATPG_H
#define ATPG_H

#include "sat.h"
#include "node.h"
#include "array.h"
#include "espresso.h"
#include "bdd.h"
#include "list.h"
#include "st.h"
#include "seqbdd.h"

typedef enum stuck_value_enum stuck_value_t;
enum stuck_value_enum { S_A_0, S_A_1 };

typedef enum fault_status_enum fault_status_t;
enum fault_status_enum { REDUNDANT, ABORTED, TESTED, UNTESTED };

typedef enum redund_type_enum redund_type_t;
enum redund_type_enum { CONTROL, OBSERVE };

typedef struct {
  node_t *node;
  int index;
} node_index_t;

typedef struct {
  array_t *vectors;
  int n_covers;
  int index;
} sequence_t;

typedef struct fault_struct fault_t;
struct fault_struct {
  node_t *node;    /* node on which fault occurs */
  node_t *fanin;   /* input connection, NIL if output */
  int index;       /* index of fanin for input fault */
  bool is_covered; /* initially 0; set to 1 if covered */
  stuck_value_t value;
  fault_status_t status;
  redund_type_t redund_type; /* Used in redundancy removal.
                                     CONTROL: fault cannot be excited
                                              (SNE fault)
                                     OBSERVE: fault can be excited, but
                                              effect cannot be observed
                                              at a primary output (ND fault)
                                Removing a CONTROL redundancy does not change
                                     reachable states or output functions.
                                Removing an OBSERVE redundancy may change
                                     reachable states and output functions   */
  sequence_t *sequence;      /* which sequence covers fault */
  int sequence_index;        /* which sequence detects fault,
                              * when using parallel sequence simulation */
  unsigned *current_state;   /* saves 32 states in parallel for use in
                              * parallel pattern simulation */
};

typedef struct fault_pattern_struct fault_pattern_t;
struct fault_pattern_struct {
  node_t *node;
  node_t *fanin;
  unsigned value;
};

typedef void (*VoidFN)();

/* simulation data structures */
typedef struct atpg_sim_node_struct atpg_sim_node_t;
struct atpg_sim_node_struct {
  node_t *node;
  int nfanout;
  int *fanout;
  int uid;
  node_function_t type;
  unsigned value;
  unsigned *or_input_masks;
  unsigned *and_input_masks;
  unsigned or_output_mask;
  unsigned and_output_mask;
  VoidFN eval;
  int n_inputs;
  unsigned **fanin_values;
  int n_cubes;
  int **function;    /* function[cube][input] is 0, 1, 2 */
  unsigned **inputs; /* inputs[0] is negative,
                        inputs[1] is inputs,
                        input[2] is 0xffffffff */
  int visited;
};

typedef struct {
  bool quick_redund;
  bool reverse_fault_sim;
  bool use_internal_states;
  bool PMT_only;
  int n_sim_sequences;
  bool deterministic_prop;
  bool random_prop;
  int rtg_depth;
  int prop_rtg_depth;
  int n_random_prop_iter;
  bool fault_simulate;
  bool fast_sat;
  bool rtg;
  bool build_product_machines;
  bool tech_decomp;
  int timeout;
  int verbosity;
  bool print_sequences;
  char *real_filename;
  FILE *fp;
  bool force_comb;
} atpg_options_t;

typedef struct {
  long setup;
  long traverse_stg;
  long RTG;
  long SAT_clauses;
  long SAT_solve;
  long justify;
  long ff_propagate;
  long random_propagate;
  long fault_simulate;
  long product_machine_verify;
  long reverse_fault_sim;
  long total_time;
} time_info_t;

typedef struct {
  int initial_faults;
  int stg_depth;
  int n_RTG_tested;
  int sat_red;
  int verified_red;
  int n_random_propagated;
  int n_not_ff_propagated;
  int n_ff_propagated;
  int n_just_reused;
  int n_prop_reused;
  int n_random_propagations;
  int n_det_propagations;
  int n_untested_by_main_loop;
  int n_verifications;
  int n_vectors;
  int n_sequences;
} statistics_t;

typedef struct {
  bool seq;                 /* false if the circuit has no latches */
  atpg_options_t *atpg_opt; /* all user options */
  time_info_t *time_info;   /* stores stats about execution time */
  statistics_t *statistics; /* stores execution statistics */
  lsList faults;            /* fault list */
  lsList tested_faults;
  lsList redundant_faults;
  lsList untested_faults;
  lsList final_untested_faults;
  st_table *sequence_table; /* test sequences */
  st_table *redund_table;
  st_table *control_node_table;
  int n_pi;
  int n_po;
  int n_real_pi;
  int n_real_po;
  int n_latch;
  network_t *network;
} atpg_info_t;

typedef struct {
  int n_nodes;
  int n_pi;
  int n_po;
  int n_real_pi;
  int n_real_po;
  int n_latch;
  network_t *network;
  sat_t *atpg_sat;              /* sat structure */
  array_t *sat_input_vars;      /* pi's used in sat call */
  atpg_sim_node_t *sim_nodes;   /* simulation nodes  */
  st_table *pi_po_table;        /* pi-po reference index */
  int *pi_uid;                  /* uid for pi's */
  int *po_uid;                  /* uid for po's */
  unsigned *reset_state;        /* reset state used in faultsim */
  array_t *word_vectors;        /* tmp space - faultsim */
  array_t *prop_word_vectors;   /* tmp space for random propagation -
                                                        faultsim */
  unsigned *real_po_values;     /* tmp space - faultsim */
  unsigned *true_value;         /* tmp space - faultsim */
  unsigned *true_state;         /* tmp space - faultsim */
  unsigned *faulty_state;       /* tmp space - faultsim */
  unsigned *all_true_value;     /* tmp space - faultsim */
  unsigned *all_po_values;      /* tmp space - faultsim */
  int *used;                    /* tmp space - faultsim */
  sequence_t **alloc_sequences; /* tmp space - faultsim */
  fault_t **faults_ptr;         /* tmp space - faultsim */
  int *changed_node_indices;    /* tmp space - faultsim */
  int *tfo;
} atpg_ss_info_t;

typedef struct {
  bool product_machine_built;
  verif_options_t *seq_opt;
  verif_options_t *seq_product_opt;
  range_data_t *range_data;
  range_data_t *product_range_data;
  network_t *network_copy;
  network_t *product_network;
  bdd_t *start_states;                   /* for short_tests */
  bdd_t *product_start_states;           /* for short_tests */
  bdd_t *start_state_used;               /* for short_tests */
  array_t *latch_to_pi_ordering;         /* used in short_tests */
  array_t *latch_to_product_pi_ordering; /* used in short_tests */
  array_t *orig_external_outputs;
  array_t *orig_transition_outputs;
  array_t *reached_sets;
  array_t *product_reached_sets;
  st_table *state_sequence_table; /* for short_tests */
  st_table *just_sequence_table;
  st_table *prop_sequence_table;
  array_t *just_sequence;        /* tmp space - justification */
  array_t *input_trace;          /* tmp space - justification */
  array_t *prop_sequence;        /* tmp space - propagation */
  array_t *real_pi_bdds;         /* used in justification */
  st_table *var_table;           /* used in justification */
  array_t *input_vars;           /* used in justification */
  array_t *product_real_pi_bdds; /* used in PMT */
  st_table *product_var_table;   /* used in PMT */
  array_t *product_input_vars;   /* used in PMT */
  bdd_t *justified_states;
  array_t *good_state;
  array_t *faulty_state;
  network_t *valid_states_network;
} seq_info_t;

/* com_atpg.c */
int com_atpg();

/* com_short_tests.c */
int com_short_tests();

/* com_redund.c */
int com_redundancy_removal();
int st_fpcmp();
int st_fphash();

/* atpg_clauses.c */
int atpg_network_fault_clauses();
void atpg_node_clause();
void atpg_setup_clause_info();
void atpg_clause_info_free();
void atpg_sat_clause_begin();
void atpg_sat_clause_end();

/* atpg_util.c */
bool bdd_is_cube();
double tmg_compute_optimal_clock();
void concat_lists();
void create_just_sequence();
void atpg_print_fault();
void atpg_print_vectors();
void atpg_print_some_vectors();
void atpg_print_bdd();
void atpg_print_results();
void atpg_derive_excitation_vector();
int *fanin_dfs_sort();
node_t **sat_fanout_dfs_sort();
sequence_t *derive_test_sequence();
void reset_word_vectors();
void lengthen_word_vectors();
void fillin_word_vectors();

/* atpg_faults.c */
void atpg_gen_faults();
void atpg_gen_node_faults();
fault_t *new_fault();
void free_fault();

/* atpg_faultsim.c */
lsList atpg_seq_single_fault_simulate();
lsList atpg_random_cover();
int random_propagate();
void fault_simulate();
void atpg_simulate_pattern_fault();
int get_min_just_sequence();
void simulate_entire_sequence();
bool atpg_verify_test();
lsList seq_single_sequence_simulate();
void fault_simulate_to_get_final_state();
void atpg_simulate_old_sequences();
void extract_test_sequences();
void reverse_fault_simulate();
void atpg_sf_set_sim_masks();
void atpg_sf_reset_sim_masks();
void atpg_set_sim_masks();
void atpg_reset_sim_masks();
void extract_sequences();

/* atpg_init.c */
int set_atpg_options();
atpg_info_t *atpg_info_init();
atpg_ss_info_t *atpg_sim_sat_info_init();
seq_info_t *atpg_seq_info_init();
void atpg_setup_seq_info();
void atpg_product_setup_seq_info();
void atpg_sim_setup();
void atpg_comb_sim_setup();
void atpg_exdc_sim_link();
void atpg_sat_init();
void atpg_sat_node_info_setup();
void print_and_destroy_sequences();
void atpg_sim_unsetup();
void atpg_comb_sim_unsetup();
void atpg_sim_free();
void atpg_sat_free();
void seq_info_free();
void seq_info_product_free();
void atpg_free_info();

/* atpg_generate_test.c */
sequence_t *generate_test();
sequence_t *generate_test_using_verification();

/* atpg_seq.c */
void seq_setup();
void seq_product_setup();
void copy_orig_bdds();
void record_reset_state();
void construct_product_start_states();
bool calculate_reachable_states();
bdd_t *seq_derive_excitation_states();
int seq_reuse_just_sequence();
int seq_state_justify();
int internal_states_seq_state_justify();
int seq_reuse_prop_sequence();
int seq_fault_free_propagate();
int traverse_product_machine();
int good_faulty_PMT();

/* atpg_seq_util.c */
void free_bdds_in_array();
network_t *convert_bdd_to_network();
int convert_bdd_to_int();
int convert_product_bdd_to_key();
bdd_t *convert_state_to_bdd();
st_table *get_pi_to_var_table();
bdd_t *seq_get_one_minterm();
bdd_t *find_good_constraint();
void use_cofactored_set();
void bdd_add_varids_to_table();
bdd_t *convert_states_to_product_bdd();
int derive_prop_key();
int derive_inverted_prop_key();

/* atpg_comb.c */
sequence_t *derive_comb_test();
lsList atpg_comb_single_fault_simulate();
void atpg_comb_simulate_old_sequences();

typedef struct {
  int true_id;
  int fault_id;
  int active_id;
  int current_id; /* either true_id, fault_id */
  int *fanin;
  node_t **fanout;
  int nfanin;
  int nfanout;
  int order;
  int visited;
  int tmp;
} atpg_clause_t;

#define ATPG_CLAUSE_GET(node) ((atpg_clause_t *)node->atpg)
#define ATPG_CLAUSE_SET(node, id) node->atpg = (char *)id
#define ATPG_GET_VARIABLE(n) (ATPG_CLAUSE_GET(n)->current_id)

#define GET_ATPG_ID(node) ((int)node->simulation)
#define SET_ATPG_ID(node, id) node->simulation = (char *)id

int init_atpg();

int end_atpg();

#endif // ATPG_H