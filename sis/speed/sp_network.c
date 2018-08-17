
#include <stdio.h>
#include "sis.h"
#include "speed_int.h"

#define SPEED_DUP_PARAM(from, to, p, r)        \
    (r = delay_get_parameter(from,p), (void) delay_set_parameter(to, p, r))

/*
 * If delay_flag == 1 take the delay of the 
 * fanins from the user field 
 */

static int
speed_delay_parameters_dup(from, to, speed_param, delay_flag)
        node_t *from, *to;
        speed_global_t *speed_param;
        int delay_flag;
{
    delay_time_t time;
    double       temp;

    SPEED_DUP_PARAM(from, to, DELAY_BLOCK_RISE, temp);
    SPEED_DUP_PARAM(from, to, DELAY_DRIVE_RISE, temp);
    SPEED_DUP_PARAM(from, to, DELAY_BLOCK_FALL, temp);
    SPEED_DUP_PARAM(from, to, DELAY_DRIVE_FALL, temp);
    SPEED_DUP_PARAM(from, to, DELAY_MAX_INPUT_LOAD, temp);

    if (node_function(to) == NODE_PI) {
        if (delay_flag) {
            speed_delay_arrival_time(from, speed_param, &time);
        } else {
            time = delay_arrival_time(from);
        }

        (void) delay_set_parameter(to, DELAY_ARRIVAL_RISE, time.rise);
        (void) delay_set_parameter(to, DELAY_ARRIVAL_FALL, time.fall);
    } else if (node_function(to) == NODE_PO) {
        temp = delay_load(from);
        (void) delay_set_parameter(to, DELAY_OUTPUT_LOAD, temp);
    }

    return 1;
}

network_t *
speed_network_create_from_node(f, speed_param, delay_flag)
        node_t *f;
        speed_global_t *speed_param;
        int delay_flag;
{
    network_t *network;
    lsGen     gen;
    node_t    *pi, *fanin, *po;

    network = network_create_from_node(f);

    /* Copy the delay parameters */
    foreach_primary_output(network, gen, po)
    {
        if (!speed_delay_parameters_dup(f, po, speed_param, delay_flag)) {
            fail("Duplication of output parameters ");
            (void) lsFinish(gen);
        }
    }

    foreach_primary_input(network, gen, pi)
    {
        if ((fanin = name_to_node(f, node_long_name(pi))) != NIL(node_t)) {
            if (!speed_delay_parameters_dup(fanin, pi, speed_param, delay_flag)) {
                fail("Duplication of input parameters ");
                (void) lsFinish(gen);
            }
        }
    }

    return network;
}

array_t *
network_to_array(network)
        network_t *network;
{
    array_t *nodevec, *array;
    node_t  *node, *fanin, *newnode;
    int     i, j;

    array   = array_alloc(node_t * , 0);
    nodevec = network_dfs_from_input(network);
    for (i  = 0, j = 0; i < array_n(nodevec); j++, i++) {
        node    = array_fetch(node_t * , nodevec, i);
        newnode = node_dup(node);
        node->copy = newnode;
        array_insert(node_t * , array, j, newnode);
    }

    /* Change the fanin pointers */
    for (i = 0; i < array_n(array); i++) {
        node = array_fetch(node_t * , array, i);
        foreach_fanin(node, j, fanin)
        {
            node->fanin[j] = fanin->copy;
        }
        fanin_add_fanout(node);
    }
    array_free(nodevec);
    return array;
}

/*
 * For the nodes in the network, return an array of nodes that can be added to
 * another network (the internal pointers are consistent). If a hash_table
 * is also provided then it records the library gate that implements that node
 */
array_t *
network_and_node_to_array(network, innode, table)
        network_t *network;
        node_t *innode;
        st_table *table;
{
    array_t *nodevec, *array;
    node_t  *node, *fanin, *newnode, *orig;
    int     i, j;

    array   = array_alloc(node_t * , 0);
    nodevec = network_dfs_from_input(network);
    for (i  = 0; i < array_n(nodevec); i++) {
        node    = array_fetch(node_t * , nodevec, i);
        newnode = node_dup(node);
        if (table != NIL(st_table)) {
            (void) st_insert(table, (char *) newnode, (char *) lib_gate_of(node));
        }
        node->copy = newnode;
        array_insert(node_t * , array, i, newnode);
    }

    /* Change the fanin pointers */
    for (i = 0; i < array_n(array); i++) {
        node = array_fetch(node_t * , array, i);
        FREE(node->fanin_fanout);
        node->fanin_fanout = ALLOC(lsHandle, node->nin);
        foreach_fanin(node, j, fanin)
        {
            if (fanin->type == PRIMARY_INPUT) {
                /* patch the fanin to original */
                if ((orig = name_to_node(innode, node_long_name(fanin))) != NIL(node_t)) {
                    node->fanin[j] = orig;
                } else {
                    fail("Failed to retrieve the original node");
                }
            } else {
                /* update the fanin pointer */
                node->fanin[j] = fanin->copy;
            }
        }
    }
    array_free(nodevec);
    return array;
}
