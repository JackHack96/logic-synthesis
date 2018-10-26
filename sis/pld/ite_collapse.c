#include "sis.h"
#include "pld_int.h"
#include "ite_int.h"
#include "math.h" 

static node_t * act_ite_partial_collapse_find_max_score();

/* Aug 21, 1991 - changed the collapse routine for node to take care of 
   no duplication of the network when a collapse is considered. */

/*-----------------------------------------------------------------------------------
  As obvious from the name, it does partial collapse on the network.
  Assigns a score to each nodewhich is a measure of
  the gain that might be obtained by collapsing the node into each of the fanouts.
  This score is just based on the number of fanins, fanouts, cost of node etc.
  Updates the score after each successful partial collapse. Tries to collapse
  till all nodes get score 0, or have been tried for  collapse, but unsuccessfully.
------------------------------------------------------------------------------------*/
int    
act_ite_partial_collapse(network, init_param)
  network_t *network;
  act_init_param_t *init_param;
{
  st_table *score_table;
  int collapsed, gain, total_gain, score;
  node_t *node, *act_ite_partial_collapse_find_max_score();

  if (! init_param->FANIN_COLLAPSE) return 0;

  score_table = st_init_table(strcmp, st_strhash);
  act_ite_partial_collapse_assign_score_network(network, score_table,  init_param);

  total_gain = 0;
  while (1) {
      node = act_ite_partial_collapse_find_max_score(network, score_table, &score);
      if (score == 0) {
          st_free_table(score_table);
          if (ACT_ITE_DEBUG) 
              (void) printf("****gain in partial collapse is %d\n", total_gain);
          return total_gain;
      }
      if (ACT_ITE_DEBUG > 2) (void) printf("node from score_table = %s\n", node_long_name(node));
      if (init_param->COLLAPSE_METHOD == NEW) {
          collapsed = act_ite_partial_collapse_node_new(network, node,  init_param,
                                                    &gain, score_table);
      } else {
          collapsed = act_ite_partial_collapse_node(network, node,  init_param,
                                                    &gain, score_table);
      }         
      if (collapsed) {
          total_gain += gain;
      }
  }  
}

/*---------------------------------------------------------------------------------------
  Checks if it is beneficial to collapse node into each of the fanouts. If so, does that 
  and returns 1 and the gain in pgain. Changes the cost_table entries of these fanouts 
  and deletes the entry of node. Else returns 0 and pgain is not assigned any values.
  Changed Aug 21, 1991 - no duplication of network now...
-----------------------------------------------------------------------------------------*/

int 
act_ite_partial_collapse_node(network, node,  init_param, pgain, score_table)
  network_t *network;
  node_t *node;
  act_init_param_t *init_param;
  int *pgain;
  st_table *score_table;
{
  int gain;
  char *name, *value;
  node_t *fanout, /* *fanout_simpl */ *dup_fanout, *fanin;
  lsGen genfo;
  ACT_ITE_COST_STRUCT *cost_struct, *cost_node;
  array_t *fanout_array, *real_fanout_array;
  int i, j, num_fanout, num_composite;
  st_table *update_nodetable;
  st_generator *stgen;

  /* check that number of fanins of each fanout are not too many */
  /*-------------------------------------------------------------*/
  foreach_fanout(node, genfo, fanout) {
      num_composite = xln_num_composite_fanin(node, fanout);
      if (num_composite <= init_param->COLLAPSE_FANINS_OF_FANOUT) continue;
      /* set the score of node to 0, free the storage */
      /*----------------------------------------------*/
      name = node_long_name(node);
      assert(st_insert(score_table, name, (char *) 0));          
      return 0;
  }
  cost_node = ACT_ITE_SLOT(node);
  gain = cost_node->cost;
  foreach_fanout(node, genfo, fanout) {
      gain += ACT_ITE_SLOT(fanout)->cost;
  }
  /* gain now is the max gain possible */

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
     (void) node_collapse(dup_fanout, node);
     simplify_node(dup_fanout, SIM_METHOD_SNOCOMP, SIM_DCTYPE_NONE, SIM_FILTER_NONE, 
                   SIM_ACCEPT_SOP_LITS);
     /* fanout_simpl = node_simplify(dup_fanout, NIL(node_t), NODE_SIM_ESPRESSO);
     node_replace(dup_fanout, fanout_simpl); */
     array_insert_last(node_t *, fanout_array, dup_fanout);
     /* a hack to get -h 1 option to work */
     dup_fanout->network = fanout->network;

     switch(init_param->MAP_METHOD) {
       case MAP_WITH_ITER:;
       case MAP_WITH_JUST_DECOMP:
         act_ite_map_node_with_iter_imp(dup_fanout, init_param);
         break;
       case NEW:
         (void) act_ite_new_map_node(dup_fanout, init_param);
         break;
       case OLD:
         (void) act_ite_map_node(dup_fanout, init_param);
         break;
       default: 
         (void) printf("mapping method %d not known\n", init_param->MAP_METHOD);
         exit(1);
     }

     cost_struct = ACT_ITE_SLOT(dup_fanout);
     dup_fanout->network = NIL (network_t);
     if (ACT_ITE_DEBUG > 2) {
         (void) printf("new cost of fanout node %s = %d, old_cost = %d\n", node_long_name(dup_fanout),
                       cost_struct->cost, ACT_ITE_SLOT(fanout)->cost);
     }
     /* put the names in act vertices now - commented because not breaking yet */
     /*------------------------------------------------------------------------*/
     if (init_param->MAP_METHOD == NEW || init_param->MAP_METHOD == OLD) {
         act_ite_partial_collapse_update_ite_fields(network, dup_fanout, cost_struct); 
     }
     gain -= cost_struct->cost;
     if (gain <= 0) break; /* now you may not have to go through all the fanouts before
                              you find out that the collapse is no good */
  }
  if (gain <= 0) {
      /* set the score of node to 0, free the storage */
      /*----------------------------------------------*/
      name = node_long_name(node);
      assert(st_insert(score_table, name, (char *) 0));          
      for (j = 0; j <= i; j++) {
          dup_fanout = array_fetch(node_t *, fanout_array, j);
          act_free_ite_node(dup_fanout);
          node_free(dup_fanout);
      }
      array_free(fanout_array);
      array_free(real_fanout_array);
      return 0;
  }

  for (i = 0; i < num_fanout; i++) {
     dup_fanout = array_fetch(node_t *, fanout_array, i);
     fanout = array_fetch(node_t *, real_fanout_array, i);
     act_free_ite_node(fanout);
     act_ite_free(fanout);
     ACT_DEF_ITE_SLOT(fanout) = ACT_DEF_ITE_SLOT(dup_fanout);
     ACT_DEF_ITE_SLOT(dup_fanout) = NIL (char);
     /* updating the score of the fanout */
     /*----------------------------------*/
     act_ite_partial_collapse_assign_score_node(fanout, score_table,  init_param);
     (void) node_replace(fanout, dup_fanout);
  } /* for i = 0 */
  array_free(fanout_array);

  /* change the scores of the fanins of the node, and the fanins of the fanouts 
     since the fanout information has changed - added Dec. 1, 1992             */
  /*---------------------------------------------------------------------------*/
  if (init_param->COLLAPSE_UPDATE == EXPENSIVE) {
      update_nodetable = st_init_table(st_ptrcmp, st_ptrhash);
      foreach_fanin(node, i, fanin) {
          (void) st_insert(update_nodetable, (char *) fanin, (char *) fanin);
      }
  }

  /* delete the collapsed node from network.*/  
  /*----------------------------------------*/
  if (ACT_ITE_DEBUG > 1) (void) printf("collapsed node %s\n", node_long_name(node)); 
  act_free_ite_node(node);
  network_delete_node(network, node); 

  if (init_param->COLLAPSE_UPDATE == EXPENSIVE) {
      for (i = 0; i < array_n(real_fanout_array); i++) {
          fanout = array_fetch(node_t *, real_fanout_array, i);
          foreach_fanin(fanout, j, fanin) {
              (void) st_insert(update_nodetable, (char *) fanin, (char *) fanin);
          }
      }
  
      st_foreach_item(update_nodetable, stgen, (char **) &fanin, &value) {
          act_ite_partial_collapse_assign_score_node(fanin, score_table,  init_param);      
      }
      st_free_table(update_nodetable);
  }
  array_free(real_fanout_array);
  *pgain = gain;
  return 1;
}

/*---------------------------------------------------------------------------------------
  Is potentially more powerful than act_ite_partial_collapse_node(), in that, even if 
  the total collapse is not beneficial, it may happen that collapsing node into some 
  fanouts (those in good_array) may be benficial. In that case, this is done. 
  If the overall gain is not positive (because of fanouts whose cost increases as 
  a result of this collapse - ie in bad_array), node will not be deleted from network. 
-----------------------------------------------------------------------------------------*/

int 
act_ite_partial_collapse_node_new(network, node,  init_param, pgain, score_table)
  network_t *network;
  node_t *node;
  act_init_param_t *init_param;
  int *pgain;
  st_table *score_table;
{
  int gain_good, gain_bad, gain_node;
  char *name, *value;
  node_t *fanout, /* *fanout_simpl, */ *dup_fanout, *fanin;
  lsGen genfo;
  ACT_ITE_COST_STRUCT *cost_struct, *cost_node;
  array_t *fanout_array, *real_fanout_array, *good_array, *bad_array;
  int i, j, num_fanout, num_composite, index;
  st_table *update_nodetable;
  st_generator *stgen;

  /* check that number of fanins of each fanout are not too many */
  /*-------------------------------------------------------------*/
  foreach_fanout(node, genfo, fanout) {
      num_composite = xln_num_composite_fanin(node, fanout);
      if (num_composite <= init_param->COLLAPSE_FANINS_OF_FANOUT) continue;
      /* set the score of node to 0, free the storage */
      /*----------------------------------------------*/
      name = node_long_name(node);
      assert(st_insert(score_table, name, (char *) 0));          
      return 0;
  }

  cost_node = ACT_ITE_SLOT(node);
  gain_bad = cost_node->cost;
  gain_good = 0; 

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
  good_array = array_alloc(int, 0);
  bad_array = array_alloc(int, 0);

  for (i = 0; i < num_fanout; i++) {
     fanout = array_fetch(node_t *, real_fanout_array, i);
     dup_fanout = node_dup(fanout);
     (void) node_collapse(dup_fanout, node);
     simplify_node(dup_fanout, SIM_METHOD_SNOCOMP, SIM_DCTYPE_NONE, SIM_FILTER_NONE, 
                   SIM_ACCEPT_SOP_LITS);
     array_insert_last(node_t *, fanout_array, dup_fanout);
     /* a hack to get -h 1 option to work */
     dup_fanout->network = fanout->network;
     switch(init_param->MAP_METHOD) {
       case MAP_WITH_ITER:;
       case MAP_WITH_JUST_DECOMP:
         act_ite_map_node_with_iter_imp(dup_fanout, init_param);
         break;
       case NEW:
         (void) act_ite_new_map_node(dup_fanout, init_param);
         break;
       case OLD:
         (void) act_ite_map_node(dup_fanout, init_param);
         break;
       default: 
         (void) printf("mapping method %d not known\n", init_param->MAP_METHOD);
         exit(1);
     }

     cost_struct = ACT_ITE_SLOT(dup_fanout);
     dup_fanout->network = NIL (network_t);
     if (ACT_ITE_DEBUG > 2) {
         (void) printf("new cost of fanout node %s = %d, old_cost = %d\n", node_long_name(dup_fanout),
                       cost_struct->cost, ACT_ITE_SLOT(fanout)->cost);
     }
     /* put the names in act vertices now */
     /*-----------------------------------*/
     if (init_param->MAP_METHOD == NEW || init_param->MAP_METHOD == OLD) {
         act_ite_partial_collapse_update_ite_fields(network, dup_fanout, cost_struct); 
     }
     gain_node = ACT_ITE_SLOT(fanout)->cost - cost_struct->cost;
     if (gain_node > 0) {
         gain_good += gain_node;
         array_insert_last(int, good_array, i);
     } else {
         gain_bad += gain_node;
         array_insert_last(int, bad_array, i);
     }
  }

  if (gain_bad <= 0) {
      /* set the score of node to 0, free the storage */
      /*----------------------------------------------*/
      name = node_long_name(node);
      assert(st_insert(score_table, name, (char *) 0));          
      for (j = 0; j < array_n(bad_array); j++) {
          index = array_fetch(int, bad_array, j);
          dup_fanout = array_fetch(node_t *, fanout_array, index);
          act_free_ite_node(dup_fanout);
          node_free(dup_fanout); /* act_free_ite() is called from within node_free() */
      }
  } else {
      for (j = 0; j < array_n(bad_array); j++) {
          index = array_fetch(int, bad_array, j);
          dup_fanout = array_fetch(node_t *, fanout_array, index);
          fanout = array_fetch(node_t *, real_fanout_array, index);
          act_free_ite_node(fanout);
          act_ite_free(fanout);
          ACT_DEF_ITE_SLOT(fanout) = ACT_DEF_ITE_SLOT(dup_fanout);
          ACT_DEF_ITE_SLOT(dup_fanout) = NIL (char);
          /* updating the score of the fanout */
          /*----------------------------------*/
          act_ite_partial_collapse_assign_score_node(fanout, score_table,  init_param);
          (void) node_replace(fanout, dup_fanout);
      }
  }

  for (j = 0; j < array_n(good_array); j++) {
      index = array_fetch(int, good_array, j);
      dup_fanout = array_fetch(node_t *, fanout_array, index);
      fanout = array_fetch(node_t *, real_fanout_array, index);
      act_free_ite_node(fanout);
      act_ite_free(fanout);
      ACT_DEF_ITE_SLOT(fanout) = ACT_DEF_ITE_SLOT(dup_fanout);
      ACT_DEF_ITE_SLOT(dup_fanout) = NIL (char);
      /* updating the score of the fanout */
      /*----------------------------------*/
      act_ite_partial_collapse_assign_score_node(fanout, score_table,  init_param);
      (void) node_replace(fanout, dup_fanout);
  } 
  
  array_free(fanout_array);

  if (gain_bad > 0) {
      /* delete the collapsed node from network - not deleting from 
         score_table since only nodes in the network are searched 
         when finding the best node                              */  
      /*---------------------------------------------------------*/
      if (ACT_ITE_DEBUG > 1) (void) printf("collapsed node %s into all the fanouts\n", node_long_name(node)); 
      act_free_ite_node(node);
      network_delete_node(network, node); 
  }

  if (init_param->COLLAPSE_UPDATE == EXPENSIVE) {
      update_nodetable = st_init_table(st_ptrcmp, st_ptrhash);

      /* gain_good > 0 else no member in the array  */
      /*--------------------------------------------*/
      for (i = 0; i < array_n(good_array); i++) {
          index = array_fetch(int, good_array, i);
          fanout = array_fetch(node_t *, real_fanout_array, index);
          foreach_fanin(fanout, j, fanin) {
              (void) st_insert(update_nodetable, (char *) fanin, (char *) fanin);
          }
      }
      if (gain_bad > 0) {
          for (i = 0; i < array_n(bad_array); i++) {
              index = array_fetch(int, bad_array, i);
              fanout = array_fetch(node_t *, real_fanout_array, index);
              foreach_fanin(fanout, j, fanin) {
                  (void) st_insert(update_nodetable, (char *) fanin, (char *) fanin);
              }
          }
      }
  
      st_foreach_item(update_nodetable, stgen, (char **) &fanin, &value) {
          act_ite_partial_collapse_assign_score_node(fanin, score_table,  init_param);      
      }
      st_free_table(update_nodetable);
  }

  array_free(real_fanout_array);
  array_free(bad_array);
  array_free(good_array);
  *pgain = gain_good + (gain_bad * (gain_bad > 0));
  return (*pgain); /* not returning 1, *pgain > 0 iff some collapsing took place */
}

int
act_ite_partial_collapse_assign_score_node(node, score_table,  init_param)
  node_t *node;
  st_table *score_table;
  act_init_param_t *init_param;
{
  int score, num_fanin, cost, num_fanouts, i;
  node_t *fanout;
  char *name;

  if (node->type != INTERNAL) return (-1);
  name = node_long_name(node);
  num_fanin = node_num_fanin(node);
  if ((num_fanin > init_param->FANIN_COLLAPSE) || (is_anyfo_PO(node))) {
     /* give a score of 0 to this node */
     /*--------------------------------*/
     (void) st_insert(score_table, name, (char *) 0);
     return 0;
  }
  cost = ACT_ITE_SLOT(node)->cost;
  if (cost > init_param->COST_LIMIT) {
     (void) st_insert(score_table, name, (char *) 0);
     return 0;
  }
  if (cost == 0) cost = 1;
  num_fanouts = node_num_fanout(node);
  score = 0;
  for (i = 0; i < num_fanouts; i++) {
     fanout = node_get_fanout(node, i);
     score += ACT_ITE_SLOT(fanout)->cost;
  }
  score = (int) ceil((double) (score / cost));
  (void) st_insert(score_table, name, (char *) score);
  return score;
}

/*-----------------------------------------------------------
  Returns the node with the highest score in the score_table.
  The corresponding score is set in pscore. Assumes that all
  scores are nonnegative.
-------------------------------------------------------------*/
static
node_t *
act_ite_partial_collapse_find_max_score(network, score_table, pscore)
  network_t *network;
  st_table *score_table;
  int *pscore;
{
  lsGen gen;
  node_t *node, *node_max;
  int max_score, score;

  max_score = -1;
  node_max = NIL (node_t);

  foreach_node(network, gen, node) {
    if (node->type != INTERNAL) continue;
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
  Assigns a score to each internal node. If the cost of the node is > 
  init_param->COST_LIMIT, it is 0.
  Else it is based on sum of costs of the fanout nodes and the cost of the node.
  Stores all the costs in the score_table.
---------------------------------------------------------------------------------*/
int
act_ite_partial_collapse_assign_score_network(network, score_table, init_param)
  network_t *network;
  st_table *score_table;
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
     score = act_ite_partial_collapse_assign_score_node(node, score_table,  
                                                    init_param);
     if (ACT_ITE_DEBUG > 3 && (score != -1)) 
         (void) printf("---assigned score of %d to node %s\n", score, node_long_name(node));
  }
  array_free(nodevec);
}

act_ite_partial_collapse_update_ite_fields(network, dup_fanout, cost_struct)
  network_t *network;
  node_t *dup_fanout;
  ACT_ITE_COST_STRUCT *cost_struct;
{
  node_t *fanout;

  fanout = network_find_node(network, node_long_name(dup_fanout));
  cost_struct->node = fanout;
  if (cost_struct->match) return;
  if (cost_struct->ite) {
      cost_struct->ite->node = fanout;  
      act_ite_partial_collapse_put_node_names_in_ite(cost_struct->ite, network);
      assert(cost_struct->act == NIL (ACT_VERTEX));
  } else {
      cost_struct->act->node = fanout;  
      act_partial_collapse_put_node_names_in_act(cost_struct->act, network);
  }      
}

act_ite_partial_collapse_put_node_names_in_ite(ite, network)
  ite_vertex *ite;
  network_t *network;
{
  st_table *table;
  char *key;
  st_generator *stgen;
  ite_vertex *vertex;
  
  table = ite_my_traverse_ite(ite);
  st_foreach_item(table, stgen, &key, (char **) &vertex) {
      if (vertex->value != 2) continue;
      if (vertex->fanin !=  network_find_node(network, node_long_name(vertex->fanin))) {
          (void) printf("need partial collapse put node names\n");
      }
      assert(vertex->fanin = network_find_node(network, node_long_name(vertex->fanin)));
      vertex->name = node_long_name(vertex->fanin);
  }
  st_free_table(table);
}
