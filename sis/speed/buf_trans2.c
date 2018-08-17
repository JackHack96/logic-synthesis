
#include "sis.h"
#include "../include/speed_int.h"
#include "../include/buffer_int.h"

static void buf_annotate_trans2();

static void buf_implement_trans2();

static delay_time_t large_req_time = {POS_LARGE, POS_LARGE};

/*
 * This routine takes the node "node" and the fanout set in "fanouts"
 * (with the negative phase signals driven by the node "node_inv").
 * It will then evaluate the BALANCED DECOMPOSITION of the signals in
 * the fanout set and possible repowerings/duplications to get the
 * required time at the input of "node" to be as large as possible
 * However we only increase the required time till it meets the "req_diff"
 * value. Increasing req time beyond this does not help circuit delay.
 *
 * Divide the positive and negative signals into groups...
 * Then evaluate the following 
 * 	-- Each group driven by an inverter and an additional 
 *	inverter driving the added inverters
 *	-- The node is itself duplicated adequately and each
 *	group is driven by one of the duplicated nodes
 */

int
buf_evaluate_trans2(network, buf_param, fanout_data,
                    gate_array, a_array, num_gates, cap_K_pos, cap_K_neg,
                    req_diff, levelp)
        network_t *network;
        buf_global_t *buf_param;
        buf_alg_input_t *fanout_data;
        delay_pin_t *gate_array;
        double *a_array;
        int num_gates;
        double *cap_K_pos, *cap_K_neg;
        delay_time_t req_diff;                      /* Amount we need to save */
        int *levelp;
{
    int             changed;
    int             met_target;
    int             k, l, start, end;
    int             n, num_pos_part, num_neg_part;
    int             g, i, j, a, b, gI;
    int             num_inv     = buf_param->num_inv;
    delay_model_t   model       = buf_param->model;
    node_t          **new_a, *new_b, **new_gI;
    int             best_g, best_i, best_j, best_a, best_b, best_gI;
    double          *new_K_pos, *new_K_neg;
    double          area_a, area_b, area_gI, min_area, cur_area;
    double          auto_route  = buf_param->auto_route;
    double          load, load_a, load_b, load_gI, orig_load;
    double          best_load_op_g, load_op_g;
    delay_pin_t     *pin_delay;
    delay_time_t    orig_drive;
    sp_fanout_t     *new_fan;
    buf_alg_input_t *new_fanout_data;
    sp_buffer_t     *buf_a, *buf_b, *buf_gI;
    sp_buffer_t     **buf_array = buf_param->buf_array;
    delay_time_t    req_g, req_a, req_b, req_gI, cur_req;
    delay_time_t    best_req_a, best_req_b, best_req_gI;
    delay_time_t    req_op_g, best_req_op_g;
    delay_time_t    margin, target, best, orig_req, req_pos;

    node_t      *node     = fanout_data->node;
    node_t      *node_inv = fanout_data->inv_node;
    sp_fanout_t *fanouts  = fanout_data->fanouts;
    int         num_pos   = fanout_data->num_pos;
    int         num_neg   = fanout_data->num_neg;

    ++(*levelp);
    changed = FALSE;
    n       = num_pos + num_neg;

    qsort((void *) fanouts, (unsigned) n, sizeof(sp_fanout_t), buf_compare_fanout);
    if (buf_param->debug > 10) {
        (void) fprintf(sisout, "******Trans2-Iteration %d -- aim to save %.3f:%.3f\n",
                       *levelp, req_diff.rise, req_diff.fall);
        (void) fprintf(sisout, "\tRequired time and Load distribution \n");
        buf_dump_fanout_data(sisout, fanouts, n);
    }

    num_neg_part = num_neg / 2;    /* Number of negative partitions */
    num_pos_part = num_pos / 2;    /* Number of positive partitions */

    if ((num_neg_part + num_pos_part) == 0) {
        if (buf_param->debug > 10) {
            (void) fprintf(sisout, "No possibility of balanced partitions\n");
        }
        return changed;
    }
    orig_req   = BUFFER(node)->req_time;
    orig_drive = BUFFER(node)->prev_dr;
    orig_load  = sp_get_pin_load(node, model);

    /* Adjust for delay due to limited drive of preceeding gate */
    sp_drive_adjustment(orig_drive, orig_load, &orig_req);

    /*
     * Set a target for the current recursion. This is the value of the
     * required time such that making the required time greater than this
     * will not effect the delay of the circuit
     */
    target.rise = orig_req.rise + req_diff.rise;
    target.fall = orig_req.fall + req_diff.fall;

    /*
     * Try duplication of node_inv "i" times and division of 
     * positive fanout signals into "j" groups
     */
    met_target = FALSE;
    best_i     = best_j = best_gI = best_a = best_b = -1;
    best.rise = best.fall = NEG_LARGE;
    min_area = POS_LARGE;

    /* Checking the various configurations */
    for (g = 0; g < num_gates; g++) {/* versions of root node */
        pin_delay = &(gate_array[g]);
        if (pin_delay->load > fanout_data->max_ip_load) {
            /* Load limit at "cfi" of root is violated !!! */
            continue;
        }
        for (gI = 0; gI < num_inv; gI++) {   /* Choice of inverter gI */
            buf_gI = buf_array[gI];
            i      = 2;
            do { /* Try the different partitions of negative fanouts */
                /*
                 * Compute the req_time at the input of each gI inverter
                 * Also compute req_gI (The min req time among gI inputs)
                 * and load_gI (the load of all the gI nodes)
                 */
                req_gI = large_req_time;
                load_gI = 0;
                area_gI = 0;
                load = 0.0;
                if (num_neg > 1) {
                    area_gI = i * buf_gI->area;
                    load_gI = i * (auto_route + buf_gI->ip_load);
                    start   = 0;
                    for (k  = i; k > 0; k--, start = end) {
                        end     = start + (num_neg - start) / k;
                        load    = cap_K_neg[end] - cap_K_neg[start];
                        cur_req = large_req_time;
                        for (l  = start; l < end; l++) {
                            MIN_DELAY(cur_req, cur_req,
                                      fanouts[num_pos + l].req);
                        }
                        sp_subtract_delay(buf_gI->phase, buf_gI->block,
                                          buf_gI->drive, load, &cur_req);
                        MIN_DELAY(req_gI, req_gI, cur_req);
                    }
                } else if (num_neg == 1) {
                    i       = 1;
                    area_gI = buf_gI->area;
                    load_gI = auto_route + buf_gI->ip_load;
                    req_gI  = fanouts[num_pos].req;
                    sp_subtract_delay(buf_gI->phase, buf_gI->block,
                                      buf_gI->drive, load, &req_gI);
                }

                j = 2;
                do {    /* Partition positive fanouts into "j" groups */
                    for (a = 0; a < num_inv; a++) { /* inv driving pos */
                        /* req_a & load_a are at the input of buffers "a" */
                        req_a = large_req_time;
                        load_a = 0;
                        area_a = 0;
                        buf_a = buf_array[a];
                        if (num_pos > 1) {
                            area_a = j * buf_a->area;
                            load_a = j * (auto_route + buf_a->ip_load);
                            start  = 0;
                            for (k = j; k > 0; k--, start = end) {
                                end     = start + (num_pos - start) / k;
                                load    = cap_K_pos[end] - cap_K_pos[start];
                                req_pos = large_req_time;
                                for (l  = start; l < end; l++) {
                                    MIN_DELAY(req_pos, req_pos, fanouts[l].req);
                                }
                                sp_subtract_delay(buf_a->phase, buf_a->block,
                                                  buf_a->drive, load, &(req_pos));
                                MIN_DELAY(req_a, req_a, req_pos);
                            }
                        } else if (num_pos == 1) {
                            j      = 1;
                            area_a = buf_a->area;
                            load_a = auto_route + buf_a->ip_load;
                            req_a  = fanouts[0].req;
                            sp_subtract_delay(buf_a->phase, buf_a->block,
                                              buf_a->drive, load, &req_a);
                        }

                        /* Now compute the req time at input of "b" */
                        for (b = 0; b < num_inv; b++) { /* inv driving a's*/
                            buf_b = buf_array[b];
                            req_b = req_a;
                            load_b = 0;
                            area_b = 0;
                            if (num_pos > 0) {
                                area_b = buf_b->area;
                                load_b = buf_b->ip_load + auto_route;
                                sp_subtract_delay(buf_b->phase, buf_b->block,
                                                  buf_b->drive, load_a, &req_b);
                            }

                            /*
                             * Now we can finally compute the required time at
                             * the root of the original gate
                             */

                            MIN_DELAY(req_g, req_b, req_gI);
                            req_op_g  = req_g;
                            load_op_g = load_b + load_gI;

                            sp_subtract_delay(pin_delay->phase,
                                              pin_delay->block, pin_delay->drive,
                                              load_op_g, &req_g);
                            /* Account for drive of previous stage */
                            sp_drive_adjustment(orig_drive, pin_delay->load, &req_g);

                            if (REQ_IMPR(req_g, target)) {
                                /* Keep the smallest area config */
                                cur_area = a_array[g] + area_b + area_a + area_gI;
                                if (cur_area < min_area) {
                                    met_target = TRUE;
                                    min_area   = cur_area;
                                    best_b     = b;
                                    best_a  = a;
                                    best_gI = gI;
                                    best_g = g;
                                    best_i = i;
                                    best_j = j;
                                    best_req_gI = req_gI;
                                    best_req_b = req_b;
                                    best_req_a = req_a;
                                }
                            }
                            if (!met_target && REQ_IMPR(req_g, best)) {
                                best   = req_g;
                                best_b = b;
                                best_a  = a;
                                best_gI = gI;
                                best_g = g;
                                best_i = i;
                                best_j = j;
                                best_req_gI = req_gI;
                                best_req_b = req_b;
                                best_req_a     = req_a;
                                best_req_op_g  = req_op_g;
                                best_load_op_g = load_op_g;
                            }
                            if (num_pos <= 1) break;
                        }
                        if (num_pos <= 1) break;
                    }
                } while (j++ < num_pos_part);
            } while (i++ < num_neg_part);
            /* No need to try more choices of gI inverters */
            if (num_neg == 0) break;
        }
    }
    if (num_pos == 0) best_j = 0;
    if (num_neg == 0) best_i = 0;

    if (buf_param->debug > 10) {
        (void) fprintf(sisout, "Without root dupliction balanced trans2 achieves %6.3f:%-6.3f\n",
                       best.rise, best.fall);
    }

    /*
     * Check the case that slack at input other than "cfi" has worsened
     * Not required when interfacing to the mapper
     */
    if (buf_param->interactive && !met_target &&
        REQ_IMPR(best, orig_req) && (node_num_fanin(node) > 1)) {
        if (buf_failed_slack_test(node, buf_param, best_g, best_req_op_g,
                                  best_load_op_g)) {
            if (buf_param->debug > 10) {
                (void) fprintf(sisout, "SLACK CHECK during balanced xform\n");
            }
            best.rise = best.fall = NEG_LARGE;
        }
    }

    if (REQ_IMPR(best, orig_req)) {
        /*
         * Have gotton some saving over naive buffering ---
         * Generate the new configuration and then recur
         */

        changed = 1;
        buf_implement_trans2(network, buf_param, node, node_inv, fanouts, num_pos,
                             num_neg, best_i, best_j, &new_a, &new_b, &new_gI);
        /*
        if (node_inv != NIL(node_t)){
            assert(node_num_fanout(node_inv) == 0);
            network_delete_node(network, node_inv);
        }
        */
        buf_annotate_trans2(network, buf_param, fanouts, num_pos, num_neg, node,
                            new_a, new_b, new_gI, best_g, best_gI, best_a,
                            best_b, best_i, best_j);

        if (!met_target) {
            /* How much are we still short */
            margin.rise = req_diff.rise - best.rise + orig_req.rise;
            margin.fall = req_diff.fall - best.fall + orig_req.fall;

            /*
             * Make recursive calls to the root node with new fanout set
             * The fanout set is the set of invertes "a" as the negative
             * set and the inverters "gI" as the positive set
             */
            new_fan = ALLOC(sp_fanout_t, best_i + best_j);

            new_fanout_data = ALLOC(buf_alg_input_t, 1);
            new_fanout_data->fanouts     = new_fan;
            new_fanout_data->node        = node;
            new_fanout_data->inv_node    = new_b;
            new_fanout_data->num_pos     = best_i;
            new_fanout_data->num_neg     = best_j;
            new_fanout_data->max_ip_load = fanout_data->max_ip_load;

            new_K_pos = ALLOC(
            double, best_i + 1);
            new_K_neg = ALLOC(
            double, best_j + 1);
            new_K_pos[0] = new_K_neg[0] = 0.0;
            for (i = 0; i < best_i; i++) {
                new_fan[i].fanout = new_gI[i];
                new_fan[i].pin    = 0;
                new_fan[i].load   = auto_route + sp_get_pin_load(new_gI[i], model);
                new_fan[i].req    = BUFFER(new_gI[i])->req_time;
                new_fan[i].phase  = PHASE_NONINVERTING;
                new_K_pos[i + 1] = new_K_pos[i] + new_fan[i].load;
            }

            for (j = 0, l = best_i; j < best_j; j++, l++) {
                new_fan[l].fanout = new_a[j];
                new_fan[l].pin    = 0;
                new_fan[l].load   = auto_route + sp_get_pin_load(new_a[j], model);
                new_fan[l].req    = BUFFER(new_a[j])->req_time;
                new_fan[l].phase  = PHASE_INVERTING;
                new_K_neg[j + 1] = new_K_neg[j] + new_fan[l].load;
            }
            /* No need to do an unbalanced decomp at this stage */
            /*
            changed += sp_buffer_recur(network, buf_param, model, node, new_b,
                new_fan, best_i, best_j, margin, levelp);
            */
            changed += buf_evaluate_trans2(network, buf_param, new_fanout_data,
                                           gate_array, a_array, num_gates, new_K_pos, new_K_neg,
                                           margin, levelp);
            FREE(new_fanout_data->fanouts);
            FREE(new_fanout_data);
            FREE(new_K_pos);
            FREE(new_K_neg);
        } else {
            /* Trans2 achieves the desired required time */
            if (buf_param->debug > 10)
                (void) fprintf(sisout,
                               "\t--- Trans2 ACHIEVES DESIRED REQUIRED TIME---\n");
        }
        FREE(new_a);
        FREE(new_gI);
    }

    return changed;
}

static void
buf_implement_trans2(network, buf_param, node, inv_node, fanouts, num_pos,
                     num_neg, best_i, best_j, new_ap, new_bp, new_gIp)
        network_t *network;
        buf_global_t *buf_param;
        node_t *node, *inv_node;
        sp_fanout_t *fanouts;
        int num_pos, num_neg;
        int best_i, best_j;
        node_t ***new_ap, **new_bp, ***new_gIp;    /* RETURN: added nodes */
{
    node_t *new_node;
    int    i, l, count, start, end;
    node_t **new_a, *new_b, **new_gI;

    /* Defaults */
    new_b  = NIL(node_t);
    new_a  = NIL(node_t * );
    new_gI = NIL(node_t * );

    /* Add "best_i" number of inverters (including inv_node) */
    if (num_neg > 0 && best_i > 0) {
        count  = start = 0;
        new_gI = ALLOC(node_t * , best_i);

        for (i = best_i; i > 0; i--, start = end) {
            end = start + (num_neg - start) / i;
            if (i == 1) {
                /* use the inv_node as the last one */
                new_node = inv_node;
            } else {
                new_node = node_literal(node, 0);
                network_add_node(network, new_node);
            }
            new_gI[count++] = new_node;

            for (l = start; l < end; l++) {
                assert(node_patch_fanin(fanouts[num_pos + l].fanout, inv_node, new_node));
            }
        }
    } else if (buf_param->debug > 10) {
        (void) fprintf(sisout, "No partion of negative fanouts\n");
    }
    /*
     * Now create the tree for the positive fanouts...
     */
    if (num_pos > 0 && best_j > 0) {
        count = start = 0;
        new_a = ALLOC(node_t * , best_j);

        /* The inverter at the root of the tree */
        new_b = node_literal(node, 0);
        network_add_node(network, new_b);

        for (i = best_j; i > 0; i--, start = end) {
            end = start + (num_pos - start) / i;

            new_node = node_literal(new_b, 0);
            network_add_node(network, new_node);
            new_a[count++] = new_node;

            for (l = start; l < end; l++) {
                assert(node_patch_fanin(fanouts[l].fanout, node, new_node));
            }
        }
    } else if (buf_param->debug > 10) {
        (void) fprintf(sisout, "No partion of positive fanouts\n");
    }

    *new_bp  = new_b;
    *new_ap  = new_a;
    *new_gIp = new_gI;
}

static void
buf_annotate_trans2(network, buf_param, fanouts, num_pos, num_neg,
                    node, new_a, new_b, new_gI,
                    best_g, best_gI, best_a, best_b, best_i, best_j)
        network_t *network;
        buf_global_t *buf_param;
        sp_fanout_t *fanouts;
        int num_pos, num_neg;
        node_t *node, **new_a, *new_b, **new_gI;
        int best_g, best_gI, best_a, best_b, best_i, best_j;
{
    int          start, end;
    int          i, j, l, cfi;
    pin_phase_t  prev_ph;
    lib_gate_t   *root_gate;
    node_t       *cur_a, *cur_gI;
    double       auto_route  = buf_param->auto_route;
    double       load, cap_b, cap_a, cap_gI;
    sp_buffer_t  *buf_b, *buf_a, *buf_g, *buf_gI;
    sp_buffer_t  **buf_array = buf_param->buf_array;
    delay_time_t prev_dr, prev_bl;
    delay_time_t req_b, req_a, req_gI, req_g, cur_req;

    cfi = BUFFER(node)->cfi;
    if (node->type == PRIMARY_INPUT) {
        prev_bl.rise = prev_bl.fall = 0.0;
        prev_dr.rise = prev_dr.fall = 0.0; /* This needs to be fixed !!!! */
        prev_ph = PHASE_NONINVERTING;
    } else if (BUFFER(node)->type == BUF) {
        buf_g = buf_array[best_g];
        BUFFER(node)->impl.buffer = buf_g;
        sp_implement_buffer_chain(network, node);
        BUFFER(node)->cfi = cfi;
        prev_bl = buf_g->block;
        prev_dr = buf_g->drive;
        prev_ph = buf_g->phase;
    } else {
        /* Find the appropriate gate in the class if needed */
        root_gate = lib_gate_of(node);
        if (buf_param->mode & REPOWER_MASK) {
            root_gate               = buf_get_gate_version(root_gate, best_g);
            assert(root_gate != NIL(lib_gate_t));
            BUFFER(node)->impl.gate = root_gate;
            sp_replace_lib_gate(node, lib_gate_of(node), root_gate);
        }
        prev_dr = ((delay_pin_t * )(root_gate->delay_info[cfi]))->drive;
        prev_ph = ((delay_pin_t * )(root_gate->delay_info[cfi]))->phase;
        prev_bl = ((delay_pin_t * )(root_gate->delay_info[cfi]))->block;
    }

    /* Now all the gI inverters are annotated */
    req_gI = large_req_time;
    cap_gI = 0;
    if (new_gI != NIL(node_t * )) {
        start  = 0;
        for (i = best_i, j = 0; i > 0; i--, j++, start = end) {
            end = start + (num_neg - start) / i;

            cur_gI = new_gI[j];
            buf_gI = buf_array[best_gI];
            BUFFER(cur_gI)->type        = BUF;
            BUFFER(cur_gI)->impl.buffer = buf_gI;
            sp_implement_buffer_chain(network, cur_gI);

            load = 0;
            cur_req = large_req_time;
            for (l = start; l < end; l++) {
                load += fanouts[num_pos + l].load;
                MIN_DELAY(cur_req, cur_req, fanouts[num_pos + l].req);
            }
            sp_subtract_delay(buf_gI->phase, buf_gI->block, buf_gI->drive,
                              load, &cur_req);

            MIN_DELAY(req_gI, req_gI, cur_req);

            cap_gI += buf_gI->ip_load + auto_route;

            BUFFER(cur_gI)->load     = load;
            BUFFER(cur_gI)->req_time = cur_req;
            BUFFER(cur_gI)->cfi      = 0;
            BUFFER(cur_gI)->prev_ph  = prev_ph;
            BUFFER(cur_gI)->prev_dr  = prev_dr;
        }
    }

    /*
     * Now see if there is a partitioning for the positive fanout signals
     * If so annotate the buffer "b" and fannning out from it the "best_j"
     * inverters "a"
     */
    req_b = large_req_time;
    cap_b = 0;
    if (new_a != NIL(node_t * )) {

        assert(new_b != NIL(node_t));
        buf_b = buf_array[best_b];

        req_a = large_req_time;
        cap_a = 0;
        start  = 0;
        for (i = best_j; i > 0; i--, start = end) {
            end = start + (num_pos - start) / i;

            cur_a = new_a[i - 1];
            buf_a = buf_array[best_a];
            BUFFER(cur_a)->type        = BUF;
            BUFFER(cur_a)->impl.buffer = buf_a;
            sp_implement_buffer_chain(network, cur_a);

            load = 0;
            cur_req = large_req_time;
            for (l = start; l < end; l++) {
                load += fanouts[l].load;
                MIN_DELAY(cur_req, cur_req, fanouts[l].req);
            }
            sp_subtract_delay(buf_a->phase, buf_a->block, buf_a->drive,
                              load, &cur_req);

            /* Keep track of the earliest required of the "a" inverters */
            MIN_DELAY(req_a, req_a, cur_req);

            cap_a += buf_a->ip_load + auto_route;

            BUFFER(cur_a)->load     = load;
            BUFFER(cur_a)->req_time = cur_req;
            BUFFER(cur_a)->cfi      = 0;
            BUFFER(cur_a)->prev_ph  = buf_b->phase;
            BUFFER(cur_a)->prev_dr  = buf_b->drive;
        }

        /* Now annotate the node "new_b" itself */
        BUFFER(new_b)->type        = BUF;
        BUFFER(new_b)->impl.buffer = buf_b;
        sp_implement_buffer_chain(network, new_b);
        req_b = req_a;
        cap_b = buf_b->ip_load + auto_route;
        sp_subtract_delay(buf_b->phase, buf_b->block, buf_b->drive,
                          cap_a, &req_b);
        BUFFER(new_b)->load     = load;
        BUFFER(new_b)->req_time = req_b;
        BUFFER(new_b)->cfi      = 0;
        BUFFER(new_b)->prev_ph  = prev_ph;
        BUFFER(new_b)->prev_dr  = prev_dr;
    }

    /* Now to update the required time data for the root gate itself */
    MIN_DELAY(req_g, req_b, req_gI);
    load = cap_b + cap_gI;
    sp_subtract_delay(prev_ph, prev_bl, prev_dr, load, &req_g);
    BUFFER(node)->load       = load;
    BUFFER(node)->req_time   = req_g;

    if (buf_param->debug > 10) {
        (void) fprintf(sisout, "Trans2 data ---\n");
        (void) fprintf(sisout, "\t%d groups of positive signals - a=%d, b=%d\n",
                       best_j, best_a, best_b);
        (void) fprintf(sisout, "\t%d partitions of negative signals -- gI=%d\n",
                       best_i, best_gI);
        (void) fprintf(sisout, "\treq_a = %6.3f:%-6.3f, req_b = %6.3f:%-6.3f\n",
                       req_a.rise, req_a.fall, req_b.rise, req_b.fall);
        (void) fprintf(sisout, "\treq_gI = %6.3f:%-6.3f, req_g = %6.3f:%-6.3f\n",
                       req_gI.rise, req_gI.fall, req_g.rise, req_g.fall);
    }
}



