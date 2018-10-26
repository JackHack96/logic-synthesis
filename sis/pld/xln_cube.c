
#include "sis.h"
#include "pld_int.h"

node_t *xln_make_big_and();
extern int XLN_DEBUG;

/*----------------------------------------------------------------------
  Employs a bin-packing Best Fit Decreasing algorithm to decompose all 
  the nodes of the network to nodes with number of fanins at most size.
------------------------------------------------------------------------*/
xln_network_ao_map(network, size)
  network_t *network;
  int size;
{
  array_t *nodevec;
  int num, i;
  node_t *node;

  nodevec = network_dfs(network);
  num = array_n(nodevec);
  for (i = 0; i < num; i++) {
      node = array_fetch(node_t *, nodevec, i);
      xln_node_ao_map(node, size);
  }
  array_free(nodevec);
}

/*------------------------------------------------------------------------
  First maps cubes of the node with number of literals
  greater than size to a CLB and after that does an BFD packing of the 
  cubes. Hopefully a similar algorithm would work for minimizing levels 
  also.
-------------------------------------------------------------------------*/
xln_node_ao_map(node, size)
  node_t *node;
  int size;
{
  int num_cube, i, node_changed, cube_changed;
  node_cube_t cube;
  network_t *network;
  
  if (node->type != INTERNAL) return;
  if (node_num_fanin(node) <= size) return;

  network = node->network;
  node_changed = 1;
  while (node_changed) {
      node_changed = 0;
      num_cube = node_num_cube(node);
      for (i = 0; i < num_cube; i++) {
          cube = node_get_cube(node, i);
          cube_changed = xln_extract_big_ands_from_cube(network, node, cube, size);
          node_changed = node_changed || cube_changed;
      }
  }
  xln_ao_replace_node_with_tlus(network, node, size);
}

/*------------------------------------------------------------------------
  Given a node and a cube in the node, extracts supercubes that have
  at least size number of literals. This is done one supercube at a time.
  The supercube is extracted, made a node in the network and also 
  substituted in the node.
-------------------------------------------------------------------------*/
xln_extract_big_ands_from_cube(network, node, cube,  size)
  network_t *network;
  node_t *node;
  node_cube_t cube;
  int size;
{
  array_t *one_array, *zero_array, *andvec;
  int count, count1, count0, num_and, j, k;
  node_t *fanin, *node_and;
  node_literal_t literal;

  /* do not extract anything if the number of literals in the
     cube is not higher than size.                           */
  /*---------------------------------------------------------*/
  if (pld_num_fanin_cube(cube, node) <= size) return 0;

  one_array = array_alloc(node_t *, 0);
  zero_array = array_alloc(node_t *, 0);
  andvec = array_alloc(node_t *, 0);

  count = 0;
  count1 = 0;
  count0 = 0;
  foreach_fanin(node, j, fanin) {
      literal = node_get_literal(cube, j);
      switch(literal) {
        case ONE:
          array_insert(node_t *, one_array, count1, fanin);
          count1++;
          count++;
          break;
        case ZERO:
          array_insert(node_t *, zero_array, count0, fanin);
          count++;
          count0++;
          break;
        case TWO:
          break;
      }
      if (count == size) {
          /* found a BIG AND */
          /*-----------------*/
          node_and = xln_make_big_and(one_array, zero_array, count1, count0);
          array_insert_last(node_t *, andvec, node_and);
          count = 0;
          count1 = 0;
          count0 = 0;
      }
  }
  num_and = array_n(andvec);
  if (num_and != 0) {
      for (k = 0; k < num_and; k++) {
          node_and = array_fetch(node_t *, andvec, k);
          network_add_node(network, node_and);
          node_substitute(node, node_and, 0); 
      }
      array_free(one_array);
      array_free(zero_array);
      array_free(andvec);
      /* xln_extract_big_ands_from_cube(network, node, pcube, size); */
      return 1;
  }
  else {
      array_free(one_array);
      array_free(zero_array);
      array_free(andvec);
      return 0;
      /*NOTREACHED */
  }
}

/*------------------------------------------------------------------------
  Given two arrays and the number of elements in them, returns a node
  that is AND of the nodes in the two arrays. The phases of literals
  is determined by which array the literal belongs to. one_array stores
  uncomplemented literals.
  ------------------------------------------------------------------------*/
node_t *
xln_make_big_and(one_array, zero_array, num_one, num_zero)
  array_t *one_array, *zero_array;
  int num_one, num_zero;
{
  int i;
  node_t *n, *n1, *n2, *fanin;

  n = node_constant(1);
  for (i = 0; i < num_one; i++) {
      fanin = array_fetch(node_t *, one_array, i);
      n1 = node_literal(fanin, 1);
      n2 = node_and(n, n1);
      node_free(n1);
      node_free(n);
      n = n2;
  }
  for (i = 0; i < num_zero; i++) {
      fanin = array_fetch(node_t *, zero_array, i);
      n1 = node_literal(fanin, 0);
      n2 = node_and(n, n1);
      node_free(n1);
      node_free(n);
      n = n2;
  }
  return n;
}


/*---------------------------------------------------------------------
  Given a node, if the number of fanins is greater than size, replaces
  it by table look up (tlu) blocks.
-----------------------------------------------------------------------*/
xln_ao_replace_node_with_tlus(network, node, size)
  network_t *network;
  node_t *node;
  int size;
{
  int xln_ao_compare(), num_cube, i, num_tlu, num_tlu1;
  array_t *cubevec, *tluvec;
  node_cube_t cube;
  node_t *cube_node, *tlu, *tlu1, *prev_tlu;
  
  if (node_num_fanin(node) <= size) return;

  /* form nodes from all the cubes and store in cubevec */
  /*----------------------------------------------------*/
  num_cube = node_num_cube(node);
  cubevec = array_alloc(node_t *, 0);
  for (i = 0; i < num_cube; i++) {
      cube = node_get_cube(node, i);
      cube_node = pld_make_node_from_cube(node, cube);
      array_insert_last(node_t *, cubevec, cube_node);
  }

  /* making lookup tables (tlu's) now */
  /*----------------------------------*/
  tluvec = array_alloc(node_t *, 0);
  while (array_n(cubevec) != 0) {
      array_sort(cubevec, xln_ao_compare);
      if (XLN_DEBUG > 3) {
          (void) printf("---printing cube nodes for %s\n", node_long_name(node));
          num_cube = array_n(cubevec);
          for (i = 0; i < num_cube; i++) {
              cube_node = array_fetch(node_t *, cubevec, i);
              (void) node_print_rhs(sisout, cube_node);
              (void) printf("(fanins = %d) ", node_num_fanin(cube_node));
          }
          (void) printf("\n\n");
      }
      /* this makes tlu's and puts them in tluvec. Each tlu generates
         a new single-literal cube which is put in cubevec          */
      /*------------------------------------------------------------*/
      xln_make_tlus(network, node, &cubevec, &tluvec, size);
  }
  array_free(cubevec);

  /* now just make the first guy in tluvec a node, add it in the
     network and continue propagating towards the end of tluvec */
  /*------------------------------------------------------------*/
  num_tlu = array_n(tluvec);
  num_tlu1 = num_tlu - 1;
  prev_tlu = node_constant(0);
  for (i = 0; i < num_tlu; i++) {
      tlu = array_fetch(node_t *, tluvec, i);
      tlu1 = node_or(prev_tlu, tlu);
      node_free(prev_tlu);
      node_free(tlu);
      if (i != num_tlu1) {
          network_add_node(network, tlu1);
          prev_tlu = node_literal(tlu1, 1);
      }
      else node_replace(node, tlu1);
  }
  array_free(tluvec);
  
}

/*-----------------------------------------------------------------------
  Given some nodes as tlu's in tluvec (possibly empty), and nodes in
  pcubevec which are to be realized as tlu's, pack the cubes. This is
  a complicated implementation, consisting of two phases:

  1) Each cube in pcubevec is checked to see if it can be implemented 
     by some tlu in ptluvec. If so, change 
     the function of that tlu appropriately (OR with the cube). Else,
     generate a new tlu, put it in ptluvec. 

  2) After all cubes in pcubevec have been thus processed, the tlu's 
     in ptluvec are checked to see if some tlu has number of inputs 
     = size. If so, that tlu is added as a node in the network, it
     generates a cube with single literal, which is then put in the
     pcubevec. Such a tlu is deleted from the ptluvec, as it cannot 
     accomodate any more cubes (all the new sinle-literal cubes have 
     different fanin).
-------------------------------------------------------------------------*/
int
xln_make_tlus(network, node, pcubevec, ptluvec, size)
  network_t *network;
  node_t *node;
  array_t **pcubevec, **ptluvec;
  int size;
{
  int num_tlu, num_cubevec, i, tlu_num, j;
  array_t *cubevec, *tluvec, *newtluvec;
  node_t *cube_node, *tlu, *tlu1;

  cubevec = *pcubevec;
  tluvec = *ptluvec;
  num_tlu = array_n(tluvec);
  num_cubevec = array_n(cubevec);

  /* absorb cube_node either into already existing tlu's or make a new tlu */
  /*-----------------------------------------------------------------------*/
  for (i= 0; i < num_cubevec; i++) {
      cube_node = array_fetch(node_t *, cubevec, i);
      tlu_num = xln_is_cube_absorbed(cube_node, tluvec, size);
      if (tlu_num < 0) {
          array_insert_last(node_t *, tluvec, node_dup(cube_node));
          num_tlu++;
      }
      node_free(cube_node);
  }
  array_free(cubevec);

  /* now have a second pass over tlu's to absorb all "size"-input tlu's */
  /*--------------------------------------------------------------------*/
  array_sort(tluvec, xln_ao_compare);
  cubevec = array_alloc(node_t *, 0);
  if (num_tlu == 1) {
      tlu = array_fetch(node_t *, tluvec, 0);
      node_replace(node, tlu);
      array_free(tluvec);
      newtluvec = array_alloc(node_t *, 0);
      *ptluvec = newtluvec;
  }
  else {
      for (i = 0; i < num_tlu; i++) {
          tlu = array_fetch(node_t *, tluvec, i);
          if (node_num_fanin(tlu) < size) break;
          network_add_node(network, tlu);
          tlu1 = node_literal(tlu, 1);
          array_insert_last(node_t *, cubevec, tlu1);
      }
      /* update the tlu list - no size input tlu's remain */
      /*--------------------------------------------------*/
      if (i != 0) {
          newtluvec = array_alloc(node_t *, 0);
          for ( j = i; j < num_tlu; j++) {
              tlu = array_fetch(node_t *, tluvec, j);
              array_insert_last(node_t *, newtluvec, tlu);
          }
          array_free(tluvec);
          *ptluvec = newtluvec;
      }
  } 
  *pcubevec = cubevec;
}

int
xln_ao_compare(pn1, pn2)
  node_t **pn1, **pn2;
{
  int num1, num2;
  
  num1 = node_num_fanin(*pn1);
  num2 = node_num_fanin(*pn2);
  if (num1 > num2) return -1;
  if (num1 == num2) return 0;
  return 1;
}

/*---------------------------------------------------------------------
  If the cube_node is mergeable with previous tlu's, do so and return
  the array location of tlu with which it was absorbed. Find the best 
  fit (that is the tlu in which the num_composite_fanin is minimum).
  Else return -1. tluvec has all the tlu's generated so far.
-----------------------------------------------------------------------*/       
int
xln_is_cube_absorbed(cube_node, tluvec, size)
  node_t *cube_node;
  array_t *tluvec;
  int size;
{
  int num, j, tlu_num, num_composite_fanin, best_num_composite_fanin;
  node_t *tlu, *new_tlu;

  tlu_num = -1;
  best_num_composite_fanin = MAXINT;

  /* get the best tlu that can absorb cube_node */
  /*--------------------------------------------*/
  num = array_n(tluvec);
  for (j = 0; j < num; j++) {
      tlu = array_fetch(node_t *, tluvec, j);
      num_composite_fanin = xln_num_composite_fanin(cube_node, tlu);
      if (num_composite_fanin <= size) {
          if (num_composite_fanin < best_num_composite_fanin) {
              best_num_composite_fanin = num_composite_fanin;
              tlu_num = j;
          }
      }
  }
  if (tlu_num < 0) return tlu_num;
  /* found a good tlu: merge the cube_node with it */
  /*-----------------------------------------------*/
  tlu = array_fetch(node_t *, tluvec, tlu_num);
  new_tlu = node_or(cube_node, tlu);
  node_free(tlu);
  array_insert(node_t *, tluvec, tlu_num, new_tlu);
  return tlu_num;
}
              
