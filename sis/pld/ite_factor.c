#include "sis.h"
#include "pld_int.h" 
#include "ite_int.h"

/*-----------------------------------------------------
  Constructs an ite for the factored form of the node. 
  First contructs a network out of the node, does a 
  good-decomposition on the node. 
  Note that the routine constructs ites
  of the nodes of the network (corresponding to the node).
  To get the ite of the node, the fields in the ite 
  vertices have to be appropriately set to fanins of the 
  node. That is done in act_ite_get_ite().

  NOTE: Hoping to use it only if the function does not
  satisfy all the filters. So the cost > 2, it is not
  an AND, OR function etc.
-------------------------------------------------------*/
ite_vertex *
act_ite_create_from_factored_form(node, init_param)
  node_t *node;
  act_init_param_t *init_param;
{

  network_t *network_node;
  int i;
  node_t *n, *node1;
  array_t *nodevec;
  ite_vertex *ite;

  if (node_num_literal(node) == factor_num_literal(node)) return NIL (ite_vertex);

  network_node = network_create_from_node(node);
  decomp_good_network(network_node);
  /* decomp_tech_network(network_node, 2, 2); */
  
  nodevec = network_dfs(network_node);
  for (i = 0; i < array_n(nodevec); i++) {
      n = array_fetch(node_t *, nodevec, i);
      act_ite_intermediate_new_make_ite(n, init_param);      
      /* change node, value fields in the ites which are either buffers or inverters */
      /*-----------------------------------------------------------------------------*/
      act_ite_mroot_modify_fields_node(n);
  }
  assert(n->type == PRIMARY_OUTPUT);
  node1 = node_get_fanin(n, 0);
  ite = ACT_ITE_ite(node1);
  /* no need of next  statements */
  /* now set all the ite pointers to 0 */
  /*-----------------------------------*/
  /*
  for (i = 0; i < array_n(nodevec); i++) {
      n = array_fetch(node_t *, nodevec, i);
      ACT_ITE_ite(n) = NIL (ite_vertex);
  }
  */

  array_free(nodevec);
  /* freeing not needed */
  /*
    act_free_ite_network(network_node);
  */

  /* the ite vertices pointing to PIs of network_node 
     are made to point to fanins of node */
  act_ite_correct_PIs(ite, node);
  network_free(network_node);
  return ite;
}
      
/*---------------------------------------------------------
  Given a node, construct an ite from the factored form. 
  Map this ite, if it is better than the previous ite 
  (must exist), accept this mapping and free the previous 
  cost_struct. Returns the resulting gain.

  NOTE: If original cost = 0, may get now a cost of 1. We 
  do not check for special cases like NODE_0, NODE_BUF etc.
----------------------------------------------------------*/
int
act_ite_map_factored_form(node, init_param)
  node_t *node;
  act_init_param_t *init_param;
{
  
  ACT_ITE_COST_STRUCT *cost_node, *cost_node_original;
  int gain;

  if (node->type == PRIMARY_INPUT || node->type == PRIMARY_OUTPUT) return 0;

  cost_node_original = ACT_ITE_SLOT(node);

  act_ite_alloc(node);
  cost_node = ACT_ITE_SLOT(node);
  ACT_ITE_ite(node) = act_ite_create_from_factored_form(node, init_param);
  if (ACT_ITE_ite(node) == NIL (ite_vertex)) {
      act_ite_free(node);
      ACT_DEF_ITE_SLOT(node) = (char *) cost_node_original;      
      return 0;
  }

  cost_node->cost = act_ite_make_tree_and_map(node);
  cost_node->arrival_time = ACT_ITE_ite(node)->arrival_time;
  cost_node->node = node;
  
  gain = cost_node_original->cost - cost_node->cost;
  if (gain > 0) {
      ACT_DEF_ITE_SLOT(node) = (char *) cost_node_original;      
      act_free_ite_node(node);
      act_ite_free(node);
      ACT_DEF_ITE_SLOT(node) = (char *) cost_node;      
      return gain;
  }
  act_free_ite_node(node);
  act_ite_free(node);
  ACT_DEF_ITE_SLOT(node) = (char *) cost_node_original;      
  return 0;
}

/*--------------------------------------------------------------------
  Change the PI pointers of ite to point to the correct fanins of the 
  node.  They are right now pointing at the primary inputs of 
  the network that was made out of network_node.
----------------------------------------------------------------------*/
act_ite_correct_PIs(ite, node)
  ite_vertex *ite;
  node_t *node;
{
  network_t *network;
  ite_vertex *vertex;
  int i, j, flag;
  node_t *n;
  array_t *vertex_vec;
  st_table *table;

  network = node->network;
  vertex_vec = array_alloc(ite_vertex *, 0);
  table = st_init_table(st_ptrcmp, st_ptrhash);
  act_ite_get_value_2_vertices(ite, vertex_vec, table);
  st_free_table(table);
  for (i = 0; i < array_n(vertex_vec); i++) {
      vertex = array_fetch(ite_vertex *, vertex_vec, i);
      if (network) {
          vertex->fanin = network_find_node(network, node_long_name(vertex->fanin));
          assert(vertex->fanin);
          vertex->name = node_long_name(vertex->fanin);
      } else {
          flag = 0;
          foreach_fanin(node, j, n) {
              if (strcmp(node_long_name(vertex->fanin), node_long_name(n)) == 0) {
                  flag = 1;
                  break;
              }
          }
          if (!flag) {
              (void) printf("problem in correcting PI routine in ite_factor.c\n");
              exit(1);
          }
          vertex->fanin = n;
          vertex->name = node_long_name(n);
      }
  }
  array_free(vertex_vec);
}

/*----------------------------------------------------------
  Returns in the vertex_vec all vertices with value 2.
  Note: Works only for the case when value = 2 => phase = 1.
-----------------------------------------------------------*/
act_ite_get_value_2_vertices(ite, vertex_vec, table)
  ite_vertex *ite;
  array_t *vertex_vec;
  st_table *table;
{
  if (st_insert(table, (char *) ite, (char *) ite)) return;
  if (ite->value <= 1) return;
  if (ite->value == 2) {
      assert(ite->phase == 1);
      array_insert_last(ite_vertex *, vertex_vec, ite);
      return;
  }
  act_ite_get_value_2_vertices(ite->IF, vertex_vec, table);
  act_ite_get_value_2_vertices(ite->THEN, vertex_vec, table);
  act_ite_get_value_2_vertices(ite->ELSE, vertex_vec, table);
}  
      
