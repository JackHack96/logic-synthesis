#include "sis.h"
#include "sat_int.h"
#include "fast_avl.h"

/* Based on original SAT package written by Paul Stephan */

#define array_reset(a) ((a)->num = 0)
#define ASIZE 8
#define N_UNBOUND(r) ((int)(r)->flag)
#define DEC_UNBOUND(r) ((r)->flag = ((r)->flag - 1))
#define INC_UNBOUND(r) ((r)->flag = ((r)->flag + 1))
#define INV_UNBOUND(r) ((r)->flag = (-(r)->flag))
#define sat_satisfies(r) ((r)->flag < 0)
#define sat_save_tos(sat, p)                                                   \
  p.tos_var = array_n(sat->stk_var);                                           \
  p.tos_inc = array_n(sat->stk_inc);                                           \
  p.tos_cla = array_n(sat->stk_cla)

static sm_col *sat_find_next_lit();
static void create_sat_lit();
static bool sat_bound();
static void sat_undo_assignment();
static bool sat_branch();
static bool sat_find_impl();
static sat_result_t sat_branch_n_bound();
static bool sat_fix_literal();

/* SAT search strategies */
static sat_strategy_t sat_strategy[] = {
    {0, 1, 15, 0, 0, 0},  {0, 2, 15, 0, 0, 0},   {0, 4, 15, 0, 0, 0},
    {0, 5, 15, 0, 0, 0},  {0, 1, 100, 10, 1, 1}, {0, 2, 100, 0, 0, 0},
    {0, 4, 100, 0, 0, 0}, {0, 5, 100, 0, 0, 0},  {0, 1, 500, 10, 1, 1},
    {0, 2, 500, 0, 0, 0}, {0, 4, 500, 0, 0, 0},  {0, 5, 500, 0, 0, 0}};

static sm_col *sat_find_next_lit(sat, next_clause) sat_t *sat;
sm_row **next_clause;
{
  int order, best;
  sm_row *cl;
  sm_element *p;

  order = sat->strategy->var_order;
  best = -1;
  cl = *next_clause;
  if (order == 1 || order == 2) {

    /* find literal in first unsatisfied clause */
    for (; cl != NIL(sm_row); cl = cl->next_row) {
      if (sat_satisfies(cl)) {
        continue;
      }
      sm_foreach_row_element(cl, p) {
        if (!sm_get_col(sat->matrix, sat_neg(p->col_num))->flag) {
          best = p->col_num;
          if (order == 1) {
            break;
          }
        }
      }
      break;
    }
  } else if (order == 4 || order == 5) {

    /* find literal in last unsatisfied clause */
    for (; cl != NIL(sm_row); cl = cl->prev_row) {
      if (sat_satisfies(cl)) {
        continue;
      }
      sm_foreach_row_element(cl, p) {
        if (!sm_get_col(sat->matrix, sat_neg(p->col_num))->flag) {
          best = p->col_num;
          if (order == 4) {
            break;
          }
        }
      }
      break;
    }
  } else {
    fail("Unknown clause / variable ordering");
  }
  *next_clause = cl;

  return sm_get_col(sat->matrix, best);
}

/* SAT interface */
sat_t *sat_new() {
  sat_t *sat;

  sat = ALLOC(sat_t, 1);
  sat->matrix = sm_alloc_size(0, 0);
  sat->one_clauses = array_alloc(int, ASIZE);
  sat->nclause = 0;
  sat->lit_index = 0;

  /* stacks */
  sat->stk_var = array_alloc(int, ASIZE);
  sat->stk_inc = array_alloc(int, ASIZE);
  sat->stk_cla = array_alloc(int, ASIZE);

  return sat;
}

void sat_reset(sat) sat_t *sat;
{
  sm_col *c;

  sm_foreach_col(sat->matrix, c) {
    fast_avl_free_tree((fast_avl_tree *)c->user_word, 0, 0);
  }
  sm_free(sat->matrix);
  sat->matrix = sm_alloc_size(100, 100);
  array_reset(sat->one_clauses);
  sat->nclause = 0;
  sat->lit_index = 0;
  sat->n_impl = -1;

  /* stacks */
  array_reset(sat->stk_var);
  array_reset(sat->stk_inc);
  array_reset(sat->stk_cla);
}

void sat_delete(sat) sat_t *sat;
{
  sm_col *c;

  sm_foreach_col(sat->matrix, c) {
    fast_avl_free_tree((fast_avl_tree *)c->user_word, 0, 0);
  }
  sm_free(sat->matrix);
  array_free(sat->one_clauses);

  /* stacks */
  array_free(sat->stk_var);
  array_free(sat->stk_inc);
  array_free(sat->stk_cla);

  FREE(sat);
}

static void create_sat_lit(A, col) sm_matrix *A;
int col;
{
  sm_col *pcol;

  if (col >= A->cols_size) {
    sm_resize(A, A->rows_size, col);
  }

  pcol = A->cols[col] = sm_col_alloc();
  pcol->col_num = col;
  sorted_insert(sm_col, A->first_col, A->last_col, A->ncols, next_col, prev_col,
                col_num, col, pcol);
  pcol->flag = FALSE;
  pcol->user_word = (char *)fast_avl_init_tree(fast_avl_numcmp);
}

/* return an index of sat variable */
int sat_new_variable(sat) sat_t *sat;
{
  create_sat_lit(sat->matrix, sat->lit_index);
  create_sat_lit(sat->matrix, sat->lit_index + 1);
  sat->lit_index += 2;
  return sat->lit_index - 2;
}

bool sat_add_implication(sat, var1, var2) sat_t *sat;
int var1;
int var2;
{
  fast_avl_tree *imply;

  if (var1 == var2) {
    return FALSE;
  }
  imply = (fast_avl_tree *)sm_get_col(sat->matrix, var1)->user_word;
  if (fast_avl_is_member(imply, (char *)var2)) {
    return FALSE;
  }
  fast_avl_insert(imply, (char *)var2, (char *)0);
  return TRUE;
}

/* SAT solver */
static bool sat_bound(sat, c, add_impl) sat_t *sat;
sm_col *c;
bool add_impl;
{
  int bot, lit, imp_lit;
  sm_row *r;
  fast_avl_tree *table;
  fast_avl_generator *sgen;
  sm_col *lit_col, *imp_col;
  sm_element *e1, *e2;
  bool implied_one;

  bot = array_n(sat->stk_var);
  array_insert_last(int, sat->stk_var, c->col_num);
  c->flag = TRUE;

  while (bot < array_n(sat->stk_var)) {
    lit = array_fetch(int, sat->stk_var, bot++);
    lit_col = sm_get_col(sat->matrix, lit);

    /* do 2-sat implications */
    table = (fast_avl_tree *)lit_col->user_word;
    fast_avl_foreach_item(table, sgen, AVL_FORWARD, (char **)&imp_lit,
                          NIL(char *)) {
      imp_col = sm_get_col(sat->matrix, imp_lit);
      if (sm_get_col(sat->matrix, sat_neg(imp_lit))->flag) {
        fast_avl_free_gen(sgen);
        return FALSE;
      }

      /* if variable not set, push on implied variable stack */
      if (!(imp_col->flag)) {
        array_insert_last(int, sat->stk_var, imp_lit);
        imp_col->flag = TRUE;
      }
    }

    /* imply its complement literal */
    sm_foreach_col_element(sm_get_col(sat->matrix, sat_neg(lit)), e1) {
      r = sm_get_row(sat->matrix, e1->row_num);
      if (sat_satisfies(r)) {
        continue;
      }
      if (N_UNBOUND(r) > 2) {
        /* push on implied clause stack */
        array_insert_last(int, sat->stk_inc, r->row_num);
        DEC_UNBOUND(r);
        continue;
      }
      implied_one = FALSE;
      sm_foreach_row_element(r, e2) {
        imp_col = sm_get_col(sat->matrix, e2->col_num);
        if (imp_col->flag) {
          implied_one = TRUE;
          break;
        }
        if (!sm_get_col(sat->matrix, sat_neg(e2->col_num))->flag) {
          implied_one = TRUE;

          /* push on implied variable stack */
          array_insert_last(int, sat->stk_var, e2->col_num);
          imp_col->flag = TRUE;

          /* if lit_col => imp_col, deduce contrapositive */
          if (add_impl && sat_add_implication(sat, sat_neg(e2->col_num),
                                              sat_neg(c->col_num))) {
            sat->n_impl++;
          }
          break;
        }
      }
      if (!implied_one) {
        return FALSE;
      }
    }

    /* satisfy clauses where it occurs */
    sm_foreach_col_element(lit_col, e1) {
      r = sm_get_row(sat->matrix, e1->row_num);

      /* check if not satisfied */
      if (N_UNBOUND(r) > 0) {

        /* push on satisfied clause stack */
        array_insert_last(int, sat->stk_cla, r->row_num);
        INV_UNBOUND(r);
      }
    }
  }
  return TRUE;
}

static void sat_undo_assignment(sat, prev_status) sat_t *sat;
stk_status_t prev_status;
{
  int i, lit, clause;

  for (i = array_n(sat->stk_var); i-- > prev_status.tos_var;) {
    lit = array_fetch(int, sat->stk_var, i);
    sm_get_col(sat->matrix, lit)->flag = FALSE;
  }
  for (i = array_n(sat->stk_cla); i-- > prev_status.tos_cla;) {
    clause = array_fetch(int, sat->stk_cla, i);
    INV_UNBOUND(sm_get_row(sat->matrix, clause));
  }
  for (i = array_n(sat->stk_inc); i-- > prev_status.tos_inc;) {
    clause = array_fetch(int, sat->stk_inc, i);
    INC_UNBOUND(sm_get_row(sat->matrix, clause));
  }
  sat->stk_var->num = prev_status.tos_var;
  sat->stk_inc->num = prev_status.tos_inc;
  sat->stk_cla->num = prev_status.tos_cla;
}

static bool sat_branch(sat, next_clause) sat_t *sat;
sm_row *next_clause;
{
  sm_col *c;
  stk_status_t prev_tos;

  c = sat_find_next_lit(sat, &next_clause);
  if (c == NIL(sm_col)) {
    return TRUE;
  }

  sat_save_tos(sat, prev_tos);

  if (sat_bound(sat, c, FALSE) && sat_branch(sat, next_clause)) {
    return TRUE;
  }
  if (++sat->bktrack > sat->strategy->bktrack_lim) {
    sat->gaveup = TRUE;
    return TRUE;
  }

  /* restore stack here */
  sat_undo_assignment(sat, prev_tos);

  return (sat_bound(sat, sm_get_col(sat->matrix, sat_neg(c->col_num)), FALSE) &&
          sat_branch(sat, next_clause));
}

static bool sat_find_impl(sat) sat_t *sat;
{
  bool result;
  int lit;
  sm_col *col, *comp_col;
  stk_status_t prev_tos;

  sat->n_impl = 0;
  sm_foreach_col(sat->matrix, col) {
    lit = col->col_num;
    comp_col = sm_get_col(sat->matrix, sat_neg(lit));
    if (!col->flag && !comp_col->flag &&
        (fast_avl_count((fast_avl_tree *)col->user_word))) {
      sat_save_tos(sat, prev_tos);
      result = sat_bound(sat, col, sat->strategy->add_nli);
      sat_undo_assignment(sat, prev_tos);
      if (!result && sat->strategy->add_unique) {
        sat->n_impl++;
        if (!sat_fix_literal(sat, comp_col)) {
          return FALSE;
        }
      }
    }
  }
  return TRUE;
}

static sat_result_t sat_branch_n_bound(sat, verbosity) sat_t *sat;
int verbosity;
{
  int i;
  sm_col *c;
  sm_row *start_clause;
  bool bnb_status;
  stk_status_t prev_tos;
  sat_result_t result;

  sat->bktrack = 0;
  sat->gaveup = FALSE;
  bnb_status = TRUE;

  /* do static implications if required */
  if (sat->n_impl && sat->strategy->n_static_pass) {
    i = 0;
    while (i++ < sat->strategy->n_static_pass && bnb_status && sat->n_impl) {
      bnb_status = sat_find_impl(sat);
      if (sat->n_impl && bnb_status && (verbosity > 1)) {
        fprintf(sisout, "%d Implications\n", sat->n_impl);
      }
    }
  }

  sat_save_tos(sat, prev_tos);
  if (sat->strategy->var_order < 4) {
    start_clause = sat->matrix->first_row;
  } else {
    start_clause = sat->matrix->last_row;
  }

  while (bnb_status) {
    c = sat_find_next_lit(sat, &start_clause);
    if (c == NIL(sm_col)) {
      break;
    }
    if (sat_bound(sat, c, FALSE) && sat_branch(sat, start_clause)) {
      break;
    }

    /* undo partial assignment */
    sat_undo_assignment(sat, prev_tos);

    bnb_status =
        sat_fix_literal(sat, sm_get_col(sat->matrix, sat_neg(c->col_num)));
  }

  if (sat->gaveup) {
    result = SAT_GAVEUP;
    /* undo partial assignment */
    sat_undo_assignment(sat, prev_tos);
  } else {
    result = (bnb_status) ? SAT_SOLVED : SAT_ABSURD;
  }
  return result;
}

/* fix variables from one-literal clauses */
static bool sat_fix_literal(sat, c) sat_t *sat;
sm_col *c;
{
  if (c->flag) {
    return TRUE;
  }
  if (sm_get_col(sat->matrix, sat_neg(c->col_num))->flag ||
      !sat_bound(sat, c, FALSE)) {
    return FALSE;
  }
  array_reset(sat->stk_var);
  array_reset(sat->stk_inc);
  array_reset(sat->stk_cla);
  return TRUE;
}

sat_result_t sat_solve(sat, fast_sat, verbosity) sat_t *sat;
bool fast_sat;
int verbosity;
{
  int i;
  sm_col *c;
  sm_row *r;
  sat_result_t result;

  sm_foreach_row(sat->matrix, r) {
    r->flag = r->length;
    if (r->length == 1) {
      INV_UNBOUND(r);
      array_insert_last(int, sat->one_clauses, r->first_col->col_num);
    } else if (r->length == 0) {
      return SAT_ABSURD;
    }
  }
  for (i = 0; i < array_n(sat->one_clauses); i++) {
    c = sm_get_col(sat->matrix, array_fetch(int, sat->one_clauses, i));
    if (!sat_fix_literal(sat, c)) {
      return SAT_ABSURD;
    }
  }
  result = SAT_GAVEUP;
  if (fast_sat) {
    for (i = 0; result == SAT_GAVEUP && i < 4; i++) {
      sat->strategy = &sat_strategy[i];
      result = sat_branch_n_bound(sat, verbosity);
    }
  } else {
    for (i = 0; result == SAT_GAVEUP && i < 12; i++) {
      sat->strategy = &sat_strategy[i];
      result = sat_branch_n_bound(sat, verbosity);
    }
  }
  return result;
}

int sat_get_value(sat, id) sat_t *sat;
int id;
{
  if (sm_get_col(sat->matrix, id)->flag) {
    return 1;
  } else if (sm_get_col(sat->matrix, sat_neg(id))->flag) {
    return 0;
  } else {
    return 2;
  }
}
