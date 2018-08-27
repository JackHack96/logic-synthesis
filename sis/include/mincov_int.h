
#include "sis.h"

typedef struct stats_struct stats_t;
struct stats_struct {
  int debug;           /* 1 if debugging is enabled */
  int max_print_depth; /* dump stats for levels up to this level */
  int max_depth;       /* deepest the recursion has gone */
  int nodes;           /* total nodes visited */
  int component;       /* currently solving a component */
  int comp_count;      /* number of components detected */
  int gimpel_count;    /* number of times Gimpel reduction applied */
  int gimpel;          /* currently inside Gimpel reduction */
  long start_time;     /* cpu time when the covering started */
  int no_branching;
  int lower_bound;
};

typedef struct solution_struct solution_t;
struct solution_struct {
  sm_row *row;
  int cost;
};

extern solution_t *solution_alloc();

extern void solution_free();

extern solution_t *solution_dup();

extern void solution_accept();

extern void solution_reject();

extern void solution_add();

extern solution_t *solution_choose_best();

extern solution_t *sm_maximal_independent_set();

extern solution_t *sm_mincov();

extern int gimpel_reduce();

#define WEIGHT(weight, col) (weight == NIL(int) ? 1 : weight[col])

/* for binate covering */
typedef struct bin_solution_struct bin_solution_t;
struct bin_solution_struct {
  pset set;
  int cost;
};

extern bin_solution_t *bin_solution_alloc();

extern void bin_solution_free();

extern bin_solution_t *bin_solution_dup();

extern void bin_solution_del();

extern void bin_solution_add();

extern void sm_bin_mincov();
