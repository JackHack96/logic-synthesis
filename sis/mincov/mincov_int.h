#ifndef MINCOV_INT_H
#define MINCOV_INT_H

#include "sis.h"

typedef struct stats_struct stats_t;
struct stats_struct {
    int  debug;           /* 1 if debugging is enabled */
    int  max_print_depth; /* dump stats for levels up to this level */
    int  max_depth;       /* deepest the recursion has gone */
    int  nodes;           /* total nodes visited */
    int  component;       /* currently solving a component */
    int  comp_count;      /* number of components detected */
    int  gimpel_count;    /* number of times Gimpel reduction applied */
    int  gimpel;          /* currently inside Gimpel reduction */
    long start_time;     /* cpu time when the covering started */
    int  no_branching;
    int  lower_bound;
};

typedef struct solution_struct solution_t;
struct solution_struct {
    sm_row *row;
    int    cost;
};

solution_t *solution_alloc();

void solution_free();

solution_t *solution_dup();

void solution_accept();

void solution_reject();

void solution_add();

solution_t *solution_choose_best();

solution_t *sm_maximal_independent_set();

solution_t *sm_mincov();

int gimpel_reduce();

#define WEIGHT(weight, col) (weight == NIL(int) ? 1 : weight[col])

/* for binate covering */
typedef struct bin_solution_struct bin_solution_t;
struct bin_solution_struct {
    pset set;
    int  cost;
};

bin_solution_t *bin_solution_alloc();

void bin_solution_free();

bin_solution_t *bin_solution_dup();

void bin_solution_del();

void bin_solution_add();

void sm_bin_mincov();

#endif