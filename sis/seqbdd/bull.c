
#ifdef SIS
#include "sis.h"

static bdd_t *bull_cache_lookup();
static int bull_key_cmp();
static int bull_manual_cmp();
static st_table *bull_cache_create();
static int bull_key_hash();
static void bull_cache_free();
static void bull_cache_insert();


st_table *gcache;

range_data_t *bull_alloc_range_data(network, options)
network_t *network;
verif_options_t *options;
{
  bdd_t *bdd;
  array_t *pi_list;
  array_t *po_ordering;
  node_t *n1, *n2;
  int i, id, count;
  char *dummy;
  lsGen gen;
  array_t *remaining_po;
  output_info_t *info = options->output_info;
  range_data_t *data = ALLOC(range_data_t, 1);

  data->type = BULL_METHOD;
  gcache = bull_cache_create(); 

  po_ordering = info->po_ordering;
  info->pi_ordering  = st_init_table(st_ptrcmp, st_ptrhash);
  pi_list = order_nodes(po_ordering, 2);
  count= array_n(pi_list);
  info->pi_ordering= from_array_to_table(pi_list);
  array_free(pi_list);

  remaining_po = get_remaining_po(network, po_ordering);
  pi_list = order_nodes(remaining_po, 1);
  array_free(remaining_po);
  for (i = 0; i< array_n(pi_list); i++){
    n1 = array_fetch(node_t *, pi_list, i);
    if(!st_is_member(info->pi_ordering, (char *) n1)){
      st_insert(info->pi_ordering, (char *) n1, (char *) count);
      count++;
    }
  }
  array_free(pi_list);


  foreach_primary_input(network, gen, n1) {
    if(!st_is_member(info->pi_ordering, (char *) n1)){
      st_insert(info->pi_ordering, (char *) n1, (char *) count);
      count++;
    }
  }

  data->manager = ntbdd_start_manager(st_count(info->pi_ordering));

  for (i = array_n(po_ordering) - 1; i >= 0; i--) {
    n1 = array_fetch(node_t *, po_ordering, i);
    (void) ntbdd_node_to_bdd(n1, data->manager, info->pi_ordering);
  }

 /* create BDD for external output_fns and init_state_fn */
 /* in case of simple range computation, output_node may be NIL(node_t) */
  data->init_state_fn = ntbdd_node_to_bdd(info->init_node, data->manager, info->pi_ordering);
  if (options->does_verification) {
    data->external_outputs= array_alloc(bdd_t *, 0);
    for (i = 0; i < array_n(info->xnor_nodes); i++) {
      n1 = array_fetch(node_t *, info->xnor_nodes, i);
      bdd = ntbdd_node_to_bdd(n1, data->manager, info->pi_ordering);
      array_insert_last(bdd_t *, data->external_outputs, bdd);
    }
  }else{
    data->external_outputs= NIL (array_t);
  }

  data->pi_inputs = array_alloc(bdd_t *, array_n(po_ordering));
  for (i =0 ; i< array_n(po_ordering) ; i++) {
    n1 = array_fetch(node_t *,po_ordering,i);
    n2= network_latch_end(n1);
    assert(n2 != NIL(node_t));
    assert(st_lookup(info->pi_ordering, (char *)n2, (char **) &dummy));
    id= (int) dummy;
    bdd = bdd_get_variable(data->manager, id);
    array_insert_last(bdd_t *, data->pi_inputs , bdd);
  }
  if (options->verbose >= 3) print_node_table(info->pi_ordering);

  return data;
}

void bull_free_range_data(data, options)
range_data_t *data;
verif_options_t *options;
{
  output_info_t *info = options->output_info;

  ntbdd_free_at_node(info->init_node);
  st_free_table(info->pi_ordering);
  /* Set the pi_ordering st_table to NIL so that output_info_free
     doesn't core dump */
  info->pi_ordering = NIL(st_table);
  bull_cache_free(gcache);
  ntbdd_end_manager(data->manager);
  array_free(data->pi_inputs);
  FREE(data);
}


bdd_t *bull_compute_next_states(current_set, data, options)
bdd_t *current_set;
range_data_t *data;
verif_options_t *options;
{
  int i;
  array_t *bdd_list;
  node_t *n1;
  bdd_t *f1, *fc;
  bdd_t *new_current_set;
  array_t  *pi_list;
  bdd_t *total_set;
  st_table *cache;
  output_info_t *info = options->output_info;

  bdd_list = array_alloc(bdd_t *,0);
  for (i =0 ; i<  array_n(info->po_ordering) ; i++) {
    n1 = array_fetch(node_t *, info->po_ordering, i);
    f1 = ntbdd_at_node(n1);
    fc = bdd_cofactor(f1, current_set);
    array_insert_last(bdd_t *,bdd_list,fc);
  }
  pi_list = array_alloc(bdd_t *,0);
  for (i =0 ; i<  array_n(data->pi_inputs) ; i++) {
    f1 = array_fetch(bdd_t *, data->pi_inputs, i);
    array_insert_last(bdd_t *, pi_list, f1);
  }
  if (options->does_verification) 
    total_set= NIL(bdd_t);
  else
    total_set= bdd_dup(data->total_set);
  cache= gcache;
  new_current_set= bull_cofactor(bdd_list, pi_list, data->manager, total_set, cache, info->pi_ordering, options);
  return(new_current_set);
}

bdd_t *bull_compute_reverse_image(next_set, data, options)
bdd_t *next_set;
range_data_t *data;
verif_options_t *options;
{
    return NIL(bdd_t);
}

 /* returns 1 iff everything is OK */
int bull_check_output(current_set, data, output_index, options)
bdd_t *current_set;
range_data_t *data;
int *output_index;
verif_options_t *options;
{
  int i;
  bdd_t *bdd;

  for (i = 0; i < array_n(data->external_outputs); i++) {
    bdd = array_fetch(bdd_t *, data->external_outputs, i);
    if (! bdd_leq(current_set, bdd, 1, 1)) {
      report_inconsistency(current_set, bdd, options->output_info->pi_ordering);
      if (output_index != NIL(int)) {
        *output_index = i;
      }
      return 0;
    }
  }
  return 1;
}

void bull_bdd_sizes(data, fn_size, output_size)
range_data_t *data;
int *fn_size;
int *output_size;
{
  int i;
  bdd_t *fn;
  node_t *node;
  bdd_t *output;
  
  if (fn_size) {
    *fn_size = 0;
    for (i = 0; i < array_n(data->output_fns); i++) {
      node = array_fetch(node_t *, data->output_fns, i);
      fn = ntbdd_at_node(node);
      *fn_size += bdd_size(fn);
    }
  }
  if (output_size) {
    *output_size = 0;
    if (data->external_outputs == NIL (array_t))
      return;
    for (i = 0; i < array_n(data->external_outputs); i++) {
      output = array_fetch(bdd_t *, data->external_outputs, i);
      *output_size += bdd_size(output);
    }
  }
}

bdd_t *bull_cofactor(bdd_list, pi_list, mg, total_set, cache, leaves, options)
array_t *bdd_list;
array_t *pi_list;
bdd_manager *mg;
bdd_t *total_set;
st_table *cache;
st_table *leaves;
verif_options_t *options;
{
  bdd_t *bdd;
  bdd_t *c, *cbar, *left_bdd, *right_bdd;
  bdd_t *new_bdd, *f1;
  array_t *sup_list;
  array_t *left_list, *right_list;
  array_t *left_pi, *right_pi;
  int flag, active;
  array_t *f_list;
  var_set_t *set;
  var_set_t *andset, *nset;
  bdd_t  *tmpbdd;
  int i, j, l, count, andcount;
  bdd_t *rtotal_set, *ltotal_set;
  array_t *oldlist;

  count= 0;
  active = 0;
  flag = -1;

  if (!options->does_verification) {
  if (bdd_is_tautology(total_set,1)){
    bdd_free(total_set);
    bdd = array_fetch(bdd_t *,bdd_list,0);
    f1 = array_fetch(bdd_t *,pi_list,0);
    if (bdd_is_tautology(bdd, 1)){
      tmpbdd= bdd_dup(f1);
    }else{
      tmpbdd= bdd_not(f1);
    }
    for (i= 0 ; i< array_n(bdd_list); i++){
      bdd = array_fetch(bdd_t *,bdd_list,i);
      if (bdd != NIL(bdd_t))
      bdd_free(bdd);
    }
    array_free(bdd_list);
    array_free(pi_list);
    return(tmpbdd);
  }
  }
  new_bdd= bdd_one(mg);
  oldlist = array_alloc(bdd_t *, array_n(bdd_list));
  if ((array_n(bdd_list) > 3) ){
  if ((bdd = bull_cache_lookup(cache, bdd_list, pi_list, mg ,leaves)) != NIL (bdd_t)) {
    return bdd;
  }
  }
  for (i= 0 ; i< array_n(bdd_list); i++){
    bdd = array_fetch(bdd_t *,bdd_list,i);
    array_insert(bdd_t *, oldlist, i, bdd);
    if (bdd == NIL(bdd_t))
      continue;
    if (bdd_is_tautology(bdd, 1)){
      array_insert(bdd_t *, bdd_list, i, NIL (bdd_t));
      f1= array_fetch(bdd_t *, pi_list, i);
      c= bdd_and(f1, new_bdd, 1, 1);
      bdd_free(new_bdd);
      new_bdd= c;
      continue;
    }
    if (bdd_is_tautology(bdd, 0)){
      array_insert(bdd_t *, bdd_list, i, NIL (bdd_t));
      f1= array_fetch(bdd_t *, pi_list, i);
      c= bdd_and(f1, new_bdd, 0, 1);
      bdd_free(new_bdd);
      new_bdd= c;
      continue;
    }
    count++;
  }
  if (count <= 1){
    for (i= 0 ; i< array_n(oldlist); i++){
      bdd = array_fetch(bdd_t *,bdd_list,i);
      if (bdd != NIL(bdd_t))
        bdd_free(bdd);
    }
    array_free(bdd_list);
    array_free(oldlist);
    array_free(pi_list);
    if (!options->does_verification)
      bdd_free(total_set);
    return(new_bdd);
  }
  sup_list = array_alloc(var_set_t *, count);
  andset = var_set_new(bdd_num_vars(mg));
  andset = var_set_not(andset, andset);
  for (i= 0 ; i< array_n(bdd_list); i++){
    bdd = array_fetch(bdd_t *,bdd_list,i);
    if (bdd == NIL(bdd_t))
      continue;
    set= bdd_get_support(bdd);
    nset= var_set_copy(set);
    nset= var_set_and(nset, set, andset);
    var_set_free(andset);
    andset=nset;
    array_insert_last(var_set_t *, sup_list, set);
  }
  f_list= disjoint_support_functions(sup_list);
  for (i= 0 ; i< array_n(sup_list); i++){
    set = array_fetch(var_set_t *,sup_list,i);
    var_set_free(set);
  }
  array_free(sup_list);
  andcount= var_set_n_elts(andset);

  if ((andcount) && (andcount < (count/4)) ){
    j= 0;
    tmpbdd = input_cofactor(bdd_list, pi_list, mg, total_set, cache, leaves, options, andset, j);
  }else{
    tmpbdd= bdd_one(mg);
    for (i= 0 ; i< array_n(f_list); i++){
      set = array_fetch(var_set_t *,f_list,i);
      if ((active= var_set_n_elts(set)) == 1)
        continue;
      if(active==2){
        bdd= range_2_compute(set, bdd_list, pi_list);
        f1 = bdd_and(bdd, tmpbdd, 1, 1);
        bdd_free(bdd);
        bdd_free(tmpbdd);
        tmpbdd= f1;
        continue;
      }
      flag = -1;
      right_list = array_alloc(bdd_t *, 0);
      left_list = array_alloc(bdd_t *, 0);
      right_pi = array_alloc(bdd_t *, 0);
      left_pi = array_alloc(bdd_t *, 0);
      for (j= 0, l=0; j< array_n(bdd_list) ; j++){
        bdd = array_fetch(bdd_t *,bdd_list,j);
        if (bdd == NIL(bdd_t)){
          continue;
        }
        if(var_set_get_elt(set,l)){
          f1= array_fetch(bdd_t *, pi_list, j);
          array_insert_last(bdd_t *, right_pi, f1); 
          array_insert_last(bdd_t *, left_pi, f1); 
          if(flag == -1){ 
            flag = j;
            c = bdd;
            cbar= bdd_not(c);
            array_insert_last(bdd_t *, right_list, bdd_one(mg));
            array_insert_last(bdd_t *, left_list, bdd_zero(mg));
            if (!options->does_verification){
              rtotal_set= bdd_cofactor(total_set, f1);
              ltotal_set= bdd_cofactor(total_set, bdd_not(f1));
            }
          }else{
            array_insert_last(bdd_t *, right_list, bdd_cofactor(bdd,c));
            array_insert_last(bdd_t *, left_list, bdd_cofactor(bdd,cbar));
          }
        }
        l++;
      }
      bdd_free(cbar);
      right_bdd= bull_cofactor(right_list, right_pi, mg, rtotal_set, cache, leaves,options);
      left_bdd= bull_cofactor(left_list, left_pi, mg, ltotal_set, cache,leaves,options);

      bdd = bdd_or(right_bdd, left_bdd, 1, 1);
      bdd_free(right_bdd);
      bdd_free(left_bdd);
      f1 = bdd_and(bdd, tmpbdd, 1, 1);
      bdd_free(bdd);
      bdd_free(tmpbdd);
      tmpbdd= f1;
    }
  }

  for (i= 0 ; i< array_n(f_list); i++){
    set = array_fetch(var_set_t *,f_list,i);
    var_set_free(set);
  }
  array_free(f_list);

  c= bdd_and(new_bdd, tmpbdd, 1, 1);
  bdd_free(new_bdd);
  bdd_free(tmpbdd);
  if ((array_n(oldlist) > 3) ){
    bull_cache_insert(cache, oldlist, c, pi_list); 
  }else{
    for (i= 0 ; i< array_n(oldlist); i++){
      bdd = array_fetch(bdd_t *,oldlist,i);
      if (bdd != NIL(bdd_t))
        bdd_free(bdd);
    }
    array_free(pi_list);
    array_free(oldlist);
  }
  var_set_free(andset);
  array_free(bdd_list);
  if (!options->does_verification)
    bdd_free(total_set);
  return(c);
}

static bdd_t *bull_cache_lookup(cache, bdd_list, pi_list, manager , leaves)
st_table *cache;
array_t *bdd_list;
array_t *pi_list;
bdd_manager *manager;
st_table *leaves;
{
  int i;
  bull_key_t key;
  bull_value_t *value;
  bdd_t *result, *new_result;
  bdd_t *f0, *f1, *inv;
  bdd_t *var_bar;
  bdd_t *c1;

  key.fns = bdd_list;
  if (! st_lookup(cache, (char *) &key, (char **) &value)) return NIL(bdd_t);
  result = bdd_substitute(value->range, value->ins, pi_list);
  for (i=0 ; i < array_n(bdd_list) ; i++){
    f0 = array_fetch(bdd_t *, value->fns, i);
    f1 = array_fetch(bdd_t *, bdd_list, i);
    c1 = array_fetch(bdd_t *, pi_list, i);
    if (bdd_equal(f0, f1))
       continue;
    inv = bdd_not(f1);
    bdd_free(inv);
    var_bar = bdd_not(c1);
    new_result= bdd_compose(result, c1, var_bar);
    bdd_free(result);
    result= new_result;
    bdd_free(var_bar);
  }
  return result;
}

/* duplicate everything so that can free everything upon exit without trouble */
 /* I wish we had a C garbage collector!!! */
static void bull_cache_insert(cache, bdd_list, range, pi_list)
st_table *cache;
array_t *bdd_list;
bdd_t *range;
array_t *pi_list;
{
  bull_key_t *key = ALLOC(bull_key_t, 1);
  bull_value_t *value = ALLOC(bull_value_t, 1);

  value->ins = pi_list;
  key->fns = bdd_list;
  value->fns = key->fns;
  value->range = bdd_dup(range);
  st_insert(cache, (char *) key, (char *) value);
}

static st_table *bull_cache_create()
{
  st_table *cache = st_init_table(bull_key_cmp, bull_key_hash);
  return cache;
}

 /* do not free value->fns: it is a pointer to key->fns */
static void bull_cache_free(cache)
st_table *cache;
{
  int i;
  bdd_t *f;
  st_generator *gen;
  bull_key_t *key;
  bull_value_t *value;

  st_foreach_item(cache, gen, (char **) &key, (char **) &value) {
    for (i = 0; i < array_n(key->fns); i++) {
      f = array_fetch(bdd_t *, key->fns, i);
      if (f != NIL(bdd_t))
      bdd_free(f);
    }
    array_free(key->fns);
    array_free(value->ins);
    FREE(key);
    bdd_free(value->range);
    FREE(value);
  }
  st_free_table(cache);
}

 /* should be 0 if match */
static int bull_key_cmp(obj1, obj2)
char *obj1;
char *obj2;
{
  int i;
  bdd_t *f1, *f2, *inv;
  array_t *array1 = ((bull_key_t *) obj1)->fns;
  array_t *array2 = ((bull_key_t *) obj2)->fns;
  
  if (array_n(array1) != array_n(array2))
    return 1;
  for (i = 0; i < array_n(array1); i++) {
    f1 = array_fetch(bdd_t *, array1, i);
    f2 = array_fetch(bdd_t *, array2, i);
    if (f1 == f2) continue;
    if (! bdd_equal(f1, f2)) {
      inv = bdd_not(f1);
      if (! bdd_equal(inv, f2)) {
      bdd_free(inv);
      return 1;
      }
      bdd_free(inv);
    }
  }
  return 0;
}

static int bull_key_hash(obj, modulus)
char *obj;
int modulus;
{
  int i;
  bdd_t *fn;
  array_t *array = ((bull_key_t *) obj)->fns;
  register unsigned int result = 0;

  for (i = 0; i < array_n(array); i++) {
    fn = array_fetch(bdd_t *, array, i);
    if (fn == NIL(bdd_t)) continue;
    result <<= 1;
    result += (int) bdd_top_var_id(fn);
  }
  return result % modulus;
}
#endif /* SIS */
