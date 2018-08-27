
/*
 * Data structure holding the values that are used throught
 * this package
 */
typedef struct speed_global_struct {
  int area_reclaim;     /* Do resub when set */
  int trace;            /* When set, show progress */
  int debug;            /* Set the level of debugging */
  int add_inv;          /* Ensure positive phase for all inputs */
  int del_crit_cubes;   /* Delete the critical cubes from divisors */
  int num_tries;        /* Number of alternatives during extraction */
  double thresh;        /* Set the "epsilon" for criticality */
  double crit_slack;    /* Sets the absolute slack that is critical */
  double coeff;         /* Area/delay tradeoff coefficient */
  int dist;             /* Distance to collapse */
  int do_script;        /* Range over some distances */
  int speed_repeat;     /* Loop at a particular distance */
  int only_init_decomp; /* Only initial 2-ip decomp */
  int interactive;      /* Whether interactive or batch */

  delay_model_t model;        /* Delay model to use */
  int library_accl;           /* FLAG: Use accelrator for the mapped model*/
  double pin_cap;             /* Capacitance of the input pin */
  delay_pin_t nand_pin_delay; /* Delay model for the 2-input primitives */
  delay_pin_t inv_pin_delay;  /* Delay model for the invertor */

  int req_times_set;    /* Set if the req times are set at the PO's */
  int region_flag;      /* NEW: Sets  strategy to select fanin-region */
  int transform_flag;   /* How to choose best transform at  node  */
  int max_recur;        /* NEW: Maximum number of recursion levels */
  int timeout;          /* NEW: time limit for an optimization */
  int new_mode;         /* NEW: set the expensive weight computation */
  int red_removal;      /* NEW: run RR after  successful iteration */
  int objective;        /* NEW: Selection strategy (either area or
                      the number of transforms as a criterion */
  int max_num_cuts;     /* # of cutsets to attempt for each output */
  array_t *local_trans; /* NEW: local transformations available */
} speed_global_t;

typedef void (*SP_void_fn)();

typedef network_t *(*SP_network_fn)();

typedef double (*SP_double_fn)();

typedef delay_time_t (*SP_delay_fn)();

/*
 * Data structure to support the different restructuring possibilities
 */
typedef struct sp_local_trans_struct {
  char *name;
  SP_network_fn optimize_func;
  SP_delay_fn arr_func;
  int priority;
  int on_flag; /* == 1 => transformation is enabled */
  int type;    /* one of CLP, DUAL, FAN */
} sp_xform_t;

/*
 * For the nodes on the mincut --- this stores the relevant information
 * as to what is the best technique, the orignal structure, new structure
 */
typedef struct sp_collapse_struct sp_clp_t;
struct sp_collapse_struct {
  char *name;             /* name of the node --- for referencing */
  node_t *node;           /* original node in the network */
  network_t *net;         /* Copy of the collapsed network */
  network_t *orig_config; /* Original configuration that was collapsed */
  sp_xform_t *ti_model;   /* What optimization to do */
  array_t *delta;         /* Improvement on side in/outputs desired */
  delay_time_t old;       /* Original arr/req time of "node" */
  int cfi;                /* critical-fanin (for buffering) */
  speed_global_t *glb;    /* global partameters */
  int can_adjust;         /* 1 if  inverter at the output can be removed */
  st_table *adjust;       /* Table tracking the adjustments of PI delays */
  st_table *equiv_table;  /* Table tracking corrspondence of nodes */
};

/*
 * When computing the weight of the nodes on the epsilon network, this
 * data-structure stores the relevant information
 */
typedef struct weight_clp_struct {
  network_t *clp_netw;       /* Collapsed node --- distance "dist" */
  network_t *dual_netw;      /* If there is a possibility of dual */
  network_t *fanout_netw;    /* The fanout configuration */
  network_t *best_netw;      /* The best config. at this node !!! */
  delay_time_t arrival_time; /* Original arrival time at o/p of root */
  delay_time_t slack;        /* Original slack */
  int cfi;                   /* index of Critical fanin of root node */
  double cfi_load;           /* Initial load of the critical input */
  delay_time_t cfi_req;      /* Required time of critical input */
  double load;               /* Original load that is driven */
  double orig_area;          /* Area of the circuit being collapsed */
  double dup_area;           /* Area duplication on collapsing */
  double *improvement;       /* Reduction in delay due to technique "i" */
  double crit_slack;         /* Threshold used during this computation  */
  double *area_cost;         /* Area cost of the transformation */
  int best_technique;        /* The technique to do the decomposition */
  int can_invert;            /* == 1 if clp_netw function can be inverted */
  double epsilon;            /* The epsilon improvement at this node */
  int select_flag;           /* Set if the transformation helps */
} sp_weight_t;

/*
 * Routines used inside the new_speed part of package
 */

/*
#define speed_network_dup network_dup
*/
extern network_t *speed_network_dup();

extern int new_speed();

extern int nsp_critical_edge();

extern int nsp_first_slack_diff();

extern int new_speed_is_fanout_po();

extern int sp_num_active_local_trans();

extern int sp_num_local_trans_of_type();

extern void sp_append_network();

extern void sp_delete_network();

extern void sp_print_network();

extern void nsp_free_buf_param();

extern void speed_reorder_cutset();

extern void sp_expand_selection();

extern void sp_print_local_trans();

extern void sp_find_output_arrival_time();

extern void speed_restore_required_times();

extern void new_speed_adjust_po_arrival();

extern void sp_patch_fanouts_of_node();

extern void new_speed_compute_weight();

extern void sp_print_delay_data();

extern void sp_free_local_trans();

extern void new_free_weight_info();

extern void sp_free_collapse_record();

extern double sp_compute_duplicated_area();

extern array_t *sp_get_local_trans();

extern array_t *new_speed_select_xform();

extern delay_time_t *sp_compute_side_req_time();

extern delay_time_t nsp_compute_delay_saving();

extern sp_xform_t *sp_local_trans_from_index();

extern sp_clp_t *sp_create_collapse_record();

extern st_table *speed_store_required_times();

extern network_t *sp_get_network_to_collapse();

extern network_t *buf_get_fanout_network();

extern network_t *nsp_get_dual_network();

/* network the interface to the various restructuring techniques */
extern network_t *sp_and_or_opt();

extern network_t *sp_noalg_opt();

extern network_t *sp_divisor_opt();

extern network_t *sp_2c_kernel_opt();

extern network_t *sp_comp_div_opt();

extern network_t *sp_comp_2c_opt();

extern network_t *sp_bypass_opt();

extern network_t *sp_cofactor_opt();

extern network_t *sp_fanout_opt();

extern network_t *sp_repower_opt();

extern network_t *sp_duplicate_opt();

extern network_t *sp_dual_opt();

extern delay_time_t new_delay_arrival();

extern delay_time_t new_delay_required();

extern delay_time_t new_delay_slack(); /* like delay_latest_output() */
extern delay_time_t new_sp_delay_arrival();

/*
 * Function declarations as extern in the package
 */
extern int speed_weight();

extern int com__speed_plot();

extern int speed_and_decomp();

extern int speed_init_decomp();

extern int speed_delay_trace();

extern int sp_po_req_times_set();

extern int speed_and_or_decomp();

extern int speed_decomp_network();

extern int speed_get_library_accl();

extern int speed_update_arrival_time();

extern int speed_buffer_recover_area();

extern int nsp_downsize_non_crit_gates(); /* wrapper routine */
extern int speed_is_fanout_po();

extern void speed_absorb();

extern void speed_absorb_array();

extern void speed_up_node();

extern void speed_up_loop();

extern void speed_up_script();

extern void speed_up_network();

extern void set_speed_thresh();

extern void speed_node_replace();

extern void speed_adjust_phase();

extern void nsp_get_orig_edge();

extern void speed_update_fanout();

extern void speed_set_library_accl();

extern void speed_set_arrival_time();

extern void speed_plot_crit_network();

extern void speed_reset_arrival_time();

extern void speed_del_critical_cubes();

extern void speed_delay_arrival_time();

extern void speed_single_level_update();

extern void speed_network_delete_node();

extern void speed_delete_single_fanin_node();

extern double sp_get_netw_area();

extern bool speed_critical();

extern node_t *name_to_node();

extern node_t *sp_minimum_slack();

extern node_t *node_del_two_cubes();

extern node_t *speed_dec_node_cube();

extern node_t *speed_node_conditional();

extern node_t *nsp_network_find_node();

extern array_t *speed_decomp();

extern array_t *network_to_array();

extern array_t *sp_generate_revised_order();

extern array_t *network_and_node_to_array();

extern st_table *speed_levelize_crit();

extern st_table *speed_compute_weight();

extern network_t *speed_network_create_from_node();

extern lib_gate_t *sp_get_gate();

extern lib_gate_t *sp_lib_get_inv();

extern lib_gate_t *sp_lib_get_buffer();

/*
 * MACRO definitions for the speedup package
 */
#define D_MIN(a, b) ((double)(a) < (double)(b) ? (double)(a) : (double)(b))
#define D_MAX(a, b) ((double)(a) > (double)(b) ? (double)(a) : (double)(b))

/*
 * Definition of some constants
 */
#define MIN_AREA_BUF_NAME ""

/* Flag values determining how to select the region to be transformed */
#define ALONG_CRIT_PATH 0
#define TRANSITIVE_FANIN 1
#define COMPROMISE 2
#define ONLY_TREE 3

/* Constants identifying the kind of network to optimize */
#define CLP 0
#define FAN 1
#define DUAL 2

/* Flag to determine the objective fn to be used for selection */
#define AREA_BASED 0
#define TRANSFORM_BASED 1

/* Flag to determine how to select the best transform at a node */
#define BEST_BENEFIT 0
#define BEST_BANG_FOR_BUCK 1

#define DEFAULT_SPEED_THRESH 0.5
#define DEFAULT_SPEED_COEFF 0.0
#define DEFAULT_SPEED_DIST 3

#define NSP_EPSILON 1.0e-6       /* For floating point comparisons */
#define NSP_INPUT_SEPARATOR '#'  /* For naming of inputs of region*/
#define NSP_OUTPUT_SEPARATOR '%' /* For naming of output of region*/

#ifndef POS_LARGE
#define POS_LARGE 10000
#endif

#ifndef NEG_LARGE
#define NEG_LARGE -10000
#endif

#define MAXWEIGHT 1000

/*
 * Definitions of mathematical constants
 */
#define SP_PI 3.14159265358979323846
#define SP_PI_2 1.57079632679489661923
#define SP_PI_4 0.78539816339744830962
#define SP_1_PI 0.31830988618379067154

/*
 * Definition of some macros for printing etc.
 */

#define SP_PRINT(global, cur, best, cur_area, best_area, cur_name, best_name)  \
  (void)fprintf(sisout, "\t%s  %5.2f -> %5.2f Area %.1f -> %.1f  %s -> %s\n",  \
                ((global)->req_times_set ? "Slack" : "Delay"), cur, best,      \
                cur_area, best_area, cur_name, best_name);

#define SP_IMPROVED(global, best, cur)                                         \
  (((global)->req_times_set) ? ((best) > ((cur) + NSP_EPSILON))                \
                             : ((best) < ((cur)-NSP_EPSILON)))

#define SP_GET_PERFORMANCE(global, network, node, value, name, area)           \
  node = ((global)->req_times_set ? sp_minimum_slack((network), &value)        \
                                  : delay_latest_output((network), &value));   \
  name = (node != NIL(node_t) ? util_strsav(node_name(node)) : "");            \
  area = sp_get_netw_area(network)

#define SP_METHOD(flag)                                                        \
  (flag == ALONG_CRIT_PATH                                                     \
       ? "CRITICAL"                                                            \
       : (flag == COMPROMISE                                                   \
              ? "COMPROMISE"                                                   \
              : (flag == ONLY_TREE ? "TREE"                                    \
                                   : (flag == TRANSITIVE_FANIN ? "TRANSITIVE"  \
                                                               : "UNKNOWN"))))

#define NSP_OPTIMIZE(wght, xform, glb)                                         \
  (xform->type == FAN                                                          \
       ? ((*(xform->optimize_func))(wght->fanout_netw, xform, glb))            \
       : (xform->type == DUAL                                                  \
              ? ((*(xform->optimize_func))(wght->dual_netw, xform, glb))       \
              : ((*(xform->optimize_func))(wght->clp_netw, xform, glb))))

#define NSP_NETWORK(wght, xform)                                               \
  (xform->type == FAN                                                          \
       ? (wght->fanout_netw)                                                   \
       : (xform->type == DUAL ? (wght->dual_netw) : (wght->clp_netw)))

#define sp_network_free(network)                                               \
  {                                                                            \
    if ((network) != NIL(network_t))                                           \
      network_free((network));                                                 \
    (network) = NIL(network_t);                                                \
  }

#define SP_IMPROVEMENT(wght)                                                   \
  ((wght)->best_technique == -1                                                \
       ? (NEG_LARGE)                                                           \
       : ((wght)->improvement[(wght)->best_technique]))
#define SP_COST(wght)                                                          \
  ((wght)->best_technique == -1 ? (POS_LARGE)                                  \
                                : ((wght)->area_cost[(wght)->best_technique]))
