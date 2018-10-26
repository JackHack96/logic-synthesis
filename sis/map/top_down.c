

/*
 * File that interfaces the algorithm "top_down" to the buffering algorithm
 * in the speed package (that can be invoked by the command "buffer_opt")
 */
#include "fanout_delay.h"
#include "fanout_int.h"
#include "map_int.h"
#include "sis.h"
#include <math.h>

#define BUF_MIN_DELAY(result, t1, t2)                                          \
  ((result).rise = MIN((t1).rise, (t2).rise),                                  \
   (result).fall = MIN((t1).fall, (t2).fall))

static int fanout_optimize();

static void put_tree_in_fanout_tree_order();

/*
 * No longer needed since the corresponding assertions have been commented out
 */
/*
static node_t *get_fanin_from_link();
*/

/* EXTERNAL INTERFACE */

/* ARGSUSED */
static int top_down_mode;

void top_down_set_mode(fanout_alg_t *alg, alg_property_t *property) { top_down_mode = property->value; }

/* ARGSUSED */
static int top_down_debug;

void top_down_set_debug(fanout_alg_t *alg, alg_property_t *property) { top_down_debug = property->value; }

/* ARGSUSED */
void top_down_init(network_t *network, fanout_alg_t *alg) {
    alg->optimize = fanout_optimize;
    if (top_down_mode & 1) {
        fprintf(sisout, "WARNING: lowest bit of mode for top_down ignored\n");
        top_down_mode--;
    }
    buf_init_top_down(network, top_down_mode, top_down_debug);
}

/* INTERNAL INTERFACE */
delay_time_t map_compute_fanout_tree_req_time(); /* fanout_tree.c */

static int fanout_optimize(opt_array_t *fanout_info, array_t *tree, fanout_cost_t *cost) {
    lsGen           gen;
    lib_gate_t      *gate;
    gate_link_t     *link;
    st_table        *fo_table;
    double          auto_route, load, def_dr;
    network_t       *network, *dup_network;
    node_t          *node, *temp1, *temp, **new_pi, *fo, *inv_node, *new_fo;
    node_t          *dup_node, *dup_inv_node;
    buf_alg_input_t *fanout_data;
    delay_pin_t     *pin_delay, *inv_pin_delay;
    delay_model_t   model = buf_get_model();
    delay_time_t    t, root_required, inp_req, drive;
    int             source_index, polarity;
    int             nin, pin, i, total, num_pos;

    /*
     * Setup the data in the required structures
     */
    inv_node = fanout_opt_get_root_inv();
    node     = fanout_opt_get_root();
    network  = node_network(node);

    /* HACK --- Need the appropriate structure of nodes in the network */
    auto_route = buf_get_auto_route();

    /*
     * Create a separate network and add the relevant data to it
     */
    gate = lib_gate_of(node);
    if (node->type == INTERNAL && gate == NIL(lib_gate_t)) {
        /* We always expect to see a mapped gate */
        return 0;
    }

    dup_network = network_alloc();
    if (node->type == INTERNAL) {
        nin    = lib_gate_num_in(gate);
        new_pi = ALLOC(node_t *, nin);
        for (i = 0; i < nin; i++) {
            new_pi[i] = node_alloc();
            network_add_primary_input(dup_network, new_pi[i]);
        }
        /*
         * Create a node with the right number of inputs
         * and annotate it with the gate
         */
        dup_node = node_constant(1);
        for (i   = 0; i < nin; i++) {
            temp  = node_literal(new_pi[i], 1);
            temp1 = node_and(temp, dup_node);
            node_free(dup_node);
            node_free(temp);
            dup_node = temp1;
        }
        network_add_node(dup_network, dup_node);
        buf_add_implementation(dup_node, gate);
        FREE(new_pi);
    } else {
        dup_node = node_dup(node);
        network_add_primary_input(dup_network, dup_node);
        if (!delay_get_pi_drive(dup_node, &drive)) {
            def_dr     = delay_get_default_parameter(node_network(node),
                                                     DELAY_DEFAULT_DRIVE_RISE);
            if (def_dr == DELAY_NOT_SET)
                def_dr = 0.0;
            delay_set_parameter(dup_node, DELAY_DRIVE_RISE, def_dr);

            def_dr     = delay_get_default_parameter(node_network(node),
                                                     DELAY_DEFAULT_DRIVE_FALL);
            if (def_dr == DELAY_NOT_SET)
                def_dr = 0.0;
            delay_set_parameter(dup_node, DELAY_DRIVE_FALL, def_dr);
        }
    }

    /* If we need an inverter --- create it too */
    if (inv_node != NIL(node_t) && lib_gate_of(inv_node) != NIL(lib_gate_t)) {
        dup_inv_node = node_literal(dup_node, 0);
        network_add_node(dup_network, dup_inv_node);
    } else {
        dup_inv_node = NIL(node_t);
    }

    source_index = fanout_delay_get_source_index(node);
    polarity     = fanout_delay_get_source_polarity(source_index);

    fo_table    = st_init_table(st_ptrcmp, st_ptrhash);
    fanout_data = ALLOC(buf_alg_input_t, 1);
    fanout_data->max_ip_load = INFINITY;
    total = fanout_info[0].n_elts + fanout_info[1].n_elts;
    fanout_data->fanouts = ALLOC(sp_fanout_t, total);
    for (i = 0; i < fanout_info[polarity].n_elts; i++) {
        link   = array_fetch(gate_link_t *, fanout_info[polarity].required, i);
        /*
        assert((fanin = get_fanin_from_link(link, &fanin_index)) != NIL(node_t));
        */
        new_fo = network_add_primary_output(dup_network, dup_node);
        fanout_data->fanouts[i].pin    = 0;
        fanout_data->fanouts[i].fanout = new_fo;
        fanout_data->fanouts[i].load   = link->load;
        fanout_data->fanouts[i].req    = link->required;
        fanout_data->fanouts[i].phase  = PHASE_NONINVERTING;
        (void) st_insert(fo_table, (char *) (new_fo), (char *) link);
        delay_set_parameter(new_fo, DELAY_REQUIRED_RISE, link->required.rise);
        delay_set_parameter(new_fo, DELAY_REQUIRED_FALL, link->required.fall);
        delay_set_parameter(new_fo, DELAY_OUTPUT_LOAD, link->load);
    }
    num_pos               = i;
    for (i                = 0; i < fanout_info[1 - polarity].n_elts; i++) {
        link   = array_fetch(gate_link_t *, fanout_info[1 - polarity].required, i);
        /*
        assert((fanin = get_fanin_from_link(link, &fanin_index)) != NIL(node_t));
        */
        new_fo = network_add_primary_output(dup_network, dup_inv_node);
        fanout_data->fanouts[num_pos + i].pin    = 0;
        fanout_data->fanouts[num_pos + i].load   = link->load;
        fanout_data->fanouts[num_pos + i].fanout = new_fo;
        fanout_data->fanouts[num_pos + i].req    = link->required;
        fanout_data->fanouts[num_pos + i].phase  = PHASE_INVERTING;
        (void) st_insert(fo_table, (char *) (new_fo), (char *) link);
        delay_set_parameter(new_fo, DELAY_REQUIRED_RISE, link->required.rise);
        delay_set_parameter(new_fo, DELAY_REQUIRED_FALL, link->required.fall);
        delay_set_parameter(new_fo, DELAY_OUTPUT_LOAD, link->load);
    }
    fanout_data->num_pos  = num_pos;
    fanout_data->num_neg  = i;
    fanout_data->root     = dup_node;
    fanout_data->node     = dup_node;
    fanout_data->inv_node = dup_inv_node;

    pin_delay = get_pin_delay(dup_node, 0, model);
    t.rise = t.fall = 0.0;
    buf_set_prev_drive(dup_node, t);
    buf_set_prev_phase(dup_node, PHASE_NONINVERTING);

    load = 0;
    if (dup_inv_node != NIL(node_t)) {
        assert(fanout_data->num_neg > 0);
        if ((gate = lib_gate_of(inv_node)) != NIL(lib_gate_t)) {
            buf_add_implementation(dup_inv_node, gate);
        }
        buf_set_prev_drive(dup_inv_node, pin_delay->drive);
        buf_set_prev_phase(dup_inv_node, pin_delay->phase);
        inv_pin_delay = get_pin_delay(dup_inv_node, 0, model);
        load          = inv_pin_delay->load + auto_route;
        t             = *(fanout_info[1 - polarity].min_required);
        sp_subtract_delay(inv_pin_delay->phase, inv_pin_delay->block,
                          inv_pin_delay->drive,
                          fanout_info[1 - polarity].total_load, &t);
        buf_set_required_time_at_input(dup_inv_node, t);
    }

    if (num_pos > 0) {
        BUF_MIN_DELAY(t, t, *(fanout_info[polarity].min_required));
    }
    sp_subtract_delay(pin_delay->phase, pin_delay->block, pin_delay->drive,
                      (load + fanout_info[polarity].total_load), &t);
    buf_set_required_time_at_input(dup_node, t);

    /* Call the recursive algorithm */
    buf_map_interface(dup_network, fanout_data);

    /* Build the tree && evaluate the cost of the fanout structure */
    cost->area = 0;
    put_tree_in_fanout_tree_order(network, tree, dup_node, dup_node, dup_inv_node,
                                  source_index, fo_table, &(cost->area));
    cost->slack = map_compute_fanout_tree_req_time(tree);

    network_free(dup_network);
    st_free_table(fo_table);
    FREE(fanout_data->fanouts);
    FREE(fanout_data);

    if (top_down_debug) {
        (void) fprintf(sisout, "Done top_down -- Area %f , Slack %f:%f\n",
                       cost->area, cost->slack.rise, cost->slack.fall);
    }
    return 1;
}

/*
 * Creates the data structure for returning to the fanout interface --
 * At the same time it deletes all the nodes that were created
 */
static void
put_tree_in_fanout_tree_order(network_t *network, array_t *tree, node_t *node, node_t *root, node_t *root_inv,
                              int source_index, st_table *fo_table, double *a) {
    lsGen       gen;
    lib_gate_t  *gate;
    gate_link_t *link;
    int         gate_index;
    node_t      *fanout;

    if (st_lookup(fo_table, (char *) node, (char **) &link)) {
        fanout_tree_insert_sink(tree, link);
    } else {
        gate = lib_gate_of(node);
        if (node == root) {
            /* The gate_index is the source index */
            gate_index = source_index;
        } else {
            /* get the appropriate gate_index from the lib_gate_t */
            gate_index = fanout_delay_get_buffer_index(gate);
            *a += lib_gate_area(gate);
        }
        fanout_tree_insert_gate(tree, gate_index, node_num_fanout(node));
        foreach_fanout(node, gen, fanout) {
            put_tree_in_fanout_tree_order(network, tree, fanout, root, root_inv,
                                          source_index, fo_table, a);
        }
    }
}

/*
static node_t *
get_fanin_from_link(link, index)
gate_link_t *link;
int *index;
{
    node_t *fanin;

    if (link->node->type == PRIMARY_OUTPUT){
        fanin = node_get_fanin(link->node, 0);
        *index = 0;
    } else if (link->node->type == INTERNAL){
        if (MAP(link->node)->gate == NIL(lib_gate_t)){
            return NIL(node_t);
        } else {
            fanin = MAP(link->node)->save_binding[link->pin];
            *index = node_get_fanin_index(link->node, fanin);
            assert(*index >= 0);
        }
    } else {
        fanin = NIL(node_t);
    }
    return fanin;
}
*/

#undef BUF_MIN_DELAY
