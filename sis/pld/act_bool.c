#include "sis.h"
#include "act_bool.h"
extern int ACT_ITE_DEBUG;

/*
*   Jan 29 '93 - added act_is_or_used.
*/

static node_t *act_form_G();

/*--------------------------------------------------------------------------------
  This includes routines for Boolean matching of a function against actel block.
  In the spirit of ENUFOR... Assumes that the nodes are expressed using 
  minimal support. act_is_or_used is 0 if the OR gate is not used.
---------------------------------------------------------------------------------*/

act_bool_map_network(network, map_alg, act_is_or_used, print_flag)
  network_t *network;
  int map_alg, act_is_or_used, print_flag;
{
  array_t *nodevec;
  int i, count_OR = 0, count = 0;
  ACT_MATCH *match, *act_is_act_function();
  node_t *node;
  node_function_t node_fun_S0, node_fun_S1;
  extern char *io_name(); /* to take care of inputs to POs */

  nodevec = network_dfs(network);
  for  (i = 0; i < array_n(nodevec); i++) {
      node = array_fetch(node_t *, nodevec, i);
      match = act_is_act_function(node, map_alg, act_is_or_used);
      if (match) {
          node_fun_S0 = node_function(match->S0);
          node_fun_S1 = node_function(match->S1);
          if (node_fun_S0 != NODE_0 && node_fun_S0 != NODE_1 &&
              node_fun_S1 != NODE_0 && node_fun_S1 != NODE_1) {
              count_OR++;
          }
          count++;
          if (print_flag) {
              (void) printf("match found for node %s\n", io_name(node, 0));
              act_print_match(match);
          }
          act_free_match(match);
      } else {
          if ((node->type != PRIMARY_INPUT) && (node->type != PRIMARY_OUTPUT))
              if (print_flag) (void) printf("no match found for node %s\n", io_name(node, 0));
      }
  }
  array_free(nodevec);
  (void) printf("out of %d matches, %d use OR gate\n", count, count_OR);
}

/*---------------------------------------------------------------------------
  Given a function f, finds if f can be implemented by one act block. If so,
  returns the first match found in match. Else returns NIL. Assumes f is 
  expressed in minimum base. Hardwired info about the block. 
----------------------------------------------------------------------------*/
ACT_MATCH *
act_is_act_function(f, map_alg, act_is_or_used)
  node_t *f;
  int map_alg, act_is_or_used;
{
  int num_fanin;
  node_t *fanin, *fanin_lit, *f_fanin_pos, *faninA, *faninB, *faninBc;
  node_t *A0, *A1, *SA, *B0, *B1, *SB;
  node_t *cof_A, *cof_B, *cof_Ac, *cof_Ac_Bc, *cof_Ac_B, *cof_A_Bc;
  st_table *table;
  int i, j;
  COFC_STRUCT *cofc, *cofcA, *cofcB;
  ACT_MATCH *match, *act_alloc_match();
  node_t *fanin_A_pos, *fanin_A_neg, *fanin_B_pos, *fanin_B_neg;
  node_t *G, *act_form_G();

  if (f->type == PRIMARY_INPUT || f->type == PRIMARY_OUTPUT) return NIL (ACT_MATCH);
  
  if (node_function(f) == NODE_0) return act_alloc_match
					 (node_constant(0),  node_constant(0), 
					  node_constant(0),  node_constant(0),
					  node_constant(0),  node_constant(0),
					  node_constant(0),  node_constant(0));
  if (node_function(f) == NODE_1) return act_alloc_match
					 (node_constant(1),  node_constant(1), 
					  node_constant(1),  node_constant(1),
					  node_constant(1),  node_constant(1),
					  node_constant(1),  node_constant(1));

  /* simplify_node(f, SIM_METHOD_SNOCOMP, SIM_DCTYPE_NONE, SIM_FILTER_NONE, 
                SIM_ACCEPT_SOP_LITS);
  */                
  num_fanin = node_num_fanin(f);

  if (num_fanin > 8) return NIL (ACT_MATCH);
  if ((!act_is_or_used) && num_fanin > 7) return NIL (ACT_MATCH);

  table = st_init_table(st_ptrcmp, st_ptrhash);

  /* find all the fanins which can potentially fanin to OR gate */
  /*---------------------------------------------------------------*/
  foreach_fanin(f, i, fanin) {
      fanin_lit = node_literal(fanin, 1);
      f_fanin_pos = node_cofactor(f, fanin_lit);
      node_free(fanin_lit);
      simplify_node(f_fanin_pos, SIM_METHOD_SNOCOMP, SIM_DCTYPE_NONE, SIM_FILTER_NONE, 
                    SIM_ACCEPT_SOP_LITS);
      /* changed 4 to 3 - Oct. 19, 1991 - since even if OR gate uses 
         both the inputs, then f = (a + b) L + a' b' M => cofac f wrt 
         a is L_a & since L is mux-implementable, L_a has <= 3 inp. */
      /*------------------------------------------------------------*/
         
      if (node_num_fanin(f_fanin_pos) > 3) {
          node_free(f_fanin_pos);
          continue;
      }
      /* store f_fanin along with the fanin */
      /*------------------------------------*/
      cofc = ALLOC(COFC_STRUCT, 1);
      cofc->pos = f_fanin_pos;
      cofc->pos_mux = 0;
      cofc->B0 = NIL (node_t);
      cofc->B1 = NIL (node_t);
      cofc->SB = NIL (node_t);      
      cofc->neg = NIL (node_t);
      cofc->node = f;
      cofc->fanin = fanin;
      assert(!st_insert(table, (char *) fanin, (char *) cofc));
  }
  
  /* check if some single fanin can be put at the OR gate */
  /*-------------------------------------------------------*/
  foreach_fanin(f, i, fanin) {
      if (!st_lookup(table, (char *) fanin, (char **)&cofc)) continue;
      if (act_is_mux_function(cofc->pos, &B0, &B1, &SB)) {
          cofc->pos_mux = 1;
          cofc->B0 = B0;
          cofc->B1 = B1;
          cofc->SB = SB;
          act_bool_set_cof_neg(cofc);
          if (act_is_mux_function(cofc->neg, &A0, &A1, &SA)) {
              /* set the pointers, free storage and return 1 */
              /*---------------------------------------------*/
              if (ACT_ITE_DEBUG) (void) printf("found a match\n");
              match = act_alloc_match(A0, A1, SA, node_dup(B0), node_dup(B1),
                                      node_dup(SB), node_literal(fanin, 1), 
                                      node_constant(0));
              act_bool_free_table(table);
              return match;
          } 
      }
  }

  if (!act_is_or_used) {
      act_bool_free_table(table);
      return NIL (ACT_MATCH);
  }

  /* check for a pair at the OR gate */
  /*---------------------------------*/
  for (i = 0; i < num_fanin; i++) {
      faninA = node_get_fanin(f, i);
      if (!st_lookup(table, (char *) faninA, (char **)&cofcA)) continue;
      if (cofcA->pos_mux != 1) continue;
      cof_A = cofcA->pos;
      act_bool_set_cof_neg(cofcA);
      for (j = i + 1; j < num_fanin; j++) {
          faninB = node_get_fanin(f, j);
          if (!st_lookup(table, (char *) faninB, (char **)&cofcB)) continue;          
          cof_B = cofcB->pos;
          act_bool_set_cof_neg(cofcB);
          /* using my_node_equal, since node_equal does not take care of 1 */
          /*---------------------------------------------------------------*/
          if (my_node_equal(cof_A, cof_B)) {
              /* check if the cof_A' cof_B' is realizable by a mux */
              /*---------------------------------------------------*/
              cof_Ac = cofcA->neg;
              faninBc = node_literal(faninB, 0);
              cof_Ac_Bc = node_cofactor(cof_Ac, faninBc);
              simplify_node(cof_Ac_Bc, SIM_METHOD_SNOCOMP, SIM_DCTYPE_NONE, 
                            SIM_FILTER_NONE, SIM_ACCEPT_SOP_LITS);
              if (act_is_mux_function(cof_Ac_Bc, &A0, &A1, &SA)) {
                  if (ACT_ITE_DEBUG) (void) printf("found a match\n");
                  /* free the storage */
                  /*------------------*/
                  match = act_alloc_match(A0, A1, SA, node_dup(cofcA->B0),
                                          node_dup(cofcA->B1), node_dup(cofcA->SB), 
                                          node_literal(faninA, 1), 
                                          node_literal(faninB, 1));
                  act_bool_free_table(table);
                  return match;
              }
          }
      }
  }
  if (map_alg) {
      /* do not do general matching */
      /*----------------------------*/
      act_bool_free_table(table);
      return NIL (ACT_MATCH);
  }

  /* disjoint match not found - try general match */
  /*----------------------------------------------*/
  for (i = 0; i < num_fanin; i++) {
      faninA = node_get_fanin(f, i);
      if (!st_lookup(table, (char *) faninA, (char **)&cofcA)) continue;
      fanin_A_neg = node_literal(faninA, 0);
      fanin_A_pos = node_literal(faninA, 1);
      act_bool_set_cof_neg(cofcA); 
      for (j = i + 1; j < num_fanin; j++) {
          faninB = node_get_fanin(f, j);
          if (!st_lookup(table, (char *) faninB, (char **)&cofcB)) continue;          
          /* check if the cof_A' cof_B' is realizable by a mux */
          /*---------------------------------------------------*/
          fanin_B_neg = node_literal(faninB, 0);
          fanin_B_pos = node_literal(faninB, 1);
          act_bool_set_cof_neg(cofcB);
          cof_Ac_Bc = node_cofactor(cofcA->neg, fanin_B_neg);
          simplify_node(cof_Ac_Bc, SIM_METHOD_SNOCOMP, SIM_DCTYPE_NONE, 
                        SIM_FILTER_NONE, SIM_ACCEPT_SOP_LITS);
          if (act_is_mux_function(cof_Ac_Bc, &A0, &A1, &SA)) {
              /* form G, H */
              /*-----------*/
              cof_Ac_B  = node_cofactor(cofcB->pos, fanin_A_neg);
              simplify_node(cof_Ac_B, SIM_METHOD_SNOCOMP, SIM_DCTYPE_NONE, 
                            SIM_FILTER_NONE, SIM_ACCEPT_SOP_LITS);
              cof_A_Bc  = node_cofactor(cofcA->pos, fanin_B_neg);
              simplify_node(cof_A_Bc, SIM_METHOD_SNOCOMP, SIM_DCTYPE_NONE, 
                            SIM_FILTER_NONE, SIM_ACCEPT_SOP_LITS);
              
              G = act_form_G(cofcA->pos, cof_Ac_B, fanin_A_pos, fanin_B_pos, 
                             fanin_A_neg);
              simplify_node(G, SIM_METHOD_SNOCOMP, SIM_DCTYPE_NONE, 
                            SIM_FILTER_NONE, SIM_ACCEPT_SOP_LITS);
              assert(node_get_fanin_index(f, faninA) == i);
              assert(node_get_fanin_index(f, faninB) == j);
              
              if (act_find_H(f, G, fanin_A_neg, fanin_B_neg, i, j, cof_Ac_B, 
                             cof_A_Bc, &B0, &B1, &SB)) {
                  match = act_alloc_match(A0, A1, SA, B0, B1, SB, fanin_A_pos, 
                                          fanin_B_pos);
                  node_free_list(fanin_A_neg, fanin_B_neg, cof_Ac_Bc, G, cof_Ac_B);
                  node_free(cof_A_Bc);
                  act_bool_free_table(table);
                  return match;
              } 
              node_free_list(cof_Ac_B, G, cof_A_Bc, NIL(node_t), NIL(node_t));
          }
          node_free_list(cof_Ac_Bc, fanin_B_pos, fanin_B_neg, NIL(node_t), NIL(node_t));
      }
      node_free_list(fanin_A_pos, fanin_A_neg, NIL(node_t), NIL(node_t), NIL(node_t));
  }
  act_bool_free_table(table);
  return NIL (ACT_MATCH);
}

/*--------------------------------------------------------------------
  Computes cofactor of cofc->node wrt. cofac->fanin'. Sets the neg
  pointer. Does this only if neg entry is NIL to begin with.
---------------------------------------------------------------------*/
act_bool_set_cof_neg(cofc)
  COFC_STRUCT *cofc;
{
  node_t *fanin_lit, *neg;

  if (cofc->neg == NIL (node_t)) {
      fanin_lit = node_literal(cofc->fanin, 0);
      neg = node_cofactor(cofc->node, fanin_lit);
      simplify_node(neg, SIM_METHOD_SNOCOMP, SIM_DCTYPE_NONE, SIM_FILTER_NONE, 
                    SIM_ACCEPT_SOP_LITS);
      cofc->neg = neg;
      
  }
}

act_bool_free_table(table)
  st_table *table;
{
  st_generator *stgen;
  node_t *fanin;
  COFC_STRUCT *cofc;

  st_foreach_item(table, stgen, (char **) &fanin, (char **) &cofc) {
      act_bool_free_cofc(cofc);
  }
  st_free_table(table);
}

act_bool_free_cofc(cofc)
  COFC_STRUCT *cofc;
{
  node_free(cofc->pos);
  node_free(cofc->B0);
  node_free(cofc->B1);
  node_free(cofc->SB);
  node_free(cofc->neg);
  FREE(cofc);
}

ACT_MATCH *
act_alloc_match(A0, A1, SA, B0, B1, SB, S0, S1)
  node_t *A0, *A1, *SA, *B0, *B1, *SB, *S0, *S1;
{
  ACT_MATCH *match;
  
  match = ALLOC(ACT_MATCH, 1);
  match->A0 = A0;
  match->A1 = A1;
  match->SA = SA;
  match->B0 = B0;
  match->B1 = B1;
  match->SB = SB;
  match->S0 = S0;
  match->S1 = S1;
  
  return match;
}

/*--------------------------------------------------------------------------------
  Returns 1 if f can be implemented as a mux. If so, it returns the corresponding
  select, zero, one fanins. Else returns 0. 
  NOTE: Assumes that the node has been simplified.
----------------------------------------------------------------------------------*/
int 
act_is_mux_function(f, zero, one, select)
  node_t *f, **zero, **one, **select;
{
  int num_fanin, num_cube;
  node_function_t node_fn;
  input_phase_t phase0, phase1, phase2;
  node_t *fanin, *fanin0, *fanin1, *fanin2;
  node_cube_t cube0, cube1;

  num_fanin = node_num_fanin(f);
  if (num_fanin > 3) return 0;
  num_cube = node_num_cube(f);
  if (num_cube > 2) return 0;
  node_fn = node_function(f);
  switch (num_fanin) {
    case 0:
      if (node_fn == NODE_0) {
          *zero = node_constant(0);
          *one = node_constant(0);
          *select = node_constant(0);
          return 1;
      }
      assert(node_fn == NODE_1);
      *zero = node_constant(1);
      *one = node_constant(1);
      *select = node_constant(1);
      return 1;
    case 1:
      fanin = node_get_fanin(f, 0);
      if (node_fn == NODE_BUF) {
          *zero = node_constant(0);
          *one = node_constant(1);
          *select = node_literal(fanin, 1);
          return 1;
      }
      assert(node_fn == NODE_INV);
      *zero = node_constant(1);
      *one = node_constant(0);
      *select = node_literal(fanin, 1);
      return 1;
    case 2:
      fanin0 = node_get_fanin(f, 0);
      fanin1 = node_get_fanin(f, 1);
      phase0 = node_input_phase(f, fanin0);
      phase1 = node_input_phase(f, fanin1);
      if ((phase0 != POS_UNATE) && (phase1 != POS_UNATE)) return 0;
      /* num cubes = 1 */
      /*---------------*/
      if (num_cube == 1) {
          if ((phase0 == POS_UNATE) && (phase1 == NEG_UNATE)) {
              *zero = node_literal(fanin0, 1);
              *one = node_constant(0);
              *select = node_literal(fanin1, 1);
          } else {
              assert((phase0 != BINATE) && (phase1 == POS_UNATE));
              *select = node_literal(fanin0, 1);
              if (phase0 == POS_UNATE) {
                  *zero = node_constant(0);
                  *one = node_literal(fanin1, 1);
              } else {
                  *zero = node_literal(fanin1, 1);
                  *one = node_constant(0);
              }                  
          }
          return 1;
      }
      /* num cubes = 2 */
      /*---------------*/
      if (phase0 == POS_UNATE) {
          if (phase1 == POS_UNATE) {
              *zero = node_literal(fanin1, 1);
              *one =  node_constant(1);
              *select = node_literal(fanin0, 1);
          } else {
              assert(phase1 == NEG_UNATE);
              *zero = node_constant(1);
              *one = node_literal(fanin0, 1);
              *select = node_literal(fanin1, 1);
          }                  
      } else {
          assert ((phase0 == NEG_UNATE) && (phase1 == POS_UNATE));
          *zero = node_constant(1);
          *one = node_literal(fanin1, 1);
          *select = node_literal(fanin0, 1);
      }
      return 1;
   case 3:
      if (num_cube != 2)  return 0;
      cube0 = node_get_cube(f, 0);
      cube1 = node_get_cube(f, 1);
      if ((pld_num_fanin_cube(cube0, f) != 2) || (pld_num_fanin_cube(cube1, f) != 2))
          return 0;
      fanin0 = node_get_fanin(f, 0);
      fanin1 = node_get_fanin(f, 1);
      fanin2 = node_get_fanin(f, 2);      
      phase0 = node_input_phase(f, fanin0);
      phase1 = node_input_phase(f, fanin1);
      phase2 = node_input_phase(f, fanin2);
      if ((phase0 == POS_UNATE) && (phase1 == POS_UNATE) && (phase2 == BINATE)) {
          if (node_get_literal(cube0, 2) == ONE) {
              if (node_get_literal(cube0, 0) == ONE) {
                  /* f = fanin0 fanin2 + fanin1 fanin2' */
                  /*------------------------------------*/
                  act_map_mux(zero, one, select, fanin1, fanin0, fanin2);
              } else {
                  /* f = fanin1 fanin2 + fanin0 fanin2' */
                  /*------------------------------------*/
                  act_map_mux(zero, one, select, fanin0, fanin1, fanin2);
              }                  
          } else {
              if (node_get_literal(cube0, 0) == ONE) {
                  /* f = fanin0 fanin2' + fanin1 fanin2 */
                  /*------------------------------------*/
                  act_map_mux(zero, one, select, fanin0, fanin1, fanin2);
              } else {
                  /* f = fanin1 fanin2' + fanin0 fanin2 */
                  /*------------------------------------*/
                  act_map_mux(zero, one, select, fanin1, fanin0, fanin2);
              }                  
          }
          return 1;
      }
      if ((phase0 == POS_UNATE) && (phase1 == BINATE) && (phase2 == POS_UNATE)) {
          if (node_get_literal(cube0, 1) == ONE) {
              if (node_get_literal(cube0, 0) == ONE) {
                  /* f = fanin0 fanin1 + fanin2 fanin1' */
                  /*------------------------------------*/
                  act_map_mux(zero, one, select, fanin2, fanin0, fanin1);
              } else {
                  /* f = fanin2 fanin1 + fanin0 fanin1' */
                  /*------------------------------------*/
                  act_map_mux(zero, one, select, fanin0, fanin2, fanin1);
              }                  
          } else {
              if (node_get_literal(cube0, 0) == ONE) {
                  /* f = fanin0 fanin1' + fanin2 fanin1 */
                  /*------------------------------------*/
                  act_map_mux(zero, one, select, fanin0, fanin2, fanin1);
              } else {
                  /* f = fanin2 fanin1' + fanin0 fanin1 */
                  /*------------------------------------*/
                  act_map_mux(zero, one, select, fanin2, fanin0, fanin1);
              }                  
          }
          return 1;
      }
      if ((phase0 == BINATE) && (phase1 == POS_UNATE) && (phase2 == POS_UNATE)) {
          if (node_get_literal(cube0, 0) == ONE) {
              if (node_get_literal(cube0, 1) == ONE) {
                  /* f = fanin1 fanin0 + fanin2 fanin0' */
                  /*------------------------------------*/
                  act_map_mux(zero, one, select, fanin2, fanin1, fanin0);
              } else {
                  /* f = fanin2 fanin0 + fanin1 fanin0' */
                  /*------------------------------------*/
                  act_map_mux(zero, one, select, fanin1, fanin2, fanin0);
              }                  
          } else {
              if (node_get_literal(cube0, 1) == ONE) {
                  /* f = fanin1 fanin0' + fanin2 fanin0 */
                  /*------------------------------------*/
                  act_map_mux(zero, one, select, fanin1, fanin2, fanin0);
              } else {
                  /* f = fanin2 fanin0' + fanin1 fanin0 */
                  /*------------------------------------*/
                  act_map_mux(zero, one, select, fanin2, fanin1, fanin0);
              }                  
          }
          return 1;
      }
      return 0;
  }
  /*NOTREACHED */
}

act_map_mux(zero, one, select, z, o, s)
  node_t **zero, **one, **select, *z, *o, *s;
{
  *zero = node_literal(z, 1);
  *one = node_literal(o, 1);
  *select = node_literal(s, 1);
}

act_print_match(match)
  ACT_MATCH *match;
{
  act_print_match_field(match->A0, "A0");
  act_print_match_field(match->A1, "A1");
  act_print_match_field(match->SA, "SA");
  act_print_match_field(match->B0, "B0");
  act_print_match_field(match->B1, "B1");
  act_print_match_field(match->SB, "SB");
  act_print_match_field(match->S0, "S0");
  act_print_match_field(match->S1, "S1");
}

act_print_match_field(node, pin)
  node_t *node;
  char *pin;
{
  (void) printf("%s = ", pin);
  node_print_rhs(stdout, node);
  (void) printf("\n");
}
            
act_free_match(match)
  ACT_MATCH *match;
{
  if (match == NIL (ACT_MATCH)) return;

  node_free(match->A0);
  node_free(match->A1);
  node_free(match->SA);
  node_free(match->B0);
  node_free(match->B1);
  node_free(match->SB);
  node_free(match->S0);
  node_free(match->S1);
  FREE(match);
}

/*---------------------------------------------------------------------
  return A a + Ac_B a_c b.
----------------------------------------------------------------------*/
static node_t *
act_form_G(A, Ac_B, a, b, a_c)
  node_t *A, *Ac_B, *a, *b, *a_c;
{
  node_t *t1, *t2, *t3, *t4;

  t1 = node_and(A, a);

  t2 = node_and(a_c, b);
  t3 = node_and(Ac_B, t2);
  t4 = node_or(t1, t3);

  node_free(t1);  
  node_free(t2);
  node_free(t3);

  return t4;
}

int
act_find_H(f, G, Ac, Bc, index_A, index_B, Ac_B, A_Bc, pB0, pB1, pSB)
  node_t *f, *G, *Ac, *Bc, *Ac_B, *A_Bc;
  int index_A, index_B;
  node_t **pB0, **pB1, **pSB;
{
  node_t *ac_bc, *one, *C;
  node_t *C_lit_pos, *C_lit_neg;
  int i;

  ac_bc = node_and(Ac, Bc);

  /* check H = 0 */
  /*-------------*/
  if (act_is_mux_function(G, pB0, pB1, pSB)) {
      node_free(ac_bc);
      return 1;
  }

  one = node_constant(1);

  /* check H = 1 */
  /*-------------*/
  if (act_find_g_and_match(f, G, one, one, 
                           ac_bc, pB0, pB1, pSB)) {
      node_free(ac_bc);
      node_free(one);
      return 1;
  }
  foreach_fanin(f, i, C) {
      if (i == index_A || i == index_B) continue;

      /* check H = C */
      /*-------------*/
      C_lit_pos = node_literal(C, 1);
      if (act_find_g_and_match(f, G, C_lit_pos, one, 
                               ac_bc, pB0, pB1, pSB)) {
          node_free_list(C_lit_pos, ac_bc, one, NIL (node_t), NIL(node_t));
          return 1;
      }
      /* check H = C' */
      /*--------------*/
      C_lit_neg = node_literal(C, 0);
      if (act_find_g_and_match(f, G, C_lit_neg, one, 
                               ac_bc, pB0, pB1, pSB)) {
          node_free_list(C_lit_pos, ac_bc, one, C_lit_neg, NIL(node_t));
          return 1;
      }
      node_free(C_lit_neg);
      node_free(C_lit_pos);
  }

  /* try two input functions H */
  /*---------------------------*/
  if (act_is_two_input_H(Ac_B)) {
      if (act_find_g_and_match(f, G, Ac_B, one, ac_bc, pB0, pB1, pSB)) {
          node_free(ac_bc);
          node_free(one);
          return 1;
      }
  }

  if (act_is_two_input_H(A_Bc)) {
      if (act_find_g_and_match(f, G, A_Bc, one, ac_bc, pB0, pB1, pSB)) {
          node_free(ac_bc);
          node_free(one);
          return 1;
      }
  }

  /* non-disjoint match not found for given A, B*/
  /*--------------------------------------------*/
  node_free(ac_bc);
  node_free(one);
  return 0;
}

/*--------------------------------------------------------------
  Construct function g = G + H ac_bc. First you have to construct 
  H. H is essentially C D.
---------------------------------------------------------------*/
int 
act_find_g_and_match(f, G, C, D, ac_bc, pB0, pB1, pSB)
  node_t *f, *G, *C, *D, *ac_bc, **pB0, **pB1, **pSB;
{

  node_t *H, *g, *t1;

  H = node_and(C, D);
  t1 = node_and(H, ac_bc);
  g = node_or(G, t1);
  node_free(t1);
  node_free(H);
  simplify_node(g, SIM_METHOD_SNOCOMP, SIM_DCTYPE_NONE, SIM_FILTER_NONE, 
                SIM_ACCEPT_SOP_LITS);
  if (act_is_mux_function(g, pB0, pB1, pSB)) {
      node_free(g);
      return 1;
  }
  node_free(g);
  return 0;
}


node_free_list(n1, n2, n3, n4, n5)
  node_t *n1, *n2, *n3, *n4, *n5;
{
  if (n1 != NIL (node_t)) node_free(n1);
  if (n2 != NIL (node_t)) node_free(n2);
  if (n3 != NIL (node_t)) node_free(n3);
  if (n4 != NIL (node_t)) node_free(n4);
  if (n5 != NIL (node_t)) node_free(n5);
}

/*-------------------------------------------------------------------------------
  Returns 1 if the node_function if of the type a b, or a' b or a b'. Else 
  returns 0.
--------------------------------------------------------------------------------*/
int
act_is_two_input_H(node)
  node_t *node;
{
  node_t *a, *b;

  if ((node_num_fanin(node) != 2) || (node_num_cube(node) != 1)) return 0;
  a = node_get_fanin(node, 0);
  b = node_get_fanin(node, 1);
  if ((node_input_phase(node, a) == NEG_UNATE) && 
      (node_input_phase(node, b) == NEG_UNATE)) return 0;
  return 1;
}
