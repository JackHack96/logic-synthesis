
/*---------------------------------------------------------------------------
|      This file contains the symbolic simulation routine that is
|  necessary for power estimation in static circuits, both combinational
|  and sequential.
|
|        power_symbolic_simulate()
|
| Copyright (c) 1991 - Abhijit Ghosh. U.C. Berkeley
|
| Jose' Monteiro, MIT, Jan/93            jcm@rle-vlsi.mit.edu
|  - Modified to be included in SIS (instead of Flames)
|  - Added power also for primary inputs
+--------------------------------------------------------------------------*/

#include "sis.h"
#include "../include/power_int.h"


/* This is the one and only routine for symbolic simulation */
network_t *power_symbolic_simulate(network, info_table)
        network_t *network;
        st_table *info_table;
{
    st_table     *internal = st_init_table(st_ptrcmp, st_ptrhash);
    array_t      *gates,
                 *local_array,
                 *array1,
                 *array3,
                 *array4,
                 *before_switching,
                 *after_switching,
                 *switching_times;
    network_t    *symbolic_network;
    node_t       *actual_gate,
                 *a, *b, *xor,
                 *fanin,
                 *fanin1,
                 **old_to_gate,
                 **new_to_gate,
                 *pi1,
                 *pi2;
    node_info_t  *node_info;
    delay_info_t *delay_info;
    int          i, j, k, l,
                 *array2,
                 next,
                 first,
                 last,
                 *switched,
                 frame,
                 gate_delay,
                 max,
                 min,
                 max_num_tr,
                 transition;
    char         buffer[1000],
                 *key;
    st_generator *sgen;

    symbolic_network = network_alloc();
    strcpy(buffer, network_name(network));
    strcat(buffer, "_symbolic");
    network_set_name(symbolic_network, buffer);

    gates = power_network_dfs(network);

    max_num_tr = -INFINITY;

    for (i = 0; i < array_n(gates); i++) {
        actual_gate = array_fetch(node_t * , gates, i);

        if (node_function(actual_gate) == NODE_PI) {
            /* This is a PI gate, we have to do something special about it */
            /* All the PI and PS lines are assumed to switch at the same time */
            pi1 = node_dup(actual_gate);
            pi2 = node_dup(actual_gate);

            network_add_node(symbolic_network, pi1);
            sprintf(buffer, "%s_000", actual_gate->name);
            network_change_node_name(symbolic_network, pi1, util_strsav(buffer));
            pi1->copy = actual_gate;            /* Link to the original PI */

            network_add_node(symbolic_network, pi2);
            sprintf(buffer, "%s_ttt", actual_gate->name);
            network_change_node_name(symbolic_network, pi2, util_strsav(buffer));
            pi2->copy = actual_gate;            /* Link to the original PI */

            switching_times = array_alloc(
            int, 1);
            array_insert_last(
            int, switching_times, 0);
            before_switching = array_alloc(node_t * , 1);
            array_insert_last(node_t * , before_switching, pi1);
            after_switching = array_alloc(node_t * , 1);
            array_insert_last(node_t * , after_switching, pi2);

            delay_info = ALLOC(delay_info_t, 1);
            delay_info->before_switching = before_switching;
            delay_info->after_switching  = after_switching;
            delay_info->switching_times  = switching_times;
            st_insert(internal, (char *) actual_gate, (char *) delay_info);

            /* JCM - also calculate power for PIs */
            a   = node_literal(pi1, 1);
            b   = node_literal(pi2, 1);
            xor = node_xor(a, b);
            node_free(a);
            node_free(b);
            network_add_node(symbolic_network, xor);
            sprintf(buffer, "%s_xor_0", node_long_name(actual_gate));
            network_change_node_name(symbolic_network, xor, util_strsav(buffer));
            xor->copy = actual_gate;            /* Link symbolic->original */
            network_add_primary_output(symbolic_network, xor);

            continue;
        }

        if (node_function(actual_gate) == NODE_PO)
            /* There is nothing to do as it is a dummy node */
            continue;

        /* Came here => node is an internal node */
        /* This part of the routine is complicated */
        /* Allocate some stuff we will need */
        switching_times = array_alloc(
        int, 0);
        before_switching = array_alloc(node_t * , 0);
        after_switching  = array_alloc(node_t * , 0);
        delay_info       = ALLOC(delay_info_t, 1);
        delay_info->before_switching = before_switching;
        delay_info->after_switching  = after_switching;
        delay_info->switching_times  = switching_times;
        st_insert(internal, (char *) actual_gate, (char *) delay_info);

        if (node_num_fanin(actual_gate) == 0) {   /* i.e., constant nodes */
            pi1 = node_dup(actual_gate);
            network_add_node(symbolic_network, pi1);
            array_insert_last(
            int, switching_times, 0);
            array_insert_last(node_t * , delay_info->before_switching, pi1);
            array_insert_last(node_t * , delay_info->after_switching, pi1);
            continue;
        }

        /* Get the delay of the node first */
        assert(st_lookup(info_table, (char *) actual_gate, (char **) &node_info));
        gate_delay = node_info->delay; /* Just the transport delay */

        /* Get earliest transition arriving at gate */
        max            = -INFINITY;
        min            = INFINITY;
        foreach_fanin(actual_gate, j, fanin)
        {
            assert(st_lookup(internal, (char *) fanin, (char **) &delay_info));
            array1 = delay_info->switching_times;
            first  = array_fetch(
            int, array1, 0);
            last = array_fetch(
            int, array1, (array_n(array1) - 1));
            if (first < min)
                min = first;
            if (last > max)
                max = last;
        }
        if ((max + gate_delay) > max_num_tr)
            max_num_tr = max + gate_delay;

        /* Now see what transitions are possible at the output of the gate */
        local_array = array_alloc(
        int *, 0);
        for (j = 0; j < (max - min + 1); j++) {
            switched = ALLOC(
            int, node_num_fanin(actual_gate));
            for (k = 0; k < node_num_fanin(actual_gate); k++)
                switched[k] = -1;
            array_insert_last(
            int *, local_array, switched);
        }

        foreach_fanin(actual_gate, j, fanin)
        {
            assert(st_lookup(internal, (char *) fanin, (char **) &delay_info));
            array1 = delay_info->switching_times;
            for (k = 0; k < array_n(array1); k++) {
                first = array_fetch(
                int, array1, k) -min;
                array2 = array_fetch(
                int *, local_array, first);
                array2[j] = 1;
            }
        }

        /* Fix the old and new inputs array from which new gate will be made */
        old_to_gate = ALLOC(node_t * , node_num_fanin(actual_gate));
        new_to_gate = ALLOC(node_t * , node_num_fanin(actual_gate));
        foreach_fanin(actual_gate, j, fanin)
        {
            assert(st_lookup(internal, (char *) fanin, (char **) &delay_info));
            array1 = delay_info->before_switching;
            array3 = delay_info->after_switching;
            array4 = delay_info->switching_times;
            old_to_gate[j] = array_fetch(node_t * , array1, 0);
            first = array_fetch(
            int, array4, 0);
            if (first == min)
                new_to_gate[j] = array_fetch(node_t * , array3, 0);
            else
                new_to_gate[j] = old_to_gate[j];
        }

        for (j = 0; j < (max - min + 1); j++) {
            /* Check if there is a transition at current j */
            transition = 0;
            array2     = array_fetch(
            int *, local_array, j);
            for (l = 0; l < node_num_fanin(actual_gate); l++)
                if (array2[l] == 1) {
                    transition = 1;
                    break;
                }
            if (transition) {
                next = j + 1;

                pi1 = node_dup(actual_gate);
                pi2 = node_dup(actual_gate);

                network_add_node(symbolic_network, pi1);
                sprintf(buffer, "%s_1_%d", actual_gate->name, j);
                network_change_node_name(symbolic_network, pi1,
                                         util_strsav(buffer));

                network_add_node(symbolic_network, pi2);
                sprintf(buffer, "%s_2_%d", actual_gate->name, j);
                network_change_node_name(symbolic_network, pi2,
                                         util_strsav(buffer));

                array_insert_last(node_t * , before_switching, pi1);
                array_insert_last(node_t * , after_switching, pi2);
                array_insert_last(
                int, switching_times, j + min + gate_delay);

                for (k = 0; k < node_num_fanin(actual_gate); k++) {
                    fanin1 = node_get_fanin(actual_gate, k); /* Patch fanins */
                    node_patch_fanin(pi1, fanin1, old_to_gate[k]);
                    node_patch_fanin(pi2, fanin1, new_to_gate[k]);
                }

                a   = node_literal(pi1, 1);
                b   = node_literal(pi2, 1);
                xor = node_xor(a, b);
                node_free(a);
                node_free(b);
                network_add_node(symbolic_network, xor);
                sprintf(buffer, "%s_xor_%d", node_long_name(actual_gate),
                        j + min + gate_delay);
                network_change_node_name(symbolic_network, xor,
                                         util_strsav(buffer));
                xor->copy = actual_gate;         /* Link symbolic->original */
                network_add_primary_output(symbolic_network, xor);

                /* Now update the new and the old to gate arrays */
                /* Note that old_to_gate changes here */
                for (k = 0; k < node_num_fanin(actual_gate); k++)
                    old_to_gate[k] = new_to_gate[k];

                if (next > (max - min))  /* We are done, go out */
                    break;
                array2 = array_fetch(
                int *, local_array, next);
                for (k = 0; k < node_num_fanin(actual_gate); k++) {
                    if (array2[k] == 1) {
                        fanin = node_get_fanin(actual_gate, k);
                        assert(st_lookup(internal, (char *) fanin, (char **) &delay_info));
                        array3 = delay_info->after_switching;
                        array4 = delay_info->switching_times;
                        for (l = 0; l < array_n(array4); l++) {
                            frame = array_fetch(
                            int, array4, l);
                            if (next + min == frame)
                                new_to_gate[k] = array_fetch(node_t * , array3, l);
                        }
                    }
                }
            } /* end if(transition) */
            else {
                /* Just make the new and old to gate arrays consistent */
                /* old_to_gate does not change in this case */
                array2 = array_fetch(
                int *, local_array, j + 1);
                for (k = 0; k < node_num_fanin(actual_gate); k++) {
                    if (array2[k] == 1) {
                        fanin = node_get_fanin(actual_gate, k);
                        assert(st_lookup(internal, (char *) fanin, (char **) &delay_info));
                        array3 = delay_info->after_switching;
                        array4 = delay_info->switching_times;
                        for (l = 0; l < array_n(array4); l++) {
                            frame = array_fetch(
                            int, array4, l);
                            if (j + 1 + min == frame)
                                new_to_gate[k] = array_fetch(node_t * , array3, l);
                        }
                    }
                }
            } /* end else above */
        } /* end for loop of variable j */
        FREE(old_to_gate);
        FREE(new_to_gate);
        for (j = 0; j < array_n(local_array); j++) {
            array2 = array_fetch(
            int *, local_array, j);
            FREE(array2);
        }
        array_free(local_array);

    } /* end outermost for loop for variable i (over all gates in network) */

    st_foreach_item(internal, sgen, &key, (char **) &delay_info)
    {
        array_free(delay_info->switching_times);
        array_free(delay_info->after_switching);
        array_free(delay_info->before_switching);
        FREE(delay_info);
    }
    st_free_table(internal);
    array_free(gates);

    assert(network_check(symbolic_network));

    if (power_verbose) {
        fprintf(sisout, "Static Timing Analysis : Normalized Delay = %d units\n",
                (max_num_tr == -INFINITY) ? 0 : max_num_tr);
        fflush(sisout);
    }

    return symbolic_network;
}
