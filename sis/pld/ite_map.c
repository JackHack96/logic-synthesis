#include "sis.h"
#include "pld_int.h"
#include "ite_int.h"

static void act_initialize_ite_area();

ite_vertex_ptr PRESENT_ITE;
extern int WHICH_ACT;

act_ite_map_network_with_iter(network, init_param)
  network_t *network;
  act_init_param_t *init_param;
{
  /* act_ite_preprocess(network, init_param); */ /* done with a separate command */
  act_ite_map_network(network, init_param);
  act_ite_iterative_improvement(network, init_param);
  if (init_param->BREAK) act_ite_break_network(network, init_param);
}

act_ite_preprocess(network, init_param)
  network_t *network;
  act_init_param_t *init_param;
{
  array_t *nodevec;
  int i;
  node_t *node;

  nodevec = network_dfs(network);
  for (i = 0; i < array_n(nodevec); i++) {
      node = array_fetch(node_t *, nodevec, i);
      if (node->type == PRIMARY_INPUT || node->type == PRIMARY_OUTPUT) continue;
      if (node_num_literal(node) > init_param->lit_bound) decomp_quick_node(network, node);
  }
  array_free(nodevec);
}

/*----------------------------------------------------------------
  For each intermediate node of the network, builds up the ite
  and then maps it to the basic blocks of the ACTEL architecture.
  NOTE: it does not free the ite at each node.
------------------------------------------------------------------*/
act_ite_map_network(network, init_param)
  network_t *network;
  act_init_param_t *init_param;
{
  array_t *nodevec;
  int i;
  node_t *node;
  int total_cost = 0;

  nodevec = network_dfs(network);

  switch(init_param->MAP_METHOD) {
    case MAP_WITH_ITER:;
    case MAP_WITH_JUST_DECOMP:
      for (i = 0; i < array_n(nodevec); i++) {
          node = array_fetch(node_t *, nodevec, i);
          total_cost += act_ite_map_node_with_iter_imp(node, init_param);
      }
      break;
    case NEW:
      for (i = 0; i < array_n(nodevec); i++) {
          node = array_fetch(node_t *, nodevec, i);
          total_cost += act_ite_new_map_node(node, init_param);
      }
      break;
    case OLD: 
      for (i = 0; i < array_n(nodevec); i++) {
          node = array_fetch(node_t *, nodevec, i);
          total_cost += act_ite_map_node(node, init_param);
      }
      break;
    default:
      (void) printf("mapping method %d not known\n", init_param->MAP_METHOD);
      exit(1);
  }
  if (ACT_ITE_DEBUG || ACT_ITE_STATISTICS) {
      (void) printf("number of basic blocks = %d\n", total_cost);
  }
  array_free(nodevec);
}

int 
act_ite_map_node(node, init_param)
  node_t *node;
  act_init_param_t *init_param;
{
  node_t **order_list, **act_ite_order_fanins();
  ACT_MATCH *match;
  ACT_ITE_COST_STRUCT *cost_node;
  ACT_VERTEX_PTR act_of_node;
  int cost_bdd;
  node_function_t node_fun;

  if ((node->type == PRIMARY_INPUT) || (node->type == PRIMARY_OUTPUT)) return 0;
  cost_node = ACT_ITE_SLOT(node);
  /* note: not handling constants separately, because either remove them by sweep 
     before, or if needed, act_is_act_function will detect and use a block */
  /* check if the node is realizable by one block */
  /*----------------------------------------------*/
  match = act_is_act_function(node, init_param->map_alg, act_is_or_used);
  if (match) {
      cost_node->match = match;
      node_fun = node_function(node);
      if (node_fun == NODE_0 || node_fun == NODE_1 || node_fun == NODE_BUF) {
          cost_node->cost = 0;  /* added Nov. 18, 92 - earlier wrong cost assignment */
      } else {
          cost_node->cost = 1;
      }
      return cost_node->cost;
  }

  switch (init_param->HEURISTIC_NUM) {
    case 0:
      /* make heuristic ite */
      /*--------------------*/
      make_ite(node);
      cost_node->cost = act_ite_make_tree_and_map(node);
      cost_node->arrival_time = ACT_ITE_ite(node)->arrival_time;
      cost_node->node = node;
      /* cost_node->ite  = ACT_ITE_ite(node); */ /* make_ite sets this */
      break;
    case 1: 
      /* construct canonical ite */
      /*-------------------------*/
      order_list = act_ite_order_fanins(node);
      /* ite_pld(node, order_list); */ /* commented Jul 21, 1992 */
      /* just for the time being */
      ACT_ITE_ite(node) = ite_get(node);
      FREE(order_list);
      cost_node->cost = act_ite_make_tree_and_map(node);
      cost_node->arrival_time = ACT_ITE_ite(node)->arrival_time;
      cost_node->node = node;
      /* cost_node->ite  = ACT_ITE_ite(node); */
      break;
    case 2: 
      /* make ROBDD */
      /*------------*/
      act_of_node = my_create_act(node, init_param->mode, NIL (st_table), node->network, 
                                  NIL (array_t));  /* this sets the pointers in node->pld */
      cost_node->cost = act_bdd_make_tree_and_map(node, act_of_node);
      cost_node->arrival_time = act_of_node->arrival_time;
      cost_node->node = node;
      cost_node->act  = act_of_node; 
      break;
    case 3: 
      /* make both heuristic ite and ROBDD - select the one with lower cost */
      /*--------------------------------------------------------------------*/
      make_ite(node);
      cost_node->cost = act_ite_make_tree_and_map(node);
      cost_node->arrival_time = ACT_ITE_ite(node)->arrival_time;
      cost_node->node = node;
      if (cost_node->cost <= 2) break;
      if (node_num_fanin(node) <= init_param->ITE_FANIN_LIMIT_FOR_BDD) {
          act_of_node = my_create_act(node, init_param->mode, NIL (st_table), node->network, 
                                      NIL (array_t));
          cost_bdd = act_bdd_make_tree_and_map(node, act_of_node);
	  /* made it <= rather than <, since ite_map -n 1 later may be
	     better for a BDD rather than an ITE */
          if (cost_bdd <= cost_node->cost) {
              ite_my_free(cost_node->ite);
              cost_node->ite = NIL (ite_vertex);
              cost_node->cost = cost_bdd;
              cost_node->arrival_time = act_of_node->arrival_time;
              cost_node->act  = act_of_node; 
          } else {
              my_free_act(act_of_node);
              cost_node->act = NIL (ACT_VERTEX);
          }
      }
      break;
    default:
      fail("heuristic number out of range");
      break;
  }
  return cost_node->cost;
}


/*-------------------------------------------------------------------
  Assumes that ite of the node has been created. Breaks the ite into
  trees and maps each tree using the pattern graphs. 
--------------------------------------------------------------------*/
int
act_ite_make_tree_and_map(node)
  node_t *node;

{
  ite_vertex_ptr ite_of_node;
  int           num_ite;
  int     	i;
  int 		num_mux_struct;
  int 		TOTAL_MUX_STRUCT; /* cost of the node */
  ite_vertex_ptr ite;
  array_t *multiple_fo_array;
  
  ite_of_node = ACT_ITE_ite(node); /* check this */
  TOTAL_MUX_STRUCT = 0;

  /* initialize the required structures */
  /*------------------------------------*/
  multiple_fo_array = array_alloc(ite_vertex_ptr, 0);
  array_insert_last(ite_vertex_ptr, multiple_fo_array, ite_of_node);
  act_initialize_ite_area(ite_of_node, multiple_fo_array); 


  if (ACT_ITE_DEBUG > 3) {
     ite_print_dag(ite_of_node);
     /* print_multiple_fo_array(multiple_fo_array); */
  }

  /* traverse the ite of node bottom up and map each tree ite */
  /*----------------------------------------------------------*/
  num_ite = array_n(multiple_fo_array);
  for (i = num_ite - 1; i >= 0; i--) {
      ite = array_fetch(ite_vertex_ptr, multiple_fo_array, i);
      PRESENT_ITE = ite;
      num_mux_struct = act_map_ite(ite);
      TOTAL_MUX_STRUCT += num_mux_struct;
  }
  if (ACT_ITE_DEBUG) 
      (void) printf("number of total mux_structures used for the node %s = %d, arrival_time = %f\n",
                    node_long_name(node), TOTAL_MUX_STRUCT, ite_of_node->arrival_time);
  array_free(multiple_fo_array);
  ite_clear_dag(ite_of_node); /* to make all mark fields 0 */
  if (ACT_ITE_DEBUG > 1) {
      node_print(stdout, node);
      ite_print_dag(ite_of_node);
  }
  return TOTAL_MUX_STRUCT;
}
  

/*-----------------------------------------------------------------------------
  Maps a given vertex of ite-dag into pattern graphs for ACTEL architecture. 
  If the vertex has multiple fanouts, does not map until PRESENT_ITE = vertex.
  The detection of OR-gate is easy in this case. 
  Note: need to change the OR-gate detection: there are additional cases that 
        could be detected: 
             IF X THEN Z1 ELSE (IF Y THEN Z1 ELSE Z2) 
        <=>  IF (X + Y) THEN Z1 ELSE Z2.
        Here X, Y, Z1, Z2 are any functions.
        This can, however, never occur in canonical ite.
------------------------------------------------------------------------------*/

int
act_map_ite(vertex)
 ite_vertex_ptr vertex;
{
  
   int TOTAL_PATTERNS = 10;
   int pat_num, best_pat_num;
   int cost[10]; /* for each of the 10 patterns derived from the mux struct */
   int vif_cost, vthen_cost, velse_cost;
   int vifif_cost, vifelse_cost;
   int vthenif_cost, vthenthen_cost, vthenelse_cost;
   int velseif_cost, velsethen_cost, velseelse_cost;
   int velseelseif_cost, velseelsethen_cost, velseelseelse_cost;
   int temp_if_cost, temp_then_cost, temp_else_cost, temp_elseelse_cost;
   int OR_pattern1, OR_pattern2;
   int compl_then, compl_else, compl_elseelse;
   int cond_then, cond_else, cond_elseelse;

  /* if vertex has multiple fanouts (but is not the root of the present 
     multiple_fo vertex) return 0, as this vertex would be/ or was
     visited earlier as a root. Basically zero is not its true cost. 
     So do not set vertex->mapped = 1 also.
     NOTE: order of this statement and the next statement (vertex->mapped
           == 1) is opposite in mapping using bdd's. This order makes 
           code of v_thenif, etc. very easy.                          */
  /*------------------------------------------------------------------*/

  if ((vertex->multiple_fo) && (vertex != PRESENT_ITE)) {
     return 0;
  }

  /* if cost already computed, don't compute again */
  /*-----------------------------------------------*/
  if (vertex->mapped) return vertex->cost;

  vertex->mapped = 1; 

  if ((vertex->value == 0) || (vertex->value == 1)) {
     vertex->cost = 0;
     return 0;
  }  

  if (vertex->value == 2) {
      assert(vertex->phase == 0 || vertex->phase == 1);
      if (vertex->phase == 1) {
          vertex->cost = 0;
          return 0;
      }
      vertex->cost = 1;
      vertex->pattern_num = 0;
      return vertex->cost;
  }
  /* vertex->value = 3 */
  /*-------------------*/
   
  /* if simply an input variable, return 0 */
  /*---------------------------------------*/
  if ((vertex->IF->value == 2) && (vertex->IF->phase == 1) 
      && (vertex->THEN->value == 1) && (vertex->ELSE->value == 0)) {
      vertex->cost = 0;
      return 0;
   }
  /* if or gate not used, just used first 4 patterns */
  /*-------------------------------------------------*/
  if (!act_is_or_used) TOTAL_PATTERNS = 4;

  /* initialize cost vector to a very high value for all the patterns*/
  /*-----------------------------------------------------------------*/
  for (pat_num = 0; pat_num < TOTAL_PATTERNS; pat_num++) {
     cost[pat_num] = MAXINT;
  }

  /* recursively calculate relevant costs */
  /*--------------------------------------*/
  
  vif_cost = act_map_ite(vertex->IF);
  vthen_cost = act_map_ite(vertex->THEN);
  velse_cost = act_map_ite(vertex->ELSE);
    
  OR_pattern1 = 0;
  if ((vertex->IF->value == 3) && (vertex->IF->THEN->value == 1)) {
      OR_pattern1 = 1;
      if (vertex->IF->multiple_fo == 0) {
          vifif_cost = act_map_ite(vertex->IF->IF);
          vifelse_cost = act_map_ite(vertex->IF->ELSE);
          temp_if_cost = vifif_cost + vifelse_cost;
      }
  }

  /* changed Dec. 10, 1992 - to handle the fact that now terminal 1 and 0
     vertices may be replicated all over the place - added value conditions */
  /*------------------------------------------------------------------------*/
  OR_pattern2 = 0;
  if ((vertex->ELSE->value == 3) && (vertex->ELSE->multiple_fo == 0)
      && ((vertex->THEN == vertex->ELSE->THEN) || 
          (vertex->THEN->value == 0 && vertex->ELSE->THEN->value == 0) ||
          (vertex->THEN->value == 1 && vertex->ELSE->THEN->value == 1)
          )
      ) {
      OR_pattern2 = 1;
      cond_elseelse = 0;
      if (vertex->ELSE->ELSE->multiple_fo == 0) {
          compl_elseelse = act_vertex_ite_complemented(vertex->ELSE->ELSE);
          cond_elseelse = ((vertex->ELSE->ELSE->value == 3) || compl_elseelse);
          if (compl_elseelse) temp_elseelse_cost = 0;
          if (vertex->ELSE->ELSE->value == 3) {
              velseelseif_cost = act_map_ite(vertex->ELSE->ELSE->IF);
              velseelsethen_cost = act_map_ite(vertex->ELSE->ELSE->THEN);
              velseelseelse_cost = act_map_ite(vertex->ELSE->ELSE->ELSE);
              temp_elseelse_cost = velseelseif_cost + velseelsethen_cost + 
                  velseelseelse_cost;
          }
      }
  }
 
  /* conditions to be used multiple times later */
  /*--------------------------------------------*/
  compl_then = act_vertex_ite_complemented(vertex->THEN);
  compl_else = act_vertex_ite_complemented(vertex->ELSE);
  cond_then = ((vertex->THEN->value == 3) || (compl_then));
  cond_else = ((vertex->ELSE->value == 3) || (compl_else));

  /* if complemented phase, can realize it with the input mux, so cost = 0 */
  /*-----------------------------------------------------------------------*/ 
  if (compl_then) {
      temp_then_cost = 0;
  }
  if (vertex->THEN->value == 3) {
      if (vertex->THEN->multiple_fo) {
          vthenif_cost = vthenthen_cost = vthenelse_cost = MAXINT;
      }
      else {
          vthenif_cost = act_map_ite(vertex->THEN->IF);
          vthenthen_cost = act_map_ite(vertex->THEN->THEN);
          vthenelse_cost = act_map_ite(vertex->THEN->ELSE);
      }
      temp_then_cost = vthenif_cost + vthenthen_cost + vthenelse_cost;
  }

  if (compl_else) {
      temp_else_cost = 0;
  }
  if (vertex->ELSE->value == 3) {
      if (vertex->ELSE->multiple_fo) {
          velseif_cost = velsethen_cost = velseelse_cost = MAXINT;
      }
      else {
          velseif_cost = act_map_ite(vertex->ELSE->IF);
          velsethen_cost = act_map_ite(vertex->ELSE->THEN);
          velseelse_cost = act_map_ite(vertex->ELSE->ELSE);
      }
      temp_else_cost = velseif_cost + velsethen_cost + velseelse_cost;
  }

  /* enumerate possible cases */
  /*--------------------------*/

  /* if_cost + then_cost + else_cost + 1 */
  /*-------------------------------------*/
  cost[0] = vif_cost + vthen_cost + velse_cost + 1;
  
  if (cond_then) {
      cost[1] = vif_cost + temp_then_cost + velse_cost + 1;
  }

  if (cond_else) {
      cost[2] = vif_cost + vthen_cost + temp_else_cost + 1;
  }

  if (cond_then && cond_else) {
      cost[3] = vif_cost + temp_then_cost + temp_else_cost + 1;
  }

  if (!act_is_or_used) {
      /* find minimum cost */
      /*-------------------*/
      best_pat_num = minimum_cost_index(cost, TOTAL_PATTERNS);
      vertex->cost = cost[best_pat_num];
      vertex->pattern_num = best_pat_num;
      if (ACT_ITE_DEBUG > 3) 
         (void) printf("vertex id = %d, index = %d, cost = %d, name = %s, pattern_num = %d\n",
         vertex->id, vertex->index, vertex->cost, vertex->name, vertex->pattern_num);
      return vertex->cost;
  }

  /* OR patterns - recognized easily here - if vertex->IF->THEN is constant
     1, then the OR pattern is there.                                      */
  /*-----------------------------------------------------------------------*/

  if ((OR_pattern1 == 1) && (vertex->IF->multiple_fo == 0)) {
      cost[4] = temp_if_cost + vthen_cost + velse_cost + 1;
      
      if (cond_then) {
          cost[5] = temp_if_cost + temp_then_cost + velse_cost + 1;
      }
      
      if (cond_else) {
          cost[6] = temp_if_cost + vthen_cost + temp_else_cost + 1;
      }
      
      if (cond_then && cond_else) {
          cost[7] = temp_if_cost + temp_then_cost + temp_else_cost + 1;
      }
  }

  if (OR_pattern2 == 1) {
      cost[8] = vif_cost + temp_else_cost + 1;
      
      if (cond_elseelse) {
          cost[9] = vif_cost + vthen_cost + velseif_cost + temp_elseelse_cost + 1;
          assert(vthen_cost == 0);
      }
  }
  /* find minimum cost */
  /*-------------------*/
  best_pat_num = minimum_cost_index(cost, TOTAL_PATTERNS);
  vertex->cost = cost[best_pat_num];
  vertex->pattern_num = best_pat_num;
  if (ACT_ITE_DEBUG > 3) 
	(void) printf("vertex id = %d, index = %d, cost = %d, name = %s, pattern_num = %d\n",
	  vertex->id, vertex->index, vertex->cost, vertex->name, vertex->pattern_num);
  return vertex->cost;
}
        
/*--------------------------------------------------------------------
 returns the index of the pattern which results in the minimum cost
 at the present node.
---------------------------------------------------------------------*/
static
int minimum_cost_index(cost, num_entries)
 int cost[];
 int num_entries;
{

  int i;
  int min_index = 0;

  for (i = 1; i < num_entries; i++) {
    if (cost[i] < cost[min_index])   min_index = i;
  }
  return min_index;
}

/*------------------------------------------------------------------
  If the vertex is just realizing an inverter, return 1. Else 0.
-------------------------------------------------------------------*/
int
act_vertex_ite_complemented(vertex)
 ite_vertex_ptr vertex;
{
  return ((vertex->value == 2) && (vertex->phase == 0));
}

/*----------------------------------------------------------------------
   recursively initialize the required structure of each act vertex.
   also finds out multiple fan out vertices and stores them in the 
   multiple_fo_array.  
------------------------------------------------------------------------*/

static void
act_initialize_ite_area(vertex, multiple_fo_array)
   ite_vertex_ptr vertex;
   array_t *multiple_fo_array;
{
  /* to save repetition, mark the vertex as visited */
  /*------------------------------------------------*/
  if (vertex->mark == 0) vertex->mark = 1;
  else vertex->mark = 0;

  vertex->pattern_num = -1;
  vertex->cost = 0;
  vertex->arrival_time = (double) 0.0;
  vertex->mapped = 0;
  vertex->multiple_fo = 0;
  vertex->multiple_fo_for_mapping = 0; 

  /* do not care for a terminal vertex- nothing is to be done */
  /*----------------------------------------------------------*/
  if (vertex->value != 3) {
  	vertex->cost = 0;
	vertex->arrival_time = (double) (0.0);
  	return;
  }
  
  /* if the IF (THEN, ELSE) not visited earlier, initialize it, else
     mark it as a multiple fo vertex */
  /*---------------------------------------------------------------*/

  if (vertex->mark != vertex->IF->mark) {
      act_initialize_ite_area(vertex->IF, multiple_fo_array);
  }
  else {
      /* important to check that it is a multiple_fo vertex, else you may be
	 adding  a vertex too many times in the multiple_fo array.          */
      /*--------------------------------------------------------------------*/
      if (vertex->IF->multiple_fo == 0) {
         array_insert_last(ite_vertex_ptr, multiple_fo_array, vertex->IF);
      }
      vertex->IF->multiple_fo += 1;
      vertex->IF->multiple_fo_for_mapping += 1; 
  }      

  if (vertex->mark != vertex->THEN->mark) {
      act_initialize_ite_area(vertex->THEN, multiple_fo_array);
  }
  else {
      if (vertex->THEN->multiple_fo == 0) {
          array_insert_last(ite_vertex_ptr, multiple_fo_array, vertex->THEN);
      }
      vertex->THEN->multiple_fo += 1;
      vertex->THEN->multiple_fo_for_mapping += 1; 
  }
  if (vertex->mark != vertex->ELSE->mark) {
      act_initialize_ite_area(vertex->ELSE, multiple_fo_array);
  }
  else {
      if (vertex->ELSE->multiple_fo == 0) {
          array_insert_last(ite_vertex_ptr, multiple_fo_array, vertex->ELSE);
      }
      vertex->ELSE->multiple_fo += 1;
      vertex->ELSE->multiple_fo_for_mapping += 1; 
  }
}

/*------------------------------------------------------------------
  Given a node, returns the fanins of the node in the order_list.
  The order is just the order used in misII numbering of the fanins.
--------------------------------------------------------------------*/
node_t **
act_ite_order_fanins(node)
  node_t *node;
{
  node_t **order_list, *fanin;
  int i;
  
  order_list = ALLOC(node_t *, node_num_fanin(node));
  foreach_fanin(node, i, fanin) {
      order_list[i] = fanin;
  }
  return order_list;
}

/*
static
print_multiple_fo_array(multiple_fo_array)
  array_t *multiple_fo_array;
{
  int nsize;
  int i;
  ite_vertex_ptr v;

  (void) printf("---- printing multiple_fo_vertices---");
  nsize = array_n(multiple_fo_array);
  for (i = 0; i < nsize; i++) {
    v = array_fetch(ite_vertex_ptr, multiple_fo_array, i);
    (void) printf(" index = %d, id = %d, num_fanouts = %d\n ", 
                  v->index, v->id, v->multiple_fo + 1);
  }
  (void) printf("\n");
}
*/

/*
int 
act_ite_decomp_big_nodes(network, DECOMP_FANIN)
  network_t *network;
  int DECOMP_FANIN;  
{
  
  array_t *nodevec;
  int size_net, i, gain;
  node_t *node;

  nodevec = network_dfs(network);
  size_net = array_n(nodevec);
  for (i = 0; i < size_net; i++) {
      node = array_fetch(node_t *, nodevec, i);
      act_ite_decomp_big_node(node, &gain, DECOMP_FANIN);
      if gain >
  }
  array_free(nodevec);
}
*/

ite_free_cost_struct(cost_struct)
  ACT_ITE_COST_STRUCT *cost_struct;
{
  if (cost_struct == NIL (ACT_ITE_COST_STRUCT)) return;
  FREE(cost_struct);
}


/*-----------------------------------------------------------------------
  Used to make the bdd work along with the ite's.
------------------------------------------------------------------------*/
int
act_bdd_make_tree_and_map(node, act_of_node)
  node_t *node;
  ACT_VERTEX_PTR act_of_node;

{
  int 		num_acts;
  int 		i;
  int 		num_mux_struct;
  int 		TOTAL_MUX_STRUCT; /* cost of the network */
  ACT_VERTEX_PTR 	act;
  
  ACT_SET(node)->LOCAL_ACT->act->node_list = NIL(array_t );
  act_of_node = ACT_SET(node)->LOCAL_ACT->act->root;
  WHICH_ACT = act_of_node->my_type = ORDERED;
  act_of_node->node = node;

  TOTAL_MUX_STRUCT = 0;

  act_init_multiple_fo_array(act_of_node); /* insert the root */
  act_initialize_act_area(act_of_node, multiple_fo_array); 
  /* initialize the required structures */
  /* if (ACT_DEBUG) my_traverse_act(act_of_node);
     if (ACT_DEBUG) print_multiple_fo_array(multiple_fo_array); */
  num_acts = array_n(multiple_fo_array);
  /* changed the following traversal , now  bottom-up */
  /*--------------------------------------------------*/
  for (i = num_acts - 1; i >= 0; i--) {
      act = array_fetch(ACT_VERTEX_PTR, multiple_fo_array, i);
      PRESENT_ACT = act;
      num_mux_struct = map_act(act);
      TOTAL_MUX_STRUCT += num_mux_struct;
  }
  if (ACT_DEBUG) {
      (void) printf("total mux_structures used for the node %s = %d, arrival_time = %f\n",
                    node_long_name(node), TOTAL_MUX_STRUCT, act_of_node->arrival_time);
  }
  array_free(multiple_fo_array);
  act_act_free(node);
  return TOTAL_MUX_STRUCT;
}

