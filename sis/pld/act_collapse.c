/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/pld/act_collapse.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:55 $
 *
 */
/* Aug 21, 1991 - changed the collapse routine for node to take care of
   no duplication of the network when a collapse is considered. */

#include "math.h"
#include "pld_int.h"
#include "sis.h"

static node_t *act_partial_collapse_find_max_score();

/*----------------------------------------------------------------------------------
  This file has routines for partial collapse of a node without using lindo.
----------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------------
  As obvious from the name, it does partial collapse on the network when it
finds that lindo is not in the path. Assigns a score to each nodewhich is a
measure of the gain that might be obtained by collapsing the node into each of
the fanouts. This score is just based on the number of fanins, fanouts, cost of
node etc. Updates the score after each successful partial collapse. Tries to
collapse till all nodes get score 0, or have been tried for  collapse, but
unsuccessfully.
------------------------------------------------------------------------------------*/
int act_partial_collapse_without_lindo(network, cost_table,
                                       init_param) network_t *network;
st_table *cost_table;
act_init_param_t *init_param;
{
  st_table *score_table;
  int collapsed, gain, total_gain, score;
  node_t *node, *act_partial_collapse_find_max_score();

  score_table = st_init_table(strcmp, st_strhash);
  act_partial_collapse_assign_score_network(network, score_table, cost_table,
                                            init_param);

  total_gain = 0;
  while (1) {
    node = act_partial_collapse_find_max_score(network, score_table, &score);
    if (score == 0) {
      st_free_table(score_table);
      if (ACT_DEBUG)
        (void)printf("****gain in partial collapse without lindo is %d\n",
                     total_gain);
      return total_gain;
    }
    if (ACT_DEBUG)
      (void)printf("node from score_table = %s\n", node_long_name(node));
    collapsed = act_partial_collapse_node(network, node, cost_table, init_param,
                                          &gain, score_table);
    if (collapsed) {
      if (ACT_DEBUG)
        (void)printf("collapsed node %s\n", node_long_name(node));
      total_gain += gain;
    }
  }
}

/*---------------------------------------------------------------------------------------
  Checks if it is beneficial to collapse node into each of the fanouts. If so,
does that and returns 1 and the gain in pgain. Changes the cost_table entries of
these fanouts and deletes the entry of node. Else returns 0 and pgain is not
assigned any values. Changed Aug 21, 1991 - no duplication of network now...
-----------------------------------------------------------------------------------------*/

int act_partial_collapse_node(network, node, cost_table, init_param, pgain,
                              score_table) network_t *network;
node_t *node;
st_table *cost_table;
act_init_param_t *init_param;
int *pgain;
st_table *score_table;
{
  int gain, old_cost, score;
  char *name, *name1, *dummy;
  node_t *fanout, *fanout_simpl, *dup_fanout;
  lsGen genfo;
  COST_STRUCT *cost_struct;
  st_table *fo_table;
  array_t *fanout_array, *real_fanout_array;
  int i, num_fanout;

  fo_table = st_init_table(strcmp, st_strhash);
  name = node_long_name(node);
  assert(st_lookup(cost_table, name, &dummy));
  gain = ((COST_STRUCT *)dummy)->cost;

  /* compute the gain if the node is collapsed into each of its
     fanouts and is then removed from the network. The cost is stored in the
     fo_table for each of the fanouts of the node                           */
  /*------------------------------------------------------------------------*/

  /* for each fanout, recompute the cost */
  /*-------------------------------------*/
  real_fanout_array = array_alloc(node_t *, 0);
  foreach_fanout(node, genfo, fanout) {
    array_insert_last(node_t *, real_fanout_array, fanout);
  }
  num_fanout = array_n(real_fanout_array);
  fanout_array = array_alloc(node_t *, 0);
  for (i = 0; i < num_fanout; i++) {
    fanout = array_fetch(node_t *, real_fanout_array, i);
    dup_fanout = node_dup(fanout);
    (void)node_collapse(dup_fanout, node);
    fanout_simpl = node_simplify(dup_fanout, NIL(node_t), NODE_SIM_ESPRESSO);
    /*
    node_free(dup_fanout);
    dup_fanout = fanout_simpl;
    */
    node_replace(dup_fanout, fanout_simpl);
    array_insert_last(node_t *, fanout_array, dup_fanout);
    /* a hack to get -h 1 option to work */
    dup_fanout->network = fanout->network;
    cost_struct = act_evaluate_map_cost(dup_fanout, init_param, NIL(network_t),
                                        NIL(array_t), NIL(st_table));
    dup_fanout->network = NIL(network_t);
    if (ACT_DEBUG) {
      (void)printf("new cost of fanout node %s = %d\n",
                   node_long_name(dup_fanout), cost_struct->cost);
    }
    /* put the names in act vertices now */
    /*-----------------------------------*/
    act_partial_collapse_update_act_fields(network, dup_fanout, cost_struct);
    name = node_long_name(dup_fanout);
    assert(!st_insert(fo_table, name, (char *)cost_struct));
    assert(st_lookup(cost_table, name, &dummy));
    old_cost = ((COST_STRUCT *)dummy)->cost;
    gain = gain + old_cost - (cost_struct->cost);
  }
  if (gain <= 0) {
    /* set the score of node to 0, free the storage */
    /*----------------------------------------------*/
    name = node_long_name(node);
    assert(st_insert(score_table, name, (char *)0));
    free_cost_table_without_freeing_key(fo_table);
    for (i = 0; i < num_fanout; i++) {
      dup_fanout = array_fetch(node_t *, fanout_array, i);
      node_free(dup_fanout);
    }
    array_free(fanout_array);
    array_free(real_fanout_array);
    return 0;
  }

  for (i = 0; i < num_fanout; i++) {
    dup_fanout = array_fetch(node_t *, fanout_array, i);
    fanout = array_fetch(node_t *, real_fanout_array, i);
    /* collapse node into fanout, update the cost_table entry */
    /*--------------------------------------------------------*/
    name1 = node_long_name(fanout);
    if (ACT_DEBUG)
      assert(!strcmp(name1, node_long_name(dup_fanout))); /* checking */
    name = util_strsav(name1);
    assert(st_delete(cost_table, &name1, &dummy));
    (void)free_cost_struct((COST_STRUCT *)dummy);
    FREE(name1);
    assert(node_long_name(fanout) != NIL(char));
    assert(st_lookup(fo_table, name, (char **)&cost_struct));
    assert(!st_insert(cost_table, name, (char *)cost_struct));
    /* updating the score of the fanout */
    /*----------------------------------*/
    (void)act_partial_collapse_assign_score_node(fanout, score_table,
                                                 cost_table, init_param);
    (void)node_replace(fanout, dup_fanout);
  } /* for i = 0 */
  array_free(fanout_array);
  array_free(real_fanout_array);
  st_free_table(fo_table);
  /* delete the collapsed node from cost_table, network.*/
  /*----------------------------------------------------*/
  name1 = node_long_name(node);
  assert(st_delete(score_table, &name1, (char **)&score));
  assert(st_delete(cost_table, &name1, &dummy));
  (void)free_cost_struct((COST_STRUCT *)dummy);
  FREE(name1);
  network_delete_node(network, node);
  *pgain = gain;
  return 1;
}

int act_partial_collapse_assign_score_node(node, score_table, cost_table,
                                           init_param) node_t *node;
st_table *score_table;
st_table *cost_table;
act_init_param_t *init_param;
{
  int score, num_fanin, cost, num_fanouts, i;
  node_t *fanout;
  char *name;

  if (node->type != INTERNAL)
    return (-1);
  name = node_long_name(node);
  num_fanin = node_num_fanin(node);
  if ((num_fanin > init_param->FANIN_COLLAPSE) || (is_anyfo_PO(node))) {
    /* give a score of 0 to this node */
    /*--------------------------------*/
    (void)st_insert(score_table, name, (char *)0);
    return 0;
  }
  cost = cost_of_node(node, cost_table);
  if (cost > 3) {
    (void)st_insert(score_table, name, (char *)0);
    return 0;
  }
  if (cost == 0)
    cost = 1;
  num_fanouts = node_num_fanout(node);
  score = 0;
  for (i = 0; i < num_fanouts; i++) {
    fanout = node_get_fanout(node, i);
    score += cost_of_node(fanout, cost_table);
  }
  score = (int)ceil((double)(score / cost));
  (void)st_insert(score_table, name, (char *)score);
  return score;
}

/*-----------------------------------------------------------
  Returns the node with the highest score in the score_table.
  The corresponding score is set in pscore. Assumes that all
  scores are nonnegative.
-------------------------------------------------------------*/
static node_t *act_partial_collapse_find_max_score(network, score_table,
                                                   pscore) network_t *network;
st_table *score_table;
int *pscore;
{
  lsGen gen;
  node_t *node, *node_max;
  int max_score, score;

  max_score = -1;
  node_max = NIL(node_t);

  foreach_node(network, gen, node) {
    if (node->type != INTERNAL)
      continue;
    assert(st_lookup_int(score_table, node_long_name(node), &score));
    if (score > max_score) {
      max_score = score;
      node_max = node;
    }
  }
  *pscore = max_score;
  return node_max;
}

/*--------------------------------------------------------------------------------
  Assigns a score to each internal node. If the cost of the node is > 3, it is
0. Else it is based on sum of costs of the fanout nodes and the cost of the
node. Stores all the costs in the score_table.
---------------------------------------------------------------------------------*/
int act_partial_collapse_assign_score_network(network, score_table, cost_table,
                                              init_param) network_t *network;
st_table *score_table;
st_table *cost_table;
act_init_param_t *init_param;
{
  array_t *nodevec;
  int i, num_nodes, score;
  node_t *node;

  nodevec = network_dfs(network);
  num_nodes = array_n(nodevec);
  /* assign score to all the internal nodes */
  /*----------------------------------------*/
  for (i = 0; i < num_nodes; i++) {
    node = array_fetch(node_t *, nodevec, i);
    score = act_partial_collapse_assign_score_node(node, score_table,
                                                   cost_table, init_param);
    if (ACT_DEBUG && (score != -1))
      (void)printf("---assigned score of %d to node %s\n", score,
                   node_long_name(node));
  }
  array_free(nodevec);
}
