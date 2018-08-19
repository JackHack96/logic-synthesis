#include "sis.h"
#include "pld_int.h"
#include "ite_int.h"

/* Log of changes
*  Jan 16 '93: In my_traverse_ite(), ite vertices were visited
*  despite having been visited earlier. Fixed it. - Rajeev Murgai
*/

/*--------------------------------------------------------
  Frees the cost struct of n1, replaces it with that of 
  n2. If match is non-nil, rebuilds the 
  match, frees the old one and generates the new one.
  Frees the ite and match of cost_struct of n1 also. 
  Makes the ACT_ITE_SLOT of n2 NIL.
---------------------------------------------------------*/
act_ite_cost_struct_replace(n1, n2, map_alg)
  node_t *n1, *n2;
  int map_alg; 
{
  ACT_ITE_COST_STRUCT *cost_struct1, *cost_struct2;
  ACT_MATCH *match;

  cost_struct1 = ACT_ITE_SLOT(n1);
  if (cost_struct1) {
      ite_my_free(cost_struct1->ite);
      my_free_act(cost_struct1->act);
      act_free_match(cost_struct1->match);
      FREE(cost_struct1);
  }
  ACT_DEF_ITE_SLOT(n1) = ACT_DEF_ITE_SLOT(n2);
  cost_struct2 = ACT_ITE_SLOT(n2);
  if (cost_struct2 != NIL (ACT_ITE_COST_STRUCT)) {
      if (cost_struct2->match) {
          act_free_match(cost_struct2->match);
          match = act_is_act_function(n1, map_alg, act_is_or_used);
          assert(match != NIL (ACT_MATCH));
          cost_struct2->match = match;
      }
  }
  ACT_DEF_ITE_SLOT(n2) = NIL (char);
}

ite_my_free(ite)
  ite_vertex *ite;
{
  st_table *table;
  char *key;
  st_generator *stgen;
  ite_vertex *vertex;

  if (ite == NIL (ite_vertex)) return;
  table = ite_my_traverse_ite(ite);
  st_foreach_item(table, stgen, &key, (char **) &vertex) {
      ite_my_free_vertex(vertex);
  }
  st_free_table(table);
  ite = NIL (ite_vertex);
}

ite_my_free_vertex(vertex)
  ite_vertex *vertex;
{
  if (vertex->fo != NULL) {
      if (vertex->fo->next != NULL) {
          (void) printf("the next is not  NIL\n");
      }
      FREE(vertex->fo);
  }
  FREE(vertex);
}

/*----------------------------------------------------------------------
  Frees the ite and match fields of the undef1 field of all the 
  internal nodes.
-----------------------------------------------------------------------*/
act_free_ite_network(network)
  network_t *network;
{
  lsGen gen;
  node_t *node;

  foreach_node(network, gen, node) {
      act_free_ite_node(node);
  }
}

act_free_ite_node(node)
  node_t *node;
{
  /*
  if (node->type != PRIMARY_INPUT || node->type != PRIMARY_OUTPUT) return;
  */
  if (ACT_ITE_SLOT(node)) {
      network_free(ACT_ITE_SLOT(node)->network);
      ACT_ITE_SLOT(node)->network = NIL (network_t);
      
      act_free_match(ACT_ITE_SLOT(node)->match);
      ACT_ITE_SLOT(node)->match = NIL (ACT_MATCH);
      
      ite_my_free(ACT_ITE_ite(node));
      ACT_ITE_ite(node) = NIL (ite_vertex);
      
      my_free_act(ACT_ITE_act(node));
      ACT_ITE_act(node) = NIL (ACT_VERTEX);
  }
}


act_ite_remap_update_ite_fields(table1, network1, node)
  st_table *table1;
  network_t *network1;
  node_t *node;
{
  ACT_ITE_COST_STRUCT *cost_struct;
  
  cost_struct = ACT_ITE_SLOT(node);
  cost_struct->node = node;
  if (cost_struct->match) return;
  if (cost_struct->ite) {
      cost_struct->ite->node = node;  
      act_ite_remap_put_node_names_in_ite(cost_struct->ite, table1, network1);
      assert(cost_struct->act == NIL (ACT_VERTEX));
  } else {
      cost_struct->act->node = node;  
      act_remap_put_node_names_in_act(cost_struct->act, table1, network1);
  }      
}

act_ite_remap_put_node_names_in_ite(ite, table1, network1)
  ite_vertex *ite;
  st_table *table1;
  network_t *network1;
{
  st_table *table;
  char *key;
  st_generator *stgen;
  ite_vertex *vertex;
  node_t *node1, *node;

  table = ite_my_traverse_ite(ite);
  st_foreach_item(table, stgen, &key, (char **) &vertex) {
      if (vertex->value != 2) continue;
      node1 = network_find_node(network1, node_long_name(vertex->fanin));
      assert(st_lookup(table1, (char *) node1, (char **) &node));
      vertex->name = node_long_name(node);
      vertex->fanin = node;
  }
  st_free_table(table);
}

my_traverse_ite(vertex, table)
  ite_vertex *vertex;
  st_table *table;
{
  if (vertex->value != 3) return;

  if (!st_is_member(table, (char *) vertex->IF)) {
      assert(!st_insert(table, (char *) vertex->IF, (char *) vertex->IF));
      my_traverse_ite(vertex->IF, table);
  }
  if (!st_is_member(table, (char *) vertex->THEN)) {
      assert(!st_insert(table, (char *) vertex->THEN, (char *) vertex->THEN));
      my_traverse_ite(vertex->THEN, table);
  }
  if (!st_is_member(table, (char *) vertex->ELSE)) {
      assert(!st_insert(table, (char *) vertex->ELSE, (char *) vertex->ELSE));
      my_traverse_ite(vertex->ELSE, table);
  }
}

/*-----------------------------------------------------------------
  Puts all the ite vertices in the tree rooted at ite in the table.
------------------------------------------------------------------*/
st_table *
ite_my_traverse_ite(ite)
ite_vertex *ite;
{
  st_table *table;
  table = st_init_table(st_ptrcmp, st_ptrhash);
  assert(!st_insert(table, (char *) ite, (char *) ite));
  my_traverse_ite(ite, table);
  return table;
}

/*-------------------------------------------------------------------
  Given an ite vertex that has value 2, returns the literal of 
  node of the network that corresponds to it. Gets the node by 
  vertex->fanin.
---------------------------------------------------------------------*/
node_t *
ite_get_node_literal_of_vertex(vertex)
  ite_vertex *vertex;
{
  node_t *node;

  assert(vertex->value == 2);
  node = vertex->fanin;
  return (node_literal(node, 1));
}

/*-----------------------------------------------------------
  Also frees the node associated with the non-terminal
  multiple fanout vertices.
-------------------------------------------------------------*/
ite_free_nodes_in_multiple_fo_ite(vertex)
  ite_vertex_ptr vertex;
{
  if (vertex->mark == MARK_COMPLEMENT_VALUE) return;
  vertex->mark = MARK_COMPLEMENT_VALUE;
  if (vertex->value <= 1) return;
  if (vertex->multiple_fo != 0) {
	node_free(vertex->node);
  }
  if (vertex->value == 3) {
      ite_free_nodes_in_multiple_fo_ite(vertex->IF);
      ite_free_nodes_in_multiple_fo_ite(vertex->THEN);
      ite_free_nodes_in_multiple_fo_ite(vertex->ELSE);
  }
}

/*-----------------------------------------------------------
  Frees the node associated with the non-terminal
  multiple fanout vertices.
-------------------------------------------------------------*/
act_free_nodes_in_multiple_fo_act(vertex)
    ACT_VERTEX_PTR vertex;
{
  if (vertex->mark == MARK_COMPLEMENT_VALUE) return;
  vertex->mark = MARK_COMPLEMENT_VALUE;
  if (vertex->value <= 1) return;
  if (vertex->multiple_fo != 0) {
	node_free(vertex->node);
  }
  act_free_nodes_in_multiple_fo_act(vertex->low);
  act_free_nodes_in_multiple_fo_act(vertex->high);
}


