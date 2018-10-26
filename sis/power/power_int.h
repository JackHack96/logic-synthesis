#ifndef POWER_INT_H
#define POWER_INT_H
/*---------------------------------------------------------------------------
|   This is an internal header file for the power estimation package.
|
| Copyright (c) 1991 - Abhijit Ghosh. University of California, Berkeley
|
| Jose' Monteiro, MIT, Jan/93            jcm@rle-vlsi.mit.edu
+--------------------------------------------------------------------------*/

#include "power.h"
#include "bdd.h"
#include "network.h"

#define sizeof_el(thing) (sizeof(thing) / sizeof(thing[0]))

#define BDD_MODE 100
#define SAMPLE_MODE 101

#define DEFAULT_MAX_INPUT_SIZING 4 /* Transistor sizing is assumed only for */
/* gates with 4 or less inputs.          */
#define DEFAULT_PS_MAX_ALLOWED_ERROR 0.01 /* For ps lines probabilities */
#define CAP_IN_LATCH 4                    /* inv+2or */
#define CAP_OUT_LATCH 20 /* 2(nand+or)/2 + 2(2(or+nand)) (internal) */
/* Register from pag 217 Weste-Eshragian */

typedef struct {
    int    delay;       /* The delay of the gate */
    double prob_one; /* Probability of gate being logical one */
} node_info_t;

typedef struct { /* Used in power_sim.c */
    array_t *before_switching;
    array_t *after_switching;
    array_t *switching_times;
} delay_info_t;

typedef struct { /* JCM: information for state probability */
    double probOne;
    int    PSLineIndex;
} power_pi_t;

#ifdef MAIN /* Global variables */
/* Table with switching and capacitance info */
st_table *power_info_table;

short power_verbose;
int power_cap_in_latch;
int power_cap_out_latch;
double power_delta; /* Max allowed error for PS lines probabilities */
int power_setSize;  /* # of PS lines correlated in the direct method */
char *power_dummy;  /* Used in the SWITCH_PROB and CAP_FACTOR macros */

st_table *power_get_node_info(); /* power_main.c */
#else
short  power_verbose;
int    power_cap_in_latch;
int    power_cap_out_latch;
double power_delta;
int    power_setSize;

int power_command_line_interface(); /* power_main.c */
st_table *power_get_node_info();    /* power_main.c */
int power_get_mapped_delay();       /* power_main.c */

int power_comb_static_zero();  /* power_comb.c */
int power_comb_static_arbit(); /* power_comb.c */

int power_dynamic();            /* power_dynamic.c */

#ifdef SIS
int power_pipe_static_arbit();  /* power_pipe.c */
int power_add_pipeline_logic(); /* power_pipe.c */

int power_seq_static_arbit();    /* power_seq.c */
int power_add_fsm_state_logic(); /* power_seq.c */

void power_PS_lines_from_state(); /* power_PSprob_exact.c */
double *power_exact_state_prob(); /* power_PSprob_exact.c */

double *power_direct_PS_lines_prob(); /* power_PSprob_direct.c*/
void power_place_PIs_last();          /* power_PSprob_direct.c*/

double power_calc_func_prob_w_stateProb(); /* power_comp.c */
double power_calc_func_prob_w_sets();      /* power_comp.c */
#endif /* SIS */

double power_calc_func_prob(); /* power_comp.c */

network_t *power_symbolic_simulate(); /* power_sim.c */

array_t *power_network_dfs(); /* power_util.c */
void power_network_print();   /* power_util.c */
pset power_lines_in_set();    /* power_util.c */

bdd_node *cmu_bdd_zero(); /* Taken directly from the CMU package */
bdd_node *cmu_bdd_one();

long cmu_bdd_if_index();

#endif
#endif