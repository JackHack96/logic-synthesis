#include "sis.h"
#include "pld_int.h" 
#include "ite_int.h"
#include "math.h"

extern st_table *ite_end_table;
static ite_vertex *ite_1, *ite_0;
static sm_matrix *build_F();
static ite_vertex* urp_F();
static int good_bin_var();
static int find_var();
static void update_F();
extern my_sm_copy_row(), my_sm_print();

int
act_ite_new_map_node(node, init_param)
  node_t *node;
  act_init_param_t *init_param;
{
  ACT_ITE_COST_STRUCT *cost_node;

  if ((node->type == PRIMARY_INPUT) || (node->type == PRIMARY_OUTPUT)) return 0;
  act_ite_intermediate_new_make_ite(node, init_param);
  cost_node = ACT_ITE_SLOT(node);
  cost_node->cost = act_ite_make_tree_and_map(node);
  cost_node->arrival_time = ACT_ITE_ite(node)->arrival_time;
  cost_node->node = node;
  return cost_node->cost;
}

act_ite_intermediate_new_make_ite(node, init_param)
  node_t *node;
  act_init_param_t *init_param;
{
  ite_vertex *ite;
  node_function_t node_fun;

  if ((node->type == PRIMARY_INPUT) || (node->type == PRIMARY_OUTPUT)) return;
  node_fun = node_function(node);
  switch(node_fun) {
    case NODE_0: 
      ite = ite_alloc();
      ite->value = 0;
      ACT_ITE_ite(node) = ite;
      break;
    case NODE_1:
      ite = ite_alloc();
      ite->value = 1;
      ACT_ITE_ite(node) = ite;
      break;
    default: 
      ite_end_table = st_init_table(st_ptrcmp, st_ptrhash);
      ite_0 = ite_alloc();
      ite_0->value = 0;
      (void) st_insert(ite_end_table, (char *)0, (char *)ite_0);
      ite_1 = ite_alloc();
      ite_1->value = 1;
      (void) st_insert(ite_end_table, (char *)1, (char *)ite_1);
      act_ite_new_make_ite(node, init_param);
      st_free_table(ite_end_table);
      break;
  }
}

/* We need this to initialise the 0 and 1 ite's and store them 
 * in a table so they can be easily accessed
   Note: node must be in the network (the only use of network is to get the fanin nodes
   from the fanin names: so the restriction can be removed in future, if needed)*/ 

act_ite_new_make_ite(node, init_param)
  node_t *node;
  act_init_param_t *init_param;
{
  ite_vertex *ite_lit, *ite_if, *ite_else, *ite_else_if, *ite_else_then, *ite_else_else,
  *ite_a, *ite_b, *ite_1_sav, *ite_0_sav, *ite_unate;
  node_function_t node_fun, node_fun_if;
  node_t *node_if, *node_else_then, *node_else_else, *fanin_a, *fanin_b, *variable, *fanin;
  int num_cube, num_lit, num_cube_if, flag;
  ACT_ITE_COST_STRUCT *cost_node;
  ACT_MATCH *match;
  st_table *ite_end_table_sav;

  /* handle trivial cases */
  /*----------------------*/
  node_fun = node_function(node);
  switch(node_fun) {
    case NODE_PI:;
    case NODE_PO: 
      return;
    case NODE_0: 
      ACT_ITE_ite(node) = ite_0;
      return;
    case NODE_1:
      ACT_ITE_ite(node) = ite_1;
      return;
    case NODE_BUF:
      fanin = node_get_fanin(node, 0);
      ite_lit = ite_new_literal(fanin);
      ACT_ITE_ite(node) = my_shannon_ite(ite_lit, ite_1, ite_0);
      return;
  }
        
  /* check if node is a single cube */
  /*--------------------------------*/
  num_cube = node_num_cube(node);
  if (num_cube == 1) {
      ACT_ITE_ite(node) = ite_new_ite_for_cubenode(node);
      return;
  }
  
  /* check if node is composed of single-literal cubes */
  /*---------------------------------------------------*/
  num_lit = node_num_literal(node);
  if (num_lit == num_cube) {
      ACT_ITE_ite(node) = ite_new_ite_for_single_literal_cubes(node);
      return;
  }
  
  /* check if the node is realizable by one block */
  /*----------------------------------------------*/
  cost_node = ACT_ITE_SLOT(node);
  match = act_is_act_function(node, init_param->map_alg, act_is_or_used);
  if (match) {
      /* cost_node->match = match; */
      ACT_ITE_ite(node) = ite_convert_match_to_ite(node, match);
      act_free_match(cost_node->match);
      assert(!(cost_node->match)); /* just a check */
      return;
  }

  /* if the node is a function of cubes of disjoint support */
  /*--------------------------------------------------------*/
  if (num_lit == node_num_fanin(node)) { 
      ACT_ITE_ite(node) = ite_create_ite_for_orthogonal_cubes(node);
      return;
  }

  if (UNATE_SELECT) {
      if (node_is_unate(node)) {
          ACT_ITE_ite(node) = ite_new_ite_for_unate_cover(node, init_param);
          return;
      }
  }

  switch (init_param->VAR_SELECTION_LIT) {
    case 0:
      variable = node_most_binate_variable(node); 
      break;
    case (-1):
      /* first save these global variables since the next routine is
         going to change them                                         */
      /*--------------------------------------------------------------*/
      ite_1_sav = ite_1;
      ite_0_sav = ite_0;
      ite_end_table_sav = ite_end_table;

      variable = ite_get_minimum_cost_variable(node, init_param);

      ite_1 = ite_1_sav;
      ite_0 = ite_0_sav;
      ite_end_table = ite_end_table_sav;
      break;
    default:
      ite_1_sav = ite_1;
      ite_0_sav = ite_0;
      ite_end_table_sav = ite_end_table;

      /* this may call indirectly act_ite_intermediate_new_make_ite() */
      variable = node_most_binate_variable_new(node, init_param, &ite_unate); 
                                                                              

      ite_1 = ite_1_sav;
      ite_0 = ite_0_sav;
      ite_end_table = ite_end_table_sav;
      break;
  }
  if (init_param->VAR_SELECTION_LIT > 0 && !variable) {
      assert(ite_unate);
      ACT_ITE_ite(node) = ite_unate;
      return;
  }
  assert(variable);
  node_algebraic_cofactor(node, variable, &node_else_then, &node_else_else, &node_if);

  /* at least one of node_else_then and node_else_then is not a constant, since
     we checked if num_occur > 1 in this case                                   */
  /*----------------------------------------------------------------------------*/
  act_ite_new_make_ite(node_else_then, init_param);
  act_ite_new_make_ite(node_else_else, init_param);  
  ite_else_if = ite_new_literal(variable);
  ite_else_then = ACT_ITE_ite(node_else_then);
  ite_else_else = ACT_ITE_ite(node_else_else);
  ite_else = my_shannon_ite(ite_else_if, ite_else_then, ite_else_else);
  
  /* check if node_if satisfies some simple conditions, under which OR gate
     may not be used or may be used effectively                            */
  /*-----------------------------------------------------------------------*/
  node_fun_if = node_function(node_if);
  switch(node_fun_if) {
    case NODE_0: 
      ACT_ITE_ite(node) = ite_else;
      break;
    case NODE_BUF:
      fanin = node_get_fanin(node_if, 0);
      ite_if = ite_new_literal(fanin);
      ACT_ITE_ite(node) = my_shannon_ite(ite_if, ite_1, ite_else);
      break;
    case NODE_INV:
      fanin = node_get_fanin(node_if, 0);
      ite_if = ite_new_literal(fanin);
      ACT_ITE_ite(node) = my_shannon_ite(ite_if, ite_else, ite_1);
      break;
    default:
      /* node_if is of the form a' b' */
      /*------------------------------*/
      flag = 0;
      num_cube_if = node_num_cube(node_if);
      if (num_cube_if == 1 && node_num_fanin(node_if) == 2) {
          fanin_a = node_get_fanin(node_if, 0);
          if (node_input_phase(node_if, fanin_a) == NEG_UNATE) {
              fanin_b = node_get_fanin(node_if, 1);
              if (node_input_phase(node_if, fanin_b) == NEG_UNATE) {
                  ite_a = ite_buffer(fanin_a);
                  ite_b = ite_buffer(fanin_b);
                  ite_if = ite_new_OR(ite_a, ite_b);
                  ACT_ITE_ite(node) = my_shannon_ite(ite_if, ite_else, ite_1);
                  flag = 1;
              }
          }
      }
      if (!flag) {
          act_ite_new_make_ite(node_if, init_param);  
          ite_if = ACT_ITE_ite(node_if);
          ACT_ITE_ite(node) = my_shannon_ite(ite_if, ite_1, ite_else);
      }
  }
  node_free(node_if);
  node_free(node_else_then);
  node_free(node_else_else);
}

/*-----------------------------------------------------------
  Return the fanin of the node that occurs most often in the
  SOP. Priority is given to a fanin that occurs in all the
  cubes in the same phase. Then, the ties are broken by 
  selecting the fanin that has minimum difference between the 
  positive and the negative occurrences. In pnum_occur, return 
  the total number of occurrences of the variable being 
  returned. 
-------------------------------------------------------------*/
node_t *
node_most_binate_variable(node)
  node_t *node;
{

  int *lit_count_array, j, j2, j2p1, num_cube, max_count, diff_count, index, j_count, j_diff_count;
  node_t *variable;

  if (node_num_fanin(node) == 0) return NIL(node_t);

  num_cube = node_num_cube(node);
  lit_count_array = node_literal_count(node);  
  max_count = -1;
  diff_count = -1;
  index = -1;
  for (j = 0; j < node_num_fanin(node); j++) {
      j2 = 2 * j;
      j2p1 = j2 + 1;
      j_count = lit_count_array[j2] + lit_count_array[j2p1];
      j_diff_count = abs(lit_count_array[j2] - lit_count_array[j2p1]);
      if (j_count > max_count) {
          index = j;
          max_count = j_count;
          diff_count = j_diff_count;
          continue;
      } 
      if (j_count == max_count) {
          /* if jth variable occurs in all the cubes in the same phase, or
             it occurs nearly the same number of times in the +ve & -ve phases
             (and the best guy so far does not occur in all the cubes in the
             same phase) select it                                            */
          /*------------------------------------------------------------------*/
          if ((j_diff_count == num_cube) ||
              ((diff_count != num_cube) && (j_diff_count < diff_count))) {
              diff_count = j_diff_count;
              index = j;
              max_count = j_count;
          }
      }
  }
  FREE(lit_count_array);
  variable = node_get_fanin(node, index);
  return variable;
}

node_t *
node_most_binate_variable_new(node, init_param, pite)
  node_t *node;
  act_init_param_t *init_param;
  ite_vertex **pite;
{

  int *lit_count_array, j, j2, j2p1, num_cube, max_count, wt, max_weight;
  int unate, diff_count, index, j_count, j_diff_count;
  node_t *variable, *fanin;

  if (node_num_fanin(node) == 0) return NIL(node_t);

  num_cube = node_num_cube(node);
  lit_count_array = node_literal_count(node);  

  /* check if the SOP is unate */
  /*---------------------------*/
  unate = 1;
  j2 = -2; 
  for (j = 0; j < node_num_fanin(node); j++) {
      j2 += 2;
      if (lit_count_array[j2] && lit_count_array[j2 + 1]) {
          unate = 0;
          break;
      }
  }
  if (unate) {
      if (USE_FAC_WHEN_UNATE) {
          *pite = act_ite_create_from_factored_form(node, init_param);
          FREE(lit_count_array);
          return NIL (node_t);
      }
         
      /* select variable that occurs most often, break the
         ties by selecting a negative unate variable      */
      /*--------------------------------------------------*/
      max_count = -1; index = -1; 
      j2 = -2; 
      for (j = 0; j < node_num_fanin(node); j++) {
          j2 += 2;
          j2p1 = j2 + 1;
          j_count = lit_count_array[j2] + lit_count_array[j2p1];
          if ((j_count > max_count) || 
              (j_count == max_count && lit_count_array[j2] == 0) 
              ) {
              max_count = j_count;
              index = j;
          } 
            
      }
      FREE(lit_count_array);
      variable = node_get_fanin(node, index);
      return variable;
  }

  if (node_num_literal(node) >= init_param->VAR_SELECTION_LIT) {
      /* assign weight to each fanin node, 
         select the fanin with the max wt */
      /*----------------------------------*/
      max_weight = -1;
      for (j = 0; j < node_num_fanin(node); j++) {
          fanin = node_get_fanin(node, j);
          wt = ite_assign_var_weight(node, fanin, j, lit_count_array);
          if (wt > max_weight) {
              max_weight = wt;
              variable = fanin;
          }
      } 
      return variable;
  }

  max_count = -1;
  diff_count = -1;
  index = -1;
  j2 = -2; j2p1 = -1;
  for (j = 0; j < node_num_fanin(node); j++) {
      j2 += 2;
      j2p1 += 2;
      j_count = lit_count_array[j2] + lit_count_array[j2p1];
      j_diff_count = abs(lit_count_array[j2] - lit_count_array[j2p1]);
      if (j_count > max_count) {
          index = j;
          max_count = j_count;
          diff_count = j_diff_count;
          continue;
      } 
      if (j_count == max_count) {
          /* if jth variable occurs in all the cubes in the same phase, or
             it occurs nearly the same number of times in the +ve & -ve phases
             (and the best guy so far does not occur in all the cubes in the
             same phase) select it                                            */
          /*------------------------------------------------------------------*/
          if ((j_diff_count == num_cube) ||
              ((diff_count != num_cube) && (j_diff_count < diff_count))) {
              diff_count = j_diff_count;
              index = j;
              max_count = j_count;
          }
      }
  }
  FREE(lit_count_array);
  variable = node_get_fanin(node, index);
  return variable;
}

/*---------------------------------------------------------------------
  Compute a fast cost estimate of the algebraic cofactors of the node
  function with respect to each fanin and returns the variable that 
  results in minimum cost.
-----------------------------------------------------------------------*/
node_t *
ite_get_minimum_cost_variable(node, init_param)
  node_t *node;
  act_init_param_t *init_param;

{
  int min_cost, j, cost;
  node_t *fanin, *variable;
  int break_sav, num_iter_sav, map_method_sav, var_selection_lit_sav, last_gasp_save;
  /* int bdd_new_sav; */

  /* change some of the init_param fields to make a faster mapping */
  /*---------------------------------------------------------------*/
  break_sav = init_param->BREAK;
  num_iter_sav = init_param->NUM_ITER;
  map_method_sav = init_param->MAP_METHOD;
  var_selection_lit_sav = init_param->VAR_SELECTION_LIT;
  last_gasp_save = init_param->LAST_GASP;
  /* bdd_new_sav = ACT_BDD_NEW; */

  init_param->BREAK = 0;
  init_param->NUM_ITER = 0;
  init_param->MAP_METHOD = NEW;
  init_param->VAR_SELECTION_LIT = 0;
  init_param->LAST_GASP = 0;
  /* ACT_BDD_NEW = 0; */

  min_cost = MAXINT;
  for (j = 0; j < node_num_fanin(node); j++) {
      fanin = node_get_fanin(node, j);
      cost = ite_assign_var_cost(node, fanin, init_param, min_cost);
      if (cost < min_cost) {
          min_cost = cost;
          variable = fanin;
      }
  } 
  /* restore the init_param fields */
  /*-------------------------------*/
  init_param->BREAK = break_sav;
  init_param->NUM_ITER = num_iter_sav;
  init_param->MAP_METHOD = map_method_sav;
  init_param->VAR_SELECTION_LIT = var_selection_lit_sav;
  init_param->LAST_GASP = last_gasp_save;
  /* ACT_BDD_NEW = bdd_new_sav; */

  return variable;
}

/*---------------------------------------------------------------------
   Converting a match to an ITE. Remember that inputs to a match are
   either 0, 1 or node_literal(fanin, 1) where fanin was an input
   of node.
----------------------------------------------------------------------*/
ite_vertex *
ite_convert_match_to_ite(node, match)
  node_t *node;
  ACT_MATCH *match;
{
  
  node_function_t node_fun, node_fun_S1, node_fun_S0;
  ite_vertex *ite, *ite_IF, *ite_THEN, *ite_ELSE, *ite_lit, *ite_IF_1, *ite_IF_2;
  node_t *fanin;

  node_fun = node_function(node);
  switch(node_fun) {
    case NODE_0: 
      return ite_0;
    case NODE_1:
      return ite_1;
    case NODE_BUF:
      fanin = node_get_fanin(node, 0);
      ite_lit = ite_new_literal(fanin);
      ite = my_shannon_ite(ite_lit, ite_1, ite_0);
      return ite;
    case NODE_INV:
      fanin = node_get_fanin(node, 0);
      ite_lit = ite_new_literal(fanin);
      ite = my_shannon_ite(ite_lit, ite_0, ite_1);
      return ite;
  }

  node_fun_S0 = node_function(match->S0);
  node_fun_S1 = node_function(match->S1);
  ite_THEN = ite_for_muxnode(match->SB, match->B0, match->B1);
  ite_ELSE = ite_for_muxnode(match->SA, match->A0, match->A1);
  if (node_fun_S0 == NODE_1 || node_fun_S1 == NODE_1) {
      ite_IF = ite_1;
  } else {
      if (node_fun_S0 == NODE_0) {
          ite_IF = ite_get_vertex(match->S1);
      } else {
          if (node_fun_S1 == NODE_0) {
              ite_IF = ite_get_vertex(match->S0);
          } else {
              ite_IF_1 = ite_get_vertex(match->S0);
              ite_IF_2 = ite_get_vertex(match->S1);
              ite_IF = my_shannon_ite(ite_IF_1, ite_1, ite_IF_2);
          }
      }
  }
  ite = my_shannon_ite(ite_IF, ite_THEN, ite_ELSE);
  return ite;
}

ite_vertex *
ite_for_muxnode(control, zero, one)
  node_t *control, *zero, *one;
{

  node_function_t node_fun;
  node_t *mux_node, *fanin;
  ite_vertex *ite, *ite_IF, *ite_THEN, *ite_ELSE;

  mux_node = act_mux_node(control, zero, one);  
  node_fun = node_function(mux_node);
  switch(node_fun) {
    case NODE_0:
      ite = ite_0;
      break;
    case NODE_1: 
      ite = ite_1;
      break;
    case NODE_BUF:
      fanin = node_get_fanin(mux_node, 0);
      ite = ite_buffer(fanin);
      break;
    case NODE_INV:
      fanin = node_get_fanin(mux_node, 0);
      ite = ite_inv(fanin);
      break;
    default:
      ite_IF = ite_get_vertex(control);
      ite_THEN = ite_get_vertex(one);
      ite_ELSE = ite_get_vertex(zero);
      ite = my_shannon_ite(ite_IF, ite_THEN, ite_ELSE);
      break;
  }
  node_free(mux_node);
  return ite;
}
      
ite_vertex *
ite_buffer(node)
  node_t *node;
{

  ite_vertex *ite, *ite_IF;

  ite_IF = ite_new_literal(node);
  ite = my_shannon_ite(ite_IF, ite_1, ite_0);      
  return ite;
}

ite_vertex *
ite_inv(node)
  node_t *node;
{

  ite_vertex *ite, *ite_IF;

  ite_IF = ite_new_literal(node);
  ite = my_shannon_ite(ite_IF, ite_0, ite_1);      
  return ite;
}

ite_vertex *
ite_get_vertex(node)
  node_t *node;
{
  node_function_t node_fun;
  ite_vertex *ite;
  node_t *fanin;

  node_fun = node_function(node);

  if (node_fun == NODE_0) {
      return ite_0;
  }
  if (node_fun == NODE_1) {
      return ite_1;
  }
  fanin = node_get_fanin(node, 0);
  ite = ite_buffer(fanin);
  return ite;
}

/*----------------------------------------------------------------------
  Get ite for each cubenode and then OR these ites.
-----------------------------------------------------------------------*/
ite_vertex *
ite_create_ite_for_orthogonal_cubes(node)
  node_t *node;
{
  array_t *full_ite_vec, *muxfree_ite_vec, *cubevec;
  node_t *cubenode;
  int i, is_mux_free, p, q;
  ite_vertex *ite;

  full_ite_vec = array_alloc(ite_vertex *, 0);
  muxfree_ite_vec = array_alloc(ite_vertex *, 0);

  cubevec = pld_nodes_from_cubes(node);
  for (i = 0; i < array_n(cubevec); i++) {
      cubenode = array_fetch(node_t *, cubevec, i);
      ite = ite_new_ite_for_cubenode(cubenode);
      pld_find_pos_neg_litcount_in_cube(cubenode, &p, &q); /* p +ve, q -ve */
      is_mux_free = ite_find_mux_remaining(p, q, 0);
      if (is_mux_free) {
          array_insert_last(ite_vertex *, muxfree_ite_vec, ite);
      } else {
          array_insert_last(ite_vertex *, full_ite_vec, ite);
      }
  }
  ite_interleave_muxfree_and_full(muxfree_ite_vec, full_ite_vec, 0, 0);
  ite = array_fetch(ite_vertex *, full_ite_vec, array_n(full_ite_vec) - 1);

  array_free(muxfree_ite_vec);
  array_free(full_ite_vec);
  for (i = 0; i < array_n(cubevec); i++) {
      cubenode = array_fetch(node_t *, cubevec, i);
      node_free(cubenode);
  }
  array_free(cubevec);

  return ite;
}

/*----------------------------------------------------------------------
  Given p = number of +ve literals, and q = number of -ve literals in a
  cube, return 1 if the top mux of the ACT1 architecture will be 
  unutilized, else return 0.
-----------------------------------------------------------------------*/
int
ite_find_mux_remaining(p, q, count)
  int p, q, count;
{
  int q_3;

  if (p == 0 && q == 0) return 0;
  if (count == 0) {
      /* if the cube is just eg f = a, then treat it as full */
      /*-----------------------------------------------------*/
      if (p == 1 && q == 0) return 0;
      if (q == 0) {
          if (p == 2) return 1;
          return ((p - 3) % 2);
      }
      if (p == 0) {
          q_3 = q % 3;
          if (q_3 == 0 || q_3 == 2) return 0;
          return 1;
      }
      if (q == 1) {
          if (p == 1) return 1;
          return ((p - 2) % 2);
      }
      if (p == 1) {
          q_3 = q % 3;
          if (q_3 == 0 || q_3 == 2) return 0;
          return 1;
      }
      return ite_find_mux_remaining(p - 2, q - 2, 1);
  }
  /* count = 1 */
  if (q == 0) return (p % 2);
  if (p == 0) {
      q_3 = q % 3;
      if (q_3 == 0 || q_3 == 2) return 0;
      return 1;
  }
  if (q == 1) return ((p - 1) %2);
  return ite_find_mux_remaining(q - 2, p -1 , 1);
}
      
/*-----------------------------------------------------------------------
  Given a cubenode, return in p number of variables in the positive phase
  and in q, the ones in negative phase.
------------------------------------------------------------------------*/
pld_find_pos_neg_litcount_in_cube(cubenode, p, q)
  node_t *cubenode;
  int *p, *q;
{
  int i;
  node_t *fanin;
  input_phase_t phase;

  *p = *q = 0;
  assert(node_num_cube(cubenode) == 1);
  for (i = 0; i < node_num_fanin(cubenode); i++) {
      fanin = node_get_fanin(cubenode, i);
      phase = node_input_phase(cubenode, fanin);
      switch(phase) {
        case POS_UNATE:
          (*p)++;
          break;
        case NEG_UNATE:
          (*q)++;
          break;
        default:
          (void) printf("pld_find_pos_neg_litcount_in_cube(): variable type binate?");
          exit(1);
      }
  }
}

/*-------------------------------------------------------------------
 Pick one element from muxfree_ite_vec and two from full_ite_vec,
 OR them in an appropriate way such that the unused mux of the muxfree
 ite gets utilized. Boundary cases are handled explicitly causing the
 code to become longish.
---------------------------------------------------------------------*/
ite_interleave_muxfree_and_full(muxfree_ite_vec, full_ite_vec, m, f)
  array_t *muxfree_ite_vec, *full_ite_vec;
  int m, f;  /* pointers to current ites in muxfree and full - ite_vecs */
{
  ite_vertex *ite, *ite_a, *ite_b, *ite_c, *ite_d, *ite_c1, *ite_c2, *ite_bc, *ite_cd, *ite_ab, *temp;
  int diff, diff_f, a_inv, b_inv, c_inv, temp_inv;

  /* when m and f point to the last elements, we are done */
  /*------------------------------------------------------*/
  if ((m == array_n(muxfree_ite_vec)) && (f >= (array_n(full_ite_vec) - 1))) return;

  /* muxfree ites are over, try to combine at most 4 of the full ites */
  /*------------------------------------------------------------------*/
  if (m == array_n(muxfree_ite_vec)) {
      while ((diff = array_n(full_ite_vec) - f) >= 2) {
          if (diff == 2) {
              ite_a = array_fetch(ite_vertex *, full_ite_vec, f++);
              ite_b = array_fetch(ite_vertex *, full_ite_vec, f++);
              ite = ite_new_OR(ite_a, ite_b);
              array_insert_last(ite_vertex *, full_ite_vec, ite);
          } else {
              if (diff == 3) {
                  ite_a = array_fetch(ite_vertex *, full_ite_vec, f++);
                  ite_b = array_fetch(ite_vertex *, full_ite_vec, f++);
                  ite_c = array_fetch(ite_vertex *, full_ite_vec, f++);
                  ite_ab = ite_new_OR(ite_a, ite_b);
                  ite = ite_new_OR(ite_c, ite_ab);
                  array_insert_last(ite_vertex *, full_ite_vec, ite);              
              } else {
                  ite_a = array_fetch(ite_vertex *, full_ite_vec, f++);
                  ite_b = array_fetch(ite_vertex *, full_ite_vec, f++);
                  ite_c = array_fetch(ite_vertex *, full_ite_vec, f++);
                  ite_d = array_fetch(ite_vertex *, full_ite_vec, f++);
                  ite_ab = ite_new_OR(ite_a, ite_b);
                  ite_cd = ite_new_OR(ite_c, ite_d);
                  ite = ite_new_OR(ite_cd, ite_ab);
                  array_insert_last(ite_vertex *, full_ite_vec, ite);              
              }
          }
      }
      return;
  }
  
  /* if full ites are over (can happen when there were none to start with),
     combine at most 3 mux ites.                                          */
  /*----------------------------------------------------------------------*/
  if (f == array_n(full_ite_vec)) {
      diff = array_n(muxfree_ite_vec) - m;
      if (diff == 1) {
          ite = array_fetch(ite_vertex *, muxfree_ite_vec, m++);
          array_insert_last(ite_vertex *, full_ite_vec, ite);          
          return;
      } else {
          if (diff == 2) {
              ite_a = array_fetch(ite_vertex *, muxfree_ite_vec, m++);
              ite_b = array_fetch(ite_vertex *, muxfree_ite_vec, m++);
              if (ite_is_inv(ite_a)) {
                  ite = ite_new_OR_with_inv(ite_a, ite_b); /* saving a mux (an inverter) */
              } else {
                  if (ite_is_inv(ite_b)) {
                      ite = ite_new_OR_with_inv(ite_b, ite_a);
                  } else {
                      ite = ite_new_OR(ite_b, ite_a);
                  }
              }
              array_insert_last(ite_vertex *, full_ite_vec, ite);
              return;
          } else {
              ite_a = array_fetch(ite_vertex *, muxfree_ite_vec, m++);
              ite_b = array_fetch(ite_vertex *, muxfree_ite_vec, m++);
              ite_c = array_fetch(ite_vertex *, muxfree_ite_vec, m++);

              /* get an inverter (if present) to the "place of c" */
              /*--------------------------------------------------*/
              a_inv = ite_is_inv(ite_a);
              b_inv = ite_is_inv(ite_b);
              c_inv = ite_is_inv(ite_c);
              if (c_inv) {
              } else {
                  if (a_inv) {
                      temp = ite_a;
                      ite_a = ite_c;
                      ite_c = temp;
                      temp_inv = a_inv;
                      a_inv = c_inv;
                      c_inv = temp_inv;
                  } else {
                      if (b_inv) {
                          temp = ite_b;
                          ite_b = ite_c;
                          ite_c = temp;
                          temp_inv = b_inv;
                          b_inv = c_inv;
                          c_inv = temp_inv;
                      }
                  }
              }
              /* get an inverter if possible on a */
              /*----------------------------------*/
              if (b_inv && !a_inv) {
                  temp = ite_a;
                  ite_a = ite_b;
                  ite_b = temp;
                  temp_inv = a_inv;
                  a_inv = b_inv;
                  b_inv = temp_inv;
              }                  
              if (c_inv) {
                  ite_bc = ite_new_OR_with_inv(ite_c, ite_b);
              } else {
                  ite_bc = ite_new_OR(ite_b, ite_c);
              }
              if (a_inv) {
                  ite = ite_new_OR_with_inv(ite_a, ite_bc);
              } else {
                  ite = ite_new_OR(ite_bc, ite_a);
              }
              array_insert_last(ite_vertex *, full_ite_vec, ite);
              ite_interleave_muxfree_and_full(muxfree_ite_vec, full_ite_vec, m, f);
              return;
          }
      }
  }      

  /* combination of muxfree and full ites to be combined - at least one of each present*/
  /*-----------------------------------------------------------------------------------*/
  diff_f = array_n(full_ite_vec) - f;
  if (diff_f == 1) {
      ite_c = array_fetch(ite_vertex *, full_ite_vec, f++);
  } else {
      ite_c1 = array_fetch(ite_vertex *, full_ite_vec, f++);
      ite_c2 = array_fetch(ite_vertex *, full_ite_vec, f++);      
      ite_c = ite_new_OR(ite_c1, ite_c2);      
  }
  ite_a = array_fetch(ite_vertex *, muxfree_ite_vec, m++);
  if (ite_is_inv(ite_a)) {
      ite = ite_new_OR_with_inv(ite_a, ite_c);        
  } else {
      ite = ite_new_OR(ite_c, ite_a);        
  }
  array_insert_last(ite_vertex *, full_ite_vec, ite);
  ite_interleave_muxfree_and_full(muxfree_ite_vec, full_ite_vec, m, f);
}

ite_vertex *
ite_new_OR(ite_a, ite_b)
  ite_vertex *ite_a, *ite_b;
{
  ite_vertex *ite;

  ite = my_shannon_ite(ite_a, ite_1, ite_b);
  return ite;
}

/*--------------------------------------------------------------------
  Returns ite_a + ite_b, knowing that ite_a is an inverter. So first 
  makes ite_a' (a positive literal) and then does OR by interchanging
  the otherwise THEN and ELSE children of the root node.
----------------------------------------------------------------------*/
ite_vertex *
ite_new_OR_with_inv(ite_a, ite_b)
  ite_vertex *ite_a, *ite_b;
{
  ite_vertex *temp, *ite;

  if (ite_a->value == 2) {
      assert (ite_a->phase == 0);
      ite_a->phase = 1;
  } else {
      temp = ite_a->THEN;
      ite_a->THEN = ite_a->ELSE;
      ite_a->ELSE = temp;
  }
  ite = my_shannon_ite(ite_a, ite_b, ite_1);
  return ite;
}

int 
ite_is_inv(ite)
  ite_vertex *ite;
{
  if (ite->value == 2 && ite->phase == 0) return 1; /* this should happen only in a canonical ite */
  if (ite->value == 3 && ite->IF->value == 2 && ite->IF->phase == 1 && ite->THEN->value == 0 &&
      ite->ELSE->value == 1) return 1; 
  return 0;
}

/*-------------------------------------------------------
  Returns the number of binate variables in a node.
-------------------------------------------------------*/
int
node_num_binate_variables(node)
  node_t *node;
{
  int i, num_binate = 0;
  node_t *fanin;

  for (i = 0; i < node_num_fanin(node); i++) {
      fanin = node_get_fanin(node, i);
      if (node_input_phase(node, fanin) == BINATE) num_binate++;
  }
  return num_binate;
}

/*------------------------------------------------------------
  Assigns weight to the fanin. The weight takes into account
  1) number of occurrences of the fanin,
  2) binateness of the fanin,
  3) binateness of the functions resulting on cofactoring 
     node wrt fanin.
-------------------------------------------------------------*/

int 
ite_assign_var_weight(node, fanin, j, lit_count_array)
  node_t *node, *fanin;
  int j, *lit_count_array;
{
  int j2, j2p1, weight1, weight2, weight;
  int binateness_if, binateness_then, binateness_else, total_binateness, 
        node_compute_binateness();
  node_t *node_if, *node_else_else, *node_else_then;

  j2 = 2 * j;
  j2p1 = j2 + 1;

  weight1 = lit_count_array[j2] + lit_count_array[j2p1];
  weight2 = lit_count_array[j2] && lit_count_array[j2p1];

  node_algebraic_cofactor(node, fanin, &node_else_then, &node_else_else, &node_if);
  binateness_if = node_compute_binateness(node_if);
  binateness_then = node_compute_binateness(node_else_then);
  binateness_else = node_compute_binateness(node_else_else);
  total_binateness = binateness_if + binateness_then + binateness_else;
  
  /*
  if (total_binateness > 0.6) {
      weight3 = 2;
  } else {
      if (total_binateness > 0.3) {
          weight3 = 1;
      } else {
          weight3 = 0;
      }
  }
  */
  weight = weight1 + ACT_ITE_ALPHA * weight2 + ACT_ITE_GAMMA * total_binateness;
  node_free(node_else_then);
  node_free(node_else_else);
  node_free(node_if);
  return weight;
}

int
node_compute_binateness(node)
  node_t *node;
{
  int num_fanin, num_bin;

  num_fanin = node_num_fanin(node);
  if (num_fanin == 0) return 0; 

  if (node_function(node) == NODE_BUF) return 0;

  num_bin = node_num_binate_variables(node);
  return num_bin;
  /* return (((float) num_bin)/num_fanin); */
}

/*------------------------------------------------------------------------------
  Maps each of three algebraic cofactors and computes the cost. If the cost 
  turns out to be more than the minimum seen so far, abandons further mapping.
  In this case, the cost returned is not the right estimate, but that is fine
  since a lower cost is known.
-------------------------------------------------------------------------------*/
int 
ite_assign_var_cost(node, fanin, init_param, min_cost_so_far)
  node_t *node, *fanin;
  act_init_param_t *init_param;
  int min_cost_so_far;
{
  node_t *node_if, *node_else_else, *node_else_then;
  int total_cost;

  node_algebraic_cofactor(node, fanin, &node_else_then, &node_else_else, &node_if);
  
  total_cost = act_ite_new_map_node(node_if, init_param);
  if (total_cost < min_cost_so_far) {
      total_cost += act_ite_new_map_node(node_else_then, init_param);
      if (total_cost < min_cost_so_far) {
          total_cost += act_ite_new_map_node(node_else_else, init_param);
      }
  }
  
  act_free_ite_node(node_if);
  act_free_ite_node(node_else_then);
  act_free_ite_node(node_else_else);

  node_free(node_if);
  node_free(node_else_then);
  node_free(node_else_else);

  return total_cost;
}


int node_is_unate(node)
  node_t *node;
{
  int i;
  node_t *fanin;
  input_phase_t input_phase;

  foreach_fanin(node, i, fanin) {
      input_phase = node_input_phase(node, fanin);
      assert(input_phase != PHASE_UNKNOWN);
      if (input_phase == BINATE)  return 0;
  }
  return 1;
}


/* Constructs the cover of the node. We use the sm package because we 
   need to solve a column covering problem at the unate leaf*/
/* no changes needed */
static
sm_matrix *
build_F(node)
node_t *node;
{
    int i, j;
    node_cube_t cube;
    node_literal_t literal;
    sm_matrix *F;
    node_t *fanin;
    sm_col *p_col;

    F = sm_alloc();
    
    for(i = node_num_cube(node)-1; i >= 0; i--) {
	cube = node_get_cube(node, i);
	foreach_fanin(node, j, fanin) {
	    literal = node_get_literal(cube, j);
	    switch(literal) {
	      case ONE:
		 sm_insert(F, i, j)->user_word = (char *)1;
		break;
	      case ZERO:
		 sm_insert(F, i, j)->user_word = (char *)0;
		break;
	      case TWO:
		 sm_insert(F, i, j)->user_word = (char *)2;
		break;
	      default:
		(void)fprintf(misout, "error in F construction \n");
		break;
	    }
	}
    }
/* STuff the fanin names in the user word for each column*/
    foreach_fanin(node, j, fanin) {
	p_col = sm_get_col(F, j);
	p_col->user_word = node_long_name(fanin);
    }
    return F;
}


ite_vertex *
ite_new_ite_for_unate_cover(node, init_param)
  node_t *node;
  act_init_param_t *init_param;
{
  sm_matrix *F, *M, *build_F();
  sm_col *pcol, *col;
  sm_row *row, *F_row, *cover;
  sm_element *p, *q;
  int *phase, i, j, is_neg_phase, *weights, pos_weight, col_num;
  ite_vertex *ite_so_far, *ite_sub_node_1, *ite_and, *vertex_if;
  node_t *variable, *variable_lit, *sub_node, *sub_node_1, *node_cube, *temp_node;
  input_phase_t inp_phase;
  node_cube_t cube;
  
  F = build_F(node);

/* Construct the M matrix which is used for covering puposes*/	
  is_neg_phase = 0; /* see if some variable in -ve phase */
  phase = ALLOC(int, F->ncols);
  M = sm_alloc();
  sm_foreach_col(F, pcol) {
      sm_foreach_col_element(pcol, p){
          if(p->user_word != (char *)2) {
              (void) sm_insert(M, p->row_num, p->col_num);
              phase[p->col_num] = (int )p->user_word;
              if (phase[p->col_num] == 0) is_neg_phase = 1;
          }
      }
  } 

  /* solve covering problem for M*/	
  /* assigning weights - in general, give slightly lower weights to the
     -ve phase variables, so that they may occur higher in the order.
     "Slight" difference, so that it is not possible to reduce the 
     weight of the cover by removing one positive guy and putting more 
     than one negative guys. . We want the
     cardinality of the cover to be minimum, with the ties being broken
     in favour of -ve wt. variables.
     If all variables of positive phase, pass NIL (int).                */
  /*--------------------------------------------------------------------*/
  if (is_neg_phase) {
      weights = ALLOC(int, M->ncols);
      pos_weight = M->ncols + 1;
      for (i = 0, j = 0; i < F->ncols; i++) {
          switch(phase[i]) {
            case 0:
              weights[j] = M->ncols;
              j++;
              break;
            case 1:  
              weights[j] = pos_weight;
              j++;
              break;
            case 2:
              break;
          }
      }
      assert(j == M->ncols);
      cover =  sm_minimum_cover(M, weights, 0, 0);
      FREE(weights);
  } else {
      cover = sm_minimum_cover(M, NIL(int), 0, 0);
  }

  sm_foreach_row_element(cover, p){
      p->user_word = (char *) phase[p->col_num];
  }
  sm_free_space(M);

  /* constructs nodes that go with selected fanins */
  /*-----------------------------------------------*/
  
  sm_foreach_row(F, row) {
      row->user_word = (char *)0;
  }
  ite_so_far = NIL (ite_vertex);
  sm_foreach_row_element(cover, q) {
      col_num = q->col_num;
      variable = node_get_fanin(node, col_num);
      assert(variable);
      sub_node = node_constant(0);
      col = sm_get_col(F, col_num);
      sm_foreach_col_element(col, p) {
          F_row = sm_get_row(F, p->row_num);
          if((p->user_word != (char *)2) && (F_row->user_word == (char *)0)) { 
              cube = node_get_cube(node, p->row_num);
              node_cube = pld_make_node_from_cube(node, cube);
              temp_node = node_or(sub_node, node_cube);
              node_free(sub_node);
              node_free(node_cube);
              sub_node = temp_node;
              F_row->user_word = (char *)1;
          }
      }
      /* do something with the sub-node */
      /*--------------------------------*/
      if (phase[col_num]) {
          variable_lit = node_literal(variable, 1);
          inp_phase = POS_UNATE;
      } else {
          variable_lit = node_literal(variable, 0);
          inp_phase = NEG_UNATE;
      }
      sub_node_1 = node_cofactor(sub_node, variable_lit);
      node_free(sub_node);
      node_free(variable_lit);
      act_ite_new_make_ite(sub_node_1, init_param);
      ite_sub_node_1 = ACT_ITE_ite(sub_node_1);
      ACT_ITE_ite(sub_node_1) = NIL (ite_vertex);
      act_free_ite_node(sub_node_1); /* ?? */
      node_free(sub_node_1);
      /* ANDing being done this way, because ite_0 is static in ite_new_urp.c */
      /*----------------------------------------------------------------------*/
      vertex_if = ite_new_literal(variable);
      if (inp_phase == POS_UNATE) {
          ite_and = my_shannon_ite(vertex_if, ite_sub_node_1, ite_0);
      } else {
          ite_and = my_shannon_ite(vertex_if, ite_0, ite_sub_node_1);
      }
      if (ite_so_far == NIL (ite_vertex)) {
          ite_so_far = ite_and;
      } else {
          ite_so_far = ite_new_OR(ite_so_far, ite_and);
      }
  }
  sm_row_free(cover);
  sm_free_space(F);
  FREE(phase);
  return ite_so_far;

}
