#include "sis.h"
#include "pld_int.h"
#include "ite_int.h"

static void ite_decomp_big_nodes();

/*
  Log of changes

  Jan 5, 93: decomp_big_nodes accepts a decomposition even if the cost remains the same.
  however, after experimenting, it turns out that this move is not good. Even gives worse 
  results. So was commented out on Jan. 6 '93.

  Jan 11 '93: In act_node_remap(), if DECOMP_FLAG is 1, number of iterations is made 1.
  Some other fields are changed as well. Also, the network after decomposition is a 
  "broken" one, ie, in terms of ACT1 blocks (for almost all practical cases).

  Jan 13 '93: Added DECOMP_METHOD field in act_init_param: USE_GOOD_DECOMP and USE_FACTOR
  are two methods now. In principle, they are the same; implementations differ. 
  USE_GOOD_DECOMP constructs a decomposed network and maps each node separately, whereas 
  USE_FACTOR contructs an ite for the factored form of the node.
*/

/*------------------------------------------------------------------------------------
 Improve the network using repeated partial collapse and decomp till NUM_ITER are 
 completed, or no further improvement is obtained.
 Then quick-phase is entered to determine which nodes should be realized in 
 negative phase.
--------------------------------------------------------------------------------------*/

act_ite_iterative_improvement(network, init_param)
  network_t *network;
  act_init_param_t *init_param;
{
  if (init_param->NUM_ITER > 0) ite_improve_network(network, init_param);
  /* if (init_param->QUICK_PHASE) act_quick_phase(network, init_param); */
  if (init_param->LAST_GASP) act_last_gasp(network, init_param);
  if (ACT_ITE_DEBUG >3) assert(network_check(network));
  if (ACT_ITE_STATISTICS) {
      (void) printf("block count = %d\n", ite_print_network(network, -1));
  }
}


ite_improve_network(network, init_param)
  network_t *network;
  act_init_param_t *init_param;

{
  int nogain;
  int iteration_num;
  int iteration_gain;
  int collapse_gain;
  int decomp_gain;

  nogain = FALSE;
  iteration_num = 0;
  while (nogain == FALSE && iteration_num < init_param->NUM_ITER) { 
      nogain = TRUE; 
      /* decomposing big nodes in the network if it is profitable*/
      /*---------------------------------------------------------*/
      ite_decomp_big_nodes(network, &decomp_gain, init_param);
      if (ACT_ITE_DEBUG > 3) {
          assert(network_check(network));
      }
      collapse_gain = act_ite_partial_collapse(network, init_param);
      iteration_gain = collapse_gain + decomp_gain;
      if (iteration_gain > 0) nogain = FALSE;
      iteration_num++;
      if (ACT_ITE_DEBUG && iteration_gain) {
          (void) printf("gain after an iteration = %d\n", iteration_gain);
          (void) ite_print_network(network, iteration_num);
      }
  }
}

/*--------------------------------------------------------------------------
  Decompose nodes of the network. If decomposition results in a 
  lesser cost node, accept the decomposition, and replace the node by
  the decomposition.
  think about how to decompose the nodes in the network. Should I use the 
  mux structure and do a resub. this is because it possibly means that the
  decomposition would be in terms of a mux structure => Boolean div not 
  implemented in misII.
-----------------------------------------------------------------------------*/

static void
ite_decomp_big_nodes(network, gain, init_param)
  network_t      *network;
  int     	 *gain;
  act_init_param_t *init_param;
{
  array_t 	*nodevec;
  int 		i;
  node_t 	*node;
  int		num_fanin, num_cube;
  node_function_t node_fun;
  int           FLAG_DECOMP;

  *gain = 0;
  FLAG_DECOMP = 1;
  nodevec = network_dfs(network);
  for (i = 0; i < array_n(nodevec); i++) {
      node = array_fetch(node_t *, nodevec, i);
      node_fun = node_function(node);
      if ((node_fun == NODE_PI) || (node_fun == NODE_PO)) continue;
      num_fanin = node_num_fanin(node);
      if (num_fanin < init_param->DECOMP_FANIN) continue;
      if (ACT_ITE_SLOT(node)->cost <= 2) continue;

      /* if constructed optimally, do not decompose - not implemented yet */
      /*------------------------------------------------------------------*/
      num_cube = node_num_cube(node);
      if ((num_cube == 1) || 
          (node_num_literal(node) == num_cube)
          ) continue;

      switch (init_param->DECOMP_METHOD) {
        case USE_GOOD_DECOMP:
          (*gain) += act_ite_node_remap(network, node, FLAG_DECOMP, init_param);
          break;
        case USE_FACTOR:
          (*gain) += act_ite_map_factored_form(node, init_param);
          break;
        default:
          (void) printf("unexpected DECOMP_METHOD\n");
          exit(1);
      }
  }
  if (ACT_ITE_DEBUG) (void) printf("---Gain in decomp_big_nodes = %d\n", *gain);
  array_free(nodevec);
}

/* this is not being used. Instead using act_last_gasp */

network_t *
act_ite_network_remap(network, init_param)
   network_t *network;
   act_init_param_t *init_param;
{
   array_t *nodevec;
   node_t *node;
   int i;
   int gain;
   int FLAG_DECOMP = 0;
   /* int HEURISTIC_NUM = 0; */ /* does not matter */

   gain = 0;
   nodevec = network_dfs(network);
   for (i = 0; i < array_n(nodevec); i++) {
       node = array_fetch(node_t *, nodevec, i);
       gain += act_ite_node_remap(network, node, FLAG_DECOMP, init_param);
   }
   if (ACT_ITE_STATISTICS) (void) printf("total gain after remapping = %d\n", gain);
   array_free(nodevec);
   return network;
}

/*-------------------------------------------------------------------------
  A last attempt at improving results. Forms a network out of the node, 
  decomposes it and then remaps it. 
--------------------------------------------------------------------------*/
act_last_gasp(network, init_param)
  network_t *network;
  act_init_param_t *init_param;
{
  array_t *nodevec;
  int i;
  node_t *node;
  int DECOMP_FLAG = 0;
  int gain;

  gain = 0;
  nodevec = network_dfs(network);
  for (i = 0; i < array_n(nodevec); i++) {
      node = array_fetch(node_t *, nodevec, i);
      gain += act_ite_node_remap(network, node, DECOMP_FLAG, init_param);
  }
  array_free(nodevec);

  if (ACT_ITE_STATISTICS) (void) printf("total gain in remapping = %d\n", gain);
}

/*----------------------------------------------------------------------------------
  Calls the same procedures for a node to obtain a good mapping of the node. 
  It accepts the solution if it is better than the earlier one. 
  Can be called either for the DECOMPOSITION or for simply remapping (LAST_GASP).
  FLAG_DECOMP  is 1 if the routine is called from decomp_big_nodes, is 0 if it is 
  called from act_last_gasp.
------------------------------------------------------------------------------------*/
int
act_ite_node_remap(network, node, FLAG_DECOMP, init_init_param)
   network_t *network;
   node_t *node;
   int FLAG_DECOMP;
   act_init_param_t *init_init_param;
{

  node_t  *dup_node, *node1, *node1_dup;
  network_t *network1;
  int  cost_node_original;
  int  cost_network1;
  st_table *table1;
  int   num_cubes, gain, alternate_gain, num_nodes2, i, improved, HEURISTIC_NUM_ORIG;
  act_init_param_t *init_param;
  node_function_t node_fun;
  array_t *nodevec1;

  HEURISTIC_NUM_ORIG = init_init_param->HEURISTIC_NUM;

  node_fun = node_function(node);
  if ((node_fun == NODE_PI) || (node_fun == NODE_PO)) return 0;

  cost_node_original = ACT_ITE_SLOT(node)->cost;
  if (ACT_ITE_DEBUG) {
      (void) printf("original cost of node %s = %d\n", node_long_name(node), 
                    cost_node_original);
  }
  if (cost_node_original <= 2) return 0;

  /* if the subject-graph for the node is optimal, do not remap */
  /*------------------------------------------------------------*/
  num_cubes = node_num_cube(node);
  if ((num_cubes == 1) ||
      (num_cubes == node_num_literal(node))
     ) return 0;

  /* if doing remapping, and constructing ITE only, then also construct 
     BDD (and vice versa), then check if cost improves.                */
  /*-------------------------------------------------------------------*/
  alternate_gain = 0; /* incurred from constructing alternate representation */
  if (FLAG_DECOMP == 0 && init_init_param->ALTERNATE_REP) {
      improved = act_ite_use_alternate_rep(node, init_init_param);
      if (improved) {
          alternate_gain = cost_node_original - ACT_ITE_SLOT(node)->cost;
          if (ACT_ITE_SLOT(node)->cost <= 2) return alternate_gain; /* can't improve any more */
          cost_node_original = ACT_ITE_SLOT(node)->cost;
      }
  }

  dup_node = node_dup(node);
  network1 = network_create_from_node(dup_node);
  decomp_good_network(network1);
  if ((FLAG_DECOMP == 1) && (network_num_internal(network1) == 1)) {
       network_free(network1);
       node_free(dup_node);
       return alternate_gain;
  }
  if (FLAG_DECOMP == 0) decomp_tech_network(network1, 1000, 1000); /* changed from 2, 2 */

  init_param = ALLOC(act_init_param_t, 1);
  if (FLAG_DECOMP == 0) {
       init_param->NUM_ITER = 1; /* changed from 2 */
       init_param->FANIN_COLLAPSE = 8; /* changed from 3 */
       init_param->DECOMP_FANIN   = 4;
       init_param->QUICK_PHASE    = 1;
       init_param->HEURISTIC_NUM = 3;
  }
  else {
       init_param->NUM_ITER = 1; /* 0 earlier - Jan 11 93*/
       init_param->FANIN_COLLAPSE = 8; /* 0 earlier */
       init_param->DECOMP_FANIN   = 4; /* 0 earlier */
       init_param->QUICK_PHASE    = 0;
       init_param->HEURISTIC_NUM = HEURISTIC_NUM_ORIG;
  }
  init_param->DISJOINT_DECOMP = 0;
  init_param->GAIN_FACTOR = 0.001;
  init_param->map_alg = init_init_param->map_alg;
  init_param->mode = 0.0;
  init_param->COLLAPSE_FANINS_OF_FANOUT = 15;
  init_param->ITE_FANIN_LIMIT_FOR_BDD = 40;
  init_param->COST_LIMIT = 3;
  init_param->LAST_GASP = 0;
  init_param->BREAK = 1; /* 0 earlier */
  init_param->ALTERNATE_REP = 0;
  init_param->COLLAPSE_UPDATE = INEXPENSIVE;
  init_param->MAP_METHOD = init_init_param->MAP_METHOD;
  init_param->VAR_SELECTION_LIT = init_init_param->VAR_SELECTION_LIT;
  init_param->COLLAPSE_METHOD = init_init_param->COLLAPSE_METHOD;
  init_param->DECOMP_METHOD = init_init_param->DECOMP_METHOD;

  act_ite_map_network_with_iter(network1, init_param);
  act_free_ite_network(network1); /* frees the ite, act and match */

  /* this makes sure that the nodes of network1 do not lose their ite, match or network */
  /*------------------------------------------------------------------------------------*/
  init_param->BREAK = 0;
  act_ite_map_network_with_iter(network1, init_param);

  FREE(init_param);

  cost_network1 = ite_print_network(network1, -1);
  assert(cost_network1 == network_num_nonzero_cost_nodes(network1));

  if (ACT_ITE_DEBUG) 
    (void) printf (" the original cost of node = %d, cost after remapping = %d\n",
		    cost_node_original, cost_network1);
  gain = cost_node_original - cost_network1;
  if (gain <= 0) {
      act_free_ite_network(network1);
      network_free(network1);
      node_free(dup_node);
      return alternate_gain;
  }
      
  /* replace the node in the original network by the network1 
     better not to call pld_replace_node_by_network, since we
     need to replace the cost_struct also.                   */
  /*---------------------------------------------------------*/
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
      node1_dup->name = NIL (char);  /* possibility of leak -> no */
      node1_dup->short_name = NIL (char);
      network_add_node(network, node1_dup);
      if (ACT_ITE_DEBUG) (void) node_print(sisout, node1_dup);
      if (init_init_param->MAP_METHOD == MAP_WITH_ITER || init_init_param->MAP_METHOD == MAP_WITH_JUST_DECOMP) {
          act_ite_network_update_pi(node1_dup, node1, table1, network1);
      } else {
          /* get the cost_struct for node1 and put it in the node1_dup */
          /*-----------------------------------------------------------*/
          act_ite_cost_struct_replace(node1_dup, node1, init_init_param->map_alg);
          act_ite_remap_update_ite_fields(table1, network1, node1_dup);
      }
      assert(!st_insert(table1, (char *) node1, (char *) node1_dup));
  }
  /* the last node in the array in primary output - do not want that */
  /*-----------------------------------------------------------------*/
  node1 = array_fetch(node_t *, nodevec1, num_nodes2);
  assert(node1->type == INTERNAL);
  node1_dup = pld_remap_get_node(node1, table1);
  node_replace(node, node1_dup);
  if (init_init_param->MAP_METHOD == MAP_WITH_ITER || init_init_param->MAP_METHOD == MAP_WITH_JUST_DECOMP) {
      act_ite_network_update_pi(node, node1, table1, network1);
  } else {
      act_ite_cost_struct_replace(node, node1, init_init_param->map_alg);
      act_ite_remap_update_ite_fields(table1, network1, node);
  }
  assert(!st_insert(table1, (char *) node1, (char *) node));

  st_free_table(table1);	
  array_free(nodevec1);
  network_free(network1);
  node_free(dup_node);
  return (gain + alternate_gain);
}

/*------------------------------------------------------------------------------
  Given node1 of the network1 and node1_dup which is a replica of node1, and
  an implementation of node1 in terms of network1, change names of the primary
  inputs of network1 so that it can be attached to node1_dup.
-------------------------------------------------------------------------------*/
  

act_ite_network_update_pi(node1_dup, node1, table1, network1)
  node_t *node1_dup, *node1;
  st_table *table1;
  network_t *network1;
{
  network_t *network_node1;
  array_t *pivec;
  lsGen gen;
  int i;
  node_t *pi, *pi1, *pi1_dup;

  network_node1 = ACT_ITE_SLOT(node1)->network;
  pivec = array_alloc(node_t *, 0);
  foreach_primary_input(network_node1, gen, pi) {
      array_insert_last(node_t *, pivec, pi);
  }
  for (i = 0; i < array_n(pivec); i++) {
      pi = array_fetch(node_t *, pivec, i);
      pi1 = network_find_node(network1, node_long_name(pi));
      assert(pi1);
      if (pi1->type == PRIMARY_INPUT) {
          assert(st_lookup(table1, (char *) pi1, (char **) &pi1_dup));
          assert(strcmp(node_long_name(pi), node_long_name(pi1_dup)) == 0);
          continue;
      }
      assert(pi1->type == INTERNAL);
      assert(st_lookup(table1, (char *) pi1, (char **) &pi1_dup));
      network_change_node_name(network_node1, pi, util_strsav(node_long_name(pi1_dup)));
  }
  network_free(ACT_ITE_SLOT(node1_dup)->network);
  ACT_ITE_SLOT(node1_dup)->network = network_node1;
  ACT_ITE_SLOT(node1_dup)->cost = network_num_nonzero_cost_nodes(network_node1);
  ACT_ITE_SLOT(node1)->network = NIL(network_t);
  array_free(pivec);
}

int ite_print_network(network, iteration_num)
  network_t *network;
  int iteration_num;
{
  lsGen gen;
  node_t *node;
  int TOTAL_COST;
  node_function_t node_fun;

  TOTAL_COST = 0;
  if (ACT_ITE_DEBUG) (void) printf("printing network %s\n", network_name(network));
  foreach_node(network, gen, node) {
      node_fun = node_function(node);
      if ((node_fun == NODE_PI) || (node_fun == NODE_PO)) continue;
      if (ACT_ITE_DEBUG) {
          node_print(stdout, node); 
          (void) printf("cost = %d\n\n", ACT_ITE_SLOT(node)->cost); 
      }
      TOTAL_COST += ACT_ITE_SLOT(node)->cost;
  }
  if (ACT_ITE_DEBUG) {
      if (iteration_num < 0)
          (void) printf("**** total cost of network after all iterations is %d\n***", 
                        TOTAL_COST);
      else 
          (void) printf("**** total cost of network after iteration %d is %d\n***", 
                        iteration_num, TOTAL_COST);
  }
  return TOTAL_COST;
}

/*-----------------------------------------------------------------------------------
  If using bdd now, use ite and vice versa. If cost improves, accept the alternate
  representation.
------------------------------------------------------------------------------------*/
int
act_ite_use_alternate_rep(node, init_param)
  node_t *node;
  act_init_param_t *init_param;
{

  ACT_ITE_COST_STRUCT *cost_node;
  ACT_VERTEX_PTR act_of_node;
  int cost_bdd, cost_ite;

  if ((node->type == PRIMARY_INPUT) || (node->type == PRIMARY_OUTPUT)) return 0;
  if (init_param->HEURISTIC_NUM == 3) return 0;
  cost_node = ACT_ITE_SLOT(node);
  if (cost_node->cost <= 2) return 0;
  if (init_param->HEURISTIC_NUM <= 1) {
      act_of_node = my_create_act(node, init_param->mode, NIL (st_table), node->network, 
                                  NIL (array_t));  /* this sets the pointers in node->pld */
      cost_bdd = act_bdd_make_tree_and_map(node, act_of_node);
      if (cost_bdd <= cost_node->cost) {
          ite_my_free(cost_node->ite);
          cost_node->ite = NIL (ite_vertex);
          cost_node->cost = cost_bdd;
          cost_node->arrival_time = act_of_node->arrival_time;
          cost_node->act  = act_of_node; 
          return 1;
      }
      my_free_act(act_of_node);
      cost_node->act = NIL (ACT_VERTEX);
      return 0;
  } 
  assert(init_param->HEURISTIC_NUM == 2);
  make_ite(node);
  cost_ite = act_ite_make_tree_and_map(node);
  if (cost_ite < cost_node->cost) {
      my_free_act(cost_node->act);
      cost_node->act = NIL (ACT_VERTEX);
      cost_node->arrival_time = ACT_ITE_ite(node)->arrival_time;
      cost_node->cost = cost_ite;
      return 1;
  }
  ite_my_free(cost_node->ite);
  cost_node->ite = NIL (ite_vertex);
  return 0;
}
