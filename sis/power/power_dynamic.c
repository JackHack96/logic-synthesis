
/*---------------------------------------------------------------------------
|    This file contains routines for power estimation of dynamic domino
|  circuits.
|
|        power_dynamic()
|
| Copyright (c) 1991 - Abhijit Ghosh. University of California, Berkeley
|
| Jose' Monteiro, MIT, Jan/93            jcm@rle-vlsi.mit.edu
|  - Modified to be included in SIS (instead of Flames)
|  - Fixed estimation for dynamic sequential circuits
+--------------------------------------------------------------------------*/

#include "sis.h"
#include "power_int.h"


/* This routine is for finding the power dissipation in combinational circuits
*  implemented using dynamic logic
*/
int power_dynamic(network, info_table, total_power)
network_t *network;
st_table *info_table;
double *total_power;
{
    st_table *leaves = st_init_table(st_ptrcmp, st_ptrhash);
    array_t *poArray,
            *piOrder,
            *psOrder;
    bdd_manager *manager;
    bdd_t *bdd;
    power_info_t *power_info;
    node_info_t *node_info;
    power_pi_t *PIInfo;
    node_t *po,
           *pi,
           *node;
    double prob_node_one,
           *psLineProb;
    int i;
    lsGen gen;

    /* Check if this is a sequential circuit. If so, calculate ps lines prob.
       For now, hardcode approximation method */
#ifdef SIS
    if(network_num_latch(network)){
        power_setSize = 1;
        psLineProb = power_direct_PS_lines_prob(network, info_table, &psOrder,
                                                NIL(network_t));
        FREE(psLineProb);     /* PS prob. have been updated inside */
        array_free(psOrder);
    }
#endif   /* SIS */

    poArray = array_alloc(node_t *, 0);

    foreach_primary_output(network, gen, po){
        array_insert_last(node_t *, poArray, po);
    }
    piOrder = order_nodes(poArray, /* PI's only */ 1);
    if(piOrder == NIL(array_t))
        piOrder = array_alloc(node_t *, 0);
    array_free(poArray);

    manager = ntbdd_start_manager(3 * network_num_pi(network));

    PIInfo = ALLOC(power_pi_t, network_num_pi(network));
    for(i = 0; i < array_n(piOrder); i++){
        pi = array_fetch(node_t *, piOrder, i);
        st_insert(leaves, (char *) pi, (char *) i);
        assert(st_lookup(info_table, (char *) pi, (char **) &node_info));
        PIInfo[i].probOne = node_info->prob_one;
    }
    array_free(piOrder);

    /* Create a BDD for each PO will create BDD at each node */
    foreach_primary_output(network, gen, po){
        bdd = ntbdd_node_to_bdd(po, manager, leaves);
    }

    /* Have to calculate the probability of a function being one given the
       probabilities in the one and the zero array */
    *total_power = 0.0;
    foreach_node(network, gen, node){
        if(node_function(node) == NODE_PI)
            continue;
        if(node_function(node) == NODE_PO)
            continue;
        bdd = ntbdd_at_node(node);
        prob_node_one = power_calc_func_prob(bdd, PIInfo);
        assert(st_lookup(power_info_table, (char*) node, (char**) &power_info));
        *total_power += power_info->cap_factor * prob_node_one * CAPACITANCE;
        power_info->switching_prob += prob_node_one;
        if(power_verbose > 50)
            fprintf(sisout, "Node %s Probability %f\n",
                    node_name(node), prob_node_one);
    }
    *total_power *= 250.0;  /* The 0.5 Vdd ^2 factor for a 5V Vdd */

    ntbdd_end_manager(manager);
    st_free_table(leaves);
    FREE(PIInfo);

    return 0;
}





