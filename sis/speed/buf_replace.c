
#include "sis.h"
#include <math.h>
#include "../include/speed_int.h"
#include "../include/buffer_int.h"

static delay_time_t large_req_time = {POS_LARGE, POS_LARGE};

/*
 * This routine will take into consideration the drive of the 
 * previous gate (input) and the delay thru all the gates of the
 * same functionality as "node" and keep the best. When the
 * "do_decomp" flag is set it will explore decompositions into
 * simpler gates if there is no replacement cell.
 *
 * In case the "inv_node" is also present, will try alternatives for the
 * inverter too. Will keep the best comb of the inverter and the gate.
 */

int
sp_replace_cell_strength(network, buf_param, fanout_data, gate_array, num_gates,
                         crit_pos, crit_neg, total_cap_pos, total_cap_neg, save)
        network_t *network;
        buf_global_t *buf_param;
        buf_alg_input_t *fanout_data;
        delay_pin_t *gate_array;        /* Delay data of versions of node */
        int num_gates;
        delay_time_t crit_pos, crit_neg;    /* Earliest signal required time */
        double total_cap_pos, total_cap_neg;    /* capacitive loads in both phases */
        delay_time_t *save;            /* RETURN: saving of transformation */
{
    extern void map_invalid();        /* Should be in map.h */
    node_t        *node                         = fanout_data->node;
    node_t        *inv_node                     = fanout_data->inv_node;
    delay_model_t model                         = buf_param->model;
    int           do_decomp                     = buf_param->do_decomp;
    sp_buffer_t   **buf_array                   = buf_param->buf_array;

    network_t    *netw;
    array_t      *nodevec;
    sp_fanout_t  *inv_fan;
    st_table     *gate_table;
    double       auto_route                     = buf_param->auto_route;
    double       orig_load, cap_gI, max_ip_load = fanout_data->max_ip_load;
    delay_pin_t  *pin_delay;
    pin_phase_t  prev_ph;
    node_t       *temp, *root_node;
    sp_buffer_t  *buf_g, *inv_buf;
    delay_time_t orig_req, req_gI, req_g, best;
    delay_time_t prev_dr, prev_bl, orig_drive;
    lib_gate_t   *new_gate, *orig_gate, *root_gate;
    int          num_inv_choices, config_changed, changed;
    int          i, j, cfi, best_inv, best_root;

    if (buf_param->debug > 10) {
        (void) fprintf(sisout, "\tRepowering cell %s \n", sp_name_of_impl(node));
        (void) fprintf(sisout, "\t (+)%5.3f at %6.3f:%-6.3f and (-)%5.3f at %6.3f:%-6.3f\n",
                       total_cap_pos, crit_pos.rise, crit_pos.fall, total_cap_neg,
                       crit_neg.rise, crit_neg.fall);
    }

    /*
     * Get the data on the drive of the previous gate. Use that to modify the
     * required time at the input... This is done to compare the replacements
     * in the environment of the circuit 
     */

    changed        = FALSE; /* Records a change in the required time */
    config_changed = FALSE; /* Records a change in the configuration */
    cfi            = BUFFER(node)->cfi;
    orig_req       = BUFFER(node)->req_time;
    orig_drive     = BUFFER(node)->prev_dr;
    orig_load      = sp_get_pin_load(node, model);
    sp_drive_adjustment(orig_drive, orig_load, &orig_req);

    /*
     * We will first find out the required times, and loads that can be
     * offered by the inverter driving the additional gates
     */

    num_inv_choices               = (inv_node != NIL(node_t) ? buf_param->num_inv : 0);
    inv_fan                       = ALLOC(sp_fanout_t, num_inv_choices + 1);
    if (inv_node != NIL(node_t)) {
        for (i = 0; i < num_inv_choices; i++) {
            inv_buf = buf_array[i];
            req_gI  = crit_neg;
            sp_subtract_delay(PHASE_INVERTING, inv_buf->block, inv_buf->drive,
                              total_cap_neg, &req_gI);
            inv_fan[i].load = inv_buf->ip_load + auto_route;
            inv_fan[i].req  = req_gI;
        }
    }
    /* To save some assignment steps -- we do this trick */
    inv_fan[num_inv_choices].load = 0.0;
    inv_fan[num_inv_choices].req  = large_req_time;

    best_inv = best_root = -1;
    best.rise = best.fall = NEG_LARGE;

    for (i = 0; i < num_gates; i++) {
        pin_delay = &(gate_array[i]);
        if (pin_delay->load > max_ip_load) {
            /* The increase in fanin_load is unacceptable !!! */
            continue;
        }
        j = 0;
        do {
            req_gI = inv_fan[j].req;
            cap_gI = inv_fan[j].load;
            MIN_DELAY(req_g, req_gI, crit_pos);
            sp_subtract_delay(pin_delay->phase, pin_delay->block,
                              pin_delay->drive, total_cap_pos + cap_gI, &req_g);
            sp_drive_adjustment(orig_drive, pin_delay->load, &req_g);

            if (req_g.rise > best.rise && req_g.fall > best.fall) {
                best     = req_g;
                best_inv = j;
                best_root = i;
            }
        } while (++j < num_inv_choices);
    }

    if (REQ_IMPR(best, orig_req)) {
        /*
         * We have saves by choice of appropriate root node and inverter
         * Implement the saved config and update the delay information
         */
        changed = 1;
        if (buf_param->debug > 10) {
            (void) fprintf(sisout, "REPOWER: Repowering achieves %6.3f:%-6.3f\n",
                           best.rise, best.fall);
            (void) fprintf(sisout, "\tRoot version = %d, Inverter index is %d\n",
                           best_root, best_inv);
        }

        if (BUFFER(node)->type == BUF) {
            buf_g = buf_array[best_root];
            if (BUFFER(node)->impl.buffer != buf_g) {
                config_changed = TRUE;
            }
            BUFFER(node)->impl.buffer = buf_g;
            sp_implement_buffer_chain(network, node);
            BUFFER(node)->cfi = cfi;
            prev_bl = buf_g->block;
            prev_dr = buf_g->drive;
            prev_ph = buf_g->phase;
        } else {
            root_gate = buf_get_gate_version(lib_gate_of(node), best_root);
            if (lib_gate_of(node) != root_gate) {
                config_changed = TRUE;
            }
            assert(root_gate != NIL(lib_gate_t));
            BUFFER(node)->impl.gate = root_gate;
            sp_replace_lib_gate(node, lib_gate_of(node), root_gate);
            prev_dr = ((delay_pin_t * )(root_gate->delay_info[cfi]))->drive;
            prev_ph = ((delay_pin_t * )(root_gate->delay_info[cfi]))->phase;
            prev_bl = ((delay_pin_t * )(root_gate->delay_info[cfi]))->block;
        }

        /* Implement the inverter if required and update its data */
        req_gI = large_req_time;
        cap_gI = 0.0;
        if (inv_node != NIL(node_t)) {
            /* Annotate the "inv_node" with the inverting buffer "best_inv" */
            inv_buf = buf_array[best_inv];
            if (BUFFER(inv_node)->impl.buffer != inv_buf) {
                config_changed = TRUE;
            }
            req_gI  = crit_neg;
            cap_gI  = auto_route + inv_buf->ip_load;
            sp_subtract_delay(PHASE_INVERTING, inv_buf->block, inv_buf->drive,
                              total_cap_neg, &req_gI);
            BUFFER(inv_node)->type        = BUF;
            BUFFER(inv_node)->impl.buffer = inv_buf;
            sp_implement_buffer_chain(network, inv_node);
            BUFFER(inv_node)->cfi      = 0;
            BUFFER(inv_node)->req_time = req_gI;
            BUFFER(inv_node)->prev_ph  = prev_ph;
            BUFFER(inv_node)->prev_dr  = prev_dr;
        }
        /*
         * Now to update the required time data for the root gate itself
         */
        MIN_DELAY(req_g, crit_pos, req_gI);
        sp_subtract_delay(prev_ph, prev_bl, prev_dr,
                          total_cap_pos + cap_gI, &req_g);
        BUFFER(node)->req_time = req_g;

        if (config_changed == FALSE) {
            (void) fprintf(sisout, "Reporting changed when nothing happened\n");
            fail("ERROR in computation");
        }

    } else if (do_decomp && num_gates <= 1 && node_num_fanin(node) > 2) {
        /*
         * No replacement (stronger gate) available in library.
         * Generate a topology using mapping for delay.
         * Check the decomposition of this gate for a saving in delay
         */
        netw      = delay_generate_decomposition(node, 1.0 /* best delay */);
        /*
         * HACK: get_pin_delay() will return the delay data for the gate
         * since the node is mapped... We have to fake the routine to
         * return the mapped parameters computed by the above routine.
         * We will invalidate the mapping and then set it later in case the
         * decomposition is not accepted
         */
        orig_gate = lib_gate_of(node);
        map_invalid(node);
        pin_delay = get_pin_delay(node, cfi, model);

        best.rise = best.fall = NEG_LARGE;
        j = 0;
        best_inv = -1;;
        do {
            req_gI = inv_fan[j].req;
            cap_gI = inv_fan[j].load;
            MIN_DELAY(req_g, req_gI, crit_pos);
            sp_subtract_delay(pin_delay->phase, pin_delay->block,
                              pin_delay->drive, total_cap_pos + cap_gI, &req_g);
            sp_drive_adjustment(orig_drive, pin_delay->load, &req_g);

            if (req_g.rise > orig_req.rise && req_g.fall > orig_req.fall) {
                best = req_g;
                best_inv = j;
            }
        } while (++j < num_inv_choices);

        if (REQ_IMPR(req_g, orig_req) && pin_delay->load < max_ip_load) {
            /*
             * We will not bother to update the delay info
             * since this is the end of recursion. Future delay
             * traces will take care of the updating
             */
            changed = 1;
            if (inv_node != NIL(node_t)) {
                new_gate                    = (buf_array[best_inv])->gate[0];
                sp_set_inverter(inv_node, node, new_gate);
                BUFFER(inv_node)->type      = GATE;
                BUFFER(inv_node)->cfi       = 0;
                BUFFER(inv_node)->impl.gate = new_gate;
                BUFFER(inv_node)->req_time  = inv_fan[best_inv].req;
            }
            /*
             * Replace "node" by nodes in "netw"
             */
            gate_table = st_init_table(st_ptrcmp, st_ptrhash);
            nodevec    = network_and_node_to_array(netw, node, gate_table);
            root_node  = array_fetch(node_t * , nodevec, 1);
            for (i     = array_n(nodevec) - 1; i > 1; i--) {
                temp = array_fetch(node_t * , nodevec, i);
                if (node_function(temp) != NODE_PI) {
                    network_add_node(network, temp);
                    /* Also need to annotate the node with corresp gate */
                    assert(st_lookup(gate_table, (char *) temp, (char **) &new_gate));
                    buf_add_implementation(temp, new_gate);
                } else {
                    node_free(temp);
                }
            }
            assert(st_lookup(gate_table, (char *) root_node, (char **) &new_gate));
            node_replace(node, root_node);
            buf_add_implementation(node, new_gate);
            array_free(nodevec);
            network_free(netw);
            st_free_table(gate_table);
        } else {
            /* Need to reinstate the mapping for the node */
            buf_add_implementation(node, orig_gate);
        }
    }

    if (changed) {
        save->rise = best.rise - orig_req.rise;
        save->fall = best.fall - orig_req.fall;
    } else {
        save->rise = save->fall = 0.0;
    }

    if (buf_param->debug > 10) {
        if (changed) {
            (void) fprintf(sisout, "Saving achieved %7.3f:%-7.3f\n",
                           save->rise, save->fall);
        } else {
            (void) fprintf(sisout, "No saving from repowering\n");
        }
    }

    FREE(inv_fan);
    return changed;
}


