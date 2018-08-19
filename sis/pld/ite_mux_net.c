#include "sis.h"
#include "pld_int.h" 
#include "ite_int.h"

act_ite_mux_network(network, init_param, FAC_TO_SOP_RATIO)
  network_t *network;
  act_init_param_t *init_param;
  float FAC_TO_SOP_RATIO;
{
  array_t *nodevec;
  int i, num_lit;
  node_t *node;
  node_function_t node_fun;

  nodevec = network_dfs(network);
  
  for (i = 0; i < array_n(nodevec); i++) {
      node = array_fetch(node_t *, nodevec, i);
      if (node->type != INTERNAL) continue;
      num_lit = node_num_literal(node);
      if ((num_lit > 4) && (factor_num_literal(node) < FAC_TO_SOP_RATIO * num_lit)) {
          ACT_ITE_ite(node) = act_ite_create_from_factored_form(node, init_param);
      } else {
          act_ite_intermediate_new_make_ite(node, init_param);
      }
      act_ite_initialize_ite_area_pattern0(ACT_ITE_ite(node));
      /* next line checks if it is a buffer node, 0, 1 node, do nothing - Sept 10, 93 */
      /*------------------------------------------------------------------------------*/
      node_fun = node_function(node);
      if (node_fun != NODE_0 && node_fun != NODE_1 && node_fun != NODE_BUF) {
          ACT_ITE_SLOT(node)->cost = 2; /* to fool act_ite_break_node */
          act_ite_break_node(node, init_param);
      }
  }
  array_free(nodevec);
  act_free_ite_network(network);
}

/*-----------------------------------------------------------------
  Initialize each vertex of the ite so that its mapped field is 1
  and pattern_num = 0 (just a mux.). Set the multiple_fo vertex
  correctly because it will be needed while breaking the node.
------------------------------------------------------------------*/
act_ite_initialize_ite_area_pattern0(ite)
  ite_vertex *ite;
{
  st_table *table;

  table = st_init_table(st_ptrcmp, st_ptrhash);
  act_ite_insert_table_area_pattern0(ite, table);
  st_free_table(table);
}

act_ite_insert_table_area_pattern0(ite, table)
  ite_vertex *ite;
  st_table *table;
{
  if (st_insert(table, (char *) ite, (char *) ite)) {
      (ite->multiple_fo)++;
      return;
  }

  ite->pattern_num = 0;
  ite->cost = 0;
  ite->mark = 0;
  ite->mapped = 1;
  ite->arrival_time = (double) 0.0;
  ite->multiple_fo = 0; 
  ite->multiple_fo_for_mapping = 0; 

  if (ite->value < 3) return;
  act_ite_insert_table_area_pattern0(ite->IF, table);  
  act_ite_insert_table_area_pattern0(ite->THEN, table);  
  act_ite_insert_table_area_pattern0(ite->ELSE, table);  
}
  
