
#include "simp_int.h"
#include "sis.h"

node_t *simp_bull_cofactor();

static array_t *simp_disjoint_support_functions();

static node_t *simp_range_2_compute();

int set_size_sort();

static void simp_free_bddlist(bdd_list, node_list)
    array_t *bdd_list; /* list of functions whose image to be computed */
array_t *node_list;    /* variables associated with each function */
{
  int j;
  bdd_t *bdd;

  for (j = 0; j < array_n(bdd_list); j++) {
    bdd = array_fetch(bdd_t *, bdd_list, j);
    if (bdd != NIL(bdd_t)) {
      bdd_free(bdd);
    }
  }
  array_free(bdd_list);
  array_free(node_list);
}

/*
 * It finds the image of a set of Boolean functions and returns the
 * image in sum-of-products form (node_t).
 */
node_t *simp_bull_cofactor(bdd_list, node_list, mg, leaves)
    array_t *bdd_list; /* list of functions whose image to be computed */
array_t *node_list;    /* variables associated with each function */
bdd_manager *mg;
st_table *leaves; /* not used here, for debugging purposes */
{
  int i;
  bdd_t *bdd;
  bdd_t *c, *cbar;
  node_t *image_node, *tmpnode;
  array_t *sup_list;
  array_t *left_list, *right_list;
  array_t *left_node, *right_node;
  node_t *node;
  node_t *n1, *n2, *n3;
  int flag, active;
  array_t *f_list;
  var_set_t *set;
  int l, j, count;
  unsigned list_not_free;

  count = 0;
  active = 0;
  flag = -1;

  /*The image is represented by image_node */
  image_node = node_constant(1);

  /*
   * Check if a function is constant. Constant functions are removed from the
   * list. AND an appropriate variable with image_node.
   */
  for (i = 0; i < array_n(bdd_list); i++) {
    bdd = array_fetch(bdd_t *, bdd_list, i);
    if (bdd == NIL(bdd_t))
      continue;
    if (bdd_is_tautology(bdd, 1)) {
      bdd_free(bdd);
      array_insert(bdd_t *, bdd_list, i, NIL(bdd_t));
      n1 = array_fetch(node_t *, node_list, i);
      n2 = node_literal(n1, 1);
      n3 = node_and(n2, image_node);
      node_free(image_node);
      node_free(n2);
      image_node = n3;
      continue;
    }
    if (bdd_is_tautology(bdd, 0)) {
      bdd_free(bdd);
      array_insert(bdd_t *, bdd_list, i, NIL(bdd_t));
      n1 = array_fetch(node_t *, node_list, i);
      n2 = node_literal(n1, 0);
      n3 = node_and(n2, image_node);
      node_free(image_node);
      node_free(n2);
      image_node = n3;
      continue;
    }
    count++;
  }

  /* If only one function left in the list return. */
  if (count <= 1) {
    for (i = 0; i < array_n(bdd_list); i++) {
      bdd = array_fetch(bdd_t *, bdd_list, i);
      if (bdd != NIL(bdd_t)) {
        bdd_free(bdd);
        break; /* because there is at most one */
      }
    }
    array_free(bdd_list);
    array_free(node_list);
    return (image_node);
  }

  /* Patition the functions into sets with disjoint support. */
  sup_list = array_alloc(var_set_t *, count);
  flag = 1;
  for (i = 0; i < array_n(bdd_list); i++) {
    bdd = array_fetch(bdd_t *, bdd_list, i);
    if (bdd == NIL(bdd_t))
      continue;
    set = bdd_get_support(bdd);
    array_insert_last(var_set_t *, sup_list, set);
  }
  f_list = simp_disjoint_support_functions(sup_list);

  for (i = 0; i < array_n(sup_list); i++) {
    set = array_fetch(var_set_t *, sup_list, i);
    var_set_free(set);
  }
  array_free(sup_list);

  /*
   * process each partition separately. If there is only function in a
   * partition, then both phases of that function are reachable. If there are
   * two functions, call a special routine for computing the range of two
   * functions. Else, do output cofactoring with respect to the first function
   * in the partition and call this routine again.
   */

  list_not_free = 1;
  assert(array_n(f_list) >= 1); /* for loop should be executed at least once */
  tmpnode = node_constant(1);
  for (i = 0; i < array_n(f_list); i++) {
    set = array_fetch(var_set_t *, f_list, i);
    active = var_set_n_elts(set);
    if (active == 1)
      continue;
    if (active == 2) {
      node = simp_range_2_compute(set, bdd_list, node_list);
      n1 = node_and(tmpnode, node);
      node_free(node);
      node_free(tmpnode);
      tmpnode = n1;
      continue;
    }
    flag = -1;
    right_list = array_alloc(bdd_t *, 0);
    left_list = array_alloc(bdd_t *, 0);
    right_node = array_alloc(node_t *, 0);
    left_node = array_alloc(node_t *, 0);
    for (j = 0, l = 0; j < array_n(bdd_list); j++) {
      bdd = array_fetch(bdd_t *, bdd_list, j);
      if (bdd == NIL(bdd_t))
        continue;
      if (var_set_get_elt(set, l)) {
        n1 = array_fetch(node_t *, node_list, j);
        array_insert_last(node_t *, right_node, n1);
        array_insert_last(node_t *, left_node, n1);
        if (flag == -1) {
          c = bdd;
          flag = j;
          cbar = bdd_not(c);
          array_insert_last(bdd_t *, right_list, bdd_one(mg));
          array_insert_last(bdd_t *, left_list, bdd_zero(mg));
        } else {
          array_insert_last(bdd_t *, right_list, bdd_cofactor(bdd, c));
          array_insert_last(bdd_t *, left_list, bdd_cofactor(bdd, cbar));
        }
      }
      l++;
    }

    if (flag != -1) {
      bdd_free(cbar);
    }

    if (i == (array_n(f_list) - 1)) {
      /* won't be going through for loop again, so free BDDs now to save memory
       */
      simp_free_bddlist(bdd_list, node_list);
      list_not_free = 0;
    }

    n1 = simp_bull_cofactor(right_list, right_node, mg, leaves);
    n2 = simp_bull_cofactor(left_list, left_node, mg, leaves);

    node = node_or(n1, n2);
    node_free(n1);
    node_free(n2);
    n1 = node_and(node, tmpnode);
    node_free(node);
    node_free(tmpnode);
    tmpnode = n1;
  }

  node = node_and(image_node, tmpnode);
  node_free(image_node);
  node_free(tmpnode);

  if (list_not_free) {
    simp_free_bddlist(bdd_list, node_list);
  }

  for (j = 0; j < array_n(f_list); j++) {
    set = array_fetch(var_set_t *, f_list, j);
    var_set_free(set);
  }
  array_free(f_list);

  return (node);
}

/*
 * Finds groups of functions whose support is completely disjoint from the
 * rest of the functions.
 */

static array_t *simp_disjoint_support_functions(list) array_t *list;
{
  int row, column;
  int i, j;
  int flag;
  array_t *tmp;
  var_set_t *set, *fset, *nset;
  array_t *prtn_list;

  row = array_n(list);
  tmp = array_alloc(var_set_t *, row);
  for (i = 0; i < row; i++) {
    set = array_fetch(var_set_t *, list, i);
    nset = var_set_copy(set);
    array_insert(var_set_t *, tmp, i, nset);
  }
  set = array_fetch(var_set_t *, list, 0);
  column = set->n_elts;

  for (i = 0; i < column; i++) {
    flag = 1;
    for (j = 0; j < row; j++) {
      set = array_fetch(var_set_t *, tmp, j);
      if (set != NIL(var_set_t)) {
        if (flag) {
          if (var_set_get_elt(set, i)) {
            fset = set;
            flag = 0;
            continue;
          }
        }
        if (var_set_get_elt(set, i)) {
          fset = var_set_or(fset, fset, set);
          var_set_free(set);
          array_insert(var_set_t *, tmp, j, NIL(var_set_t));
          continue;
        }
      }
    }
  }

  prtn_list = array_alloc(var_set_t *, 0);
  for (j = 0; j < row; j++) {
    set = array_fetch(var_set_t *, tmp, j);
    if (set != NIL(var_set_t)) {
      nset = var_set_new(row);
      for (i = 0; i < row; i++) {
        fset = array_fetch(var_set_t *, list, i);
        if (var_set_intersect(set, fset))
          var_set_set_elt(nset, i);
      }
      var_set_free(set);
      array_insert(var_set_t *, tmp, j, NIL(var_set_t));
      array_insert_last(var_set_t *, prtn_list, nset);
    }
  }
  array_free(tmp);
  return (prtn_list);
}

/* special case of range computation for only two functions */

static node_t *simp_range_2_compute(set, bdd_list, node_list) var_set_t *set;
array_t *bdd_list;
array_t *node_list;
{
  int i, j;
  int k1, k2;
  bdd_t *bdd, *f1, *f2, *out1, *out2, *f1_bar;
  node_t *c1, *c2, *g;
  node_t *n1, *n2;
  node_t *n3, *n4;

  k1 = -1;
  for (i = 0, j = 0; i < array_n(bdd_list); i++) {
    bdd = array_fetch(bdd_t *, bdd_list, i);
    if (bdd == NIL(bdd_t))
      continue;
    if (!var_set_get_elt(set, j++))
      continue;
    if (k1 == -1) {
      k1 = i;
      f1 = bdd;
    } else {
      k2 = i;
      f2 = bdd;
      break;
    }
  }
  n1 = array_fetch(node_t *, node_list, k1);
  n2 = array_fetch(node_t *, node_list, k2);
  f1_bar = bdd_not(f1);
  out1 = bdd_cofactor(f2, f1);
  out2 = bdd_cofactor(f2, f1_bar);
  bdd_free(f1_bar);
  if (bdd_is_tautology(out1, 1)) {
    n3 = node_literal(n1, 1);
    n4 = node_literal(n2, 1);
    c1 = node_and(n3, n4);
    node_free(n3);
    node_free(n4);
  } else if (bdd_is_tautology(out1, 0)) {
    n3 = node_literal(n1, 1);
    n4 = node_literal(n2, 0);
    c1 = node_and(n3, n4);
    node_free(n3);
    node_free(n4);
  } else
    c1 = node_literal(n1, 1);

  if (bdd_is_tautology(out2, 1)) {
    n3 = node_literal(n1, 0);
    n4 = node_literal(n2, 1);
    c2 = node_and(n3, n4);
    node_free(n3);
    node_free(n4);
  } else if (bdd_is_tautology(out2, 0)) {
    n3 = node_literal(n1, 0);
    n4 = node_literal(n2, 0);
    c2 = node_and(n3, n4);
    node_free(n3);
    node_free(n4);
  } else {
    c2 = node_literal(n1, 0);
  }

  g = node_or(c1, c2);
  node_free(c1);
  node_free(c2);
  bdd_free(out1);
  bdd_free(out2);
  return (g);
}

/* sort sets based on the number of bits that are set to 1. */

int set_size_sort(obj1, obj2) char **obj1;
char **obj2;
{
  node_t *node1, *node2;
  int el1, el2;

  node1 = *((node_t **)obj1);
  node2 = *((node_t **)obj2);

  /* The following two lines were added to make this function correctly on VMS
   */
  if (node_function(node1) == NODE_PI && node_function(node2) == NODE_PI)
    return 0;

  if (node_function(node1) == NODE_PI)
    return (-1);
  if (node_function(node2) == NODE_PI)
    return (1);
  el1 = var_set_n_elts(CSPF(node1)->set);
  el2 = var_set_n_elts(CSPF(node2)->set);
  if (el1 < el2)
    return (-1);
  if (el1 > el2)
    return (1);
  return (0);
}
