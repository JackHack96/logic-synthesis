
#include "sis.h"
#include "speed_int.h"
#include <math.h>

int sp_compare_cutset_nodes();

static int new_speed_recur();

static void sp_compute_delta();

static void nsp_set_initial_load();

static void sp_set_updated_constraints();

static void sp_do_initial_resynthesis();

static network_t *sp_generate_network_from_array();

static array_t *speed_filter_cutset();

static array_t *new_speed_collapse_bfs();

static delay_time_t nsp_get_input_drive();

/* MACRO to compare two delay_time_t structures */
#define SP_DELAY_IMPROVED(fanout_flag, old, new) \
(fanout_flag != CLP ? \
 (new.rise > old.rise+NSP_EPSILON && new.fall > old.fall+NSP_EPSILON) : \
 (new.rise < old.rise-NSP_EPSILON && new.fall < old.fall-NSP_EPSILON))

/*
 * Interface routine that will take the appropriate arguments and shove them
 * into the appropriate data-structure that is used by routines in new_speed
 */
/* ARGSUSED */
int
new_speed(network, speed_param)
        network_t *network;
        speed_global_t *speed_param;
{
    int      status;
    double   saved_value, slack_diff;
    st_table *table;

    if (network_num_pi(network) == 0) return 0;    /* No improvement possibe */
    /*
     * Assume that a valid delay trace has been performed....
     * Use the required time at the o/p as the  user-supplied times
     * so that we are consistent in computing the slacks. Also the
     * initial threshold is set adaptively (based on slacks)...
     */
    saved_value = speed_param->thresh;
    if (speed_param->new_mode && nsp_first_slack_diff(network, &slack_diff)) {
        speed_param->thresh = 2 * slack_diff;
    }
    table  = speed_store_required_times(network);
    status = new_speed_recur(network, speed_param, 0);
    speed_restore_required_times(table);
    speed_param->thresh = saved_value;

    return status;
}

/*
 * Actual routine that implements the idea that the side paths can be sped up
 * to yeild greater saving allong the critical path. The algorithm is
 * recursive and at each stage the network is carved out into a critical
 * and non critical region --- recursion being done on the non-critical
 * region with appropriate delay constraints
 *
 * In addition the selection mechanism for the local transformations to
 * apply is better compared to the (-f option) of speedup...
 */

static int
new_speed_recur(network, speed_param, level)
        network_t *network;
        speed_global_t *speed_param;
        int level;
{
    int          i, j;
    sp_clp_t     *rec;
    lsGen        gen, gen1;
    sp_weight_t  *wght;
    sp_xform_t   *ti_model;
    network_t    *opt_netw, *orig_netw;
    st_generator *stgen;
    char         *key, *value;
    double       delta, epsilon;
    node_t       *po, *fo, *fanin, *fanin_in, *fan, *np;
    node_t       *node, *new_pi, *new_po, *input;
    delay_time_t old_arr, old_req, old, new;
    array_t      *mincut, *network_array, *new_roots;
    st_table     *equiv_name_table, *clp_table, *select_table;
    int          is_special_case;
    int          recur_helped, should_recur, need_recursion, success, delay_improved;

    success = FALSE;

    if (level >= speed_param->max_recur) {
        if (speed_param->debug > 1) (void) fprintf(sisout, "HIT RECURSION LIMIT\n");
        return success;
    }
    /*
     * Since at the last recursion we do not need to doe the extra work
     * required for computing the constraints for recursion
     */
    should_recur = ((level + 1) < speed_param->max_recur);

    if (speed_param->debug > 1)
        (void) fprintf(sisout, "\n<<<< RECURSION LEVEL %d\n", level);

    if (speed_param->model == DELAY_MODEL_MAPPED &&
        !lib_network_is_mapped(network)) {
        (void) fprintf(siserr, "ERROR: initial network not mapped \n");
    }
    /*
     * Identify the epsilon critical network and the cutsets to improve
     */
    delay_trace(network, speed_param->model);
    set_speed_thresh(network, speed_param);
    clp_table = st_init_table(st_ptrcmp, st_ptrhash);
    new_speed_compute_weight(network, speed_param, clp_table, &epsilon);

    if (epsilon < NSP_EPSILON) {
        if (speed_param->debug) {
            (void) fprintf(sisout, "No saving possible at this stage\n");
        }
        new_free_weight_info(clp_table);
        return -1; /* Indicates no futher impr possible or required*/
    }

    /* Use the new formulation of evaluating all cuts */
    mincut = new_speed_select_xform(network, speed_param,
                                    clp_table, epsilon);
    /*  Reset the caches that were used for efficiency */
    nsp_free_buf_param();

    if (mincut == NIL(array_t)) {
        if (speed_param->model == DELAY_MODEL_MAPPED) {
            (void) com_execute(&network, "write_blif -n /usr/tmp/sp_fail.blif");
        } else {
            (void) com_execute(&network, "write_blif /usr/tmp/sp_fail.blif");
        }
        fail("ERROR: no mincut :: Saved network in /usr/tmp/sp_fail.blif");
    } else if (array_n(mincut) == 0) {
        /* There was no selection that gave a positive saving --- all the
           selections interacted so as to overcome the predicted saving */
        if (speed_param->debug) {
            (void) fprintf(sisout, "All selections were rejected !!!\n");
        }
        new_free_weight_info(clp_table);
        return -1; /* Indicates no futher impr possible or required*/
    }

    /* Just see if there are nodes where transformations result in a delay
     * saving with a decrease in area. Accept these anyway !!!! 
     */
    sp_expand_selection(speed_param, mincut, clp_table, &select_table);

    if (speed_param->debug > 1) {
        (void) fprintf(sisout, "Original selection\n");
        for (i = 0; i < array_n(mincut); i++) {
            node = array_fetch(node_t * , mincut, i);
            (void) fprintf(sisout, "  %s", node_long_name(node));
        }
        (void) fprintf(sisout, "\n");
    }

    /* Check if all the nodes in the selection set are valid ones */
    speed_reorder_cutset(speed_param, clp_table, &mincut);

    if (speed_param->debug) {
        (void) fprintf(sisout, "Revised selection order\n");
        for (i = 0; i < array_n(mincut); i++) {
            node = array_fetch(node_t * , mincut, i);
            (void) st_lookup(clp_table, (char *) node, (char **) &wght);
            j = wght->best_technique;
            (void) fprintf(sisout, "\t%10s :d=(%.2f) a=%6.2f %s\n",
                           node_long_name(node),
                           wght->improvement[j], wght->area_cost[j],
                           (wght->can_invert ? "CAN_INV" : "NO-INVERT"));
        }
    }

    if (speed_param->model == DELAY_MODEL_MAPPED &&
        !lib_network_is_mapped(network)) {
        (void) fprintf(siserr, "ERROR: network not mapped after selection\n");
    }

    /*
     * Save the configuration in case we need to restore it later
     * make dummy PI/PO's so that the resynthesized sections can be removed
     */
    network_array = array_alloc(sp_clp_t * , 0);
    for (i        = 0; i < array_n(mincut); i++) {
        node = array_fetch(node_t * , mincut, i);
        assert(st_lookup(clp_table, (char *) node, (char **) &wght));
        ti_model = sp_local_trans_from_index(speed_param, wght->best_technique);

        /* Restore the value of "crit_slack" to be the one used earlier */
        speed_param->crit_slack = wght->crit_slack;

        /*
         * Keep a single structure to record the neccessary info like
         * the original configuration, etc.
         */

        rec = sp_create_collapse_record(node, wght, speed_param, ti_model);
        if (ti_model->type == FAN) {
            rec->cfi = wght->cfi;
            rec->old = wght->cfi_req;
        }
        array_insert_last(sp_clp_t * , network_array, rec);

        /*
         * Now remove all the  nodes from the original netw that correspond
         * to the resynthesized sections ,adding PI & PO nodes appropriately
         * First deal with the PI's of the sub-network (PO's to be added
         * to the network)
         */
        rec->equiv_table = st_init_table(strcmp, st_strhash);
        sp_delete_network(speed_param, network, node, rec->net, select_table,
                          clp_table, rec->equiv_table);

        /* Do an initial decompostion of the collapsed network */
        if (speed_param->debug > 2) {
            (void) fprintf(sisout, "^^^^ ORIGINAL CONFIGURATION at %s\n",
                           node->name);
            sp_print_network(rec->net, "Before Initial Resynthesis");
        }
        sp_do_initial_resynthesis(rec);
        if (speed_param->debug > 2) {
            sp_print_network(rec->net, "After Initial Resynthesis");
        }

    }
    /*  Reset the caches that were used during local transformations */
    nsp_free_buf_param();

    /* Cleanup any remaining nodes that do not fanout or are not driven */
    network_ccleanup(network);

    /*
     * find the "delta" value by which side inputs need to be improved 
     * We shall also consider the additional loading that may need to be
     * driven due to the additional resynthesis at the selected nodes...
     */
    for (i = array_n(network_array); should_recur && i-- > 0;) {
        sp_compute_delta(network, speed_param, network_array, i);
    }

    if (speed_param->debug > 3) {
        sp_print_network(network, "After applying transforms\n");
    }

    new_free_weight_info(clp_table);

    /* If there is no node to be decomposed,  return. */
    if (array_n(network_array) == 0) goto free_and_return;

    if (speed_param->debug > 1) {
        (void) fprintf(sisout, "CUTSET TRIES improvement at %d nodes\n",
                       array_n(network_array));
    }

    /*
     * Add appropriate timing constraints on the dummy PI/PO's based on 
     * the delta values that have been computed for them  ....
     */
    need_recursion = FALSE;
    for (i         = 0; should_recur && i < array_n(network_array); i++) {
        rec = array_fetch(sp_clp_t * , network_array, i);

        if (rec->ti_model->type != CLP) {
            /* Set the new timing constraint for the new PI's that correspond
               to the PO's of the network being resynthesized */
            j = 1;
            foreach_primary_output(rec->net, gen, po)
            {
                (void) st_lookup(rec->equiv_table, po->name, (char **) &new_pi);
                delta = array_fetch(
                double, rec->delta, j++);
                delta          = MAX(0.0, delta);
                need_recursion = (need_recursion || (delta > 0));

                assert(delay_get_po_required_time(po, &old_req));
                delay_set_parameter(new_pi, DELAY_ARRIVAL_RISE, old_req.rise + delta);
                delay_set_parameter(new_pi, DELAY_ARRIVAL_FALL, old_req.fall + delta);
            }
        } else {
            /* Set the appropriate required time for PO's that correspond
             * to the inputs of the reynthesized network */
            j = 1;
            foreach_primary_input(rec->net, gen, input)
            {
                (void) st_lookup(rec->equiv_table, input->name, (char **) &new_po);
                delta = array_fetch(
                double, rec->delta, j++);
                delta          = MAX(0.0, delta);
                need_recursion = (need_recursion || (delta > 0));

                assert(delay_get_pi_arrival_time(input, &old_arr));
                delay_set_parameter(new_po, DELAY_REQUIRED_RISE, old_arr.rise - delta);
                delay_set_parameter(new_po, DELAY_REQUIRED_FALL, old_arr.fall - delta);
            }
        }
    }


    if (speed_param->model == DELAY_MODEL_MAPPED &&
        !lib_network_is_mapped(network)) {
        (void) fprintf(siserr, "ERROR: network not mapped before recursion\n");
    }

    /* Now recur on this new network generated with appropriate constraints */
    if (need_recursion) {
        recur_helped = new_speed_recur(network, speed_param, level + 1);
    }

    /* 
     * After the recursion, use the correct delay values to resynthesize
     * the original sections that were to be sped up
     */
    if (need_recursion) {
        if (speed_param->debug > 1) {
            (void) fprintf(sisout, "RECURSION %s\n",
                           (recur_helped ? "HELPED" : "DID NOT HELP AT ALL"));
        }
        assert(delay_trace(network, speed_param->model));
        for (i = 0; i < array_n(network_array); i++) {
            rec = array_fetch(sp_clp_t * , network_array, i);
            if (recur_helped) {
                if (speed_param->debug > 1) {
                    (void) fprintf(sisout, "Redecomposing %s\n", rec->name);
                }
                j = speed_param->debug;
                speed_param->debug = FALSE;
                sp_set_updated_constraints(rec, speed_param, network);
                opt_netw = (*rec->ti_model->optimize_func)(rec->net,
                                                           rec->ti_model, rec->glb);
                network_free(rec->net);
                rec->net           = opt_netw;
                speed_param->debug = j;
            }
        }
        /*  Reset the caches that were used for efficiency */
        nsp_free_buf_param();
    }
    if (speed_param->model == DELAY_MODEL_MAPPED &&
        !lib_network_is_mapped(network)) {
        (void) fprintf(siserr, "ERROR: network not mapped before check\n");
    }

    /*
     * Check the decomposed network and the original configuration.
     * Keep the one with smaller delay around ....
     */
    for (i = 0; i < array_n(network_array); i++) {
        rec = array_fetch(sp_clp_t * , network_array, i);
        /*
         * Get the performance of the optimized circuits ...
         */
        if (rec->ti_model->type == FAN) {
            np  = network_get_pi(rec->net, rec->cfi);
            new = delay_required_time(np);
            /* HERE -- should account for load of PREV gate !!! */
        } else if (rec->ti_model->type == DUAL) {
            fail("Do not know how to handle DUAL networks yet");
            np  = network_get_pi(rec->net, rec->cfi);
            new = delay_required_time(np);
        } else {
            new = (*rec->ti_model->arr_func)
                    (network_get_po(rec->net, 0), rec->glb);
            new_speed_adjust_po_arrival(rec, speed_param->model, &new);
        }
        old = rec->old;

        /*
          if (speed_param->debug > 1){
          (void)fprintf(sisout, "\tl=%d IMPROVEMENT at %s= %6.3f:%-6.3f\n",
          level, rec->name, t.rise, t.fall);
          (void)fprintf(sisout, "\t\tAt %s AFTER RECURSION %.2f:%-.2f\n",
          rec->name, new.rise, new.fall);
          }
          */
        /* Only accept the decomposition if it saves delay */
        if (!SP_DELAY_IMPROVED(rec->ti_model->type, old, new)) {
            delay_improved = FALSE;
            orig_netw      = rec->orig_config;
            if (speed_param->debug > 2) {
                (void) fprintf(sisout, "---- USING ORIGINAL CONFIGURATION\n");
                (void) com_execute(&orig_netw, "print");
                (void) fprintf(sisout, "---- INSTEAD OF\n");
                (void) com_execute(&(rec->net), "print");
            }

            /* Use the stored configuration instead of the new one */
            network_free(rec->net);
            rec->net = speed_network_dup(orig_netw);
            new = rec->old; /* Just for printing */
        } else {
            delay_improved = TRUE;
            if (speed_param->debug > 2) {
                (void) fprintf(sisout, "++++ USING NEW CONFIGURATION\n");
                (void) com_execute(&(rec->net), "print");
            }
            success = TRUE;
        }

        if (speed_param->debug > 1) {
            (void) fprintf(sisout, "At %s FINAL %s %s %.2f:%-.2f\n",
                           rec->name, (delay_improved ? "IMPROVED" : "ORIGINAL"),
                           (rec->ti_model->type == FAN ? "REQUIRED" :
                            (rec->ti_model->type == DUAL ? "SLACK" : "ARRIVAL")),
                           new.rise, new.fall);
        }
    }

    if (speed_param->model == DELAY_MODEL_MAPPED &&
        !lib_network_is_mapped(network)) {
        (void) fprintf(siserr, "ERROR: network not mapped before append\n");
    }

    /* Add the resynthesized sections to the original network */
    new_roots        = array_alloc(node_t * , 0);
    equiv_name_table = st_init_table(strcmp, st_strhash);
    for (i           = 0; i < array_n(network_array); i++) {
        rec = array_fetch(sp_clp_t * , network_array, i);
        sp_append_network(speed_param, network, rec->net, rec->ti_model,
                          equiv_name_table, new_roots);
        nsp_free_collapse_record(rec);
    }
    /* Free the table storing the equivalence between nodes */
    st_foreach_item(equiv_name_table, stgen, &key, &value)
    {
        FREE(key);
        FREE(value);
    }
    st_free_table(equiv_name_table);

    if (speed_param->model == DELAY_MODEL_MAPPED &&
        !lib_network_is_mapped(network)) {
        (void) fprintf(siserr, "ERROR: network becomes mapped after append\n");
    }

    if (speed_param->model != DELAY_MODEL_MAPPED) {
        for (i = 0; i < array_n(new_roots); i++) {
            node = array_fetch(node_t * , new_roots, i);
            if ((node_num_fanin(node) == 1) &&
                (new_speed_is_fanout_po(node) == FALSE)) {
                if (speed_param->debug > 2) {
                    (void) fprintf(sisout, "Collapsing added root with single fanin\n");
                }
                speed_delete_single_fanin_node(network, node);
            }
        }
    }
    array_free(new_roots);
    /* 
     * Due to the collapsing of the inverters, some
     * primary o/p may become buffers. Remove these.
     */
    foreach_primary_output(network, gen, fo)
    {
        fanin = node_get_fanin(fo, 0);
        if (node_function(fanin) == NODE_BUF &&
            lib_gate_of(fanin) == NIL(lib_gate_t)) {
            fanin_in = node_get_fanin(fanin, 0);
            assert(node_patch_fanin(fo, fanin, fanin_in));
            foreach_fanout(fanin, gen1, fan)
            {
                (void) node_collapse(fan, fanin);
            }
            if (node_num_fanout(fanin) == 0)
                speed_network_delete_node(network, fanin);
        }
    }
    st_free_table(select_table);

    /* There is no harm in calling an area recovery step */
#if 0
    if (nsp_downsize_non_crit_gates(network, speed_param->model) &&
    1 /* speed_param->debug */) {
    (void)fprintf(sisout, "Downsizing helped !!!\n");
    }
#endif

    free_and_return:
    /*
     * Free all the configurations that were stored 
     */
    array_free(mincut);
    array_free(network_array);
    if (speed_param->debug > 1) {
        (void) fprintf(sisout, ">>>> END RECURSION LEVEL %d\n\n", level);
    }
    return success;
}
/*
 * Routine that will add area-saving transformations to the selections set
 */

void
sp_expand_selection(speed_param, mincut, clp_table, select_table)
        speed_global_t *speed_param;
        array_t *mincut;
        st_table *clp_table;      /* table of weigths */
        st_table **select_table; /* RETURN: a table of the nodes finally used */
{
    int          i, k;
    node_t       *node;
    sp_weight_t  *wght;
    st_generator *stgen;

    *select_table = st_init_table(st_ptrcmp, st_ptrhash);
    for (i = 0; i < array_n(mincut); i++) {
        node = array_fetch(node_t * , mincut, i);
        (void) st_insert(*select_table, (char *) node, NIL(
        char));
    }
    k      = st_count(*select_table);
    if (speed_param->model != DELAY_MODEL_MAPPED) {
        st_foreach_item(clp_table, stgen, (char **) &node, (char **) &wght)
        {
            if (!st_is_member(*select_table, (char *) node) &&
                SP_IMPROVEMENT(wght) > NSP_EPSILON && SP_COST(wght) < 0) {
                array_insert_last(node_t * , mincut, node);
                (void) st_insert(*select_table, (char *) node, NIL(
                char));
            }
        }
        if ((array_n(mincut) > k)/* && speed_param->debug*/) {
            (void) fprintf(sisout, "INFO: Added %d nodes for area saving\n",
                           array_n(mincut) - k);
        }
    }
}
/* Encapsulation that orders the nodes in a selection and also deletes
 * the nodes that it thinks are not required !!!
 */
void
speed_reorder_cutset(speed_param, clp_table, mincut)
        speed_global_t *speed_param;
        st_table *clp_table;
        array_t **mincut;
{
    array_t *new_mincut;

    new_mincut = speed_filter_cutset(*mincut, speed_param, clp_table);
    array_free(*mincut);
    *mincut = sp_generate_revised_order(new_mincut, speed_param, clp_table);
    array_free(new_mincut);
}
/*
 * This routine computes the amount "delta" by which the non-critical
 * paths need to be sped up... Works for both the fanin or fanout based opt.
 * The returned data contains the deltas for the PI's and PO's of the resynth
 * regions based on the type of optimization ....
 */
static void
sp_compute_delta(network, speed_param, network_array, current_pos)
        network_t *network;
        speed_global_t *speed_param;
        array_t *network_array;
        int current_pos;
{
    int           i;
    double        load;
    array_t       *temp;
    lsGen         gen, gen1;
    node_t        *node, *orig, *orig_fi, *pi;
    double        min_slack, delta;
    delay_pin_t   *pin_delay;
    delay_time_t  t, new, slack, new_slack;
    delay_model_t model = speed_param->model;
    sp_clp_t      *rec  = array_fetch(sp_clp_t * , network_array, current_pos);
    sp_clp_t      *temp_rec;

    rec->delta = array_alloc(
    double, 0);
    if (rec->ti_model->type == CLP) {
        new = (*rec->ti_model->arr_func)
                (node_get_fanin(network_get_po(rec->net, 0), 0));
        new_speed_adjust_po_arrival(rec, rec->glb->model, &new);

        /* Compute the delta for the output: Difference between arrival times */
        delta = MIN((new.rise - rec->old.rise), (new.fall - rec->old.fall));
        array_insert_last(
        double, rec->delta, delta);

        /* Record original slacks of the inputs */
        temp = array_alloc(
        double, 0);
        foreach_primary_input(rec->net, gen, node)
        {
            orig = nsp_network_find_node(network, node_long_name(node));
            if (orig != NIL(node_t)) {
                load      = 0.0;
                for (i    = array_n(network_array); i-- > 0;) {
                    if (i != current_pos) {
                        temp_rec = array_fetch(sp_clp_t * , network_array, i);
                        foreach_primary_input(temp_rec->net, gen1, pi)
                        {
                            if (!strcmp(node_long_name(node), node_long_name(pi))) {
                                load += delay_load(pi);
                            }
                        }
                    }
                }
                pin_delay = get_pin_delay(node, 0, model);
                t.rise = pin_delay->drive.rise * load;
                t.fall = pin_delay->drive.fall * load;
                slack     = delay_slack_time(orig);
                min_slack = MIN((slack.rise - t.rise), (slack.fall - t.fall));
                array_insert_last(
                double, temp, min_slack);
            } else {
                fail("Failed to find node corresponding to input ");
            }
        }

        /* Compute updated slacks using the original arrival time
         * at the output.
         */
        node = network_get_po(rec->net, 0);
        delay_set_parameter(node, DELAY_REQUIRED_RISE, rec->old.rise);
        delay_set_parameter(node, DELAY_REQUIRED_FALL, rec->old.fall);
        delay_trace(rec->net, model);

        /* Compute deltas for the inputs. */
        /* delta = old_slack - new_slack  */
        i = 0;
        foreach_primary_input(rec->net, gen, node)
        {
            slack     = delay_slack_time(node);
            min_slack = MIN(slack.rise, slack.fall);
            delta     = array_fetch(
            double, temp, i++) -min_slack;
            array_insert_last(
            double, rec->delta, delta);
        }

        array_free(temp);

    } else {
        /* HERE --- do the same for the fanout delta : input req times*/
        delay_wire_required_time(network_get_pi(rec->net, rec->cfi),
                                 0, speed_param->model, &new);
        /* Compute the delta for the critical input */
        delta = MIN((rec->old.rise - new.rise), (rec->old.fall - new.fall));
        array_insert_last(
        double, rec->delta, delta);

        /* Record original slacks of the outputs */
        temp = array_alloc(
        double, 0);
        foreach_primary_output(rec->net, gen, node)
        {
            slack     = delay_slack_time(node);
            min_slack = MIN(slack.rise, slack.fall);
            array_insert_last(
            double, temp, min_slack);
        }

        /* Compute updated slacks using the original required time
         * as the input arrival time.
         */
        node = network_get_pi(rec->net, rec->cfi);
        delay_set_parameter(node, DELAY_ARRIVAL_RISE, rec->old.rise);
        delay_set_parameter(node, DELAY_ARRIVAL_FALL, rec->old.fall);
        delay_trace(rec->net, model);

        /* Compute deltas for the inputs. */
        /* delta = old_slack - new_slack  */
        i = 0;
        foreach_primary_output(rec->net, gen, node)
        {
            slack     = delay_slack_time(node);
            min_slack = MIN(slack.rise, slack.fall);
            delta     = array_fetch(
            double, temp, i++) -min_slack;
            array_insert_last(
            double, rec->delta, delta);
        }
        array_free(temp);
    }
    if (speed_param->debug > 1) {
        (void) fprintf(sisout, "At %s INITIAL %.2f to %.2f DELTA %.2f\n",
                       rec->name, rec->old.rise, new.rise,
                       array_fetch(
        double, rec->delta, 0));
    }
}

/*
 * Depending on the adjustments made to the delays (due to load considerations)
 * stored in "rec->adjust", the routine will change the arrival times of PI 's
 */
static void
sp_set_updated_constraints(rec, speed_param, network)
        sp_clp_t *rec;
        speed_global_t *speed_param;
        network_t *network;
{
    node_t       *node, *orig;
    delay_time_t *t, old_const, new_const;
    int          improved = 0;
    lsGen        gen;

    if (rec->ti_model->type != CLP) return;

    foreach_primary_input(rec->net, gen, node)
    {
        orig = network_find_node(network, node_long_name(node));
        if (orig != NIL(node_t)) {
            new_const = delay_arrival_time(orig);
            if (speed_param->debug > 1) {
                speed_delay_arrival_time(node, speed_param, &old_const);
                improved += ((old_const.rise > new_const.rise) &&
                             (old_const.fall > new_const.fall));
            }
            if (st_lookup(rec->adjust, (char *) orig, (char **) &t)) {
                new_const.rise -= t->rise;
                new_const.fall -= t->fall;
            }
            delay_set_parameter(node, DELAY_ARRIVAL_RISE, new_const.rise);
            delay_set_parameter(node, DELAY_ARRIVAL_FALL, new_const.fall);
        }
    }
    if (speed_param->debug > 1) {
        (void) fprintf(sisout, "**** For %s --- %d of %d side paths improved\n",
                       rec->name, improved, network_num_pi(rec->net));
    }
}

/*
 * For the nodes in the d-critical-fanin, make a network that represents
 * the original configuration of the collapsed node. This is required
 * since we may want to reject the result of resynthesis.
 */
network_t *
sp_get_network_to_collapse(f, speed_param, areap)
        node_t *f;
        speed_global_t *speed_param;
        double *areap;            /* Returns the duplicated area */
{
    network_t *net;
    array_t   *temp_array;   /* Temp array for bfs implementation */
    st_table  *table, *dup_nodes;

    if (f->type != INTERNAL) {
        return NIL(network_t);
    }

    table      = st_init_table(st_ptrcmp, st_ptrhash);
    temp_array = new_speed_collapse_bfs(f, speed_param, table);

    dup_nodes = st_init_table(st_ptrcmp, st_ptrhash);
    *areap = sp_compute_duplicated_area(temp_array, table, dup_nodes,
                                        DELAY_MODEL_UNKNOWN);

    /* 
     * Make a network consisting of the nodes in the "temp_aray". The 
     * primary inputs of this network are the fanins of the collapsed node.
     */
    net = sp_generate_network_from_array(temp_array, speed_param);

    st_free_table(dup_nodes);
    st_free_table(table);
    array_free(temp_array);
/*
    sprintf(dump, "write_blif /usr/tmp/test/test%d.blif", num++);
    com_execute(&net, dump);
*/
    return net;
}
/*
 * Set the amount of contribution of each input that is already in
 * consideration when the load of the input node was computed. Used
 * to evaluate the interaction of the different transformations
 * NOTE: Overloading of the "max_ip_load" field...
 */
static void
nsp_set_initial_load(orig_netw, net, speed_param, dup_nodes)
        network_t *orig_netw, *net;
        speed_global_t *speed_param;
        st_table *dup_nodes;
{
    int         pin;
    lsGen       gen, gen1;
    node_t      *pi, *orig, *fo;
    delay_pin_t *pin_delay;
    double      load;

    foreach_primary_input(net, gen, pi)
    {
        load = 0.0;
        foreach_fanout_pin(pi, gen1, fo, pin)
        {
            orig = network_find_node(orig_netw, node_long_name(fo));
            if (orig != NIL(node_t) && !st_is_member(dup_nodes, (char *) orig)) {
                pin_delay = get_pin_delay(fo, pin, speed_param->model);
                load += pin_delay->load;
            }
        }
        delay_set_parameter(pi, DELAY_MAX_INPUT_LOAD, load);
    }
}
/*
 * Returns an array of nodes that are within a distance "dist" along the
 * critical paths of "f". "table" also record these elements, When the
 * region_flag is set to ONLY_TREE, the distance metric is the number of
 * trees along the critical path that are collapsed
 */
static array_t *
new_speed_collapse_bfs(f, speed_param, table)
        node_t *f;
        speed_global_t *speed_param;
        st_table *table;
{
    int          i, j, k;
    node_t       *temp, *inv_fi, *fi, *new_fanin;
    array_t      *temp_array;        /* Temp array for bfs implementation */
    int          should_add, need_to_add;
    int          new_dist, cur_dist, dist = speed_param->dist;
    int          more_to_come, first      = 0, last; /* bfs traversal flags */
    double       min_arr, max_arr;
    delay_time_t a_t;

    temp_array = array_alloc(node_t * , 0);
    array_insert_last(node_t * , temp_array, f);
    if (speed_param->region_flag == ONLY_TREE) {
        (void) st_insert(table, (char *) f, (char *) dist);
    } else {
        (void) st_insert(table, (char *) f, (char *) (dist + 1));
    }

    more_to_come = TRUE;
    while (more_to_come) {
        more_to_come = FALSE;
        last         = array_n(temp_array);
        for (j       = first; j < last; j++) {
            temp = array_fetch(node_t * , temp_array, j);
            assert(st_lookup(table, (char *) temp, (char **) &cur_dist));

            /* When this node reaches 1 => no further traversal required
             * unless it is a tree in which case the tree is to be traversed
             * NOTE: for the ONLY_TREE case, the termination of bfs  will
             * occur when there are no more gates in trees...
             */
            if (speed_param->region_flag != ONLY_TREE && cur_dist == 1) {
                continue;
            }
            if (speed_param->region_flag == ONLY_TREE && cur_dist < 0) {
                continue;
            }

            foreach_fanin(temp, k, fi)
            {
                if (fi->type == INTERNAL &&
                    !st_is_member(table, (char *) fi) /* Not yet visited */) {

                    /* Depending on the select-flag add it */
                    should_add = TRUE;
                    new_dist   = cur_dist;
                    if (speed_param->region_flag != TRANSITIVE_FANIN) {
                        if (!speed_critical(fi, speed_param)) {
                            /* not along the  critical path */
                            if (speed_param->region_flag != ONLY_TREE) {
                                should_add = FALSE;
                            } else if (node_num_fanout(fi) > 1) {
                                should_add = FALSE; /* Not part of tree */
                            } else {
                                /* No more traversal !!! Not even this tree */
                                new_dist = -1;
                            }
                        } else if (speed_param->region_flag == ONLY_TREE) {
                            if (cur_dist <= 0 && node_num_fanout(fi) > 1 &&
                                node_num_fanin(fi) > 1) {
                                /* complex critical input at tree edge !!! */
                                should_add = FALSE;
                            }
                        }
                    }
                    if (should_add) {
                        /*
                         * Add to list and set the correct distance in table
                         */
                        if (speed_param->region_flag == ONLY_TREE) {
                            if (node_num_fanout(fi) == 1) {
                                /* Distance is unchanged */
                                (void) st_insert(table, (char *) fi,
                                                 (char *) (new_dist));
                                array_insert_last(node_t * , temp_array, fi);
/*
     (void)fprintf(sisout, "%s -> %s(%d) : fo= %d, fi = %d\n", temp->name,
      fi->name, new_dist, node_num_fanout(fi), node_num_fanin(fi));
*/
                            } else if (cur_dist >= 0) {
                                /*
                                 * Traverse any chains of inverters adding
                                 * (if appropriate) to temp array
                                 */
                                need_to_add = TRUE;
                                while (node_num_fanin(fi) == 1) {
                                    if (st_is_member(table, (char *) fi)) {
                                        need_to_add = FALSE;
                                        break;
                                    }
                                    (void) st_insert(table, (char *) fi,
                                                     (char *) (new_dist - 1));
                                    array_insert_last(node_t * , temp_array, fi);
/*
     (void)fprintf(sisout, "%s -> INV %s(%d) : fo= %d, fi = %d\n", temp->name,
      fi->name, new_dist-1,node_num_fanout(fi), node_num_fanin(fi));
*/
                                    inv_fi = node_get_fanin(fi, 0);
                                    fi     = inv_fi;
                                }
                                if (fi->type == PRIMARY_INPUT ||
                                    st_is_member(table, (char *) fi) ||
                                    cur_dist == 0) {
                                    need_to_add = FALSE;
                                }

                                if (need_to_add) {
                                    /* Reduce distance since a tree is crossed */
                                    array_insert_last(node_t * , temp_array, fi);
                                    (void) st_insert(table, (char *) fi,
                                                     (char *) (new_dist - 1));
/*
     (void)fprintf(sisout, "%s -> %s(%d) : fo= %d, fi = %d\n", temp->name,
      fi->name, new_dist-1,node_num_fanout(fi), node_num_fanin(fi));
*/
                                }
                            }
                        } else {
                            (void) st_insert(table, (char *) fi, (char *) (new_dist - 1));
                            array_insert_last(node_t * , temp_array, fi);
                        }
                        more_to_come = TRUE;
                    }
                }
            }
        }
        first        = last;
    }

    /* COMPROMISE looks at the inputs of the critical region to include */
    if (speed_param->region_flag == COMPROMISE) {
        /* determine the range of arrival times of the critical region */
        min_arr      = POS_LARGE;
        max_arr      = NEG_LARGE;
        for (i       = 0; i < array_n(temp_array); i++) {
            temp = array_fetch(node_t * , temp_array, i);
            foreach_fanin(temp, j, fi)
            {
                if (!st_is_member(table, (char *) fi)) {
                    a_t     = delay_arrival_time(fi);
                    min_arr = MIN(min_arr, MIN(a_t.rise, a_t.fall));
                    max_arr = MAX(max_arr, MAX(a_t.rise, a_t.fall));
                }
            }
        }
        /* Add the approporiate nodes to the critical region */
        first        = 0;
        more_to_come = TRUE;
        while (more_to_come) {
            more_to_come = FALSE;
            last         = array_n(temp_array);
            for (i       = first; i < last; i++) {
                temp = array_fetch(node_t * , temp_array, i);
                assert(st_lookup(table, (char *) temp, (char **) &cur_dist));
                foreach_fanin(temp, j, fi)
                {
                    if (!st_is_member(table, (char *) fi) &&
                        cur_dist > 1) { /* Distance limit is not exceeded */
                        /*
                         * Input to the critical region -- Check if the delay
                         * data warrants an addition into the critical region
                         */
                        should_add = FALSE;
                        foreach_fanin(fi, k, new_fanin)
                        {
                            a_t = delay_arrival_time(new_fanin);
                            if (a_t.rise >= min_arr && a_t.fall >= min_arr) {
                                should_add = TRUE;
                            }
                        }
                        if (should_add) {
                            array_insert_last(node_t * , temp_array, fi);
                            st_insert(table, (char *) fi, (char *) (cur_dist - 1));
                        }
                    }
                }
            }
            first        = last;
        }
        if (speed_param->debug > 2) {
            (void) fprintf(sisout, "At %s COMPROMISE added %d nodes in [%.2f %.2f]\n",
                           node_name(f), (array_n(temp_array) - last), min_arr, max_arr);
            for (i = last; i < array_n(temp_array); i++) {
                temp = array_fetch(node_t * , temp_array, i);
                node_print(sisout, temp);
            }
        }
    }

    return temp_array;
}
/*
 * Create a network containing the nodes in "node_array".
 */
static network_t *
sp_generate_network_from_array(node_array, speed_param)
        array_t *node_array;    /* Nodes to be collapsed into  node "f" */
        speed_global_t *speed_param;
{
    int          i, j;
    char         *name;
    double       load;
    network_t    *net;
    lib_gate_t   *gate;
    delay_time_t drive, a_t;
    st_table     *gate_table, *dup_table, *pi_table;
    node_t       *node, *po_node, *pi_node, *dup_node, *dup_fi, *fi;

    net        = network_alloc();
    dup_table  = st_init_table(st_ptrcmp, st_ptrhash);
    gate_table = st_init_table(st_ptrcmp, st_ptrhash);
    pi_table   = st_init_table(st_ptrcmp, st_ptrhash);

    /* Visit nodes in reverse order -- inputs to outputs */
    for (i = array_n(node_array); i-- > 0;) {
        node      = array_fetch(node_t * , node_array, i);
        dup_node  = node_dup(node);
        if ((gate = lib_gate_of(node)) != NIL(lib_gate_t)) {
            (void) st_insert(gate_table, (char *) dup_node, (char *) gate);
        } else if (speed_param->model == DELAY_MODEL_MAPPED) {
            (void) fprintf(sisout, "WARN: unmapped node in mapped opt\n");
        }
        (void) st_insert(dup_table, (char *) node, (char *) dup_node);
    }

    /*
     * Add the nodes --- make sure that the fanins to the added nodes
     * point to nodes in the network being generated...
     */
    for (i = array_n(node_array); i-- > 0;) {
        node = array_fetch(node_t * , node_array, i);
        assert(st_lookup(dup_table, (char *) node, (char **) &dup_node));
        foreach_fanin(node, j, fi)
        {
            if (st_lookup(dup_table, (char *) fi, (char **) &dup_fi)) {
                dup_node->fanin[j] = dup_fi;
            } else {
                if (!st_lookup(pi_table, (char *) fi, (char **) &pi_node)) {
                    pi_node = node_alloc();
                    network_add_primary_input(net, pi_node);
                    name = ALLOC(
                    char, strlen(fi->name) + 10);
                    sprintf(name, "%s%c%d", fi->name, NSP_INPUT_SEPARATOR, j);
                    network_change_node_name(net, pi_node, name);
                    (void) st_insert(pi_table, (char *) fi, (char *) pi_node);
                    /*
                     * Add the delay data for the primary input node
                     */
                    a_t = delay_arrival_time(fi);
                    delay_set_parameter(pi_node, DELAY_ARRIVAL_RISE, a_t.rise);
                    delay_set_parameter(pi_node, DELAY_ARRIVAL_FALL, a_t.fall);

                    /* For mapped ckts: set the drives : max_ip_load
                 * is set by the routine nsp_set_initial_load()
                 */
/*
		    pin_delay = get_pin_delay(node, j, speed_param->model);
		    delay_set_parameter(pi_node, DELAY_MAX_INPUT_LOAD,
					pin_delay->load);
*/
                    drive = nsp_get_input_drive(fi, speed_param->model);
                    delay_set_parameter(pi_node, DELAY_DRIVE_RISE, drive.rise);
                    delay_set_parameter(pi_node, DELAY_DRIVE_FALL, drive.fall);
                }
                dup_node->fanin[j] = pi_node;
            }
        }
        network_add_node(net, dup_node);
        if (st_lookup(gate_table, (char *) dup_node, (char **) &gate)) {
            buf_add_implementation(node, gate);
        }
    }
    /* Create the PO node from the last (root) node added */
    load    = delay_load(node);
    po_node = network_add_primary_output(net, dup_node);
    delay_set_parameter(po_node, DELAY_OUTPUT_LOAD, load);
    network_set_name(net, node_long_name(node));

    /* Also add the wire_load slope and all that default stuff */
    delay_network_dup(net, node_network(node));

    if (!network_check(net)) fail(error_string());
    if (st_count(gate_table) > 0 && !lib_network_is_mapped(net)) {
        (void) fprintf(sisout, "WARN: COLLAPSED network copy in not mapped\n");
    }

    st_free_table(dup_table);
    st_free_table(pi_table);
    st_free_table(gate_table);

    return net;
}
/* 
 * Compute the worst drive that this node can offere to any load
 */
static delay_time_t
nsp_get_input_drive(node, model)
        node_t *node;
        delay_model_t model;
{
    int          i;
    delay_time_t drive;
    delay_pin_t  *pin_delay;

    if (node->type == PRIMARY_INPUT) {
        pin_delay = get_pin_delay(node, 0, model);
        return (pin_delay->drive);
    } else {
        drive.rise = drive.fall = NEG_LARGE;
        for (i = node_num_fanin(node); i-- > 0;) {
            pin_delay = get_pin_delay(node, i, model);
            drive.rise = MAX(drive.rise, pin_delay->drive.rise);
            drive.fall = MAX(drive.fall, pin_delay->drive.fall);
        }
        return drive;
    }
}
/*
 * Decompose the network based on arrival times 
 */
static void
sp_do_initial_resynthesis(rec)
        sp_clp_t *rec;
{
    int          tmp;
    node_t       *np;
    delay_time_t new;
    network_t    *opt_netw;

    tmp = rec->glb->debug;
    rec->glb->debug = FALSE;
    opt_netw = (*rec->ti_model->optimize_func)(rec->net, rec->ti_model,
                                               rec->glb);
    network_free(rec->net);
    rec->net = opt_netw;

    rec->glb->debug = tmp;
    if (rec->ti_model->type == FAN) {
        np  = network_get_pi(rec->net, rec->cfi);
        new = delay_required_time(np);
        /* HERE -- should account for load of PREV gate !!! */
    } else if (rec->ti_model->type == DUAL) {
        fail("write this");
        np  = network_get_pi(rec->net, rec->cfi);
        new = delay_required_time(np);
    } else {
        new = (*rec->ti_model->arr_func)(network_get_po(rec->net, 0), rec->glb);
        new_speed_adjust_po_arrival(rec, rec->glb->model, &new);
    }
    if (rec->glb->debug) {
        (void) fprintf(sisout, "At %10s \"%s\" new= %.2f:%.2f old= %.2f:%.2f CS=%.2f \n",
                       network_name(rec->net), rec->ti_model->name, new.rise,
                       new.fall, rec->old.rise, rec->old.fall, rec->glb->crit_slack);
    }
}

/*
 * Ensure that it is not the case that a node in the cutset will be removed
 * when the critical region of another node in the cutset is collapsed.
 * Why this can happen still needs to be explained (Has to do with using
 * the way the flow network is created)...
 */
static array_t *
speed_filter_cutset(mincut, speed_param, weight_table)
        array_t *mincut;
        speed_global_t *speed_param;
        st_table *weight_table;
{
    lsGen       gen;
    int         i, j, k, first, last;
    int         save_node, more_to_come;
    sp_weight_t *wght, *root_wght;
    node_t      *node, *root, *temp, *fo;
    array_t     *temp_array, *bfs_array, *actual_mincut;
    st_table    *table, *del_table;

    del_table = st_init_table(st_ptrcmp, st_ptrhash);
    for (i    = array_n(mincut); i-- > 0;) {
        /* Get rid of the nodes where there is no improvement possible */
        root = array_fetch(node_t * , mincut, i);
        (void) st_lookup(weight_table, (char *) root, (char **) &root_wght);
        if (root_wght->improvement[root_wght->best_technique] < NSP_EPSILON) {
            (void) st_insert(del_table, (char *) root, NIL(
            char));
            continue;
        }
        /*
         * Generate the nodes that lie within the collapsing distance.
         */
        table     = st_init_table(st_ptrcmp, st_ptrhash);
        bfs_array = new_speed_collapse_bfs(root, speed_param, table);
        array_free(bfs_array);

        /* For all the other nodes, check if they need to be deleted */
        for (j = array_n(mincut); j-- > 0;) {
            if (i == j) continue;
            node = array_fetch(node_t * , mincut, j);
            (void) st_lookup(weight_table, (char *) node, (char **) &wght);
            if (st_is_member(table, (char *) node)) {
                /*
             * Check if the path from "node" to "root" has no fanouts
             */
                first     = 0;
                save_node = FALSE;
                more_to_come = TRUE;
                temp_array = array_alloc(node_t * , 0);
                array_insert_last(node_t * , temp_array, node);
                while (more_to_come) {
                    more_to_come = FALSE;
                    last         = array_n(temp_array);
                    for (k       = first; k < last && save_node == FALSE; k++) {
                        temp = array_fetch(node_t * , temp_array, k);
                        if (temp == root) {
                            /* reached the root */
                            continue;
                        }
                        foreach_fanout(temp, gen, fo)
                        {
                            if (st_is_member(table, (char *) fo)) {
                                array_insert_last(node_t * , temp_array, fo);
                            } else {
                                save_node = TRUE;
                            }
                        }
                    }
                    first        = last;
                    if (save_node) {
                        /* Some node fans out --- no need to continue */
                        more_to_come = FALSE;
                    }
                }
                if (save_node == FALSE) {
                    st_insert(del_table, (char *) node, (char *) root);
                    if (speed_param->debug) {
                        (void) fprintf(sisout,
                                       "%s (%.2f) in TFI of %s (%.2f)\n",
                                       node->name, SP_IMPROVEMENT(wght),
                                       root->name, SP_IMPROVEMENT(root_wght));
                    }
                }
                array_free(temp_array);
            }
        }
        st_free_table(table);
    }

    /*
     * Now put all unmarked node in the actual array 
     */
    actual_mincut = array_alloc(node_t * , array_n(mincut));
    for (i        = array_n(mincut); i-- > 0;) {
        node = array_fetch(node_t * , mincut, i);
        if (!st_is_member(del_table, (char *) node)) {
            array_insert_last(node_t * , actual_mincut, node);
        }
    }

    if (speed_param->debug && array_n(mincut) > array_n(actual_mincut)) {
        (void) fprintf(sisout, "NOTE: %d spurious nodes deleted from cut\n",
                       array_n(mincut) - array_n(actual_mincut));
    }
    st_free_table(del_table);

    return (actual_mincut);
}












