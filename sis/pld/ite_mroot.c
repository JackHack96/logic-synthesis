#include "sis.h"
#include "pld_int.h" 
#include "ite_int.h"

extern ite_vertex *PRESENT_ITE;

/* 
*   This file contains routines to construct a multiple-root ite for the 
*   entire network. Then it is mapped. First we 
*   use the existing code to construct the ite for each node, then change the 
*   information in the ite: like appropriately changing the pointers. Then a
*   mapping is performed. 
*/

act_ite_create_and_map_mroot_network(network, init_param)
  network_t *network;
  act_init_param_t *init_param;
{
  act_ite_create_mroot_ite_network(network, init_param);
  (void) act_ite_mroot_make_tree_and_map(network);
  /* if (init_param->BREAK) act_ite_mroot_break_network(network); */
  act_ite_mroot_traverse_and_free(network);
}

act_ite_create_mroot_ite_network(network, init_param)
  network_t *network;
  act_init_param_t *init_param;
{

  node_t *node;
  array_t *nodevec;
  int i;

  nodevec = network_dfs(network);
  for (i = 0; i < array_n(nodevec); i++) {
      node = array_fetch(node_t *, nodevec, i);
      act_ite_intermediate_new_make_ite(node, init_param); /* to skirt the problem of ite_1, ite_0 */
      /* change node, value fields in the ites which are either buffers or inverters */
      /*-----------------------------------------------------------------------------*/
      act_ite_mroot_modify_fields_node(node);
  }
  array_free(nodevec);
}


act_ite_mroot_modify_fields_node(node)
  node_t *node;
{
  ite_vertex *ite, *vertex;
  st_table *table, *table_free;
  st_generator *stgen;
  char *key, *value;
  node_t *fanin;

  if (node->type == PRIMARY_INPUT || node->type == PRIMARY_OUTPUT) return;

  ite = ACT_ITE_ite(node);

  /* ite is buffer, replace it by the fanin ite, free the old one 
     assuming here that vertex 1 and 0 in the ite are not shared  */
  /*--------------------------------------------------------------*/
  if (ite_is_buffer(ite)) {
      fanin = ite->IF->fanin;
      if (fanin->type != PRIMARY_INPUT) {          
          ACT_ITE_ite(node) = ACT_ITE_ite(fanin);
          ite_my_free(ite);
      }
  } else {
      table = st_init_table(st_ptrcmp, st_ptrhash); /* for traversing ite */
      table_free = st_init_table(st_ptrcmp, st_ptrhash); /* for storing the potential vertices
                                                            to be freed */
      act_ite_mroot_modify_fields_ite(ite, table, table_free);
      /* free only those vertices of table_free 
         that are not found in the table      */
      /*--------------------------------------*/
      st_foreach_item(table_free, stgen, &key, &value) {
          if (st_lookup(table, key, &value)) continue;
          vertex = (ite_vertex *) key;
          ite_my_free_vertex(vertex);
      }
      st_free_table(table);
      st_free_table(table_free);
  }
}

/*-------------------------------------------------------------------
 Given an ite, checks if there is a child which has value 2: then 
 replaces it by ite of the appropriate fanin node.

 Notes: 1) Most likely, will not work for canonical ite because I am 
        not taking care of cases like phase = 0.
        2). Also may not work with the ite constructed with -m 0. 
        Try it and see. May work too because changing the fields 
        after constructing the ite.
        3) Assuming that ite_1, ite_0 (may be multiple for one node)
        not being pointed at by any other node.
--------------------------------------------------------------------*/
act_ite_mroot_modify_fields_ite(ite, table, table_free)
  ite_vertex *ite;
  st_table *table, *table_free;
{
  ite_vertex *ite_if, *ite_then, *ite_else;
  node_t *fanin;
  
  if (st_insert(table, (char *) ite, (char *) ite)) return;
  if (ite->value <= 1) return;
  assert(ite->value == 3);

  ite_if = ite->IF;
  if (ite_if->value == 2) {
      assert(ite_if->phase == 1);
      fanin = ite_if->fanin;
      if (fanin->type != PRIMARY_INPUT) {          
          ite->IF = ACT_ITE_ite(fanin);
          (void) st_insert(table_free, (char *) ite_if, (char *) ite_if);
      } else {
          (void) st_insert(table, (char *) ite_if, (char *) ite_if);
      }
  } else {
      if (ite_is_buffer(ite_if)) {
          act_ite_mroot_modify_part(ite, ite_if, 'I', table, table_free);
      } else {
          act_ite_mroot_modify_fields_ite(ite_if, table, table_free);
      }
  }

  ite_then = ite->THEN;
  if (ite_is_buffer(ite_then)) {
      act_ite_mroot_modify_part(ite, ite_then, 'T', table, table_free);
  } else {
      act_ite_mroot_modify_fields_ite(ite_then, table, table_free);
  }

  ite_else = ite->ELSE;
  if (ite_is_buffer(ite_else)) {
      act_ite_mroot_modify_part(ite, ite_else, 'E', table, table_free);
  } else {
      act_ite_mroot_modify_fields_ite(ite_else, table, table_free);
  }
}

int 
ite_is_buffer(ite)
  ite_vertex *ite;
{
  return (ite->value == 3 && ite->IF->value == 2 && ite->IF->phase == 1 && ite->THEN->value == 1 
      && ite->ELSE->value == 0);
}

/*-----------------------------------------------------------------------
  Parent's "c" (could be 'I', 'T', 'E') child is a buffer. 
------------------------------------------------------------------------*/
act_ite_mroot_modify_part(parent, child, c, table, table_free)
  ite_vertex *parent, *child;
  char c;
  st_table *table, *table_free;
{
  node_t *fanin;

  fanin = child->IF->fanin;
  if (fanin->type != PRIMARY_INPUT) {
      (void) st_insert(table_free, (char *) child->THEN, (char *) child->THEN);
      (void) st_insert(table_free, (char *) child->ELSE, (char *) child->ELSE);
      (void) st_insert(table_free, (char *) child->IF, (char *) child->IF);
      (void) st_insert(table_free, (char *) child, (char *) child);
      switch(c) {
        case 'I': 
          parent->IF = ACT_ITE_ite(fanin);
          break;
        case 'T': 
          parent->THEN = ACT_ITE_ite(fanin);
          break;
        case 'E': 
          parent->ELSE = ACT_ITE_ite(fanin);
          break;
      }
      assert(ACT_ITE_ite(fanin));
  } else {
      (void) st_insert(table, (char *) child, (char *) child);
      (void) st_insert(table, (char *) child->IF, (char *) child->IF);
      (void) st_insert(table, (char *) child->THEN, (char *) child->THEN);
      (void) st_insert(table, (char *) child->ELSE, (char *) child->ELSE);
  }      
}


act_ite_create_mroot_ite_node(node, init_param)
  node_t *node;
  act_init_param_t *init_param;
{

  if (node->type == PRIMARY_INPUT || node->type == PRIMARY_OUTPUT) return;
  /*
  if (factor_num_literal(node) > FAC_TO_SOP_RATIO_BOUND * node_num_literal(node)) {
      act_ite_new_make_ite(node, init_param);
  } else {
      ACT_ITE_ite(node) = act_ite_create_from_factored_form(node, init_param);
  }
  */ /* this is for the time being */
  act_ite_new_make_ite(node, init_param);
}




/*-------------------------------------------------------------------
  Assumes that ite of the node has been created. Breaks the ite into
  trees and maps each tree using the pattern graphs. 
--------------------------------------------------------------------*/
int
act_ite_mroot_make_tree_and_map(network)
  network_t *network;

{
  int           num_ite;
  int     	i;
  int 		num_mux_struct;
  int 		TOTAL_MUX_STRUCT; /* cost of the node */
  ite_vertex_ptr ite;
  array_t *multiple_fo_array;
  node_t *po, *po_fanin;
  lsGen gen;

  TOTAL_MUX_STRUCT = 0;
  multiple_fo_array = array_alloc(ite_vertex_ptr, 0);
  foreach_primary_output(network, gen, po) {
      po_fanin = node_get_fanin(po, 0);
      if (po_fanin->type != PRIMARY_INPUT) {
          array_insert_last(ite_vertex_ptr, multiple_fo_array, ACT_ITE_ite(po_fanin));
      }
  }

  act_ite_mroot_initialize_ite_area_network(multiple_fo_array); 

  /*
  if (ACT_ITE_DEBUG > 3) {
     ite_print_dag(ite_of_node);
     print_multiple_fo_array(multiple_fo_array); 
  }
  */

  /* traverse the ite of node bottom up and map each tree ite */
  /*----------------------------------------------------------*/
  num_ite = array_n(multiple_fo_array);
  for (i = num_ite - 1; i >= 0; i--) {
      ite = array_fetch(ite_vertex_ptr, multiple_fo_array, i);
      PRESENT_ITE = ite;
      num_mux_struct = act_map_ite(ite);
      TOTAL_MUX_STRUCT += num_mux_struct;
  }
  (void) printf("number of total mux_structures used for the network  = %d\n",
                TOTAL_MUX_STRUCT);
  array_free(multiple_fo_array);
  /* ite_clear_dag(ite_of_node); */ /* to make all mark fields 0 */
  /*
  if (ACT_ITE_DEBUG > 1) {
      node_print(stdout, node);
      ite_print_dag(ite_of_node);
  }
  */
  return TOTAL_MUX_STRUCT;
}
  

act_ite_mroot_initialize_ite_area_network(multiple_fo_array)
  array_t *multiple_fo_array;
{
  int init_num, i;
  st_table *table;
  ite_vertex *ite;

  init_num = array_n(multiple_fo_array);
  table = st_init_table(st_ptrcmp, st_ptrhash);
  for (i = 0; i < init_num; i++) {
      ite = array_fetch(ite_vertex *, multiple_fo_array, i);
      (void) st_insert(table, (char *) ite, (char *) ite);
      ite->multiple_fo = 1;  /* to fool the mapper */
      ite->pattern_num = -1;
      ite->cost = 0;
      ite->arrival_time = (double) 0.0;
      ite->mapped = 0;
      ite->multiple_fo_for_mapping = 0; 
      ite->mark = 0;
  }
  for (i = 0; i < init_num; i++) {
      ite = array_fetch(ite_vertex *, multiple_fo_array, i);
      if (ite->value == 3) {
          act_ite_mroot_initialize_area_ite(ite->IF, multiple_fo_array, table);
          act_ite_mroot_initialize_area_ite(ite->THEN, multiple_fo_array, table);
          act_ite_mroot_initialize_area_ite(ite->ELSE, multiple_fo_array, table);
      }
  }
  st_free_table(table);
}

act_ite_mroot_initialize_area_ite(vertex, multiple_fo_array, table)
  ite_vertex *vertex;
  array_t *multiple_fo_array;
  st_table *table;
{
  char *value;

  if (st_lookup(table, (char *) vertex, (char **) &value)) {
      if (vertex->multiple_fo == 0) {
          array_insert_last(ite_vertex *, multiple_fo_array, vertex);
      }
      vertex->multiple_fo += 1;
      vertex->multiple_fo_for_mapping += 1;
      return;
  }
  assert(!st_insert(table, (char *) vertex, (char *) vertex));
  vertex->pattern_num = -1;
  vertex->cost = 0;
  vertex->arrival_time = (double) 0.0;
  vertex->mapped = 0;
  vertex->multiple_fo = 0;
  vertex->multiple_fo_for_mapping = 0; 
  vertex->mark = 0;
  
  if (vertex->value != 3) return;
  act_ite_mroot_initialize_area_ite(vertex->IF, multiple_fo_array, table);
  act_ite_mroot_initialize_area_ite(vertex->THEN, multiple_fo_array, table);
  act_ite_mroot_initialize_area_ite(vertex->ELSE, multiple_fo_array, table);
  
}
      
act_ite_mroot_traverse_and_free(network)
  network_t *network;
{
  lsGen gen;
  node_t *po, *po_fanin;
  char *key;
  st_generator *stgen;
  ite_vertex *vertex;
  st_table *table;

  table = st_init_table(st_ptrcmp, st_ptrhash);
  foreach_primary_output(network, gen, po) {
      po_fanin = node_get_fanin(po, 0);
      if (po_fanin->type == PRIMARY_INPUT) continue;
      vertex = ACT_ITE_ite(po_fanin);
      if (!st_is_member(table, (char *) vertex)) {
          assert(!st_insert(table, (char *) vertex, (char *) vertex));
          ite_mroot_traverse_ite(vertex, table);
      }
  }
  st_foreach_item(table, stgen, &key, (char **) &vertex) {
      ite_my_free_vertex(vertex);
  }
  st_free_table(table);
}

ite_mroot_traverse_ite(vertex, table)
  ite_vertex *vertex;
  st_table *table;
{
  if (vertex->value != 3) return;

  if (!st_is_member(table, (char *) vertex->IF)) {
      assert(!st_insert(table, (char *) vertex->IF, (char *) vertex->IF));
      ite_mroot_traverse_ite(vertex->IF, table);
  }
  if (!st_is_member(table, (char *) vertex->THEN)) {
      assert(!st_insert(table, (char *) vertex->THEN, (char *) vertex->THEN));
      ite_mroot_traverse_ite(vertex->THEN, table);
  }
  if (!st_is_member(table, (char *) vertex->ELSE)) {
      assert(!st_insert(table, (char *) vertex->ELSE, (char *) vertex->ELSE));
      ite_mroot_traverse_ite(vertex->ELSE, table);
  }
}

/*
act_ite_mroot_break_network(network)
  network_t *network;
{
  foreach_primary_output(network, gen, po) {
      po_fanin = node_get_fanin(po, 0);
      act_ite_mroot_break_node(po, 
}
*/
