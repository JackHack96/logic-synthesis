
#ifdef SIS
/* Routines to enforce CSC in an ASTG
 */

#include "sis.h"
#ifdef MDD
#include "mdd.h"
#endif
#include "astg_core.h"
#include "astg_int.h"
#include "bwd_int.h"

#define TRANS_TYPE(c) (((c) == '0') ? ASTG_POS_X : ASTG_NEG_X)

struct ptr_pair_struct {
  char *p1, *p2;
};
typedef struct ptr_pair_struct ptr_pair;

/* Structure to store pairs of markings that must be distinguished inserting
 * an appropriate state sig_p transition
 */
struct marking_pair_struct {
  char *from_state;
  /* array of astg_marking * */
  array_t *from_markings;
  char *to_state;
  /* array of astg_marking * */
  array_t *to_markings;
};
typedef struct marking_pair_struct marking_pair_t;

/* array of (marking_pair_t) */
static array_t *marking_pairs;
/* array of (astg_signal *) */
static array_t *partitioning_signals;

/* from bin_mincov.c */
extern int call_count;

/* dummy signal for dummy transitions... */
static astg_signal *dum_sig;
static int dum_i;

/* symbol table with costs */
static st_table *g_sig_cost;

/* column selection in the binate covering formulation; first we put
 * "nsig" signals, then we put the states
 */
#define POS_STATE(nsig, ncl, cl, st) (2 * ((nsig) + (ncl) * (st) + (cl)))
#define NEG_STATE(nsig, ncl, cl, st) (2 * ((nsig) + (ncl) * (st) + (cl)) + 1)
#define POS_SIG(sig) (2 * (sig))
#define NEG_SIG(sig) (2 * (sig) + 1)

static int ptr_pair_cmp(key1, key2) char *key1, *key2;
{
  int result;
  ptr_pair *pair1, *pair2;

  pair1 = (ptr_pair *)key1;
  pair2 = (ptr_pair *)key2;
  result = st_ptrcmp(pair1->p1, pair2->p1);
  if (result) {
    return result;
  }
  result = st_ptrcmp(pair1->p2, pair2->p2);
  return result;
}

static int ptr_pair_hash(key, modulus) char *key;
int modulus;
{
  int result;
  ptr_pair *pair;

  pair = (ptr_pair *)key;
  result = st_ptrhash(pair->p1, modulus) ^ st_ptrhash(pair->p2, modulus);
  return result % modulus;
}

static void constrain(str, astg, trans1, trans2) char *str;
astg_graph *astg;
astg_trans *trans1, *trans2;
{
  static st_table *printed;
  static astg_graph *curr_astg;
  ptr_pair pair, *pairptr;
  st_generator *stgen;

  if (astg_is_rel(trans1, trans2)) {
    return;
  }

  if (astg != curr_astg) {
    if (printed != NIL(st_table)) {
      st_foreach_item(printed, stgen, (char **)&pairptr, NIL(char *)) {
        FREE(pairptr);
      }
      st_free_table(printed);
    }
    printed = st_init_table(ptr_pair_cmp, ptr_pair_hash);
    curr_astg = astg;
  }

  pair.p1 = (char *)trans1;
  pair.p2 = (char *)trans2;
  if (st_is_member(printed, (char *)&pair)) {
    return;
  }

  pairptr = ALLOC(ptr_pair, 1);
  *pairptr = pair;
  st_insert(printed, (char *)pairptr, NIL(char));

  fprintf(siserr, "warning: %s: may need constraint %s -> %s\n", str,
          astg_trans_name(trans1), astg_trans_name(trans2));
}

static int signal_cost(sig_p, astg) astg_signal *sig_p;
astg_graph *astg;
{
  int cost;

  if (g_sig_cost != NIL(st_table) &&
      st_lookup(g_sig_cost, astg_signal_name(sig_p), (char **)&cost)) {
    return cost;
  }
  if (astg_signal_type(sig_p) == ASTG_INPUT_SIG) {
    return MAX(100, 2 * astg->n_sig);
  } else {
    return 1;
  }
}

static int subset_cost(subset, astg) astg_scode subset;
astg_graph *astg;
{
  int cost;
  astg_generator gen;
  astg_signal *sig_p;

  cost = 0;
  astg_foreach_signal(astg, gen, sig_p) {
    if (subset & bwd_astg_state_bit(sig_p)) {
      cost += signal_cost(sig_p, astg);
    }
  }
  return cost;
}

static void print_subset(subset, astg) astg_scode subset;
astg_graph *astg;
{
  astg_generator gen;
  astg_signal *sig_p;

  if (astg_debug_flag > 1) {
    fprintf(sisout, " ");
    astg_foreach_signal(astg, gen, sig_p) {
      if (subset & bwd_astg_state_bit(sig_p)) {
        fprintf(sisout, "1");
      } else {
        fprintf(sisout, "0");
      }
    }
  }
  astg_foreach_signal(astg, gen, sig_p) {
    if (subset & bwd_astg_state_bit(sig_p)) {
      if (astg_signal_type(sig_p) == ASTG_INPUT_SIG) {
        fprintf(sisout, " %s (i)", sig_p->name);
      } else if (astg_signal_type(sig_p) == ASTG_OUTPUT_SIG) {
        fprintf(sisout, " %s (o)", sig_p->name);
      } else if (astg_signal_type(sig_p) == ASTG_INTERNAL_SIG) {
        fprintf(sisout, " %s (x)", sig_p->name);
      }
    }
  }
  if (astg_debug_flag > 0) {
    fprintf(sisout, " (cost %d)", subset_cost(subset, astg));
  }
  fprintf(sisout, "\n");
}

static void equiv_free(equiv_states) equiv_t *equiv_states;
{
  array_free(equiv_states->index_state);
  equiv_states->index_state = NIL(array_t);
  st_free_table(equiv_states->state_index);
  equiv_states->state_index = NIL(st_table);
  sf_free(equiv_states->classes);
  equiv_states->classes = NIL(set_family_t);
}

static equiv_t *equiv_alloc(nstates, nclasses) int nstates, nclasses;
{
  equiv_t *equiv_states;
  pset p, last;

  equiv_states = ALLOC(equiv_t, 1);
  equiv_states->index_state = array_alloc(vertex_t *, nstates);
  equiv_states->state_index = st_init_table_with_params(
      st_ptrcmp, st_ptrhash, ST_DEFAULT_INIT_TABLE_SIZE, /* density */ 1,
      ST_DEFAULT_GROW_FACTOR, ST_DEFAULT_REORDER_FLAG);
  equiv_states->classes = sf_new(nclasses, nstates);
  equiv_states->classes->count = nclasses;
  foreach_set(equiv_states->classes, last, p) { set_clear(p, nstates); }
  return equiv_states;
}

equiv_t *bwd_read_equiv_states(file, stg) FILE *file;
graph_t *stg;
{
  char buf[256], name[256], *str;
  int class, n, nclasses, idx, i, j, len;
  equiv_t *equiv_states;
  pset p;
  int nstates;
  vertex_t *state;
  array_t *names;

  if (global_orig_stg_names == NIL(st_table) ||
      global_orig_stg == NIL(graph_t)) {
    fprintf(siserr, "no state transition graph (use astg_to_stg)\n");
    return NIL(equiv_t);
  }

  nstates = stg_get_num_states(global_orig_stg);

  nclasses = -1;
  while (fgets(buf, (sizeof(buf) / sizeof(char)), file) != NULL) {
    if (sscanf(buf, "#begin_classes %d", &nclasses) == 1) {
      break;
    }
  }
  if (nclasses < 0) {
    fprintf(siserr, "no class information in minimizer output\n");
    return NIL(equiv_t);
  }

  equiv_states = equiv_alloc(nstates, nclasses);
  idx = 0;
  while (fgets(buf, (sizeof(buf) / sizeof(char)), file) != NULL) {
    if (!strcmp(buf, "#end_classes\n")) {
      break;
    }
    n = sscanf(buf, "# %s %d", name, &class);
    if (n != 2) {
      fprintf(siserr, "invalid class information in minimizer output:\n%s\n",
              buf);
      equiv_free(equiv_states);
      return NIL(equiv_t);
    }
    if (!st_lookup(global_orig_stg_names, (char *)name, (char **)&names)) {
      fprintf(siserr, "invalid class information in minimizer output:\n%s\n",
              buf);
      equiv_free(equiv_states);
      return NIL(equiv_t);
    }
    for (j = 0; j < array_n(names); j++) {
      state =
          bwd_get_state_by_name(global_orig_stg, array_fetch(char *, names, j));
      if (!st_lookup(equiv_states->state_index, (char *)state, (char **)&i)) {
        st_insert(equiv_states->state_index, (char *)state, (char *)idx);
        array_insert(vertex_t *, equiv_states->index_state, idx, state);
        i = idx++;
      }
      set_insert(GETSET(equiv_states->classes, class), i);
    }
  }
  if (idx != nstates) {
    fprintf(siserr,
            "not enough states in class information in minimizer output (%d "
            "vs. %d)\n",
            idx, nstates);
    equiv_free(equiv_states);
    return NIL(equiv_t);
  }

  if (astg_debug_flag > 1) {
    fprintf(sisout, "Equivalence classes:\n");
    foreachi_set(equiv_states->classes, i, p) {
      fprintf(sisout, "%2d ", i);
      len = 3;
      for (j = 0; j < array_n(equiv_states->index_state); j++) {
        if (is_in_set(p, j)) {
          state = array_fetch(vertex_t *, equiv_states->index_state, j);
          str = stg_get_state_name(state);
          len += strlen(str) + 1;
          if (len >= 80) {
            fprintf(sisout, "\n   ");
            len = 3 + strlen(str) + 1;
          }
          fprintf(sisout, "%s ", str);
        }
      }
      fprintf(sisout, "\n");
    }
  }

  return equiv_states;
}

static int same_class(classes, st1, st2) pset_family classes;
int st1, st2;
{
  pset last, class;

  foreach_set(classes, last, class) {
    if (is_in_set(class, st1) && is_in_set(class, st2)) {
      return 1;
    }
  }

  return 0;
}

/* Test if all the signals that are enabled in state2 and not in state1
 * are in subset (so that subset "distinguishes" among them).
 */
static int distinguishable(edge, subset, edge_trans,
                           state_marking) edge_t *edge;
astg_scode subset;
st_table *edge_trans, *state_marking;
{
  astg_scode new_enabled;

  new_enabled = bwd_new_enabled_signals(edge, edge_trans, state_marking);
  if (new_enabled == 0 || (subset & new_enabled) != new_enabled) {
    return 0;
  }

  return 1;
}

#ifdef MDD
/* FROM YOSINORI WATANABE
 * br_dfs_bdd() recursively performs a depth first search over nodes
 * in BDD 'f' and assigns post_visit numbers for all the nodes except
 * 0 terminal nodes.  The post_visit number is a number assigned to a
 * node after all of its children have been visited.  The number
 * starts with 'start_num'.  Once a node that is not a 0 terminal is
 * assigned a post_visit number, then the number is set in a hash_table
 * 'id_table' with key as the pointer to the node.  Also, the pointer
 * to the node is stored in a hash_table 'order' with key as its post_
 * visit number. *total_num_ptr is returned as the number of the
 * nodes in f except the 0 terminal nodes plus start_num.
 */
static void br_dfs_bdd(mg, f, id_table, order, start_num,
                       total_num_ptr) bdd_manager *mg;
bdd_node *f;
st_table *id_table, *order;
int start_num, *total_num_ptr;
{
  int total;
  bdd_node *v0, *v1;
  bdd_node *b0, *b1;

  b0 = BDD_ZERO(mg);
  b1 = BDD_ONE(mg);
  if (st_is_member(id_table, (char *)f)) {
    *total_num_ptr = start_num;
    return;
  }
  if (f == b0) {
    *total_num_ptr = start_num;
    return;
  }
  if (f == b1) {
    (void)st_insert(id_table, (char *)f, (char *)start_num);
    (void)st_insert(order, (char *)start_num, (char *)f);
    *total_num_ptr = start_num + 1;
    return;
  }

  bdd_get_branches(f, &v1, &v0);

  br_dfs_bdd(mg, v1, id_table, order, start_num, &total);

  start_num = total;
  br_dfs_bdd(mg, v0, id_table, order, start_num, &total);

  /* The post_cisit number of f is total */
  (void)st_insert(id_table, (char *)f, (char *)total);
  (void)st_insert(order, (char *)total, (char *)f);
  *total_num_ptr = total + 1;

  return;
}

/* FROM YOSINORI WATANABE
 * br_shortestpath() finds the shortest path in a BDD f that
 * ends with a 1 terminal, where an edge has a weight 'wtt' if it
 * is a then edge and has a weight 'wte' if it is an else edge.
 * This routine does not check if f is NIL or is a constant
 * terminal BDD.  This routine should not be used for these
 * cases.
 */
static st_table *br_shortestpath(f, wtt_table, wte_table) bdd_t *f;
st_table *wtt_table, *wte_table;
{
  st_table *id_table, *order, *visited;
  int num_nodes, t_num, i, k;
  int *length, *phase, *terminal;
  int wtt, wte;
  bdd_node **parent, *v, *w0, *w1;
  bdd_node *b0, *b1;
  char *value, *id;
  int best_length, path_len, best_i;
  bdd_manager *mg;
  st_table *result;

  mg = f->bdd;
  b0 = BDD_ZERO(mg);
  b1 = BDD_ONE(mg);

  if (f->node == b0) {
    return NIL(st_table);
  } else if (f->node == b1) {
    return NIL(st_table);
  }
  /* id_table contains a post_visit number of the top node of
   * BDD 'g' with g as a key.
   */
  id_table = st_init_table(st_ptrcmp, st_ptrhash);
  /* order contains a  BDD 'g' with the post_visit number of the
   * top node of g in the depth first search of BDD 'f'.
   */
  order = st_init_table(st_numcmp, st_numhash);

  br_dfs_bdd(mg, f->node, id_table, order, 0, &num_nodes);

  /* num_nodes is the number of nodes visited in the dfs. */
  length = ALLOC(int, num_nodes);
  parent = ALLOC(bdd_node *, num_nodes);
  phase = ALLOC(int, num_nodes);
  terminal = ALLOC(int, num_nodes);
  visited = st_init_table(st_ptrcmp, st_ptrhash);

  /* length[i] gives the length of the shortest path of the
   * BDD node with the post_visit number i from the top node of f.
   * parent[i] gives the parent BDD of the BDD node with
   * the post_visit number i in the path.  phase[i] is 1 if the
   * BDD node with  id = i is a then child of parent[i] and
   * 0 if an else child.  terminal is an array of post_visit number
   * of BDDs of 1 terminal nodes in f.
   */
  for (i = 0; i < num_nodes - 1; i++) {
    length[i] = 99999;
  }
  length[i] = 0;
  parent[i] = NIL(bdd_node);

  /* Each BDD node in f visited in DFS is processed in topological
   * order, which is the reverse post_visit order.
   * The first node to be processed is the top node of f.
   */
  t_num = 0;

  for (i = num_nodes - 1; i >= 0; i--) {
    (void)st_lookup(order, (char *)i, &value);
    v = (bdd_node *)value;

    /* Skip if v is a leaf. */
    if ((v == b0) || (v == b1))
      continue;

    bdd_get_branches(v, &w1, &w0);

    /* Labeling the then child */
    if (w1 != b0) { /* Skip if w is 0 terminal. */
      (void)st_lookup(id_table, (char *)w1, &id);
      if (!st_lookup(wtt_table, (char *)w1->id, (char **)&wtt)) {
        wtt = 0;
      }
      k = (int)id;
      if (length[k] > (wtt + length[i])) {
        length[k] = wtt + length[i];
        parent[k] = v;
        phase[k] = 1;
      }
      if ((w1 == b1) && (!st_is_member(visited, (char *)w1))) {
        (void)st_insert(visited, (char *)w1, (char *)k);
        terminal[t_num++] = k;
      }
    }

    /* Labeling the else child */
    if (w0 != b0) {
      (void)st_lookup(id_table, (char *)w0, &id);
      if (wte_table == NIL(st_table) ||
          !st_lookup(wte_table, (char *)w0->id, (char **)&wte)) {
        wte = 0;
      }
      k = (int)id;
      if (length[k] > (wte + length[i])) {
        length[k] = wte + length[i];
        parent[k] = v;
        phase[k] = 0;
      }
      if ((w0 == b1) && (!st_is_member(visited, (char *)w0))) {
        (void)st_insert(visited, (char *)w0, (char *)k);
        terminal[t_num++] = k;
      }
    }
  }

  /* Look for the terminal with the shortest path */
  best_length = 99999;
  for (i = 0; i < t_num; i++) {
    path_len = length[terminal[i]];
    if (best_length > path_len) {
      best_length = path_len;
      best_i = terminal[i];
    }
  }
  (void)st_lookup(order, (char *)best_i, &value);
  v = (bdd_node *)value;
  k = best_i;

  /* setup brpt_t * data structure. */
  result = st_init_table(st_ptrcmp, st_ptrhash);
  while (parent[k] != NIL(bdd_node)) {
    if (v != b1) {
      st_insert(result, (char *)v->id, (char *)phase[k]);
    }
    v = parent[k];
    (void)st_lookup(id_table, (char *)v, &id);
    k = (int)id;
  }

  /* cleanup */
  FREE(length);
  FREE(phase);
  FREE(terminal);
  FREE(parent);
  st_free_table(id_table);
  st_free_table(order);
  st_free_table(visited);

  return result;
}

/* Find a shortest path from the root to the "1" leaf on the MDD, using sig_p
 * cost as weight. Use Yosinori's function for the shortest path computation.
 */

static astg_scode mdd_subset(mgr, mdd, sig_mdd_var, wtt_table) mdd_manager *mgr;
mdd_t *mdd;
st_table *sig_mdd_var, *wtt_table;
{
  st_table *var_phase;
  int phase;
  astg_scode subset;
  bdd_t *lit;
  st_generator *stgen;
  astg_signal *sig_p;

  var_phase = br_shortestpath(mdd, wtt_table, NIL(st_table));

  subset = 0;
  st_foreach_item(sig_mdd_var, stgen, (char **)&sig_p, (char **)&lit) {
    if (!st_lookup(var_phase, (char *)lit->node->id, (char **)&phase)) {
      continue;
    }
    if (phase) {
      subset |= bwd_astg_state_bit(sig_p);
    }
  }

  st_free_table(var_phase);

  return subset;
}
#endif

/* Return a set of signals in the ASTG sufficient to partition the state graph
 * so that each partition includes only states belonging only to one STG
 * equivalence class.
 */
static astg_scode find_partitioning_signals(astg, stg, equiv_states, index_sig,
                                            visited, mincov_option, use_mdd,
                                            edge_trans,
                                            state_marking) astg_graph *astg;
graph_t *stg;
equiv_t *equiv_states;
array_t *index_sig;
pset visited;
int mincov_option, use_mdd;
st_table *edge_trans, *state_marking;
{
  int cl1, cl2, s1, s2, i, row, found, count;
  int nclasses, nsig, nstates;
  int mincov_debug = 0;
  astg_scode subset, new_enabled;
  lsGen gen;
  astg_generator sgen;
  vertex_t *state1, *state2;
  edge_t *edge;
  pset class1, class2, single_class;
  sm_matrix *M;
  sm_row *cover;
  sm_element *el;
  astg_signal *sig_p;
  int *weight;
#ifdef MDD
  int v;
  array_t *mvar_sizes, *values;
  st_table *state_mdd_var, *wtt_table, *sig_mdd_var;
  st_generator *stgen;
  mdd_manager *mgr;
  mdd_t *mdd, *mdd1, *lit1, *lit2, *lit3;
#endif

  call_count = 0;
  nclasses = equiv_states->classes->count;
  nstates = array_n(equiv_states->index_state);
  nsig = array_n(index_sig);
  single_class = set_new(nstates);
  if (use_mdd) {
#ifdef MDD
    state_mdd_var = st_init_table(st_ptrcmp, st_ptrhash);
    wtt_table = st_init_table(st_ptrcmp, st_ptrhash);
    mvar_sizes = array_alloc(int, 0);
    sig_mdd_var = st_init_table(st_ptrcmp, st_ptrhash);
#else
    fail("MDD's not supported\n");
#endif
  } else {
    M = sm_alloc();
  }

  for (s1 = 0; s1 < nstates; s1++) {
    count = 0;
    foreachi_set(equiv_states->classes, cl1, class1) {
      if (is_in_set(class1, s1)) {
        count++;
      }
    }
    if (count == 1) {
      set_insert(single_class, s1);
    }
  }

  /* for each state create a set of rows stating that it must be in one
   * and only one class:
   * for each state
   *     create a row with all its classes in the positive phase
   *     for each class to which it does not belong
   *         create a row with the class in the negative phase
   *     for each pair of classes to which it belongs
   *         create a row with the classes in the negative phase
   */
  for (s1 = 0; s1 < nstates; s1++) {
    state1 = array_fetch(vertex_t *, equiv_states->index_state, s1);
    if (is_in_set(single_class, s1)) {
      found = 0;
      foreach_state_outedge(state1, gen, edge) {
        state2 = stg_edge_to_state(edge);
        if (state1 == state2) {
          continue;
        }
        assert(
            st_lookup(equiv_states->state_index, (char *)state2, (char **)&s2));
        if (!is_in_set(single_class, s2) ||
            !same_class(equiv_states->classes, s1, s2)) {
          found = 1;
          lsFinish(gen);
          break;
        }
      }
      if (!found) {
        foreach_state_inedge(state1, gen, edge) {
          state2 = stg_edge_from_state(edge);
          if (state1 == state2) {
            continue;
          }
          assert(st_lookup(equiv_states->state_index, (char *)state2,
                           (char **)&s2));
          if (!is_in_set(single_class, s2) ||
              !same_class(equiv_states->classes, s1, s2)) {
            found = 1;
            lsFinish(gen);
            break;
          }
        }
      }

      if (!found) {
        if (astg_debug_flag > 4) {
          fprintf(sisout, "skipping %s\n", stg_get_state_name(state1));
        }
        continue;
      }
    }
    if (use_mdd) {
#ifdef MDD
      assert(!st_insert(state_mdd_var, (char *)state1,
                        (char *)array_n(mvar_sizes)));
      array_insert_last(int, mvar_sizes, nclasses);
#endif
    } else {
      row = M->nrows;
      if (astg_debug_flag > 3) {
        fprintf(sisout, " %d:", row);
      }
      foreachi_set(equiv_states->classes, cl1, class1) {
        if (is_in_set(class1, s1)) {
          sm_insert(M, row, POS_STATE(nsig, nclasses, cl1, s1));
          if (astg_debug_flag > 3) {
            fprintf(sisout, " ( %s %d )", stg_get_state_name(state1), cl1);
          }
        }
      }
      if (astg_debug_flag > 3) {
        fprintf(sisout, "\n");
      }
      foreachi_set(equiv_states->classes, cl1, class1) {
        if (is_in_set(class1, s1)) {
          for (cl2 = cl1 + 1; cl2 < nclasses; cl2++) {
            class2 = GETSET(equiv_states->classes, cl2);
            if (!is_in_set(class2, s1)) {
              continue;
            }
            row = M->nrows;
            sm_insert(M, row, NEG_STATE(nsig, nclasses, cl1, s1));
            sm_insert(M, row, NEG_STATE(nsig, nclasses, cl2, s1));
            if (astg_debug_flag > 3) {
              fprintf(sisout, " %d: ! ( %s %d ) ! ( %s %d )\n", row,
                      stg_get_state_name(state1), cl1,
                      stg_get_state_name(state1), cl2);
            }
          }
        } else {
          row = M->nrows;
          sm_insert(M, row, NEG_STATE(nsig, nclasses, cl1, s1));
          if (astg_debug_flag > 3) {
            fprintf(sisout, " %d: ! ( %s %d )\n", row,
                    stg_get_state_name(state1), cl1);
          }
        }
      }
    }
  }

  /* assert the range of values for each variable */
  if (use_mdd) {
#ifdef MDD
    /* we should really do something smarter for variable ordering... */
    mgr = mdd_init(mvar_sizes);
    array_free(mvar_sizes);

    mdd = bdd_one(mgr);

    st_foreach_item(state_mdd_var, stgen, (char **)&state1, (char **)&v) {
      values = array_alloc(int, 0);
      assert(
          st_lookup(equiv_states->state_index, (char *)state1, (char **)&s1));
      foreachi_set(equiv_states->classes, cl1, class1) {
        if (is_in_set(class1, s1)) {
          array_insert_last(int, values, cl1);
        }
      }
      lit1 = mdd_literal(mgr, v, values);
      array_free(values);

      mdd = mdd_and(mdd, lit1, 1, 1);
    }

    astg_foreach_signal(astg, sgen, sig_p) {
      lit1 = bdd_create_variable(mgr);
      st_insert(sig_mdd_var, (char *)sig_p, (char *)lit1);
      st_insert(wtt_table, (char *)lit1->node->id,
                (char *)signal_cost(sig_p, astg));
    }
#endif
  }

  /* for each pair of states that are adjacent and may not be in the same
   * class (i.e. with the exception of a pair of states that belong to only
   * one class, and this happens to be the same class), create a set of rows
   * stating that they must be distinguishable:
   * for each edge such that the states may be in a different class
   *   for each class to which from may belong
   *     if there exists a distinguishing set
   *         for each sig_p distinguishing the edge
   *             create a row with the negative phase of from and class,
   *				   the positive phase of to and class, the sig_p
   *     else
   *         create a row with the negative phase of from and class,
   *             the positive phase of to
   */

  stg_foreach_transition(stg, gen, edge) {
    state1 = stg_edge_from_state(edge);
    state2 = stg_edge_to_state(edge);
    if (state1 == state2) {
      continue;
    }
    assert(st_lookup(equiv_states->state_index, (char *)state1, (char **)&s1));
    assert(st_lookup(equiv_states->state_index, (char *)state2, (char **)&s2));
    if (is_in_set(single_class, s1) && is_in_set(single_class, s2) &&
        same_class(equiv_states->classes, s1, s2)) {
      continue;
    }

    new_enabled = bwd_new_enabled_signals(edge, edge_trans, state_marking);
    foreachi_set(equiv_states->classes, cl1, class1) {
      if (!is_in_set(class1, s1)) {
        continue;
      }

      if (new_enabled) {
        astg_foreach_signal(astg, sgen, sig_p) {
          if (!(new_enabled & bwd_astg_state_bit(sig_p))) {
            continue;
          }
          i = bwd_log2(bwd_astg_state_bit(sig_p));
          /* we should use mdd_eq here !!!!!!! */
          if (use_mdd) {
#ifdef MDD
            assert(st_lookup(state_mdd_var, (char *)state1, (char **)&v));
            values = array_alloc(int, 0);
            array_insert_last(int, values, cl1);
            lit1 = mdd_literal(mgr, v, values);
            array_free(values);

            assert(st_lookup(state_mdd_var, (char *)state2, (char **)&v));
            values = array_alloc(int, 0);
            array_insert_last(int, values, cl1);
            lit2 = mdd_literal(mgr, v, values);
            array_free(values);

            mdd1 = bdd_or(lit1, lit2, 0, 1);
            assert(st_lookup(sig_mdd_var, (char *)sig_p, (char **)&lit3));
            mdd1 = bdd_or(mdd1, lit3, 1, 1);
            mdd = mdd_and(mdd, mdd1, 1, 1);
#endif
          } else {
            row = M->nrows;
            sm_insert(M, row, NEG_STATE(nsig, nclasses, cl1, s1));
            sm_insert(M, row, POS_STATE(nsig, nclasses, cl1, s2));
            sm_insert(M, row, POS_SIG(i));
            if (astg_debug_flag > 3) {
              sig_p = array_fetch(astg_signal *, index_sig, i);
              fprintf(sisout, " %d: ! ( %s %d ) ( %s %d ) %s\n", row,
                      stg_get_state_name(state1), cl1,
                      stg_get_state_name(state2), cl1, sig_p->name);
            }
          }
        }
      } else {
        if (use_mdd) {
#ifdef MDD
          assert(st_lookup(state_mdd_var, (char *)state1, (char **)&v));
          values = array_alloc(int, 0);
          array_insert_last(int, values, cl1);
          lit1 = mdd_literal(mgr, v, values);
          array_free(values);

          assert(st_lookup(state_mdd_var, (char *)state2, (char **)&v));
          values = array_alloc(int, 0);
          array_insert_last(int, values, cl1);
          lit2 = mdd_literal(mgr, v, values);
          array_free(values);

          mdd1 = bdd_or(lit1, lit2, 0, 1);
          mdd = mdd_and(mdd, mdd1, 1, 1);
#endif
        } else {
          row = M->nrows;
          sm_insert(M, row, NEG_STATE(nsig, nclasses, cl1, s1));
          sm_insert(M, row, POS_STATE(nsig, nclasses, cl1, s2));
          if (astg_debug_flag > 3) {
            fprintf(sisout, " %d: ! ( %s %d ) ( %s %d )\n", row,
                    stg_get_state_name(state1), cl1, stg_get_state_name(state2),
                    cl1);
          }
        }
      }
    }
  }

  if (use_mdd) {
#ifdef MDD
    if (astg_debug_flag > 1) {
      fprintf(sisout, "bdd size: %d\n", mdd_size(mdd));
    }
    subset = mdd_subset(mgr, mdd, sig_mdd_var, wtt_table);
    mdd_quit(mgr);
    st_free_table(sig_mdd_var);
    st_free_table(state_mdd_var);
    st_free_table(wtt_table);
#endif
  } else {
    if (M->last_col == NIL(sm_col)) {
      fprintf(siserr, "empty matrix ???\n");
      subset = 0;
      return subset;
    }

    weight = ALLOC(int, M->last_col->col_num + 1);
    for (i = 0; i < M->last_col->col_num + 1; i++) {
      weight[i] = 0;
    }
    astg_foreach_signal(astg, sgen, sig_p) {
      weight[POS_SIG(bwd_log2(bwd_astg_state_bit(sig_p)))] =
          signal_cost(sig_p, astg);
    }

    sm_row_dominance(M);
    if (astg_debug_flag > 2) {
      mincov_debug = 1;
    }

    cover = sm_mat_bin_minimum_cover(M, weight, /* heuristic */ 0, mincov_debug,
                                     /* ubound */ 0, /* option */ mincov_option,
                                     /* record_fun */ (int (*)())0);

    subset = 0;
    sm_foreach_row_element(cover, el) {
      if (el->col_num >= 2 * array_n(index_sig) || el->col_num % 2) {
        continue;
      }
      subset |= 1 << (el->col_num / 2);
    }

    sm_row_free(cover);
    sm_free(M);
  }
  set_free(single_class);

  return subset;
}

static int is_forbidden(cur_trans_sets, edge_trans,
                        edge) st_table *cur_trans_sets,
    *edge_trans;
edge_t *edge;
{
  array_t *trans_arr;
  int i;
  astg_trans *trans;

  assert(st_lookup(edge_trans, (char *)edge, (char **)&trans_arr));
  for (i = 0; i < array_n(trans_arr); i++) {
    trans = array_fetch(astg_trans *, trans_arr, i);
    if (st_is_member(cur_trans_sets, (char *)trans)) {
      return 1;
    }
  }
  return 0;
}

/* return 1 if something changed */
static int closure(stg, allowed_partitions, equiv_states, trans_comb,
                   edge_trans) graph_t *stg;
array_t *allowed_partitions;
equiv_t *equiv_states;
array_t *trans_comb;
st_table *edge_trans;
{
  lsGen gen;
  edge_t *edge;
  vertex_t *from, *to;
  pset intersection, class, part;
  int s, e, cl, changed, result, j;
  char *input;
  array_t *edges;
  st_generator *stgen;
  st_table *in_to_edges, *cur_trans_sets;
  astg_trans *trans;

  intersection = set_new(equiv_states->classes->count);

  /* create a symbol table with all edges with the same label */
  in_to_edges = st_init_table(strcmp, st_strhash);
  stg_foreach_transition(stg, gen, edge) {
    if (stg_edge_from_state(edge) == stg_edge_to_state(edge)) {
      continue;
    }
    input = stg_edge_input_string(edge);
    if (!st_lookup(in_to_edges, (char *)input, (char **)&edges)) {
      edges = array_alloc(edge_t *, 0);
      st_insert(in_to_edges, (char *)input, (char *)edges);
    }
    array_insert_last(edge_t *, edges, edge);
  }
  result = 0;

  do {
    changed = 0;

    /* for each set of excluded transitions */
    for (j = 0; j < array_n(trans_comb); j++) {
      cur_trans_sets = array_fetch(st_table *, trans_comb, j);
      if (astg_debug_flag > 2 && st_count(cur_trans_sets)) {
        fprintf(sisout, "excluding transitions");
        st_foreach_item(cur_trans_sets, stgen, (char **)&trans, NIL(char *)) {
          fprintf(sisout, " %s", astg_trans_name(trans));
        }
        fprintf(sisout, "\n");
      }

      /* for each input label, for each class, for each from_state in
       * this class with this label, intersect the allowed next states
       */
      st_foreach_item(in_to_edges, stgen, (char **)&input, (char **)&edges) {
        if (astg_debug_flag > 3) {
          fprintf(sisout, "examining input %s\n", input);
        }
        foreachi_set(equiv_states->classes, cl, class) {
          if (astg_debug_flag > 3) {
            fprintf(sisout, "examining class %d\n", cl);
          }
          set_fill(intersection, equiv_states->classes->count);
          for (e = 0; e < array_n(edges); e++) {
            edge = array_fetch(edge_t *, edges, e);
            /* check if associated with the forbidden transitions */
            if (is_forbidden(cur_trans_sets, edge_trans, edge)) {
              continue;
            }

            /* check if the "from" state is in this class */
            from = stg_edge_from_state(edge);
            assert(st_lookup(equiv_states->state_index, (char *)from,
                             (char **)&s));
            if (!is_in_set(class, s)) {
              continue;
            }

            /* intersect the classes of the "to" states */
            to = stg_edge_to_state(edge);
            assert(
                st_lookup(equiv_states->state_index, (char *)to, (char **)&s));
            part = array_fetch(pset, allowed_partitions, s);
            set_and(intersection, intersection, part);
            if (astg_debug_flag > 3) {
              fprintf(sisout, "%s -> %s part %s ", stg_get_state_name(from),
                      stg_get_state_name(to), ps1(part));
              fprintf(sisout, "intersection %s\n", ps1(intersection));
            }
          }

          /* now put the information back */
          for (e = 0; e < array_n(edges); e++) {
            edge = array_fetch(edge_t *, edges, e);
            /* check if associated with the forbidden transitions */
            if (is_forbidden(cur_trans_sets, edge_trans, edge)) {
              continue;
            }

            /* check if the "from" state is in this class */
            from = stg_edge_from_state(edge);
            assert(st_lookup(equiv_states->state_index, (char *)from,
                             (char **)&s));
            if (!is_in_set(class, s)) {
              continue;
            }

            /* copy the classes of the "to" states */
            to = stg_edge_to_state(edge);
            assert(
                st_lookup(equiv_states->state_index, (char *)to, (char **)&s));
            part = array_fetch(pset, allowed_partitions, s);
            if (!setp_equal(part, intersection)) {
              set_copy(part, intersection);
              changed = 1;
              result = 1;
              if (astg_debug_flag > 3) {
                fprintf(sisout, "%s updated part %s\n", stg_get_state_name(to),
                        ps1(part));
              }
            }
          }
        }
      }
    }
  } while (changed);

  st_foreach_item(in_to_edges, stgen, (char **)&input, (char **)&edges) {
    array_free(edges);
  }
  st_free_table(in_to_edges);

  return result;
}

static int *choose_class(stg, allowed_partitions, equiv_states,
                         orig_stg_names) graph_t *stg;
array_t *allowed_partitions;
equiv_t *equiv_states;
st_table *orig_stg_names;
{
  int *index_part;
  vertex_t *state;
  pset intersection, part;
  int s, s1, idx, cl, nstates;
  char *str;
  array_t *names;
  st_generator *stgen;

  intersection = set_new(equiv_states->classes->count);
  nstates = array_n(equiv_states->index_state);

  /* put all "forced" merges together */
  index_part = ALLOC(int, nstates);
  for (s = 0; s < nstates; s++) {
    index_part[s] = -1;
  }
  st_foreach_item(orig_stg_names, stgen, (char **)&str, (char **)&names) {
    if (astg_debug_flag > 3) {
      fprintf(sisout, "considering the group %s\n", str);
    }
    set_fill(intersection, equiv_states->classes->count);
    for (s = 0; s < array_n(names); s++) {
      state = bwd_get_state_by_name(stg, array_fetch(char *, names, s));
      assert(st_lookup(equiv_states->state_index, (char *)state, (char **)&s1));
      part = array_fetch(pset, allowed_partitions, s1);
      set_and(intersection, intersection, part);

      if (astg_debug_flag > 3) {
        fprintf(sisout, "%s part %s ", stg_get_state_name(state), ps1(part));
        fprintf(sisout, "intersection %s\n", ps1(intersection));
      }

      if (set_ord(intersection) == 0) {
        if (astg_debug_flag > 2) {
          fprintf(sisout, "aborting due to state %s\n",
                  stg_get_state_name(state));
        }
        set_free(intersection);
        FREE(index_part);
        return NIL(int);
      }
    }

    for (s = 0; s < array_n(names); s++) {
      state = bwd_get_state_by_name(stg, array_fetch(char *, names, s));
      if (astg_debug_flag > 2) {
        if (set_ord(intersection) > 1) {
          fprintf(sisout, "multiple choice for %s:", stg_get_state_name(state));
          for (cl = 0; cl < equiv_states->classes->count; cl++) {
            if (is_in_set(intersection, cl)) {
              fprintf(sisout, " %d", cl);
            }
          }
          fprintf(sisout, "\n");
        }
      }

      for (cl = 0; cl < equiv_states->classes->count; cl++) {
        if (is_in_set(intersection, cl)) {
          if (astg_debug_flag > 2) {
            fprintf(sisout, "inserting %s in partition %d\n",
                    stg_get_state_name(state), cl);
          }
          assert(st_lookup(equiv_states->state_index, (char *)state,
                           (char **)&idx));
          index_part[idx] = cl;
          break;
        }
      }
    }
  }

  set_free(intersection);
  return index_part;
}

/* Check if state1 is in this class. Otherwise move from state1 forward
 * in the STG stopping as soon as one selected sig_p is traversed.
 * If doing so no other class is met, then state1 belongs to this class.
 * Returns 1 if state 1 is assigned to this class, 0 otherwise.
 */
static int check_this_partition_recur(state1, cl, equiv_states, subset, visited,
                                      allowed_partitions, forward, rm,
                                      edge_trans,
                                      state_marking) vertex_t *state1;
int cl;
equiv_t *equiv_states;
astg_scode subset;
pset visited;
array_t *allowed_partitions;
int forward, rm;
st_table *edge_trans, *state_marking;
{
  int st1, st2, result;
  lsGen gen;
  edge_t *edge;
  vertex_t *state2;
  pset part1;

  assert(st_lookup(equiv_states->state_index, (char *)state1, (char **)&st1));
  /* check if we can belong to this partition at all */
  part1 = array_fetch(pset, allowed_partitions, st1);
  if (!rm && !is_in_set(part1, cl)) {
    return 0;
  }

  /* now proceed to check recursively the fanout of state1 */
  set_insert(visited, st1);
  result = 1;
  for (gen =
           lsStart(forward ? g_get_out_edges(state1) : g_get_in_edges(state1));
       lsNext(gen, (lsGeneric *)&edge, LS_NH) == LS_OK ||
       ((void)lsFinish(gen), 0);) {
    state2 = stg_edge_to_state(edge);
    if (state2 == state1) {
      continue;
    }

    assert(st_lookup(equiv_states->state_index, (char *)state2, (char **)&st2));
    if (is_in_set(visited, st2)) {
      continue;
    }

    if (!distinguishable(edge, subset, edge_trans, state_marking)) {
      /* if the two cannot be distinguished, then st1 is in this class
       * only if st2 is, so first check if st2 is in this class...
       */
      if (!check_this_partition_recur(state2, cl, equiv_states, subset, visited,
                                      allowed_partitions, forward, rm,
                                      edge_trans, state_marking) &&
          is_in_set(part1, cl)) {
        if (astg_debug_flag > 3) {
          fprintf(sisout, "Removing %d from %s due to %s\n", cl,
                  stg_get_state_name(state1), stg_get_state_name(state2));
        }
        set_remove(part1, cl);
        result = 0;
      }
    }
  }

  if (rm && is_in_set(part1, cl)) {
    if (astg_debug_flag > 3) {
      fprintf(sisout, "Removing %d from %s\n", cl, stg_get_state_name(state1));
    }
    set_remove(part1, cl);
  }
  return result;
}

static int check_this_partition(state1, cl, equiv_states, subset, visited,
                                allowed_partitions, rm, edge_trans,
                                state_marking) vertex_t *state1;
int cl;
equiv_t *equiv_states;
astg_scode subset;
pset visited;
array_t *allowed_partitions;
int rm;
st_table *edge_trans, *state_marking;
{
  int result;

  set_clear(visited, array_n(equiv_states->index_state));
  result = check_this_partition_recur(state1, cl, equiv_states, subset, visited,
                                      allowed_partitions, /* forward */ 1, rm,
                                      edge_trans, state_marking);
  if (rm || !result) {
    set_clear(visited, array_n(equiv_states->index_state));
    result |= check_this_partition_recur(
        state1, cl, equiv_states, subset, visited, allowed_partitions,
        /* forward */ 1, rm, edge_trans, state_marking);
  }

  return result;
}

/* Check if subset performs a valid partition of the states, and return it
 */
static int *check_partitions(stg, equiv_states, subset, visited,
                             allowed_partitions, trans_comb, edge_trans,
                             state_marking, orig_stg_names) graph_t *stg;
equiv_t *equiv_states;
astg_scode subset;
pset visited;
array_t *allowed_partitions, *trans_comb;
st_table *edge_trans, *state_marking, *orig_stg_names;
{
  vertex_t *state;
  pset part, class;
  int s, cl;

  /* transpose the "class" information into "allowed_partitions" */
  for (s = 0; s < array_n(equiv_states->index_state); s++) {
    part = array_fetch(pset, allowed_partitions, s);
    set_clear(part, equiv_states->classes->count);
    foreachi_set(equiv_states->classes, cl, class) {
      if (is_in_set(class, s)) {
        set_insert(part, cl);
      }
    }
  }

  do {
    /* now examine all states */
    for (s = 0; s < array_n(equiv_states->index_state); s++) {
      state = array_fetch(vertex_t *, equiv_states->index_state, s);
      foreachi_set(equiv_states->classes, cl, class) {
        set_clear(visited, array_n(equiv_states->index_state));
        if (!check_this_partition(state, cl, equiv_states, subset, visited,
                                  allowed_partitions, /* rm */ 0, edge_trans,
                                  state_marking)) {
          (void)check_this_partition(state, cl, equiv_states, subset, visited,
                                     allowed_partitions, /* rm */ 1, edge_trans,
                                     state_marking);
        }
      }
    }
  } while (
      closure(stg, allowed_partitions, equiv_states, trans_comb, edge_trans));

  /* and choose one class for each state */
  return choose_class(stg, allowed_partitions, equiv_states, orig_stg_names);
}

/* generate all subsets in decreasing cost order */
static astg_scode partition_states_recur(astg, stg, equiv_states, base_subset,
                                         visited, masks, allowed_partitions,
                                         index_sig, first_signal, first_mask,
                                         trans_comb, edge_trans, state_marking,
                                         orig_stg_names) astg_graph *astg;
graph_t *stg;
equiv_t *equiv_states;
astg_scode base_subset;
pset visited;
array_t *masks, *allowed_partitions, *index_sig;
int first_signal, first_mask;
array_t *trans_comb;
st_table *edge_trans, *state_marking, *orig_stg_names;
{
  astg_scode best_subset, subset, mask;
  int i, m;
  int *index_part;
  astg_signal *sig_p;

  best_subset = base_subset;
  for (m = first_mask; m < array_n(masks); m++) {
    mask = array_fetch(astg_scode, masks, m);
    for (i = first_signal; i < array_n(index_sig); i++) {
      sig_p = array_fetch(astg_signal *, index_sig, i);
      if (!(mask & bwd_astg_state_bit(sig_p))) {
        continue;
      }

      /* first try without this sig_p */
      subset = base_subset & ~bwd_astg_state_bit(sig_p);
      if (astg_debug_flag > 1) {
        fprintf(sisout, "trying signals:");
        print_subset(subset, astg);
      }

      index_part = check_partitions(stg, equiv_states, subset, visited,
                                    allowed_partitions, trans_comb, edge_trans,
                                    state_marking, orig_stg_names);

      if (index_part != NIL(int)) {
        FREE(index_part);

        /* at most this will return the current subset */
        subset = partition_states_recur(
            astg, stg, equiv_states, subset, visited, masks, allowed_partitions,
            index_sig, i + 1, m, trans_comb, edge_trans, state_marking,
            orig_stg_names);
        if (subset_cost(subset, astg) < subset_cost(best_subset, astg)) {
          best_subset = subset;
        }
      }

      /* then try with this sig_p */
      subset = partition_states_recur(
          astg, stg, equiv_states, base_subset, visited, masks,
          allowed_partitions, index_sig, i + 1, m, trans_comb, edge_trans,
          state_marking, orig_stg_names);

      if (subset_cost(subset, astg) < subset_cost(best_subset, astg)) {
        best_subset = subset;
      }

      return best_subset;
    }
    first_signal = 0;
  }
  return best_subset;
}

static void forced_sets_recur(equiv_states, allowed_partitions, state1, visited,
                              tmp, copy, edge_trans,
                              state_marking) equiv_t *equiv_states;
array_t *allowed_partitions;
vertex_t *state1;
pset visited, tmp;
int copy;
st_table *edge_trans, *state_marking;
{
  lsGen gen;
  edge_t *edge;
  vertex_t *state2;
  int s1, s2;
  pset part1;
  astg_scode new_enabled;

  assert(st_lookup(equiv_states->state_index, (char *)state1, (char **)&s1));
  set_insert(visited, s1);
  part1 = array_fetch(pset, allowed_partitions, s1);
  if (copy) {
    if (!setp_equal(part1, tmp)) {
      set_copy(part1, tmp);
      if (astg_debug_flag > 3) {
        fprintf(sisout, "%s updated part %s\n", stg_get_state_name(state1),
                ps1(part1));
      }
    }
  } else {
    set_and(tmp, tmp, part1);
  }

  foreach_state_outedge(state1, gen, edge) {
    state2 = stg_edge_to_state(edge);
    if (state1 == state2) {
      continue;
    }
    new_enabled = bwd_new_enabled_signals(edge, edge_trans, state_marking);
    if (new_enabled) {
      continue;
    }
    assert(st_lookup(equiv_states->state_index, (char *)state2, (char **)&s2));
    if (is_in_set(visited, s2)) {
      continue;
    }

    forced_sets_recur(equiv_states, allowed_partitions, state2, visited, tmp,
                      copy, edge_trans, state_marking);
  }

  foreach_state_inedge(state1, gen, edge) {
    state2 = stg_edge_from_state(edge);
    if (state1 == state2) {
      continue;
    }
    new_enabled = bwd_new_enabled_signals(edge, edge_trans, state_marking);
    if (new_enabled) {
      continue;
    }
    assert(st_lookup(equiv_states->state_index, (char *)state2, (char **)&s2));
    if (is_in_set(visited, s2)) {
      continue;
    }

    forced_sets_recur(equiv_states, allowed_partitions, state2, visited, tmp,
                      copy, edge_trans, state_marking);
  }
}

/* force in the same set of classes all states that are separated by some
 * input sig_p (may be very far from optimal)
 */
static void reduce_input_dependency(stg, equiv_states, allowed_partitions,
                                    index_sig, visited, edge_trans,
                                    state_marking) graph_t *stg;
equiv_t *equiv_states;
array_t *allowed_partitions, *index_sig;
pset visited;
st_table *edge_trans, *state_marking;
{
  lsGen gen;
  edge_t *edge;
  vertex_t *state1, *state2;
  int i, s1, s2, nclasses, changed, nstates;
  astg_scode new_enabled, in_mask;
  pset class, part1, part2, tmp, tmp1, tmp2;
  astg_signal *sig_p;

  in_mask = 0;
  for (i = 0; i < array_n(index_sig); i++) {
    sig_p = array_fetch(astg_signal *, index_sig, i);
    if (astg_signal_type(sig_p) == ASTG_INPUT_SIG) {
      in_mask |= bwd_astg_state_bit(sig_p);
    }
  }

  nclasses = equiv_states->classes->count;
  nstates = array_n(equiv_states->index_state);
  tmp = set_new(nclasses);
  tmp1 = set_new(nclasses);
  tmp2 = set_new(nclasses);

  for (s1 = 0; s1 < array_n(equiv_states->index_state); s1++) {
    part1 = array_fetch(pset, allowed_partitions, s1);
    set_clear(part1, nclasses);
    foreachi_set(equiv_states->classes, i, class) {
      if (is_in_set(class, s1)) {
        set_insert(part1, i);
      }
    }
  }

  do {
    changed = 0;
    stg_foreach_transition(stg, gen, edge) {
      state1 = stg_edge_from_state(edge);
      state2 = stg_edge_to_state(edge);
      if (state1 == state2) {
        continue;
      }
      new_enabled = bwd_new_enabled_signals(edge, edge_trans, state_marking);
      /* check if only input signals are enough to distinguish them */
      if (!(new_enabled & in_mask)) {
        continue;
      }

      set_fill(tmp1, nclasses);
      set_clear(visited, nstates);
      forced_sets_recur(equiv_states, allowed_partitions, state1, visited, tmp1,
                        /* copy */ 0, edge_trans, state_marking);

      set_fill(tmp2, nclasses);
      set_clear(visited, nstates);
      forced_sets_recur(equiv_states, allowed_partitions, state2, visited, tmp2,
                        /* copy */ 0, edge_trans, state_marking);

      /* make the two sets equal, if this does not rm the last class
       * from each
       */
      assert(
          st_lookup(equiv_states->state_index, (char *)state1, (char **)&s1));
      part1 = array_fetch(pset, allowed_partitions, s1);
      assert(
          st_lookup(equiv_states->state_index, (char *)state2, (char **)&s2));
      part2 = array_fetch(pset, allowed_partitions, s2);
      if ((setp_equal(part1, tmp1) && setp_equal(part2, tmp2) &&
           setp_equal(tmp1, tmp2)) ||
          !set_andp(tmp, tmp1, tmp2)) {
        continue;
      }

      changed = 1;
      set_clear(visited, nstates);
      forced_sets_recur(equiv_states, allowed_partitions, state1, visited, tmp,
                        /* copy */ 1, edge_trans, state_marking);
      set_clear(visited, nstates);
      forced_sets_recur(equiv_states, allowed_partitions, state2, visited, tmp,
                        /* copy */ 1, edge_trans, state_marking);
    }
  } while (changed);

  /* put back the information */
  for (s1 = 0; s1 < array_n(equiv_states->index_state); s1++) {
    part1 = array_fetch(pset, allowed_partitions, s1);
    foreachi_set(equiv_states->classes, i, class) {
      if (!is_in_set(part1, i)) {
        set_remove(class, s1);
      }
    }
  }

  set_free(tmp);
  set_free(tmp1);
  set_free(tmp2);
}

/* return 1 if trans1 and trans2 are in direct conflict, 0 otherwise */
static int is_conflict(trans1, trans2) astg_trans *trans1, *trans2;
{
  int confl;
  astg_generator tgen, pgen;
  astg_trans *trans;
  astg_place *place;

  if (trans1 == trans2) {
    return 0;
  }
  /* check if there is a free choice place in the common fanin of the two */
  confl = 0;
  astg_foreach_input_place(trans1, pgen, place) {
    if (confl) {
      continue;
    }
    astg_foreach_output_trans(place, tgen, trans) {
      if (confl) {
        continue;
      }
      if (trans == trans2) {
        confl = 1;
      }
    }
  }

  return confl;
}

/* If the from and to states of edge belong to p_from and p_to respectively,
 * then:
 * (1) insert the corresponding markings in from_markings and to_markings
 * (2) recur on all states reachable from from and to (using the merged super-
 *     states) that do not require to traverse any edge in conflict with edge
 */
static void find_marking_pairs_recur(stg, astg, orig_stg_names, state_index,
                                     edge_trans, partitions, from, p_from, p_to,
                                     to_visit, from_set, to_set,
                                     cur_trans_sets) graph_t *stg;
astg_graph *astg;
st_table *orig_stg_names, *state_index;
st_table *edge_trans, *partitions;
vertex_t *from;
int p_from, p_to;
pset to_visit, from_set, to_set;
st_table *cur_trans_sets;
{
  lsGen gen;
  edge_t *edge;
  vertex_t *to, *from1;
  int p, s_from, s_to;

  assert(st_lookup(state_index, (char *)from, (char **)&s_from));
  if (!is_in_set(to_visit, s_from)) {
    return;
  }
  set_remove(to_visit, s_from);

  foreach_state_outedge(from, gen, edge) {
    to = stg_edge_to_state(edge);
    if (to == from) {
      continue;
    }

    if (is_forbidden(cur_trans_sets, edge_trans, edge)) {
      continue;
    }

    assert(st_lookup(state_index, (char *)to, (char **)&s_to));
    /* insert the pair only if they belong to the proper partitions */
    assert(st_lookup(partitions, (char *)to, (char **)&p));
    if (p_to == p) {
      set_insert(from_set, s_from);
      set_insert(to_set, s_to);
    }

    if (!is_in_set(to_visit, s_to)) {
      continue;
    }

    find_marking_pairs_recur(stg, astg, orig_stg_names, state_index, edge_trans,
                             partitions, to, p_from, p_to, to_visit, from_set,
                             to_set, cur_trans_sets);
  }

  foreach_state_inedge(from, gen, edge) {
    from1 = stg_edge_from_state(edge);
    if (from1 == from) {
      continue;
    }

    if (is_forbidden(cur_trans_sets, edge_trans, edge)) {
      continue;
    }

    assert(st_lookup(state_index, (char *)from1, (char **)&s_from));
    if (!is_in_set(to_visit, s_from)) {
      continue;
    }

    find_marking_pairs_recur(stg, astg, orig_stg_names, state_index, edge_trans,
                             partitions, from1, p_from, p_to, to_visit,
                             from_set, to_set, cur_trans_sets);
  }
}

static void free_marking_pair(marking_pair) marking_pair_t marking_pair;
{
  FREE(marking_pair.from_state);
  FREE(marking_pair.to_state);
  array_free(marking_pair.from_markings);
  array_free(marking_pair.to_markings);
}

/* check for multiple exit points */
static void check_multiple_exit(astg, stg, from, partitions, merged_head,
                                orig_stg_names, edge_trans,
                                index_sig) astg_graph *astg;
graph_t *stg;
vertex_t *from;
st_table *partitions, *merged_head, *orig_stg_names, *edge_trans;
array_t *index_sig;
{
  lsGen gen1, gen2;
  vertex_t *from_head, *to1, *to2, *from2, *new_from;
  edge_t *edge1, *edge2;
  int i, p_to1, p_to2, p_from, p_from2, k1, k2;
  char *str;
  astg_trans *trans1, *trans2;
  array_t *names, *trans_arr1, *trans_arr2;

  /* check all pre-merged states together */
  assert(st_lookup(merged_head, (char *)from, (char **)&from_head));
  str = stg_get_state_name(from_head);
  /* avoid useless duplication of work... */
  if (strcmp(str, stg_get_state_name(from))) {
    return;
  }

  assert(st_lookup(orig_stg_names, (char *)str, (char **)&names));
  assert(st_lookup(partitions, (char *)from, (char **)&p_from));
  for (i = 0; i < array_n(names); i++) {
    str = array_fetch(char *, names, i);
    new_from = bwd_get_state_by_name(stg, str);
    foreach_state_outedge(new_from, gen1, edge1) {
      to1 = stg_edge_to_state(edge1);
      if (to1 == new_from) {
        continue;
      }
      assert(st_lookup(partitions, (char *)to1, (char **)&p_to1));
      if (p_from == p_to1) {
        continue;
      }

      foreach_state_outedge(new_from, gen2, edge2) {
        to2 = stg_edge_to_state(edge2);
        if (to2 == to1) {
          /* avoid duplicates by considering them "ordered" */
          lsFinish(gen2);
          break;
        }

        if (to2 == new_from) {
          continue;
        }
        assert(st_lookup(partitions, (char *)to2, (char **)&p_to2));
        if (p_from == p_to2) {
          continue;
        }

        assert(st_lookup(edge_trans, (char *)edge1, (char **)&trans_arr1));
        assert(st_lookup(edge_trans, (char *)edge2, (char **)&trans_arr2));

        for (k1 = 0; k1 < array_n(trans_arr1); k1++) {
          trans1 = array_fetch(astg_trans *, trans_arr1, k1);
          for (k2 = 0; k2 < array_n(trans_arr2); k2++) {
            trans2 = array_fetch(astg_trans *, trans_arr2, k2);
            if (!is_conflict(trans1, trans2)) {
              constrain("the STG may not be live (multiple exit point)", astg,
                        trans1, trans2);
            }
          }
        }
      }

      /* highly inefficient way to check p_from2 -> p_from -> p_to1 */
      foreach_state_inedge(new_from, gen2, edge2) {
        from2 = stg_edge_from_state(edge2);
        if (from2 == new_from) {
          continue;
        }
        assert(st_lookup(partitions, (char *)from2, (char **)&p_from2));
        if (p_from == p_from2) {
          continue;
        }

        assert(st_lookup(edge_trans, (char *)edge1, (char **)&trans_arr1));
        assert(st_lookup(edge_trans, (char *)edge2, (char **)&trans_arr2));

        for (k1 = 0; k1 < array_n(trans_arr1); k1++) {
          trans1 = array_fetch(astg_trans *, trans_arr1, k1);
          for (k2 = 0; k2 < array_n(trans_arr2); k2++) {
            trans2 = array_fetch(astg_trans *, trans_arr2, k2);
            if (!is_conflict(trans1, trans2)) {
              constrain("the STG may not be live (multiple exit point)", astg,
                        trans2, trans1);
            }
          }
        }
      }
    }
  }
}

/* Find all maximal sets of transitions that are concurrently enabled in
 * "state". For each such set generate a symbol table containing edges
 * enabled in "state" but not in the set.
 */
static void find_maximal_concurrent(astg, stg, state, marking, cur_fc,
                                    edge_trans, state_marking, trans_set,
                                    trans_sets) astg_graph *astg;
graph_t *stg;
vertex_t *state;
astg_marking *marking;
int cur_fc;
st_table *edge_trans, *state_marking, *trans_set;
array_t *trans_sets;
{
  int fc, all_in;
  astg_trans *trans, *trans1;
  astg_generator tgen, pgen, tgen1;
  astg_place *place;
  st_generator *stgen;

  fc = 0;
  astg_foreach_place(astg, pgen, place) {
    if (!astg_get_marked(marking, place)) {
      continue;
    }
    if (astg_out_degree(place) > 1) {
      fc++;
    }
    if (fc > cur_fc) {
      break;
    }
  }

  if (fc > cur_fc) {
    /* split on "place" */
    astg_foreach_output_trans(place, tgen, trans) {
      all_in = 1;
      astg_foreach_output_trans(place, tgen1, trans1) {
        if (trans1 == trans) {
          continue;
        }
        if (!st_is_member(trans_set, (char *)trans1)) {
          all_in = 0;
        }
      }
      if (all_in) {
        continue;
      }
      st_insert(trans_set, (char *)trans, NIL(char));
      find_maximal_concurrent(astg, stg, state, marking, fc, edge_trans,
                              state_marking, trans_set, trans_sets);
      st_delete(trans_set, (char **)&trans, NIL(char *));
    }
  } else {
    /* everything is concurrent: add a new element to the array */
    if (st_count(trans_set)) {
      if (astg_debug_flag > 1) {
        fprintf(sisout,
                "forbidden transitions for %s:", stg_get_state_name(state));
        st_foreach_item(trans_set, stgen, (char **)&trans, NIL(char *)) {
          fprintf(sisout, " %s", astg_trans_name(trans));
        }
        fprintf(sisout, "\n");
      }
      array_insert_last(st_table *, trans_sets, st_copy(trans_set));
    }
  }
}

static void create_combinations_recur(trans_comb, all_trans_sets,
                                      cur_trans_sets, i) array_t *trans_comb,
    *all_trans_sets;
st_table *cur_trans_sets;
int i;
{
  int j;
  array_t *trans_sets;
  st_table *trans_set;
  st_generator *stgen;
  astg_trans *trans;

  if (i >= array_n(all_trans_sets)) {
    array_insert_last(st_table *, trans_comb, st_copy(cur_trans_sets));
  } else {
    trans_sets = array_fetch(array_t *, all_trans_sets, i);
    for (j = 0; j < array_n(trans_sets); j++) {
      trans_set = array_fetch(st_table *, trans_sets, j);
      st_foreach_item(trans_set, stgen, (char **)&trans, NIL(char *)) {
        st_insert(cur_trans_sets, (char *)trans, NIL(char));
      }

      create_combinations_recur(trans_comb, all_trans_sets, cur_trans_sets,
                                i + 1);

      st_foreach_item(trans_set, stgen, (char **)&trans, NIL(char *)) {
        st_delete(cur_trans_sets, (char **)&trans, NIL(char *));
      }
    }
  }
}

static void find_to_visit(stg, to_visit, partitions, orig_stg_names,
                          state_index, p_from, p_to) graph_t *stg;
pset to_visit;
st_table *partitions, *orig_stg_names, *state_index;
int p_from, p_to;
{
  lsGen gen;
  edge_t *edge;
  vertex_t *from, *to, *head;
  int s, p, i, found;
  st_generator *stgen;
  char *str;
  array_t *names;

  st_foreach_item(orig_stg_names, stgen, (char **)&str, (char **)&names) {
    head = bwd_get_state_by_name(stg, str);
    assert(st_lookup(partitions, (char *)head, (char **)&p));
    if (p != p_from) {
      continue;
    }
    found = 0;
    for (i = 0; !found && i < array_n(names); i++) {
      str = array_fetch(char *, names, i);
      from = bwd_get_state_by_name(stg, str);
      foreach_state_outedge(from, gen, edge) {
        to = stg_edge_to_state(edge);
        if (to == from) {
          continue;
        }
        assert(st_lookup(partitions, (char *)to, (char **)&p));
        if (p == p_to) {
          found = 1;
          lsFinish(gen);
          break;
        }
      }
    }
    if (found) {
      for (i = 0; i < array_n(names); i++) {
        str = array_fetch(char *, names, i);
        from = bwd_get_state_by_name(stg, str);
        assert(st_lookup(state_index, (char *)from, (char **)&s));
        set_insert(to_visit, s);
      }
    }
  }
}

static int st_eq(st1, st2) st_table *st1, *st2;
{
  st_generator *stgen;
  char *key;

  if (st_count(st1) != st_count(st2)) {
    return 0;
  }
  st_foreach_item(st1, stgen, &key, NIL(char *)) {
    if (!st_is_member(st2, key)) {
      return 0;
    }
  }

  return 1;
}

/* return 1 if maring can be reached from any from_marking firing a single
 * transition
 */
static int has_neighbor(astg, marking, markings, forward) astg_graph *astg;
astg_marking *marking;
array_t *markings;
int forward;
{
  int i;
  astg_marking *dup, *cand;
  astg_generator tgen;
  astg_trans *trans;

  for (i = 0; i < array_n(markings); i++) {
    cand = array_fetch(astg_marking *, markings, i);
    astg_foreach_trans(astg, tgen, trans) {
      if (forward) {
        /* try marking [trans> cand */
        dup = astg_dup_marking(marking);
        if (!astg_disabled_count(trans, dup)) {
          (void)astg_fire(dup, trans);
          if (!astg_cmp_marking(dup, cand)) {
            astg_delete_marking(dup);
            return 1;
          }
        }
        astg_delete_marking(dup);
      } else {
        /* try cand [trans> marking */
        dup = astg_dup_marking(cand);
        if (!astg_disabled_count(trans, dup)) {
          (void)astg_fire(dup, trans);
          if (!astg_cmp_marking(dup, marking)) {
            astg_delete_marking(dup);
            return 1;
          }
        }
        astg_delete_marking(dup);
      }
    }
  }

  return 0;
}

/* Partition the set of states in stg according to the classes in equiv_states,
 * using signals as class separators.
 */
static st_table *partition_states(astg, stg, equiv_states, use_mincov, greedy,
                                  go_down, mincov_option, use_mdd, edge_trans,
                                  state_marking,
                                  orig_stg_names) astg_graph *astg;
graph_t *stg;
equiv_t *equiv_states;
int use_mincov, greedy, go_down, mincov_option, use_mdd;
st_table *edge_trans, *state_marking, *orig_stg_names;
{
  st_table *partitions, *trans_set, *cur_trans_sets, *old_trans_set;
  array_t *index_sig, *allowed_partitions, *masks, *names, *froms, *tos;
  array_t *trans_sets, *all_trans_sets, *trans_comb, *old_trans_sets;
  array_t *marking_arr, *from_markings, *to_markings;
  lsGen gen;
  vertex_t *state_head, *state, *from, *to;
  pset part, from_set, to_set, visited, to_visit, old_from, old_to;
  int s, i, j, m, in, out, cl, len, p_from, p_to, npart, found, k;
  int *index_part;
  char *str, buf[80];
  astg_scode base_subset, best_subset, subset, in_mask, out_mask, mask;
  astg_signal *sig_p;
  astg_generator sgen;
  st_generator *stgen;
  marking_pair_t marking_pair;
  st_table *merged_head;
  astg_marking *marking;
  astg_trans *trans;

  if (equiv_states->classes->count < 2) {
    /* too easy... */
    fprintf(siserr, "info: the STG has complete state coding (no state coding "
                    "is necessary)\n");

    partitions = st_init_table(st_ptrcmp, st_ptrhash);
    stg_foreach_state(stg, gen, state) {
      st_insert(partitions, (char *)state, (char *)0);
    }
    return partitions;
  }

  /* initialize variables */
  index_sig = array_alloc(astg_signal *, 0);
  astg_foreach_signal(astg, sgen, sig_p) {
    if (astg_signal_type(sig_p) == ASTG_DUMMY_SIG) {
      continue;
    }
    array_insert_last(astg_signal *, index_sig, sig_p);
  }
  visited = set_new(array_n(equiv_states->index_state));
  to_visit = set_new(array_n(equiv_states->index_state));
  allowed_partitions = array_alloc(pset, 0);
  for (i = 0; i < array_n(equiv_states->index_state); i++) {
    part = set_new(equiv_states->classes->count);
    array_insert_last(pset, allowed_partitions, part);
  }

  /* find the sets of mutually exclusive transitions, to avoid traversing
   * them together while computing closure etc.
   */
  all_trans_sets = array_alloc(array_t *, 0);
  st_foreach_item(orig_stg_names, stgen, (char **)&str, (char **)&names) {
    state = bwd_get_state_by_name(stg, str);
    trans_sets = array_alloc(st_table *, 0);
    trans_set = st_init_table(st_ptrcmp, st_ptrhash);

    assert(st_lookup(state_marking, (char *)state, (char **)&marking_arr));
    for (k = 0; k < array_n(marking_arr); k++) {
      marking = array_fetch(astg_marking *, marking_arr, k);
      find_maximal_concurrent(astg, stg, state, marking, 0, edge_trans,
                              state_marking, trans_set, trans_sets);
    }

    st_free_table(trans_set);
    if (array_n(trans_sets)) {
      found = 0;
      for (i = 0; !found && i < array_n(all_trans_sets); i++) {
        old_trans_sets = array_fetch(array_t *, all_trans_sets, i);
        if (array_n(old_trans_sets) != array_n(trans_sets)) {
          continue;
        }
        for (j = 0; !found && j < array_n(trans_sets); j++) {
          trans_set = array_fetch(st_table *, trans_sets, j);
          old_trans_set = array_fetch(st_table *, old_trans_sets, j);
          if (st_eq(trans_set, old_trans_set)) {
            found = 1;
          }
        }
      }

      if (found) {
        for (j = 0; !found && j < array_n(trans_sets); j++) {
          trans_set = array_fetch(st_table *, trans_sets, j);
          st_free_table(trans_set);
        }
        array_free(trans_sets);
      } else {
        array_insert_last(array_t *, all_trans_sets, trans_sets);
      }
    }
  }
  trans_comb = array_alloc(st_table *, 0);
  cur_trans_sets = st_init_table(st_ptrcmp, st_ptrhash);
  create_combinations_recur(trans_comb, all_trans_sets, cur_trans_sets, 0);
  st_free_table(cur_trans_sets);

  if (greedy > 1) {
    reduce_input_dependency(stg, equiv_states, allowed_partitions, index_sig,
                            visited, edge_trans, state_marking);
  }
  /* find the initial subset */
  if (use_mincov || use_mdd) {
    assert(go_down);
    base_subset = find_partitioning_signals(astg, stg, equiv_states, index_sig,
                                            visited, mincov_option, use_mdd,
                                            edge_trans, state_marking);
  } else {
    base_subset = 0;
    if (go_down) {
      for (i = 0; i < array_n(index_sig); i++) {
        sig_p = array_fetch(astg_signal *, index_sig, i);
        base_subset |= bwd_astg_state_bit(sig_p);
      }
    }
  }

  if (astg_debug_flag > 1) {
    fprintf(sisout, "initial signals:");
    print_subset(base_subset, astg);
  }

  if (use_mincov >= 2 || use_mdd >= 2) {
    index_part = check_partitions(stg, equiv_states, base_subset, visited,
                                  allowed_partitions, trans_comb, edge_trans,
                                  state_marking, orig_stg_names);
    best_subset = base_subset;
  } else {
    /* try to improve (or make valid altogether) the initial solution */
    out_mask = in_mask = 0;
    for (i = 0; i < array_n(index_sig); i++) {
      sig_p = array_fetch(astg_signal *, index_sig, i);
      if (go_down && !(bwd_astg_state_bit(sig_p) & base_subset)) {
        continue;
      }
      if (signal_cost(sig_p, astg) == 1) {
        in_mask |= bwd_astg_state_bit(sig_p);
      } else {
        out_mask |= bwd_astg_state_bit(sig_p);
      }
    }

    if (go_down) {
      masks = array_alloc(astg_scode, 0);
      array_insert_last(astg_scode, masks, out_mask);
      array_insert_last(astg_scode, masks, in_mask);
      if (greedy) {
        best_subset = base_subset;
        for (m = 0; m < array_n(masks); m++) {
          mask = array_fetch(astg_scode, masks, m);
          for (i = 0; i < array_n(index_sig); i++) {
            sig_p = array_fetch(astg_signal *, index_sig, i);
            if (!(mask & bwd_astg_state_bit(sig_p))) {
              continue;
            }

            subset = best_subset & ~bwd_astg_state_bit(sig_p);
            if (astg_debug_flag > 1) {
              fprintf(sisout, "trying signals:");
              print_subset(subset, astg);
            }

            index_part = check_partitions(
                stg, equiv_states, subset, visited, allowed_partitions,
                trans_comb, edge_trans, state_marking, orig_stg_names);

            if (index_part == NIL(int)) {
              continue;
            }

            best_subset = subset;
            FREE(index_part);
          }
        }
      } else {
        best_subset = partition_states_recur(
            astg, stg, equiv_states, base_subset, visited, masks,
            allowed_partitions, index_sig, 0, 0, trans_comb, edge_trans,
            state_marking, orig_stg_names);
      }
      if (astg_debug_flag > 1) {
        fprintf(sisout, "checking final signals:");
        print_subset(best_subset, astg);
      }
      array_free(masks);
      index_part = check_partitions(stg, equiv_states, best_subset, visited,
                                    allowed_partitions, trans_comb, edge_trans,
                                    state_marking, orig_stg_names);
    } else {
      /* generate all subsets in increasing cost order */
      index_part = NIL(int);
      for (in = 0; in <= count_ones(in_mask); in++) {
        for (out = 0; out <= count_ones(out_mask); out++) {
          for (subset = 1; subset <= (in_mask | out_mask); subset++) {
            if (count_ones((subset & in_mask)) != in ||
                count_ones((subset & out_mask)) != out) {
              continue;
            }

            if (astg_debug_flag > 1) {
              fprintf(sisout, "trying signals:");
              print_subset(subset, astg);
            }
            index_part = check_partitions(
                stg, equiv_states, subset, visited, allowed_partitions,
                trans_comb, edge_trans, state_marking, orig_stg_names);
            if (index_part != NIL(int)) {
              break;
            }
          }
          if (index_part != NIL(int)) {
            break;
          }
        }
        if (index_part != NIL(int)) {
          break;
        }
      }
      best_subset = subset;
    }
  }

  if (astg_debug_flag > 0 && index_part != NIL(int)) {
    fprintf(sisout, "selected signals:");
    print_subset(best_subset, astg);
  }

  if (index_part == NIL(int)) {
    fprintf(siserr, "internal error: cannot find a valid partition\n");
    return NIL(st_table);
  }
  partitions = st_init_table(st_ptrcmp, st_ptrhash);
  npart = 0;
  for (i = 0; i < array_n(equiv_states->index_state); i++) {
    state = array_fetch(vertex_t *, equiv_states->index_state, i);
    assert(index_part[i] >= 0);
    npart = MAX(npart, index_part[i] + 1);
    st_insert(partitions, (char *)state, (char *)index_part[i]);
  }

  /* create a table with key each state in the un-minimized graph
   * and data the corresponding pre-minimized state (invert the information
   * of the orig_stg_names table)
   */
  merged_head = st_init_table(st_ptrcmp, st_ptrhash);
  st_foreach_item(orig_stg_names, stgen, (char **)&str, (char **)&names) {
    state_head = bwd_get_state_by_name(stg, str);
    for (s = 0; s < array_n(names); s++) {
      state = bwd_get_state_by_name(stg, array_fetch(char *, names, s));
      st_insert(merged_head, (char *)state, (char *)state_head);
    }
  }

  stg_foreach_state(stg, gen, from) {
    check_multiple_exit(astg, stg, from, partitions, merged_head,
                        orig_stg_names, edge_trans, index_sig);
  }
  st_free_table(merged_head);

  /* store the marking pairs */
  if (marking_pairs != NIL(array_t)) {
    for (i = 0; i < array_n(marking_pairs); i++) {
      marking_pair = array_fetch(marking_pair_t, marking_pairs, i);
      free_marking_pair(marking_pair);
    }
    array_free(marking_pairs);
  }
  marking_pairs = array_alloc(marking_pair_t, 0);

  for (p_from = 0; p_from < npart; p_from++) {
    for (p_to = 0; p_to < npart; p_to++) {
      if (p_to == p_from) {
        continue;
      }

      froms = array_alloc(pset, 0);
      tos = array_alloc(pset, 0);

      for (j = 0; j < array_n(trans_comb); j++) {
        cur_trans_sets = array_fetch(st_table *, trans_comb, j);
        if (astg_debug_flag > 2 && st_count(cur_trans_sets)) {
          fprintf(sisout, "excluding transitions");
          st_foreach_item(cur_trans_sets, stgen, (char **)&trans, NIL(char *)) {
            fprintf(sisout, " %s", astg_trans_name(trans));
          }
          fprintf(sisout, "\n");
        }

        /* insert in to_visit all the states in p_from that are
         * adjacent (as super-states) to a state in p_to
         */
        set_clear(to_visit, array_n(equiv_states->index_state));
        find_to_visit(stg, to_visit, partitions, orig_stg_names,
                      equiv_states->state_index, p_from, p_to);

        stg_foreach_state(stg, gen, from) {
          from_set = set_new(array_n(equiv_states->index_state));
          to_set = set_new(array_n(equiv_states->index_state));

          find_marking_pairs_recur(stg, astg, orig_stg_names,
                                   equiv_states->state_index, edge_trans,
                                   partitions, from, p_from, p_to, to_visit,
                                   from_set, to_set, cur_trans_sets);

          for (i = 0; i < array_n(froms); i++) {
            old_from = array_fetch(pset, froms, i);
            old_to = array_fetch(pset, tos, i);
            if (setp_implies(old_from, from_set) &&
                setp_implies(old_to, to_set)) {
              /* new pair contains old pair */
              set_copy(old_from, from_set);
              set_copy(old_to, to_set);
              break;
            }

            if (setp_implies(from_set, old_from) &&
                setp_implies(to_set, old_to)) {
              /* old pair contains new pair */
              break;
            }
          }
          if (i >= array_n(froms)) {
            /* not found: add it */
            array_insert_last(pset, froms, from_set);
            array_insert_last(pset, tos, to_set);
          } else {
            set_free(from_set);
            set_free(to_set);
          }
        }
      }

      for (i = 0; i < array_n(froms); i++) {
        from_set = array_fetch(pset, froms, i);
        to_set = array_fetch(pset, tos, i);

        sprintf(buf, "s%d", p_from);
        marking_pair.from_state = util_strsav(buf);
        sprintf(buf, "s%d", p_to);
        marking_pair.to_state = util_strsav(buf);
        if (astg_debug_flag > 1) {
          fprintf(sisout, "creating marking pair from %s to %s\n from",
                  marking_pair.from_state, marking_pair.to_state);
        }

        /* Now, thanks to those dummy transitions, we must do all our
         * checks again... This may be LONG...
         */
        from_markings = array_alloc(astg_marking *, 0);
        for (s = 0; s < array_n(equiv_states->index_state); s++) {
          if (!is_in_set(from_set, s)) {
            continue;
          }
          from = array_fetch(vertex_t *, equiv_states->index_state, s);
          assert(st_lookup(state_marking, (char *)from, (char **)&marking_arr));
          for (k = 0; k < array_n(marking_arr); k++) {
            marking = array_fetch(astg_marking *, marking_arr, k);
            array_insert_last(astg_marking *, from_markings, marking);
          }
          if (astg_debug_flag > 1) {
            fprintf(sisout, " %s", stg_get_state_name(from));
          }
        }

        if (astg_debug_flag > 1) {
          fprintf(sisout, "\n to");
        }

        to_markings = array_alloc(astg_marking *, 0);
        for (s = 0; s < array_n(equiv_states->index_state); s++) {
          if (!is_in_set(to_set, s)) {
            continue;
          }
          to = array_fetch(vertex_t *, equiv_states->index_state, s);
          assert(st_lookup(state_marking, (char *)to, (char **)&marking_arr));
          for (k = 0; k < array_n(marking_arr); k++) {
            marking = array_fetch(astg_marking *, marking_arr, k);
            array_insert_last(astg_marking *, to_markings, marking);
          }
          if (astg_debug_flag > 1) {
            fprintf(sisout, " %s", stg_get_state_name(to));
          }
        }

        if (astg_debug_flag > 1) {
          fprintf(sisout, "\n");
        }

        marking_pair.from_markings = array_alloc(astg_marking *, 0);
        for (k = 0; k < array_n(from_markings); k++) {
          marking = array_fetch(astg_marking *, from_markings, k);
          if (!has_neighbor(astg, marking, to_markings,
                            /*forward*/ 1)) {
            continue;
          }
          array_insert_last(astg_marking *, marking_pair.from_markings,
                            marking);
        }
        marking_pair.to_markings = array_alloc(astg_marking *, 0);
        for (k = 0; k < array_n(to_markings); k++) {
          marking = array_fetch(astg_marking *, to_markings, k);
          if (!has_neighbor(astg, marking, from_markings,
                            /*forward*/ 0)) {
            continue;
          }
          array_insert_last(astg_marking *, marking_pair.to_markings, marking);
        }

        if (array_n(marking_pair.to_markings) > 0 &&
            array_n(marking_pair.from_markings) > 0) {
          array_insert_last(marking_pair_t, marking_pairs, marking_pair);
        } else {
          array_free(marking_pair.to_markings);
          array_free(marking_pair.from_markings);
        }
        array_free(to_markings);
        array_free(from_markings);
        set_free(from_set);
        set_free(to_set);
      }

      array_free(froms);
      array_free(tos);
    }
  }

  /* store the partitioning signals */
  if (partitioning_signals != NIL(array_t)) {
    array_free(partitioning_signals);
  }
  partitioning_signals = array_alloc(astg_signal *, 0);
  for (i = 0; i < array_n(index_sig); i++) {
    sig_p = array_fetch(astg_signal *, index_sig, i);
    if (!(best_subset & bwd_astg_state_bit(sig_p))) {
      continue;
    }
    array_insert_last(astg_signal *, partitioning_signals, sig_p);
  }

  for (j = 0; j < array_n(trans_comb); j++) {
    cur_trans_sets = array_fetch(st_table *, trans_comb, j);
    st_free_table(cur_trans_sets);
  }
  array_free(index_sig);
  FREE(index_part);
  for (i = 0; i < array_n(all_trans_sets); i++) {
    trans_sets = array_fetch(array_t *, all_trans_sets, i);
    for (j = 0; j < array_n(trans_sets); j++) {
      trans_set = array_fetch(st_table *, trans_sets, j);
      st_free_table(trans_set);
    }
    array_free(trans_sets);
  }
  array_free(all_trans_sets);
  array_free(trans_comb);

  if (astg_debug_flag > 1) {
    fprintf(sisout, "Reduced machine state partitions:\n");
    for (i = 0; i < equiv_states->classes->count; i++) {
      fprintf(sisout, "%2d ", i);
      len = 3;
      st_foreach_item(partitions, stgen, (char **)&state, (char **)&cl) {
        if (cl == i) {
          str = stg_get_state_name(state);
          len += strlen(str) + 1;
          if (len >= 80) {
            fprintf(sisout, "\n   ");
            len = 3 + strlen(str) + 1;
          }
          fprintf(sisout, "%s ", str);
        }
      }
      fprintf(sisout, "\n");
    }
  }

  set_free(visited);
  set_free(to_visit);
  for (i = 0; i < array_n(allowed_partitions); i++) {
    part = array_fetch(pset, allowed_partitions, i);
    set_free(part);
  }
  array_free(allowed_partitions);

  return partitions;
}

/* Minimize the flow table (represented as an STG) using the equivalence class
 * information.
 * First find a set of signals that partitions the equivalence classes. Then
 * for each class class1, decide what states belong to it, according to whether
 * or not the relevant transition was in the cover. Then build the minimized
 * flow table (for Tracey encoding).
 */
graph_t *bwd_state_minimize(astg, equiv_states, use_mincov, greedy, go_down,
                            mincov_option, use_mdd, sig_cost) astg_graph *astg;
equiv_t *equiv_states;
int use_mincov, greedy, go_down, mincov_option, use_mdd;
st_table *sig_cost;
{
  graph_t *red_stg;
  st_table *partitions;
  st_generator *stgen;
  array_t *names;
  char *str;

  if (global_orig_stg == NIL(graph_t)) {
    fprintf(siserr, "No state transition graph (use astg_to_stg)\n");
    return NIL(graph_t);
  }
  g_sig_cost = sig_cost;
  partitions =
      partition_states(astg, global_orig_stg, equiv_states, use_mincov, greedy,
                       go_down, mincov_option, use_mdd, global_edge_trans,
                       global_state_marking, global_orig_stg_names);
  if (partitions == NIL(st_table)) {
    /* something wrong occurred */
    g_sig_cost = NIL(st_table);
    return NIL(graph_t);
  }

  red_stg = bwd_reduced_stg(global_orig_stg, partitions, NIL(st_table),
                            NIL(array_t), equiv_states->classes->count);

  equiv_free(equiv_states);
  st_free_table(partitions);
  stg_free(global_orig_stg);
  global_orig_stg = NIL(graph_t);
  st_foreach_item(global_orig_stg_names, stgen, (char **)&str,
                  (char **)&names) {
    array_free(names);
  }
  st_free_table(global_orig_stg_names);

  g_sig_cost = NIL(st_table);
  return red_stg;
}

/* return (if any) a transition of sig_p between from_trans and place */
static astg_trans *find_transition(from_trans, from_to_place,
                                   sig_p) astg_trans *from_trans;
astg_place *from_to_place;
astg_signal *sig_p;
{
  astg_generator pgen, tgen, pgen1, tgen1, pgen2;
  astg_place *place, *place1, *place2;
  astg_trans *result, *trans1, *sig_trans;

  result = NIL(astg_trans);
  astg_foreach_output_place(from_trans, pgen, place) {
    if (result != NIL(astg_trans)) {
      continue;
    }
    astg_foreach_output_trans(place, tgen, sig_trans) {
      if (result != NIL(astg_trans)) {
        continue;
      }
      if (astg_trans_sig(sig_trans) != sig_p) {
        continue;
      }
      astg_foreach_output_place(sig_trans, pgen1, place1) {
        if (result != NIL(astg_trans)) {
          continue;
        }
        if (place1 == from_to_place) {
          result = sig_trans;
        }
        astg_foreach_output_trans(place1, tgen1, trans1) {
          if (result != NIL(astg_trans) ||
              astg_signal_type(astg_trans_sig(trans1)) != ASTG_DUMMY_SIG) {
            continue;
          }
          astg_foreach_output_place(trans1, pgen2, place2) {
            if (result != NIL(astg_trans)) {
              continue;
            }
            if (place2 == from_to_place) {
              result = sig_trans;
            }
          }
        }
      }
    }
  }
  return result;
}

static astg_place *add_and_return_constraint(from_trans,
                                             to_trans) astg_trans *from_trans,
    *to_trans;
{
  astg_generator tgen, pgen;
  astg_place *new_p, *place;
  astg_trans *trans;

  astg_add_constraint(from_trans, to_trans, ASTG_TRUE);
  new_p = NIL(astg_place);
  astg_foreach_output_place(from_trans, pgen, place) {
    if (new_p != NIL(astg_place)) {
      continue;
    }
    astg_foreach_output_trans(place, tgen, trans) {
      if (new_p != NIL(astg_place)) {
        continue;
      }
      if (trans == to_trans) {
        new_p = place;
      }
    }
  }
  assert(new_p != NIL(astg_trans));
  return new_p;
}

/* return 1 if from_trans is predecessor of all to_transitions
 */
static int is_predecessor(from_trans, to_transitions) astg_trans *from_trans;
st_table *to_transitions;
{
  int result, found;
  st_generator *stgen;
  astg_generator pgen, tgen;
  astg_trans *to_trans, *trans;
  astg_place *place;

  result = 1;
  st_foreach_item(to_transitions, stgen, (char **)&to_trans, NIL(char *)) {
    if (!result) {
      continue;
    }
    found = 0;
    astg_foreach_input_place(to_trans, pgen, place) {
      if (found) {
        continue;
      }
      astg_foreach_input_trans(place, tgen, trans) {
        if (found) {
          continue;
        }
        if (trans == from_trans) {
          found = 1;
        }
      }
    }
    if (!found) {
      result = 0;
    }
  }

  return result;
}

/* Split each place in new_places and insert a dummy transition dum1, a set
 * of state transitions and a dummy transition dum2 between them
 */
static void add_state_vars(astg, initial_marking, added, initial,
                           from_transitions, new_places, from_code, to_code,
                           nbits, idx, state_signals) astg_graph *astg;
astg_marking *initial_marking;
st_table *added, *initial, *from_transitions, *new_places;
char *from_code, *to_code;
int nbits, idx;
astg_signal **state_signals;
{
  int i;
  astg_place *new_p1, *new_p2, *from_to_place;
  astg_trans *new_t, *from_trans, *dum_t1, *dum_t2;
  st_generator *stgen, *stgen1;
  static char buf[50];

  /* create a pair of dummy transition, dum_t1 with fanout the new_places
   * dum_t2 with fanin copied from the set of transitions in
   * from_transitions
   */
  if (dum_sig == NIL(astg_signal)) {
    strcpy(buf, "dummy");
    while ((dum_sig = astg_find_signal(astg, buf, ASTG_DUMMY_SIG,
                                       ASTG_FALSE)) != NIL(astg_signal)) {
      strcat(buf, "_");
    }
    dum_sig = astg_find_signal(astg, buf, ASTG_DUMMY_SIG, ASTG_TRUE);
  }
  dum_t1 = astg_find_trans(astg, astg_signal_name(dum_sig), ASTG_DUMMY_X,
                           dum_i++, ASTG_TRUE);
  dum_t2 = astg_find_trans(astg, astg_signal_name(dum_sig), ASTG_DUMMY_X,
                           dum_i++, ASTG_TRUE);
  st_foreach_item(new_places, stgen, (char **)&from_to_place, NIL(char *)) {
    new_p1 = NIL(astg_place);
    st_foreach_item(from_transitions, stgen1, (char **)&from_trans,
                    NIL(char *)) {
      if (astg_find_edge(from_trans, from_to_place, ASTG_FALSE) ==
          NIL(astg_edge)) {
        continue;
      }

      /* move the fanout from from_trans to dum_t2 */
      if (astg_debug_flag > 0) {
        fprintf(sisout, "replacing %s %s with\n", astg_trans_name(from_trans),
                astg_place_name(from_to_place));
      }
      astg_delete_edge(astg_find_edge(from_trans, from_to_place, ASTG_FALSE));
      if (astg_find_edge(dum_t2, from_to_place, ASTG_FALSE) == NIL(astg_edge)) {
        astg_find_edge(dum_t2, from_to_place, ASTG_TRUE);
      }
      if (astg_debug_flag > 0) {
        fprintf(sisout, "             %s %s and\n", astg_trans_name(dum_t2),
                astg_place_name(from_to_place));
      }

      /* create a fanin from dum_t1 */
      if (new_p1 == NIL(astg_place)) {
        new_p1 = add_and_return_constraint(from_trans, dum_t1);
        st_insert(added, (char *)new_p1, NIL(char));
      } else {
        if (astg_find_edge(new_p1, dum_t2, ASTG_FALSE) == NIL(astg_edge)) {
          astg_find_edge(new_p1, dum_t2, ASTG_TRUE);
        }
      }
      if (astg_debug_flag > 0) {
        fprintf(sisout, "             %s %s %s\n", astg_trans_name(from_trans),
                astg_place_name(new_p1), astg_trans_name(dum_t1));
      }
    }

    /* if from_to_place was initially marked, then
     * new_p1 must be marked instead
     */
    if (!st_is_member(added, (char *)from_to_place)) {
      if (astg_get_marked(initial_marking, from_to_place)) {
        st_insert(initial, (char *)new_p1, NIL(char));
        st_delete(initial, (char **)&from_to_place, NIL(char *));
      }
    }
  }

  /* now add the state transitions between dum_t1 and dum_t2 */
  for (i = 0; i < nbits; i++) {
    if (from_code[i] == to_code[i]) {
      continue;
    }

    /* create the transition */
    new_t = astg_find_trans(astg, state_signals[i]->name,
                            TRANS_TYPE(from_code[i]), idx, ASTG_TRUE);

    new_p1 = add_and_return_constraint(dum_t1, new_t);
    st_insert(added, (char *)new_p1, NIL(char));
    new_p2 = add_and_return_constraint(new_t, dum_t2);
    st_insert(added, (char *)new_p2, NIL(char));
  }
}

void bwd_add_state(stg, astg, restore_marking) graph_t *stg;
astg_graph *astg;
int restore_marking;
{
  vertex_t *from_state, *to_state;
  int nbits, i, m, j, found, idx;
  astg_signal **state_signals, *sig_p;
  char *from_code, *to_code;
  astg_place *place;
  astg_trans *to_trans, *from_trans, *trans;
  st_table *new_places, *sig_index, *initial, *added, *from_transitions;
  astg_generator sgen, pgen, tgen;
  marking_pair_t marking_pair;
  st_generator *stgen;
  astg_marking *initial_marking, *from_marking, *to_marking, *marking;

  idx = 1;
  dum_sig = NIL(astg_signal);
  dum_i = 1;

  /* get the number of bits */
  from_state = stg_get_start(stg);
  from_code = stg_get_state_encoding(from_state);
  if (from_code == NIL(char)) {
    fprintf(siserr, "No state encoding: define it with astg_encode\n");
    return;
  }
  nbits = strlen(from_code);

  /* store the initially marked places */
  astg_reset_state_graph(astg);
  initial_marking = astg_initial_marking(astg);
  initial = st_init_table(st_ptrcmp, st_ptrhash);
  astg_foreach_place(astg, pgen, place) {
    place->type.place.initial_token = ASTG_FALSE;
    if (!astg_get_marked(initial_marking, place)) {
      continue;
    }
    st_insert(initial, (char *)place, NIL(char));
  }

  /* store the initial ASTG signals */
  sig_index = st_init_table(st_ptrcmp, st_ptrhash);
  i = 0;
  astg_foreach_signal(astg, sgen, sig_p) {
    st_insert(sig_index, (char *)sig_p, (char *)i);
    i++;
  }

  /* create the state variable signals */
  state_signals = ALLOC(astg_signal *, nbits);
  for (i = 0; i < nbits; i++) {
    state_signals[i] = astg_new_sig(astg, ASTG_OUTPUT_SIG);
  }

  added = st_init_table(st_ptrcmp, st_ptrhash);
  /* insert the transitions */
  for (m = 0; m < array_n(marking_pairs); m++) {
    marking_pair = array_fetch(marking_pair_t, marking_pairs, m);
    if (astg_debug_flag > 1) {
      fprintf(sisout, "marking pair from %s to %s\n", marking_pair.from_state,
              marking_pair.to_state);
      fprintf(sisout, "from");
      for (i = 0; i < array_n(marking_pair.from_markings); i++) {
        marking = array_fetch(astg_marking *, marking_pair.from_markings, i);
        fprintf(sisout, " s%s", bwd_marking_string(marking->marked_places));
      }
      fprintf(sisout, "\nto");
      for (i = 0; i < array_n(marking_pair.to_markings); i++) {
        marking = array_fetch(astg_marking *, marking_pair.to_markings, i);
        fprintf(sisout, " s%s", bwd_marking_string(marking->marked_places));
      }
      fprintf(sisout, "\n");
    }

    /* get the state codes */
    from_state = bwd_get_state_by_name(stg, marking_pair.from_state);
    assert(from_state != NIL(vertex_t));
    from_code = stg_get_state_encoding(from_state);

    to_state = bwd_get_state_by_name(stg, marking_pair.to_state);
    assert(to_state != NIL(vertex_t));
    to_code = stg_get_state_encoding(to_state);

    /* get the places marked in ALL to_marking and NOT in SOME
     * from_marking
     */
    new_places = st_init_table(st_ptrcmp, st_ptrhash);
    if (astg_debug_flag > 1) {
      fprintf(sisout, "new marked places:");
    }
    astg_foreach_place(astg, pgen, place) {
      if (st_is_member(added, (char *)place)) {
        continue;
      }
      /* check if place is marked in ALL to_marking */
      for (j = 0; j < array_n(marking_pair.to_markings); j++) {
        to_marking = array_fetch(astg_marking *, marking_pair.to_markings, j);
        if (!astg_get_marked(to_marking, place)) {
          break;
        }
      }
      if (j < array_n(marking_pair.to_markings)) {
        continue;
      }

      /* check if place is marked in ALL from_marking */
      for (j = 0; j < array_n(marking_pair.from_markings); j++) {
        from_marking =
            array_fetch(astg_marking *, marking_pair.from_markings, j);
        if (!astg_get_marked(from_marking, place)) {
          break;
        }
      }
      if (j >= array_n(marking_pair.from_markings)) {
        continue;
      }

      /* skip those places that are not predecessors of a transition
       * that is enabled in some to_marking
       */
      found = 0;
      astg_foreach_output_trans(place, tgen, to_trans) {
        /* skip added dummy signal... */
        if (astg_trans_sig(to_trans) == dum_sig) {
          continue;
        }

        /* check if trans is enabled in SOME to_marking */
        for (j = 0; j < array_n(marking_pair.to_markings); j++) {
          to_marking = array_fetch(astg_marking *, marking_pair.to_markings, j);
          if (!astg_disabled_count(to_trans, to_marking)) {
            break;
          }
        }
        if (j < array_n(marking_pair.to_markings)) {
          found = 1;
        }
      }
      if (!found) {
        continue;
      }

      st_insert(new_places, (char *)place, NIL(char));
      if (astg_debug_flag > 1) {
        fprintf(sisout, " %s", astg_place_name(place));
      }
    }
    if (astg_debug_flag > 1) {
      fprintf(sisout, "\n");
    }

    if (st_count(new_places) == 0) {
      fprintf(siserr, "internal error: the STG will not be live:\n");
      fprintf(siserr, "there is no place marked in all\n");
      for (j = 0; j < array_n(marking_pair.to_markings); j++) {
        to_marking = array_fetch(astg_marking *, marking_pair.to_markings, j);
        astg_foreach_place(astg, pgen, place) {
          if (astg_get_marked(to_marking, place)) {
            fprintf(siserr, " %s", astg_place_name(place));
          }
        }
        fprintf(siserr, "\n");
      }
      fprintf(siserr, "that is not marked in some\n");
      for (j = 0; j < array_n(marking_pair.from_markings); j++) {
        from_marking =
            array_fetch(astg_marking *, marking_pair.from_markings, j);
        astg_foreach_place(astg, pgen, place) {
          if (astg_get_marked(from_marking, place)) {
            fprintf(siserr, " %s", astg_place_name(place));
          }
        }
        fprintf(siserr, "\n");
      }

      continue;
    }

    /* now check what transitions enbled those places */
    from_transitions = st_init_table(st_ptrcmp, st_ptrhash);
    if (astg_debug_flag > 1) {
      fprintf(sisout, "from transitions:");
    }
    astg_foreach_trans(astg, tgen, from_trans) {
      /* skip added dummy signal... */
      if (astg_trans_sig(from_trans) == dum_sig) {
        continue;
      }

      /* check if trans is enabled in SOME from_marking */
      for (j = 0; j < array_n(marking_pair.from_markings); j++) {
        from_marking =
            array_fetch(astg_marking *, marking_pair.from_markings, j);
        if (!astg_disabled_count(from_trans, from_marking)) {
          break;
        }
      }
      if (j >= array_n(marking_pair.from_markings)) {
        continue;
      }
      /* check if its fanout is in new_places */
      found = 0;
      astg_foreach_output_place(from_trans, pgen, place) {
        if (found) {
          continue;
        }
        if (st_is_member(new_places, (char *)place)) {
          found = 1;
        }
      }
      if (!found) {
        continue;
      }

      st_insert(from_transitions, (char *)from_trans, NIL(char));
      if (astg_debug_flag > 1) {
        fprintf(sisout, " %s", astg_trans_name(from_trans));
      }
    }
    if (astg_debug_flag > 1) {
      fprintf(sisout, "\n");
    }
    if (st_count(from_transitions) == 0) {
      fprintf(siserr, "internal error: no from transitions\n");
    }

    add_state_vars(astg, initial_marking, added, initial, from_transitions,
                   new_places, from_code, to_code, nbits, idx, state_signals);

    idx++;
    st_free_table(new_places);
  }

  if (restore_marking) {
    st_foreach_item(initial, stgen, (char **)&place, NIL(char *)) {
      place->type.place.initial_token = ASTG_TRUE;
      if (astg_debug_flag > 0) {
        fprintf(sisout, "marking place");
        astg_foreach_input_trans(place, tgen, trans) {
          fprintf(sisout, " %s", astg_trans_name(trans));
        }
        fprintf(sisout, " ->");
        astg_foreach_output_trans(place, tgen, trans) {
          fprintf(sisout, " %s", astg_trans_name(trans));
        }
        fprintf(sisout, "\n");
      }
    }

    astg->has_marking = ASTG_TRUE;
  } else {
    astg->has_marking = ASTG_FALSE;
  }

  st_free_table(initial);
  st_free_table(sig_index);
  st_free_table(added);
  FREE(state_signals);
  idx = 0;
  dum_sig = NIL(astg_signal);
  dum_i = 0;
}
#endif /* SIS */
