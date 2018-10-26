
#include "sis.h"
#include "pld_int.h"

/*-----------------------------------------------------------------------
  Replaces node by network1. Node should be in some network. Postscript
  1 always refers to the network for node or any node therein.
------------------------------------------------------------------------*/
pld_replace_node_by_network(node, network1)
  node_t *node;
  network_t *network1;
{
  node_t *node1, *node1_dup;
  array_t *nodevec1;
  int i, num_nodes2;
  st_table *table1;
  network_t *network;

  network = node->network;
  nodevec1 = network_dfs(network1);
  table1 = st_init_table(st_ptrcmp, st_ptrhash);
  /* set correspondence between p.i. and ... */
  /*-----------------------------------------*/
  pld_remap_init_corr(table1, network, network1);
  num_nodes2 = array_n(nodevec1) - 2;
  for (i  = 0; i < num_nodes2; i++) {
         node1 = array_fetch(node_t *, nodevec1, i);
         if (node1->type != INTERNAL) continue;
         node1_dup = pld_remap_get_node(node1, table1);
         FREE(node1_dup->name);
         FREE(node1_dup->short_name);
         network_add_node(network, node1_dup);
         assert(!st_insert(table1, (char *) node1, (char *) node1_dup));
         /* if (XLN_DEBUG > 3) (void) printf("****corr. between %s and %s\n",
                                      node_long_name(node1), node_long_name(node1_dup));
         */
  }
  /* the last node in the array in primary output - do not want that */
  /*-----------------------------------------------------------------*/
  node1 = array_fetch(node_t *, nodevec1, num_nodes2);
  assert(node1->type == INTERNAL);
  node1_dup = pld_remap_get_node(node1, table1);
  /* if (XLN_DEBUG > 3) (void) printf("****corr. finally between %s and %s\n",
                               node_long_name(node1), node_long_name(node1_dup)); */
  node_replace(node, node1_dup);

  st_free_table(table1);	
  array_free(nodevec1);
}

/*-----------------------------------------------------------------
  Returns a node for the original network which has same logic 
  function as node1 of network1.
------------------------------------------------------------------*/
node_t *
pld_remap_get_node(node1, table1)
  node_t *node1;
  st_table *table1;
{
  node_t *node, *n2, *cube, *cube2, *fanin, *fanin1, *fanin_lit;
  node_cube_t cube1;
  node_literal_t literal1;
  int i, j;

  node = node_constant(0);
  for (i = node_num_cube(node1) - 1; i >= 0; i--) {
     cube = node_constant(1);
     cube1 = node_get_cube(node1, i);
     foreach_fanin(node1, j, fanin1) {
       assert(st_lookup(table1, (char *) fanin1, (char **) &fanin));       
       literal1 = node_get_literal(cube1, j);
       switch(literal1) {
          case ONE: /* +ve phase */   
            fanin_lit = node_literal(fanin, 1);
            cube2 = node_and(cube, fanin_lit);
            node_free(fanin_lit);
            node_free(cube);
            cube = cube2;
            break;
          case ZERO:
            fanin_lit = node_literal(fanin, 0);
            cube2 = node_and(cube, fanin_lit);
            node_free(fanin_lit);
            node_free(cube);
            cube = cube2;
            break;
          case TWO:
            break;
          default:
            (void) printf("error in the cube\n");
            exit(1);
       }
     }
     n2 = node_or(cube, node);
     node_free(cube);
     node_free(node);
     node = n2;
  }
  return node;
}

/*------------------------------------------------------------------
  Sets up correspondence between primary inputs of network1 and 
  the corresponding nodes of network. 
--------------------------------------------------------------------*/
pld_remap_init_corr(table1, network, network1)
  st_table *table1;
  network_t *network, *network1;
{
  lsGen gen;
  node_t *pi1, *node;

  foreach_primary_input(network1, gen, pi1) {
      node = network_find_node(network, node_long_name(pi1));
      assert(!st_insert(table1, (char *) pi1, (char *) node));
      /* if (ACT_DEBUG) 
         (void) printf("inserting node %s (%s) in table\n", node_long_name(node), 
                       node_name(node)); */
  }
}

/*---------------------------------------------------------------------------
  For each cube of the node, forms a node. Returns an array of these nodes.
----------------------------------------------------------------------------*/
array_t *
pld_nodes_from_cubes(node)
  node_t *node;
{
  array_t *nodevec;
  int i;
  node_cube_t cube;
  node_t *node_cube;

  nodevec = array_alloc(node_t *, 0);
  for (i = node_num_cube(node) - 1; i >= 0; i--) {
      cube = node_get_cube(node, i);
      node_cube = pld_make_node_from_cube(node, cube);
      array_insert_last(node_t *, nodevec, node_cube);
  }
  return nodevec;
}

/*-----------------------------------------------------------------
  Returns a node from the cube.
------------------------------------------------------------------*/
node_t *
pld_make_node_from_cube(node, cube)
  node_t *node;
  node_cube_t cube;
{
  node_t *n, *n1, *n2, *fanin;
  int j;
  node_literal_t literal;

  n = node_constant(1);
  foreach_fanin(node, j, fanin) {
      literal = node_get_literal(cube, j);
      switch(literal) {
        case ONE:
          n1 = node_literal(fanin, 1);
          n2 = node_and(n, n1);
          node_free(n);
          node_free(n1);
          n = n2;
          break;
        case ZERO:
          n1 = node_literal(fanin, 0);
          n2 = node_and(n, n1);
          node_free(n);
          node_free(n1);
          n = n2;
          break;
        case TWO:
          break;
      }
  }
  return n;
}

/*-----------------------------------------------------------------
  Given a node, returns an array of cubes of the nodes.
------------------------------------------------------------------*/
array_t *
pld_cubes_of_node(node)
  node_t *node;
{
  array_t *cubevec;
  int i;
  node_cube_t cube;

  cubevec = array_alloc(node_cube_t, 0);
  for (i = node_num_cube(node) - 1; i >= 0; i--) {
      cube = node_get_cube(node, i);
      array_insert_last(node_cube_t, cubevec, cube);
  }
  return cubevec;
}

/*-------------------------------------------------------------------------
  Given a node, returns an array of fanins of the node not present in the 
  array common.
--------------------------------------------------------------------------*/
array_t *
pld_get_non_common_fanins(node, common)
  node_t *node;
  array_t *common;
{
  array_t *nc;
  node_t *fanin;
  int i;

  nc = array_alloc(node_t *, 0);
  foreach_fanin(node, i, fanin) {
      if (pld_is_node_in_array(fanin, common)) continue;
      array_insert_last(node_t *, nc, fanin);
  }
  return nc;
}

int 
pld_is_node_in_array(node, vec)
  node_t *node;
  array_t *vec;
{
  int i;
  node_t *n;

  for(i = 0; i < array_n(vec); i++) {
      n = array_fetch(node_t *, vec, i);
      if (n == node) return 1;
  }
  return 0;
}

int 
pld_num_fanin_cube(cube, node)
  node_cube_t cube;
  node_t *node;
{
  int i;
  int num_fanin;
  int count;
  node_literal_t literal;

  count = 0;
  num_fanin = node_num_fanin(node);
  for (i = 0; i < num_fanin; i++) {
      literal = node_get_literal(cube, i);
      if ((literal == ONE) || (literal == ZERO)) count++;
  }
  return count;
}

/*--------------------------------------------------------------
  Returns 1 if the fanin-set of n1 is a subset of fanin-set of
  n2. Else 0.
---------------------------------------------------------------*/
int 
pld_is_fanin_subset(n1, n2)
  node_t *n1, *n2;
{
  int j;
  node_t *fanin;

  foreach_fanin(n1, j, fanin) {
      if (node_get_fanin_index(n2, fanin) >= 0) continue;
      return 0;
  }
  return 1;
}

array_t *
pld_get_array_of_fanins(node)
  node_t *node;
{
  int i;
  node_t *fanin;
  array_t *faninvec;

  if (node->type == PRIMARY_INPUT) return NIL (array_t);
  faninvec = array_alloc(node_t *, 0);
  foreach_fanin(node, i, fanin) {
      array_insert_last(node_t *, faninvec, fanin);
  }
  return faninvec;
}

pld_replace_node_with_array(node, nodevec)
  node_t *node;
  array_t *nodevec;
{
  node_t *n;
  int i;

  for (i = 1; i < array_n(nodevec); i++) {
      n = array_fetch(node_t *, nodevec, i);
      network_add_node(node->network, n);
  }
  n = array_fetch(node_t *, nodevec, 0);
  node_replace(node, n);
}

pld_simplify_network_without_dc(network)
  network_t *network;
{
  lsGen gen;
  node_t *node;

  foreach_node(network, gen, node) {
      if (node->type != INTERNAL) continue;
      simplify_node(node, SIM_METHOD_SNOCOMP, SIM_DCTYPE_NONE, SIM_FILTER_NONE, 
                    SIM_ACCEPT_SOP_LITS);
  }
}

int
xln_is_network_feasible(network, support)
  network_t *network;
  int support;
{
  lsGen gen;
  node_t *node;

  foreach_node(network, gen, node) {
      if (node->type != INTERNAL) continue;
      if (node_num_fanin(node) > support) return 0;
  }
  return 1;
}

int 
my_node_equal(node1, node2)
  node_t *node1, *node2;
{
  node_function_t fn1, fn2;

  fn1 = node_function(node1);
  fn2 = node_function(node2);
  if ((fn1 == NODE_0) && (fn2 == NODE_0)) return 1;
  if ((fn1 == NODE_1) && (fn2 == NODE_1)) return 1;  
  return node_equal(node1, node2);
}

st_table* 
pld_insert_intermediate_nodes_in_table(network)
  network_t *network;
{
  st_table *table;
  node_t *node;
  lsGen gen;

  table = st_init_table(st_ptrcmp, st_ptrhash);
  foreach_node(network, gen, node) {
      if (node->type == INTERNAL) {
          (void) st_insert(table, (char *) node, (char *) node);
      }
  }
  return table;
}

/*-----------------------------------------
  Delete nodes of array from table.
------------------------------------------*/
pld_delete_array_nodes_from_table(table, array)
  st_table *table;
  array_t *array;
{
  int i;
  node_t *node;
  char *dummy;

  for (i = 0; i < array_n(array); i++) {
      node = array_fetch(node_t *, array, i);
      assert(st_delete(table, (char **) &node, &dummy));
  }
}

sm_delete_rows_covered_by_col(matrix, col)
  sm_matrix *matrix;
  sm_col *col;
{
  array_t *row_array;

  row_array = sm_get_rows_covered_by_col(matrix, col);
  sm_delete_rows_in_array(matrix, row_array);
  array_free(row_array);
}

sm_delete_rows_in_array(matrix, row_array)
  sm_matrix *matrix;
  array_t *row_array;
{
  int i;
  sm_row *row;

  for (i = 0; i < array_n(row_array); i++) {
      row = array_fetch(sm_row *, row_array, i);
      sm_delrow(matrix, row->row_num);
  }

}

sm_delete_cols_covered_by_row(matrix, row)
  sm_matrix *matrix;
  sm_row *row;
{
  array_t *col_array;

  col_array = sm_get_cols_covered_by_row(matrix, row);
  sm_delete_cols_in_array(matrix, col_array);
  array_free(col_array);
}

sm_delete_cols_in_array(matrix, col_array)
  sm_matrix *matrix;
  array_t *col_array;
{
  int i;
  sm_col *col;

  for (i = 0; i < array_n(col_array); i++) {
      col = array_fetch(sm_col *, col_array, i);
      sm_delcol(matrix, col->col_num);
  }

}

array_t *
sm_get_rows_covered_by_col(matrix, col)
  sm_matrix *matrix;
  sm_col *col;
{
  array_t *row_array;
  sm_element *p;
  sm_row *row;

  row_array = array_alloc(sm_row *, 0);
  sm_foreach_col_element(col, p) {
      assert(row = sm_get_row(matrix, p->row_num));
      array_insert_last(sm_row *, row_array, row);
  }      
  return row_array;
}

array_t *
sm_get_cols_covered_by_row(matrix, row)
  sm_matrix *matrix;
  sm_row *row;
{
  array_t *col_array;
  sm_element *p;
  sm_col *col;

  col_array = array_alloc(sm_col *, 0);
  sm_foreach_row_element(row, p) {
      assert(col = sm_get_col(matrix, p->col_num));
      array_insert_last(sm_col *, col_array, col);
  }      
  return col_array;
}

