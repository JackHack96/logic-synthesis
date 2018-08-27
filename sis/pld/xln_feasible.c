/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/pld/xln_feasible.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:56 $
 *
 */

#include "pld_int.h"
#include "sis.h"

/*---------------------------------------------------------------------------
   This file carries routines that could make a non-feasible node feasible
   without increasing the number of nodes in the network. This is done by
   looking at a non-feasible node n, then trying to remove one of the fanins f
   of the node, by making f a fanin of node g (that is a fanin of node n)
   that is feasible and has some vacancy in it. This is done by collapsing g
   into n, then look for a decomposition (roth-karp) with bound set that
includes {fanins of g different from the ones of n} U {f}. If number of
equivalence classes is 2, you can do it!!! Hopefully, this routine will be
useful for "critical" nodes, i.e., with m + 1 inputs for CLB of m inputs. In
this file, it is done for each infeasible node till it becomes feasible, or we
run out of its fanins.

   Notes:

   1) Difference with Fujita's work: Here, we are changing the function of
   some other node of the network (g) too apart from n. Fujita just re-expresses
   n in terms of some other transitive fanins. The two methods are essentially
   non-overlapping and hence there should be cases covered in one, not covered
by the other.

   2) Partial collapse does similar stuff but this is a special case, and
   cube-packing may not be able to detect such a case.

   3) We are restricting ourselves by looking at only disjoint decomposition.

   4) Of course, we are ignoring the possibility of collapsing some other node
   into g later. Difficult to take into account.
-------------------------------------------------------------------------------*/

xln_network_reduce_infeasibility_by_moving_fanins(
    network, support, MAX_FANINS) network_t *network;
int support;
int MAX_FANINS; /* do not move fanins if number of fanins of node exceeds this
                 */
{
  array_t *nodevec;
  int i;
  node_t *node;

  nodevec = network_dfs(network);
  for (i = 0; i < array_n(nodevec); i++) {
    node = array_fetch(node_t *, nodevec, i);
    (void)xln_node_move_fanins(node, support, MAX_FANINS,
                               0); /* 0 => make it feasible */
  }
  array_free(nodevec);
}

int xln_node_move_fanins(node, support, MAX_FANINS, diff) node_t *node;
int support, MAX_FANINS;
int diff; /* if 0, then from infeasible to feasible, else just moving fanins as
             much as diff wants */
{
  int infeasibility;
  int improvement;
  int i, j, num_fanin;
  node_t *fanin, *f;
  array_t *faninvec;
  int bound_alphas;

  bound_alphas = 1; /* for area */

  if (node->type != INTERNAL)
    return 0;
  num_fanin = node_num_fanin(node);

  if (num_fanin > MAX_FANINS)
    return 0;
  if (diff == 0) {
    infeasibility = num_fanin - support;
    if (infeasibility <= 0)
      return 0;
  } else {
    infeasibility = diff;
  }

  /* make an array of fanin nodes */
  /*------------------------------*/
  faninvec = array_alloc(node_t *, 0);
  foreach_fanin(node, j, fanin) {
    array_insert_last(node_t *, faninvec, fanin);
  }
  /* try each f in faninvec for decreasing infeasibility */
  /*-----------------------------------------------------*/
  improvement = 0;
  for (i = 0; i < array_n(faninvec); i++) {
    f = array_fetch(node_t *, faninvec, i);
    improvement += xln_node_move_fanin(node, f, support, bound_alphas, AREA);
    if (improvement == infeasibility)
      break;
  }
  array_free(faninvec);
  return improvement;
}

/*-------------------------------------------------------------------
  Returns 1 if the fanin f of an infeasible node can be removed to
  one of the other fanins g. This decreases "infeasibility of the
  node by 1. Moves the f fanin. Right now, bound alphas can only
  be 1. Also, if DELAY mode, then do not select a critical node as g.
--------------------------------------------------------------------*/
int xln_node_move_fanin(node, f, support, bound_alphas, mode) node_t *node, *f;
int support;
int bound_alphas;
float mode;
{
  array_t *faninvec;
  node_t *g;
  int j, i;
  node_t *dup_node, *n, *node_simpl;
  array_t *Y;
  int *lambda_indices;
  array_t *nodevec;

  assert(bound_alphas == 1);
  /* get feasible internal fanins of node with vacant spot */
  /*-------------------------------------------------------*/
  faninvec = array_alloc(node_t *, 0);
  foreach_fanin(node, j, g) {
    if (g == f)
      continue;
    if (g->type == PRIMARY_INPUT)
      continue;
    if ((node_num_fanout(g) == 1) && (node_num_fanin(g) < support)) {
      /* in DELAY mode, do not want a critical g */
      /*-----------------------------------------*/
      if (mode == DELAY) {
        if (xln_is_node_critical(g, (double)0))
          continue;
      }
      array_insert_last(node_t *, faninvec, g);
    }
  }

  /* try collapsing each of the faninvec nodes g into node and
     redecomposing.                                           */
  /*----------------------------------------------------------*/
  for (i = 0; i < array_n(faninvec); i++) {
    dup_node = node_dup(node);
    FREE(dup_node->name);
    FREE(dup_node->short_name);
    network_add_node(node->network,
                     dup_node); /* to make roth-karp decomp work */
    g = array_fetch(node_t *, faninvec, i);
    assert(node_collapse(dup_node, g));
    node_simpl = node_simplify(dup_node, (node_t *)NULL,
                               NODE_SIM_ESPRESSO); /* Added Jul 1, 93 */
    node_replace(dup_node, node_simpl);
    if (XLN_DEBUG) {
      (void)printf("----------------------------------------\n");
      (void)printf("dup_node after collapse = ");
      node_print(sisout, dup_node);
      (void)printf("\n");
    }

    /* passing dup_node to xln_get_bound_Set, since it may happen that a
       node that was a fanin to g is not a fanin to dup_node. Now make sure
       that only those fanins of g are inserted in Y that are actually fanins
       of dup_node also -- April 23 1992                                   */
    /*---------------------------------------------------------------------*/
    Y = xln_get_bound_set(node, f, g, dup_node); /* node, not dup_node */
    if (array_n(Y) <= 1) {
      array_free(Y);
      network_delete_node(dup_node->network, dup_node);
      continue;
    }
    lambda_indices = xln_array_to_indices(Y, dup_node);
    /* try out this partition for decomposition */
    /*------------------------------------------*/
    nodevec = xln_k_decomp_node_with_array(dup_node, lambda_indices, array_n(Y),
                                           bound_alphas);
    FREE(lambda_indices);
    array_free(Y);
    network_delete_node(dup_node->network, dup_node);
    if (nodevec == NIL(array_t)) {
      continue;
    }
    assert(array_n(nodevec) <= (bound_alphas + 1));
    if (XLN_DEBUG) {
      (void)printf("node = ");
      node_print(sisout, node);
      (void)printf("\n");
      (void)printf("f = ");
      node_print(sisout, f);
      (void)printf("\n");
      (void)printf("g = ");
      node_print(sisout, g);
      (void)printf("\n");
      (void)printf("--------\n");
    }
    n = array_fetch(node_t *, nodevec, 0);
    if (XLN_DEBUG) {
      (void)printf("root node after decomp = ");
      node_print_rhs(sisout, n);
      (void)printf("\n");
    }
    node_replace(node, n);
    if (XLN_DEBUG) {
      (void)printf("root node after replace = ");
      node_print(sisout, node);
      (void)printf("\n");
    }
    n = array_fetch(node_t *, nodevec, 1);
    if (XLN_DEBUG) {
      (void)printf("g node after decomp = ");
      node_print_rhs(sisout, n);
      (void)printf("\n");
    }
    node_patch_fanin(node, n, g);
    node_replace(g, n);
    if (XLN_DEBUG) {
      (void)printf("g node after replace = ");
      node_print(sisout, g);
      (void)printf("\n");
    }
    array_free(nodevec);
    array_free(faninvec);
    return 1;
  }
  array_free(faninvec);
  return 0;
}

/*-----------------------------------------------------------------
  Add all those fanins of g into Y that aren't fanins of
  the node, but should be fanins of dup_node.
  Do add f in Y. Here, Y is the bound set to be used in
  roth-karp decomposition.
-------------------------------------------------------------------*/
array_t *xln_get_bound_set(node, f, g, dup_node) node_t *node, *f, *g,
    *dup_node;
{
  array_t *Y;
  int j;
  node_t *fanin;

  Y = array_alloc(node_t *, 0);
  array_insert_last(node_t *, Y, f);
  foreach_fanin(g, j, fanin) {
    if (node_get_fanin_index(node, fanin) < 0) {
      if (node_get_fanin_index(dup_node, fanin) >= 0) {
        array_insert_last(node_t *, Y, fanin);
      } else {
        if (XLN_DEBUG) {
          (void)printf("a fanin of g does not fanout to dup_node\n");
        }
      }
    } else {
      if (XLN_DEBUG) {
        (void)printf("there is a common fanin\n");
      }
    }
  }
  return Y;
}
