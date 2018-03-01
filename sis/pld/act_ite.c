#include "sis.h"
#include "pld_int.h"  /* uncommented Feb 9, 1992 */
#include "ite_int.h"

static void my_ite_OR();
/*
#ifndef
#define ON 1
#define OFF 0
#endif
*/
/* Initialize the ACT*/
/*
act_t *
my_init_act()
{
    act_t *vertex;
    
    vertex = ALLOC(act_t ,1);
    vertex->high = NULL;
    vertex->low = NULL;
    vertex->index = 0;
    vertex->value = 0;
    vertex->id = 0;
    vertex->mark = 0;
    vertex->index_size = 0;
    vertex->node = NULL;
    vertex->name = NULL;
    vertex->multiple_fo = 0;
    vertex->cost = 0;
    vertex->mapped = 0;
    vertex->parent = NULL;
    return vertex;
}

*/

/* Construct ite for a cube ie a ite which is the and of its variables*/
ite_vertex *
ite_for_cube_F(F, node)
sm_matrix *F;
node_t *node;
{
    ite_vertex *vertex, *my_ite_and();
    sm_row *row;
    sm_element *p;
    char *name;
    node_t *cube_node, *fanin;
    array_t *order_list, *single_cube_order();
    st_table *table, *act_make_name_to_element_table();
    int i;
    
    /* Avoid stupidity */    
    /*-----------------*/
    if(F->nrows > 1){
	(void)fprintf(misout, "error in act_f - more than one cube\n");
	exit(1);
    }
    row = F->first_row;
    cube_node = act_make_node_from_row(row, F, node->network);
    table = act_make_name_to_element_table(row, F);
    order_list = single_cube_order(cube_node);
    vertex = NIL (ite_vertex);
    /* reverse order, because of the way, my_ite_and works */
    /*-----------------------------------------------------*/
    for (i = array_n(order_list) - 1; i >= 0; i--) {
        fanin = array_fetch(node_t *, order_list, i);
        assert(st_lookup(table, node_long_name(fanin), (char **) &p));
        name = sm_get_col(F, p->col_num)->user_word;
        assert (strcmp(name, node_long_name(fanin)) == 0);
        assert(name == node_long_name(fanin));
        vertex = my_ite_and(p, vertex, node_long_name(fanin), fanin);
    }
    array_free(order_list);
    vertex->index_size = node_num_fanin(cube_node);
    node_free(cube_node);
    st_free_table(table);
    return vertex;
}

/* And variables in a cube*/
ite_vertex *
my_ite_and(p, vertex, name, fanin)
sm_element *p;
ite_vertex *vertex;
char *name;
node_t *fanin;
{
    ite_vertex *vertex_IF, *p_vertex, *ite_0, *ite_1;

    assert(st_lookup(ite_end_table, (char *)1, (char **) &ite_1));
    assert(st_lookup(ite_end_table, (char *)0, (char **) &ite_0));
    vertex_IF = ite_literal(p->col_num, name, fanin);

    if(vertex == NIL (ite_vertex)) {
	switch((int )p->user_word) {
	  case 0:
            p_vertex = my_shannon_ite(vertex_IF, ite_0, ite_1);
	    break;
	  case 1:
	    p_vertex = my_shannon_ite(vertex_IF, ite_1, ite_0);
            break;
	  case 2:
	    (void)fprintf(misout, "error in my_ite_and,\n");
            exit(1);
	}
    } else {
	switch((int )p->user_word) {
	  case 0:
            p_vertex = my_shannon_ite(vertex_IF, ite_0, vertex);
	    break;
	  case 1:
            p_vertex = my_shannon_ite(vertex_IF, vertex, ite_0);
	    break;
	  case 2:
	    (void)fprintf(misout, "error in my_and act, p->u_w == 2\n");
	    exit(1);

	}
    }
    return p_vertex;
}

/* from a set of sub-covers in array, and the corresponding ite_s in array_b,
   and the minimum column cover variables in cover, generates an ite for the 
   original function. */

ite_vertex *
my_or_ite_F(array_b, cover, array, network)
array_t *array_b;
array_t *array;
sm_row *cover;
network_t *network;
{
    static int compare();
    int  i;
    ite_vertex *vertex;
    sm_element *p;
    ite_vertex *ite;
    char *name; 
    node_t *fanin;

    /* Append the appropriate variables at top of ite's*/
    i = 0;
    sm_foreach_row_element(cover, p) {
	name = sm_get_col(array_fetch(sm_matrix *, array, i), p->col_num)
	  ->user_word;
	vertex = array_fetch(ite_vertex *, array_b, i);
        fanin = network_find_node(network, name);
	ite = my_ite_and(p, vertex, name, fanin);
	array_insert(ite_vertex *, array_b, i, ite);
	i++;
    }
    /* Sort the cubes so that the smaller ite s are at the top so that less fanout 
     * problem also we could use the heuristic merging done earlier*/ 

    array_sort(array_b, compare);
    ite = ite_OR_itevec(array_b);
    return ite;
}

/* Recursively Join ACT's constructed*/
static void
my_ite_OR(up_vertex, down_vertex)
ite_vertex *up_vertex, *down_vertex;
{
    
  st_table *table;
  st_generator *stgen;
  char *key;
  ite_vertex *vertex;

  table = ite_my_traverse_ite(up_vertex);

  /* change all the pointers to 0 terminal vertex to down vertex */
  /*-------------------------------------------------------------*/
  st_foreach_item(table, stgen, &key, (char **) &vertex) {
      if (vertex->value != 3) continue;
      if (vertex->IF->value == 0) vertex->IF = down_vertex;
      if (vertex->THEN->value == 0) vertex->THEN = down_vertex;
      if (vertex->ELSE->value == 0) vertex->ELSE = down_vertex;
  }
  st_free_table(table);
}

/*------------------------------------------------------------------------------
  Given the column number b_col and the name, makes a vertex for the variable.
-------------------------------------------------------------------------------*/
ite_vertex *
ite_literal(b_col, name, fanin)
  int b_col;
  char *name;
  node_t *fanin;
{
  ite_vertex *ite;
  	
  ite = ite_alloc();
  ite->value = 2;
  ite->phase = 1;
  ite->index = b_col;
  ite->index_size = 1;
  ite->name = name;
  ite->fanin = fanin;
  return ite;
}

/* ite construction where ite_IF, ite_THEN, ite_ELSE are known */
ite_vertex *
my_shannon_ite(ite_IF, ite_THEN, ite_ELSE) 
ite_vertex *ite_IF, *ite_THEN, *ite_ELSE;
{
    ite_vertex *ite;
    
    ite = ite_alloc();
    ite->value = 3;
    ite->index_size = ite_ELSE->index_size  + ite_THEN->index_size + ite_IF->index_size;
    ite->IF = ite_IF;
    ite->ELSE = ite_ELSE;
    ite->THEN = ite_THEN;
    return ite;
}

static int
compare(obj1, obj2)
char *obj1, *obj2;
{
    ite_vertex *ite1 = *(ite_vertex **)obj1;
    ite_vertex *ite2 = *(ite_vertex **)obj2;
    int value;
    
    if((ite1->index_size == 0 ) || (ite2->index_size == 0)) {
	(void)fprintf(misout, "Hey u DOLT ite of size \n");
    }
    if(ite1->index_size == ite2->index_size) value = 0;
    if(ite1->index_size > ite2->index_size) value = 1;
    if(ite1->index_size < ite2->index_size) value = -1;
    return value;
}

/*------------------------------------------------------------
  From the row of F, makes a node. This node corresponds to
  the cube corresponding to the row.
-------------------------------------------------------------*/
node_t *
act_make_node_from_row(row, F, network)
  sm_row *row;
  sm_matrix *F;
  network_t *network;
{
  node_t *n, *n1, *f, *fanin;
  int phase;
  sm_col *col;
  sm_element *p;
  char *name;

  n = node_constant(1);
  sm_foreach_row_element(row, p) {
      phase = (int) (p->user_word);
      assert(phase >= 0 && phase <= 2);
      if (phase != 2) {
          col = sm_get_col(F, p->col_num);
          name = col->user_word;
          fanin = network_find_node(network, name);
          f = node_literal(fanin, phase);
          n1 = node_and(n, f);
          node_free(n);
          node_free(f);
          n = n1;
      }
  }
  return n;
}

/*-------------------------------------------------------
  From the row, gets each element p, finds its column
  and stores p in the table with key = name of the column.
--------------------------------------------------------*/
st_table *
act_make_name_to_element_table(row, F)
  sm_row *row;
  sm_matrix *F;
{
  st_table *table;
  char *name;
  sm_element *p;
  sm_col *col;

  table = st_init_table(strcmp, st_strhash);
  sm_foreach_row_element(row, p) {
      col = sm_get_col(F, p->col_num);
      name = col->user_word;
      assert(!st_insert(table, name, (char *) p));
  }
  return table;
}
      
ite_vertex *
ite_OR_itevec(itevec)
  array_t *itevec;
{ 
  int i;
  ite_vertex *ite, *up_vertex, *down_vertex;

  up_vertex = array_fetch(ite_vertex *, itevec, 0);
  ite = up_vertex;
  for(i = 1; i < array_n(itevec); i++) {
      down_vertex = array_fetch(ite_vertex *, itevec, i);
      my_ite_OR(up_vertex, down_vertex);
      up_vertex = down_vertex;
  }
  return ite;
}

ite_vertex *
ite_check_for_single_literal_cubes(F, network)
  sm_matrix *F;
  network_t *network;
{
  int flag, i, phase, num;
  node_t *n, *n1, *f, *fanin;
  st_table *table;
  sm_row *row;
  sm_col *col;
  sm_element *p;
  char *name;
  array_t *order_list, *OR_literal_order();
  ite_vertex *ite_0, *ite_1, *ite, *ite_IF;
  array_t *ite_vec;

  /* flag is 1 whenever it is detected that some row has more than 1
     non-2 element (that cube is not single literal)                */

  flag = 0;
  n = node_constant(0);
  table = st_init_table(strcmp, st_strhash);
  sm_foreach_row(F, row) {
      if (flag) break;
      num = 0;
      sm_foreach_row_element(row, p) {
          phase = (int) p->user_word;
          if (phase == 2) continue;
          num++;
          if (num > 1) {
              flag = 1;
              break;
          }
          col = sm_get_col(F, p->col_num);
          name = col->user_word;
          fanin = network_find_node(network, name);
          f = node_literal(fanin, phase);
          n1 = node_or(n, f);
          node_free(n);
          node_free(f);
          n = n1;
          if (st_insert(table, name, (char *) p)) {
              /* a variable occurred twice */
              flag = 1;
              break;
          }
      }
  }
  if (flag) {
      node_free(n);
      st_free_table(table);
      return NIL (ite_vertex);
  }
  assert(st_lookup(ite_end_table, (char *)0, (char **) &ite_0));
  assert(st_lookup(ite_end_table, (char *)1, (char **) &ite_1));
  order_list = OR_literal_order(n);

  /* form the ite's for the single literal cubes and store them in ite_vec */
  /*-----------------------------------------------------------------------*/
  ite_vec = array_alloc(ite_vertex *, 0);
  for (i = 0; i < array_n(order_list); i++) {
      fanin = array_fetch(node_t *, order_list, i);
      assert(st_lookup(table, node_long_name(fanin), (char **) &p));
      name = sm_get_col(F, p->col_num)->user_word;
      assert (strcmp(name, node_long_name(fanin)) == 0);
      assert(name == node_long_name(fanin));
      phase = (int) p->user_word;
      assert(phase != 2);
      ite_IF = ite_literal(p->col_num, name, fanin);
      if (phase) {
          ite = my_shannon_ite(ite_IF, ite_1, ite_0);
      } else {
          ite = my_shannon_ite(ite_IF, ite_0, ite_1);          
      }
      array_insert_last(ite_vertex *, ite_vec, ite);
  }
  /* OR the ite's together */
  /*-----------------------*/
  ite = ite_OR_itevec(ite_vec);
  array_free(ite_vec);
  node_free(n);
  array_free(order_list);
  st_free_table(table);
  return ite;
}
      
/*----------------------------------------------------------------
  Given an sm_matrix, construct a node with the same function as
  the matrix. Matrix is sum of cubes.
-----------------------------------------------------------------*/      
node_t *
act_make_node_from_matrix(F, network)
  sm_matrix *F;
  network_t *network;
{
  node_t *n, *n1, *f;
  sm_row *row;
  
  n = node_constant(0);
  sm_foreach_row(F, row) {
      f = act_make_node_from_row(row, F, network);
      n1 = node_or(n, f);
      node_free(n);
      node_free(f);
      n = n1;
  }
  return n;
}
