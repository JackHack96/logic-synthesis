
/* file @(#)virtual_del.c	1.7 */
/* last modified on 7/22/91 at 12:36:39 */
/*
 * $Log: virtual_del.c,v $
 * Revision 1.1.1.1  2004/02/07 10:14:26  pchong
 * imported
 *
 * Revision 1.3  1992/05/06  18:55:51  sis
 * SIS release 1.1
 *
 * Revision 1.3  1992/05/06  18:55:51  sis
 * SIS release 1.1
 *
 * Revision 1.2  1992/04/17  22:00:59  sis
 * *** empty log message ***
 *
 * Revision 1.1  92/01/08  17:40:59  sis
 * Initial revision
 * 
 * Revision 1.3  91/07/02  11:19:23  touati
 * corrected a bug: handles default required times properly now.
 * 
 * Revision 1.2  91/06/30  22:19:17  touati
 * change MAP(node)->arrival_info from pointers to actual copy of delay info
 * 
 * Revision 1.1  91/06/28  22:51:22  touati
 * Initial revision
 * 
 */
#include "sis.h"
#include "map_int.h"
#include "map_delay.h"
#include "fanout_int.h"
#include "fanout_delay.h"

static void update_gate_link_loads_rec();

static void virtual_delay_compute_node_arrival_time();

static bin_global_t global;

void virtual_delay_compute_arrival_times(network, globals)
        network_t *network;
        bin_global_t globals;
{
    lsGen    gen;
    node_t   *pi, *po;
    st_table *computed = st_init_table(st_numcmp, st_numhash);

    global = globals;
    foreach_primary_input(network, gen, pi)
    {
        update_gate_link_loads_rec(computed, pi);
        MAP(pi)->load = gate_link_compute_load(pi);
    }
    st_free_table(computed);

    computed = st_init_table(st_numcmp, st_numhash);
    foreach_primary_output(network, gen, po)
    {
        node_t *node = map_po_get_fanin(po);
        virtual_delay_arrival_times_rec(computed, node);
        MAP(po)->map_arrival = MAP(node)->map_arrival;
    }
    st_free_table(computed);
}


/* force the required times to be negative to force maximum optimization */
/* also keep the relative positions of the PO's in terms of criticality */

void virtual_delay_set_po_negative_required(network)
        network_t *network;
{
    lsGen        gen;
    node_t       *output;
    double       shift, max_value;
    delay_time_t required, max_po_required;

    max_po_required = MINUS_INFINITY;
    foreach_primary_output(network, gen, output)
    {
        required = pipo_get_po_required(output);
        SETMAX(max_po_required, max_po_required, required);
    }

    max_value  = GETMAX(max_po_required);
    for (shift = 1.0; shift < max_value; shift *= 10.0);

    foreach_primary_output(network, gen, output)
    {
        required = pipo_get_po_required(output);
        required.rise -= shift;
        required.fall -= shift;
        MAP(output)->required = required;
        virtual_network_update_link_required_times(output, NIL(delay_time_t));
    }
}

void virtual_delay_set_po_required(network)
        network_t *network;
{
    lsGen        gen;
    node_t       *output;
    delay_time_t max_po_arrival;

    max_po_arrival = MINUS_INFINITY;
    foreach_primary_output(network, gen, output)
    {
        SETMAX(max_po_arrival, max_po_arrival, MAP(output)->map_arrival);
    }
    pipo_set_default_po_required(max_po_arrival);
    foreach_primary_output(network, gen, output)
    {
        MAP(output)->required     = pipo_get_po_required(output);
        if (IS_EQUAL(MAP(output)->required, MINUS_INFINITY))
            MAP(output)->required = max_po_arrival;
        virtual_network_update_link_required_times(output, NIL(delay_time_t));
    }
}


/* the required time at a node is the min of the required times in its gate_link */
/* it does not take into account the delay through the gate at that node */
/* this routine also computes the required times on the wires ending up at "node" */
/* those required times take the delay through the gate at "node" into account */

void virtual_delay_compute_node_required_time(node)
        node_t *node;
{
    double       load;
    delay_time_t *link_required;

    assert(node->type == PRIMARY_INPUT || MAP(node)->gate != NIL(lib_gate_t));
    MAP(node)->required = gate_link_compute_min_required(node);
    if (node->type == PRIMARY_INPUT) return;
    load          = gate_link_compute_load(node);
    link_required = ALLOC(delay_time_t, MAP(node)->ninputs);
    delay_map_compute_required_times(MAP(node)->ninputs, MAP(node)->gate->delay_info, MAP(node)->required, load,
                                     link_required);
    virtual_network_update_link_required_times(node, link_required);
    FREE(link_required);
}


/* INTERNAL INTERFACE */

static void update_gate_link_loads_rec(table, node)
        st_table *table;
        node_t *node;
{
    int         i;
    node_t      *input;
    gate_link_t link;

    if (st_lookup(table, (char *) node, NIL(char *))) return;
    if (node->type == PRIMARY_OUTPUT) {
        MAP(node)->load = pipo_get_po_load(node);
        link.node       = node;
        link.pin        = -1;
        input = map_po_get_fanin(node);
        assert(gate_link_get(input, &link));
        link.load = MAP(node)->load;
        gate_link_put(input, &link);
        st_insert(table, (char *) node, NIL(
        char));
        return;
    }
    if (gate_link_n_elts(node) == 0) return;
    gate_link_first(node, &link);
    do {
        update_gate_link_loads_rec(table, link.node);
    } while (gate_link_next(node, &link));
    MAP(node)->load = gate_link_compute_load(node);
    if (node->type == PRIMARY_INPUT) {
        st_insert(table, (char *) node, NIL(
        char));
        return;
    }
    assert(MAP(node)->gate);
    for (i = 0; i < MAP(node)->ninputs; i++) {
        link.node = node;
        link.pin  = i;
        input = MAP(node)->save_binding[i];
        assert(gate_link_get(input, &link));
        link.load = delay_get_load(MAP(node)->gate->delay_info[i]);
        gate_link_put(input, &link);
    }
    st_insert(table, (char *) node, NIL(
    char));
}


/* exported to map_interface.c */

void virtual_delay_arrival_times_rec(table, node)
        st_table *table;
        node_t *node;
{
    int pin;

    if (st_lookup(table, (char *) node, NIL(char *))) return;
    if (node->type != PRIMARY_INPUT) {
        assert(MAP(node)->gate);
        for (pin = 0; pin < MAP(node)->ninputs; pin++)
            virtual_delay_arrival_times_rec(table, MAP(node)->save_binding[pin]);
    }
    virtual_delay_compute_node_arrival_time(node);
    st_insert(table, (char *) node, NIL(
    char));
}

static void virtual_delay_compute_node_arrival_time(node)
        node_t *node;
{
    int          i;
    node_t       *input;
    delay_time_t arrival;
    delay_time_t drive;
    delay_time_t **arrival_times;

    switch (node_function(node)) {
        case NODE_PI:arrival = pipo_get_pi_arrival(node);
            drive            = pipo_get_pi_drive(node);
            arrival.rise += drive.rise * MAP(node)->load;
            arrival.fall += drive.fall * MAP(node)->load;
            MAP(node)->map_arrival = arrival;
            break;
        case NODE_PO:input = map_po_get_fanin(node);
            MAP(node)->map_arrival = MAP(input)->map_arrival;
            break;
        case NODE_0:
        case NODE_1:
            if (!global.no_warning)
                (void) fprintf(siserr, "WARNING: arrival time at constant node is (0.00,0.00) \n");
            MAP(node)->map_arrival = ZERO_DELAY;
            break;
        case NODE_BUF:
        case NODE_INV:
        case NODE_AND:
        case NODE_OR:
        case NODE_COMPLEX:
            if (strcmp(MAP(node)->gate->name, "**wire**") == 0) {
                MAP(node)->map_arrival = MAP(MAP(node)->save_binding[0])->map_arrival;
            } else {
                arrival_times = ALLOC(delay_time_t * , MAP(node)->ninputs);
                for (i        = 0; i < MAP(node)->ninputs; i++) {
                    input = MAP(node)->save_binding[i];
                    arrival_times[i] = &(MAP(input)->map_arrival);
                }
                arrival = delay_map_simulate(MAP(node)->ninputs, arrival_times, MAP(node)->gate->delay_info,
                                             MAP(node)->load);
                MAP(node)->map_arrival = arrival;
                if (MAP(node)->arrival_info) FREE(MAP(node)->arrival_info);
                MAP(node)->arrival_info = ALLOC(delay_time_t, MAP(node)->ninputs);
                for (i = 0; i < MAP(node)->ninputs; i++) {
                    MAP(node)->arrival_info[i] = *arrival_times[i];
                }
                FREE(arrival_times);
            }
            break;
        case NODE_UNDEFINED:
        default:fail("illegal node type");
    }
}
