
#include "sis.h"
#include "../include/pld_int.h"
#include "math.h"

extern node_t *pld_remap_get_node();

extern network_t *xln_best_script();

/*--------------------------------------------------------------------------
  If the network is small (number of literals in SOP less than lit_limit), 
  does not do anything. Else, looks at each node and its sop and fac forms, 
  and if num_fac_literals < alpha * num_sop_literals, decomposes the node in
  place. alpha is typically less than 1.
----------------------------------------------------------------------------*/
xln_selective_good_decomp(network, lit_limit, alpha
)
network_t *network;
int       lit_limit;
float     alpha;
{
int     num_lit_sop, num_lit_fac, lit_sop_total;
int     i, num;
lsGen   gen;
array_t *nodevec;
node_t  *node;

nodevec       = network_dfs(network);
num           = array_n(nodevec);
lit_sop_total = 0;
foreach_node(network, gen, node
) {
lit_sop_total +=
node_num_literal(node);
}
if (lit_sop_total < lit_limit) return; /* small network */
for (
i = 0;
i<num;
i++) {
node        = array_fetch(node_t * , nodevec, i);
num_lit_sop = node_num_literal(node);
num_lit_fac = factor_num_literal(node);
if (num_lit_fac <
num_lit_sop *alpha
)
decomp_good_node(network, node
);
}
array_free(nodevec);
}

xln_improve_network(network, init_param
)
network_t        *network;
xln_init_param_t *init_param;

{
array_t *nodevec;
int     i, num;
node_t  *node;

nodevec = network_dfs(network);
num     = array_n(nodevec);
for (
i = 0;
i<num;
i++) {
node = array_fetch(node_t * , nodevec, i);
(void)
xln_improve_node(node, init_param
);
}
array_free(nodevec);
}

/*---------------------------------------------------------
  If the node is infeasible, then it replaces the node by 
  network. Then it applies the best script on the node, and 
  gets a good representation of that. Replaces the node by
  the new network.
----------------------------------------------------------*/
xln_improve_node(node, init_param
)
node_t           *node;
xln_init_param_t *init_param;
{
node_t    *dup_node;
network_t *network1;

if (node->type != INTERNAL) return 0;
if (
node_num_fanin(node)
<= init_param->support) return 0;

/* added Aug 24 93 - for nodes which may not be on minimum support - then
   node_replace_by_network() can die                                     */
/*-----------------------------------------------------------------------*/

simplify_node(node, SIM_METHOD_SNOCOMP, SIM_DCTYPE_NONE, SIM_FILTER_NONE,
                    SIM_ACCEPT_SOP_LITS
);

dup_node = node_dup(node);
if (XLN_DEBUG > 0) {
(void) printf("node %s[%s]\n",
node_long_name(node), node_name(node)
);
node_print(siserr, node
);
}

/* run best script on network from node */
/*--------------------------------------*/
network1 = xln_best_script(dup_node, init_param);

if (XLN_DEBUG)
(void) printf("\t---replacing node %s by %d feasible nodes\n\n",
node_long_name(node), network_num_internal(network1)
);

/* replace the node in the original network by the network1 */
/*----------------------------------------------------------*/
pld_replace_node_by_network(node, network1
);
network_free(network1);
node_free   (dup_node);
return 1;
}

/*---------------------------------------------------------------------------
  Take in a node, make a network out of it and return a good feasible network
  out of it. cover_node_limit is needed for deciding what options to use 
  while generating the network.
-----------------------------------------------------------------------------*/
network_t *
xln_best_script(node, init_param)
        node_t *node;
        xln_init_param_t *init_param;
{
    network_t *network1, *network2;
    int       num_fanin;
    int       num_lit;
    int       diff1;
    double    upper_bound;

    num_fanin = node_num_fanin(node);
    if (num_fanin <= init_param->support) {
        network1 = network_create_from_node(node);
        return network1;
    }
    num_lit   = node_num_literal(node);
    /* good ao decomposition from feasibility point of view */
    /*------------------------------------------------------*/
    network1  = network_create_from_node(node);

    if (num_lit == num_fanin) {
        xln_network_ao_map(network1, init_param->support);
        if (XLN_DEBUG > 1) (void) printf("the best possible??\n");
        return network1;
    }

    if (init_param->flag_decomp_good == 1) {
        decomp_good_network(network1); /* added later with return network1 */
        if (init_param->absorb) {
            xln_network_reduce_infeasibility_by_moving_fanins(network1,
                                                              init_param->support, 15);
        }
    }
    xln_network_ao_map(network1, init_param->support);
    if (XLN_DEBUG > 2)
        (void) printf("\tafter ao map %d nodes, ",
                      network_num_internal(network1));
    if (network_num_internal(network1) == 2) {
        if (XLN_DEBUG > 1) (void) printf("the best possible??\n");
        return network1; /* not seen any counterexamples so far */
    }
    xln_cover_or_partition(network1, init_param);
    if (init_param->flag_decomp_good == 2) {
        network2 = network_create_from_node(node);
        decomp_good_network(network2);
        if (network_num_internal(network2) == 1) {
            network_free(network2);
        } else {
            if (init_param->absorb) {
                xln_network_reduce_infeasibility_by_moving_fanins(network2,
                                                                  init_param->support, 15);
            }
            xln_network_ao_map(network2, init_param->support);
            if (XLN_DEBUG > 2)
                (void) printf("\tafter good decomp & ao map %d nodes, ",
                              network_num_internal(network2));
            xln_cover_or_partition(network2, init_param);
            if (network_num_internal(network2) < network_num_internal(network1)) {
                network_free(network1);
                network1 = network2;
            } else {
                network_free(network2);
            }
        }
    }

    /* make a check based on cofactor decomposition (extend it for support
       from 6 to 10)                                                      */
    /*--------------------------------------------------------------------*/
    if ((init_param->support > 2) && (init_param->support <= 5)) {
        diff1 = num_fanin - init_param->support + 1;
        /* make it more efficient */
        /*------------------------*/
        if (diff1 <= 31) {
            upper_bound = (double) ((1 << diff1) - 1);
        } else {
            upper_bound = (pow((double) 2, (double) diff1)) - 1.0;
        }
        if (network_num_internal(network1) > upper_bound) {
            network_free(network1);
            network1 = xln_cofactor_decomp(node, init_param->support, AREA); /* for AREA */
            if (XLN_DEBUG > 2)
                (void) printf("\tafter cofactor %d nodes, ",
                              network_num_internal(network1));
        }
    }

    if (init_param->good_or_fast == FAST) return network1;
    xln_try_other_mapping_options(node, &network1, init_param);
    return network1;
}

/*-----------------------------------------------------------
  If number of nodes in network not large, then do an exact
  cover. Else, use partition command.
-------------------------------------------------------------*/
xln_cover_or_partition(network, init_param
)
network_t        *network;
xln_init_param_t *init_param;
{
(void)
xln_do_trivial_collapse_network(network, init_param
->support,
init_param->xln_move_struct.MOVE_FANINS,
init_param->xln_move_struct.MAX_FANINS);
/* use exact xl_cover routine */
/*----------------------------*/
if (
network_num_internal(network)
<= init_param->cover_node_limit) {
partition_network(network, init_param
->support, 0);
if (XLN_DEBUG > 2) {
(void) printf("using trivial collapse & xl_cover exact ");
(void) printf("=> %d feasible nodes\n",
network_num_internal(network)
);
}

}
if (XLN_DEBUG > 3)
xln_network_print(network);
}


xln_network_print(network)
network_t *network;
{
int     i, num;
node_t  *node;
array_t *nodevec;

nodevec = network_dfs(network);
num     = array_n(nodevec);
for (
i = 0;
i<num;
i++) {
node = array_fetch(node_t * , nodevec, i);
node_print(siserr, node
);
}
array_free(nodevec);
}

/*-----------------------------------------------------------------------
  Tries other decomposition methods: disjoint decomposition, roth-karp
  decomp, tech. decomposition. Replaces pnetwork with the lower cost one.
-------------------------------------------------------------------------*/
xln_try_other_mapping_options(node, pnetwork, init_param
)

node_t           *node;
network_t        **pnetwork;
xln_init_param_t *init_param;
{
network_t *network1, *network2, *network3, *network4 /* , *network5 */;
int       num1, num2, num3, num4 /* , num5 */;
int       num_lit, flag;

network1 = *pnetwork;
num_lit  = node_num_literal(node);

/* in second case, we already have probably the best network */
/*-----------------------------------------------------------*/
num1 = network_num_internal(network1);
if ((num1 <= 2) || (
node_num_fanin(node)
== num_lit)) return;

/* splitting and covering */
/*------------------------*/
flag     = 0;
network4 = network_create_from_node(node);
if (num_lit > init_param->lit_bound || init_param->flag_decomp_good)
decomp_good_network(network4);
if (num_lit > init_param->lit_bound) {
num4 = network_num_internal(network4);
if (num4 > 1) {
flag = 1;
if (XLN_BEST)
xln_improve_network(network4, init_param
);
}
if (XLN_DEBUG > 2) (void) printf("\tdone good decomp, num nodes = %d\n",
network_num_internal(network4)
);
}
split_network(network4, init_param
->support);
if (XLN_DEBUG > 2) (void) printf("\tafter splitting %d nodes, ",
network_num_internal(network4)
);
xln_cover_or_partition(network4, init_param
);

num4 = network_num_internal(network4);
if (num4 == 2) {
network_free(network1);
*
pnetwork = network4;
return;
}

if (num4 < num1) {
network_free(network1);
network1 = network4;
num1     = num4;
}
else {
network_free(network4);
}

network3 = network_create_from_node(node);
if (init_param->flag_decomp_good)
decomp_good_network(network3);
decomp_disj_network(network3);
num3 = network_num_internal(network3);
if (XLN_DEBUG > 2) (void) printf("\tafter disj decomp %d nodes, ", num3);

/* accept disj decomp only if good decomp was done or num nodes > 1*/
/*-----------------------------------------------------------------*/
if (flag || (num3 != 1)) {
split_network(network3, init_param
->support);
if (XLN_DEBUG > 2) (void) printf("\tafter splitting %d nodes, ",
network_num_internal(network3)
);
xln_cover_or_partition(network3, init_param
);
num3 = network_num_internal(network3);
if (num1 < num3) {
network_free(network3);
}
else {
network_free(network1);
network1 = network3;
num1     = num3;
}
}
else {
network_free(network3);
}

/* if number of literals is large, do splitting followed by reduction */
/*--------------------------------------------------------------------*/
/* if ((num_lit > init_param->cover_node_limit + 1) || (num1 == 2)) { */
if (num1 == 2) {
*
pnetwork = network1;
return;
}

/* try tech-decomposing and reduction */
/*------------------------------------*/

network2 = network_create_from_node(node);
if (init_param->flag_decomp_good)
decomp_good_network(network2);
decomp_tech_network(network2,
2, 2);
if (XLN_DEBUG > 2) (void) printf("\tafter tech decomp %d nodes, ",
network_num_internal(network2)
);
xln_cover_or_partition(network2, init_param
);

num2 = network_num_internal(network2);
if (num1 < num2) {
network_free(network2);
}
else {
network_free(network1);
network1 = network2;
num1     = num2;
}

/* try roth-karp and cover next */
/*------------------------------*/
/*
network5 = network_create_from_node(node);
xln_exhaustive_k_decomp_network(network5, init_param);
num5 = network_num_internal(network5);
if (num1 < num5) {
    network_free(network5);
} else {
    network_free(network1);
    network1 = network5;
    num1 = num5;
}
*/

*
pnetwork = network1;
}

network_t *
xln_cofactor_decomp(node, support, mode)
        node_t *node;
        int support;
        float mode;
{
    network_t *network;
    node_t    *po, *node1;

    network = network_create_from_node(node);
    po      = network_get_po(network, 0);
    node1   = node_get_fanin(po, 0);
    (void) xln_cofactor_decomp_node(node1, support, mode);
    (void) network_sweep(network);
    return network;
}

/*-----------------------------------------------------------------------
  Given a node of the network, cofactors it recursively, till the nodes 
  added nodes become feasible. The selection of the cofactoring variable 
  is done based on the fact whether there is a variable that occurs in 
  all the cubes in the same phase (pos. or neg). 
------------------------------------------------------------------------*/
int
xln_cofactor_decomp_node(node, support, mode)
        node_t *node;
        int support;
        float mode;
{
    node_t *fanin, *fanin0, *fanin1;
    node_t *pos, *neg, *rem;
    node_t *p, *q;
    node_t *p1, *q1, *temp1, *temp2;
    node_t *new_node;

    if ((node->type == PRIMARY_INPUT) || (node->type == PRIMARY_OUTPUT)) return 0;
    if (node_num_fanin(node) <= support) return 0;
    if (mode == AREA) {
        fanin = xln_select_fanin_for_cofactor_area(node);
    } else {
        fanin = xln_select_fanin_for_cofactor_delay(node);
    }
    node_algebraic_cofactor(node, fanin, &pos, &neg, &rem);
    p        = node_or(pos, rem);
    q        = node_or(neg, rem);
    fanin0   = node_literal(fanin, 0); /* complemented */
    fanin1   = node_literal(fanin, 1); /* uncomplemented */
    p1       = node_literal(p, 1);
    q1       = node_literal(q, 1);
    temp1    = node_and(p1, fanin1);
    temp2    = node_and(q1, fanin0);
    new_node = node_or(temp1, temp2);
    node_free(fanin0);
    node_free(fanin1);
    node_free(pos);
    node_free(neg);
    node_free(rem);
    node_free(p1);
    node_free(q1);
    node_free(temp1);
    node_free(temp2);
    network_add_node(node->network, p);
    network_add_node(node->network, q);
    node_replace(node, new_node);
    (void) xln_cofactor_decomp_node(p, support, mode);
    (void) xln_cofactor_decomp_node(q, support, mode);
    return 1;
}

/*-----------------------------------------------------------------
  If there is a fanin that is in positive (or negative) phase in
  all the cubes, returns that fanin. Else returns the last fanin
  (does not actually matter).
------------------------------------------------------------------*/
node_t *
xln_select_fanin_for_cofactor_area(node)
        node_t *node;
{
    int    num_cube;
    int    *lit_count_arr;
    int    j, jx2;
    node_t *fanin;

    num_cube      = node_num_cube(node);
    lit_count_arr = node_literal_count(node);
    foreach_fanin(node, j, fanin)
    {
        jx2 = 2 * j;
        if ((lit_count_arr[jx2] == num_cube) || lit_count_arr[jx2 + 1] == num_cube) {
            FREE(lit_count_arr);
            return fanin;
        }
    }
    FREE(lit_count_arr);
    return fanin;
}

/*-----------------------------------------------------------------
  Returns the most critical fanin (with least slack). 
  Assumes that a delay trace has been done on the network.
------------------------------------------------------------------*/
node_t *
xln_select_fanin_for_cofactor_delay(node)
        node_t *node;
{
    node_t       *fanin, *min_fanin;
    delay_time_t slack_fanin;
    double       slack;
    int          j;

    slack     = (double) MAXINT;
    min_fanin = NIL(node_t);
    assert(node->type != PRIMARY_INPUT);
    foreach_fanin(node, j, fanin)
    {
        slack_fanin = delay_slack_time(fanin);
        if (slack > slack_fanin.rise) {
            slack     = slack_fanin.rise;
            min_fanin = fanin;
        }
    }
    assert(min_fanin != NIL(node_t));
    return min_fanin;
}
      
