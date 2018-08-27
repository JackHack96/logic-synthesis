

#ifdef SIS
#include "prioqueue.h"
#include "prl_util.h"
#include "sis.h"

static void compute_po_ordering(seq_info_t *, prl_options_t *);
static void compute_pi_ordering(seq_info_t *, prl_options_t *);

/*
 *----------------------------------------------------------------------
 *
 * Prl_ProductBddOrder -- EXPORTED ROUTINE
 *
 * Produce the variable ordering required for product / consistency method.
 *
 * Results:
 * 	None.
 *
 * Side effects:
 *	'seq_info' is updated with the variable ordering information.
 *
 *----------------------------------------------------------------------
 */

void Prl_ProductBddOrder(seq_info, options) seq_info_t *seq_info;
prl_options_t *options;
{
  compute_po_ordering(seq_info, options);
  compute_pi_ordering(seq_info, options);
}

/*
 *----------------------------------------------------------------------
 *
 * Routines to implement Prl_SeqInitNetwork
 *
 *----------------------------------------------------------------------
 */

/* puts the result in seq_info->output_nodes */
/* add the external outputs at the end */

static var_set_t **extract_support_info(network_t *, array_t *);

static void compute_po_ordering(seq_info, options) seq_info_t *seq_info;
prl_options_t *options;
{
  int i, index;
  lsGen gen;
  node_t *output;
  array_t *next_state_po;
  var_set_t **support_info;
  set_info_t set_info;
  array_t *ordering;
  network_t *network = seq_info->network;

  /* extract the next_state PO's */
  next_state_po = array_alloc(node_t *, 0);
  foreach_primary_output(network, gen, output) {
    if (network_is_real_po(network, output))
      continue;
    array_insert_last(node_t *, next_state_po, output);
  }

  /* fits the information in the right format */
  /* compute the support for all nodes at the same time: much faster O(n)
   * instead of O(n^2) */
  support_info = extract_support_info(seq_info->network, next_state_po);
  set_info.n_vars = network_num_pi(network);
  set_info.n_sets = array_n(next_state_po);
  set_info.sets = support_info;

  /* orders and recasts the ordering information */
  ordering = Prl_OrderSetHeuristic(&set_info, options->verbose,
                                   options->ordering_depth);
  assert(array_n(ordering) == array_n(next_state_po));
  seq_info->output_nodes = array_alloc(node_t *, network_num_po(network));
  for (i = 0; i < array_n(ordering); i++) {
    index = array_fetch(int, ordering, i);
    output = array_fetch(node_t *, next_state_po, index);
    array_insert_last(node_t *, seq_info->output_nodes, output);
  }

  /* append the external outputs */
  foreach_primary_output(network, gen, output) {
    if (!network_is_real_po(network, output))
      continue;
    array_insert_last(node_t *, seq_info->output_nodes, output);
  }

  /* cleanup */
  for (i = 0; i < array_n(next_state_po); i++) {
    var_set_free(support_info[i]);
  }
  FREE(support_info);
  array_free(ordering);
  array_free(next_state_po);
}

static array_t *order_dfs_from_count(node_t *, st_table *, int, int *, int);
static void add_new_inputs(seq_info_t *, array_t *);
static void add_next_state_vars(seq_info_t *, st_table *);
static void ResetVariableOrder(st_table *, node_t *);
static void SetVariableOrder(st_table *, node_t *, int, int);

/*
 *----------------------------------------------------------------------
 *
 * compute_pi_ordering -- INTERNAL ROUTINE
 *
 *----------------------------------------------------------------------
 */

static void compute_pi_ordering(seq_info, options) seq_info_t *seq_info;
prl_options_t *options;
{
  char buffer[10];
  char *value;
  int i, index;
  int n_vars;
  lsGen gen;
  bdd_t *var;
  node_t *input;
  node_t *output;
  array_t *input_order;
  int next_index;
  int n_next_state_po;
  st_generator *table_gen;
  network_t *network = seq_info->network;
  st_table *leaves = st_init_table(st_ptrcmp, st_ptrhash);
  st_table *next_state_table = st_init_table(st_ptrcmp, st_ptrhash);

  /* bdd manager + leaves init: make all PI's leaves */
  foreach_primary_input(network, gen, input) {
    ResetVariableOrder(leaves, input);
  }
  seq_info->leaves = leaves;

  /* we add one variable per latch for consistency method: -> next_state_vars */
  n_vars = st_count(leaves) + network_num_latch(seq_info->network);
  seq_info->manager = ntbdd_start_manager(n_vars);

  /* initialize a bunch of arrays */
  seq_info->present_state_vars = array_alloc(bdd_t *, 0);
  seq_info->transition.transition_vars = array_alloc(bdd_t *, 0);
  seq_info->transition.next_state_vars = array_alloc(bdd_t *, 0);
  seq_info->external_input_vars = array_alloc(bdd_t *, 0);
  seq_info->input_vars = array_alloc(bdd_t *, 0);
  seq_info->input_nodes = array_alloc(node_t *, 0);
  seq_info->var_names = array_alloc(char *, 0);

  /* generate the order */
  next_index = 0;

  /* first treat the next state outputs */
  n_next_state_po = network_num_latch(seq_info->network);
  for (i = 0; i < n_next_state_po; i++) {
    output = array_fetch(node_t *, seq_info->output_nodes, i);
    input_order = order_dfs_from_count(output, leaves, DFS_ORDER, &next_index,
                                       options->verbose);
    add_new_inputs(seq_info, input_order);

    sprintf(buffer, "y:%d", i);
    array_insert_last(char *, seq_info->var_names, util_strsav(buffer));
    st_insert(next_state_table, (char *)output, (char *)next_index);
    var = bdd_get_variable(seq_info->manager, next_index);
    array_insert_last(bdd_t *, seq_info->transition.transition_vars, var);

    next_index++;
    array_free(input_order);
  }

  /* then treat the external outputs */
  for (i = n_next_state_po; i < array_n(seq_info->output_nodes); i++) {
    output = array_fetch(node_t *, seq_info->output_nodes, i);
    input_order = order_dfs_from_count(output, leaves, DFS_ORDER, &next_index,
                                       options->verbose);
    add_new_inputs(seq_info, input_order);
    array_free(input_order);
  }

  /* then add all the input variables that have not been handled so far */
  input_order = array_alloc(node_t *, 0);
  table_gen = st_init_gen(leaves);
  while (st_gen_int(table_gen, (char **)&input, &index)) {
    if (index == -1) {
      array_insert_last(node_t *, input_order, input);
    }
  }
  st_free_gen(table_gen);
  for (i = 0; i < array_n(input_order); i++) {
    input = array_fetch(node_t *, input_order, i);
    SetVariableOrder(leaves, input, i + next_index, options->verbose);
  }
  add_new_inputs(seq_info, input_order);
  array_free(input_order);

  /* finally treat the next_state_vars: should be in the same order as
   * present_state_vars */
  add_next_state_vars(seq_info, next_state_table);

  /* clean up and consistency check */
  st_free_table(next_state_table);
  assert(array_n(seq_info->input_vars) == network_num_pi(seq_info->network));

  /* debug information */
  if (options->verbose >= 1) {
    (void)fprintf(sisout, "Variable ordering selected for product method:\n");
    for (i = 0; i < array_n(seq_info->var_names); i++) {
      (void)fprintf(sisout, "%s<id=%d> ",
                    array_fetch(char *, seq_info->var_names, i), i);
      if (i % 8 == 7)
        (void)fprintf(sisout, "\n");
    }
    if (i % 8 != 7)
      (void)fprintf(sisout, "\n");
  }
  if (options->verbose >= 2) {
    int discrepancy_found = 0;
    (void)fprintf(sisout, "Checking the indices of input variables ... \n");
    for (i = 0; i < array_n(seq_info->input_nodes); i++) {
      input = array_fetch(node_t *, seq_info->input_nodes, i);
      assert(st_lookup_int(seq_info->leaves, (char *)input, &index));
      (void)fprintf(sisout, "%s<id=%d> ", input->name, index);
      if (i % 8 == 7)
        (void)fprintf(sisout, "\n");
      discrepancy_found |=
          strcmp(input->name, array_fetch(char *, seq_info->var_names, index));
    }
    if (i % 8 != 7)
      (void)fprintf(sisout, "\n");
    assert(discrepancy_found == 0);
    (void)fprintf(sisout, "Check passed. \n");
  }
}

/*
 *----------------------------------------------------------------------
 *
 * add_new_inputs -- INTERNAL ROUTINE
 *
 * For each node found in 'node_list' that is a PI, add:
 * 1. to 'seq_info->var_names', the name of that PI
 * 2. to 'seq_info->input_nodes', a pointer to that PI
 * 3. either to 'seq_info->external_input_vars'
 *   or to 'seq_info->present_state_vars' a bdd_t*
 *   representing that PI as a BDD variable.
 *
 *----------------------------------------------------------------------
 */

static void add_new_inputs(seq_info, node_list) seq_info_t *seq_info;
array_t *node_list;
{
  int i;
  int index;
  node_t *input;
  bdd_t *var;

  for (i = 0; i < array_n(node_list); i++) {
    input = array_fetch(node_t *, node_list, i);
    if (input->type != PRIMARY_INPUT)
      continue;
    array_insert_last(char *, seq_info->var_names, util_strsav(input->name));
    array_insert_last(node_t *, seq_info->input_nodes, input);
    assert(st_lookup_int(seq_info->leaves, (char *)input, &index));
    assert(index >= 0);
    var = bdd_get_variable(seq_info->manager, index);
    array_insert_last(bdd_t *, seq_info->input_vars, var);
    var = bdd_dup(var);
    if (network_is_real_pi(seq_info->network, input)) {
      array_insert_last(bdd_t *, seq_info->external_input_vars, var);
    } else {
      array_insert_last(bdd_t *, seq_info->present_state_vars, var);
    }
  }
}

/*
 *----------------------------------------------------------------------
 *
 * add_next_state_vars -- INTERNAL ROUTINE
 *
 * The next_state_vars should be inserted in the same order
 * as they appear in 'present_state_vars'.
 *
 *----------------------------------------------------------------------
 */

static void add_next_state_vars(seq_info,
                                next_state_table) seq_info_t *seq_info;
st_table *next_state_table;
{
  int i;
  int index;
  bdd_t *var;
  node_t *input;
  node_t *output;
  network_t *network = seq_info->network;

  for (i = 0; i < array_n(seq_info->input_nodes); i++) {
    input = array_fetch(node_t *, seq_info->input_nodes, i);
    if (network_is_real_pi(network, input))
      continue;
    output = network_latch_end(input);
    assert(st_lookup_int(next_state_table, (char *)output, &index));
    var = bdd_get_variable(seq_info->manager, index);
    array_insert_last(bdd_t *, seq_info->transition.next_state_vars, var);
  }
}

static void extract_support_info_rec(node_t *, st_table *, st_table *,
                                     var_set_t *);

/*
 *----------------------------------------------------------------------
 *
 * extract_support_info -- INTERNAL ROUTINE
 *
 * Results:
 * 	returns a var_set_t containing
 * 	in bitmap the info: which PI is in the support of the node.
 * 	Skips over those nodes that are not PI's.
 *
 *----------------------------------------------------------------------
 */

static var_set_t **extract_support_info(network, node_list) network_t *network;
array_t *node_list;
{
  int i, uid;
  lsGen gen;
  node_t *input, *output;
  int n_pi = network_num_pi(network);
  int n_po = array_n(node_list);
  st_table *pi_table = st_init_table(st_ptrcmp, st_ptrhash);
  var_set_t **support_info = ALLOC(var_set_t *, n_po);

  for (i = 0; i < n_po; i++) {
    support_info[i] = var_set_new(n_pi);
  }
  uid = 0;
  foreach_primary_input(network, gen, input) {
    st_insert(pi_table, (char *)input, (char *)uid);
    uid++;
  }
  for (i = 0; i < array_n(node_list); i++) {
    st_table *visited = st_init_table(st_ptrcmp, st_ptrhash);
    output = array_fetch(node_t *, node_list, i);
    extract_support_info_rec(output, pi_table, visited, support_info[i]);
    st_free_table(visited);
  }
  st_free_table(pi_table);
  return support_info;
}

static void extract_support_info_rec(node, pi_table, visited, set) node_t *node;
st_table *pi_table;
st_table *visited;
var_set_t *set;
{
  int i;
  node_t *fanin;
  int uid;

  if (st_lookup(visited, (char *)node, NIL(char *)))
    return;
  st_insert(visited, (char *)node, NIL(char));
  if (node->type == PRIMARY_INPUT) {
    assert(st_lookup_int(pi_table, (char *)node, &uid));
    var_set_set_elt(set, uid);
  } else {
    foreach_fanin(node, i, fanin) {
      extract_support_info_rec(fanin, pi_table, visited, set);
    }
  }
}

/*
 *----------------------------------------------------------------------
 *
 * order_dfs_from_count -- INTERNAL ROUTINE
 *
 * A hack: order PIs reached from 'node' starting at 'order_count'.
 * The hack is necessary due to the fact that the library routine
 * 'order_dfs' is not general enough to start allocating variable indices
 * from another value than 0. We call 'order_dfs' and shift the resulting
 * ordering using '*order_count' as an offset. We check that PIs
 * have not been allocated an index previously by looking in 'leaves'.
 *
 * Results:
 * 	returns an array of PIs for which a variable ordering has been found.
 *
 * Side effects:
 *	1. stores that variable ordering in 'leaves'.
 *	2. increments '*order_count' by the number of PIs returned.
 *
 *----------------------------------------------------------------------
 */

static array_t *order_dfs_from_count(node, leaves, fixed_root_order,
                                     order_count, verbosity)
    node_t *node; /* root node: its transitive fanin has to be ordered */
st_table
    *leaves; /* where the dfs search stops: those nodes have to be ordered */
int fixed_root_order; /* flag: if set, can't change the order of visit of the
                         roots */
int *order_count;     /* start the var indices from '*order_count' */
int verbosity;
{
  st_generator *gen;
  char *key;
  char *value;
  int i, index;
  int previous_index;
  array_t *roots;
  array_t *nodes;
  array_t *result;

  st_table *local_leaves = st_init_table(st_ptrcmp, st_ptrhash);
  st_foreach_item(leaves, gen, &key, &value) {
    ResetVariableOrder(local_leaves, (node_t *)key);
  }
  roots = array_alloc(node_t *, 0);
  array_insert_last(node_t *, roots, node);
  nodes = order_dfs(roots, local_leaves, DFS_ORDER);
  array_free(roots);

  /*
   * If the node already has an index (i.e. >= 0)
   * we skip it (i.e. we keep the index it has).
   * Otherwise we look into 'local_leaves'.
   * If the node does not have either an index in 'local_leaves'
   * either, we skip it. Otherwise, we store it into the array 'result'.
   * 'previous_index' is here just as a consistency check.
   */

  result = array_alloc(node_t *, 0);
  previous_index = -1;
  for (i = 0; i < array_n(nodes); i++) {
    node = array_fetch(node_t *, nodes, i);
    if (node->type != PRIMARY_INPUT)
      continue;
    assert(st_lookup_int(leaves, (char *)node, &index));
    if (index >= 0)
      continue;
    assert(st_lookup_int(local_leaves, (char *)node, &index));
    if (index == -1)
      continue;
    assert(previous_index < index);
    previous_index = index;
    array_insert_last(node_t *, result, node);
  }
  array_free(nodes);
  st_free_table(local_leaves);

  /*
   * For each node in 'result', we store a varid in 'leaves'.
   * That varid is the index of the node in 'result'
   * offset by '*order_count'.
   */

  for (i = 0; i < array_n(result); i++) {
    node = array_fetch(node_t *, result, i);
    SetVariableOrder(leaves, node, i + *order_count, verbosity);
  }

  *order_count += array_n(result);
  return result;
}

/*
 *----------------------------------------------------------------------
 *
 * ResetVariableOrder -- INTERNAL ROUTINE
 *
 *----------------------------------------------------------------------
 */

static void ResetVariableOrder(leaves, node) st_table *leaves;
node_t *node;
{
  int index;

  assert(!st_lookup_int(leaves, (char *)node, &index));
  st_insert(leaves, (char *)node, (char *)-1);
}

/*
 *----------------------------------------------------------------------
 *
 * SetVariableOrder -- INTERNAL ROUTINE
 *
 *----------------------------------------------------------------------
 */

static void SetVariableOrder(leaves, node, varid, verbosity) st_table *leaves;
node_t *node;
int varid;
int verbosity;
{
  int index;

  if (verbosity >= 2) {
    (void)fprintf(sisout, "set var id: %s <id=%d>\n", node->name, varid);
    assert(st_lookup_int(leaves, (char *)node, &index));
    assert(index == -1);
  }
  st_insert(leaves, (char *)node, (char *)varid);
}

/*
 *----------------------------------------------------------------------
 *
 * Prl_ProductInitSeqInfo -- EXPORTED ROUTINE
 *
 * Side effects:
 *	Allocates the transition relation part of the 'seq_info' records.
 *
 *----------------------------------------------------------------------
 */

void Prl_ProductInitSeqInfo(seq_info, options) seq_info_t *seq_info;
prl_options_t *options;
{
  int i;
  bdd_t *fn;
  bdd_t *var;
  int n_elts;
  array_t *xnor_array;

  n_elts = array_n(seq_info->next_state_fns);
  xnor_array = array_alloc(bdd_t *, n_elts);
  for (i = 0; i < n_elts; i++) {
    fn = array_fetch(bdd_t *, seq_info->next_state_fns, i);
    var = array_fetch(bdd_t *, seq_info->transition.transition_vars, i);
    fn = bdd_xnor(fn, var);
    array_insert_last(bdd_t *, xnor_array, fn);
  }
  seq_info->transition.product.transition_fns = xnor_array;
}

/*
 *----------------------------------------------------------------------
 *
 * Prl_ProductFreeSeqInfo -- EXPORTED ROUTINE
 *
 * Side effects:
 *	Frees the transition relation part of the 'seq_info' records.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */

void Prl_ProductFreeSeqInfo(seq_info, options) seq_info_t *seq_info;
prl_options_t *options;
{
  Prl_FreeBddArray(seq_info->transition.transition_vars);
  Prl_FreeBddArray(seq_info->transition.next_state_vars);
  Prl_FreeBddArray(seq_info->transition.product.transition_fns);
}

/*
 *----------------------------------------------------------------------
 *
 * Prl_ProductExtractNextworkInputNames -- EXPORTED ROUTINE
 *
 * Results:
 * 	Returns an array of char*, where the name of each PI
 *	is stored in order, except for the PIs that have been
 *	inserted for the product / consistency methods.
 *	Those PIs get a NULL pointer instead.
 *
 *----------------------------------------------------------------------
 */

array_t *Prl_ProductExtractNetworkInputNames(seq_info,
                                             options) seq_info_t *seq_info;
prl_options_t *options;
{
  int i;
  array_t *result;
  char *name;

  result = array_alloc(char *, 0);
  for (i = 0; i < array_n(seq_info->var_names); i++) {
    name = array_fetch(char *, seq_info->var_names, i);
    if (name[0] == 'y' && name[1] == ':')
      name = NIL(char);
    array_insert_last(char *, result, name);
  }
  return result;
}

static bdd_t *BddIncrAndSmooth(seq_info_t *, bdd_t *, array_t *,
                               prl_options_t *);

/*
 *----------------------------------------------------------------------
 *
 * Prl_ProductComputeNextStates -- EXTERNAL ROUTINE
 *
 * Results:
 *	the states reachable in one step from the states in 'current_set'.
 *
 * Side effects:
 * 	Does not free 'current_set'.
 *
 *----------------------------------------------------------------------
 */

bdd_t *Prl_ProductComputeNextStates(current_set, seq_info,
                                    options) bdd_t *current_set;
seq_info_t *seq_info;
prl_options_t *options;
{
  bdd_t *result_as_next_state_vars;
  bdd_t *new_current_set;

  if (bdd_is_tautology(current_set, 0)) {
    result_as_next_state_vars = bdd_dup(current_set);
  } else {
    result_as_next_state_vars = BddIncrAndSmooth(
        seq_info, current_set, seq_info->transition.next_state_vars, options);
  }
  new_current_set = bdd_substitute(result_as_next_state_vars,
                                   seq_info->transition.next_state_vars,
                                   seq_info->present_state_vars);

  if (options->verbose >= 3) {
    (void)fprintf(sisout, "(%d->%d)]", bdd_size(result_as_next_state_vars),
                  bdd_size(new_current_set));

    (void)fflush(sisout);
  }

  bdd_free(result_as_next_state_vars);
  return new_current_set;
}

/*
 *----------------------------------------------------------------------
 *
 * Prl_ProductReverseImage -- EXTERNAL ROUTINE
 *
 * Traverses a sequential circuit backwards.
 *
 *----------------------------------------------------------------------
 */

bdd_t *Prl_ProductReverseImage(next_set, seq_info, options) bdd_t *next_set;
seq_info_t *seq_info;
prl_options_t *options;
{
  bdd_t *next_set_as_next_state_vars;
  bdd_t *result;

  next_set_as_next_state_vars =
      bdd_substitute(next_set, seq_info->present_state_vars,
                     seq_info->transition.next_state_vars);

  result = BddIncrAndSmooth(seq_info, next_set_as_next_state_vars,
                            seq_info->input_vars, options);

  bdd_free(next_set_as_next_state_vars);
  return result;
}

/* INTERNAL INTERFACE */

/* types used in PRODUCT for smoothing as soon as possible */

typedef struct {
  int varid;
  bdd_t *var; /* result of bdd_get_variable(mgr, varid) */
  st_table *
      fn_table; /* list of indices of fns depending on that variable in fns[] */
  int last_index; /* the most recent fn index depending on this var */
} var_info_t;

typedef struct {
  int fnid;           /* small integer; unique identifier for the function */
  bdd_t *fn;          /* accumulation of AND's of functions */
  int cost;           /* simply an index for static merging; */
                      /* size of bdd for dynamic merging */
  var_set_t *support; /* support of that function */
  int partition;      /* to which partition the fn belongs */
} fn_info_t;

/* should avoid smoothing out next_state_vars ... */

typedef struct {
  int n_fns;
  fn_info_t **fns;
  int n_vars;
  var_info_t **vars;
  queue_t *queue; /* priority queue where to store the functions by size */
  bdd_manager *manager;
  int n_partitions;
  int *partition_count; /* number of fns alive in each partition */
  int *next_partition;  /* map to tell in which partition to go when current one
                           is empty */
} support_info_t;

/* result < 0 means obj1 higher priority than obj2 (obj1 comes first) */

static int fn_info_cmp(obj1, obj2) char *obj1;
char *obj2;
{
  int diff;
  fn_info_t *info1 = (fn_info_t *)obj1;
  fn_info_t *info2 = (fn_info_t *)obj2;
  diff = info1->partition - info2->partition;
  if (diff == 0)
    diff = info1->cost - info2->cost;
  return diff;
}

static void fn_info_print(obj) char *obj;
{
  fn_info_t *info = (fn_info_t *)obj;
  (void)fprintf(misout, "fn %d", info->fnid);
}

static support_info_t *support_info_extract(seq_info_t *, bdd_t **, int,
                                            array_t *, prl_options_t *);
static void support_info_free(support_info_t *);
static bdd_t *do_fn_merging(support_info_t *, prl_options_t *);

/*
 *----------------------------------------------------------------------
 *
 * BddIncrAndSmooth -- INTERNAL ROUTINE
 *
 * Performs the bdd_and and the existential quantifier in one step.
 * The existential quantifier is applied as early as possible.
 *
 *----------------------------------------------------------------------
 */

static bdd_t *BddIncrAndSmooth(seq_info, current_set, keep_vars,
                               options) seq_info_t *seq_info;
bdd_t *current_set;
array_t *keep_vars;
prl_options_t *options;
{
  int i;
  int n_fns;
  bdd_t *tmp;
  bdd_t **fns;
  bdd_t *resulting_product;
  support_info_t *support_info;
  array_t *transition_outputs;

  if (options->verbose >= 3) {
    (void)fprintf(misout, "[");
    (void)fflush(misout);
  }

  transition_outputs = seq_info->transition.product.transition_fns;
  n_fns = array_n(transition_outputs);
  fns = ALLOC(bdd_t *, n_fns);
  for (i = 0; i < n_fns; i++) {
    tmp = array_fetch(bdd_t *, transition_outputs, i);
    fns[i] = bdd_cofactor(tmp, current_set);
    if (options->verbose >= 3) {
      (void)fprintf(misout, "(#%d->%d),", i, bdd_size(tmp));
      (void)fprintf(misout, "(#%d->%d),", i, bdd_size(fns[i]));
      (void)fflush(misout);
    }
  }

  support_info = support_info_extract(seq_info, fns, n_fns, keep_vars, options);
  FREE(fns);
  resulting_product = do_fn_merging(support_info, options);
  support_info_free(support_info);
  return resulting_product;
}

static void extract_var_info(support_info_t *, st_table *);
static void smooth_lonely_variables(support_info_t *, prl_options_t *);
static void initialize_partition_info(support_info_t *);
static st_table *get_varids_in_table(array_t *);
static void extract_fn_info(support_info_t *, bdd_t **, prl_options_t *);

/*
 *----------------------------------------------------------------------
 *
 * support_info_extract -- INTERNAL ROUTINE
 *
 *----------------------------------------------------------------------
 */

static support_info_t *support_info_extract(seq_info, fns, n_fns, keep_vars,
                                            options) seq_info_t *seq_info;
bdd_t **fns;
int n_fns;
array_t *keep_vars;
prl_options_t *options;
{
  int i;
  st_table *skip_varids;
  support_info_t *support_info = ALLOC(support_info_t, 1);

  support_info->n_fns = n_fns;

  /* allocate a priority queue for storing the functions */
  support_info->queue =
      init_queue(support_info->n_fns, fn_info_cmp, fn_info_print);

  /* compute the info related to each fn */
  support_info->fns = ALLOC(fn_info_t *, support_info->n_fns);
  extract_fn_info(support_info, fns, options);

  /* get a hash table containing the varids of all the next state variables */
  skip_varids = get_varids_in_table(keep_vars);

  /* compute the info related to each variable; skip over next_state_vars */
  support_info->manager = seq_info->manager;
  support_info->n_vars = array_n(seq_info->var_names);
  support_info->vars = ALLOC(var_info_t *, support_info->n_vars);
  extract_var_info(support_info, skip_varids);

  /* simple static binary tree */
  support_info->n_partitions = support_info->n_fns;
  for (i = 0; i < support_info->n_fns; i++) {
    support_info->fns[i]->partition = i;
  }
  initialize_partition_info(support_info);

  /* smooth now all variables that appear only once */
  smooth_lonely_variables(support_info, options);

  /* deallocate what is no longer needed */
  st_free_table(skip_varids);

  return support_info;
}

/* ARGSUSED */
static void extract_fn_info(support_info, fns,
                            options) support_info_t *support_info;
bdd_t **fns;
prl_options_t *options;
{
  int i;
  fn_info_t *info;

  for (i = 0; i < support_info->n_fns; i++) {
    info = ALLOC(fn_info_t, 1);
    info->fnid = i;
    info->partition = 0;
    info->fn = fns[i];
    info->support = bdd_get_support(fns[i]);
    info->cost = i;
    put_queue(support_info->queue, (char *)info);
    support_info->fns[i] = info;
  }
}

/* extract varids from array of bdd_t *'s and put in a hash table */
static st_table *get_varids_in_table(bdd_array) array_t *bdd_array;
{
  int i;
  int varid;
  st_table *result = st_init_table(st_numcmp, st_numhash);
  array_t *var_array = bdd_get_varids(bdd_array);

  for (i = 0; i < array_n(var_array); i++) {
    varid = array_fetch(int, var_array, i);
    st_insert(result, (char *)varid, NIL(char));
  }
  array_free(var_array);
  return result;
}

/* valid for all methods */
/* compute n such that n = 2^p <= n_partitions < 2^{p+1} */
/* compute x such that x + n = n_partitions */
/* x is guaranteed to be such that 2 * x < n_partitions */

static void
    initialize_partition_info(support_info) support_info_t *support_info;
{
  int n_partitions;
  int partition;
  int i, n, x;
  int count;
  int n_entries;
  int *partition_map;
  st_table *table = st_init_table(st_numcmp, st_numhash);

  if (support_info->n_fns == 0) {
    support_info->partition_count = ALLOC(int, 0);
    support_info->next_partition = ALLOC(int, 0);
    return;
  }

  /* first make the partition identifiers consecutive integers from 0 to
   * n_entries - 1 */
  n_partitions = 0;
  for (i = 0; i < support_info->n_fns; i++) {
    if (st_lookup_int(table, (char *)support_info->fns[i]->partition,
                      &partition)) {
      support_info->fns[i]->partition = partition;
    } else {
      st_insert(table, (char *)support_info->fns[i]->partition,
                (char *)n_partitions);
      support_info->fns[i]->partition = n_partitions++;
    }
  }
  st_free_table(table);

  /* just a consistency check */
  assert(n_partitions == support_info->n_partitions);

  /* compute n=2^p such that 2^p <= n_partitions < 2^{p+1} */
  /* and compute x = n_partitions - 2^p */
  for (n = 1; n <= n_partitions; n <<= 1)
    ;
  if (n > n_partitions)
    n >>= 1;
  assert(n <= n_partitions && 2 * n > n_partitions);
  x = n_partitions - n;

  /* make the map from small integers to partition identifiers */
  partition_map = ALLOC(int, n_partitions);
  for (i = 0; i < 2 * x; i++) {
    partition_map[i] = i;
  }
  for (i = 2 * x; i < n_partitions; i++) {
    partition_map[i] = i + x;
  }

  /* make the map from one node in the binary tree of AND's to the next higher
   * one */
  n_entries = n_partitions * 2 - 1;
  support_info->next_partition = ALLOC(int, n_entries);
  for (i = 0; i < 2 * x; i++) {
    support_info->next_partition[i] = 2 * x + i / 2;
  }
  for (count = n_entries - 1, i = n_entries - 3; i >= 2 * x; i -= 2, count--) {
    support_info->next_partition[i] = count;
    support_info->next_partition[i + 1] = count;
  }
  support_info->next_partition[n_entries - 1] = n_entries - 1;

  /* the hard part is done; simple bookkeeping now */
  support_info->partition_count = ALLOC(int, n_entries);
  for (i = 0; i < n_entries; i++) {
    support_info->partition_count[i] = 0;
  }
  for (i = 0; i < support_info->n_fns; i++) {
    support_info->fns[i]->partition =
        partition_map[support_info->fns[i]->partition];
    support_info->partition_count[support_info->fns[i]->partition]++;
    adj_queue(support_info->queue, (char *)support_info->fns[i]);
  }
  FREE(partition_map);
}

static var_info_t *var_info_free(var_info_t *);

/*
 *----------------------------------------------------------------------
 *
 * smooth_lonely_variables -- EXPORTED ROUTINE
 *
 * smooth (existentially quantify) all variables that appear only once.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */

static void smooth_lonely_variables(support_info,
                                    options) support_info_t *support_info;
prl_options_t *options;
{
  int varid;
  int index;
  bdd_t *tmp;
  int remaining;
  array_t *var_array;

  var_array = array_alloc(bdd_t *, 1);
  for (varid = 0; varid < support_info->n_vars; varid++) {
    if (support_info->vars[varid] == NIL(var_info_t))
      continue;
    remaining = st_count(support_info->vars[varid]->fn_table);
    if (remaining == 1) {
      array_insert(bdd_t *, var_array, 0, support_info->vars[varid]->var);
      index = support_info->vars[varid]->last_index;
      tmp = bdd_smooth(support_info->fns[index]->fn, var_array);
      bdd_free(support_info->fns[index]->fn);
      support_info->fns[index]->fn = tmp;
    }
    if (remaining == 0 || remaining == 1) {
      var_info_free(support_info->vars[varid]);
      support_info->vars[varid] = NIL(var_info_t);
    }
  }
  array_free(var_array);
}

static void fn_info_free(fn_info_t *);
static array_t *smooth_vars_extract(support_info_t *, fn_info_t *, fn_info_t *);

/*
 *----------------------------------------------------------------------
 *
 * do_fn_merging -- INTERNAL ROUTINE
 *
 * AND all functions together and return the result.
 *
 *----------------------------------------------------------------------
 */

static bdd_t *do_fn_merging(support_info, options) support_info_t *support_info;
prl_options_t *options;
{
  int i;
  bdd_t *tmp;
  fn_info_t *fn0, *fn1;
  array_t *smooth_vars;

  if (support_info->n_fns == 0)
    return bdd_one(support_info->manager);

  /* fn1 disappears after being anded to fn0 */
  for (;;) {
    fn0 = (fn_info_t *)get_queue(support_info->queue);
    assert(fn0 != NIL(fn_info_t));
    fn1 = (fn_info_t *)get_queue(support_info->queue);
    if (fn1 == NIL(fn_info_t))
      break;
    smooth_vars = smooth_vars_extract(support_info, fn0, fn1);
    if (array_n(smooth_vars) == 0) {
      tmp = bdd_and(fn0->fn, fn1->fn, 1, 1);
    } else {
      tmp = bdd_and_smooth(fn0->fn, fn1->fn, smooth_vars);
    }
    bdd_free(fn0->fn);
    fn0->fn = tmp;

    /* if fn0 is the last of its partition, promote it in a higher partition */
    support_info->partition_count[fn1->partition]--;
    if (support_info->partition_count[fn0->partition] == 1) {
      support_info->partition_count[fn0->partition]--;
      fn0->partition = support_info->next_partition[fn0->partition];
      support_info->partition_count[fn0->partition]++;
    }
    /* adjust the costs */
    fn0->cost += support_info->n_fns;
    put_queue(support_info->queue, (char *)fn0);

    if (options->verbose >= 3) {
      if (array_n(smooth_vars) > 0) {
        (void)fprintf(misout, "(s%d/#%d->#%d,%d),", array_n(smooth_vars),
                      fn1->fnid, fn0->fnid, bdd_size(fn0->fn));
      } else {
        (void)fprintf(misout, "(#%d->#%d,%d),", fn1->fnid, fn0->fnid,
                      bdd_size(fn0->fn));
      }
      (void)fflush(misout);
    }
    support_info->fns[fn1->fnid] = NIL(fn_info_t);
    fn_info_free(fn1);
    for (i = 0; i < array_n(smooth_vars); i++) {
      tmp = array_fetch(bdd_t *, smooth_vars, i);
      bdd_free(tmp);
    }
    array_free(smooth_vars);
  }
  tmp = bdd_dup(fn0->fn);
  support_info->fns[fn0->fnid] = NIL(fn_info_t);
  fn_info_free(fn0);
  return tmp;
}

/* a variable still active here appears in at least two functions */
/* thus the assertion. If the two functions are fns[index0] and fns[index1] */
/* we can smooth that variable out now */
/* can only disappear if it is in both; some bookkeeping if it is in index1 and
 * not in index0 */

static array_t *smooth_vars_extract(support_info, fn0,
                                    fn1) support_info_t *support_info;
fn_info_t *fn0;
fn_info_t *fn1;
{
  int i;
  char *key;
  int remaining;
  array_t *result;
  int is_in_index0;
  int is_in_index1;
  var_info_t *var_info;

  result = array_alloc(bdd_t *, 0);
  for (i = 0; i < support_info->n_vars; i++) {
    var_info = support_info->vars[i];
    if (var_info == NIL(var_info_t))
      continue;
    remaining = st_count(var_info->fn_table);
    assert(remaining > 1);
    is_in_index1 = var_set_get_elt(fn1->support, i);
    if (!is_in_index1)
      continue;
    is_in_index0 = var_set_get_elt(fn0->support, i);
    if (!is_in_index0) { /* move var i to fn0 */
      var_set_set_elt(fn0->support, i);
      key = (char *)fn1->fnid; /* for 64 bit support */
      st_delete(var_info->fn_table, &key, NIL(char *));
      st_insert(var_info->fn_table, (char *)fn0->fnid, NIL(char));
    } else if (remaining == 2) { /* is in both fns only: can be smoothed out */
      array_insert_last(bdd_t *, result, bdd_dup(var_info->var));
      var_info_free(var_info);
      support_info->vars[i] = NIL(var_info_t);
    } else { /* is in both but somewhere else as well: remove fn1 occurrence */
      key = (char *)fn1->fnid;
      st_delete(var_info->fn_table, &key, NIL(char *));
    }
  }
  return result;
}

static void support_info_free(support_info) support_info_t *support_info;
{
  int i;

  free_queue(support_info->queue);
  for (i = 0; i < support_info->n_vars; i++) {
    assert(support_info->vars[i] == NIL(var_info_t));
  }
  FREE(support_info->vars);
  for (i = 0; i < support_info->n_fns; i++) {
    assert(support_info->fns[i] == NIL(fn_info_t));
  }
  FREE(support_info->fns);
  FREE(support_info->partition_count);
  FREE(support_info->next_partition);
  FREE(support_info);
}

static void fn_info_free(info) fn_info_t *info;
{
  var_set_free(info->support);
  bdd_free(info->fn);
  FREE(info);
}

/* creates an object with a varid, the corresponding BDD and a hash table */
/* containing the indices of the fns having the varid in their support */

static void extract_var_info(support_info,
                             skip_varids) support_info_t *support_info;
st_table *skip_varids;
{
  int i;
  int varid;
  var_info_t *info;

  for (varid = 0; varid < support_info->n_vars; varid++) {
    if (st_lookup(skip_varids, (char *)varid, NIL(char *))) {
      support_info->vars[varid] = NIL(var_info_t);
      continue;
    }
    info = ALLOC(var_info_t, 1);
    info->varid = varid;
    info->var = bdd_get_variable(support_info->manager, varid);
    info->last_index = -1;
    info->fn_table = st_init_table(st_numcmp, st_numhash);
    for (i = 0; i < support_info->n_fns; i++) {
      if (var_set_get_elt(support_info->fns[i]->support, varid)) {
        assert(i == support_info->fns[i]->fnid);
        st_insert(info->fn_table, (char *)i, NIL(char));
        info->last_index = i;
      }
    }
    support_info->vars[varid] = info;
  }
}

static var_info_t *var_info_free(info) var_info_t *info;
{
  bdd_free(info->var);
  st_free_table(info->fn_table);
  FREE(info);
}

#endif /* SIS */
