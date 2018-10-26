
#include "sis.h"

/*
 * Revision 1.3  1992/05/06  18:58:25  sis
 * SIS release 1.1
 *
 * Revision 1.3  1992/05/06  18:58:25  sis
 * SIS release 1.1
 *
 * Revision 1.2  1992/04/17  22:06:10  sis
 * *** empty log message ***
 *
 * Revision 1.1  92/01/08  17:43:56  sis
 * Initial revision
 * 
 * Revision 1.1  91/04/01  00:52:30  shiple
 * Initial revision
 * 
 *
 */

/* for order nodes */
typedef struct {
    node_t *node;
    int level;
    int max_leaf;
} info_t ;

static int max_leaf_cmp();
static int propagate_max_leaf_from_fanins();
static void extract_transitive_fanin();
static void order_nodes_rec();
static void propagate_level_thru_factored_form();
static void propagate_level_to_fanins();
static void visit_inputs_first();
static void visit_outputs_first();

/*
 *    order_dfs - Find a good ordering for the nodes.
 *    The second argument is a hash table.
 *    Each node that requires ordering is stored in the hash table
 *    as a key; the value associated to it should be -1.
 *    The ordering is done by replacing the -1's by variable orders.
 *
 *    Returns an array of nodes in the order of traversal.  This used to be used
 *    to specify in which order the nodes of the transistive fanin should be 
 *    visited when creating the BDD of a root.  Simple recursion from the root 
 *    seems to work as well.
 *
 *    Only touches the part of the network that is both in the transitive fanin
 *    of nodes in roots and in the transitive fanout of nodes in leaves.
 *    As a consistency condition, it is required that every path from roots to
 *    a primary input is cut by a node in leaves. Otherwise -> error.
 *  
 *    This function was known historically as bdd_sharad_order.
 */
array_t *order_dfs(roots, leaves, fixed_root_order)
array_t *roots;			/* root nodes: their transitive fanin has to be ordered */
st_table *leaves;		/* where the dfs search stops: those nodes have to be ordered */
int fixed_root_order;		/* flag: if set, can't change the order of visit of the roots */
{
  int order_count;
  node_t *node;
  st_generator *gen;
  array_t *tfi_inputs_first, *tfi_outputs_first;
  array_t *order_list, *internal_roots;
  st_table *info_table, *visited;
  info_t *info;
  int i;

  if(array_n(roots) == 0) {
    assert(st_count(leaves) == 0);
    return NIL(array_t);
  }

  /* replace PO's by their fanins */
  internal_roots = array_alloc(node_t *, 0);
  for(i = 0; i < array_n(roots); i++) {
    node = array_fetch(node_t *, roots, i);
    if (node->type == PRIMARY_OUTPUT) {
      node = node_get_fanin(node, 0);
    }
    array_insert_last(node_t *, internal_roots, node);
  }

  /* form dfs array */
  tfi_inputs_first = array_alloc(node_t *, 0);
  tfi_outputs_first = array_alloc(node_t *, 0);
  (void) extract_transitive_fanin(internal_roots, leaves, tfi_inputs_first, tfi_outputs_first);

  /*	initialize info_table	*/
  info_table = st_init_table(st_ptrcmp, st_ptrhash);
  for(i = 0; i < array_n(tfi_inputs_first); i++) {
    node = array_fetch(node_t *, tfi_inputs_first, i);
    info = ALLOC(info_t, 1);
    info->node = node;
    info->level = 0;
    info->max_leaf = 0;
    (void) st_insert(info_table, (char *) node, (char *) info);
  }

  /* compute level information: levelize from outputs to inputs */
  for (i = 0; i < array_n(tfi_outputs_first); i++) {
    node = array_fetch(node_t *, tfi_outputs_first, i);
    if (st_lookup(leaves, (char *) node, NIL(char *)))
      continue;
    (void) propagate_level_to_fanins(node, info_table);
  }
  array_free(tfi_outputs_first);

  /* compute max_leaf fields */
  for (i = 0; i < array_n(tfi_inputs_first); i++) {
    node = array_fetch(node_t *, tfi_inputs_first, i);
    assert(st_lookup(info_table, (char *) node, (char **) &info));
    if (st_lookup(leaves, (char *) node, NIL(char *))) {
      info->max_leaf = info->level;
      continue;
    }
    info->max_leaf = propagate_max_leaf_from_fanins(node, info_table);
  }
  array_free(tfi_inputs_first);

 /* if fixed root order, take care of that */
  if (fixed_root_order) {
    for (i = 0; i < array_n(internal_roots); i++) {
      node = array_fetch(node_t *, internal_roots, i);
      assert(st_lookup(info_table, (char *) node, (char **) &info));
      info->max_leaf = 0;
      info->level = i;
    }
  }

  /* all the bookkeeping done, go ahead and order */
  order_count = 0;
  order_list = array_alloc(node_t *, 0);
  visited = st_init_table(st_ptrcmp, st_ptrhash);
  (void) order_nodes_rec(internal_roots, info_table, visited, leaves, order_list, &order_count);
  assert(order_count <= st_count(leaves));

  st_foreach_item(info_table, gen, (char **) &node, (char **) &info) {
    FREE(info);
  }
  (void) st_free_table(info_table);
  (void) st_free_table(visited);
  (void) array_free(internal_roots);

  return order_list;
}

/*
 * Returns an array of the nodes in the transistive fanin of roots, in DFS
 * order.  Note that the leaves in the TFI are not put into the return array.
 * TODO: this function is not currently used anywhere.  Historically, it was
 * used to provide an alternative to the node ordering returned by Sharad's
 * ordering technique.
 */
array_t *bdd_tfi_inputs_first(roots, leaves)
array_t *roots;
st_table *leaves;
{
  int i;
  node_t *node;
  array_t *tfi_inputs_first = array_alloc(node_t *, 0);
  st_table *tfi = st_init_table(st_ptrcmp, st_ptrhash);
  
  for (i = 0; i < array_n(roots); i++) {
    node = array_fetch(node_t *, roots, i);
    (void) visit_inputs_first(node, leaves, tfi, tfi_inputs_first);
    (void) array_insert_last(node_t *, tfi_inputs_first, node);
    (void) st_insert(tfi, (char *) node, NIL(char));  /* tfi table just used for quick searching */
  }
  (void) st_free_table(tfi);
  return tfi_inputs_first;
}


/* 
 * Extract nodes in transitive fanin of nodes in roots,
 * but stop at nodes in table "leaves."
 */
static void extract_transitive_fanin(roots, leaves, tfi_inputs_first, tfi_outputs_first)
array_t *roots;
st_table *leaves;
array_t *tfi_inputs_first;
array_t *tfi_outputs_first;
{
  int i;
  node_t *node;
  st_generator *gen;
  st_table *tfi = st_init_table(st_ptrcmp, st_ptrhash);
  st_table *visited = st_init_table(st_ptrcmp, st_ptrhash);
  
  for (i = 0; i < array_n(roots); i++) {
    node = array_fetch(node_t *, roots, i);
    (void) visit_inputs_first(node, leaves, tfi, tfi_inputs_first);
  }
  st_foreach_item(leaves, gen, (char **) &node, NIL(char *)) {
    (void) visit_outputs_first(node, visited, tfi, tfi_outputs_first);
  }
  (void) st_free_table(visited);
  (void) st_free_table(tfi);
}

/* 
 * The transitive fanin of node limited to the tfi.
 * It is an error if find a PRIMARY INPUT not in leaves.
 */
static void visit_inputs_first(node, leaves, tfi, tfi_inputs_first)
node_t *node;
st_table *leaves;
st_table *tfi;
array_t *tfi_inputs_first;
{
  int i;
  node_t *fanin;

  /*
   * If the node has already been visited, then return.
   */
  if (st_lookup(tfi, (char *) node, NIL(char *))) return;

  /* 
   * Process the node if it's not a leaf. If it's not a constant, then 
   * it must have fanin.  Visit each fanin.
   */
  if (! st_lookup(leaves, (char *) node, NIL(char *))) {
    if (node_function(node) != NODE_0 && node_function(node) != NODE_1) {
      assert(node_num_fanin(node) > 0);
      foreach_fanin(node, i, fanin) {
	(void) visit_inputs_first(fanin, leaves, tfi, tfi_inputs_first);
      }
    }
  }
  (void) array_insert_last(node_t *, tfi_inputs_first, node);
  (void) st_insert(tfi, (char *) node, NIL(char));
}

static void visit_outputs_first(node, visited, tfi, tfi_outputs_first)
node_t *node;
st_table *visited;
st_table *tfi;
array_t *tfi_outputs_first;
{
  lsGen gen;
  node_t *fanout;

  if (! st_lookup(tfi, (char *) node, NIL(char *))) return;
  if (st_lookup(visited, (char *) node, NIL(char *))) return;
  foreach_fanout(node, gen, fanout) {
    (void) visit_outputs_first(fanout, visited, tfi, tfi_outputs_first);
  }
  (void) array_insert_last(node_t *, tfi_outputs_first, node);
  (void) st_insert(visited, (char *) node, NIL(char));
}


 /* level propagation through factored nodes */

static void propagate_level_to_fanins(node, info_table)
node_t *node;
st_table *info_table;
{
  int i;
  info_t *info;
  int level;
  node_t *factor_node;
  array_t *factor_array = factor_to_nodes(node);
  st_table *factor_table = st_init_table(st_ptrcmp, st_ptrhash);

  for (i = 0; i < array_n(factor_array); i++) {
    factor_node = array_fetch(node_t *, factor_array, i);
    (void) st_insert(factor_table, (char *) factor_node, NIL(char));
  }
  factor_node = array_fetch(node_t *, factor_array, 0);
  assert(st_lookup(info_table, (char *) node, (char **) &info));
  level = info->level;
  (void) propagate_level_thru_factored_form(factor_node, level, factor_table, info_table);
  for (i = 0; i < array_n(factor_array); i++) {
    factor_node = array_fetch(node_t *, factor_array, i);
    (void) node_free(factor_node);
  }
  (void) array_free(factor_array);
  (void) st_free_table(factor_table);
}

static void propagate_level_thru_factored_form(node, level, factor_table, info_table)
node_t *node;
int level;
st_table *factor_table, *info_table;
{
  int i;
  node_t *fanin;
  info_t *info;

  foreach_fanin(node, i, fanin) {
    if (st_lookup(factor_table, (char *) fanin, NIL(char *))) {
      (void) propagate_level_thru_factored_form(fanin, level + 1, factor_table, info_table);
      continue;
    } 
    assert(st_lookup(info_table, (char *) fanin, (char **) &info));
    info->level = MAX(info->level, level + 1);
  }
}

 /* compute max_leaf from inputs to outputs */

static int propagate_max_leaf_from_fanins(node, info_table)
node_t *node;
st_table *info_table;
{
  int i;
  info_t *info;
  node_t *fanin;
  int max_leaf = 0;

  foreach_fanin(node, i, fanin) {
    assert(st_lookup(info_table, (char *) fanin, (char **) &info));
    max_leaf = MAX(max_leaf, info->max_leaf);
  }
  return max_leaf;
}

 /* order nodes using dfs. Break ties using max_leaf_cmp */

static void order_nodes_rec(roots, info_table, visited, leaves, node_list, order_count)
array_t *roots;
st_table *info_table;
st_table *visited;
st_table *leaves;
array_t *node_list;
int *order_count;
{
  int *slot;
  array_t *info_array, *fanin_array;
  int i, j;
  node_t *node, *fanin;
  info_t *info;

 /* sort current list of roots according to their max_leaf entry */
  info_array = array_alloc(info_t *, 0);
  for (i = 0; i < array_n(roots); i++) {
    node = array_fetch(node_t *, roots, i);
    if (st_lookup(visited, (char *) node, NIL(char *))) continue;
    assert(st_lookup(info_table, (char *) node, (char **) &info));
    (void) array_insert_last(info_t *, info_array, info);
  }
  (void) array_sort(info_array, max_leaf_cmp);
  
 /* continue traversal in dfs mode; stop at leaves ("leaves" nodes) */
 /* need to check for visited here because a node can appear several times */
 /* in info_array */
  for (i = 0; i < array_n(info_array); i++) {
    info = array_fetch(info_t *, info_array, i);
    node = info->node;
    if (st_lookup(visited, (char *) node, NIL(char *))) continue;
    if (st_find(leaves, (char *) node, (char ***) &slot)) {
      *slot = (*order_count)++;
    } else {
      fanin_array = array_alloc(node_t *, 0);
      foreach_fanin(node, j, fanin) {
	(void) array_insert_last(node_t *, fanin_array, fanin);
      }
      (void) order_nodes_rec(fanin_array, info_table, visited, leaves, node_list, order_count);
      (void) array_free(fanin_array);
    }
    (void) array_insert_last(node_t *, node_list, node);
    (void) st_insert(visited, (char *) node, NIL(char));
  }
  (void) array_free(info_array);
}


 /* break ties using first depth, then level */
 /* deeper->higher priority; otherwise lower level->higher priority */

static int max_leaf_cmp(key1, key2)
char **key1, **key2;
{
  info_t *info1 = *((info_t **) key1);
  info_t *info2 = *((info_t **) key2);

  if (info1->max_leaf == info2->max_leaf) {
    return info1->level - info2->level;
  } else {
    return - (info1->max_leaf - info2->max_leaf);
  }
}
