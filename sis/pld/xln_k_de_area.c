
#include "sis.h"
#include "pld_int.h"

/*------------------------------------------------------------------------
  For each node of the network, tries to decompose it using Roth-Karp 
  decomp. Uses all possible combinations if number of fanins of the node
  is <= MAX_FANINS. Then decomposes the resulting root node recursively
  if the RECURSIVE is non-zero.
  If the Roth-Karp decomposition fails (a disjoint decomp not possible),
  and DESPERATE is non-zero, then use xl_ao map to decompose the node.
--------------------------------------------------------------------------*/
xln_exhaustive_k_decomp_network(network, init_param
)
network_t        *network;
xln_init_param_t *init_param;
{
array_t   *nodevec;
int       i;
node_t    *node;
int       done;
network_t *net_decomp;

nodevec = network_dfs(network);
for (
i = 0;
i <
array_n(nodevec);
i++) {
node = array_fetch(node_t * , nodevec, i);
if (node->type != INTERNAL) continue;

done = 0;
while (!done) {
net_decomp = xln_exhaustive_k_decomp_node(node, init_param->support,
                                          init_param->MAX_FANINS_K_DECOMP);
if (net_decomp ==
NIL (network_t)
) {
done = 1;
continue;
}
pld_replace_node_by_network(node, net_decomp
);
network_free(net_decomp);
if (!init_param->RECURSIVE)
done = 1;
}
/* final resort */
/*--------------*/
if ((
node_num_fanin(node)
> init_param->support)) {
xln_node_ao_map(node, init_param
->support);
}
}
array_free(nodevec);
}


/*------------------------------------------------------------------
  Call Roth Karp decomp on a node of the network. Try all possible
  input partitions for the node if the number of fanins of the node
  is not too high: i.e., no more than MAX_FANINS. If higher, 
  generate just one partition. Returns an array of nodes as the 
  decomposition. The selection of the best node is found by a
  decomposition of the new root node by xl_ao (a fast estimate). 
  This decomposes just once, but it can be called recursively, if
  wanted from the calling routine.
-------------------------------------------------------------------*/

network_t *
xln_exhaustive_k_decomp_node(node, support, MAX_FANINS)
        node_t *node;
        int support;
        int MAX_FANINS;
{
    int    best_cost;
    int    cost;
    int    num_fanin;
    int    *lambda_indices;
    int    num_comb, i;
    node_t *po, *node_root;
    network_t * net_new, *net_best;
    array_t *Y, *Z;

    if (node->type != INTERNAL) return NIL(network_t);
    num_fanin = node_num_fanin(node);
    if (num_fanin <= support) return NIL(network_t);
    /* somehow generate a good combination */
    /*-------------------------------------*/
    if (num_fanin > MAX_FANINS) {
        return NIL(network_t); /* for the time being */
    }

    /* generate all the partitions, one by one, and ask for a decomposition */
    /*----------------------------------------------------------------------*/
    best_cost = HICOST;
    net_best  = NIL(network_t);
    /* num_comb = (int) (pow ((double) 2, (double) num_fanin)); */
    num_comb  = 1 << num_fanin;
    for (i    = 0; i < num_comb; i++) {
        /* can I ignore this partition from the bound info. calculated earlier */
        /*---------------------------------------------------------------------*/
        if ((i == 1) && (num_fanin > MAX_FANINS)) break;
        Y = array_alloc(node_t * , 0);
        Z = array_alloc(node_t * , 0);
        xln_generate_fanin_combination(node, i, Y, Z);

        /* check if the partition is non-trivial and also for feasibility of Y */
        /*---------------------------------------------------------------------*/
        if ((array_n(Y) <= 1) || (array_n(Y) > support) || (array_n(Z) == 0)) {
            array_free(Y);
            array_free(Z);
            continue;
        }
        lambda_indices = xln_array_to_indices(Y, node);
        net_new        = xln_k_decomp_node_with_network(node, lambda_indices, array_n(Y));
        FREE(lambda_indices);
        if (net_new == NIL(network_t)) {
            array_free(Y);
            array_free(Z);
            continue;
        }
        /* calculate the cost of this decomposition - root node's cost
           estimated by xl_node_ao_map                                */
        /*------------------------------------------------------------*/
        po        = network_get_po(net_new, 0);
        node_root = node_get_fanin(po, 0);
        xln_node_ao_map(node_root, support);
        cost = network_num_internal(net_new);
        if (cost < best_cost) {
            best_cost = cost;
            /* free the best_nodevec, along with all the nodes */
            /*-------------------------------------------------*/
            if (net_best != NIL(network_t)) {
                network_free(net_best);
            }
            net_best = net_new;
        }
        array_free(Y);
        array_free(Z);
    }
    return net_best;
}

