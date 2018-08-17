
#include "retime.h"

#define RETIME_SLOT undef1
#define RETIME(node)     ((int) node->RETIME_SLOT)

#define re_empty_network(n) ((network_num_po(n)+network_num_pi(n)) == 0)

typedef struct class_data re_class_t;
struct class_data {
    array_t *gates;    /* Array of type (lib_gate_t *) */
    array_t *delay;    /* Array of type double		*/
};

typedef struct wd_struct wd_t;
struct wd_struct {
    int w;
    int d;
};

/* Global variables */
extern int retime_debug;

/* Extern function declarations */
extern void re_computeWD();

extern void retime_dump_graph();

extern void retime_single_node();

extern void re_graph_add_node();

extern void re_evaluate_delay();

extern int re_simplx();

extern int retime_check_graph();

extern int retime_min_register();        /* re_minreg.c */
extern int retime_nanni_routine();        /* re_nanni.c */
extern int retime_lies_routine();        /* re_milp.c */
extern int retime_update_init_states();

extern int retime_get_dff_info();

extern int retime_get_clock_data();

extern node_t *retime_network_latch_end();    /* utility */
extern double retime_simulate_gate();

extern double retime_cycle_lower_bound();

extern array_t *re_graph_dfs();

extern array_t *re_graph_dfs_from_input();

extern re_node *retime_alloc_node();

extern re_edge *re_graph_add_edge();

#define RETIME_DEFAULT_REG_AREA 1.0    /* Area of a latch */
#define RETIME_DEFAULT_REG_DELAY 0.0    /* Delay through a latch */
#define MAX_LATCH_TO_INITIALIZE 16 /* No of latches acceptable to extract STG */

#define RETIME_NOT_SET -1
#define RETIME_USER_NOT_SET -100000.0
#define RETIME_TEST_NOT_SET -50000.0    /* Half of the USER_NOT_SET value */

#define RETIME_DEF_TOL    0.1    /* Tolerence of the binary search */

#define POS_LARGE 10000
#define NEG_LARGE -10000
