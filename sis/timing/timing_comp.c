
#ifdef SIS
#include "timing_int.h"

#define FEASIBLE 1
#define SOLVE 2

/* function definition
    name:     tmg_compute_optimal_clock()
    args:     latch_graph, network
    job:      compute optimal clocking params
    return value:  -
    calls:
             tmg_clock_lower_bound(),
         tmg_construct_clock_graph(), tmg_add_short_path_constraints(),
         tmg_add_model_constraints(), tmg_add_phase_separation_constraints(),
         tmg_print_constraint_graph(),
         tmg_solve_constraints(), solve_gen_constraints(),
         tmg_circuit_clock_update()
         tmg_constraint_graph_free()
*/

double tmg_compute_optimal_clock(latch_graph, network) graph_t *latch_graph;
network_t *network;
{
  graph_t *clock_graph;
  phase_t *phase;
  double clock, clock_lb;

  clock_lb = 0;
  clock_lb = tmg_clock_lower_bound(latch_graph);
  (void)fprintf(sisout, "Clock lower bound found %.2lf : cpu_time = %s\n",
                clock_lb, util_print_time(util_cpu_time()));

  /* Construct the clock graph - a graph with a vertex for rise/
     fall of each phase. The long path constraints are added now*/
  /* ---------------------------------------------------------- */

  clock_graph = tmg_construct_clock_graph(latch_graph, clock_lb);
  if (debug_type == BB || debug_type == ALL) {
    (void)fprintf(sisout, "Only long paths: \n");
    tmg_print_constraint_graph(clock_graph);
    (void)fprintf(sisout, "\n\n");
  }

  /* Add the short path constraints for each edge */
  /* -------------------------------------------- */

  tmg_add_short_path_constraints(clock_graph, latch_graph);

  /* Get the last edge (e_k)- recall 0 (ref) vertex is also in table*/
  /* -------------------------------------------------------------- */

  phase = PHASE_LIST(clock_graph)[NUM_PHASE(clock_graph) - 1];

  /* Add other constraints */
  /* --------------------- */

  tmg_add_model_constraints(latch_graph, clock_graph, phase->fall);
  tmg_add_phase_separation_constraints(latch_graph, clock_graph, phase->fall);

  if (debug_type == BB || debug_type == ALL) {
    (void)fprintf(sisout, "Long and Short paths\n");
    tmg_print_constraint_graph(clock_graph);
    (void)fprintf(sisout, "\n\n");
  }

  if (debug_type == CGRAPH || debug_type == ALL) {
    (void)fprintf(sisout, "clock lower bound = %.2lf\n", clock_lb);
  }

  /* Solve the optimization problem */
  /* ------------------------------ */

  if (tmg_get_gen_algorithm_flag()) {
    clock = tmg_solve_gen_constraints(clock_graph, phase->fall, clock_lb);
  } else {
    clock = tmg_solve_constraints(clock_graph, phase->fall, clock_lb);
  }
  if (clock > 0) {

    /* Set the values in the tmg_circuit */
    /* ----------------------------- */

    tmg_circuit_clock_update(latch_graph, clock_graph, network, clock);
  }

  /* Free memory associated with the clock_graph */
  /* ------------------------------------------- */

  tmg_constraint_graph_free(clock_graph);
  return clock;
}

/* function definition
    name:     tmg_clock_lower_bound()
    args:     graph_t *g
    job:      finds a lower bound for the clock cycle using Lawlers
              binary search
    method.
    return value: double
    calls:    guess_lower_bound(),
              tmg_all_negative_cycles()
*/
double tmg_clock_lower_bound(g) graph_t *g;
{
  double cur_clock, new_clock, clock_ub, clock_lb;

  clock_ub = 0;
  new_clock = tmg_guess_clock_bound(g);
  clock_lb = new_clock;
  while (TRUE) {
    cur_clock = new_clock;
    if (debug_type == CGRAPH || debug_type == ALL) {
      (void)fprintf(sisout, "Trying %.4lf  ", cur_clock);
    }
    if (tmg_all_negative_cycles(g, cur_clock)) { /* Negative cycle */
      if (debug_type == CGRAPH || debug_type == ALL) {
        (void)fprintf(sisout, "Success - try lower \n");
      }
      clock_ub = cur_clock;
    } else {
      if (debug_type == CGRAPH || debug_type == ALL) {
        (void)fprintf(sisout, "Failure - try higher \n");
      }
      clock_lb = cur_clock;
    }
    if (ABS(clock_ub - clock_lb) < EPS2)
      break;
    if (clock_ub > clock_lb) {
      new_clock = (clock_ub + clock_lb) / 2;
    } else {
      new_clock *= 2;
    }
  }
  return clock_ub;
}

/* function definition
    name:     tmg_guess_clock_bound()
    args:     graph_t *g
    job:      uses DFS to guess lower bound on clock cycle
    return value:  double
    calls: guess_clock_bound_recursive()
*/
double tmg_guess_clock_bound(g) graph_t *g;
{
  double bound1, bound2;
  lsGen gen;
  vertex_t *v, *host;

  /* init */
  /* ---- */

  host = HOST(g);
  foreach_vertex(g, gen, v) {
    Rv(v) = -1;
    Wv(v) = 0;
    COPT_DIRTY(v) = FALSE;
  }
  if (host == NIL(vertex_t)) {
    host = v;
  }
  bound1 = 0;
  bound2 = 0;
  Rv(host) = 0;
  if (debug_type == CGRAPH || debug_type == ALL) {
    (void)fprintf(sisout, "guessing a bound for clock cycle\n");
  }
  tmg_guess_clock_bound_recursive(host, &bound1, &bound2);
  if (bound1 < bound2) {
    (void)fprintf(sisout, "bound1 = %.2lf   bound2 = %.2lf \n", bound1, bound2);
    bound1 = bound2;
  }
  assert(bound1 > 0);

  return bound1;
}

/* function definition
    name:      tmg_guess_clock_bound_recursive()
    args:      vertex_t *u, double *b1, *b2
    job:       Recursive DFS
    return value: -
    calls:
*/
tmg_guess_clock_bound_recursive(u, b1, b2) vertex_t *u;
double *b1, *b2;
{
  lsGen gen;
  edge_t *e;
  vertex_t *v;
  int den;

  (*b2) = MAX((*b2), Wv(u) / (Rv(u) + 1));
  if (debug_type == CGRAPH || debug_type == ALL) {
    (void)fprintf(sisout, "Path bound = %.4lf\n", *b2);
  }
  COPT_DIRTY(u) = TRUE;
  foreach_out_edge(u, gen, e) {
    v = g_e_dest(e);

    /* Check if v has been visited before*/
    /* --------------------------------- */

    if (COPT_DIRTY(v)) {

      /* Check if v is on DFS stack */
      /* -------------------------- */

      if (Rv(v) > 0) {
        if ((den = Rv(u) + Ke(e) - Rv(v)) == 0) {
          (void)fprintf(sisout, "Error cycle without memory elements\n");
        }
        if (den > 0) {
          (*b1) = MAX((*b1), (Wv(u) + De(e) - Wv(v)) / den);
          if (debug_type == CGRAPH || debug_type == ALL) {
            (void)fprintf(sisout, "Cycle bound = %.4lf\n", *b1);
          }
        }
      }
    } else {
      Rv(v) = Rv(u) + Ke(e);
      Wv(v) = Wv(u) + De(e);
      tmg_guess_clock_bound_recursive(v, b1, b2);
    }
  }
  /* Remove u from DFS stack */
  /* ----------------------- */
  Rv(u) = -1;
}

#ifdef OCT

#define UTILITY_H
#include "da.h"
#include "pq.h"
#undef UTILITY_H

/* function definition
    name:     tmg_all_negative_cycles()
    args:     graph_t *g, double c
    job:      uses Bellman-Ford to test if given c causes a positive cycle in g
    return value:   0 if there is a positive cycle
                    1 if all cycles are negative or 0 weighted

    calls:
*/
#define NUM_CYCLE 5
int tmg_all_negative_cycles(g, c) graph_t *g;
double c;
{
  vertex_t *v, *u;
  char *dummy;
  st_table *table;
  st_generator *sgen;
  pq_t *tree;
  edge_t *e;
  int num_v, flag, i, my_pq_cmp();
  double C[2], c_eps, val;
  lsGen gen;

  num_v = lsLength(g_get_vertices(g));

  /* Initialize all edges with appropriate weights*/
  /* -------------------------------------------- */

  C[0] = 0;
  C[1] = c;
  c_eps = c + EPS1;
  /*  Weigh edges */
  /* ------------ */
  foreach_edge(g, gen, e) { We(e) = De(e) - C[Ke(e)]; }
  /* Init vertices */
  /* ------------- */
  foreach_vertex(g, gen, u) {
    Wv(u) = -INFTY;
    PARENT(u) = NIL(vertex_t);
  }

  Wv(HOST(g)) = 0;
  tree = pq_init_queue(my_pq_cmp);
  pq_insert_fast(tree, (char *)HOST(g), (char *)HOST(g));
  foreach_vertex(g, gen, v) {
    if (TYPE(v) == LATCH && PHASE(v) == 1) {
      Wv(v) = 0;
      pq_insert_fast(tree, (char *)v, (char *)v);
    }
  }

  /* Repeat num_v times */
  /* ------------------ */
  for (i = 0; i < num_v; i++) {
    /* flag to detect early convergence or failure therof */
    /* -------------------------------------------------- */
    flag = TRUE;

    /* Use table to hash vertices that have changed */
    /* -------------------------------------------- */
    table = st_init_table(st_ptrcmp, st_ptrhash);
    while (!pq_empty(tree)) {
      pq_pop(tree, (char **)&u);
      foreach_out_edge(u, gen, e) {
        v = g_e_dest(e);
        if ((val = Wv(u) + We(e)) > Wv(v)) {
          flag = FALSE;
          if (val > c_eps) { /* path requires more that c units/cycle
                                        -----------------------------------*/
            if (debug_type == CGRAPH || debug_type == ALL) {
              (void)fprintf(sisout, "Positive path exceeding c \n");
            }
            pq_free_queue(tree);
            st_free_table(table);
            lsFinish(gen);
            return FALSE;
          }
          Wv(v) = val;

          /* Keep track of one parent in hope for cycle detection */
          /* ---------------------------------------------------- */
          PARENT(v) = u;
          st_insert(table, (char *)v, (char *)v); /* Hash it!! */
          /* ------------------------------------------------- */
        }
      }
    }
    pq_free_queue(tree);
    if (flag) {
      st_free_table(table);
      break;
    } else {
      /* Build priority queue of hashed vertices with hope of detecting
         > c_eps */
      /* ----------------------------------------------------------- */
      tree = pq_init_queue(my_pq_cmp);
      st_foreach_item(table, sgen, (char **)&u, &dummy) {
        pq_insert_fast(tree, (char *)u, (char *)u);
      }
      st_free_table(table);
      if ((i % NUM_CYCLE) == 0) {
        if (cycle_in_graph(tree, g)) {
          pq_free_queue(tree);
          flag = FALSE;
          break;
        }
      }
    }
  }

  /*     check for positive cycle */
  /* ---------------------------- */
  return flag;
}

static int my_pq_cmp(k1, k2) char *k1, *k2;
{
  vertex_t *v1, *v2;

  v1 = (vertex_t *)(k1);
  v2 = (vertex_t *)(k2);

  if (ABS(Wv(v1) - Wv(v2)) < EPS1)
    return 0;
  if (Wv(v1) < Wv(v2))
    return -1;
  return 1;
}

#define NUM_POP 5

int cycle_in_graph(tree, g) pq_t *tree;
graph_t *g;
{
  lsGen gen;
  vertex_t *u, *v, **pop_array;
  int i, cycle_found;

  pop_array = ALLOC(vertex_t *, NUM_POP);
  for (i = 0; i < NUM_POP; i++) {
    pop_array[i] = NIL(vertex_t);
  }
  cycle_found = FALSE;
  i = 0;
  while (i < NUM_POP && !pq_empty(tree)) {
    foreach_vertex(g, gen, v) { COPT_DIRTY(v) = FALSE; }
    pq_pop(tree, (char **)&u);
    pop_array[i] = u;
    i++;
    while (TRUE) {
      COPT_DIRTY(u) = TRUE;
      v = PARENT(u);
      if (v == NIL(vertex_t)) {
        break;
      }
      if (COPT_DIRTY(v)) {
        cycle_found = TRUE;
        break;
      }
      u = v;
    }
    if (cycle_found)
      break;
  }

  if (cycle_found) {
    FREE(pop_array);
    return TRUE;
  } else {
    for (i = 0; i < NUM_POP; i++) {
      if (pop_array[i] != NIL(vertex_t)) {
        pq_insert_fast(tree, (char *)pop_array[i], (char *)pop_array[i]);
      }
    }
    FREE(pop_array);
    return FALSE;
  }
}

#else

/* function definition
    name:     all_negative_cycles()
    args:     graph_t *g, double c
    job:      uses Bellman-Ford to test if given c causes a positive cycle in g
    return value:   0 if there is a positive cycle
                    1 if all cycles are negative or 0 weighted

    calls:
*/
int tmg_all_negative_cycles(g, c) graph_t *g;
double c;
{
  vertex_t *v, *u;
  edge_t *e;
  int num_v, flag, i;
  double C[2], c_eps, val;
  lsGen gen;

  num_v = lsLength(g_get_vertices(g));

  /* Initialize all edges with appropriate weights*/
  /* -------------------------------------------- */

  C[0] = 0;
  C[1] = c;
  c_eps = c + EPS;
  foreach_edge(g, gen, e) { We(e) = De(e) - C[Ke(e)]; }
  foreach_vertex(g, gen, u) { Wv(u) = -INFTY; }

  Wv(HOST(g)) = 0;

  for (i = 0; i < num_v; i++) {

    /* flag to detect early convergence */
    /* -------------------------------- */

    flag = TRUE;
    foreach_edge(g, gen, e) {
      v = g_e_dest(e);
      u = g_e_source(e);
      if ((val = Wv(u) + We(e)) > Wv(v)) {
        flag = FALSE;
        if (val > c_eps) {
          if (debug_type == CGRAPH || debug_type == ALL) {
            (void)fprintf(sisout, "Positive path exceeding c \n");
          }
          lsFinish(gen);
          return FALSE;
        }
        Wv(v) = val;
      }
    }
    if (flag)
      break;
  }

  return flag;
}

#endif

/* function definition
    name:     tmg_construct_clock_graph()
    args:     latch_graph
    job:      constructs the clock graph (a vertex for each
              transition of a phase
    return value: graph_t *
    calls:  tmg_alloc_cgraph(),
    tmg_gen_constraints_from(),

*/

graph_t *tmg_construct_clock_graph(lg, clock_lb) graph_t *lg;
double clock_lb;
{
  graph_t *cg;
  lsGen gen;
  vertex_t *u, *p1, *v;
  edge_t *e;
  int i, from_index;
  array_t **array_list, *zarray, *red, *black;

  /* Allocate constraint graph + ZERO vertex */
  /* --------------------------------------- */

  cg = g_alloc();
  cg->user_data = (gGeneric)(tmg_alloc_cgraph());
  tmg_alloc_phase_vertices(cg, lg);

  ZERO_V(cg) = g_add_vertex(cg);
  ZERO_V(cg)->user_data = (gGeneric)ALLOC(c_node_t, 1);
  PID(ZERO_V(cg)) = 0;
  array_list = ALLOC(array_t *, 2 * NUM_PHASE(cg));
  for (i = 0; i < 2 * NUM_PHASE(cg); i++) {
    array_list[i] = NIL(array_t);
  }
  foreach_vertex(lg, gen, v) {
    if (TYPE(v) != LATCH)
      continue;
    from_index = get_index(v, FROM, cg);
    if (array_list[from_index] == NIL(array_t)) {
      array_list[from_index] = array_alloc(vertex_t *, 0);
    }
    array_insert_last(vertex_t *, array_list[from_index], v);
  }

  /* Get edges with Ke(e) = 1 and Ke(e) = 0 */
  /* -------------------------------------- */

  red = array_alloc(edge_t *, 0);
  black = array_alloc(edge_t *, 0);
  foreach_edge(lg, gen, e) {
    if (Ke(e)) {
      array_insert_last(edge_t *, red, e);
    } else {
      array_insert_last(edge_t *, black, e);
    }
  }

  for (i = 0; i < 2 * NUM_PHASE(cg); i++) {
    if (array_list[i] != NIL(array_t)) {
      foreach_vertex(lg, gen, v) {
        Wv(v) = -INFTY;
        Rv(v) = 0;
        COPT_DIRTY(v) = FALSE;
      }
      p1 = get_vertex_from_index(cg, i);
      if (tmg_get_phase_inv() && i == 1) {

        /* Special case handling!! */
        /* ----------------------- */

        p1 = ZERO_V(cg);
      }
      tmg_gen_constraints_from(lg, cg, array_list[i], p1, clock_lb, red, black);
      array_free(array_list[i]);
    }
  }
  FREE(array_list);

  /* Add long path constraints from primary inputs. Inputs assumed stable
     for a clock cycle */
  /* ------------------ */

  u = HOST(lg);
  p1 = ZERO_V(cg);
  foreach_vertex(lg, gen, v) {
    Wv(v) = -INFTY;
    Rv(v) = 0;
    COPT_DIRTY(v) = FALSE;
  }
  zarray = array_alloc(vertex_t *, 0);
  array_insert_last(vertex_t *, zarray, u);
  tmg_gen_constraints_from(lg, cg, zarray, p1, clock_lb, red, black);
  array_free(zarray);
  array_free(red);
  array_free(black);
  return cg;
}

/* function definition
    name:     tmg_gen_constraints_from()
    args:     latch_graph, constraint_graph, array of latches on given
              phase event, corresponding constraint vertex, lower_bound on
          clock, array of red edges
    job:      Extract constraints between clock event represented by p1 and
              all possible latches reachable by the set of latches from
          latch_array
    return value: -
    calls:
*/

tmg_gen_constraints_from(lg, cg, latch_array, p1, clock_lb, red,
                         black) graph_t *lg,
    *cg;
vertex_t *p1;
array_t *latch_array;
double clock_lb;
array_t *red, *black;
{
  edge_t **e_array;
  lsGen gen;
  edge_t *e;
  vertex_t *u, *v, *p2;
  int num_v, i, to_index, current_key, more_to_come, j, local_converged;
  double *value, val;

  /* Cache the possible edges from p1 */
  /* -------------------------------- */

  e_array = ALLOC(edge_t *, 2 * NUM_PHASE(cg));
  for (i = 0; i < 2 * NUM_PHASE(cg); i++) {
    e_array[i] = NIL(edge_t);
  }

  /* Initialize delays from set of latches that start on p1 */
  /* ------------------------------------------------------ */

  for (i = 0; i < array_n(latch_array); i++) {
    u = array_fetch(vertex_t *, latch_array, i);
    /*	if (LTYPE(u) == FFF || LTYPE(u) == FFR) continue;*/
    if (TYPE(u) == LATCH) {
      Wv(u) = tmg_max_clock_skew(u);
    } else {
      Wv(u) = 0.0;
    }
    Rv(u) = 0;
    if (debug_type == CGRAPH || debug_type == ALL) {
      (void)fprintf(sisout, "From %d :\n", ID(u));
    }
  }

  /* The remaining section of this subroutine follows the paper by
     Dr. T. G. Szymanski in DAC 92 for extracting the constraints
     efficiently                                                 */
  /* ----------------------------------------------------------- */

  current_key = 0;
  num_v = lsLength(g_get_vertices(lg));
  more_to_come = TRUE;

  while (more_to_come) {
    more_to_come = FALSE;
    foreach_vertex(lg, gen, v) {
      if (Rv(v) == current_key) {
        PREV_Wv(v) = Wv(v);
      }
    }

    for (i = 0; i < num_v; i++) {
      local_converged = TRUE;
      for (j = 0; j < array_n(black); j++) {
        e = array_fetch(edge_t *, black, j);
        u = g_e_source(e);
        v = g_e_dest(e);
        if (TYPE(v) == NNODE || Wv(u) == -INFTY)
          continue;
        if (TYPE(u) == NNODE && Wv(u) < 0)
          continue;
        if ((Wv(u) + De(e) - current_key * clock_lb) >
            Wv(v) - Rv(v) * clock_lb) {
          Wv(v) = Wv(u) + De(e);
          Rv(v) = Rv(u);
          COPT_DIRTY(v) = TRUE;
          if (debug_type == CGRAPH || debug_type == ALL) {
            (void)fprintf(sisout, "%d (%.2lf)->(%.2lf )-> %d (%.2lf)\n", ID(u),
                          Wv(u), De(e), ID(v), Wv(v) - Rv(v) * clock_lb);
          }
          local_converged = FALSE;
        }
      }
      if (local_converged) {
        break;
      }
    }
    assert(local_converged); /* Sanity check */
    foreach_vertex(lg, gen, v) {
      if (TYPE(v) == NNODE)
        continue;
      if (!COPT_DIRTY(v))
        continue;
      if (Rv(v) == current_key) {
        to_index = get_index(v, TO, cg);
        if ((e = e_array[to_index]) == NIL(edge_t)) {
          p2 = get_vertex(v, TO, cg);
          if (!g_get_edge(p2, p1, &e)) {
            e->user_data = (gGeneric)tmg_alloc_cedge();
          }
          e_array[to_index] = e;
        }
        val = Wv(v) + tmg_get_set_up(v) - tmg_min_clock_skew(v);
        if (debug_type == CGRAPH || debug_type == ALL) {
          (void)fprintf(sisout, "to %d : %.2lf - %d c\n", ID(v), val,
                        current_key);
        }
        if (st_lookup(CONS1(e), (char *)current_key, (char **)&value)) {
          if (*value < val) {
            *value = val;
          }
        } else {
          value = ALLOC(double, 1);
          *value = val;
          st_insert(CONS1(e), (char *)current_key, (char *)value);
        }
        PREV_Wv(v) = Wv(v);
      }
    }
    for (i = 0; i < array_n(red); i++) {
      e = array_fetch(edge_t *, red, i);
      u = g_e_source(e);
      if ((LTYPE(u) == FFF || LTYPE(u) == FFR) && COPT_DIRTY(u))
        continue;
      v = g_e_dest(e);
      if (TYPE(v) == NNODE)
        continue;
      if (TYPE(u) == NNODE && Wv(u) < 0)
        continue;
      if ((PREV_Wv(u) + De(e) - (current_key + 1) * clock_lb) >
          (Wv(v) - Rv(v) * clock_lb + EPS)) {
        Wv(v) = PREV_Wv(u) + De(e);
        Rv(v) = current_key + 1;
        COPT_DIRTY(v) = TRUE;
        if (debug_type == CGRAPH || debug_type == ALL) {
          (void)fprintf(sisout, "%d (%.2lf)->(%.2lf)-> %d (%.2lf)\n", ID(u),
                        Wv(u), De(e) - clock_lb, ID(v),
                        Wv(v) - Rv(v) * clock_lb);
        }
        more_to_come = TRUE;
      }
    }
    current_key++;
  }
  FREE(e_array);
}

/* function definition
    name:     tmg_add_short_path_constraints()
    args:     clock_graph, latch_graph
    job:      Adds the short path constraints for each edge
              Does a conservative job by extracting constraints for
          each edge only
    return value:  -
    calls:    g_get_edge()
*/
tmg_add_short_path_constraints(cg, lg) graph_t *cg, *lg;
{
  edge_t *cedge, *ledge;
  double **array;
  lsGen gen;
  int i, j, from_id, to_id, dim, k;
  vertex_t *u, *v;
  vertex_t *p1, *p2;

  dim = 2 * NUM_PHASE(cg);
  array = ALLOC(double *, dim);
  for (i = 0; i < dim; i++) {
    array[i] = ALLOC(double, dim);
    for (j = 0; j < dim; j++) {
      array[i][j] = INFTY;
    }
  }
  foreach_edge(lg, gen, ledge) {
    u = g_e_source(ledge);
    v = g_e_dest(ledge);
    if (TYPE(u) != LATCH || TYPE(v) != LATCH)
      continue;
    from_id = get_index(u, FROM, cg);
    to_id = get_index(v, TO, cg);
    array[from_id][to_id] =
        MIN(array[from_id][to_id], tmg_min_clock_skew(u) + de(ledge) -
                                       tmg_get_hold(v) - tmg_max_clock_skew(v));
  }
  for (i = 0; i < dim; i++) {
    for (j = 0; j < dim; j++) {
      if (array[i][j] < INFTY) {
        p1 = get_vertex_from_index(cg, i);
        p2 = get_vertex_from_index(cg, j);
        if (!g_get_edge(p1, p2, &cedge)) {
          cedge->user_data = (gGeneric)tmg_alloc_cedge();
        }
        if (CONS2(cedge) == NIL(double)) {
          CONS2(cedge) = ALLOC(double, 2);
          for (k = 0; k < 2; k++) {
            CONS2(cedge)[k] = INFTY;
          }
        }
        if (ABS(PID(p1)) < ABS(PID(p2))) {
          CONS2(cedge)[1] = MIN(CONS2(cedge)[1], array[i][j]);
        } else {
          CONS2(cedge)[0] = MIN(CONS2(cedge)[0], array[i][j]);
        }
      }
    }
    FREE(array[i]);
  }
  FREE(array);
}

/* function definition
    name:     tmg_solve_constraints()
    args:     clock_graph, vertex (e_k)
    job:      Finds the minimum c (>= clock_lb) for which the constraints are
              satisfied. Contains the binary search routine (while(1) { ...})
    return value: double (clock cycle)
    calls:    tmg_is_feasible()
*/

double tmg_solve_constraints(cg, v0, clock_lb) graph_t *cg;
vertex_t *v0;
double clock_lb;
{
  double cur_clock, clock_ub;

  clock_ub = INFTY;
  cur_clock = clock_lb;
  while (TRUE) {
    (void)fprintf(sisout, "trying %.2lf \n", cur_clock);
    if (!tmg_is_feasible(cg, cur_clock, v0)) {
      clock_lb = cur_clock;
      if (clock_ub == INFTY) {
        cur_clock = 2 * cur_clock;
      } else {
        cur_clock = (clock_lb + clock_ub) / 2;
      }
    } else {
      if (clock_ub - clock_lb < EPS)
        break;
      clock_ub = cur_clock;
    }
    cur_clock = (clock_ub + clock_lb) / 2;
  }

  tmg_print_graph_solution(cg);
  return cur_clock;
}

/* function definition
    name:     tmg_is_feasible()
    args:     clock_graph, clock_cycle, e_k
    job:      Check if the clock_cycle is feasible.

    return value: 1 on success, 0 on failure
    calls:       tmg_evaluate_right_hand_side(),
                 tmg_print_graph_solution()
*/
int tmg_is_feasible(g, c, v0) graph_t *g;
double c;
vertex_t *v0;
{
  edge_t *e;
  vertex_t *u, *v;
  lsGen gen;
  int num_it, flag, i;

  if (c == 0)
    return FALSE;
  tmg_evaluate_right_hand_side(g, c);

  if (debug_type == BB || debug_type == ALL) {
    (void)fprintf(sisout, "LP constraints evaluated for c = %.2lf\n", c);
    tmg_print_constraints(g);
  }

  /* Initialize for Bellman-Ford on Constraint Graph */
  /* ----------------------------------------------- */

  foreach_vertex(g, gen, v) { P(v) = INFTY; }
  P(ZERO_V(g)) = 0;

  /* Bellman Ford carried out here */
  /* ----------------------------- */

  num_it = lsLength(g_get_vertices(g));
  for (i = 0; i < num_it; i++) {
    flag = TRUE;
    foreach_edge(g, gen, e) {
      if (IGNORE(e))
        continue;
      v = g_e_dest(e);
      u = g_e_source(e);
      if (P(v) > P(u) + WEIGHT(e)) {
        P(v) = P(u) + WEIGHT(e);
        flag = FALSE;
      }
    }
    if (flag)
      break;
  }

  /* Check for negative cycle */
  /* ------------------------ */

  if (debug_type == BB || debug_type == ALL) {
    tmg_print_graph_solution(g);
  }
  return flag;
}

/* function definition
    name:     tmg_print_constraint_graph()
    args:
    job:      prints the constraint graph
    return value: -
    calls:-
*/
tmg_print_constraint_graph(g) graph_t *g;
{
  edge_t *e;
  vertex_t *u, *v;
  char *dummy;
  st_generator *sgen;
  double *value;
  lsGen gen;
  int i, flag;

  foreach_edge(g, gen, e) {
    if (IGNORE(e))
      continue;
    flag = TRUE;
    u = g_e_source(e);
    v = g_e_dest(e);
    if (PID(v) > 0) {
      (void)fprintf(sisout, "r(%d) - ", PID(v));
    } else if (PID(v) < 0) {
      (void)fprintf(sisout, "e(%d) - ", -PID(v));
    } else {
      (void)fprintf(sisout, "ZERO - ");
    }
    if (PID(u) > 0) {
      (void)fprintf(sisout, "r(%d)\n", PID(u));
    } else if (PID(u) < 0) {
      (void)fprintf(sisout, "e(%d)\n", -PID(u));
    } else {
      (void)fprintf(sisout, "ZERO \n");
    }

    if (EVAL(e) == TRUE) {
      if (flag) {
        (void)fprintf(sisout, "\t  <= \n");
        flag = FALSE;
      }
      (void)fprintf(sisout, "FIXED \n");
      (void)fprintf(sisout, "\t %.2lf \n", FIXED(e));
    }

    if (ABS(DUTY(e)) <= 1) {
      if (flag) {
        (void)fprintf(sisout, "\t  <= \n");
        flag = FALSE;
      }
      (void)fprintf(sisout, "DUTY \n");
      (void)fprintf(sisout, "\t %.2lf c\n", DUTY(e));
    }
    if (st_count(CONS1(e)) > 0) {
      if (flag) {
        (void)fprintf(sisout, "\t  <= \n");
        flag = FALSE;
      }
      (void)fprintf(sisout, "LP \n");
      st_foreach_item(CONS1(e), sgen, (char **)&dummy, (char **)&value) {
        i = (int)dummy;
        (void)fprintf(sisout, "\t -%.2lf + %d c\n", *value, i);
      }
    }
    if (CONS2(e) != NIL(double)) {
      if (flag) {
        (void)fprintf(sisout, "\t  <= \n");
        flag = FALSE;
      }
      (void)fprintf(sisout, "SP \n");
      for (i = 0; i < 2; i++) {
        (void)fprintf(sisout, "\t %.2lf + %d c\n", CONS2(e)[i], i);
      }
    }
  }
}

/* function definition
    name:     tmg_add_model_constraints()
    args:
    job: adds the constraints ZERO <= v <= v0 forall vertices except the
    ZERO and v0 vertex. Also make sure ZERO + c = v0
    return value:
    calls:
*/

tmg_add_model_constraints(lg, cg, v0) graph_t *lg, *cg;
register vertex_t *v0;
{
  lsGen gen;
  vertex_t *v;
  edge_t *e, *edge_z2ek, *edge_ek2z;

  /* Add constraints */
  /* --------------- */

  if (debug_type == BB || debug_type == ALL) {
    (void)fprintf(sisout, "Adding constraints\n");
  }
  foreach_vertex(cg, gen, v) {
    if (v == v0 || v == ZERO_V(cg))
      continue;

    /* Add constraints so that ZERO_V <= v */
    /* ----------------------------------- */

    if (!g_get_edge(v, ZERO_V(cg), &e)) {
      e->user_data = (gGeneric)tmg_alloc_cedge();
    }
    if (debug_type == BB || debug_type == ALL) {
      if (PID(v) > 0) {
        (void)fprintf(sisout, "r(%d) >= 0\n", PID(v));
      } else if (PID(v) < 0) {
        (void)fprintf(sisout, "e(%d) >= 0\n", -PID(v));
      }
    }
    if (EVAL(e) == NOT_SET) {
      FIXED(e) = 0;
      EVAL(e) = TRUE;
    } else if (EVAL(e) == TRUE) {
      FIXED(e) = MIN(FIXED(e), 0);
    }

    /* Add constraints so that v <= v0*/
    /* ------------------------------ */

    if (!g_get_edge(v0, v, &e)) {
      e->user_data = (gGeneric)tmg_alloc_cedge();
    }
    if (debug_type == BB || debug_type == ALL) {
      if (PID(v) > 0) {
        (void)fprintf(sisout, "r(%d) <= e(%d)\n", PID(v), array_n(CL(lg)));
      } else if (PID(v) < 0) {
        (void)fprintf(sisout, "e(%d) <= e(%d)\n", -PID(v), array_n(CL(lg)));
      }
    }
    if (EVAL(e) == NOT_SET) {
      FIXED(e) = 0;
      EVAL(e) = TRUE;
    } else if (EVAL(e) == TRUE) {
      FIXED(e) = MIN(FIXED(e), 0);
    }
  }

  /* Force ZERO_V and v0 to be separated by exactly c. */
  /* Add v0 <= ZERO_V + c                              */
  /* ------------------------------------------------- */

  if (!g_get_edge(ZERO_V(cg), v0, &edge_z2ek)) {
    edge_z2ek->user_data = (gGeneric)tmg_alloc_cedge();
  }
  DUTY(edge_z2ek) = 1;

  /* ZERO <= v0 - c */
  /* -------------- */

  if (!g_get_edge(v0, ZERO_V(cg), &edge_ek2z)) {
    edge_ek2z->user_data = (gGeneric)tmg_alloc_cedge();
  }
  DUTY(edge_ek2z) = -1;
}

/* function definition
    name:     tmg_add_phase_separation_constraints()
    args:
    job:     add the phase separartion constraints (duty cycle or
             fixed separation)
    return value:
    calls:
*/
tmg_add_phase_separation_constraints(lg, cg, v0) graph_t *lg, *cg;
vertex_t *v0;
{
  double min_sep, max_sep;
  int i;
  phase_t *phase;
  edge_t *e;
  vertex_t *old_v;

  /* Add the constraints e1 <= e2 <= e3...<= ek and min and
     max separations */
  /* ----------------------------------------------------- */

  old_v = NIL(vertex_t);
  min_sep = tmg_get_min_sep();
  max_sep = tmg_get_max_sep();

  for (i = 0; i < array_n(CL(lg)); i++) {
    phase = PHASE_LIST(cg)[i];

    /* Add minimum and maximum constraint separation*/
    /* -------------------------------------------- */

    if (!g_get_edge(phase->rise, phase->fall, &e)) {
      e->user_data = (gGeneric)tmg_alloc_cedge();
    }
    DUTY(e) = MIN(DUTY(e), max_sep);
    if (debug_type == BB || debug_type == ALL) {
      (void)fprintf(sisout, "e(%d) - r(%d) <= %.2lf * c\n", -PID(phase->fall),
                    PID(phase->rise), max_sep);
    }
    if (!g_get_edge(phase->fall, phase->rise, &e)) {
      e->user_data = (gGeneric)tmg_alloc_cedge();
    }
    DUTY(e) = MIN(DUTY(e), -min_sep);
    if (debug_type == BB || debug_type == ALL) {
      (void)fprintf(sisout, "r(%d) - e(%d) <= %.2lf * c\n", PID(phase->rise),
                    -PID(phase->fall), -min_sep);
    }

    /* e(k-1) <= e(k) */
    /* -------------- */

    if (old_v != NIL(vertex_t)) {
      if (!g_get_edge(phase->fall, old_v, &e)) {
        e->user_data = (gGeneric)tmg_alloc_cedge();
      }
      if (debug_type == BB || debug_type == ALL) {
        (void)fprintf(sisout, "e(%d) - e(%d) <= 0\n", -PID(old_v),
                      -PID(phase->fall));
      }
      if (EVAL(e) == NOT_SET) {
        FIXED(e) = 0;
        EVAL(e) = TRUE;
      } else {
        FIXED(e) = MIN(FIXED(e), 0);
      }
    }
    old_v = phase->fall;
  }
}

/* function definition
    name:     tmg_evaluate_right_hand_side()
    args:
    job:      evaluate the edge weights for the given clock cycle
              and keep the minimum
    weight
    return value:
    calls:
*/
tmg_evaluate_right_hand_side(g, c) graph_t *g;
double c;
{
  lsGen gen;
  edge_t *e;
  char *dummy;
  double *value, C[2];
  int i;
  st_generator *sgen;

  C[0] = 0;
  C[1] = c;
  foreach_edge(g, gen, e) {
    if (EVAL(e) == NOT_SET) {
      WEIGHT(e) = INFTY;
    } else {
      WEIGHT(e) = FIXED(e);
    }

    if (ABS(DUTY(e)) <= 1) {
      WEIGHT(e) = MIN(WEIGHT(e), DUTY(e) * c);
    }

    if (st_count(CONS1(e)) > 0) {
      st_foreach_item(CONS1(e), sgen, (char **)&dummy, (char **)&value) {
        i = (int)dummy;
        WEIGHT(e) = MIN(WEIGHT(e), -(*value) + i * c);
      }
    }
    if (CONS2(e) != NIL(double)) {
      for (i = 0; i < 2; i++) {
        if (CONS2(e)[i] < INFTY) {
          WEIGHT(e) = MIN(WEIGHT(e), CONS2(e)[i] + C[i]);
        }
      }
    }
    if (WEIGHT(e) == INFTY) {
      /* No constraint so far - disregard it!!*/
      IGNORE(e) = TRUE;
    }
  }
}

/* function definition
    name:     tmg_print_constraints()
    args:
    job:      print the constraints once the right hand sides
              have been evaluated
    return value:
    calls:
*/
tmg_print_constraints(g) graph_t *g;
{
  lsGen gen;
  edge_t *e;
  vertex_t *u, *v;

  foreach_edge(g, gen, e) {
    if (IGNORE(e))
      continue;
    v = g_e_dest(e);
    u = g_e_source(e);
    if (PID(v) == 0) {
      if (PID(u) > 0) {
        (void)fprintf(sisout, " -r(%d) =< %.2lf\n", PID(u), WEIGHT(e));
      } else if (PID(u) < 0) {
        (void)fprintf(sisout, " -e(%d) =< %.2lf\n", -PID(u), WEIGHT(e));
      }
      continue;
    }
    if (PID(u) == 0) {
      if (PID(v) > 0) {
        (void)fprintf(sisout, "r(%d) <= %.2lf \n", PID(v), WEIGHT(e));
      } else if (PID(v) < 0) {
        (void)fprintf(sisout, "e(%d) <= %.2lf \n", -PID(v), WEIGHT(e));
      }
      continue;
    }
    if (PID(v) > 0) {
      (void)fprintf(sisout, "r(%d) ", PID(v));
    } else if (PID(v) < 0) {
      (void)fprintf(sisout, "e(%d) ", -PID(v));
    }
    if (PID(u) > 0) {
      (void)fprintf(sisout, "- r(%d) <= %.2lf\n", PID(u), WEIGHT(e));
    } else if (PID(u) < 0) {
      (void)fprintf(sisout, "- e(%d) <= %.2lf\n", -PID(u), WEIGHT(e));
    }
  }
}

tmg_print_graph_solution(g) graph_t *g;
{
  lsGen gen;
  vertex_t *u;

  foreach_vertex(g, gen, u) {
    if (PID(u) < 0) {
      (void)fprintf(sisout, "e(%d) = %.2lf\n", -PID(u), P(u));
    } else if (PID(u) > 0) {
      (void)fprintf(sisout, "r(%d) = %.2lf\n", PID(u), P(u));
    } else {
      (void)fprintf(sisout, "ZERO = %.2lf \n", P(u));
    }
  }
}

/* function definition
    name:     tmg_circuit_clock_update()
    args:
    job:      set the rise and fall transitions of the clocks in the network
    return value:
    calls:
*/
tmg_circuit_clock_update(lg, cg, network, clock) graph_t *lg, *cg;
network_t *network;
double clock;
{
  lsGen gen;
  vertex_t *u;
  clock_edge_t transition;

  clock_set_cycletime(network, clock);
  foreach_vertex(cg, gen, u) {
    if (PID(u) < 0) {
      transition.clock = array_fetch(sis_clock_t *, CL(lg), -PID(u) - 1);
      transition.transition = FALL_TRANSITION;
      clock_set_parameter(transition, CLOCK_NOMINAL_POSITION, P(u));
    } else if (PID(u) > 0) {
      transition.clock = array_fetch(sis_clock_t *, CL(lg), PID(u) - 1);
      transition.transition = RISE_TRANSITION;
      clock_set_parameter(transition, CLOCK_NOMINAL_POSITION, P(u));
    }
  }
}

/* function definition
    name:     tmg_solve_gen_constraints()
    args:
    job:      solve the genaral formulation (with min-max phase separation)
    return value: double
    calls:   tmg_exterior_path_search(), tmg_is_feasible()
*/
double tmg_solve_gen_constraints(cg, v0, clock_lb) graph_t *cg;
vertex_t *v0;
double clock_lb;
{

  double cur_clock, L, R;

  L = clock_lb;
  R = INFTY;
  if (tmg_exterior_path_search(cg, &cur_clock, L, R)) {
    if (!tmg_is_feasible(cg, cur_clock, v0)) {
      (void)fprintf(sisout, "Serious error: contradictory results\n");
      (void)fprintf(sisout, "from 2 algorithms\n");
      return (-1.0);
    }
    tmg_print_graph_solution(cg);
    return cur_clock;
  }
  (void)fprintf(sisout, "Infeasible problem!!\n");
  return (-1.0);
}

#define NEGATIVE_CYCLE -1
#define INFEASIBLE 0
#define ALL_POSITIVE_CYCLES 2

/* function definition
    name:     tmg_exterior_path_search()
    args:
    job:      does a floyyd warshall for the given clock cycle. Implements the
    special algorithm described in Shenoy et al(. ICCAD 92)
    return value: int
    calls: tmg_init_matrix(), tmg_update_matrix(), tmg_free_matrix(),
   tmg_evaluate_matrix()
*/
int tmg_exterior_path_search(cg, clock, L, R) graph_t *cg;
double *clock, L, R;
{
  int num_v, flag;
  register int x, i, j, k;
  double val, c;
  register matrix_t *m;

  m = tmg_init_matrix(cg, L);
  num_v = NUM_V(m);

  while (TRUE) {

    /* Get new bound on clock cycle */
    /* ---------------------------- */

    c = CURRENT_CLOCK(m);

    /* Evaluate the dominant matrix entries */
    /* ------------------------------------ */

    tmg_evaluate_matrix(cg, m, c);

    /* Floyd Warshall */
    /* -------------- */

    for (x = 0; x < num_v; x++) {
      for (i = 0; i < num_v; i++) {
        for (j = 0; j < num_v; j++) {
          W_n(m)[i][j] = W_o(m)[i][j];
          BETA_n(m)[i][j] = BETA_o(m)[i][j];
          for (k = 0; k < num_v; k++) {
            if (k == i || k == j)
              continue;
            if (W_o(m)[i][k] == INFTY || W_o(m)[k][j] == INFTY)
              continue;
            if (BETA_o(m)[i][k] == INFTY || BETA_o(m)[k][j] == INFTY) {
              assert(0); /* Sanity check */
            }
            if ((val = W_o(m)[i][k] + W_o(m)[k][j]) < W_n(m)[i][j]) {
              W_n(m)[i][j] = val;
              BETA_n(m)[i][j] = BETA_o(m)[i][k] + BETA_o(m)[k][j];
            }
          }
        }
      }

      /* Search if any diagonal entries have finite values
         to update bounds
      /* ------------------------------------------------- */

      flag = tmg_update_matrix(m);

      /* If flag is negative cycle need to update clock cycle */
      /*            infeasible then return                    */
      /*            all positive cycles continue till FW is done */
      /* ------------------------------------------------------- */

      if (flag == INFEASIBLE || flag == NEGATIVE_CYCLE) {
        break;
      }
    }
    if (flag == INFEASIBLE || flag == ALL_POSITIVE_CYCLES) {
      break;
    }
  }
  tmg_free_matrix(m);
  if (flag == INFEASIBLE) {
    *clock = -1.0;
    return FALSE;
  } else if (flag == ALL_POSITIVE_CYCLES) {
    *clock = c;
    return TRUE;
  } else {
    assert(0); /* Sanity check to ensure that flag is never neg_cycle
                    on getting out of while loop*/
  }
}

/* function definition
    name:     tmg_init_matrix()
    args:
    job:      initializes the matrix for the floyd warshall
    return value: matrix_t *
    calls:
*/
matrix_t *tmg_init_matrix(g, c) graph_t *g;
double c;
{
  lsGen gen;
  vertex_t *v;
  matrix_t *m;
  int num_v;
  register int i, j;

  num_v = lsLength(g_get_vertices(g));
  m = ALLOC(matrix_t, 1);
  i = 0;
  foreach_vertex(g, gen, v) {
    if (debug_type == GENERAL || debug_type == ALL) {
      if (PID(v) < 0) {
        (void)fprintf(sisout, "e%d = %d\n", -PID(v), i);
      } else if (PID(v) > 0) {
        (void)fprintf(sisout, "r%d = %d\n", PID(v), i);
      } else {
        (void)fprintf(sisout, "ZERO = %d\n", i);
      }
    }
    MID(v) = i;
    i++;
  }
  CURRENT_CLOCK(m) = c;

  W_n(m) = ALLOC(double *, num_v);
  W_o(m) = ALLOC(double *, num_v);
  BETA_n(m) = ALLOC(double *, num_v);
  BETA_o(m) = ALLOC(double *, num_v);
  for (i = 0; i < num_v; i++) {
    W_n(m)[i] = ALLOC(double, num_v);
    W_o(m)[i] = ALLOC(double, num_v);
    BETA_o(m)[i] = ALLOC(double, num_v);
    BETA_n(m)[i] = ALLOC(double, num_v);
    for (j = 0; j < num_v; j++) {
      W_n(m)[i][j] = INFTY;
      W_o(m)[i][j] = INFTY;
      BETA_o(m)[i][j] = INFTY;
      BETA_n(m)[i][j] = INFTY;
    }
  }

  LOWER_BOUND(m) = -INFTY;
  UPPER_BOUND(m) = INFTY;
  NUM_V(m) = num_v;
  return m;
}

/* function definition
    name:     tmg_evaluate_matrix()
    args:
    job:      updates the matrix structure for a given clock cycle
    return value:
    calls:
*/
tmg_evaluate_matrix(g, m, c) graph_t *g;
matrix_t *m;
double c;
{
  lsGen gen;
  edge_t *e;
  char *dummy;
  st_generator *sgen;
  vertex_t *u, *v;
  int i, j, num_v, k;
  double *value, val;

  num_v = NUM_V(m);

  foreach_edge(g, gen, e) {
    u = g_e_source(e);
    v = g_e_dest(e);
    i = MID(u);
    j = MID(v);
    if (EVAL(e) == NOT_SET) {
      W_o(m)[i][j] = INFTY;
      BETA_o(m)[i][j] = INFTY;
    } else {
      W_o(m)[i][j] = FIXED(e);
      BETA_o(m)[i][j] = 0;
    }
    if (ABS(DUTY(e)) <= 1) {
      if (W_o(m)[i][j] > DUTY(e) * c) {
        W_o(m)[i][j] = DUTY(e) * c;
        BETA_o(m)[i][j] = DUTY(e);
      }
    }

    if (st_count(CONS1(e)) > 0) {
      st_foreach_item(CONS1(e), sgen, (char **)&dummy, (char **)&value) {
        k = (int)dummy;
        if ((val = -(*value) + k * c) < W_o(m)[i][j]) {
          W_o(m)[i][j] = val;
          BETA_o(m)[i][j] = (double)k;
        }
      }
    }
    if (CONS2(e) != NIL(double)) {
      for (k = 0; k < 2; k++) {
        if (CONS2(e)[k] < INFTY) {
          if ((val = CONS2(e)[k] + k * c) < W_o(m)[i][j]) {
            W_o(m)[i][j] = val;
            BETA_o(m)[i][j] = (double)k;
          }
        }
      }
    }
  }
  if (debug_type == GENERAL || debug_type == ALL) {
    (void)fprintf(sisout, "Weights\n");
    for (i = 0; i < num_v; i++) {
      for (j = 0; j < num_v; j++) {
        (void)fprintf(sisout, "%-10.2lf", W_o(m)[i][j]);
      }
      (void)fprintf(sisout, "\n");
    }
    (void)fprintf(sisout, "\n");

    (void)fprintf(sisout, "beta\n");
    for (i = 0; i < num_v; i++) {
      for (j = 0; j < num_v; j++) {
        (void)fprintf(sisout, "%-10.2lf", BETA_o(m)[i][j]);
      }
      (void)fprintf(sisout, "\n");
    }
    (void)fprintf(sisout, "\n");
  }
}

/* function definition
    name:     tmg_update_matrix()
    args:
    job:      Check if a negative cycle is detected in Floyd warshall.
              If so update bounds .
    return value:int
    calls:
*/

int tmg_update_matrix(m) matrix_t *m;
{
  int flag, num_v;
  register int i, j;
  double c;

  num_v = NUM_V(m);
  flag = TRUE;
  c = CURRENT_CLOCK(m);
  for (i = 0; i < num_v; i++) {
    for (j = 0; j < num_v; j++) {
      if (i == j) {                   /* Diagonal entry */
                                      /*--------------  */
        if (W_n(m)[i][j] < -EPS1) {   /* Entry is -ve */
                                      /* ------------ */
          if (BETA_n(m)[i][i] <= 0) { /* Coeff of c is <= 0 */
                                      /*--------------------*/
            return INFEASIBLE;
          } else {
            LOWER_BOUND(m) =
                MAX(LOWER_BOUND(m), (-W_n(m)[i][i] / BETA_n(m)[i][i]) + c);
            flag = FALSE;
          }
        } else if (W_n(m)[i][i] >= 0 && W_n(m)[i][i] < INFTY) {
          if (BETA_n(m)[i][i] < 0) {
            UPPER_BOUND(m) =
                MIN(UPPER_BOUND(m), (W_n(m)[i][i] / -BETA_n(m)[i][i]) + c);
          }
        }
        W_o(m)[i][j] = W_n(m)[i][j];
        BETA_o(m)[i][j] = BETA_n(m)[i][j];
      } else { /* old iterate = new iterate */
        if (W_n(m)[i][j] < INFTY) {
          W_o(m)[i][j] = W_n(m)[i][j];
          BETA_o(m)[i][j] = BETA_n(m)[i][j];
        }
      }
    }
  }
  if (debug_type == GENERAL || debug_type == ALL) {
    for (i = 0; i < num_v; i++) {
      for (j = 0; j < num_v; j++) {
        (void)fprintf(sisout, "%-10.2lf ", W_o(m)[i][j]);
      }
      (void)fprintf(sisout, "\n");
    }
    (void)fprintf(sisout, "\n");
  }

  if (!flag) { /* There is negative cycle */
    /* CLearly C_L > c */
    /* Check for c_L < c_U */
    if (LOWER_BOUND(m) > UPPER_BOUND(m))
      return INFEASIBLE;
    CURRENT_CLOCK(m) = LOWER_BOUND(m);
    /* Initialize for new Floyd Warshall */
    /* --------------------------------- */
    for (i = 0; i < num_v; i++) {
      for (j = 0; j < num_v; j++) {
        W_o(m)[i][j] = INFTY;
        W_n(m)[i][j] = INFTY;
        BETA_o(m)[i][j] = INFTY;
        BETA_n(m)[i][j] = INFTY;
      }
    }
    return NEGATIVE_CYCLE;
  }
  return ALL_POSITIVE_CYCLES;
}

/* function definition
    name:     tmg_free_matrix()
    args:
    job:     Frees the memory associated with the mtrix
    return value:
    calls:
*/
tmg_free_matrix(m) matrix_t *m;
{
  int num_v, i;

  num_v = NUM_V(m);
  for (i = 0; i < num_v; i++) {
    FREE(W_o(m)[i]);
    FREE(W_n(m)[i]);
    FREE(BETA_n(m)[i]);
    FREE(BETA_o(m)[i]);
  }
  FREE(W_o(m));
  FREE(W_n(m));
  FREE(BETA_n(m));
  FREE(BETA_o(m));
  FREE(m);
}
#endif /* SIS */
