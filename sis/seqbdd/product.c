
 /* file %M% release %I% */
 /* last modified: %G% at %U% */
#ifdef SIS
#include "sis.h"
#include "prioqueue.h"


 /* types used in PRODUCT for smoothing as soon as possible */

typedef struct {
  int varid;
  bdd_t *var;			   /* result of bdd_get_variable(mgr, varid) */
  st_table *fn_table;		   /* list of indices of fns depending on that variable in fns[] */
  int last_index;		   /* the most recent fn index depending on this var */
} var_info_t;

typedef struct {
  int fnid;			   /* small integer; unique identifier for the function */
  bdd_t *fn;			   /* accumulation of AND's of functions */
  int cost;			   /* simply an index for static merging; */
				   /* size of bdd for dynamic merging */
  var_set_t *support;		   /* support of that function */
  int partition;		   /* to which partition the fn belongs */
} fn_info_t;


 /* should avoid smoothing out next_state_vars ... */

typedef struct {
  int n_fns;
  fn_info_t **fns;
  int n_vars;
  var_info_t **vars;
  queue_t *queue;		   /* priority queue where to store the functions by size */
  bdd_manager *manager;
  int n_partitions;
  int *partition_count;		   /* number of fns alive in each partition */
  int *next_partition;		   /* map to tell in which partition to go when current one is empty */ 
} support_info_t;


 /* result < 0 means obj1 higher priority than obj2 (obj1 comes first) */
static int fn_info_cmp(obj1, obj2)
char *obj1;
char *obj2;
{
  int diff;
  fn_info_t *info1 = (fn_info_t *) obj1;
  fn_info_t *info2 = (fn_info_t *) obj2;
  diff = info1->partition - info2->partition;
  if (diff == 0) diff = info1->cost - info2->cost;
  return diff;
}

static void fn_info_print(obj)
char *obj;
{
  fn_info_t *info = (fn_info_t *) obj;
  (void) fprintf(misout, "fn %d", info->fnid);
}

static array_t *smooth_vars_extract();
static bdd_t *do_fn_merging();
static int fn_info_cmp();
static st_table *get_varids_in_table();
static support_info_t *support_info_extract();
static void *var_info_free();
static void extract_fn_info();
static void extract_var_info();
static void fn_info_free();
static void fn_info_print();
static void initialize_partition_info();
static void smooth_lonely_variables();
static void support_info_free();



 /* EXTERNAL INTERFACE */

 /* keep everything separate: transition relation and the output fn in case of verif */

 /* ARGSUSED */
range_data_t *product_alloc_range_data(network, options)
network_t *network;
verif_options_t *options;
{
  int i;
  node_t *node;
  bdd_t *output;
  st_table *input_to_output_table;
  output_info_t *info = options->output_info;
  range_data_t *data = ALLOC(range_data_t, 1);

  data->type = PRODUCT_METHOD;

 /* create a BDD manager */
  if (options->verbose >= 3) print_node_table(info->pi_ordering);
  data->manager = ntbdd_start_manager(st_count(info->pi_ordering));

 /* create BDD for external output_fn and init_state_fn */
  data->init_state_fn = ntbdd_node_to_bdd(info->init_node, data->manager, info->pi_ordering);
  data->external_outputs = array_alloc(bdd_t *, 0);
  if (options->does_verification) {
    for (i = 0; i < array_n(info->xnor_nodes); i++) {
      node = array_fetch(node_t *, info->xnor_nodes, i);
      output = ntbdd_node_to_bdd(node, data->manager, info->pi_ordering);
      array_insert_last(bdd_t *, data->external_outputs, output);
    }
  }

 /* consistency fn and the like */
  data->consistency_fn = 0;
  data->transition_outputs = array_alloc(bdd_t *, 0);
  for (i = 0; i < array_n(info->transition_nodes); i++) {
    node = array_fetch(node_t *, info->transition_nodes, i);
    output = ntbdd_node_to_bdd(node, data->manager, info->pi_ordering);
    array_insert_last(bdd_t *, data->transition_outputs, output);
  }
  data->smoothing_inputs = bdd_extract_var_array(data->manager, info->org_pi, info->pi_ordering);
  
 /* create and save two arrays of bdd variables in order: yi's and their corresponding xi's */
  input_to_output_table = extract_input_to_output_table(info->org_pi, info->new_pi, info->po_ordering, network);
  data->input_vars = extract_state_input_vars(data->manager, info->pi_ordering, input_to_output_table);
  data->output_vars = extract_state_output_vars(data->manager, info->pi_ordering, input_to_output_table);

 /* only present state variables: used for counting the onset of reached set */
  data->pi_inputs = data->input_vars;

 /* free space allocated here */
  st_free_table(input_to_output_table);

  return data;
}

void product_free_range_data(data, options)
range_data_t *data;
verif_options_t *options;
{
  int i;
  node_t *node;
  output_info_t *info = options->output_info;

 /* do not free data->pi_inputs (== data->input_vars) */
  array_free(data->input_vars);
  array_free(data->output_vars);
  array_free(data->smoothing_inputs);
  if (options->does_verification) {
    for (i = 0; i < array_n(info->xnor_nodes); i++) {
      node = array_fetch(node_t *, info->xnor_nodes, i);
      ntbdd_free_at_node(node);
    }
  }
  array_free(data->external_outputs);
  ntbdd_free_at_node(info->init_node);
  for (i = 0; i < array_n(info->transition_nodes); i++) {
    node = array_fetch(node_t *, info->transition_nodes, i);
    ntbdd_free_at_node(node);
  }
  array_free(data->transition_outputs);
  ntbdd_end_manager(data->manager);
  FREE(data);
}

 /* first cofactor all the transition outputs with the current set */
 /* */
 /* do the AND by building a binary trees of those  */
 /* keep track of variables that can be removed immediately */


bdd_t *product_compute_next_states(current_set, data, options)
bdd_t *current_set;
range_data_t *data;
verif_options_t *options;
{
  bdd_t *result_as_output = bdd_incr_and_smooth(data->transition_outputs, current_set, data->output_vars, options);
  bdd_t *new_current_set = bdd_substitute(result_as_output, data->output_vars, data->input_vars);

  if (options->verbose >= 3) {
    (void) fprintf(misout, "(%d->%d)", bdd_size(result_as_output), bdd_size(new_current_set));
    (void) fprintf(sisout, "\n");
    (void) fflush(misout);    
  }
  bdd_free(result_as_output);
  return new_current_set;
}

bdd_t *product_compute_reverse_image(next_set, data, options)
bdd_t *next_set;
range_data_t *data;
verif_options_t *options;
{
  int i;
  node_t *node;
  array_t *ps_and_pi_array;
  bdd_t *result;
  bdd_t *next_set_as_next_state_vars;

  next_set_as_next_state_vars =
       bdd_substitute(next_set, data->input_vars, data->output_vars);

  ps_and_pi_array = array_alloc(bdd_t *, 0);
  for (i = array_n(options->output_info->org_pi); i-- > 0; ){
      node = array_fetch(node_t *, options->output_info->org_pi, i);
      array_insert_last(bdd_t *, ps_and_pi_array, ntbdd_at_node(node));
  }

  result = bdd_incr_and_smooth(data->transition_outputs, next_set_as_next_state_vars, ps_and_pi_array, options);

  bdd_free(next_set_as_next_state_vars);
  array_free(ps_and_pi_array);
  return result;
}


 /* use this function as a general interface for this operator */

bdd_t *bdd_incr_and_smooth(transition_outputs, current_set, keep_vars, options)
array_t *transition_outputs;
bdd_t *current_set;
array_t *keep_vars;
verif_options_t *options;
{
  int i;
  int n_fns;
  bdd_t *tmp;
  bdd_t **fns;
  bdd_t *resulting_product;
  support_info_t *support_info;

  if (options->verbose >= 3) {
    (void) fprintf(misout, "["); (void) fflush(misout);
  }
  n_fns = array_n(transition_outputs);
  fns = ALLOC(bdd_t *, n_fns);
  for (i = 0; i < n_fns; i++) {
    tmp = array_fetch(bdd_t *, transition_outputs, i);
    fns[i] = bdd_cofactor(tmp, current_set);
    if (options->verbose >= 3) {
      (void) fprintf(misout, "(#%d->%d),", i, bdd_size(tmp));
      (void) fprintf(misout, "(#%d->%d),", i, bdd_size(fns[i]));
      (void) fflush(misout);    
    }
  }
  if (options->verbose >= 3) {
    (void) fprintf(sisout, "]\n");
  }
  support_info = support_info_extract(keep_vars, n_fns, fns, options);
  FREE(fns);
  resulting_product = do_fn_merging(support_info, options);
  support_info_free(support_info);
  return resulting_product;
}

int product_check_output(current_set, data, output_index, options)
bdd_t *current_set;
range_data_t *data;
int *output_index;
verif_options_t *options;
{
  int i;
  bdd_t *output;

  for (i = 0; i < array_n(data->external_outputs); i++) {
    output = array_fetch(bdd_t *, data->external_outputs, i);
    if (! bdd_leq(current_set, output, 1, 1)) {
      report_inconsistency(current_set, output, options->output_info->pi_ordering);
      if (output_index != NIL(int)) {
        *output_index = i;
      }
      return 0;
    }
  }
  return 1;
}

void product_bdd_sizes(data, fn_size, output_size)
range_data_t *data;
int *fn_size;
int *output_size;
{
  int i;
  bdd_t *output;

  if (fn_size) {
    *fn_size = 0;
    for (i = 0; i < array_n(data->transition_outputs); i++) {
      output = array_fetch(bdd_t *, data->transition_outputs, i);
      *fn_size += bdd_size(output);
    }
  }
  if (output_size) {
    *output_size = 0;
    for (i = 0; i < array_n(data->external_outputs); i++) {
      output = array_fetch(bdd_t *, data->external_outputs, i);
      *output_size += bdd_size(output);
    }
  }
}


 /* INTERNAL INTERFACE */

static support_info_t *support_info_extract(keep_vars, n_fns, fns, options)
array_t *keep_vars;
int n_fns;
bdd_t **fns;
verif_options_t *options;
{
  int i;
  st_table *skip_varids;
  support_info_t *support_info = ALLOC(support_info_t, 1);

  support_info->n_fns = n_fns;
  if (n_fns == 0) return support_info;

 /* allocate a priority queue for storing the functions */
  support_info->queue = init_queue(support_info->n_fns, fn_info_cmp, fn_info_print);

 /* compute the info related to each fn */
  support_info->fns = ALLOC(fn_info_t *, support_info->n_fns);
  extract_fn_info(support_info, fns, options);

 /* get a hash table containing the varids of all the next state variables */
  skip_varids = get_varids_in_table(keep_vars);

 /* compute the info related to each variable; skip over next_state_vars */
  support_info->manager = bdd_get_manager(fns[0]);
  support_info->n_vars = bdd_num_vars(support_info->manager);
  support_info->vars = ALLOC(var_info_t *, support_info->n_vars);
  extract_var_info(support_info, skip_varids);

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

static void support_info_free(support_info)
support_info_t *support_info;
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

static void extract_fn_info(support_info, fns, options)
support_info_t *support_info;
bdd_t **fns;
verif_options_t *options;
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
    put_queue(support_info->queue, (char *) info);
    support_info->fns[i] = info;
  }
}

static void fn_info_free(info)
fn_info_t *info;
{
  var_set_free(info->support);
  bdd_free(info->fn);
  FREE(info);
}

 /* creates an object with a varid, the corresponding BDD and a hash table */
 /* containing the indices of the fns having the varid in their support */

static void extract_var_info(support_info, skip_varids)
support_info_t *support_info;
st_table *skip_varids;
{
  int i;
  int varid;
  var_info_t *info;

  for (varid = 0; varid < support_info->n_vars; varid++) {
    if (st_lookup(skip_varids, (char *) varid, NIL(char *))) {
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
	st_insert(info->fn_table, (char *) i, NIL(char));
	info->last_index = i;
      }
    }
    support_info->vars[varid] = info;
  }
}

static void *var_info_free(info)
var_info_t *info;
{
  bdd_free(info->var);
  st_free_table(info->fn_table);
  FREE(info);
}


 /* smooth all variables that appear only once */

static void smooth_lonely_variables(support_info, options)
support_info_t *support_info;
verif_options_t *options;
{
  int varid;
  int index;
  bdd_t *tmp;
  int remaining;
  array_t *var_array;

  var_array = array_alloc(bdd_t *, 1);
  for (varid = 0; varid < support_info->n_vars; varid++) {
    if (support_info->vars[varid] == NIL(var_info_t)) continue;
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

 /* AND all functions together and return the result */

static bdd_t *do_fn_merging(support_info, options)
support_info_t *support_info;
verif_options_t *options;
{
  int i;
  bdd_t *tmp;
  fn_info_t *fn0, *fn1;
  array_t *smooth_vars;

 /* fn1 disappears after being anded to fn0 */
  for (;;) {
    fn0 = (fn_info_t *) get_queue(support_info->queue);
    assert(fn0 != NIL(fn_info_t));
    fn1 = (fn_info_t *) get_queue(support_info->queue);
    if (fn1 == NIL(fn_info_t)) break;
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
    put_queue(support_info->queue, (char *) fn0);

    if (options->verbose >= 3) {
      if (array_n(smooth_vars) > 0) {
	(void) fprintf(misout, "(s%d/#%d->#%d,%d),", array_n(smooth_vars), fn1->fnid, fn0->fnid, bdd_size(fn0->fn));
      } else {
	(void) fprintf(misout, "(#%d->#%d,%d),", fn1->fnid, fn0->fnid, bdd_size(fn0->fn));
      }
      (void) fflush(misout);    
    }
    support_info->fns[fn1->fnid] = NIL(fn_info_t);
    fn_info_free(fn1);
    for (i = 0; i < array_n(smooth_vars); i++) {
      tmp = array_fetch(bdd_t *, smooth_vars, i);
      bdd_free(tmp);
    }
    array_free(smooth_vars);
  }
  if (options->verbose >= 3) {
    (void) fprintf(sisout, "\n");
  }
  tmp = bdd_dup(fn0->fn);
  support_info->fns[fn0->fnid] = NIL(fn_info_t);
  fn_info_free(fn0);
  return tmp;
}


 /* a variable still active here appears in at least two functions */
 /* thus the assertion. If the two functions are fns[index0] and fns[index1] */
 /* we can smooth that variable out now */
 /* can only disappear if it is in both; some bookkeeping if it is in index1 and not in index0 */

static array_t *smooth_vars_extract(support_info, fn0, fn1)
support_info_t *support_info;
fn_info_t *fn0;
fn_info_t *fn1;
{
  int i;
  int remaining;
  array_t *result;
  int is_in_index0;
  int is_in_index1;
  var_info_t *var_info;
  
  result = array_alloc(bdd_t *, 0);
  for (i = 0; i < support_info->n_vars; i++) {
    var_info = support_info->vars[i];
    if (var_info == NIL(var_info_t)) continue;
    remaining = st_count(var_info->fn_table);
    assert(remaining > 1);
    is_in_index1 = var_set_get_elt(fn1->support, i);
    if (! is_in_index1) continue;
    is_in_index0 = var_set_get_elt(fn0->support, i);
    if (! is_in_index0) {					       /* move var i to fn0 */
      var_set_set_elt(fn0->support, i);
      st_delete_int(var_info->fn_table, &fn1->fnid, NIL(char *)); 
      st_insert(var_info->fn_table, (char *) fn0->fnid, NIL(char));
    } else if (remaining == 2) {				       /* is in both fns only: can be smoothed out */
      array_insert_last(bdd_t *, result, bdd_dup(var_info->var));
      var_info_free(var_info);
      support_info->vars[i] = NIL(var_info_t);
    } else { /* is in both but somewhere else as well: remove fn1 occurrence */
      st_delete_int(var_info->fn_table, &fn1->fnid, NIL(char *));
    }
  }
  return result;
}

 /* extract varids from array of bdd_t *'s and put in a hash table */
static st_table *get_varids_in_table(bdd_array)
array_t *bdd_array;
{
  int i;
  int varid;
  st_table *result = st_init_table(st_numcmp, st_numhash);
  array_t *var_array = bdd_get_varids(bdd_array);

  for (i = 0; i < array_n(var_array); i++) {
    varid = array_fetch(int, var_array, i);
    st_insert(result, (char *) varid, NIL(char));
  }
  array_free(var_array);
  return result;
}

 /* valid for all methods */
 /* compute n such that n = 2^p <= n_partitions < 2^{p+1} */
 /* compute x such that x + n = n_partitions */
 /* x is guaranteed to be such that 2 * x < n_partitions */

static void initialize_partition_info(support_info)
support_info_t *support_info;
{
  int n_partitions;
  int partition;
  int i, n, x;
  int count;
  int n_entries;
  int *partition_map;
  st_table *table =  st_init_table(st_numcmp, st_numhash);

 /* first make the partition identifiers consecutive integers from 0 to n_entries - 1 */
  n_partitions = 0;
  for (i = 0; i < support_info->n_fns; i++) {
    if (st_lookup(table, (char *) support_info->fns[i]->partition, (char **) &partition)) {
      support_info->fns[i]->partition = partition;
    } else {
      st_insert(table, (char *) support_info->fns[i]->partition, (char *) n_partitions);
      support_info->fns[i]->partition = n_partitions++;
    }
  }
  st_free_table(table);

 /* just a consistency check */
  assert(n_partitions == support_info->n_partitions);

 /* compute n=2^p such that 2^p <= n_partitions < 2^{p+1} */
 /* and compute x = n_partitions - 2^p */
  for (n = 1; n <= n_partitions; n <<= 1) ;
  if (n > n_partitions) n >>= 1;
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

 /* make the map from one node in the binary tree of AND's to the next higher one */
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
    support_info->fns[i]->partition = partition_map[support_info->fns[i]->partition];
    support_info->partition_count[support_info->fns[i]->partition]++;
    adj_queue(support_info->queue, (char *) support_info->fns[i]);
  }
  FREE(partition_map);
}
#endif /* SIS */
