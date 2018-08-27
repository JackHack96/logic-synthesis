
#ifdef SIS
#include "retime_int.h"
#include "sis.h"

/*
 * Compute the cycle time for the retime graph ----
 * 	Account for the user specified constraints and delay throught latches
 */
double re_cycle_delay(graph, delay_r) re_graph *graph;
double delay_r;
{
  re_node *node;
  bool *valid_table;
  int i, n = re_num_nodes(graph);
  double crit, d, offset, *delay_table;

  valid_table = ALLOC(bool, n);
  delay_table = ALLOC(double, n);

  for (i = n; i-- > 0;) {
    valid_table[i] = FALSE;
    delay_table[i] = 0.0;
  }

  /* primary input and primary output nodes need not be evaluated */
  re_foreach_node(graph, i, node) {
    if (re_num_fanins(node) == 0) {
      valid_table[i] = TRUE;
      if (node->user_time > RETIME_TEST_NOT_SET) {
        delay_table[i] = node->user_time;
      }
    }
  }

  /* compute delays */
  for (i = n; i-- > 0;) {
    if (!valid_table[i]) {
      node = array_fetch(re_node *, graph->nodes, i);
      re_evaluate_delay(node, valid_table, delay_table);
    }
  }

  /* find critical delay */
  crit = 0.0;
  for (i = n; i-- > 0;) {
    d = delay_table[i];
    node = array_fetch(re_node *, graph->nodes, i);

    if (node->type == RE_PRIMARY_OUTPUT &&
        node->user_time > RETIME_TEST_NOT_SET) {
      /* The po signal is required early */
      offset = 0.0 - (node->user_time);
    } else if (re_max_fanout_weight(node) > 0) {
      /* Add the delay delay thru the latch that follows */
      offset = delay_r;
    } else {
      offset = 0.0;
    }

    crit = MAX(crit, d + offset);
  }
  FREE(valid_table);
  FREE(delay_table);

  return crit;
}

/*
 * For the specified "node" compute the arrival time at its output
 * The entries in the delay_table and the valid table are updated
 * after recording the delay at the output of the node
 */
void re_evaluate_delay(node, valid_table, delay_table) re_node *node;
bool *valid_table;
double *delay_table;
{
  int i;
  double m;
  re_edge *edge;

  re_foreach_fanin(node, i, edge) {
    if ((edge->weight == 0) && (!valid_table[edge->source->id])) {
      /* recursively evaluate the fanins */
      re_evaluate_delay(edge->source, valid_table, delay_table);
    }
  }

  m = 0.0;
  re_foreach_fanin(node, i, edge) {
    if (edge->weight == 0) {
      m = MAX(m, delay_table[edge->source->id]);
    }
  }
  valid_table[node->id] = TRUE;
  delay_table[node->id] = m + node->final_delay;
}
/*
 * Find a lower bound on the cycle time.
 * THis is actually MAX(cycle delay/ cycle weight)...
 * However for now we will just take the largest delay of a gate
 */
double retime_cycle_lower_bound(graph) re_graph *graph;
{
  int i;
  re_node *node;
  double bound;

  bound = -1.0;
  re_foreach_node(graph, i, node) {
    if (node->type == RE_INTERNAL) {
      bound = MAX(bound, node->final_delay);
    }
  }
  return bound;
}
#endif /* SIS */
