/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/pld/xln_filter.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:56 $
 *
 */
#include "math.h"
#include "pld_int.h"
#include "sis.h"

/*---------------------------------------------------------------------------
  This file contains routines which detect filter condition under which
  a node can be essentially absorbed in its fanins.
----------------------------------------------------------------------------*/

xln_absorb_nodes_in_fanins(network, support) network_t *network;
int support;
{
  array_t *Y, *Z;
  array_t *nodevec;
  array_t *decomp_vec;
  int i, j, k;
  node_t *node, *n, *fanin;
  array_t *xln_can_node_be_absorbed_in_fanins();

  nodevec = network_dfs(network);
  for (i = 0; i < array_n(nodevec); i++) {
    node = array_fetch(node_t *, nodevec, i);
    if (node->type != INTERNAL)
      continue;
    decomp_vec = xln_can_node_be_absorbed_in_fanins(node, support, &Y, &Z);
    if (decomp_vec == NIL(array_t))
      continue;

    /* found a decomposition */
    /*-----------------------*/
    if (XLN_DEBUG) {
      (void)printf("----found a valid decomposition\n");
    }
    for (j = 1; j < array_n(decomp_vec); j++) {
      n = array_fetch(node_t *, decomp_vec, j);
      network_add_node(network, n);
      for (k = 0; k < array_n(Y); k++) {
        fanin = array_fetch(node_t *, Y, k);
        (void)node_collapse(n, fanin);
      }
    }
    n = array_fetch(node_t *, decomp_vec, 0);
    node_replace(node, n);
    for (k = 0; k < array_n(Z); k++) {
      fanin = array_fetch(node_t *, Z, k);
      (void)node_collapse(node, fanin);
    }
    array_free(Y);
    array_free(Z);
    array_free(decomp_vec);
  }
  array_free(nodevec);
  (void)network_sweep(network);
}

/*-------------------------------------------------------------------------------
  Given a "feasible" node (for the time being), returns two arrays pY and pZ
  (which form a partition of the fanin-set of node), such that it is possible
  to absorb the node in the CLB's for fanins Y and Z. It returns the array of
  nodes which will replace the node in nodevec.
--------------------------------------------------------------------------------*/
array_t *xln_can_node_be_absorbed_in_fanins(node, support, pY, pZ) node_t *node;
int support;
array_t **pY, **pZ;
{

  array_t *nodevec; /* returned array: stores possible decomposition of node */
  array_t *Y, *Z;   /* for bound-set and free-set respectively */
  int num_fanin;    /* number of fanins of node */
  int num_comb;     /* 2 ** num_fanin */
  node_t *fanin;
  int i, k;
  int num_union_fanin_Y, num_union_fanin_Z;
  int bound1, bound2;
  int num_internal_Y, num_internal_Z, sum_internal;
  int bound_alphas;
  int *lambda_indices;
  array_t *faninvec;
  int support_x_2;
  int flag;

  /* return NIL if the support of the node is high */
  /*---------------------------------------------*/
  num_fanin = node_num_fanin(node);
  support_x_2 = 2 * support;

  if (num_fanin >= support_x_2)
    return NIL(array_t);

  /* return NIL if some fanin that is an
     intermediate node has multiple fanouts
     or if all the fanins are primary inputs     */
  /*---------------------------------------------*/
  flag = 0;
  foreach_fanin(node, k, fanin) {
    if (fanin->type == INTERNAL) {
      if (node_num_fanout(fanin) > 1)
        return NIL(array_t);
      flag = 1;
    }
  }
  if (!flag)
    return NIL(array_t);

  /* return NIL if the union of fanins of fanins is >= 2 * support */
  /*---------------------------------------------------------------*/
  faninvec = pld_get_array_of_fanins(node);
  if (xln_num_union_of_fanins(faninvec) > support_x_2) {
    array_free(faninvec);
    return NIL(array_t);
  }
  array_free(faninvec);

  /* try all possible partitions of the fanin-space one by one */
  /*-----------------------------------------------------------*/
  /* num_comb = (int) (pow ((double) 2, (double) num_fanin)); */
  /* added for robustness */
  /*----------------------*/
  if (num_fanin > 31)
    return NIL(array_t);
  num_comb = 1 << num_fanin;
  for (i = 0; i < num_comb; i++) {
    Y = array_alloc(node_t *, 0);
    Z = array_alloc(node_t *, 0);
    xln_generate_fanin_combination(node, i, Y, Z);
    if (XLN_DEBUG) {
      xln_print_absorb_node(node, Y, Z);
    }
    /* check if the partition is non-trivial */
    /*---------------------------------------*/
    if ((array_n(Y) <= 1) || (array_n(Z) == 0)) {
      array_free(Y);
      array_free(Z);
      continue;
    }

    /* if the union_fanin_support high, ignore the partition */
    /*-------------------------------------------------------*/
    num_union_fanin_Y = xln_num_union_of_fanins(Y); /* Y - bound set */
    num_union_fanin_Z = xln_num_union_of_fanins(Z); /* Z - free set */
    if ((num_union_fanin_Y > support) || (num_union_fanin_Z > support)) {
      array_free(Y);
      array_free(Z);
      continue;
    }
    /* calculate bounds on number of alphas (new nodes after decomposition */
    /* bound1 is based on the fact that the resulting node after decomposition
       should be feasible (for the time being - else do a cost estimation. */
    /*---------------------------------------------------------------------*/
    bound1 = support - num_union_fanin_Z; /* upper bounds num. of alphas */

    /* bound2 is calculated based on the observation that the saving is
       in fact, internal_Y + internal_Z - num_alphas. This should be >0*/
    /*----------------------------------------------------------------*/

    num_internal_Y = xln_num_internal_nodes(Y);
    num_internal_Z = xln_num_internal_nodes(Z);
    sum_internal = num_internal_Y + num_internal_Z;
    bound2 = sum_internal - 1;
    bound_alphas = min(bound1, bound2);
    if (XLN_DEBUG) {
      (void)printf("---num_union_fanin_Y = %d, num_union_fanin_Z = %d\n",
                   num_union_fanin_Y, num_union_fanin_Z);
    }
    /* try out this partition for decomposition */
    /*------------------------------------------*/
    lambda_indices = xln_array_to_indices(Y, node);
    nodevec = xln_k_decomp_node_with_array(node, lambda_indices, array_n(Y),
                                           bound_alphas);
    FREE(lambda_indices);
    if (nodevec == NIL(array_t)) {
      array_free(Y);
      array_free(Z);
      continue;
    }
    /* found the required partition */
    /*------------------------------*/
    *pY = Y;
    *pZ = Z;
    return nodevec;
  }
  return NIL(array_t);
}

/*---------------------------------------------------------------------
  Given a set of nodes (nodevec), returns the total number of fanins
  of the set. There may be primary inputs in the set.
----------------------------------------------------------------------*/
int xln_num_union_of_fanins(nodevec) array_t *nodevec;
{
  st_table *table;
  int i, j;
  int num; /* total number of fanins of all the nodes in nodevec */
  node_t *node, *fanin;

  table = st_init_table(st_ptrcmp, st_ptrhash);
  for (i = 0; i < array_n(nodevec); i++) {
    node = array_fetch(node_t *, nodevec, i);
    /* check this */
    /*------------*/
    if (node->type == PRIMARY_INPUT) {
      (void)st_insert(table, (char *)node, (char *)node);
      continue;
    }
    foreach_fanin(node, j, fanin) {
      (void)st_insert(table, (char *)fanin, (char *)fanin);
    }
  }
  num = st_count(table);
  if (XLN_DEBUG) {
    (void)printf("union of the fanins of nodes = %d\n", num);
  }
  st_free_table(table);
  return num;
}

/*-----------------------------------------------------------------------------
  This routine systematically generates ALL partitions (disjoint) of
  fanins of the node in the arrays pY and pZ. There are (2 ** num_fanin)
  possible combinations. The ith entries Y and Z in
  the two arrays form a partition of the fanins of node. The way used to
  generate these partitions is that first a binary representation is generated
  for the number "i" in num_fanin bits. If there is a 0 at a bit position, the
  corresponding fanin is put in the array Y, else in the array Z.
-------------------------------------------------------------------------------*/

xln_generate_all_fanin_combinations(node, pY, pZ) node_t *node;
array_t *pY, *pZ;

{
  int num_fanin;
  int num_comb;
  char *encoded_char;
  int i, l;
  node_t *fanin;
  array_t *Y, *Z; /* the two arrays storing the partition
                     of the fanins of the node */

  assert(node->type == INTERNAL);

  num_fanin = node_num_fanin(node);
  encoded_char = ALLOC(char, num_fanin + 1);
  /* num_comb = (int) (pow ((double) 2, (double) num_fanin)); */
  assert(num_fanin <= 31);
  num_comb = 1 << num_fanin;
  for (i = 0; i < num_comb; i++) {
    Y = array_alloc(node_t *, 0);
    Z = array_alloc(node_t *, 0);
    xl_binary1(i, num_fanin, encoded_char);
    for (l = 0; l < num_fanin; l++) {
      fanin = node_get_fanin(node, l);
      if (encoded_char[l] == '0') {
        array_insert_last(node_t *, Y, fanin);
      } else {
        assert(encoded_char[l] == '1');
        array_insert_last(node_t *, Z, fanin);
      }
    }
    array_insert_last(array_t *, pY, Y);
    array_insert_last(array_t *, pZ, Z);
  }
  FREE(encoded_char);
}

/*----------------------------------------------------------------------
  Generates a fanin combination corresponding to i. Returns in arrays
  Y and Z the partition of fanins corresponding to binary encoding of
  i.
-----------------------------------------------------------------------*/
xln_generate_fanin_combination(node, i, Y, Z) node_t *node;
int i;
array_t *Y, *Z; /* the two arrays storing the partition
                   of the fanins of the node */

{
  int num_fanin;
  char *encoded_char;
  int l;
  node_t *fanin;

  num_fanin = node_num_fanin(node);
  encoded_char = ALLOC(char, num_fanin + 1);
  xl_binary1(i, num_fanin, encoded_char);
  for (l = 0; l < num_fanin; l++) {
    fanin = node_get_fanin(node, l);
    if (encoded_char[l] == '0') {
      array_insert_last(node_t *, Y, fanin);
    } else {
      assert(encoded_char[l] == '1');
      array_insert_last(node_t *, Z, fanin);
    }
  }
  FREE(encoded_char);
}

xln_print_absorb_node(node, Y, Z) node_t *node;
array_t *Y;
array_t *Z;
{
  int i;
  node_t *fanin;

  (void)printf("candidate node for absorption => %s\n", node_long_name(node));
  (void)printf("---total number of fanins = %d\n", node_num_fanin(node));

  (void)printf("bound set: number of nodes = %d\n", array_n(Y));
  for (i = 0; i < array_n(Y); i++) {
    fanin = array_fetch(node_t *, Y, i);
    (void)printf(" %s, ", node_long_name(fanin));
  }
  (void)printf("\n");

  (void)printf("free set: number of nodes = %d\n", array_n(Z));
  for (i = 0; i < array_n(Z); i++) {
    fanin = array_fetch(node_t *, Z, i);
    (void)printf(" %s, ", node_long_name(fanin));
  }
  (void)printf("\n");
}

int xln_num_internal_nodes(nodevec) array_t *nodevec;
{
  int sum;
  int i;
  node_t *node;

  sum = 0;
  for (i = 0; i < array_n(nodevec); i++) {
    node = array_fetch(node_t *, nodevec, i);
    if (node->type == INTERNAL)
      sum++;
  }
  return sum;
}
