
#include "pld.h"
/* definitions for actel */
#define LOW 0
#define HIGH 1
#define AREA 0.0
#define DELAY 1.0
#define UNORDERED 0
#define ORDERED 1
#ifndef NULL
#define NULL 0
#endif
#ifndef FALSE
#define FALSE 0
#endif
#define HICOST 100000
#define DELETE 3
#define NO_VALUE 4
#define FANIN 0
#define RANDOM 1
#define OPTIMAL 2
#define COMPOSE 0
#define ALAP 3
#define HEURISTIC 4
#define MAXSTYLES 6
#define MAXTYPES 5
/* #define MAXOPTIMAL 6  changed from 7 to 6 */
#define AND     0
#define OR      4
#define P11     0
#define P10     1
#define P01     2
#define P00     3
#define XOR    8
#define XNOR   9
#define min(A, B) ( (A) < (B) ? (A) : (B) )
#undef equal

#define PLD_SLOT    pld
#define ACT_SET(node)    ((act_type_t *) (node)->PLD_SLOT)
#define ACT_GET(node)    ((node)->PLD_SLOT)
#define GLOBAL_ACT act_num[0]
#define LOCAL_ACT act_num[1]

#define A_F 1.0;
#define S_F 0.0;
#define ACT_MAP_NAME   actel_map
/*    DAG vertex	*/
typedef struct act_vertex_defn {
    struct act_vertex_defn *low, *high;
    int                    index;
    int                    value;
    int                    id;
    int                    mark;
    int                    index_size;
    node_t                 *node;
    char                   *name;
    int                    multiple_fo;
    int                    cost;
    int                    pattern_num;                      /* for the pattern_matched at a node */
    int                    mapped;
    struct act_vertex_defn *parent; /* delete later*/
    char                   *fn_type; /* delete later */
    int                    my_type;              /* ORDERED or UNORDERED */
    double                 arrival_time;
    int                    multiple_fo_for_mapping; /* for mapping */
} ACT_VERTEX, *ACT_VERTEX_PTR;

#define act_t ACT_VERTEX

typedef struct cost_struct_defn {
    int            cost;        /* number of basic blocks used to realize the node */
    char           *fn_type;           /* a "hacked" field to get bdnet file right */
    ACT_VERTEX_PTR act;    /* stores the bdd (act) for the function at the node */
    node_t         *node;           /* pointer to the node of the network */
    double         arrival_time;    /* arrival_time at the output of the node */
    double         required_time;    /* required_time at the output of the node */
    double         slack;        /* slack = required_time - arrival_time */
    int            is_critical;    /* is the slack <= some threshold */
    double         area_weight;    /* penalty if node is collapsed */
    double         cost_and_arrival_time;
} COST_STRUCT;

/*	ACT with root, name, and order_list	*/
typedef struct act_defn {
    ACT_VERTEX_PTR root;
    array_t        *node_list;
    node_t         *node;
    char           *node_name;
} ACT, *ACT_PTR;

/*	ACT and order style	*/
typedef struct act_entry_defn {
    ACT_PTR act;
    int     order_style;
} ACT_ENTRY, *ACT_ENTRY_PTR;

/*	local and global ACTs	*/
typedef struct act_slot_defn {
/*		COST_STRUCT *cost_node;*/  /* added Aug 9 for delay */
    ACT_ENTRY_PTR act_num[2];
} act_type_t;

/*	used in act2ntwk and reduce	*/
typedef struct key_defn {
    int low, high;
    int low_sign, high_sign;
} KEY;

typedef struct queue_node_defn {
    KEY            key;
    ACT_VERTEX_PTR v;
} Q_NODE, *Q_NODE_PTR;

/* init_param structure to put all the options */
typedef struct act_init_param_defn {
    int   HEURISTIC_NUM;  /* which act to construct */
    int   NUM_ITER;       /* number of iterations in iter. improvement */
    int   FANIN_COLLAPSE; /* upper limit on fanins of a node to collapse */
    float GAIN_FACTOR;  /* go from one iteration to other only if gain increases
                                  by this ratio */
    int   DECOMP_FANIN;   /* lower limit on fanins of node to decompose */
    int   DISJOINT_DECOMP;/* do disjoint decomposition before mapping */
    int   QUICK_PHASE;    /* do phase-assignment */
    int   LAST_GASP;      /* make network out of node and do iterations on it */
    int   BREAK;     /* make the final network in terms of basic blocks */
    char  delayfile[500];    /* name of the file with delay numbers for basic block
                                  as a function of number of fanouts */
    float mode;         /* 0 for area and 1 for delay mode, in between, a
                                  weighted sum */
    /* the following used in ite routines */
    int   COLLAPSE_FANINS_OF_FANOUT; /* u.l. on fanins of a fanout node after collapse */
    int   map_alg;         /* 1 if just algebraic matching used in act_bool.c */
    int   lit_bound;       /* decompose the node if it has more than these literals */
    int   ITE_FANIN_LIMIT_FOR_BDD; /* construct an robdd (along with ite) if the node has
                                        at most these many fanins */
    int   COST_LIMIT;      /* collapse a node into its fanout only if the cost of the node
                                is at most this much */
    int   COLLAPSE_UPDATE; /* if set to EXPENSIVE, when a node is accepted for collapse,
                                include the fanins of the node, and the fanins of the fanouts for
                                the potential candidates for collapse along with the fanouts of the
                                node, else (INEXPENSIVE) just include the fanouts of the node */
    int   COLLAPSE_METHOD; /* if OLD, then accept a collapse only if the sum of the costs of the
                                new fanouts is less than the cost of the node plus the sum of the 
                                old costs of the fanouts. If NEW, if the cost of a single fanout
                                goes down, the collapse is accepted. Node remains in the network
                                unless the criterion in the OLD is satisfied too               */
    int   DECOMP_METHOD;   /* if USE_GOOD_DECOMP, then use good decomp, get a network, map each
                                node of the network independently and then enter iterative
                                improvement. If USE_FACTOR, then an ite is constructed for the
                                factored form of the node. This factored form is arrived at by
                                decomp -g and tech_decomp -a 2 -o 2.                           */
    int   ALTERNATE_REP;   /* used right now only with ite_map - if 1, use robdd and see if the
                                cost improves. */
    int   MAP_METHOD;      /* if NEW, use the new mapping ite method (Dec. 92) else use
                                old method*/
    int   VAR_SELECTION_LIT; /* if 0, use the old method of selecting the selection variable
                                  at each step of ITE construction. If -1, then at each step, actual
                                  mapping of the algebraic cofactors is done to get a better 
                                  estimate of the cost. Else it calls the new
                                  method. If number of literals in a function is greater than
                                  this number, weight assignment to each input is based on
                                  three criterion: see ite_new_urp.c                        */

} act_init_param_t;

/* just to store the fanout and arrival time info before and after a transformation */
typedef struct temp_struct_defn {
    double old_arrival_time;
    int    old_num_fanouts;
    int    new_num_fanouts;
    node_t *node;
} TEMP_STRUCT;

/* stores info about the node and its fanout that we have to collapse into.
   names are stored because on collapsing, fanout node is deleted and is replaced
   by a new node of the same name. */
typedef struct collapsible_pair_defn {
    char   *nodename;
    char   *fanoutname;
    double weight;  /* reflects the total gain out of collapsing */
} COLLAPSIBLE_PAIR;

/* for freeing collapsed node from the network and cost_table */
typedef struct argument_defn {
    st_table  *cost_table;
    network_t *network;
} ARGUMENT;

/* update the arrival time info for collapsing only in topol.
   order of fanins. This structure associates an index with a 
   nodename and hence allows sorting.			   */
typedef struct topol_defn {
    char *nodename;
    int  index;
} TOPOL_STRUCT;

/* to associate a vertex of the bdd with the node */
typedef struct vertex_node_defn {
    ACT_VERTEX_PTR vertex;
    node_t         *node;
} VERTEX_NODE;

/*	global	*/

array_t         *index_list_array;
array_t         *index_list_array2;
array_t         *global_lists;

extern void p_actAlloc();            /* act_init.c	*/
extern void p_actFree();            /* act_init.c	*/
extern void p_actfree();            /* act_init.c	*/
extern void p_actDup();                /* act_init.c	*/
extern int p_actRemove();            /* act_remove.c	*/
extern void p_actDestroy();            /* act_remove.c	 */
extern void p_dagDestroy();            /* act_remove.c	 */
extern void p_actCreate4Set();

extern void p_addLists();

extern void p_applyCreate();

extern void traverse();             /*	act_trav.c*/
extern ACT_VERTEX_PTR actReduce();            /*act_reduce.c*/

/* act_map_delay.c (act_map.c) */
extern COST_STRUCT *act_evaluate_map_cost();

static decomp_big_nodes();

static improve_network();

static act_quick_phase();

static iterative_improvement();

extern node_t *act_mux_node();

static node_t *basic_block_node();

static node_t *act_get_function();

node_t *get_node_literal_of_vertex();

static node_t *get_node_of_vertex();

void free_node_if_possible();

extern void put_node_names_in_act();

extern void print_vertex();

static int OR_pattern();

extern array_t *OR_literal_order();

extern array_t *single_cube_order();

static int minimum_cost_index();

static int minimum_costdelay_index();

static char *formulate_IP();

static void read_LINDO_file();

static void partial_scan_lindoFile();

extern int is_anyfo_PO();

static int compute_cost_array();

extern int cost_of_node();

static void print_node_with_string();

void set_mark_act();

static void free_nodes_in_act_and_change_multiple_fo();

static void init_bdnet();

static char *act_get_node_fn_type();

static enum st_retval free_table_entry();

extern enum st_retval free_table_entry_without_freeing_key();

extern void free_cost_table_without_freeing_key();

extern ACT_VERTEX_PTR my_create_act();

static ACT_VERTEX_PTR my_create_act_general();

static int all_fanins_positive();

extern int      MARK_VALUE, MARK_COMPLEMENT_VALUE;

extern array_t *act_order_for_delay();

extern int arrival_compare_fn();

extern array_t *act_read_delay(); /*act_read.c */
extern array_t  *vertex_name_array;

/* act_collapse.c */
extern int act_partial_collapse_assign_score_network();

extern int act_partial_collapse_assign_score_node();

extern int act_partial_collapse_node();

extern int act_partial_collapse_without_lindo();

extern network_t *xln_check_network_for_collapsing_area();

/* act_util.c */
extern int is_fanin_of();

extern double my_max();

extern COST_STRUCT *act_allocate_cost_node();

extern ACT_VERTEX_PTR act_allocate_act_vertex();

extern void act_initialize_act_area();

extern void act_initialize_act_delay();

extern VERTEX_NODE *act_allocate_vertex_node_struct();

extern void act_put_nodes();

extern void my_traverse_act();

/*act_dutil.c */
extern void act_delay_trace_forward();

extern void act_delay_trace_backward();

extern double act_get_arrival_time();

extern double act_get_required_time();

extern COST_STRUCT *act_set_arrival_time();

extern COST_STRUCT *act_set_required_time();

extern double act_get_bddfanout_delay();

extern double act_get_node_delay_correction();

extern void act_set_pi_arrival_time_network();

extern COST_STRUCT *act_set_pi_arrival_time_node();

extern void act_invalidate_cost_and_arrival_time();

extern double act_cost_delay();

extern double act_delay_for_fanout();

extern void act_set_slack_network();

extern COST_STRUCT *act_set_slack_node();

extern double act_get_slack_node();

extern void act_find_critical_nodes();

extern array_t *act_compute_area_delay_weight_network_for_collapse();

extern array_t *act_compute_area_delay_weight_node_for_collapse();

extern double act_compute_area_weight_node_for_collapse();

extern COLLAPSIBLE_PAIR *act_allocate_collapsible_pair();

extern double act_smallest_fanin_arrival_time_at_fanout_except_node();

extern double act_largest_fanin_arrival_time_at_node();

extern int c_pair_compare_function();

extern double act_get_max_arrival_time();

extern st_table *act_assign_topol_indices_network();

extern array_t *act_topol_sort_fanins();

extern void act_delete_topol_table_entries();

extern enum st_retval act_delete_topol_table_entry();

/* act_map_delay.c (act_map.c) */
extern void act_init_multiple_fo_array( /* VERTEX_PTR act_of_node */ );

extern int map_act( /* VERTEX_PTR vertex */ );

extern st_table *end_table;                    /* needed in make_bdd() urp_bdd.c*/
extern network_t *act_map_network(); /* act_map.c */
extern network_t *act_break_network(); /*act_map.c */
extern network_t *act_network_remap(); /* act_map.c */
extern void act_act_free();

extern void free_cost_struct();

extern COST_STRUCT *make_tree_and_map();

extern COST_STRUCT *make_tree_and_map_delay();

extern int  WHICH_ACT;    /* act_map.c */
extern int print_network();

extern FILE *BDNET_FILE;  /* com_pld.c */
extern int  ACT_DEBUG;  /*  com_pld.c */
extern int  ACT_STATISTICS;  /*  com_pld.c */

extern int     MAXOPTIMAL;
extern array_t *multiple_fo_array; /* holds the nodes that have multiple_fo in the act */
extern int     WHICH_ACT;
extern ACT_VERTEX *PRESENT_ACT; /* for temporary storage */
extern int num_or_patterns;

/* act_delay.c */
extern array_t *act_order_for_delay();

/* Definitions for Xilinx*/

/* xln_merge and xln_map_par*/
#define MAX_MATCHING    1000
#define BUFSIZE        500

#define SOURCE_NAME    "source"
#define SINK_NAME    "sink"

#define FAST            0
#define GOOD            1
#define ON        1
#define OFF        0
#define YES        1
#define NO        0
#define OK        1
#define UNKNOWN    -1

extern void ULM_decompose_func();

extern void ULM_eval_func();

extern void ULM_create_partition_matrix();

extern void merge_node();

extern void partition_network();

extern void change_edge_capacity();

extern int get_maxflow();

extern char *formulate_Lindo();

extern mf_edge_t *get_maxflow_edge();

extern node_t *graph2network_node();

extern void print_fanin();

extern int comp_ptr();

extern void count_intsec_union();

extern enum st_retval print_table_entry();

extern void print_array();
/* added March 30, 1992 */
/*----------------------*/
extern sm_row *sm_shortest_row();

extern st_table *xln_collapse_nodes_after_merge();

extern sm_row *sm_mat_bin_minimum_cover_my();

extern sm_row *sm_mat_bin_minimum_cover_greedy();

/**********/
/* macros */
/**********/

/* Macro procedure that swaps two entities. */
/*------------------------------------------*/
#define  SWAP(type, a, b)   {type swapx; swapx = a; a = b; b = swapx;}


/* Macro to copy a string: it returns the pointer to the string */
/*--------------------------------------------------------------*/
#define SAVE(ptostring)  strcpy(CALLOC(char,strlen(ptostring)+1),ptostring)

/* Macro for random number generation */
/*------------------------------------*/
#define RAND(a) ( a = ( a * 1103515245 + 12345 ) & MAX_INT ))
#define NORM(n) ( (double) n / (double) MAX_INT )
#define NRAND(a) ( (double) ( a = ( a * 1103515245 + 12345 ) & MAX_INT ) / (double) MAX_INT )

#define EXP(d, t) ( (t) == 0. ? 0. : exp( -(double) (d) / (t) ) )
#define LOG_B(base, x) ( log(x)/log(base) )

#define RINT(a) ( (int) floor( (double) a + 0.5 ) )

/* xln_part.c */
typedef struct a_node_struct {
    float value;
    char  *name;
    char  *fanout;
}          a_node, *a_node_ptr;


typedef struct a_divisor {
    node_t *divisor;
    int    kernelsize;
}          divisor_t;

typedef struct a_kern_node {
    node_t  *node;
    array_t *cost_array;
    int     size; /* added later */
}          kern_node;

/* xln_k_decomp.c and xln_ufind.c*/

typedef struct tree_node {
    int              index;
    struct tree_node *parent;
    int              num_child;
    int              class_num;
}          tree_node;


extern void binary();          /* (value, string) */   /* xln_aux.c */
extern void reverse_string();  /* (answer, givenstring) */  /* xln_aux.c */
extern void xl_binary1();      /*xln_aux.c */
extern int *xln_array_to_indices();  /* xln_aux.c */
extern tree_node *find_tree();  /*xln_ufind.c */
extern int unionop();          /* xln_ufind.c */

/* xln_feasible.c */
extern array_t *xln_get_bound_set();

extern int xln_node_move_fanin();

extern node_t *dec_node_cube();     /* xln_aodecomp.c */
extern int pld_decomp_and_or();      /* xln_part_dec.c */

extern int XLN_DEBUG;
extern int XLN_BEST;
extern int use_complement;

extern xln_kernel_extract();

/* xln_imp.c */
extern network_t *xln_best_script();

extern network_t *xln_cofactor_decomp();

extern node_t *xln_select_fanin_for_cofactor_area();

extern node_t *xln_select_fanin_for_cofactor_delay();

extern array_t *xln_k_decomp_node_with_array();

extern network_t *xln_k_decomp_node_with_network();

/* xln_level.c */
extern network_t *xln_check_network_for_collapsing_delay();

extern array_t *xln_array_of_levels();

extern array_t *xln_array_of_critical_nodes_at_levels();

/* xln_k_decomp_area.c */
extern network_t *xln_exhaustive_k_decomp_node();

/* xln_cube.c */
extern int xln_ao_compare();

/* pld_util.c */
extern pld_replace_node_by_network();

extern pld_remap_init_corr();

extern node_t *pld_remap_get_node();

extern node_t *pld_make_node_from_cube();

extern array_t *pld_nodes_from_cubes();

extern array_t *pld_cubes_of_node();

extern array_t *pld_get_non_common_fanins();

extern int pld_is_node_in_array();

extern int pld_is_fanin_subset();

extern array_t *pld_get_array_of_fanins();

extern st_table *pld_insert_intermediate_nodes_in_table();

extern array_t *sm_get_rows_covered_by_col();

extern array_t *sm_get_cols_covered_by_row();

/* xln_dec_merge.c */
#define ALL_CUBES 1
#define PAIR_NODES 2

extern int pld_affinity_compare_function();

extern array_t *xln_node_find_common_inputs();

extern node_t *xln_extract_supercube_from_cube();

extern array_t *xln_fill_tables();

extern array_t *xln_infeasible_nodes();

/* for storing the pairs of nodes and their affinity */
typedef struct xln_affinity_struct_defn {
    node_t  *node1;     /* first node of the pair */
    node_t  *node2;     /* second node of the pair*/
    array_t *common;   /* array of common fanins of node1 & node2 */
}          AFFINITY_STRUCT;

typedef struct xln_move_struct_defn {
    int MOVE_FANINS;
    int MAX_FANINS;
    int bound_alphas;     /* for delay: this controls the number of functions created */
}          XLN_MOVE_STRUCT;

typedef struct xln_init_param_defn {
    int             support;                   /* for xilinx, support = 5 */
    int             MAX_FANIN;                 /* for two-output blocks - condition for merging*/
    int             MAX_COMMON_FANIN;          /* for two-output blocks - condition for merging*/
    int             MAX_UNION_FANIN;           /* for two-output blocks - condition for merging*/
    int             heuristic;
    int             common_lower_bound;        /* for dec_merge */
    int             cube_support_lower_bound;  /* for dec_merge */
    int             lit_bound;
    int             cover_node_limit;      /* apply exact xl_cover if number of nodes in some
                                  subnetwork (or network) is at most this */
    int             flag_decomp_good;      /* could be 0 (no decomp -g), 1 (decomp -g),  2
                                  (pick the better of the previous two) */
    int             good_or_fast;          /* just apply cube-packing or all decomp techniques */
    int             absorb;                /* use Roth-Karp to move fanins */
    int             num_iter_partition;    /* xl_reduce will call xl_partition these many times */
    int             num_iter_cover;        /* xl_reduce will call xl_cover these many times */
    int             DESPERATE;
    int             RECURSIVE;
    int             MAX_FANINS_K_DECOMP;   /* node considered for Roth-Karp decomp if has at most
                                  these many fanins */
    int             COST_LIMIT;            /* nodes with at most this cost to be collapsed in
                                  partial_collapse routine */
    XLN_MOVE_STRUCT xln_move_struct;
    int             collapse_input_limit;  /* consider total collapse of the network if number
                                  of PI's no more than this */
    int             traversal_method;      /* 1 then topological traversal, else levels sorted
                                  wrt width */
}          xln_init_param_t;
  

