/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/atpg/sat.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:17 $
 *
 */
#ifndef SATISFY_H
#define SATISFY_H

#define sat_neg(i) ((i%2)?i-1:i+1)
#define sat_begin_clause(s) s->nclause ++
#define sat_add_literal(s, v) sm_insert(s->matrix, s->nclause - 1, (v))
#define sat_add_1clause(s, v) array_insert_last(int, s->one_clauses, (v))
#define sat_add_2clause(s, v1, v2) \
    (void) sat_add_implication(s, sat_neg((v1)), (v2)); \
    (void) sat_add_implication(s, sat_neg((v2)), (v1))
#define sat_add_3clause(s, v1, v2, v3)  \
    sm_insert(s->matrix, s->nclause, (v1)); \
    sm_insert(s->matrix, s->nclause, (v2)); \
    sm_insert(s->matrix, s->nclause, (v3)); \
    s->nclause ++;

typedef struct {	
    int cl_order;	/* Subformula clause ordering */
    int	var_order;	/* Variable ordering for branching */
    int	bktrack_lim;	/* Limit on backtracks for this phase */
    int	n_static_pass;	/* Number of static GI passes to make */
    bool add_nli;	/* Add nonlocal implications? */
    bool add_unique;	/* Assign values found by contradition?	*/
} sat_strategy_t;

typedef struct {
    sm_matrix *matrix;
    array_t *one_clauses;
    array_t *imply;
    array_t *stk_var;
    array_t *stk_inc;
    array_t *stk_cla;
    sat_strategy_t *strategy;
    int nclause;
    int lit_index;
    int bktrack;
    int n_impl;
    bool gaveup;
} sat_t;

typedef enum sat_result {
    SAT_SOLVED,                 /* Satisfying solution is found.        */
    SAT_ABSURD,                 /* Formula has no satisfying solution.  */
    SAT_GAVEUP                  /* SAT package gave up search.  */
} sat_result_t;

/* interface data structure */
typedef struct {
    int sat_id;
    char *info;
} sat_input_t;

extern sat_t *sat_new();
extern void sat_reset();
extern void sat_delete();
extern int sat_new_variable();
extern bool sat_add_implication();
extern sat_result_t sat_solve();
extern int sat_get_value();
#endif /* SATISFY_H */


