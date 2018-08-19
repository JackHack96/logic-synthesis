#include "sis.h"
#include "pld_int.h"
#include "ite_int.h"

/*--------------------------------------------------------------------------
  This is another mapping method which takes a node, makes a network out
  of it and then uses MAP_METHOD = NEW to map the network, uses iterative
  improvement on this network to get a good representation. If 
  init_param->MAP_METHOD requires just decomp, then FANIN_COLLAPSE field 
  is set to 0. Else it is left untouched.
---------------------------------------------------------------------------*/
act_ite_map_node_with_iter_imp(node, init_param)
  node_t *node;
  act_init_param_t *init_param;
{
  node_function_t node_fun;
  network_t *network1;
  /* network_t *new_network; */ /* commented because using new bdd package - not using */
  int save_iter, save_break, save_last_gasp;
  /* int act_bdd_new_sav; */
  /* int new_cost */ /* commented because using new bdd package - not using */
  int save_map_method, save_fanin_collapse;

  node_fun = node_function(node);
  if (node_fun == NODE_PI || node_fun == NODE_PO) return 0;
  if (node_fun == NODE_0 || node_fun == NODE_1 || node_fun == NODE_BUF) {
      ACT_ITE_SLOT(node)->network = NIL (network_t);
      ACT_ITE_SLOT(node)->cost = 0;
  } else {
      network1 = network_create_from_node(node);

      save_iter = init_param->NUM_ITER;
      save_break = init_param->BREAK;
      save_last_gasp = init_param->LAST_GASP;
      save_map_method = init_param->MAP_METHOD;
      save_fanin_collapse = init_param->FANIN_COLLAPSE;

      if (init_param->MAP_METHOD == MAP_WITH_JUST_DECOMP) {
          init_param->FANIN_COLLAPSE = 0;
      }
      init_param->NUM_ITER = 1;
      /* change init_param->MAP_METHOD to NEW */
      init_param->MAP_METHOD = NEW;
      init_param->BREAK = 1;
      init_param->LAST_GASP = 0;
      /* 
      act_bdd_new_sav = ACT_BDD_NEW;
      ACT_BDD_NEW = 0;
      */

      act_ite_map_network_with_iter(network1, init_param); /* with at most one iteration */
      act_free_ite_network(network1); /* frees the ite, act and match */
      ACT_ITE_SLOT(node)->cost = network_num_nonzero_cost_nodes(network1);

      /* this takes care of single literal cubes, single cubes and optimum mapping */
      /*---------------------------------------------------------------------------*/
      if (ACT_ITE_SLOT(node)->cost > 2 && (node_fun != NODE_AND) && (node_fun != NODE_OR) && 
          save_map_method == MAP_WITH_ITER) {
          act_ite_map_network_with_iter(network1, init_param); 
          act_free_ite_network(network1); /* frees the ite, act and match */
          ACT_ITE_SLOT(node)->cost = network_num_nonzero_cost_nodes(network1);
      }
      ACT_ITE_SLOT(node)->network = network1;

      /* commented Mar 3 94 for official release - Long bdd package!*/
      /*
      if (ACT_ITE_SLOT(node)->cost > 3 && act_bdd_new_sav && (node_fun != NODE_AND) && (node_fun != NODE_OR)) {
          new_network = act_map_using_new_bdd(network1, init_param);
          act_free_ite_network(new_network);
          new_cost = network_num_nonzero_cost_nodes(new_network);
          if (new_cost < ACT_ITE_SLOT(node)->cost) {
              network_free(ACT_ITE_SLOT(node)->network);
              ACT_ITE_SLOT(node)->network = new_network;
              ACT_ITE_SLOT(node)->cost = new_cost;
          } else {
              network_free(new_network);
          }
      }
      */
      init_param->FANIN_COLLAPSE = save_fanin_collapse;
      init_param->MAP_METHOD = save_map_method;
      init_param->NUM_ITER = save_iter;
      init_param->LAST_GASP = save_last_gasp;
      init_param->BREAK = save_break;
      /* ACT_BDD_NEW = act_bdd_new_sav; */

  }
  return ACT_ITE_SLOT(node)->cost;
}

int 
network_num_nonzero_cost_nodes(network)
  network_t *network;
{
  int count;
  node_t *node;
  node_function_t node_fun;
  lsGen gen;

  count = 0;
  foreach_node(network, gen, node) {
      if (node->type != INTERNAL) continue;
      node_fun = node_function(node);
      if (node_fun == NODE_0 || node_fun == NODE_1 || node_fun == NODE_BUF) continue;
      count++;
  }
  return count;
}


