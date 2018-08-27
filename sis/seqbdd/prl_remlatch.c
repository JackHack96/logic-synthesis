

#ifdef SIS
#include "prl_util.h"
#include "sis.h"

static void PerformLocalRetiming(network_t *, int);
static void FirstFitLatchRemoval(network_t *, seq_info_t *, bdd_t *,
                                 prl_options_t *);
static void RemoveOneBootLatch(network_t *, prl_options_t *);
static bdd_t *GetReachableStatesFromDc(seq_info_t *, prl_options_t *);

/*
 *----------------------------------------------------------------------
 *
 * Prl_RemoveLatches -- EXPORTED ROUTINE
 *
 * 1. Remove latches that are functionally deducible from the others.
 *    Latches whose equivalent combinational logic has too many inputs
 *    (exceeding some threshold specified as a command line parameter)
 *    are not removed.
 * 2. In addition, performs some local retiming by moving latches
 *    forward if that reduces the total latch count.
 * 3. Finally, tries to remove boot latches (i.e. latches fed by a constant
 *    but initialized by a different constant) by looking for a state
 *    equivalent to the initial state in which the initial value of the latch
 *    is equal to the value of its constant input.
 *
 * Results:
 *	None.
 *
 * Side effects:
 * 	The network is modified in place.
 * 	To avoid problems with don't care networks (may depend on PI variables
 * 	that do not exist any more after latch removal), we destroy the
 *dc_network if there is one.
 *
 *----------------------------------------------------------------------
 */

void Prl_RemoveLatches(network, options) network_t *network;
prl_options_t *options;
{
  seq_info_t *seq_info;
  bdd_t *reachable_states;
  network_t *new_network;
  int n_latches, new_n_latches;

  /* remove logically redundant latches */
  new_network = network_dup(network);
  seq_info = Prl_SeqInitNetwork(new_network, options);
  reachable_states = GetReachableStatesFromDc(seq_info, options);
  if (network->dc_network != NIL(network_t) &&
      bdd_is_tautology(reachable_states, 1)) {
    (void)fprintf(siserr,
                  "no reachability information found in don't care network\n");
  }
  FirstFitLatchRemoval(network, seq_info, reachable_states, options);
  bdd_free(reachable_states);
  Prl_SeqInfoFree(seq_info, options);
  network_free(new_network);

  /* remove the DC network */
  Prl_RemoveDcNetwork(network);

  /* remove the boot latches */
  if (options->remlatch.remove_boot) {
    do {
      n_latches = network_num_latch(network);
      RemoveOneBootLatch(network, options);
      network_sweep(network);
      new_n_latches = network_num_latch(network);
    } while (new_n_latches < n_latches);
  }

  /* push latches forward across logic if it decreases latch count */
  if (options->remlatch.local_retiming) {
    PerformLocalRetiming(network, options->verbose);
  }
}

/*
 *----------------------------------------------------------------------
 *
 * GetReachableStatesFromDc -- INTERNAL ROUTINE
 *
 * Results:
 *	Computes the bdd_and of all external don't cares.
 *      Complements it.
 *	Existentially quantify the external inputs.
 *
 * Side Effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static bdd_t *GetReachableStatesFromDc(seq_info, options) seq_info_t *seq_info;
prl_options_t *options;
{
  bdd_t *reachable_states;
  bdd_t *tmp = Prl_GetSimpleDc(seq_info);
  bdd_t *all_inputs = bdd_not(tmp);
  bdd_free(tmp);
  reachable_states = bdd_smooth(all_inputs, seq_info->external_input_vars);
  bdd_free(all_inputs);
  return reachable_states;
}

/* INTERNAL INTERFACE */

/* Structure used by 'latch_removal'. */

typedef struct {
  bdd_t *var;         /* the PI (latch output) identifying a latch */
  bdd_t *recoding_fn; /* the combinational logic fn that can be substituted to
                         the latch */
  bdd_t *
      recoding_fn_dc; /* the don't care of that function (unreachable states) */
  bdd_t *new_set;   /* the set of reachable states after this var is removed (if
                       red.) */
  int support_size; /* the size of the support of the recoding_fn */
  int output_distance; /* the depth of the output of 'recoding_fn' */
} var_info_t;

/* It seems that only 'old_vars' is ever used */

typedef struct {
  bdd_manager *manager;
  array_t *old_vars;     /* of bdd_t*; present state variables */
  array_t *new_vars;     /* of bdd_t*; as many as 'old_vars'; fresh variables */
  array_t *support_vars; /* of bdd_t*; as many as 'old_vars'; fresh variables */
  st_table *new_to_support_map; /* int-->int; maps new_vars->id -->
                                   support_vars->id */
} extra_vars_t;

static extra_vars_t *extra_vars_alloc();
static int get_output_distance();
static int get_output_distance_rec();
static int save_enough_latches();
static st_table *compute_from_var_to_pi_table();
static void extra_vars_free();
static void extract_var_info();
static void get_good_support_fn();
static void get_minimum_support_fn();
static void move_latches_forward();
static void red_latch_remove_latches();
static void RemoveOneLatch();
static void RemoveRedundantLatches();
static void remove_unused_pis();
static void replace_latch_by_node();

/* Where the real work is done.
 * We do the modifications directly on 'network'.
 * WARNING: seq_info->network is a COPY of 'network', not the same network.
 */

static void FirstFitLatchRemoval(network, seq_info, valid_states,
                                 options) network_t *network;
seq_info_t *seq_info;
bdd_t *valid_states;
prl_options_t *options;
{
  int i;
  bdd_t *set;
  bdd_t *var;
  node_t *pi;
  array_t *var_array;
  var_info_t var_info;
  array_t *state_vars;
  extra_vars_t *extra_vars;
  array_t *input_names;
  st_table *from_var_to_pi;

  input_names = (*options->extract_network_input_names)(seq_info, options);
  from_var_to_pi = compute_from_var_to_pi_table(network, seq_info, input_names);

  extra_vars = extra_vars_alloc(seq_info);
  state_vars = seq_info->present_state_vars;
  var_array = array_alloc(var_info_t, 0);
  set = bdd_dup(valid_states);
  for (i = 0; i < array_n(state_vars); i++) {
    var = array_fetch(bdd_t *, state_vars, i);
    extract_var_info(extra_vars, var, set, &var_info);
    if (var_info.var == NIL(bdd_t))
      continue;
    if (var_info.support_size > options->remlatch.max_cost) {
      if (options->verbose > 0) {
        (void)fprintf(siserr, "latch support %d too large\n",
                      var_info.support_size);
      }
      bdd_free(var_info.new_set);
      bdd_free(var_info.recoding_fn);
      bdd_free(var_info.recoding_fn_dc);
      continue;
    }
    if (options->verbose > 0) {
      assert(st_lookup(from_var_to_pi, (char *)var_info.var, (char **)&pi));
      (void)fprintf(siserr, "remove latch [%s] of support %d\n", io_name(pi, 0),
                    var_info.support_size);
    }
    array_insert_last(var_info_t, var_array, var_info);
    bdd_free(set);
    set = bdd_dup(var_info.new_set);
  }
  extra_vars_free(extra_vars);
  st_free_table(from_var_to_pi);
  array_free(input_names);

  RemoveRedundantLatches(network, seq_info, var_array, options);

  for (i = 0; i < array_n(var_array); i++) {
    var_info = array_fetch(var_info_t, var_array, i);
    bdd_free(var_info.new_set);
    bdd_free(var_info.recoding_fn);
    bdd_free(var_info.recoding_fn_dc);
  }
  array_free(var_array);
}

/*
 * a variable can be removed iff all the minterms in 'set'
 * can still be distinguished in the variable is removed.
 * This is equivalent to saying that the universal quantifier
 * applied to 'set' on the candidate variable yields the empty set.
 * For all such variables, we compute the size of its minimum support.
 * We remove the variables of smaller support first.
 */

static void extract_var_info(extra_vars, var, set,
                             var_info) extra_vars_t *extra_vars;
bdd_t *var;
bdd_t *set;
var_info_t *var_info;
{
  int var_is_redundant;
  bdd_t *independence_check;
  bdd_t *fn, *dc;
  bdd_t *new_fn, *new_dc;
  int support_size;
  array_t *var_array;

  var_array = array_alloc(bdd_t *, 0);
  array_insert_last(bdd_t *, var_array, var);
  independence_check = bdd_consensus(set, var_array);
  var_is_redundant = bdd_is_tautology(independence_check, 0);
  bdd_free(independence_check);
  if (var_is_redundant) {
    var_info->var = var;
    var_info->new_set = bdd_smooth(set, var_array);
    fn = bdd_cofactor(set, var);
    dc = bdd_not(var_info->new_set);
    get_good_support_fn(extra_vars, fn, dc, &support_size, &new_fn, &new_dc);
    var_info->recoding_fn = new_fn;
    var_info->recoding_fn_dc = new_dc;
    var_info->support_size = support_size;
    bdd_free(fn);
    bdd_free(dc);
  } else {
    var_info->support_size = INFINITY;
    var_info->var = NIL(bdd_t);
  }
  array_free(var_array);
}

/*
 *----------------------------------------------------------------------
 *
 * RemoveRedundantLatches -- INTERNAL ROUTINE
 *
 * Since 'network' is updated possibly several times, to avoid
 * dangling pointers from 'seq_info', 'seq_info' is constructed
 * from a copy of 'network'.
 *
 * 'input_names' is an array_t * of char*; one per PI of 'network'.
 * The index in 'input_names' correspond to the BDD varid of the PI.
 * If the product method is used, some entries of 'input_names' are set to NIL.
 * Those entries correspond to the extra PI corresponding to the next state POs.
 *
 * 'from_var_to_pi' is a st_table mapping bdd_t * vars (corresponding to PIs of
 *'network') onto the corresponding PIs in 'network'.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	'network' is modified. The latches in 'removed_vars' are replaced
 *	by logically equivalent combinational logic.
 *
 *----------------------------------------------------------------------
 */

static void RemoveRedundantLatches(network, seq_info, removed_vars,
                                   options) network_t *network;
seq_info_t *seq_info;
array_t *removed_vars;
prl_options_t *options;
{
  int i;
  node_t *pi;
  var_info_t var_info;
  array_t *input_names;
  st_table *from_var_to_pi;

  input_names = (*options->extract_network_input_names)(seq_info, options);
  from_var_to_pi = compute_from_var_to_pi_table(network, seq_info, input_names);

  for (i = 0; i < array_n(removed_vars); i++) {
    var_info = array_fetch(var_info_t, removed_vars, i);
    var_info.output_distance =
        get_output_distance(from_var_to_pi, var_info.var);
    if (var_info.output_distance + 1 > options->remlatch.max_level) {
      assert(st_lookup(from_var_to_pi, (char *)var_info.var, (char **)&pi));
      if (options->verbose > 0) {
        (void)fprintf(siserr, "latch [%s,%d] too far: kept\n", io_name(pi, 0),
                      var_info.output_distance);
      }
      continue;
    }
    RemoveOneLatch(network, input_names, from_var_to_pi, &var_info);
  }
  st_free_table(from_var_to_pi);
  array_free(input_names);
}

/*
 *----------------------------------------------------------------------
 *
 * compute_from_var_to_pi_table -- INTERNAL ROUTINE
 *
 * Maps var's under the form of bdd_t *'s to PI's in 'network'.
 *
 * NOTE: 'seq_info->network' != 'network' but the former is a copy of the
 *latter.
 *
 *----------------------------------------------------------------------
 */

static st_table *compute_from_var_to_pi_table(network, seq_info,
                                              input_names) network_t *network;
seq_info_t *seq_info;
array_t *input_names;
{
  int i, var_id;
  bdd_t *var;
  node_t *pi;
  char *input_name;
  st_table *var_to_pi_table;

  var_to_pi_table = st_init_table(st_numcmp, st_numhash);
  for (i = 0; i < array_n(seq_info->present_state_vars); i++) {
    var = array_fetch(bdd_t *, seq_info->present_state_vars, i);
    var_id = (int)bdd_top_var_id(var);
    input_name = array_fetch(char *, input_names, var_id);
    if (input_name == NIL(char))
      continue;
    assert((pi = network_find_node(network, input_name)) != NIL(node_t));
    st_insert(var_to_pi_table, (char *)var, (char *)pi);
  }
  return var_to_pi_table;
}

/*
 *----------------------------------------------------------------------
 *
 * RemoveOneLatch -- INTERNAL ROUTINE
 *
 * Replaces one latch by a piece of combinational logic.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	'network' is modified. The latch described in 'var_info' is replaced
 *	by logically equivalent combinational logic.
 *
 *----------------------------------------------------------------------
 */

static void RemoveOneLatch(network, input_names, from_var_to_pi,
                           var_info) network_t *network;
array_t *input_names;
st_table *from_var_to_pi;
var_info_t *var_info;
{
  node_t *output, *recoding_output, *pi;
  network_t *recoding_network;
  network_t *dc_recoding_network;
  latch_t *latch;

  /* get latch corresponding to var */
  assert(st_lookup(from_var_to_pi, (char *)var_info->var, (char **)&pi));
  assert((latch = latch_from_node(pi)) != NIL(latch_t));

  /* build a network with a single output for the recoding function */
  recoding_network = ntbdd_bdd_single_to_network(var_info->recoding_fn,
                                                 "dummy_name!", input_names);
  dc_recoding_network = ntbdd_bdd_single_to_network(var_info->recoding_fn_dc,
                                                    "dummy_name!", input_names);
  recoding_network->dc_network = dc_recoding_network;
  com_execute(&recoding_network, "collapse; full_simplify");
  remove_unused_pis(recoding_network);

  /* copy the single output function in 'network' */
  assert(network_num_po(recoding_network) == 1);
  recoding_output = network_get_po(recoding_network, 0);
  recoding_output = node_get_fanin(recoding_output, 0);
  Prl_SetupCopyFields(network, recoding_network);
  output = Prl_CopySubnetwork(network, recoding_output);
  network_free(recoding_network);

  /* remove the latch and move its fanouts to node 'output' */
  replace_latch_by_node(network, latch, output);
}

static void remove_unused_pis(network) network_t *network;
{
  int i;
  lsGen gen;
  int n_nodes;
  node_t *node;
  node_t **nodes;

  n_nodes = 0;
  nodes = ALLOC(node_t *, network_num_pi(network));
  foreach_primary_input(network, gen, node) {
    if (node_num_fanout(node) == 0) {
      nodes[n_nodes++] = node;
    }
  }
  for (i = 0; i < n_nodes; i++) {
    network_delete_node(network, nodes[i]);
  }
  FREE(nodes);
}

static void replace_latch_by_node(network, latch, node) network_t *network;
latch_t *latch;
node_t *node;
{
  lsGen gen;
  node_t *pi, *po, *fanin;

  pi = latch_get_output(latch);
  po = latch_get_input(latch);
  network_delete_latch(network, latch);
  fanin = node_get_fanin(po, 0);
  assert(node_patch_fanin(po, fanin, node));
  node_scc(po);
  network_connect(po, pi);
}

/*
 * given a function 'fn' and its don't care set 'dc'
 * computes a function in the interval (fn, dc)
 * with small support. Heuristic.
 */

static void get_good_support_fn(extra_vars, fn, dc, support_size, new_fn,
                                new_dc) extra_vars_t *extra_vars;
bdd_t *fn;
bdd_t *dc;
int *support_size;
bdd_t **new_fn;
bdd_t **new_dc;
{
  int i;
  bdd_t *var;
  bdd_t *derived_fn;
  bdd_t *g, *h;
  bdd_t *bigger_g, *smaller_h;
  array_t *var_array;
  var_set_t *support;

  g = bdd_and(fn, dc, 1, 0);
  h = bdd_or(fn, dc, 1, 1);
  var_array = array_alloc(bdd_t *, 1);
  for (i = 0; i < array_n(extra_vars->old_vars); i++) {
    var = array_fetch(bdd_t *, extra_vars->old_vars, i);
    array_insert(bdd_t *, var_array, 0, var);
    bigger_g = bdd_smooth(g, var_array);
    smaller_h = bdd_consensus(h, var_array);
    if (bdd_leq(bigger_g, smaller_h, 1, 1)) {
      bdd_free(g);
      g = bigger_g;
      bdd_free(h);
      h = smaller_h;
    } else {
      bdd_free(bigger_g);
      bdd_free(smaller_h);
    }
  }
  array_free(var_array);
  *new_fn = g;
  *new_dc = bdd_xor(g, h);
  bdd_free(h);
  support = bdd_get_support(*new_fn);
  *support_size = var_set_n_elts(support);
  var_set_free(support);
}

/*
 * Allocates an 'extra_var_t' structure.
 */

static extra_vars_t *extra_vars_alloc(seq_info) seq_info_t *seq_info;
{
  int i;
  extra_vars_t *result;
  bdd_t *new_var, *support_var;

  result = ALLOC(extra_vars_t, 1);
  result->manager = seq_info->manager;
  result->old_vars = seq_info->present_state_vars;
  result->new_vars = array_alloc(bdd_t *, 0);
  result->support_vars = array_alloc(bdd_t *, 0);
  result->new_to_support_map = st_init_table(st_numcmp, st_numhash);
  for (i = 0; i < array_n(result->old_vars); i++) {
    support_var = bdd_create_variable(seq_info->manager);
    array_insert_last(bdd_t *, result->support_vars, support_var);
    new_var = bdd_create_variable(seq_info->manager);
    array_insert_last(bdd_t *, result->new_vars, new_var);
    st_insert(result->new_to_support_map, (char *)bdd_top_var_id(new_var),
              (char *)bdd_top_var_id(support_var));
  }
  return result;
}

static void extra_vars_free(extra_vars) extra_vars_t *extra_vars;
{
  Prl_FreeBddArray(extra_vars->new_vars);
  Prl_FreeBddArray(extra_vars->support_vars);
  st_free_table(extra_vars->new_to_support_map);
  FREE(extra_vars);
}

/*
 * To compute output distances.
 */

static int get_output_distance(from_var_to_pi, var) st_table *from_var_to_pi;
bdd_t *var;
{
  node_t *pi;
  int distance;
  st_table *output_distances = st_init_table(st_numcmp, st_numhash);

  assert(st_lookup(from_var_to_pi, (char *)var, (char **)&pi));
  distance = get_output_distance_rec(output_distances, pi);
  st_free_table(output_distances);
  return distance;
}

static int get_output_distance_rec(table, node) st_table *table;
node_t *node;
{
  node_t *pi;
  lsGen gen;
  node_t *fanout;
  latch_t *latch;
  int distance, fanout_distance;

  if (st_lookup(table, (char *)node, NIL(char *)))
    return;
  distance = 0;
  foreach_fanout(node, gen, fanout) {
    fanout_distance = get_output_distance_rec(table, fanout);
    distance = MAX(distance, fanout_distance);
  }
  if (node->type != PRIMARY_INPUT)
    distance++;
  st_insert(table, (char *)node, (char *)distance);
  return distance;
}

/*
 * Checks for gates that are fed only by latches
 * and move the latches forward.
 * Be careful to compute the initial state properly.
 */

static void PerformLocalRetiming(network, verbosity) network_t *network;
int verbosity;
{
  int done;
  int i;
  node_t *node;
  array_t *nodes;

  network_sweep(network);
  do {
    done = 1;
    nodes = network_dfs(network);
    for (i = 0; i < array_n(nodes); i++) {
      node = array_fetch(node_t *, nodes, i);
      if (save_enough_latches(network, node)) {
        move_latches_forward(network, node, verbosity);
        network_sweep(network);
        done = 0;
        break;
      }
    }
    array_free(nodes);
  } while (done == 0);
}

/*
 * we need to add a latch at the output
 */

static int save_enough_latches(network, node) network_t *network;
node_t *node;
{
  int i;
  int n_latches_saved;
  node_t *fanin;

  if (node->type == PRIMARY_INPUT)
    return 0;
  if (node->type == PRIMARY_OUTPUT)
    return 0;
  n_latches_saved = -1;
  foreach_fanin(node, i, fanin) {
    if (fanin->type != PRIMARY_INPUT)
      return 0;
    if (network_is_real_pi(network, fanin))
      return 0;
    if (node_num_fanout(fanin) == 1)
      n_latches_saved++;
  }
  return (n_latches_saved < 0) ? 0 : n_latches_saved;
}

/*
 * "node" has only latches at its inputs
 * these latches are moved forward
 */

static void move_latches_forward(network, node, verbosity) network_t *network;
node_t *node;
int verbosity;
{
  int i;
  node_t *po, *pi;
  lsGen gen;
  node_t *fanout;
  node_t *buffer;
  node_t *fanin, *input;
  int init_value;
  latch_t *latch;
  st_table *latches;

  if (verbosity > 0) {
    (void)fprintf(sisout, "latches moved forward node %s\n", node->name);
  }

  /*
   * First add the new latch at the output of 'node'
   */

  foreach_fanin(node, i, fanin) {
    assert((latch = latch_from_node(fanin)) != NIL(latch_t));
    SET_VALUE(fanin, latch_get_initial_value(latch));
    if (verbosity > 1) {
      (void)fprintf(sisout, "fanin %s\n", fanin->name);
    }
  }
  simulate_node(node);
  init_value = GET_VALUE(node);
  buffer = node_literal(node, 1);
  network_add_node(network, buffer);
  foreach_fanout(node, gen, fanout) {
    if (fanout == buffer)
      continue;
    assert(node_patch_fanin(fanout, node, buffer));
    node_scc(fanout);
  }
  network_disconnect(node, buffer, &po, &pi);
  network_create_latch(network, &latch, po, pi);
  latch_set_initial_value(latch, init_value);
  latch_set_current_value(latch, init_value);

  /*
   * Then remove the latches at the input of 'node'
   */

  latches = st_init_table(st_ptrcmp, st_ptrhash);
  foreach_fanin(node, i, fanin) {
    assert((latch = latch_from_node(fanin)) != NIL(latch_t));
    st_insert(latches, (char *)latch, NIL(char));
  }
  red_latch_remove_latches(network, node, latches);
  st_free_table(latches);
}

/*
 * "table" contains a list of latches to be removed
 * presumably the latches at the inputs of node "node".
 * replace, in the fanin of "node", the output of each latch
 * by the input of that latch. If the latch has no more outputs,
 * it is removed.
 */

static void red_latch_remove_latches(network, node, table) network_t *network;
node_t *node;
st_table *table;
{
  st_generator *gen;
  latch_t *latch;
  node_t *input, *output, *fanin;

  st_foreach_item(table, gen, (char **)&latch, NIL(char *)) {
    input = latch_get_input(latch);
    output = latch_get_output(latch);
    if (node_num_fanout(output) == 1) {
      assert(node_get_fanout(output, 0) == node);
      network_delete_latch(network, latch);
      network_connect(input, output);
    } else {
      fanin = node_get_fanin(input, 0);
      assert(node_patch_fanin(node, output, fanin));
      node_scc(node);
    }
  }
}

/* EXTERNAL INTERACE */

/*
 *----------------------------------------------------------------------
 * Prl_LatchOutput -- INTERNAL INTERFACE
 *
 * Forces the listed external POs to be fed by a latch
 * by forward retiming of latches in the transitive fanin of the PO.
 * If one of the POs depends combinationally on one of the external PIs
 * the routine reports an error status code.
 * This function is useful out there in the real world; i.e.
 * if we want to control a memory chip, 'latch_output' is a simple way to make
 *sure that the write_enable signal does not glitch.
 *
 * Results:
 *	Returns 0 iff the listed POs are all fed by a latch or a constant.
 *
 * Side effects:
 *	'network' is modified in place.
 *
 *----------------------------------------------------------------------
 */

static int force_latched_output(network_t *, node_t *, int);

int Prl_LatchOutput(network, node_vec, verbosity) network_t *network;
array_t *node_vec;
int verbosity;
{
  int i;
  int status;
  node_t *node;

  for (i = 0; i < array_n(node_vec); i++) {
    node = array_fetch(node_t *, node_vec, i);
    if (!network_is_real_po(network, node)) {
      (void)fprintf(siserr, "cannot apply latch_output to node %s\n",
                    node->name);
      return 1;
    }
    status = force_latched_output(network, node, verbosity);
    if (status == 1) {
      (void)fprintf(siserr,
                    "Combinational logic dependency detected for node \"%s\":",
                    node->name);
      (void)fprintf(siserr, "latches cannot be moved forward\n");
      return 1;
    } else if (status == 2) {
      (void)fprintf(siserr, "Warning: node \"%s\" is fed by a constant\n",
                    node->name);
    }
  }
  return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * force_latched_output -- INTERNAL ROUTINE
 *
 * Moves latches forward until a primary output is reached.
 * Looks through the transitive fanin of the PO until it finds
 * a node only fed by latches. Across such a node, called 'candidate' here,
 * we push latches. This process continues until the fanin of the PO is
 * a latch or until we do not find any more candidates.
 *
 * WARNING: if the PO is fed by a constant node, the algorithm outlined
 * above enters an infinite loop ('candidate' will always be equal to 'fanin')
 * To avoid problems with constant nodes, we perform a 'network_sweep'
 * before entering the loop, and add a special test.
 *
 * Results:
 *	0 if PO is set to be the output of a latch.
 *	1 if PO depends combinationally on some external PIs.
 *	2 if PO is fed by a constant.
 *
 * Side effects:
 * 	'network' is modified in place. Some latches may be moved forward
 *	across logic.
 *
 *----------------------------------------------------------------------
 */

static node_t *get_candidate_node(node_t *);

static int force_latched_output(network, po, verbosity) network_t *network;
node_t *po;
int verbosity;
{
  node_t *fanin, *candidate;

  network_sweep(network);
  for (;;) {
    fanin = node_get_fanin(po, 0);
    if (fanin->type == PRIMARY_INPUT && latch_from_node(fanin) != NIL(latch_t))
      return 0;
    if (node_num_fanin(fanin) == 0)
      return 2;
    candidate = get_candidate_node(fanin);
    if (candidate == NIL(node_t))
      return 1;
    move_latches_forward(network, candidate, verbosity);
    network_sweep(network);
  }
}

/*
 * returns a node in the transitive fanin of "node" whose fanins are all
 * latches. returns NIL(node_t) if it cannot find any.
 */

static node_t *get_candidate_node_rec(node_t *, st_table *);

static node_t *get_candidate_node(node) node_t *node;
{
  node_t *candidate;
  st_table *visited = st_init_table(st_ptrcmp, st_ptrhash);

  candidate = get_candidate_node_rec(node, visited);
  st_free_table(visited);
  return candidate;
}

static int node_only_fed_by_latches(node_t *);

static node_t *get_candidate_node_rec(node, visited) node_t *node;
st_table *visited;
{
  int i;
  node_t *fanin;
  node_t *candidate;

  if (st_lookup(visited, (char *)node, NIL(char *)))
    return NIL(node_t);
  if (node_only_fed_by_latches(node))
    return node;
  foreach_fanin(node, i, fanin) {
    if ((candidate = get_candidate_node_rec(fanin, visited)) != NIL(node_t))
      return candidate;
  }
  st_insert(visited, (char *)node, NIL(char));
  return NIL(node_t);
}

static int node_only_fed_by_latches(node) node_t *node;
{
  int i;
  node_t *fanin;
  network_t *network;

  if (node->type == PRIMARY_INPUT)
    return 0;
  if (node->type == PRIMARY_OUTPUT)
    return 0;
  foreach_fanin(node, i, fanin) {
    if (fanin->type != PRIMARY_INPUT)
      return 0;
    if (latch_from_node(fanin) == NIL(latch_t))
      return 0;
  }
  return 1;
}

/*
 * For debugging
 */

static void prl_print_bdd(fn, var_names) bdd_t *fn;
array_t *var_names;
{
  network_t *network;

  network = ntbdd_bdd_single_to_network(fn, "foo", var_names);
  com_execute(&network, "collapse; simplify; print");
  network_free(network);
}

static int CanFlipInitialBit(network_t *, seq_info_t *, latch_t *);

/*
 *----------------------------------------------------------------------
 *
 * RemoveOneBootLatch -- INTERNAL ROUTINE
 *
 * a 'boot_latch' is a latch fed by a constant different from its initial value.
 * This routine looks for a boot latch such that there is a state
 * equivalent to the initial state for which the initial value of the latch
 * becomes equal to the constant that feeds it.
 * We remove only one latch at a time to avoid the additional complexity
 * of updating 'seq_info' records and other related data structures.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If possible, the initial state is changed to an equivalent state
 *	and one boot latch is removed.
 *
 *----------------------------------------------------------------------
 */

static void RemoveOneBootLatch(network, options) network_t *network;
prl_options_t *options;
{
  lsGen gen;
  latch_t *latch;
  node_t *po, *fanin;
  node_t *constant_node;
  int input_value;
  seq_info_t *seq_info;

  seq_info = Prl_SeqInitNetwork(network, options);
  foreach_latch(network, gen, latch) {
    po = latch_get_input(latch);
    fanin = node_get_fanin(po, 0);
    switch (node_function(fanin)) {
    case NODE_0:
      input_value = 0;
      break;
    case NODE_1:
      input_value = 1;
      break;
    default:
      continue;
    }
    if (input_value == latch_get_initial_value(latch))
      continue;
    if (!CanFlipInitialBit(network, seq_info, latch))
      continue;

    /* replace the latch by a constant and exit the loop */
    constant_node = node_constant(latch_get_initial_value(latch));
    network_add_node(network, constant_node);
    replace_latch_by_node(network, latch, constant_node);
    break;
  }
  Prl_SeqInfoFree(seq_info, options);
}

static bdd_t *ComputeFlippedLiteral(network_t *, seq_info_t *, latch_t *);
static bdd_t *BddFindAlternateInit(bdd_t *, bdd_t *, seq_info_t *);
static bdd_t *BddAndPositive(bdd_t *, bdd_t *);
static bdd_t *BinaryMergeBdds(bdd_t **, int, BddFn, bdd_t *);
static void ForceNewInitState(network_t *, seq_info_t *, bdd_t *);

/*
 *----------------------------------------------------------------------
 *
 * CanFlipInitialBit -- INTERNAL ROUTINE
 *
 * Try to flip the initial bit of latch 'latch' by selecting a state
 * equivalent to the initial state.
 *
 * Results:
 *	returns 1 if such an initial state with found.
 *	returns 0 otherwise.
 *
 * Side effects:
 *	if such an initial state is found, 'seq_info->init_state_fn'
 *	is modified to correspond to that new state
 *	and the latch initial values and current values are updated
 *	to correspond to the new initial state.
 *
 *----------------------------------------------------------------------
 */

static int CanFlipInitialBit(network, seq_info, latch) network_t *network;
seq_info_t *seq_info;
latch_t *latch;
{
  int i, n_latches, n_elts;
  bdd_t *fn;
  bdd_t **fn_array;
  bdd_t *literal;
  bdd_t *result;
  bdd_t *one;
  bdd_t *new_init_state;

  literal = ComputeFlippedLiteral(network, seq_info, latch);
  n_latches = array_n(seq_info->next_state_fns);
  n_elts = n_latches + array_n(seq_info->external_output_fns);
  fn_array = ALLOC(bdd_t *, n_elts);
  for (i = 0; i < n_latches; i++) {
    fn = array_fetch(bdd_t *, seq_info->next_state_fns, i);
    fn_array[i] = BddFindAlternateInit(fn, literal, seq_info);
  }
  for (i = 0; i < array_n(seq_info->external_output_fns); i++) {
    fn = array_fetch(bdd_t *, seq_info->external_output_fns, i);
    fn_array[n_latches + i] = BddFindAlternateInit(fn, literal, seq_info);
  }
  bdd_free(literal);
  one = bdd_one(seq_info->manager);
  result = BinaryMergeBdds(fn_array, n_elts, BddAndPositive, one);
  FREE(fn_array);
  if (bdd_is_tautology(result, 0)) {
    bdd_free(result);
    return 0;
  }
  Prl_GetOneEdge(result, seq_info, &new_init_state, NIL(bdd_t *));
  bdd_free(seq_info->init_state_fn);
  seq_info->init_state_fn = new_init_state;
  ForceNewInitState(network, seq_info, new_init_state);
  return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * BddFindAlternateInit -- INTERNAL ROUTINE
 *
 * Computes all the states x such that the output 'fn'(x,i)
 * is equal to 'fn'(init_state,i) for all input values i.
 *
 * Results:
 *	A bdd_t* corresponding to the set of such states.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static bdd_t *BddFindAlternateInit(fn, literal, seq_info) bdd_t *fn;
bdd_t *literal;
seq_info_t *seq_info;
{
  bdd_t *tmp1, *tmp2;

  tmp1 =
      bdd_and_smooth(fn, seq_info->init_state_fn, seq_info->present_state_vars);
  tmp2 = bdd_xnor(tmp1, fn);
  bdd_free(tmp1);
  tmp1 = bdd_consensus(tmp2, seq_info->external_input_vars);
  bdd_free(tmp2);
  tmp2 = bdd_and(tmp1, literal, 1, 1);
  bdd_free(tmp1);
  return tmp2;
}

/*
 *----------------------------------------------------------------------
 *
 * ComputeFlippedLiteral -- INTERNAL ROUTINE
 *
 * let Xi be the input var corresponding to the output of 'latch'. Then:
 * 	if init value of latch is 0 returns     Xi
 *	if init value of latch is 1 returns not(Xi)
 *
 * Results:
 *	A literal, in BDD form.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static bdd_t *ComputeFlippedLiteral(network, seq_info,
                                    latch) network_t *network;
seq_info_t *seq_info;
latch_t *latch;
{
  int i;
  node_t *pi;
  bdd_t *var, *literal;

  pi = latch_get_output(latch);
  for (i = 0; i < array_n(seq_info->input_nodes); i++) {
    if (pi == array_fetch(node_t *, seq_info->input_nodes, i))
      break;
  }
  var = array_fetch(bdd_t *, seq_info->input_vars, i);
  literal = (latch_get_initial_value(latch)) ? bdd_not(var) : bdd_dup(var);
  return literal;
}

/*
 *----------------------------------------------------------------------
 *
 * ForceNewInitState -- INTERNAL ROUTINE
 *
 * Given a new initial state expressed as a BDD,
 * visit all the latches and assert the new value
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The initial state of 'network' is modified.
 *
 *----------------------------------------------------------------------
 */

static void ForceNewInitState(network, seq_info,
                              new_init_state) network_t *network;
seq_info_t *seq_info;
bdd_t *new_init_state;
{
  int i;
  latch_t *latch;
  node_t *pi, *po;
  bdd_t *var;
  int init_value;
  int index;
  st_table *pi_to_var_table;

  pi_to_var_table = Prl_GetPiToVarTable(seq_info);
  for (i = 0; i < array_n(seq_info->next_state_fns); i++) {
    po = array_fetch(node_t *, seq_info->output_nodes, i);
    assert((latch = latch_from_node(po)) != NIL(latch_t));
    pi = latch_get_output(latch);
    assert(st_lookup(pi_to_var_table, (char *)pi, (char **)&var));
    init_value = (bdd_leq(new_init_state, var, 1, 1)) ? 1 : 0;
    latch_set_initial_value(latch, init_value);
    latch_set_current_value(latch, init_value);
  }
  st_free_table(pi_to_var_table);
}

/*
 *----------------------------------------------------------------------
 *
 * BinaryMergeBdds -- INTERNAL ROUTINE
 *
 * Applies 'bdd_fn', which takes two bdd_t*s as arguments and returns one,
 * on all elements in 'bdd_array' and returns the result.
 * 'neutral_elt' is used as start of the merging.
 *
 * Results:
 *	A bdd_t*, the result of the application of 'bdd_fn' on the array.
 *
 * Side effects:
 *	'neutral_elt' is freed.
 *
 *----------------------------------------------------------------------
 */

static bdd_t *BinaryMergeBdds(bdd_array, n_elts, bdd_fn,
                              neutral_elt) bdd_t **bdd_array;
int n_elts;
BddFn bdd_fn;
bdd_t *neutral_elt;
{
  bdd_t **arg1, **arg2, **result, **top;
  bdd_t *fn;

  if (n_elts == 0)
    return neutral_elt;
  bdd_free(neutral_elt);
  for (;;) {
    if (n_elts == 1)
      return bdd_array[0];
    arg1 = &bdd_array[0];
    arg2 = &bdd_array[1];
    result = &bdd_array[0];
    top = &bdd_array[n_elts];
    while (arg2 < top) {
      fn = (*bdd_fn)(*arg1, *arg2);
      bdd_free(*arg1);
      bdd_free(*arg2);
      *result++ = fn;
      arg1 += 2;
      arg2 += 2;
    }
    if (arg1 < top) {
      *result++ = *arg1;
    }
    n_elts = result - bdd_array;
  }
}

/*
 *----------------------------------------------------------------------
 *
 * BddAndPositive --
 *
 * Returns the 'and' of two BDDs.
 *
 *----------------------------------------------------------------------
 */

static bdd_t *BddAndPositive(fn1, fn2) bdd_t *fn1;
bdd_t *fn2;
{ return bdd_and(fn1, fn2, 1, 1); }

#endif /* SIS */
