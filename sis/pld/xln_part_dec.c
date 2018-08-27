/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/pld/xln_part_dec.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:57 $
 *
 */

/* fixed the bug which resulted because of the way kernel,
   cokernel and remainder were substituted in the node.
   They were substituted in the negative phase too earlier.
                  June 21, 93. */

/* changes made on cost function and the way split works:
   now it finds nonfeasible kernels also. Feb 13, 91 ---- Rajeev
*/
#include "pld_int.h"
#include "sis.h"

int split_node();
static void find_best_div();
static int kernel_value();

split_network(network, size) network_t *network;
int size;
{
  array_t *order;
  node_t *node;
  int i;
  char *err;

  order = network_dfs(network);
  for (i = 0; i < array_n(order); i++) {
    node = array_fetch(node_t *, order, i);
    if (node_num_fanin(node) > size) {
      (void)split_node(network, node, size);
      error_init();
      if (!network_check(network)) {
        err = error_string();
        (void)fprintf(sisout, "%s", err);
      }
    }
  }
  array_free(order);
  (void)network_sweep(network);
}

int split_node(network, node, size) network_t *network;
node_t *node;
int size;
{
  int i, value = 1;
  kern_node *sorted;
  divisor_t *div, *best_div;
  node_t *best, *co_best, *rem, *t_node;
  node_t *p, *q, *r, *pq, *new_node;

  if (node_num_fanin(node) <= size)
    return 0;

  /* First try finding kernels */
  if ((t_node = ex_find_divisor_quick(node)) != NIL(node_t)) {
    sorted = ALLOC(kern_node, 1);
    sorted->node = node;
    sorted->cost_array = array_alloc(divisor_t *, 0);
    sorted->size = size;
    ex_kernel_gen(node, kernel_value, (char *)sorted);
    ex_subkernel_gen(node, kernel_value, 0, (char *)sorted);
    best_div = ALLOC(divisor_t, 1);
    best_div->divisor = NIL(node_t);
    best_div->kernelsize = HICOST;

    for (i = 0; i < array_n(sorted->cost_array); i++) {
      div = array_fetch(divisor_t *, sorted->cost_array, i);
      (void)find_best_div(div, best_div, size);
    }
    /* (void) printf("\n\n"); */

    array_free(sorted->cost_array);
    FREE(sorted);

    if (best_div->divisor != NIL(node_t)) {
      best = best_div->divisor;
      FREE(best_div);
      if (XLN_DEBUG) {
        (void)fprintf(sisout, "Node %s has been added to the network\n",
                      node_name(best));
        (void)node_print(sisout, best);
      }
      co_best = node_div(node, best, &rem);
      if (XLN_DEBUG) {
        (void)fprintf(sisout, "cokernel");
        node_print(sisout, co_best);
        (void)fprintf(sisout, "remainder");
        node_print(sisout, rem);
      }
      network_add_node(network, best);
      network_add_node(network, co_best);
      network_add_node(network, rem);
      p = node_literal(best, 1);
      q = node_literal(co_best, 1);
      r = node_literal(rem, 1);
      pq = node_and(p, q);
      new_node = node_or(pq, r);
      node_free(p);
      node_free(q);
      node_free(pq);
      node_free(r);
      node_replace(node, new_node);
      (void)split_node(network, rem, size);
      (void)split_node(network, co_best, size);
      (void)split_node(network, best, size);
      (void)split_node(network, node, size);

      if (XLN_DEBUG) {
        node_print(sisout, node);
      }
    } else {
      node_free(best_div->divisor);
      FREE(best_div);
      value = pld_decomp_and_or(network, node, size);
    }
  } else {
    if (!pld_decomp_and_or(network, node, size)) {
      (void)fprintf(sisout, "No and_or decomposition possible\n");
      value = 0;
    }
  }
  node_free(t_node);
  return (value);
}

static int kernel_value(kernel, cokernel, state) node_t *kernel;
node_t *cokernel;
char *state;
{
  divisor_t *k_data, *c_data;
  kern_node *store;
  node_t *rem, *new_cokernel;
  int size;
  int num_nc, num_k, num_node, num_rem, cost;
  /* int infeasible_nc, infeasible_k, infeasible_rem, total_infeasibility; */

  store = (kern_node *)state;
  size = store->size;
  new_cokernel = node_div(store->node, kernel, &rem);

  num_node = node_num_fanin(store->node);
  num_nc = node_num_fanin(new_cokernel);
  num_k = node_num_fanin(kernel);
  num_rem = node_num_fanin(rem);
  cost = num_nc + num_k + num_rem;

  /* do infeasibility study */
  /*------------------------*/
  /* infeasible_nc = xln_infeasibility_measure(new_cokernel, size);
  infeasible_k = xln_infeasibility_measure(kernel, size);
  infeasible_rem = xln_infeasibility_measure(rem, size); */

  /* change the cost by feasibility number */
  /*---------------------------------------*/
  /* total_infeasibility = infeasible_nc + infeasible_k + infeasible_rem; */

  /* if all three nodes feasible, accept it definitely */
  /*---------------------------------------------------*/
  if ((num_nc == size) && (num_k == size) && (num_rem == size))
    cost = -HICOST;
  /* else cost += total_infeasibility; */

  /* if((node_num_fanin(new_cokernel) <= size)&&(node_num_fanin(new_cokernel) >
   * 1)){ */
  if ((num_nc > 1) && (num_nc < num_node)) {
    c_data = ALLOC(divisor_t, 1);
    c_data->divisor = new_cokernel;
    /* c_data->kernelsize = 2 * composite_fanin(rem, kernel) +
      node_num_fanin(new_cokernel); */
    c_data->kernelsize = cost;
    array_insert_last(divisor_t *, store->cost_array, c_data);
  }

  /* if((node_num_fanin(kernel) <= size)&&(node_num_fanin(kernel) > 1)){ */
  if ((num_k > 1) && (num_k < num_node)) {
    k_data = ALLOC(divisor_t, 1);
    k_data->divisor = kernel;
    /* k_data->kernelsize = 2 * composite_fanin(rem ,new_cokernel) +
      node_num_fanin(kernel); */
    k_data->kernelsize = cost;
    array_insert_last(divisor_t *, store->cost_array, k_data);
  }

  node_free(rem);
  node_free(cokernel);
}

static void find_best_div(div, best_div, size) divisor_t *div, *best_div;
int size;
{
  /* (void) printf("%d ", div->kernelsize); */
  if (div->kernelsize < best_div->kernelsize) {
    if (best_div->divisor != NIL(node_t)) {
      node_free(best_div->divisor);
    }
    best_div->divisor = node_dup(div->divisor);
    best_div->kernelsize = div->kernelsize;
  }
  node_free(div->divisor);
  FREE(div);
}

xln_infeasibility_measure(node, size) node_t *node;
int size;
{
  int num_lit, num_fanin, diff;

  num_fanin = node_num_fanin(node);
  if (num_fanin <= size)
    return 0; /* node feasible */
  /* return a feasibility number reflecting diff. between num_fanin and size
     and also the number of literals                                        */
  /*------------------------------------------------------------------------*/
  num_lit = node_num_literal(node);
  diff = num_fanin - size;
  if (diff == 1)
    return 1;
  if (diff <= 4) {
    if (num_lit <= 25)
      return 2;
    if (num_lit <= 50)
      return 3;
    if (num_lit <= 100)
      return 5;
  }
  return 6;
}
