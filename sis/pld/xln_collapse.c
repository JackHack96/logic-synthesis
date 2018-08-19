/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/pld/xln_collapse.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:56 $
 *
 */
#include "sis.h"
#include "pld_int.h"


/*--------------------------------------------------------------------------
  This file includes a routine similar to the one used in Actel - cost for
  each node is known. (The node is not decomposed into feasible nodes, but
  remains as such). Then a score is given to each node which reflects the
  possible gain that might be achieved if this node is collapsed into all 
  the fanouts. Node with maximum score is picked and tried for collapsing.
  If it helps, it is accepted and the score of each fanout is updated. 
  Repeat the process and in the end, replace each node by the network 
  associated with it.
-----------------------------------------------------------------------------*/

xln_partial_collapse(network, init_param)
  network_t *network;
  xln_init_param_t *init_param;
{
  array_t *nodevec, *fanoutvec;
  int i, j, k, num;
  node_t *node, *fanin, *fanout;
  st_table *cost_table; /* storing a feasible network for each internal node */
  network_t *network_node; /* feasible network for a node */
  int gain, total_gain, collapsed;
  int cost_node;
  int num_collapsed, trivial_collapsed;
  int total_cost; /* of the network in terms of tlu's*/
  /* int changed; */
  st_table *table_affected_nodes;
  st_generator *stgen;
  lsGen genfo;
  char *dummy, *key, *name;

  /* do  a trivial collapse first */
  /*------------------------------*/
  num_collapsed = 0;
  trivial_collapsed = 1;
  while (trivial_collapsed) {
      trivial_collapsed = 
          xln_do_trivial_collapse_network(network, init_param->support, 
                                          init_param->xln_move_struct.MOVE_FANINS,
                                          init_param->xln_move_struct.MAX_FANINS); 
                                            /* added March 15, 1991 */
      num_collapsed += trivial_collapsed;
  }

  if (XLN_DEBUG) {
      (void) printf("---collapsed %d nodes in trivial collapse\n", num_collapsed);
  }

  nodevec = network_dfs(network);
  num = array_n(nodevec);

  /* compute the cost of each node and store the resulting feasible network */
  /*------------------------------------------------------------------------*/
  cost_table = st_init_table(strcmp, st_strhash); /* stores the network here */
  total_cost = 0;
  for (i = 0; i < num; i++) {
      node = array_fetch(node_t *, nodevec, i);
      if (node->type != INTERNAL) continue;

      /* Added Aug 26 93 to avoid the problem encountered when the node
         is not expressed in minimum support -          see xln_imp.c */
      /*--------------------------------------------------------------*/
      simplify_node(node, SIM_METHOD_SNOCOMP, SIM_DCTYPE_NONE, SIM_FILTER_NONE, 
                    SIM_ACCEPT_SOP_LITS);
      network_node = xln_best_script(node, init_param);
      total_cost += network_num_internal(network_node);
      assert(!st_insert(cost_table, node_long_name(node), (char *) network_node));
  }
  array_free(nodevec);
  if (XLN_DEBUG) 
      (void) printf("****total cost after initial mapping is %d\n", total_cost);

  /* alternate collapsing - hopefully faster */
  /*-----------------------------------------*/
  total_gain = 0;
  nodevec = network_dfs(network);
  num = array_n(nodevec);
  while (num) {
      /* for next iteration, store affected nodes in this table */
      /*--------------------------------------------------------*/
      table_affected_nodes = st_init_table(strcmp, st_strhash); 
      for (i = 0; i < num; i++) {
          node = array_fetch(node_t *, nodevec, i);
          
          /* if the node was to be considered in next iteration, delete it 
             from table, as it is being considered now                    */
          /*--------------------------------------------------------------*/
          name = node_long_name(node);
          (void) st_delete(table_affected_nodes, &name, &dummy);
          if (node->type != INTERNAL) continue;
          if (is_anyfo_PO(node)) continue;
          cost_node = xln_cost_of_node(node, cost_table);
          if (cost_node > init_param->COST_LIMIT) continue;

          /* save fanouts, as we may lose them in collapse routine */
          /*-------------------------------------------------------*/
          fanoutvec = array_alloc(node_t *, 0);
          foreach_fanout(node, genfo, fanout) {
              array_insert_last(node_t *, fanoutvec, fanout);
          }
          /* see if collapse is good */
          /*-------------------------*/
          collapsed = xln_partial_collapse_node(network, node, cost_table, 
                                                init_param, &gain);
          if (collapsed) {

              /* if node is collapsed, all the fanouts (& their fanins) 
                 and fanins should be considered in the next iteration.   */
              /*----------------------------------------------------------*/
              for (j = 0; j < array_n(fanoutvec); j++) {
                  fanout = array_fetch(node_t *, fanoutvec, j);
                  (void) st_insert(table_affected_nodes, node_long_name(fanout), 
                                   (char *) fanout);
                  foreach_fanin(fanout, k, fanin) {
                      if (fanin == node) continue;
                      (void) st_insert(table_affected_nodes, node_long_name(fanin), 
                                       (char *) fanin);
                  }
                      
              }
              foreach_fanin(node, j, fanin) {
                  (void) st_insert(table_affected_nodes, node_long_name(fanin), 
                                   (char *) fanin);
              }
              network_delete_node(network, node); 
              total_gain += gain;
          }
          array_free(fanoutvec);
      }
      array_free(nodevec);
      
      /* form an array of nodes to be tried for collapsing in the next iteration */
      /*-------------------------------------------------------------------------*/
      nodevec = array_alloc(node_t *, 0);
      st_foreach_item(table_affected_nodes, stgen, &key, (char **) &node) {
          if (node->type != INTERNAL) continue;
          array_insert_last(node_t *, nodevec, node);
      }
      st_free_table(table_affected_nodes);
      num = array_n(nodevec);
  } /* while num */
  array_free(nodevec);

  if (XLN_DEBUG) 
      (void) printf("****gain in partial collapse is %d\n", total_gain);

  /* finally replace each node by its network */
  /*------------------------------------------*/
  assert(network_check(network));
  nodevec = network_dfs(network);
  num = array_n(nodevec);
  for (i = 0; i < num; i++) {
      node = array_fetch(node_t *,  nodevec, i);
      if (node->type != INTERNAL) continue;
      assert(st_lookup(cost_table, node_long_name(node), (char **) &network_node));
      assert(network_check(network_node));
      if (node_num_fanin(node) <= init_param->support) {
          network_free(network_node);
          continue;
      }
      /* trying to improve results by trying other decomposition options*/
      /*----------------------------------------------------------------*/
      xln_collapse_remap(node, &network_node, init_param);
      pld_replace_node_by_network(node, network_node);
      network_free(network_node);
  }
  /* free */
  /*------*/
  st_free_table(cost_table);
  array_free(nodevec);
}


int
xln_cost_of_node(node, cost_table)
  node_t *node;
  st_table *cost_table;
{
  network_t *network_node;
  
  assert(st_lookup(cost_table, node_long_name(node), (char **) &network_node));
  if (network_node == NIL(network_t)) return 0;
  return network_num_internal(network_node);
}

/*-----------------------------------------------------------------------------------------
  Checks if it is beneficial to collapse node into each of the fanouts. If so, does that 
  and returns 1 and the gain in pgain. Changes the cost_table entries of these fanouts 
  and deletes the entry of node. Else returns 0 and pgain is not assigned any values.
--------------------------------------------------------------------------------------------*/
int 
xln_partial_collapse_node(network, node, cost_table, init_param, pgain)
  network_t *network;
  node_t *node;
  st_table *cost_table;
  xln_init_param_t *init_param;
  int *pgain;
{
  int gain, old_cost;
  char *name, *name1, *dummy, *key;
  network_t *network_fanout;
  node_t *fanout, *fanout_simpl, *dup_fanout;
  lsGen genfo;
  st_generator *stgen;
  st_table *fo_table;
  array_t *fanout_array, *real_fanout_array;
  int i, num_fanout, cost_node;

  fo_table = st_init_table(strcmp, st_strhash);
  name = node_long_name(node);
  assert(st_lookup(cost_table, name, &dummy));
  cost_node =  network_num_internal((network_t *)dummy);
  gain = cost_node;

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
     fanout_simpl = node_simplify(dup_fanout, NIL(node_t), NODE_SIM_ESPRESSO);
     /* 
     node_free(dup_fanout);
     dup_fanout = fanout_simpl;
     */
     node_replace(dup_fanout, fanout_simpl);
     array_insert_last(node_t *, fanout_array, dup_fanout);
     network_fanout = xln_best_script(dup_fanout, init_param);
     if (XLN_DEBUG > 3) {
         (void) printf("number of nodes of network for node %s = %d\n", node_long_name(fanout),
                       network_num_internal(network_fanout));
     }
     name = node_long_name(dup_fanout);
     assert(!st_insert(fo_table, name, (char *) network_fanout));
     assert(st_lookup(cost_table, name, &dummy));
     old_cost = network_num_internal((network_t *)dummy);
     gain = gain + old_cost - network_num_internal(network_fanout);
  }
  if (gain <= 0) {
      if (XLN_DEBUG > 3) {
          (void) printf("could not collapse node, gain = %d", gain);
          node_print(stdout, node);
          (void) printf("num_fanin = %d, num_fanout = %d, cost = %d\n", 
                        node_num_fanin(node), node_num_fanout(node), cost_node);
      }
      name = node_long_name(node);
      xln_free_fo_table_without_freeing_key(fo_table);
      for (i = 0; i < num_fanout; i++) {
          dup_fanout = array_fetch(node_t *, fanout_array, i);
          node_free(dup_fanout);
      }
      array_free(fanout_array);
      array_free(real_fanout_array);
      return 0;
  }
  if (XLN_DEBUG) {
      (void) printf("------collapsed node, gain = %d ", gain);
      node_print(stdout, node);
      (void) printf("num_fanin = %d, num_fanout = %d, cost = %d\n", 
                    node_num_fanin(node), node_num_fanout(node), cost_node);
      st_foreach_item(fo_table, stgen, &key, (char **) &network_fanout) {
          (void) printf("\t cost of fanout %s is %d\n", key, 
                        network_num_internal(network_fanout));
      }
  }

  for (i = 0; i < num_fanout; i++) {
     dup_fanout = array_fetch(node_t *, fanout_array, i);
     fanout = array_fetch(node_t *, real_fanout_array, i);
     /* collapse node into fanout, update the cost_table entry */
     /*--------------------------------------------------------*/
     name1 = node_long_name(dup_fanout);
     assert(st_delete(cost_table, &name1, &dummy));
     network_free((network_t *) dummy);
     assert(st_lookup(fo_table, name1, (char **)&network_fanout));
     assert(!st_insert(cost_table, name1, (char *) network_fanout));
     (void) node_replace(fanout, dup_fanout);
     if (XLN_DEBUG > 3) {
         (void) printf("number of nodes of network for node %s = %d\n", name1, 
                       network_num_internal(network_fanout));
     }
  } /* foreach_fanout */
  array_free(fanout_array);
  array_free(real_fanout_array);
  st_free_table(fo_table);
  /* delete the collapsed node from cost_table, network.*/  
  /*----------------------------------------------------*/
  name1 = node_long_name(node);
  assert(st_delete(cost_table, &name1,  &dummy)); 
  network_free((network_t *) dummy);
  *pgain = gain;
  return 1;
}

xln_free_fo_table_without_freeing_key(fo_table)
  st_table *fo_table;
{
  st_generator *gen;
  char *key;
  network_t *network_node;

  st_foreach_item(fo_table, gen, &key, (char **) &network_node) {
      network_free(network_node);
  }
  st_free_table(fo_table);
}

  
xln_collapse_remap(node, pnetwork, init_param)
  node_t *node;
  network_t **pnetwork;
  xln_init_param_t *init_param;
{
  network_t *network2;
  int flag_decomp_save;

  /* if this flag is GOOD, then all the mapping options have been tried */
  /*--------------------------------------------------------------------*/
  if (init_param->good_or_fast == GOOD) return;

  /* if the decomp flag was 2 (used both with and without good_decomp),
     call xln_try_other_options.                                       */
  /*-------------------------------------------------------------------*/
  if (init_param->flag_decomp_good == 2) {
      xln_try_other_mapping_options(node, pnetwork, init_param);
      return;
  }

  flag_decomp_save = init_param->flag_decomp_good;
  if (flag_decomp_save) {
      init_param->flag_decomp_good = 0;
  } else {
      init_param->flag_decomp_good = 1;
  }
  network2 = xln_best_script(node, init_param);
  if (network_num_internal(network2) < network_num_internal(*pnetwork)) {
      network_free(*pnetwork);
      *pnetwork = network2;
  } else {
      network_free(network2);
  }
  init_param->flag_decomp_good = flag_decomp_save;
}

/*------------------------------------------------------------------
  Assumes that the pnetwork is feasible to begin with.
--------------------------------------------------------------------*/
xln_collapse_check_area(pnetwork, init_param, roth_karp_flag)
  network_t **pnetwork;
  xln_init_param_t *init_param;
  int roth_karp_flag;
{
  network_t *network1;

  network1 = xln_check_network_for_collapsing_area(*pnetwork, init_param, roth_karp_flag);
  if (network1 == NIL(network_t)) return;
  if (network_num_internal(*pnetwork) > network_num_internal(network1)) {
      network_free(*pnetwork);
      *pnetwork = network1;
  } else {
      network_free(network1);
  }
}


/*---------------------------------------------------------------------------------
  If the network has small number of inputs, collapses the network. Then applies
  Roth-Karp decomposition and cofactoring techniques on the network and evaluates
  them. Returns the network with fewer nodes. Returns NIL if collapsing
  could not be done. If roth_karp flag is 0, roth_karp decomposition is not done.
-----------------------------------------------------------------------------------*/
network_t *
xln_check_network_for_collapsing_area(network, init_param, roth_karp_flag)
  network_t *network;
  xln_init_param_t *init_param;
  int roth_karp_flag;
{
  network_t *network1, *network2;

  /* if support = 2, do not do anything */
  /*------------------------------------*/
  if (init_param->support == 2) return NIL (network_t);

  if (network_num_pi(network) > init_param->collapse_input_limit) return NIL (network_t);

  network1 = network_dup(network);
  (void) network_collapse(network1);
  if (roth_karp_flag) network2 = network_dup(network1);

  pld_simplify_network_without_dc(network1);
  xln_cofactor_decomp_network(network1, init_param->support, AREA); 

  /* karp_decomp_network simplifies each node */
  /*------------------------------------------*/
  if (roth_karp_flag) {
      karp_decomp_network(network2, init_param->support, 0, NIL (node_t));
      assert(xln_is_network_feasible(network2, init_param->support));

      if (network_num_internal(network1) < network_num_internal(network2)) {
          network_free(network2);
          return network1;
      }
      network_free(network1);
      return network2;
  }
  return network1;
}

