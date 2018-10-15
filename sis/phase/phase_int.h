#ifndef PHASE_INT_H
#define PHASE_INT_H

#include "array.h"
#include "node.h"

typedef struct node_phase_struct node_phase_t;
struct node_phase_struct {
    sm_row *row;
};

typedef struct net_phase_struct net_phase_t;
struct net_phase_struct {
    sm_matrix *matrix;
    array_t   *rows;
    double    cost;
};

typedef struct row_data_struct row_data_t;
struct row_data_struct {
    int    pos_used;     /* number of times the positive phase is used */
    int    neg_used;     /* number of times the positive phase is used */
    int    inv_save;     /* number of inverters saved when inverting the node */
    bool   marked;      /* stamp used by good-phase */
    bool   invertible;  /* whether the node is invertible */
    bool   inverted;    /* whether the node is inverted */
    bool   po;          /* whether the node fans out to a PO */
    node_t *node;     /* points to the node associate with the row. */
    double area;      /* area of the gate */
    double dual_area; /* area of the dual gate */
};

typedef struct element_data_struct element_data_t;
struct element_data_struct {
    int phase; /* 0 - pos. unate, 1 - neg_ungate, 2 - binate */
};

net_phase_t *phase_setup();

int invert_saving();

void phase_node_invert();

node_phase_t *phase_get_best();

double network_cost();

void phase_unmark_all();

void phase_mark();

net_phase_t *phase_dup();

void phase_replace();

void phase_free();

void phase_record();

void phase_check_unset();

void phase_check_set();

void phase_random_assign();

void phase_invert();

double phase_value();

double cost_comp();

void phase_invertible_set();

bool phase_trace;
bool phase_check;

#endif