
#ifdef SIS
#include "sis.h"

static char *convert_input_minterm_to_char_string();
static void free_bdd_array();
static void seqbdd_get_one_edge();
static bdd_t *seqbdd_get_one_minterm();
static void bdd_add_varids_to_table();
static void store_ancestor_state_info();

/*
 * For computing the initial state after retiming.
 *
 * 'network': sis network on which the retiming will be applied

 * 'n_shift': the number of clock ticks of simulation the initial state
 computation will do.
 * 'n_shift' is also the number of entries in 'input_seq' that this routine
 should compute.

 * 'input_order': hash table which maps external PI's from network to small
 integers,
 * from 0 to st_count(input_order)-1.

 * 'input_seq': an array of char strings of length 'n_inputs'. Each correspond
 to a vector of inputs.
 *  input = array_fetch(char *, input_seq, i) is interpreted as follows:
 *  entry input[j] is the value of the external PI mapped to j by 'input_order'
 at simulation time i.
 *  input[j] can take one of two possible small integer values: 0, 1.
 *  BEWARE: simulation time i: input_seq[0] corresponds to the last input
 transition
 *  input_seq[n_shift-1] corresponds to the transition leaving the ancestor
 state
 *
 * 'reset_ancestor': maps latches to their initial values. If a latch is
 missing.sh from the table,
 * it keeps its previous value. Values can be 0 or 1, as above.

 * returns 0 iff succeeds (1 if the initial state was not reachable from
 itself).
 */

int seqbdd_extract_input_sequence(network, n_shift, input_order, input_seq,
                                  reset_ancestor_table) network_t *network;
int n_shift;
st_table *input_order;
array_t *input_seq;
st_table *reset_ancestor_table;
{
  int i;
  int argc;
  lsGen gen;
  char **argv;
  node_t *node;
  output_info_t output_info;
  verif_options_t local_options, *options;
  range_data_t *range_data;
  int cycle_found;
  bdd_t *edges, *constrained_edges;
  bdd_t *ancestor_state;
  int ancestor_index;
  int cycle_length;
  int edge_index;
  array_t *total_sets;
  bdd_t *current_set, *new_current_set;
  bdd_t *var, *care_set;
  bdd_t *total_set, *new_total_set;
  bdd_t *current_state;
  int iter_count;
  array_t *cycle_states, *cycle_inputs;
  array_t *present_state_array, *true_pi_array;
  bdd_t *current_input;
  char *input;

  argc = 0;
  assert(!bdd_range_fill_options(&local_options, &argc, &argv));
  options = &(local_options);
  options->does_verification = 0;
  output_info_init(&output_info);
  options->output_info = &output_info;
  output_info.is_product_network = 0;
  extract_network_info(network, options);
  if (output_info.init_node == NIL(node_t))
    return 0;
  range_data = (*options->alloc_range_data)(network, options);

  present_state_array = array_alloc(bdd_t *, 0);
  true_pi_array = array_alloc(bdd_t *, 0);
  foreach_primary_input(network, gen, node) {
    var = ntbdd_at_node(node);
    if (network_latch_end(node) != NIL(node_t)) {
      array_insert_last(bdd_t *, present_state_array, var);
    } else {
      array_insert_last(bdd_t *, true_pi_array, var);
    }
  }

  total_sets = array_alloc(bdd_t *, 0);
  current_set = bdd_dup(range_data->init_state_fn);
  total_set = bdd_dup(current_set);
  iter_count = 0;
  cycle_found = 0;
  for (;;) {
    array_insert(bdd_t *, total_sets, iter_count, bdd_dup(total_set));
    iter_count++;
    new_current_set =
        (*options->compute_next_states)(current_set, range_data, options);
    bdd_free(current_set);
    if (bdd_leq(range_data->init_state_fn, new_current_set, 1, 1)) {
      cycle_found = 1;
      break;
    }
    if (bdd_leq(new_current_set, total_set, 1, 1)) {
      free_bdd_array(total_sets);
      (*options->free_range_data)(range_data, options);
      return 1;
    }
    care_set = bdd_not(total_set);
    current_set = bdd_cofactor(new_current_set, care_set);
    bdd_free(care_set);
    new_total_set = bdd_or(new_current_set, total_set, 1, 1);
    bdd_free(new_current_set);
    bdd_free(total_set);
    total_set = new_total_set;
  }

  /* need to find the input sequence now */
  assert(cycle_found);
  current_state = bdd_dup(range_data->init_state_fn);
  cycle_states = array_alloc(bdd_t *, iter_count);
  cycle_inputs = array_alloc(bdd_t *, iter_count);
  do {
    iter_count--;
    edges =
        (*options->compute_reverse_image)(current_state, range_data, options);
    bdd_free(current_state);
    total_set = array_fetch(bdd_t *, total_sets, iter_count);
    constrained_edges = bdd_and(edges, total_set, 1, 1);
    bdd_free(edges);
    seqbdd_get_one_edge(constrained_edges, range_data, present_state_array,
                        true_pi_array, &current_state, &current_input);
    bdd_free(constrained_edges);
    array_insert(bdd_t *, cycle_states, iter_count, bdd_dup(current_state));
    array_insert(bdd_t *, cycle_inputs, iter_count, bdd_dup(current_input));
    bdd_free(current_input);
  } while (iter_count > 0);
  bdd_free(current_state);
  free_bdd_array(total_sets);
  array_free(true_pi_array);
  array_free(present_state_array);

  /* consistency check */
  current_state = array_fetch(bdd_t *, cycle_states, 0);
  assert(bdd_equal(current_state, range_data->init_state_fn));

  /* generate the ancestor state and input sequence */
  cycle_length = array_n(cycle_states);
  if (n_shift == 0 || cycle_length == 1) {
    ancestor_index = 0;
  } else {
    ancestor_index = (-n_shift) % cycle_length;
    if (ancestor_index < 0) {
      ancestor_index += cycle_length;
    }
  }
  ancestor_state = array_fetch(bdd_t *, cycle_states, ancestor_index);
  store_ancestor_state_info(network, range_data, reset_ancestor_table,
                            ancestor_state);
  free_bdd_array(cycle_states);

  edge_index = ancestor_index;
  for (i = n_shift - 1; i >= 0; i--, edge_index++) {
    current_input =
        array_fetch(bdd_t *, cycle_inputs, (edge_index % cycle_length));
    input = convert_input_minterm_to_char_string(network, range_data,
                                                 input_order, current_input);
    array_insert(char *, input_seq, i, input);
  }
  free_bdd_array(cycle_inputs);
  (*options->free_range_data)(range_data, options);
  output_info_free(&output_info);
  return 0;
}

/* INTERNAL INTERFACE */

static void store_ancestor_state_info(network, seq_info, reset_ancestor_table,
                                      ancestor_state) network_t *network;
range_data_t *seq_info;
st_table *reset_ancestor_table;
bdd_t *ancestor_state;
{
  lsGen gen;
  node_t *pi;
  latch_t *latch;
  bdd_t *var;
  int value;

  foreach_primary_input(network, gen, pi) {
    if (network_is_real_pi(network, pi))
      continue;
    assert((latch = latch_from_node(pi)) != NIL(latch_t));
    var = ntbdd_at_node(pi);
    value = (bdd_leq(ancestor_state, var, 1, 1)) ? 1 : 0;
    st_insert(reset_ancestor_table, (char *)latch, (char *)value);
  }
}

static char *
    convert_input_minterm_to_char_string(network, seq_info, input_order,
                                         input_minterm) network_t *network;
range_data_t *seq_info;
st_table *input_order;
bdd_t *input_minterm;
{
  st_generator *stgen;
  node_t *pi;
  bdd_t *var, *bdd_var;
  int value;
  char *input;
  int index;

  input = ALLOC(char, st_count(input_order));

  st_foreach_item_int(input_order, stgen, (char **)&pi, &index) {
    var = ntbdd_at_node(pi);
    value = (bdd_leq(input_minterm, var, 1, 1)) ? 1 : 0;
    input[index] = value;
  }
  return input;
}

static void free_bdd_array(bdd_array) array_t *bdd_array;
{
  int i;
  bdd_t *fn;

  for (i = 0; i < array_n(bdd_array); i++) {
    fn = array_fetch(bdd_t *, bdd_array, i);
    bdd_free(fn);
  }
  array_free(bdd_array);
}

static void seqbdd_get_one_edge(from, seq_info, ps_array, pi_array, state,
                                input) bdd_t *from;
range_data_t *seq_info;
array_t *ps_array, *pi_array;
bdd_t **state;
bdd_t **input;
{
  bdd_t *minterm;
  st_table *var_table;

  assert(!bdd_is_tautology(from, 0));

  /* precompute the vars to be used */
  var_table = st_init_table(st_numcmp, st_numhash);
  bdd_add_varids_to_table(ps_array, var_table);
  bdd_add_varids_to_table(pi_array, var_table);

  minterm = seqbdd_get_one_minterm(seq_info->manager, from, var_table);

  st_free_table(var_table);

  if (state != NIL(bdd_t *))
    *state = bdd_smooth(minterm, pi_array);
  if (input != NIL(bdd_t *))
    *input = bdd_smooth(minterm, ps_array);
}

/*
 * returns a minterm from fn
 * Function should only depend on variables contain in 'vars'.
 * 'vars' is a table of variable_id, not of bdd_t *'s.
 * WARNING: Relies on the fact that the ith bdd_literal in cube
 * corresponds to the variable of variable_id == i.
 */

static bdd_t *seqbdd_get_one_minterm(manager, fn, vars) bdd_manager *manager;
bdd_t *fn;
st_table *vars;
{
  int i;
  bdd_t *tmp;
  bdd_t *bdd_lit;
  bdd_t *minterm;
  bdd_gen *gen;
  int n_lits;
  bdd_literal *lits;
  array_t *cube;

  assert(!bdd_is_tautology(fn, 0));

  /* need to free the bdd_gen immediately, otherwise problems with the BDD GC */
  minterm = bdd_one(manager);
  gen = bdd_first_cube(fn, &cube);
  n_lits = array_n(cube);
  lits = ALLOC(bdd_literal, n_lits);
  for (i = 0; i < n_lits; i++) {
    lits[i] = array_fetch(bdd_literal, cube, i);
  }
  (void)bdd_gen_free(gen);

  for (i = 0; i < n_lits; i++) {
    if (!st_lookup(vars, (char *)i, NIL(char *))) {
      assert(lits[i] == 2);
      continue;
    }
    switch (lits[i]) {
    case 0:
    case 2:
      tmp = bdd_get_variable(manager, i);
      bdd_lit = bdd_not(tmp);
      bdd_free(tmp);
      break;
    case 1:
      bdd_lit = bdd_get_variable(manager, i);
      break;
    default:
      fail("unexpected literal in get_one_minterm");
      break;
    }
    tmp = bdd_and(minterm, bdd_lit, 1, 1);
    bdd_free(minterm);
    bdd_free(bdd_lit);
    minterm = tmp;
  }
  return minterm;
}

static void bdd_add_varids_to_table(vars, table) array_t *vars;
st_table *table;
{
  int i;
  int varid;
  array_t *id_array;

  id_array = bdd_get_varids(vars);
  for (i = 0; i < array_n(id_array); i++) {
    varid = array_fetch(int, id_array, i);
    st_insert(table, (char *)varid, NIL(char));
  }
  array_free(id_array);
}
#endif /* SIS */
