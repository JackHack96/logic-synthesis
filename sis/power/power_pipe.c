
/*---------------------------------------------------------------------------
|      This file contains routines that evaluate the power dissipation
|  in various types of static pipelined circuits.
|
|        power_pipe_static_zero()
|        power_pipe_static_unit()
|        power_pipe_static_arbit()
|        power_add_pipeline_logic();
|
| Jose' Monteiro, MIT, Feb/93            jcm@rle-vlsi.mit.edu
+--------------------------------------------------------------------------*/

#ifdef SIS

#include "power_int.h"
#include "sis.h"

static void power_pipe_network_concatenate();

int power_pipe_arbit(network_t *network, st_table *info_table, double *total_power) {
    st_table     *leaves = st_init_table(st_ptrcmp, st_ptrhash);
    array_t      *poArray, *piOrder;
    bdd_manager  *manager;
    bdd_t        *bdd;
    network_t    *symbolic;
    node_t       *node, *pi, *po;
    power_info_t *power_info;
    node_info_t  *node_info;
    power_pi_t   *PIInfo;
    double       prob_node_one;
    int          i;
    lsGen        gen;

    /* First thing to do is to get the symbolic network */
    symbolic = power_symbolic_simulate(network, info_table);

    /* Add 0-delay logic to simulate outputs of registers */
    if (power_add_pipeline_logic(network, symbolic))
        return 1;

    /* Now the probability calculation for the circuit */
    poArray = array_alloc(node_t *, 0);
    foreach_primary_output(symbolic, gen, po) {
        array_insert_last(node_t *, poArray, po);
    }
    piOrder     = order_nodes(poArray, /* PI's only */ 1);
    if (piOrder == NIL(array_t))
        piOrder = array_alloc(node_t *, 0);
    array_free(poArray);

    manager = ntbdd_start_manager(3 * network_num_pi(symbolic));

    PIInfo = ALLOC(power_pi_t, network_num_pi(symbolic));
    for (i = 0; i < array_n(piOrder); i++) {
        pi = array_fetch(node_t *, piOrder, i);
        st_insert(leaves, (char *) pi, (char *) i);
        node = pi->copy; /* Node in the original network */
        assert(st_lookup(info_table, (char *) node, (char **) &node_info));
        PIInfo[i].probOne = node_info->prob_one;
    }
    array_free(piOrder);

    *total_power = 0;
    foreach_primary_output(symbolic, gen, po) {
        node          = po->fanin[0]; /* The actual node we are looking at */
        bdd           = ntbdd_node_to_bdd(node, manager, leaves);
        prob_node_one = power_calc_func_prob(bdd, PIInfo);
        node          = node->copy; /* Link to the original network */
        assert(st_lookup(power_info_table, (char *) node, (char **) &power_info));
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

int power_add_pipeline_logic(network_t *network, network_t *symbolic) {
    array_t   *latchArray, *nodeArray;
    network_t *zero_delay_net;
    latch_t   *latch;
    node_t    *node, *po, *inLatch, *outLatch;
    int       i;
    lsGen     gen;

    /* Get the network corresponding to the whole combinational network */
    zero_delay_net = network_dup(network);

    /* Now delete the real PO gates */
    nodeArray = array_alloc(node_t *, 0);
    foreach_primary_output(zero_delay_net, gen, po) {
        if (network_is_real_po(zero_delay_net, po))
            array_insert_last(node_t *, nodeArray, po);
    }
    for (i = 0; i < array_n(nodeArray); i++) {
        node = array_fetch(node_t *, nodeArray, i);
        network_delete_node(zero_delay_net, node);
    }
    array_free(nodeArray);

    /* Short-circuit latches, also creating PO's at those nodes */
    latchArray = array_alloc(latch_t *, 0);
    foreach_latch(zero_delay_net, gen, latch) {
        array_insert_last(latch_t *, latchArray, latch);
    }
    for (i = 0; i < array_n(latchArray); i++) {
        latch    = array_fetch(latch_t *, latchArray, i);
        inLatch  = latch_get_input(latch);
        outLatch = latch_get_output(latch);
        network_add_primary_output(zero_delay_net, outLatch);
        network_delete_latch(zero_delay_net, latch);
        network_connect(inLatch, outLatch);
    }
    array_free(latchArray);

    /* Doesn't do any good, network_connect will crash before this test!
    if(!network_is_acyclic(zero_delay_net)){
        fprintf(siserr, "Network is not acyclic! Quit!\n");
        return 1;
    }                                                                      */

    /* Delete all the nodes that don't go anywhere */
    network_sweep(zero_delay_net);

    /* Now have to concatenate the networks: one instant 0 and other for t */
    power_pipe_network_concatenate(symbolic, zero_delay_net, "000");
    power_pipe_network_concatenate(symbolic, zero_delay_net, "ttt");

    network_free(zero_delay_net);

    /* Delete all the PI's that don't go anywhere */
    nodeArray = array_alloc(node_t *, 0);
    foreach_primary_input(symbolic, gen, node) {
        if (node_num_fanout(node) == 0)
            array_insert_last(node_t *, nodeArray, node);
    }
    for (i = 0; i < array_n(nodeArray); i++) {
        node = array_fetch(node_t *, nodeArray, i);
        network_delete_node(symbolic, node);
    }
    array_free(nodeArray);

    return 0;
}

static void power_pipe_network_concatenate(network_t *symbolic, network_t *zero_delay_net, char *instant) {
    st_table *correspondance = st_init_table(st_ptrcmp, st_ptrhash);
    array_t  *nodeArray, *inArray, *outArray;
    node_t   *node, *new_node, *fanin, *fanout;
    int      i, j;
    char     buffer[1000];
    lsGen    gen;

    inArray  = array_alloc(node_t *, 0);
    outArray = array_alloc(node_t *, 0);

    nodeArray = network_dfs(zero_delay_net);
    for (i    = 0; i < array_n(nodeArray); i++) {

        node = array_fetch(node_t *, nodeArray, i);

        new_node = node_dup(node);
        st_insert(correspondance, (char *) node, (char *) new_node);
        network_add_node(symbolic, new_node);

        /* There is no guarantee that the names in each net are unique.
           Add the prefix n0_ or nT_ to each name in the zero_delay_net */
        sprintf(buffer, "n%c_%s", instant[0] == '0' ? '0' : 'T', node->name);
        network_change_node_name(symbolic, new_node, util_strsav(buffer));

        if (node_function(node) == NODE_PI) {
            array_insert_last(node_t *, inArray, new_node);
            continue;
        }

        if (node_function(node) == NODE_PO) {
            array_insert_last(node_t *, outArray, new_node);

            /* Patch the fanin right */
            fanin = node->fanin[0];
            assert(st_lookup(correspondance, (char *) fanin, (char **) &node));
            node_patch_fanin(new_node, fanin, node);
            continue;
        }

        /* Came here => internal node */
        /*Patch its fanins */
        foreach_fanin(new_node, j, fanin) {
            assert(st_lookup(correspondance, (char *) fanin, (char **) &node));
            node_patch_fanin(new_node, fanin, node);
        }
    }
    array_free(nodeArray);
    st_free_table(correspondance);

    /* Now we have copied the zero_delay network into the symbolic network.
       The next thing to do is to link them up.                            */

    /* Substitute PIs of zero_delay_net by the PIs of symbolic network */
    for (i = 0; i < array_n(inArray); i++) {
        new_node = array_fetch(node_t *, inArray, i);

        sprintf(buffer, "%s_%s", &(new_node->name[3]), instant);
        node = network_find_node(symbolic, buffer);

        foreach_fanout(new_node, gen, fanout) {
            node_patch_fanin(fanout, new_node, node);
        }
        network_delete_node(symbolic, new_node);
    }
    array_free(inArray);

    /* Connect the POs of zero_delay_net to the respective PIs of symbolic
       network */
    for (i = 0; i < array_n(outArray); i++) {
        new_node = array_fetch(node_t *, outArray, i);

        sprintf(buffer, "%s_%s", &(new_node->name[3]), instant);
        node  = network_find_node(symbolic, buffer);
        fanin = new_node->fanin[0];

        foreach_fanout(node, gen, fanout) { node_patch_fanin(fanout, node, fanin); }
        network_delete_node(symbolic, new_node);
        network_delete_node(symbolic, node);
    }
    array_free(outArray);

    /* Check to make sure that network thing is valid */
    assert(network_check(symbolic));
}

#endif /* SIS */
