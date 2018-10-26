#ifndef RETIME_INT_H
#define RETIME_INT_H

#include "retime.h"

#define RETIME_SLOT undef1
#define RETIME(node) ((int)node->RETIME_SLOT)

#define re_empty_network(n) ((network_num_po(n) + network_num_pi(n)) == 0)

typedef struct class_data re_class_t;
struct class_data {
    array_t *gates; /* Array of type (lib_gate_t *) */
    array_t *delay; /* Array of type double		*/
};

typedef struct wd_struct wd_t;
struct wd_struct {
    int w;
    int d;
};

/* Global variables */
int retime_debug;

/* Extern function declarations */
void re_computeWD();

void retime_dump_graph();

void retime_single_node();

void re_graph_add_node();

void re_evaluate_delay();

int re_simplx(double **a, int m, int n, int m1, int m2, int m3, int *icase, int izrov[], int iposv[]);

int retime_check_graph();

int retime_min_register();  /* re_minreg.c */
int retime_nanni_routine(); /* re_nanni.c */
int retime_lies_routine();  /* re_milp.c */
int retime_update_init_states();

int retime_get_dff_info();

int retime_get_clock_data();

node_t *retime_network_latch_end(); /* utility */
double retime_simulate_gate();

double retime_cycle_lower_bound();

array_t *re_graph_dfs();

array_t *re_graph_dfs_from_input();

re_node *retime_alloc_node();

re_edge *re_graph_add_edge();

#define RETIME_DEFAULT_REG_AREA 1.0  /* Area of a latch */
#define RETIME_DEFAULT_REG_DELAY 0.0 /* Delay through a latch */
#define MAX_LATCH_TO_INITIALIZE 16 /* No of latches acceptable to extract STG  \
                                    */

#define RETIME_NOT_SET -1
#define RETIME_USER_NOT_SET -100000.0
#define RETIME_TEST_NOT_SET -50000.0 /* Half of the USER_NOT_SET value */

#define RETIME_DEF_TOL 0.1 /* Tolerence of the binary search */

#define POS_LARGE 10000
#define NEG_LARGE -10000

#endif