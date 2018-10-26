
#include "sis.h"
#include "pld_int.h"

/*---------------------------------------------------------------------------
   This file carries routines that move fanins f around of a node n
   that are not critical. f is made a fanin of g (a fanin of n) that 
   is feasible and has some vacancy in it. This is done by collapsing g
   into n, then look for a decomposition (roth-karp) with bound set that includes
   {fanins of g different from the ones of n} U {f}. If number of equivalence 
   classes is 2, you can do it!!! g has to be a single fanout node, not a PI.
   
   Notes:
   1) Assumes that a delay trace has been done on the network.

   2) We are restricting ourselves by looking at only disjoint decomposition.

   3) Of course, we are ignoring the possibility of collapsing some other node
   into g later. Difficult to take into account.
-------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------
  Moves fanins of the critical nodes as much as possible, ie, tries to have 
  as few fanins there as possible. If init_delay_trace is 1, does a delay trace
  first.
---------------------------------------------------------------------------------*/
xln_network_move_fanins_for_delay(network, init_param, init_delay_trace)
  network_t *network;
  xln_init_param_t *init_param;
  int init_delay_trace;
{
  array_t *nodevec;
  int i;
  node_t *node;
  int support;
  int MAX_FANINS; /* do not move fanins if number of fanins of node exceeds this */
  int bound_alphas;

  if (init_param->xln_move_struct.MOVE_FANINS == 0) return;

  support = init_param->support;
  MAX_FANINS = init_param->xln_move_struct.MAX_FANINS;
  bound_alphas = init_param->xln_move_struct.bound_alphas;

  nodevec = network_dfs(network);
  if (init_delay_trace) assert(delay_trace(network, DELAY_MODEL_UNIT));  
  for (i = 0; i < array_n(nodevec); i++) {
      node = array_fetch(node_t *, nodevec, i);
      if (!xln_is_node_critical(node, (double) 0)) continue;
      /* move as much as possible - takes care that a non-critical fanin does not
         become over-critical */
      /*----------------------------------------------------------------------------*/
      (void) xln_node_move_fanins_for_delay(node, support, MAX_FANINS, 
                                            MAXINT, bound_alphas);
  }
  array_free(nodevec);
}

int
xln_node_move_fanins_for_delay(node, support, MAX_FANINS, diff, bound_alphas)
  node_t *node;
  int support, MAX_FANINS;
  int diff; /* if 0, then from infeasible to feasible, else just moving fanins as
               much as diff wants */
  int bound_alphas;
{
  int infeasibility;
  int improvement;
  int i, num_fanin;
  node_t *f;
  array_t *faninvec;
  int moved;

  if (node->type != INTERNAL) return 0;
  num_fanin = node_num_fanin(node);

  if (num_fanin > MAX_FANINS) return 0;
  if (diff == 0) {
      infeasibility = num_fanin - support;
      if (infeasibility <= 0) return 0;
  } else {
      infeasibility = diff;
  }

  faninvec = pld_get_array_of_fanins(node);

  /* try each f in faninvec for moving */
  /*-----------------------------------*/
  improvement = 0;
  for (i = 0; i < array_n(faninvec); i++) {
      f = array_fetch(node_t *, faninvec, i);
      if (xln_is_node_critical(f, (double) 0)) continue;
      moved = xln_node_move_fanin(node, f, support, bound_alphas, DELAY);
      if (moved) {
          /* need this, since a non-critical node may become critical */
          /*----------------------------------------------------------*/
          assert(delay_trace(node->network, DELAY_MODEL_UNIT));
          improvement++;
      }
      if (improvement == infeasibility) break;
  }
  array_free(faninvec);
  return improvement;
}


