#include "sis.h"
#include "pld_int.h"
#include "ite_int.h"

act_ite_break_network(network, init_param)
  network_t *network;
  act_init_param_t *init_param;
{
  array_t *nodevec;
  int i;
  /* int is_one; */ /* = 1 if the node has cost > 2 */
  node_t *node;

  num_or_patterns = 0;
  nodevec = network_dfs(network);
  for (i = 0; i < array_n(nodevec); i++) {
      node = array_fetch(node_t *, nodevec, i);
      if (node->type == PRIMARY_INPUT || node->type == PRIMARY_OUTPUT) continue;
      /* is_one = act_ite_break_node(node, init_param); */
      (void) act_ite_break_node(node, init_param);
  }
  if (ACT_ITE_DEBUG > 4) assert(network_check(network));
  array_free(nodevec);
}

/*---------------------------------------------------------------------------
  Given the node, breaks it up into many nodes, each  with cost 1.
  Based on the bdd at that node. Network is changed. New nodes may be added
  and the functionality of the node may change. Also, breaks the bdd (act)
  in the cost_node and puts their appropriate parts in the node. After the
  bdd is broken, all the new bdd's are trees (not even leaf dag's). All the
  multiple fo points are duplicated. Care has to be taken for all the nodes
  so that name at the bdd's are correct.
----------------------------------------------------------------------------*/
int
act_ite_break_node(node, init_param)
  node_t *node;
  act_init_param_t *init_param;
{

  network_t *network;
  node_t *node1, *node_returned;
  ACT_ITE_COST_STRUCT *cost_struct;
  node_function_t node_fun;

  network = node_network(node);
  node_fun = node_function(node);
  if ((node_fun == NODE_PI) || (node_fun == NODE_PO)) return 0;

  if (init_param->MAP_METHOD == MAP_WITH_ITER || init_param->MAP_METHOD == MAP_WITH_JUST_DECOMP) {
      if (ACT_ITE_SLOT(node)->network) {
          if (ACT_ITE_SLOT(node)->cost != 1) {
              pld_replace_node_by_network(node, ACT_ITE_SLOT(node)->network);
          }
          network_free(ACT_ITE_SLOT(node)->network);
          ACT_ITE_SLOT(node)->network = NIL (network_t);
          assert(ACT_ITE_SLOT(node)->ite == NIL (ite_vertex));
          return 1;
      }
      assert(node_fun == NODE_0 || node_fun == NODE_1 || node_fun == NODE_BUF);
      return 0; /* cost should be at most 0 */
  }

  cost_struct = ACT_ITE_SLOT(node);
  if (cost_struct->cost <= 1) return 0;
  if (cost_struct->act) return (act_new_act_break_node(node));

  PRESENT_ITE = cost_struct->ite;
  ite_reset_mark_ite(PRESENT_ITE);
  ite_set_MARK_VALUE_ite(PRESENT_ITE);
  if (ACT_ITE_DEBUG) ite_check_ite(PRESENT_ITE, node);

  node_returned =  act_ite_get_function(network, node, PRESENT_ITE);
  node1 = node_get_fanin(node_returned, 0);
  node_free(node_returned);

  /* all the multiple fo vertices had a node in the node field. Free that node */
  /*---------------------------------------------------------------------------*/
  ite_set_MARK_VALUE_ite(PRESENT_ITE);
  ite_free_nodes_in_multiple_fo_ite(PRESENT_ITE);
  /* free the structure of node */
  /*----------------------------*/
  ite_my_free(cost_struct->ite);
  cost_struct->ite = NIL (ite_vertex); /* without it, no good */
  node_replace(node, node1); 
  
  return 1;
}

/**************************************************************************************
  Given a node and its corresponding ACT(OACT), breaks the node into nodes, each 
  new node realizing a function that can be implemented by one basic block. Puts all 
  these nodes in the network. Original node is changed. 
***************************************************************************************/
node_t *
act_ite_get_function(network, node, vertex)
  network_t *network;
  node_t *node;
  ite_vertex_ptr vertex;
{

  node_t *node1, *node2, *ELSE_THEN, *ELSE_ELSE, *ELSE_IF, *ELSE_ELSE_IF, *ELSE_ELSE_THEN, *ELSE_ELSE_ELSE;
  node_t *IF, *THEN, *ELSE, *IF_THEN, *IF_ELSE, *IF_IF, *THEN_THEN, *THEN_ELSE, *THEN_IF;

  /* vertex->node corresponds to the node at the vertex - only for the  
     multiple_fanout vertex */
  /*-----------------------------------------------------------------*/

  if (vertex->value == 0) {
      vertex->mark = MARK_COMPLEMENT_VALUE;
      return node_constant(0);
  }
  if (vertex->value == 1) {
      vertex->mark = MARK_COMPLEMENT_VALUE;
      return node_constant(1);
  }


  if (vertex->mark == MARK_COMPLEMENT_VALUE) {
	if (vertex->multiple_fo == 0) {
		(void) printf("error: act_ite_get_function()\n");
		exit(1);
	}
	return vertex->node;
   }

  /* added April 4, 1991*/
  /*-------------------*/
  if (!act_is_or_used) {
      assert(vertex->pattern_num < 4);
  }

  /* even multiple fo vertices will be broken when visited first time */
  /*------------------------------------------------------------------*/
  vertex->mark = MARK_COMPLEMENT_VALUE;

  if (vertex->value == 2 && vertex->phase == 1) {
      node1 = ite_get_node_literal_of_vertex(vertex);
      if (vertex->multiple_fo != 0) vertex->node = node1;  
      return node1;
  }
  /* handle inverter */
  /*-----------------*/
  if (vertex->value == 2 && vertex->phase == 0) {
      assert(vertex->pattern_num == 0);
      node1 = node_literal(vertex->fanin, 0);
      if (vertex->multiple_fo != 0) vertex->node = node1;
      return node1;
  }

  /* vertex->value = 3 */
  /* if simply an input variable, handle separately */
  /*------------------------------------------------*/
  if ((vertex->IF->value == 2) && (vertex->IF->phase == 1) 
       && (vertex->THEN->value == 1) && (vertex->ELSE->value == 0)) {
      vertex->IF->mark = MARK_COMPLEMENT_VALUE;
      vertex->THEN->mark = MARK_COMPLEMENT_VALUE;
      vertex->ELSE->mark = MARK_COMPLEMENT_VALUE;
      node1 = ite_get_node_literal_of_vertex(vertex->IF);
      if (vertex->multiple_fo != 0) vertex->node = node1;  
      return node1;
  }
  
  if (vertex->pattern_num > 3) num_or_patterns++;

  switch(vertex->pattern_num) {

     case 0: 
	IF = act_ite_get_function(network, node, vertex->IF);
	THEN = act_ite_get_function(network, node, vertex->THEN);
	ELSE = act_ite_get_function(network, node, vertex->ELSE);
        node1 = act_mux_node(IF, ELSE, THEN);
        if (PRESENT_ITE != vertex) {
            network_add_node(network, node1);
        }

	ite_free_node_if_possible(IF, vertex->IF);
 	ite_free_node_if_possible(THEN, vertex->THEN);
 	ite_free_node_if_possible(ELSE, vertex->ELSE);        

        node2 = node_literal(node1, 1);
        if (vertex->multiple_fo != 0) vertex->node = node2; 
        return node2;

     case 1:
	vertex->THEN->mark = MARK_COMPLEMENT_VALUE;
        THEN_IF = act_ite_get_function(network, node, vertex->THEN->IF);
        THEN_THEN = act_ite_get_function(network, node, vertex->THEN->THEN); 
        THEN_ELSE = act_ite_get_function(network, node, vertex->THEN->ELSE); 
        THEN = act_mux_node(THEN_IF, THEN_ELSE, THEN_THEN);
        IF = act_ite_get_function(network, node, vertex->IF);
        ELSE = act_ite_get_function(network, node, vertex->ELSE);
        node1 = act_mux_node(IF, ELSE, THEN);
        if (PRESENT_ITE != vertex) {
            network_add_node(network, node1);
        }
        
	ite_free_node_if_possible(IF, vertex->IF);
 	ite_free_node_if_possible(ELSE, vertex->ELSE);        
 	ite_free_node_if_possible(THEN_IF, vertex->THEN->IF);
 	ite_free_node_if_possible(THEN_THEN, vertex->THEN->THEN);
 	ite_free_node_if_possible(THEN_ELSE, vertex->THEN->ELSE);
        node_free(THEN);

        node2 = node_literal(node1, 1);
        if (vertex->multiple_fo != 0) vertex->node = node2; 
        return node2;

     case 2:
	vertex->ELSE->mark = MARK_COMPLEMENT_VALUE;
        ELSE_IF = act_ite_get_function(network, node, vertex->ELSE->IF);
        ELSE_ELSE = act_ite_get_function(network, node, vertex->ELSE->ELSE); 
        ELSE_THEN = act_ite_get_function(network, node, vertex->ELSE->THEN); 
        ELSE = act_mux_node(ELSE_IF, ELSE_ELSE, ELSE_THEN);
        IF = act_ite_get_function(network, node, vertex->IF);
        THEN = act_ite_get_function(network, node, vertex->THEN);
        node1 = act_mux_node(IF, ELSE, THEN);
        if (PRESENT_ITE != vertex) {
            network_add_node(network, node1);
        }
        
	ite_free_node_if_possible(IF, vertex->IF);
 	ite_free_node_if_possible(THEN, vertex->THEN);        
 	ite_free_node_if_possible(ELSE_IF, vertex->ELSE->IF);
 	ite_free_node_if_possible(ELSE_ELSE, vertex->ELSE->ELSE);
 	ite_free_node_if_possible(ELSE_THEN, vertex->ELSE->THEN);
        node_free(ELSE);

        node2 = node_literal(node1, 1);
        if (vertex->multiple_fo != 0) vertex->node = node2; 
        return node2;

     case 3:
	vertex->THEN->mark = MARK_COMPLEMENT_VALUE;
	vertex->ELSE->mark = MARK_COMPLEMENT_VALUE;

        THEN_IF = act_ite_get_function(network, node, vertex->THEN->IF);
        THEN_THEN = act_ite_get_function(network, node, vertex->THEN->THEN); 
        THEN_ELSE = act_ite_get_function(network, node, vertex->THEN->ELSE); 
        THEN = act_mux_node(THEN_IF, THEN_ELSE, THEN_THEN);

        ELSE_IF = act_ite_get_function(network, node, vertex->ELSE->IF);
        ELSE_ELSE = act_ite_get_function(network, node, vertex->ELSE->ELSE); 
        ELSE_THEN = act_ite_get_function(network, node, vertex->ELSE->THEN); 
        ELSE = act_mux_node(ELSE_IF, ELSE_ELSE, ELSE_THEN);

        IF = act_ite_get_function(network, node, vertex->IF);
        node1 = act_mux_node(IF, ELSE, THEN);
        if (PRESENT_ITE != vertex) {
            network_add_node(network, node1);
        }
        
	ite_free_node_if_possible(IF, vertex->IF);
 	ite_free_node_if_possible(THEN_IF, vertex->THEN->IF);
 	ite_free_node_if_possible(THEN_THEN, vertex->THEN->THEN);
 	ite_free_node_if_possible(THEN_ELSE, vertex->THEN->ELSE);
 	ite_free_node_if_possible(ELSE_IF, vertex->ELSE->IF);
 	ite_free_node_if_possible(ELSE_ELSE, vertex->ELSE->ELSE);
 	ite_free_node_if_possible(ELSE_THEN, vertex->ELSE->THEN);
        node_free(THEN);
        node_free(ELSE);

        node2 = node_literal(node1, 1);
        if (vertex->multiple_fo != 0) vertex->node = node2; 
        return node2;

     case 4: 
        vertex->IF->mark = MARK_COMPLEMENT_VALUE;
        IF_IF = act_ite_get_function(network, node, vertex->IF->IF);
        IF_THEN = act_ite_get_function(network, node, vertex->IF->THEN); 
        IF_ELSE = act_ite_get_function(network, node, vertex->IF->ELSE); 
        IF = act_mux_node(IF_IF, IF_ELSE, IF_THEN);
	THEN = act_ite_get_function(network, node, vertex->THEN);
	ELSE = act_ite_get_function(network, node, vertex->ELSE);
        node1 = act_mux_node(IF, ELSE, THEN);
        if (PRESENT_ITE != vertex) {
            network_add_node(network, node1);
        }

	ite_free_node_if_possible(IF_IF, vertex->IF->IF);
	ite_free_node_if_possible(IF_THEN, vertex->IF->THEN);
	ite_free_node_if_possible(IF_ELSE, vertex->IF->ELSE);
 	ite_free_node_if_possible(THEN, vertex->THEN);
 	ite_free_node_if_possible(ELSE, vertex->ELSE);        
        node_free(IF);

        node2 = node_literal(node1, 1);
        if (vertex->multiple_fo != 0) vertex->node = node2; 
        return node2;

     case 5:
	vertex->IF->mark = MARK_COMPLEMENT_VALUE;
	vertex->THEN->mark = MARK_COMPLEMENT_VALUE;

        IF_IF = act_ite_get_function(network, node, vertex->IF->IF);
        IF_THEN = act_ite_get_function(network, node, vertex->IF->THEN); 
        IF_ELSE = act_ite_get_function(network, node, vertex->IF->ELSE); 
        IF = act_mux_node(IF_IF, IF_ELSE, IF_THEN);

        THEN_IF = act_ite_get_function(network, node, vertex->THEN->IF);
        THEN_THEN = act_ite_get_function(network, node, vertex->THEN->THEN); 
        THEN_ELSE = act_ite_get_function(network, node, vertex->THEN->ELSE); 
        THEN = act_mux_node(THEN_IF, THEN_ELSE, THEN_THEN);

        ELSE = act_ite_get_function(network, node, vertex->ELSE);
        node1 = act_mux_node(IF, ELSE, THEN);
        if (PRESENT_ITE != vertex) {
            network_add_node(network, node1);
        }
        
	ite_free_node_if_possible(IF_IF, vertex->IF->IF);
	ite_free_node_if_possible(IF_THEN, vertex->IF->THEN);
	ite_free_node_if_possible(IF_ELSE, vertex->IF->ELSE);
 	ite_free_node_if_possible(ELSE, vertex->ELSE);        
 	ite_free_node_if_possible(THEN_IF, vertex->THEN->IF);
 	ite_free_node_if_possible(THEN_THEN, vertex->THEN->THEN);
 	ite_free_node_if_possible(THEN_ELSE, vertex->THEN->ELSE);
        node_free(IF);
        node_free(THEN);

        node2 = node_literal(node1, 1);
        if (vertex->multiple_fo != 0) vertex->node = node2; 
        return node2;

     case 6:
	vertex->IF->mark = MARK_COMPLEMENT_VALUE;
	vertex->ELSE->mark = MARK_COMPLEMENT_VALUE;

        IF_IF = act_ite_get_function(network, node, vertex->IF->IF);
        IF_THEN = act_ite_get_function(network, node, vertex->IF->THEN); 
        IF_ELSE = act_ite_get_function(network, node, vertex->IF->ELSE); 
        IF = act_mux_node(IF_IF, IF_ELSE, IF_THEN);

        ELSE_IF = act_ite_get_function(network, node, vertex->ELSE->IF);
        ELSE_ELSE = act_ite_get_function(network, node, vertex->ELSE->ELSE); 
        ELSE_THEN = act_ite_get_function(network, node, vertex->ELSE->THEN); 
        ELSE = act_mux_node(ELSE_IF, ELSE_ELSE, ELSE_THEN);

        THEN = act_ite_get_function(network, node, vertex->THEN);
        node1 = act_mux_node(IF, ELSE, THEN);
        if (PRESENT_ITE != vertex) {
            network_add_node(network, node1);
        }
        
	ite_free_node_if_possible(IF_IF, vertex->IF->IF);
	ite_free_node_if_possible(IF_THEN, vertex->IF->THEN);
	ite_free_node_if_possible(IF_ELSE, vertex->IF->ELSE);
 	ite_free_node_if_possible(THEN, vertex->THEN);        
 	ite_free_node_if_possible(ELSE_IF, vertex->ELSE->IF);
 	ite_free_node_if_possible(ELSE_ELSE, vertex->ELSE->ELSE);
 	ite_free_node_if_possible(ELSE_THEN, vertex->ELSE->THEN);
        node_free(IF);
        node_free(ELSE);

        node2 = node_literal(node1, 1);
        if (vertex->multiple_fo != 0) vertex->node = node2; 
        return node2;

     case 7:
	vertex->IF->mark = MARK_COMPLEMENT_VALUE;
	vertex->THEN->mark = MARK_COMPLEMENT_VALUE;
	vertex->ELSE->mark = MARK_COMPLEMENT_VALUE;

        IF_IF = act_ite_get_function(network, node, vertex->IF->IF);
        IF_THEN = act_ite_get_function(network, node, vertex->IF->THEN); 
        IF_ELSE = act_ite_get_function(network, node, vertex->IF->ELSE); 
        IF = act_mux_node(IF_IF, IF_ELSE, IF_THEN);

        THEN_IF = act_ite_get_function(network, node, vertex->THEN->IF);
        THEN_THEN = act_ite_get_function(network, node, vertex->THEN->THEN); 
        THEN_ELSE = act_ite_get_function(network, node, vertex->THEN->ELSE); 
        THEN = act_mux_node(THEN_IF, THEN_ELSE, THEN_THEN);

        ELSE_IF = act_ite_get_function(network, node, vertex->ELSE->IF);
        ELSE_ELSE = act_ite_get_function(network, node, vertex->ELSE->ELSE); 
        ELSE_THEN = act_ite_get_function(network, node, vertex->ELSE->THEN); 
        ELSE = act_mux_node(ELSE_IF, ELSE_ELSE, ELSE_THEN);

        node1 = act_mux_node(IF, ELSE, THEN);
        if (PRESENT_ITE != vertex) {
            network_add_node(network, node1);
        }
        
	ite_free_node_if_possible(IF_IF, vertex->IF->IF);
	ite_free_node_if_possible(IF_THEN, vertex->IF->THEN);
	ite_free_node_if_possible(IF_ELSE, vertex->IF->ELSE);
 	ite_free_node_if_possible(THEN_IF, vertex->THEN->IF);
 	ite_free_node_if_possible(THEN_THEN, vertex->THEN->THEN);
 	ite_free_node_if_possible(THEN_ELSE, vertex->THEN->ELSE);
 	ite_free_node_if_possible(ELSE_IF, vertex->ELSE->IF);
 	ite_free_node_if_possible(ELSE_ELSE, vertex->ELSE->ELSE);
 	ite_free_node_if_possible(ELSE_THEN, vertex->ELSE->THEN);
        node_free(IF);
        node_free(THEN);
        node_free(ELSE);

        node2 = node_literal(node1, 1);
        if (vertex->multiple_fo != 0) vertex->node = node2; 
        return node2;

     case 8:
	vertex->ELSE->mark = MARK_COMPLEMENT_VALUE;

        IF = act_ite_get_function(network, node, vertex->IF); 
        ELSE_THEN = THEN = act_ite_get_function(network, node, vertex->THEN);

        ELSE_ELSE = act_ite_get_function(network, node, vertex->ELSE->ELSE);
        ELSE_IF = act_ite_get_function(network, node, vertex->ELSE->IF);
        ELSE = act_mux_node(ELSE_IF, ELSE_ELSE, ELSE_THEN);

        node1 = act_mux_node(IF, ELSE, THEN);
        if (PRESENT_ITE != vertex) {
            network_add_node(network, node1);
        }
        
	ite_free_node_if_possible(IF, vertex->IF);
	ite_free_node_if_possible(THEN, vertex->THEN);
	ite_free_node_if_possible(ELSE_ELSE, vertex->ELSE->ELSE);
 	ite_free_node_if_possible(ELSE_IF, vertex->ELSE->IF);
        node_free(ELSE);

        node2 = node_literal(node1, 1);
        if (vertex->multiple_fo != 0) vertex->node = node2; 
        return node2;

     case 9:
	vertex->ELSE->mark = MARK_COMPLEMENT_VALUE;
	vertex->ELSE->ELSE->mark = MARK_COMPLEMENT_VALUE;

        IF = act_ite_get_function(network, node, vertex->IF); 
        ELSE_THEN = THEN = act_ite_get_function(network, node, vertex->THEN);

        ELSE_ELSE_IF = act_ite_get_function(network, node, vertex->ELSE->ELSE->IF);
        ELSE_ELSE_THEN = act_ite_get_function(network, node, vertex->ELSE->ELSE->THEN);
        ELSE_ELSE_ELSE = act_ite_get_function(network, node, vertex->ELSE->ELSE->ELSE);
        ELSE_ELSE = act_mux_node(ELSE_ELSE_IF, ELSE_ELSE_ELSE, ELSE_ELSE_THEN);
        ELSE_IF = act_ite_get_function(network, node, vertex->ELSE->IF);
        ELSE = act_mux_node(ELSE_IF, ELSE_ELSE, ELSE_THEN);

        node1 = act_mux_node(IF, ELSE, THEN);
        if (PRESENT_ITE != vertex) {
            network_add_node(network, node1);
        }
        
	ite_free_node_if_possible(IF, vertex->IF);
	ite_free_node_if_possible(THEN, vertex->THEN);
	ite_free_node_if_possible(ELSE_ELSE_IF, vertex->ELSE->ELSE->IF);
 	ite_free_node_if_possible(ELSE_ELSE_THEN, vertex->ELSE->ELSE->THEN);
 	ite_free_node_if_possible(ELSE_ELSE_ELSE, vertex->ELSE->ELSE->ELSE);
 	ite_free_node_if_possible(ELSE_IF, vertex->ELSE->IF);
        node_free(ELSE);
        node_free(ELSE_ELSE);

        node2 = node_literal(node1, 1);
        if (vertex->multiple_fo != 0) vertex->node = node2; 
        return node2;


     default:
       (void) printf(" act_ite_get_function(): unknown pattern_num %d found\n", vertex->pattern_num);
       exit(1);
  } /* switch */        
  return NIL (node_t);
}

ite_set_MARK_VALUE_ite(ite)
  ite_vertex *ite;
{

  MARK_VALUE = ite->mark;
  if (MARK_VALUE == 0) MARK_COMPLEMENT_VALUE = 1;
  else {
      assert (MARK_VALUE == 1);
      MARK_COMPLEMENT_VALUE = 0;
  }
}


/*---------------------------------------------------------------
  Resets the mark field of all ite's in the tree rooted at ite.
---------------------------------------------------------------*/
ite_reset_mark_ite(ite)
  ite_vertex *ite;
{
  st_table *table;
  char *key;
  st_generator *stgen;
  ite_vertex *vertex;

  table = ite_my_traverse_ite(ite);
  st_foreach_item(table, stgen, &key, (char **) &vertex) {
      vertex->mark = 0;
  }
  st_free_table(table);
}

/***********************************************************************************
   If the vertex is just an input, then the node cannot be freed. If the vertex is
   a multiple_fo point, then also it cannot be freed. This is because these nodes 
   are needed in the network. 
************************************************************************************/
ite_free_node_if_possible(node, vertex)
  node_t *node; 
  ite_vertex *vertex;
{
  node_function_t node_fn;

  node_fn = node_function(node);
  if ((node_fn == NODE_0) || (node_fn == NODE_1)){
       node_free(node);
       return;
  }
  /* should be after the node_fn check, because 0, 1 vertices 
     do not store anything */
  /*---------------------------------------------------------*/
  if (vertex->multiple_fo != 0) return; 
  node_free(node); 
}  

int
act_new_act_break_node(node)
  node_t *node;
{
  network_t *network;
  node_t *node_returned, *node1;
  ACT_ITE_COST_STRUCT *cost_struct;

  network = node->network;
  cost_struct = ACT_ITE_SLOT(node);

  PRESENT_ACT = cost_struct->act;
  set_mark_act(PRESENT_ACT);
  if (ACT_ITE_DEBUG) act_check(PRESENT_ACT);

  node_returned =  act_new_act_get_function(network, node, PRESENT_ACT);
  node1 = node_get_fanin(node_returned, 0);
  node_free(node_returned);

  /* all the multiple fo vertices had a node in the node field. Free that node */
  /*---------------------------------------------------------------------------*/
  set_mark_act(PRESENT_ACT);
  act_free_nodes_in_multiple_fo_act(PRESENT_ACT);
  /* free the structure of node */
  /*----------------------------*/
  my_free_act(cost_struct->act);
  cost_struct->act = NIL (ACT_VERTEX); /* without it, no good */
  node_replace(node, node1); 
  return 1;
}

/*---------------------------------------------------------------
  Checks a few things about ite of the node.
---------------------------------------------------------------*/
ite_check_ite(ite, node)
  ite_vertex *ite;
  node_t *node;
{
  st_table *table;
  char *key;
  st_generator *stgen;
  ite_vertex *vertex;

  table = ite_my_traverse_ite(ite);
  st_foreach_item(table, stgen, &key, (char **) &vertex) {
      if (vertex->pattern_num < 0) {
          if (vertex->cost > 0) {
              (void) printf("ite of node %s has wrong pattern num\n", node_long_name(node));
              exit(1);
          }
      }
  }
  st_free_table(table);
}

/**************************************************************************************
  Given a node and its corresponding ACT(OACT), breaks the node into nodes, each 
  new node realizing a function that can be implemented by one basic block. Puts all 
  these nodes in the network. Original node is changed. 
***************************************************************************************/
node_t *
act_new_act_get_function(network, node, vertex)
  network_t *network;
  node_t *node;
  ACT_VERTEX_PTR vertex;
{

  node_t *node1, *node2, *ELSE_THEN, *ELSE_ELSE, *ELSE_IF, *ELSE_ELSE_IF, *ELSE_ELSE_THEN, *ELSE_ELSE_ELSE;
  node_t *IF, *THEN, *ELSE, *THEN_THEN, *THEN_ELSE, *THEN_IF;
  node_t *THEN_ELSE_ELSE, *THEN_ELSE_THEN, *THEN_ELSE_IF;

  /* vertex->node corresponds to the node at the vertex - only for the  
     multiple_fanout vertex */
  /*-----------------------------------------------------------------*/

  if (vertex->value == 0) {
      vertex->mark = MARK_COMPLEMENT_VALUE;
      return node_constant(0);
  }
  if (vertex->value == 1) {
      vertex->mark = MARK_COMPLEMENT_VALUE;
      return node_constant(1);
  }


  if (vertex->mark == MARK_COMPLEMENT_VALUE) {
	if (vertex->multiple_fo == 0) {
		(void) printf("error: act_get_function()\n");
		exit(1);
	}
	return vertex->node;
   }

  /* added April 4, 1991*/
  /*-------------------*/
  if (!act_is_or_used) {
      assert(vertex->pattern_num < 4);
  }

  /* even multiple fo vertices will be mapped when they are visited first time */
  /*---------------------------------------------------------------------------*/
  vertex->mark = MARK_COMPLEMENT_VALUE;

  if ((vertex->low->value == 0) && (vertex->high->value == 1)) {
      node1 = get_node_literal_of_vertex(vertex, network);
      if (vertex->multiple_fo != 0) vertex->node = node1;  
      return node1;
  }

  if (vertex->pattern_num > 3) num_or_patterns++;

  switch(vertex->pattern_num) {

     case 0: 

        ELSE = act_new_act_get_function(network, node, vertex->low);
	THEN = act_new_act_get_function(network, node, vertex->high);
        IF = get_node_literal_of_vertex(vertex, network);
        node1 = act_mux_node(IF, ELSE, THEN);
        if (PRESENT_ACT != vertex) {
            network_add_node(network, node1);
        }

	free_node_if_possible(ELSE, vertex->low);
 	free_node_if_possible(THEN, vertex->high);
	node_free(IF);

        node2 = node_literal(node1, 1);
        if (vertex->multiple_fo != 0) vertex->node = node2; 
        return node2;

     case 1:

	vertex->low->mark = MARK_COMPLEMENT_VALUE;
	ELSE_ELSE = act_new_act_get_function(network, node, vertex->low->low);
	ELSE_THEN = act_new_act_get_function(network, node, vertex->low->high);
	ELSE_IF = get_node_literal_of_vertex(vertex->low, network);
        ELSE = act_mux_node(ELSE_IF, ELSE_ELSE, ELSE_THEN);
        IF = get_node_literal_of_vertex(vertex, network);
        THEN = act_new_act_get_function(network, node, vertex->high);
        node1 = act_mux_node(IF, ELSE, THEN);
        if (PRESENT_ACT != vertex) {
            network_add_node(network, node1);
        }
        
 	free_node_if_possible(THEN, vertex->high);  
 	free_node_if_possible(ELSE_ELSE, vertex->low->low);
 	free_node_if_possible(ELSE_THEN, vertex->low->high);
        node_free(ELSE_IF);
        node_free(IF);
        node_free(ELSE);

        node2 = node_literal(node1, 1);
        if (vertex->multiple_fo != 0) vertex->node = node2; 
        return node2;

     case 2:
	vertex->high->mark = MARK_COMPLEMENT_VALUE;

	THEN_ELSE = act_new_act_get_function(network, node, vertex->high->low);
	THEN_THEN = act_new_act_get_function(network, node, vertex->high->high);
	THEN_IF = get_node_literal_of_vertex(vertex->high, network);
        THEN = act_mux_node(THEN_IF, THEN_ELSE, THEN_THEN);
        IF = get_node_literal_of_vertex(vertex, network);
        ELSE = act_new_act_get_function(network, node, vertex->low);
        node1 = act_mux_node(IF, ELSE, THEN);
        if (PRESENT_ACT != vertex) {
            network_add_node(network, node1);
        }
        
 	free_node_if_possible(THEN_ELSE, vertex->high->low);
 	free_node_if_possible(THEN_THEN, vertex->high->high);
 	free_node_if_possible(ELSE, vertex->low);        
        node_free(THEN_IF);
        node_free(IF);
        node_free(THEN);

        node2 = node_literal(node1, 1);
        if (vertex->multiple_fo != 0) vertex->node = node2; 
        return node2;


     case 3:
	vertex->low->mark = MARK_COMPLEMENT_VALUE;
	vertex->high->mark = MARK_COMPLEMENT_VALUE;

	ELSE_ELSE = act_new_act_get_function(network, node, vertex->low->low);
	ELSE_THEN = act_new_act_get_function(network, node, vertex->low->high);
	ELSE_IF = get_node_literal_of_vertex(vertex->low, network);
        ELSE = act_mux_node(ELSE_IF, ELSE_ELSE, ELSE_THEN);

	THEN_ELSE = act_new_act_get_function(network, node, vertex->high->low);
	THEN_THEN = act_new_act_get_function(network, node, vertex->high->high);
	THEN_IF = get_node_literal_of_vertex(vertex->high, network);
        THEN = act_mux_node(THEN_IF, THEN_ELSE, THEN_THEN);

        IF = get_node_literal_of_vertex(vertex, network);
        node1 = act_mux_node(IF, ELSE, THEN);

        if (PRESENT_ACT != vertex) {
            network_add_node(network, node1);
        }
        
 	free_node_if_possible(ELSE_ELSE, vertex->low->low);
 	free_node_if_possible(ELSE_THEN, vertex->low->high);
 	free_node_if_possible(THEN_ELSE, vertex->high->low);
 	free_node_if_possible(THEN_THEN, vertex->high->high);

        node_free(ELSE_IF);
        node_free(THEN_IF);
        node_free(IF);
        node_free(THEN);
        node_free(ELSE);

        node2 = node_literal(node1, 1);
        if (vertex->multiple_fo != 0) vertex->node = node2; 
        return node2;

     case 4:
	vertex->low->low->mark = MARK_COMPLEMENT_VALUE;
	vertex->low->mark = MARK_COMPLEMENT_VALUE;

	ELSE_ELSE_ELSE = act_new_act_get_function(network, node, vertex->low->low->low);
	ELSE_ELSE_THEN = act_new_act_get_function(network, node, vertex->low->low->high);
	ELSE_ELSE_IF = get_node_literal_of_vertex(vertex->low->low, network);
        ELSE_ELSE = act_mux_node(ELSE_ELSE_IF, ELSE_ELSE_ELSE, ELSE_ELSE_THEN);

        ELSE_THEN = THEN = act_new_act_get_function(network, node, vertex->high);
        ELSE_IF = get_node_literal_of_vertex(vertex->low, network);
        ELSE = act_mux_node(ELSE_IF, ELSE_ELSE, ELSE_THEN);

        IF = get_node_literal_of_vertex(vertex, network);
        
        node1 = act_mux_node(IF, ELSE, THEN);
        if (PRESENT_ACT != vertex) {
            network_add_node(network, node1);
        }
        
 	free_node_if_possible(ELSE_ELSE_ELSE, vertex->low->low->low);
 	free_node_if_possible(ELSE_ELSE_THEN, vertex->low->low->high);
 	free_node_if_possible(THEN, vertex->high);

        node_free(ELSE_ELSE_IF);
        node_free(ELSE_ELSE);
        node_free(ELSE_IF);
        node_free(IF);
        node_free(ELSE);

        node2 = node_literal(node1, 1);
        if (vertex->multiple_fo != 0) vertex->node = node2; 
        return node2;

     case 5:;
     case 6:
	vertex->high->mark = MARK_COMPLEMENT_VALUE;
	vertex->high->low->mark = MARK_COMPLEMENT_VALUE;
        
        THEN_ELSE_ELSE = act_new_act_get_function(network, node, vertex->high->low->low);
        THEN_ELSE_THEN = THEN_THEN = act_new_act_get_function(network, node, vertex->high->low->high);
        THEN_ELSE_IF = get_node_literal_of_vertex(vertex->high->low, network);
        THEN_ELSE = act_mux_node(THEN_ELSE_IF, THEN_ELSE_ELSE, THEN_ELSE_THEN);
        
        THEN_IF = get_node_literal_of_vertex(vertex->high, network);
        THEN = act_mux_node(THEN_IF, THEN_ELSE, THEN_THEN);
        ELSE = act_new_act_get_function(network, node, vertex->low);
        IF = get_node_literal_of_vertex(vertex, network);

        node1 = act_mux_node(IF, ELSE, THEN);
        if (PRESENT_ACT != vertex) {
            network_add_node(network, node1);
        }
        
 	free_node_if_possible(THEN_ELSE_ELSE, vertex->high->low->low);
 	free_node_if_possible(THEN_ELSE_THEN, vertex->high->low->high);
 	free_node_if_possible(ELSE, vertex->low);

        node_free(THEN_ELSE_IF);
        node_free(THEN_ELSE);
        node_free(THEN_IF);
        node_free(THEN);
        node_free(IF);

        node2 = node_literal(node1, 1);
        if (vertex->multiple_fo != 0) vertex->node = node2; 
        return node2;

     case 7:
	vertex->low->mark = MARK_COMPLEMENT_VALUE;
	vertex->low->low->mark = MARK_COMPLEMENT_VALUE;
        ELSE_ELSE_THEN = ELSE_THEN = act_new_act_get_function(network, node, vertex->low->low->high);
        ELSE_ELSE_ELSE = act_new_act_get_function(network, node, vertex->low->low->low);
        ELSE_ELSE_IF = get_node_literal_of_vertex(vertex->low->low, network);
	ELSE_ELSE = act_mux_node(ELSE_ELSE_IF, ELSE_ELSE_ELSE, ELSE_ELSE_THEN);
	ELSE_IF = get_node_literal_of_vertex(vertex->low, network);
        ELSE = act_mux_node(ELSE_IF, ELSE_ELSE, ELSE_THEN);
        IF = get_node_literal_of_vertex(vertex, network);
        THEN = act_new_act_get_function(network, node, vertex->high);
        node1 = act_mux_node(IF, ELSE, THEN);
        if (PRESENT_ACT != vertex) {
            network_add_node(network, node1);
        }
        
 	free_node_if_possible(ELSE_ELSE_THEN, vertex->low->low->high);
 	free_node_if_possible(ELSE_ELSE_ELSE, vertex->low->low->low);
 	free_node_if_possible(THEN, vertex->high);  
        node_free(ELSE_ELSE_IF);
        node_free(ELSE_ELSE);
        node_free(ELSE_IF);
        node_free(ELSE);
        node_free(IF);

        node2 = node_literal(node1, 1);
        if (vertex->multiple_fo != 0) vertex->node = node2; 
        return node2;


     default:
       (void) printf(" act_get_function(): unknown pattern_num %d found\n", vertex->pattern_num);
       exit(1);
  } /* switch */        
  return NIL (node_t);
}
