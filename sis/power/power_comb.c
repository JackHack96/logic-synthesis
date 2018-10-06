
/*---------------------------------------------------------------------------
|    This file contains routines for power estimation of combinational
|  circuits.
|
|        power_comb_static_zero()
|        power_comb_static_unit()
|        power_comb_static_arbit()
|
| Copyright (c) 1991 - Abhijit Ghosh. University of California, Berkeley
|
| Jose' Monteiro, MIT, Jan/93            jcm@rle-vlsi.mit.edu
|  - Modified to be included in SIS (instead of Flames)
|  - Added power information for each node
+--------------------------------------------------------------------------*/

#include "power_int.h"
#include "sis.h"

/* This routine is for finding the power dissipation in combinational circuits
 *  implemented using static logic, assuming zero delay
 */
int power_comb_static_zero(network_t *network, st_table *info_table, double *total_power) {
    st_table     *leaves = st_init_table(st_ptrcmp, st_ptrhash);
    bdd_manager  *manager;
    bdd_t        *bdd;
    power_info_t *power_info;
    node_info_t  *node_info;
    power_pi_t   *PIInfo;
    array_t      *poArray, *piOrder;
    node_t       *po, *pi, *node;
    double       prob_node_one;
    int          i;
    lsGen        gen;

    poArray = array_alloc(node_t *, 0);

    foreach_primary_output(network, gen, po) {
        array_insert_last(node_t *, poArray, po);
    }
    piOrder     = order_nodes(poArray, /* PI's only */ 1);
    if (piOrder == NIL(array_t))
        piOrder = array_alloc(node_t *, 0);
    array_free(poArray);

    manager = ntbdd_start_manager(3 * network_num_pi(network));

    PIInfo = ALLOC(power_pi_t, network_num_pi(network));
    for (i = 0; i < array_n(piOrder); i++) {
        pi = array_fetch(node_t *, piOrder, i);
        st_insert(leaves, (char *) pi, (char *) i);
        assert(st_lookup(info_table, (char *) pi, (char **) &node_info));
        PIInfo[i].probOne = node_info->prob_one;
    }
    array_free(piOrder);

    /* Create a BDD for each PO will create a BDD at each node */
    foreach_primary_output(network, gen, po) {
        bdd = ntbdd_node_to_bdd(po, manager, leaves);
    }

    /* Have to calculate the probability of a function being one given the
       probabilities in the one and the zero array */
    *total_power = 0.0;
    foreach_node(network, gen, node) {
        if (node_function(node) == NODE_PO)
            continue;
        assert(st_lookup(power_info_table, (char *) node, (char **) &power_info));

        if (node_function(node) == NODE_PI) {
            assert(st_lookup(info_table, (char *) node, (char **) &node_info));
            prob_node_one = node_info->prob_one;
        } else {
            bdd           = ntbdd_at_node(node);
            prob_node_one = power_calc_func_prob(bdd, PIInfo);
        }
        prob_node_one = 2 * prob_node_one * (1.0 - prob_node_one);
        if (power_verbose > 50)
            fprintf(sisout, "Node %s Probability %f\n", node_name(node),
                    prob_node_one);

        *total_power += power_info->cap_factor * prob_node_one * CAPACITANCE;
        power_info->switching_prob += prob_node_one;
    }
    *total_power *= 250.0; /* The 0.5 Vdd ^2 factor for a 5V Vdd */

    ntbdd_end_manager(manager);
    st_free_table(leaves);
    FREE(PIInfo);

    return 0;
}

/* This is the core routine for doing power computation for static circuits,
 *  using any kind of delay
 */
int power_comb_static_arbit(network_t *network, st_table *info_table, double *total_power) {
    st_table     *leaves = st_init_table(st_ptrcmp, st_ptrhash);
    array_t      *poArray, *piOrder;
    bdd_manager  *manager;
    bdd_t        *bdd;
    network_t    *symbolic;
    power_info_t *power_info;
    node_info_t  *node_info;
    power_pi_t   *PIInfo;
    node_t       *node, *po, *pi, *orig_node;
    double       prob_node_one;
    int          i;
    lsGen        gen;

    /* First simulate the circuit symbolically */
    symbolic = power_symbolic_simulate(network, info_table);

#ifdef DEBUG
    write_blif(sisout, symbolic, 0, 0);
    fflush(sisout);
    power_network_print(symbolic);
#endif

    /* Remove all PI's that don't go anywhere. Such PIs might exist */
    poArray = array_alloc(node_t *, 0);
    foreach_primary_input(symbolic, gen, pi) {
        if (node_num_fanout(pi) == 0)
            array_insert_last(node_t *, poArray, pi);
    }
    for (i = 0; i < array_n(poArray); i++) {
        node = array_fetch(node_t *, poArray, i);
        network_delete_node(symbolic, node);
    }
    array_free(poArray);

    /* We have the symbolic network, so now just get the probability of inputs
       and calculate the probability of function */

    poArray = array_alloc(node_t *, 0);
    foreach_primary_output(symbolic, gen, po) {
        array_insert_last(node_t *, poArray, po);
    }
    piOrder     = order_nodes(poArray, /* PI's only */ 1);
    if (piOrder == NIL(array_t)) /* So it doesn't crash without network */
        piOrder = array_alloc(node_t *, 0);
    array_free(poArray);

    manager = ntbdd_start_manager(3 * network_num_pi(symbolic));

    PIInfo = ALLOC(power_pi_t, network_num_pi(symbolic));
    for (i = 0; i < array_n(piOrder); i++) {
        pi = array_fetch(node_t *, piOrder, i);
        st_insert(leaves, (char *) pi, (char *) i);
        orig_node = pi->copy; /* Link to the original network */
        assert(st_lookup(info_table, (char *) orig_node, (char **) &node_info));
        PIInfo[i].probOne = node_info->prob_one;
    }
    array_free(piOrder);

    *total_power = 0.0;
    foreach_primary_output(symbolic, gen, po) {
        node          = po->fanin[0]; /* The actual node we are looking at */
        bdd           = ntbdd_node_to_bdd(node, manager, leaves);
        prob_node_one = power_calc_func_prob(bdd, PIInfo);
        orig_node     = node->copy; /* Link to the original network */
        assert(
                st_lookup(power_info_table, (char *) orig_node, (char **) &power_info));
        *total_power += power_info->cap_factor * prob_node_one * CAPACITANCE;
        power_info->switching_prob += prob_node_one;
        if (power_verbose > 50)
            fprintf(sisout, "Node %s Probability %f\n", node_name(node),
                    prob_node_one);
    }
    *total_power *= 250.0; /* The 0.5 Vdd ^2 factor for a 5V Vdd */

    ntbdd_end_manager(manager);
    st_free_table(leaves);
    FREE(PIInfo);
    network_free(symbolic);

    return 0;
}
