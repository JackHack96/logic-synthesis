
#include "speed_int.h"
#include "sis.h"

/* Exported interface  */
array_t *speed_decomp_interface(node_t *f, double coeff, delay_model_t model) {
    array_t        *a;
    speed_global_t speed_param;

    (void) speed_fill_options(&speed_param, 0, NIL(char *));
    speed_param.coeff = coeff;
    speed_param.model = model;

    speed_set_delay_data(&speed_param, 0 /* No library acceleration */);
    a = speed_decomp(f, &speed_param, 0 /* Use the arrival times of fanins */);

    return a;
}

array_t *speed_decomp(node_t *innode, speed_global_t *speed_param, int delay_flag) {
    lsGen        gen;
    int          i, best_i;
    array_t      *array;
    delay_time_t delay;
    node_t       *temp, *node;
    double       best_delay, cur_delay;
    network_t    *best_network, *network;

    network = speed_network_create_from_node(innode, speed_param, delay_flag);

    /* Do a delay trace on the network and call the
    decomposition routine on the network */

    if (!speed_delay_trace(network, speed_param)) {
        fail(error_string());
    }

    foreach_node(network, gen, node) {
        if (node->type == INTERNAL) {
            (void) lsFinish(gen);
            break;
        }
    }

    /* If the MAPPED model is used get quick delays from global data_str */
    speed_set_library_accl(speed_param, 1);

    /*
     * Try different decompositions and select the best area/delay
     * tradeoff... Done, by choosing different number of inputs to call
     * critical
     */
    best_delay   = POS_LARGE;
    best_network = NIL(network_t);
    best_i       = -1;
    for (i       = 0; i < speed_param->num_tries; i++) {
        (void) network_collapse(network);
        if (!speed_decomp_network(network, node, speed_param,
                                  i /* i'th attempt */)) {
            error_append("Failed trying to speed_decomp network");
            fail(error_string());
        }
        /* Get the arrival-time of the primary output node */
        temp = network_get_po(network, 0);
        speed_delay_arrival_time(temp, speed_param, &delay);
        cur_delay = MAX(delay.rise, delay.fall);
        if (speed_param->num_tries > 1 && speed_param->debug) {
            (void) fprintf(sisout, "%d => %.2f\t", i, cur_delay);
        }
        if ((i == 0) || (cur_delay < best_delay)) {
            if (best_network != NIL(network_t))
                network_free(best_network);
            best_network = network_dup(network);
            best_delay   = cur_delay;
            best_i       = i;
        }
    }
    if (speed_param->num_tries > 1 && speed_param->debug) {
        (void) fprintf(sisout, " BEST is %d\n", best_i);
    }
    network_free(network);
    network = best_network;
    /* reset the library accelerator so as to get realistic delays */
    speed_set_library_accl(speed_param, 0);

    /*
     * Cleanup all the inverters and buffers if required
     */

    if (speed_param->add_inv) {
        add_inv_network(network);
        (void) speed_delay_trace(network, speed_param);
    }

    /* Convert the network into an array */
    if (speed_param->debug) {
        (void) fprintf(sisout, "After decomposition ----- \n");
        (void) com_execute(&network, "p");
    }
    array = network_and_node_to_array(network, innode, NIL(st_table));

    network_free(network);
    return array;
}

void speed_adjust_phase(network_t *network) {
    node_t  *node, *fo;
    lsGen   gen1;
    int     i;
    array_t *nodevec;

    /* Collapse all the inverters and buffers into their
    fanouts -- except if fanout is a primary output  */

    nodevec = network_dfs_from_input(network);
    for (i  = 0; i < array_n(nodevec); i++) {
        node = array_fetch(node_t *, nodevec, i);
        if (node->type == INTERNAL) {
            if ((node_function(node) == NODE_BUF) ||
                (node_function(node) == NODE_INV)) {
                foreach_fanout(node, gen1, fo) {
                    if (node_function(fo) != NODE_PO) {
                        (void) node_collapse(fo, node);
                    }
                }

                if (node_num_fanout(node) == 0) {
                    network_delete_node(network, node);
                }
            }
        }
    }
    array_free(nodevec);
}
