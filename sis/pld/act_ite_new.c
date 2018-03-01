#include "sis.h"
#include "pld_int.h"  
#include "ite_int.h"

static void my_ite_OR();
static ite_vertex *ite_1, *ite_0;

ite_vertex *
ite_new_ite_for_cubenode(node)
  node_t *node;
{
  array_t *order_list;
  int i;
  node_t *fanin;
  ite_vertex *vertex;
  input_phase_t phase;

  order_list = single_cube_order(node);
  assert(st_lookup(ite_end_table, (char *)1, (char **) &ite_1));
  assert(st_lookup(ite_end_table, (char *)0, (char **) &ite_0));
  vertex = NIL (ite_vertex);
  /* reverse order, because of the way, my_ite_and works */
  /*-----------------------------------------------------*/
  for (i = array_n(order_list) - 1; i >= 0; i--) {
      fanin = array_fetch(node_t *, order_list, i);
      phase = node_input_phase(node, fanin);
      vertex = ite_new_ite_and(vertex, fanin, phase);
  }
  array_free(order_list);
  vertex->index_size = node_num_fanin(node);
  return vertex;
}

ite_vertex *
ite_new_ite_for_single_literal_cubes(node)
  node_t *node;
{

  array_t *order_list;
  int i;
  input_phase_t phase;
  node_t *fanin;
  ite_vertex *vertex;

  order_list = OR_literal_order(node);
  assert(st_lookup(ite_end_table, (char *)1, (char **) &ite_1));
  assert(st_lookup(ite_end_table, (char *)0, (char **) &ite_0));
  vertex = NIL (ite_vertex);
  /* reverse order, because of the way, my_ite_and works */
  /*-----------------------------------------------------*/
  for (i = array_n(order_list) - 1; i >= 0; i--) {
      fanin = array_fetch(node_t *, order_list, i);
      phase = node_input_phase(node, fanin);
      vertex = ite_new_ite_or(vertex, fanin, phase);
  }
  array_free(order_list);
  vertex->index_size = node_num_fanin(node);
  return vertex;
}  

/* And variables in a cube*/
ite_vertex *
ite_new_ite_and(vertex, fanin, phase)
ite_vertex *vertex;
node_t *fanin;
input_phase_t phase;
{
    ite_vertex *vertex_IF, *p_vertex;

    vertex_IF = ite_new_literal(fanin);
    
    if(vertex == NIL (ite_vertex)) {
	switch(phase) {
	  case POS_UNATE:
            p_vertex = my_shannon_ite(vertex_IF, ite_1, ite_0);
	    break;
	  case NEG_UNATE:
	    p_vertex = my_shannon_ite(vertex_IF, ite_0, ite_1);
            break;
	  case BINATE:;
          case PHASE_UNKNOWN:
	    (void)fprintf(misout, "error in new_my_ite_and():phase is binate or unknown\n");
            exit(1);
	}
    } else {
	switch(phase) {
	  case POS_UNATE:
            p_vertex = my_shannon_ite(vertex_IF, vertex, ite_0);
	    break;
	  case NEG_UNATE:
            p_vertex = my_shannon_ite(vertex_IF, ite_0, vertex);
	    break;
	  case BINATE:;
          case PHASE_UNKNOWN:
	    (void)fprintf(misout, "error in new_my_and act, phase is binate or unknown\n");
	    exit(1);
	}
    }
    return p_vertex;
}

/* Or variables */
ite_vertex *
ite_new_ite_or(vertex, fanin, phase)
ite_vertex *vertex;
node_t *fanin;
input_phase_t phase;
{
    ite_vertex *vertex_IF, *p_vertex;

    vertex_IF = ite_new_literal(fanin);
    
    if(vertex == NIL (ite_vertex)) {
	switch(phase) {
	  case POS_UNATE:
            p_vertex = my_shannon_ite(vertex_IF, ite_1, ite_0);
	    break;
	  case NEG_UNATE:
	    p_vertex = my_shannon_ite(vertex_IF, ite_0, ite_1);
            break;
	  case BINATE:;
          case PHASE_UNKNOWN:
	    (void)fprintf(misout, "error in new_my_ite_or():phase is binate or unknown\n");
            exit(1);
	}
    } else {
	switch(phase) {
	  case POS_UNATE:
            p_vertex = my_shannon_ite(vertex_IF, ite_1, vertex);
	    break;
	  case NEG_UNATE:
            p_vertex = my_shannon_ite(vertex_IF, vertex, ite_1);
	    break;
	  case BINATE:;
          case PHASE_UNKNOWN:
	    (void)fprintf(misout, "error in new_my_or act, phase is binate or unknown\n");
	    exit(1);
	}
    }
    return p_vertex;
}

/*-----------------------------------
  Makes an ite vertex for the fanin.
-----------------------------------*/
ite_vertex *
ite_new_literal(fanin)
  node_t *fanin;
{
  ite_vertex *ite;
  	
  ite = ite_alloc();
  ite->value = 2;
  ite->phase = 1;
  /* ite->index = b_col; */
  ite->index_size = 1;
  ite->name = node_long_name(fanin);
  ite->fanin = fanin;
  return ite;
}

