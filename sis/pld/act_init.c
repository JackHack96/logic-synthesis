/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/pld/act_init.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:55 $
 *
 */
#include "pld_int.h"
#include "sis.h"

/*	the init and end routines for the ACT package
 */

/*extern void p_transitiveFree();*/

end_p_act() {
  int i;
  array_t *list;
  for (i = 0; i < array_n(global_lists); i++) {
    list = array_fetch(array_t *, global_lists, i);
    array_free(list);
  }
  array_free(global_lists);
}

/*	install all the node daemons
 */

void p_actAlloc(node) node_t *node;
{
  node->PLD_SLOT = (char *)ALLOC(act_type_t, 1);
  ACT_SET(node)->LOCAL_ACT = NIL(ACT_ENTRY);
  ACT_SET(node)->GLOBAL_ACT = NIL(ACT_ENTRY);
}

void local_actFree(node) node_t *node;
{

  if (ACT_SET(node) == NIL(act_type_t))
    return;
  if (ACT_SET(node)->LOCAL_ACT != NIL(ACT_ENTRY)) {
    p_actDestroy(&ACT_SET(node)->LOCAL_ACT, 1);
    ACT_SET(node)->LOCAL_ACT = NIL(ACT_ENTRY);
  }
  if (ACT_SET(node)->GLOBAL_ACT != NIL(ACT_ENTRY)) {
    p_actDestroy(&ACT_SET(node)->GLOBAL_ACT, 0);
    ACT_SET(node)->GLOBAL_ACT = NIL(ACT_ENTRY);
  }
}

void p_actFree(node) node_t *node;
{
  act_type_t *a;

  a = (act_type_t *)ACT_GET(node);
  FREE(a);
}

/* ARGSUSED */
void p_actDup(old, new) node_t *old, new;
{ /*	ACTs are NOT copied on duplication	*/ }

p_transitiveFree(node) node_t *node;
{
  lsGen gen;
  node_t *fanout;

  if (ACT_SET(node)->GLOBAL_ACT != NIL(ACT_ENTRY)) {
    p_actDestroy(&ACT_SET(node)->GLOBAL_ACT, 0);
    ACT_SET(node)->GLOBAL_ACT = NIL(ACT_ENTRY);
    if (node_network(node) != NIL(network_t)) {
      foreach_fanout(node, gen, fanout) { p_transitiveFree(fanout); }
    }
  }
}

/* instead of node */
/*ARGSUSED */
my_free_act(vertex) ACT_VERTEX_PTR vertex;
{
  st_table *table;
  char *arg;
  enum st_retval free_table_entry_improper();

  if (vertex == NIL(ACT_VERTEX))
    return;
  table = st_init_table(st_ptrcmp, st_ptrhash);
  delete_act(vertex, table);
  st_foreach(table, free_table_entry_improper, arg);
  st_free_table(table);
}

/* ARGSUSED */
enum st_retval free_table_entry_improper(key, value, arg) char *key, *value,
    *arg;
{
  ACT_VERTEX_PTR vertex;
  vertex = (ACT_VERTEX_PTR)key;
  FREE(vertex);
  return ST_DELETE;
}

delete_act(vertex, table) act_t *vertex;
st_table *table;
{
  char *dummy;

  if (!st_lookup(table, (char *)vertex, &dummy)) {
    if (vertex->value == 4) {
      delete_act(vertex->low, table);
      delete_act(vertex->high, table);
    }
    (void)st_insert(table, (char *)vertex, NIL(char));
  }
}
