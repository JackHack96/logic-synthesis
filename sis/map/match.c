
/* file @(#)match.c	1.2 */
/* last modified on 5/1/91 at 15:51:35 */
#include "map_int.h"
#include "sis.h"

#define DEBUG

static int (*g_func)();

static prim_t *g_prim;
static int g_allow_internal_fanout;
static int g_fanout_limit;
static int g_debug;

/*
 *  Graph matching algorithm.
 */

static int match(net_node, list) register node_t *net_node;
register prim_edge_t *list;
{
  register int i, cont;
  register prim_node_t *prim_node;
  register node_t *matching_net_node;
  node_t *fanin, *fanout;
  int already_bound;
  lsGen gen;

  /* Fetch primitive node from the edge list */
  prim_node = list->this_node;

  /* See if the binding of the primitive node is consistent */
  already_bound = 0;
  if (MAP(net_node)->binding == NIL(prim_node_t)) {

    if (prim_node->binding != NIL(node_t)) {
      return 1;
    }

    /* Only makes sense if # inputs is the same */
    if (prim_node->type != PRIMARY_INPUT &&
        node_num_fanin(net_node) != prim_node->nfanin) {
      return 1;
    }

    /* Only makes sense if the # fanout is the same */
    if (g_allow_internal_fanout & 8) {
      if (prim_node->type == INTERNAL &&
          (list->next && list->next->this_node->type == INTERNAL) &&
          node_num_fanout(net_node) != prim_node->nfanout) {
        return 1;
      }
    } else if (!(g_allow_internal_fanout & 2)) {
      if (prim_node->type == INTERNAL &&
          node_num_fanout(net_node) != prim_node->nfanout) {
        return 1;
      }
    } else if (g_allow_internal_fanout & 2) {
      if (prim_node->type == INTERNAL &&
          node_num_fanout(net_node) > g_fanout_limit) {
        return 1;
      }
    }
  } else {
    if (MAP(net_node)->binding != prim_node) {
      return 1;
    }
    already_bound = 1;
    /* could assert that prim_node->binding == MAP(net_node) */
  }

  /* Bind the nodes to each other (if they were not already bound) */
  if (!already_bound) {
    prim_node->binding = net_node;
    MAP(net_node)->binding = prim_node;
  }

  if (g_debug) {
    (void)fprintf(sisout, "binding %s to %s\n", net_node->name,
                  prim_node->name);
  }

  /* Move to the next arc in the primitive graph */
  if ((list = list->next) == NIL(prim_edge_t)) {
    /* Exhausted all arcs, must mean there's a match */
    cont = (*g_func)(g_prim);

  } else {
    /* Attempt to match the matching graph node to all inputs and outputs */
    matching_net_node = list->connected_node->binding;

    if (list->dir == DIR_IN) {
      if (list->connected_node->isomorphic_sons) {

        /* recur for only the first unbound son */
        foreach_fanin(matching_net_node, i, fanin) {
          if (MAP(fanin)->binding == NIL(prim_node_t)) {
            cont = match(fanin, list);
            break;
          }
        }

      } else {

        /* recur for all inputs */
        foreach_fanin(matching_net_node, i, fanin) {
          cont = match(fanin, list);
          if (!cont) {
            break;
          }
        }
      }

    } else {
      foreach_fanout(matching_net_node, gen, fanout) {
        cont = match(fanout, list);
        if (!cont) {
          LS_ASSERT(lsFinish(gen));
          break;
        }
      }
    }
  }

  /* Unbind the nodes */
  if (!already_bound) {
    prim_node->binding = NIL(node_t);
    MAP(net_node)->binding = NIL(prim_node_t);
    if (g_debug) {
      (void)fprintf(sisout, "unbinding %s from %s\n", net_node->name,
                    prim_node->name);
    }
  }

  return cont;
}

void gen_all_matches(node, prim, func, debug, allow_internal_fanout,
                     fanout_limit) node_t *node;
prim_t *prim;
int (*func)();
int debug;
int allow_internal_fanout;
int fanout_limit;
{
  g_func = func;
  g_prim = prim;
  g_debug = debug;
  g_allow_internal_fanout = allow_internal_fanout;
  g_fanout_limit = fanout_limit;

  if (debug) {
    (void)fprintf(sisout, "entering gen_all_matches() ...\n");
  }

  (void)match(node, prim->edges);
}
