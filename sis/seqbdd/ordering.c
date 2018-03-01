/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/seqbdd/ordering.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:54 $
 *
 */
 /* file %M% release %I% */
 /* last modified: %G% at %U% */
#ifdef SIS
#include "sis.h"

 /* this file implements an exact solution to the following problem */
 /* given A1,...,An n subsets of some finite set S */
 /* find an ordering of {1,...,n} such that */
 /* if s_i = |\cup{1\leq j\leq i} A_i | */
 /* \sum_{1\leq i \leq n} s_i   is minimized */
 /* This is used for ordering output fns in BDD range computation heuristics */

 /* the algorithm is exponential in the worst-case */
 /* it uses a branch and bound heuristic */
 /* as well as the fact that if A_i included in A_j  */
 /* we can always suppose that A_i appears before A_j */
 /* In fact, more strongly that that: if A_i is included in A_j */
 /* except for elements that only appear in A_i and in no other set */
 /* then A_i always appear before A_j if #singletons(A_i) \leq #singletons(A_j) */

 /* key into the cache: should use var_set_t compare */
 /* describes as a bit string the sets scheduled so far */
typedef struct {
  var_set_t *placed_so_far;
} set_key_t;

 /* value stored in the cache */
 /* the best next set index, and the cost at that point */
typedef struct {
  int index;
  int cost;
} set_value_t;

 /* should not overflow too easily but should be large enough */
#define LARGE_NUMBER ((int) 0x1fffffff)

 /* information associated with dominators used in branch & bound */
 /* a dominator is a set that does not contain any other */
 /* (not exactly true; for exact definition, see compare routine) */
 /* the size of the dominator is the number of elts it contains */
 /* that are not in the union of the sets visited so far */
typedef struct {
  int index;		     /* index of the set (if A5, index is 5) */
  int bound;		     /* lower bound of the cost of choosing this index */
  int cost;		     /* real cost, after recursive computation */
  int size;		     /* size of the index, minus all variables already there */
} dominator_t;


static array_t *extract_dominators();
static int arg_min_remaining_size();
static int dominator_cmp();
static int do_find_best_set_order();
static int set_is_less_than();
static int set_key_cmp();
static int set_key_hash();
static var_set_t *extract_uncovered_variables();
static void compute_bounds();
static void extract_best_order_from_cache();
static void print_int_array();
static void print_set_info();
static void set_cache_free();
static int compute_order_cost();

 /* EXTERNAL INTERFACE */

 /* returns an array of size n_sets representing the optimal ordering of the sets */

static int global_verbose;

array_t *find_best_set_order(info, options)
set_info_t *info;
verif_options_t *options;
{
  int cost;
  set_key_t *key = ALLOC(set_key_t, 1);
  array_t *result = array_alloc(int, 0);
  st_table *cache = st_init_table(set_key_cmp, set_key_hash);
  
  global_verbose = options->verbose;
  if (options->verbose >= 4) print_set_info(info);
  key->placed_so_far = var_set_new(info->n_sets); /* automatically initialized to 0 */
  if (options->ordering_depth >= 0) {
    cost = do_find_best_set_order(info, key, cache, options->ordering_depth, LARGE_NUMBER);
    extract_best_order_from_cache(info, cache, key, result);
    assert(cost == compute_order_cost(info, result));
  } else {
    array_free(result);
    result = find_greedy_set_order(info);
    cost = compute_order_cost(info, result);
  }
 /* TODO: compute the cost again; check with evaluated cost above */
  if (options->verbose >= 2) {
    printf("ordering cost is: %d\n", cost);
    if (options->verbose >= 3) print_int_array(result);
  }
  set_cache_free(cache);
  return result;
}


 /* INTERNAL INTERFACE */

 /* returns the cost of the solution so far */
 /* the resulting order can be derived from the cache */
 /* The key is newly allocated when it comes in */
 /* it is either kept in the cache or freed by this routine */
 /* info is read-only */

static int do_find_best_set_order(info, key, cache, depth, allocated)
set_info_t *info;
set_key_t *key;
st_table *cache;
int depth;
int allocated; /* for bounding the search: if allocated < 0, can kill this subtree */
{
  int i;
  int local_cost;
  set_key_t *new_key;
  set_value_t *value;
  var_set_t *remaining_vars, *dead_vars;
  array_t *dominators;
  dominator_t d;
  dominator_t *dom;
  int n_remaining_sets;
  var_set_t *tmp = NIL(var_set_t);
  
  n_remaining_sets = info->n_sets - var_set_n_elts(key->placed_so_far);
  if (n_remaining_sets == 0) {
    var_set_free(key->placed_so_far);
    FREE(key);
    return 0;
  }
  if (allocated < 0) {
    var_set_free(key->placed_so_far);
    FREE(key);
    return LARGE_NUMBER;
  }
  if (st_lookup(cache, (char *) key, (char **) &value)) {
    var_set_free(key->placed_so_far);
    FREE(key);
    return value->cost;
  }
 /* variables still active (i.e. complement of union of sets in key) */
  remaining_vars = extract_uncovered_variables(info, key);
  dead_vars = var_set_copy(remaining_vars);
  (void) var_set_not(dead_vars, dead_vars);

  if (depth <= 0) {
    dominators = array_alloc(dominator_t, 0);
 /* get the first remaining set of minimum size relative to remaining vars */
    d.bound = 0;
    d.cost = LARGE_NUMBER;
    d.index = arg_min_remaining_size(info, key->placed_so_far, dead_vars, &(d.size));
    array_insert_last(dominator_t, dominators, d);
  } else {
    dominators = extract_dominators(info, key->placed_so_far, remaining_vars);
    if (array_n(dominators) > 1) {
      array_sort(dominators, dominator_cmp);
      compute_bounds(info, key->placed_so_far, remaining_vars, dominators);
    }
  }
  assert(array_n(dominators) > 0);
 /* compute the best solution among all alternatives */
  tmp = var_set_new(info->n_vars);
  value = ALLOC(set_value_t, 1);
  value->index = -1;
  value->cost = LARGE_NUMBER;
  for (i = 0; i < array_n(dominators); i++) {
    dom = array_fetch_p(dominator_t, dominators, i);
    if (dom->bound >= value->cost) continue;

 /* to compute the cost at this level: number of 1's here */
 /* times how many times they are going to appear: # remaining_sets */
    (void) var_set_and(tmp, info->sets[dom->index], remaining_vars);
    local_cost = var_set_n_elts(tmp) * n_remaining_sets;

    allocated = value->cost - local_cost;
    new_key = ALLOC(set_key_t, 1);
    new_key->placed_so_far = var_set_copy(key->placed_so_far);
    var_set_set_elt(new_key->placed_so_far, dom->index);
    dom->cost = local_cost + do_find_best_set_order(info, new_key, cache, depth - 1, allocated);
    if (value->cost > dom->cost) {
      value->cost = dom->cost;
      value->index = dom->index;
    }
  }
  assert(value->index != -1);
  assert(! var_set_get_elt(key->placed_so_far, value->index));
  st_insert(cache, (char *) key, (char *) value);
  array_free(dominators);
  var_set_free(remaining_vars);
  var_set_free(dead_vars);
  if (tmp) var_set_free(tmp);
  return value->cost;
}

static int dominator_cmp(obj1, obj2)
char *obj1;
char *obj2;
{
  dominator_t *dom1 = (dominator_t *) obj1;
  dominator_t *dom2 = (dominator_t *) obj2;

  return dom1->size - dom2->size;
}


static int set_key_hash(k, modulus)
char *k;
int modulus;
{
  register set_key_t *key = (set_key_t *) k;
  register unsigned int hash = var_set_hash(key->placed_so_far);
  return hash % modulus;
}

 /* returns 0 iff equal */
static int set_key_cmp(k1, k2)
char *k1;
char *k2;
{
  register set_key_t *key1 = (set_key_t *) k1;
  register set_key_t *key2 = (set_key_t *) k2;

  return (! var_set_equal(key1->placed_so_far, key2->placed_so_far));
}

 /* frees keys and values */

static void set_cache_free(cache)
st_table *cache;
{
  st_generator *gen;
  set_key_t *key;
  set_value_t *value;
    
  st_foreach_item(cache, gen, (char **) &key, (char **) &value) {
    var_set_free(key->placed_so_far);
    FREE(key);
    FREE(value);
  }
  st_free_table(cache);
}

 /* check that we generate a permutation of 0->n_sets-1 */
 /* if not in hash table: means all remaining ones are equivalent */

static void extract_best_order_from_cache(info, cache, first_key, result)
set_info_t *info;
st_table *cache;
set_key_t *first_key;
array_t *result;
{
  int i, j;
  set_key_t key;
  set_value_t *value;

  key.placed_so_far = var_set_copy(first_key->placed_so_far);
  assert(var_set_is_empty(key.placed_so_far));
  for (i = 0; i < info->n_sets; i++) {
    if (! st_lookup(cache, (char *) &key, (char **) &value)) break;
    assert(value->index >= 0 && value->index < info->n_sets);
    assert(! var_set_get_elt(key.placed_so_far, value->index));
    array_insert(int, result, i, value->index);
    var_set_set_elt(key.placed_so_far, value->index);
  }
  if (i < info->n_sets) {
    for (j = 0; j < info->n_sets; j++) {
      if (var_set_get_elt(key.placed_so_far, j)) continue;
      array_insert(int, result, i, j);
      i++;
    }
  }
  var_set_free(key.placed_so_far);
}

 /* try to compute a good lower bound of cost of solution */
 /* that would try with each separate dominator */
 /* mask: union of variables still to be covered */
static void compute_bounds(info, placed_so_far, mask, dominators)
set_info_t *info;
var_set_t *placed_so_far;
var_set_t *mask;
array_t *dominators;
{
  int i, j;
  dominator_t *dom;
  var_set_t *set = var_set_new(info->n_vars);
  var_set_t *tmp = var_set_new(info->n_vars);
  int n_remaining = info->n_sets - var_set_n_elts(placed_so_far);

  for (i = 0; i < array_n(dominators); i++) {
    dom = array_fetch_p(dominator_t, dominators, i);
    dom->bound = dom->size * n_remaining;
    var_set_clear(set);
    for (j = 0; j < info->n_sets; j++) {
      if (j == dom->index) continue;
      if (var_set_get_elt(placed_so_far, j)) continue;
      (void) var_set_or(set, set, info->sets[j]);
    }
    (void) var_set_and(set, set, mask);
    (void) var_set_not(tmp, info->sets[dom->index]);
    (void) var_set_and(set, set, tmp);
    dom->bound += var_set_n_elts(set);
  }
  var_set_free(set);
  var_set_free(tmp);
}

 /* put in an dominator_t array_t info about the dominator sets */
 /* after we have eliminated the placed_so_far */
 /* (mask is complement of union of placed_so_far) */
 /* a dominator is a set that does not contain any other */
 /* we also need the size of each dominator (not counting the elts in mask) */
 /* If there is no dominator, it means that all sets are equivalent (all equal) */
 /* with respect to mask: in that case, can conclude immediately */
static array_t *extract_dominators(info, placed_so_far, mask)
set_info_t *info;
var_set_t *placed_so_far;
var_set_t *mask;
{
  int i, j;
  int is_dominator;
  dominator_t dom;
  array_t *result = array_alloc(dominator_t, 0);
  var_set_t *dominators = var_set_new(info->n_sets);
  var_set_t *tmp;

  for (i = 0; i < info->n_sets; i++) {
    if (var_set_get_elt(placed_so_far, i)) continue;
    is_dominator = 1;
    for (j = 0; j < info->n_sets; j++) {
      if (j == i) continue;
      if (var_set_get_elt(placed_so_far, j)) continue;
      if (set_is_less_than(info->sets[j], info->sets[i], mask, j - i)) {
	is_dominator = 0;
	break;
      }
    }
    if (is_dominator) var_set_set_elt(dominators, i);
  }
  tmp = var_set_new(info->n_vars);
  for (i = 0; i < info->n_sets; i++) {
    if (! var_set_get_elt(dominators, i)) continue;
    (void) var_set_and(tmp, mask, info->sets[i]);
    dom.index = i;
    dom.size = var_set_n_elts(tmp);
    dom.cost = LARGE_NUMBER;
    dom.bound = 0;
    array_insert_last(dominator_t, result, dom);
  }
  var_set_free(dominators);
  var_set_free(tmp);
  return result;
}


 /* check whether set1 inter set2 is equal to set1 (modulo mask) */
 /* should be strictly less than, thus check for equality */
 /* if equal, use diff to make the difference */
static int set_is_less_than(set1, set2, mask, diff)
var_set_t *set1;
var_set_t *set2;
var_set_t *mask;
int diff;
{
  int result;
  var_set_t *tmp1 = var_set_copy(set1);
  var_set_t *tmp2 = var_set_copy(set2);
  
  (void) var_set_and(tmp1, tmp1, mask);
  (void) var_set_and(tmp2, tmp2, mask);
  if (var_set_equal(tmp1, tmp2)) {
    result = (diff < 0) ? 1 : 0;
  } else {
    (void) var_set_and(tmp2, tmp1, tmp2);
    result = var_set_equal(tmp1, tmp2);
  }
  var_set_free(tmp1);
  var_set_free(tmp2);
  return result;
}

 /* extract variables still active (i.e. complement of union of sets in key) */
static var_set_t *extract_uncovered_variables(info, key)
set_info_t *info;
set_key_t *key;
{
  int i;
  var_set_t *result = var_set_new(info->n_vars);

  for (i = 0; i < info->n_sets; i++) {
    if (! var_set_get_elt(key->placed_so_far, i)) continue;
    (void) var_set_or(result, result, info->sets[i]);
  }
  (void) var_set_not(result, result);
  return result;
}

 /* for debugging */


 /* simple greedy heuristic */

array_t *find_greedy_set_order(info)
set_info_t *info;
{
  int i;
  int size;
  int best_next_index;
  array_t *result = array_alloc(int, 0);
  var_set_t *placed_so_far = var_set_new(info->n_sets);
  var_set_t *mask = var_set_new(info->n_vars);

  for (i = 0; i < info->n_sets; i++) {
    best_next_index = arg_min_remaining_size(info, placed_so_far, mask, &size);
    array_insert_last(int, result, best_next_index);
    var_set_set_elt(placed_so_far, best_next_index);
    (void) var_set_or(mask, mask, info->sets[best_next_index]);
  }
  var_set_free(placed_so_far);
  var_set_free(mask);
  return result;
}


 /* get the first remaining set of minimum size relative to remaining vars */
 /* also returns its size relative to remaining vars (vars not in mask) */

static int arg_min_remaining_size(info, placed_so_far, mask, min_size)
set_info_t *info;
var_set_t *placed_so_far;
var_set_t *mask; /* all those already there: do not discriminate any more */
int *min_size;
{
  int i, size;
  int best_index = -1;
  int best_size = LARGE_NUMBER;
  var_set_t *tmp = var_set_new(info->n_vars);

  for (i = 0; i < info->n_sets; i++) {
    if (var_set_get_elt(placed_so_far, i)) continue;
    size = var_set_n_elts(var_set_or(tmp, mask, info->sets[i]));
    if (size < best_size) {
      best_size = size;
      best_index = i;
    }
  }
  var_set_free(tmp);
  *min_size = best_size - var_set_n_elts(mask);
  return best_index;
}

static void print_set_info(info)
set_info_t *info;
{
  int i, j, value;
  for (i = 0; i < info->n_sets; i++) {
    assert(info->sets[i]->n_elts == info->n_vars);
    for (j = 0; j < info->n_vars; j++) {
      value = var_set_get_elt(info->sets[i], j);
      printf("%d%s", value, (j == info->n_vars - 1) ? "" : " ");
    }
    printf("\n");
  }
}

static void print_int_array(data)
array_t *data;
{
  int i;

  for (i = 0; i < array_n(data); i++) {
    printf("%d ", array_fetch(int, data, i));
  }
  printf("\n");
}

static int compute_order_cost(info, order)
set_info_t *info;
array_t *order;
{
  int i;
  int index;
  int result = 0;
  var_set_t *union_so_far = var_set_new(info->n_vars);

  for (i = 0; i < array_n(order); i++) {
    index = array_fetch(int, order, i);
    (void) var_set_or(union_so_far, union_so_far, info->sets[index]);
    result += var_set_n_elts(union_so_far);
  }
  var_set_free(union_so_far);
  return result;
}
#endif /* SIS */
