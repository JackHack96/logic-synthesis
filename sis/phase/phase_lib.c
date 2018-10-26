
#include "phase.h"
#include "phase_int.h"
#include "sis.h"

static bool       keep_mapped;
static lib_gate_t *inv_gate;

static lib_gate_t *min_area_gate();

void phase_lib_setup(network_t *network) {
    lib_class_t *class;
    node_t      *node;
    lsGen       gen;

    /* is there a library? */
    if (

            lib_get_library()

            == NIL(library_t)) {
        keep_mapped = FALSE;
        inv_gate    = NIL(lib_gate_t);
        return;
    }

    /* are all the gates mapped? */
    foreach_node(network, gen, node) {
        if (node->type == INTERNAL && lib_gate_of(node) == NIL(lib_gate_t)) {
            keep_mapped = FALSE;
            inv_gate    = NIL(lib_gate_t);
            (void) lsFinish(gen);
            return;
        }
    }

    /* is there an inverter in the library? */
    network = read_eqn_string("f = a';");
    assert(network != NIL(network_t));
    class = lib_get_class(network, lib_get_library());
    network_free(network);
    if (class == NIL(lib_class_t)) {
        keep_mapped = FALSE;
        inv_gate    = NIL(lib_gate_t);
        return;
    }

    /* Ok, I'll keep the network mapped */
    inv_gate    = min_area_gate(class);
    keep_mapped = TRUE;
}

void phase_invertible_set(row_data_t *row_data) {
    lib_gate_t  *gate, *dual_gate;
    lib_class_t *dual_class;

    switch (node_function(row_data->node)) {
        case NODE_PI:
        case NODE_PO:row_data->invertible = FALSE;
            row_data->area                = 0.0;
            row_data->dual_area           = 0.0;
            return;
        default:
            if (!keep_mapped) {
                row_data->invertible = TRUE;
                row_data->area       = 0.0;
                row_data->dual_area  = 0.0;
                return;
            }
            gate = lib_gate_of(row_data->node);
            if (gate == NIL(lib_gate_t)) {
                row_data->invertible = TRUE;
                row_data->area       = 0.0;
                row_data->dual_area  = 0.0;
                return;
            } else {
                dual_class = lib_class_dual(lib_gate_class(gate));
                if (dual_class == NIL(lib_class_t)) {
                    row_data->invertible = FALSE;
                    row_data->area       = 0.0;
                    row_data->dual_area  = 0.0;
                    return;
                } else {
                    dual_gate = min_area_gate(dual_class);
                    assert(dual_gate != NIL(lib_gate_t));
                    row_data->invertible = TRUE;
                    row_data->area       = lib_gate_area(gate);
                    row_data->dual_area  = lib_gate_area(dual_gate);
                    return;
                }
            }
    }
}

void phase_node_invert(node_t *np) {
    lib_gate_t  *gate, *dual_gate;
    lib_class_t *dual_class;
    char        **inv_formals, **formals;
    node_t      **inv_actuals, **actuals, *node, *fanin;
    int         i;

    if (keep_mapped) {
        gate       = lib_gate_of(np);
        dual_class = lib_class_dual(lib_gate_class(gate));
        dual_gate  = min_area_gate(dual_class);

        inv_formals = ALLOC(char *, 1);
        inv_actuals = ALLOC(node_t *, 1);
        inv_formals[0] = lib_gate_pin_name(inv_gate, 0, /*input*/ 1);

        formals = ALLOC(char *, node_num_fanin(np));
        actuals = ALLOC(node_t *, node_num_fanin(np));

        foreach_fanin(np, i, fanin) {
            inv_actuals[0] = fanin;
            node = node_alloc();
            (void) lib_set_gate(node, inv_gate, inv_formals, inv_actuals, 1);
            network_add_node(node_network(np), node);
            formals[i] = lib_gate_pin_name(dual_gate, i, 1);
            actuals[i] = node;
        }

        node = node_alloc();
        (void) lib_set_gate(node, dual_gate, formals, actuals, node_num_fanin(np));
        network_add_node(node_network(np), node);
        inv_actuals[0] = node;
        (void) lib_set_gate(np, inv_gate, inv_formals, inv_actuals, 1);

        FREE(formals);
        FREE(inv_formals);
        FREE(actuals);
        FREE(inv_actuals);

    } else {
        (void) node_invert(np);
    }
}

static lib_gate_t *min_area_gate(lib_class_t *class) {
    lib_gate_t *gate;
    lsGen      gen;
    char       *dummy;

    /* find the gate with minimum area */
    gen = lib_gen_gates(class);
    if (lsNext(gen, &dummy, LS_NH) != LS_OK) {
        fail("Error, dual class is empty\n");
    }
    gate = (lib_gate_t *) dummy;
    while (lsNext(gen, &dummy, LS_NH) == LS_OK) {
        if (lib_gate_area((lib_gate_t *) dummy) < lib_gate_area(gate)) {
            gate = (lib_gate_t *) dummy;
        }
    }
    (void) lsFinish(gen);

    return gate;
}

double phase_value(node_phase_t *node_phase) {
    row_data_t  *row_data;
    lib_gate_t  *gate, *dual_gate;
    lib_class_t *dual_class;
    double      value;

    row_data = sm_get(row_data_t *, node_phase->row);
    if (!keep_mapped) {
        return (double) row_data->inv_save;
    } else {
        gate       = lib_gate_of(row_data->node);
        dual_class = lib_class_dual(lib_gate_class(gate));
        dual_gate  = min_area_gate(dual_class);
        if (row_data->inverted) {
            value = lib_gate_area(dual_gate) - lib_gate_area(gate);
        } else {
            value = lib_gate_area(gate) - lib_gate_area(dual_gate);
        }
        value += row_data->inv_save * lib_gate_area(inv_gate);
        return value;
    }
}

double cost_comp(net_phase_t *net_phase) {
    sm_row     *row;
    row_data_t *rd;
    double     count;

    count = 0;
    sm_foreach_row(net_phase->matrix, row) {
        rd = sm_get(row_data_t *, row);
        if (rd->inverted) {
            count += rd->dual_area;
        } else {
            count += rd->area;
        }
        if (rd->pos_used != 0) {
            if (keep_mapped) {
                count += lib_gate_area(inv_gate);
            } else {
                count += 1;
            }
        }
    }

    return count;
}

void phase_record(network_t *network, net_phase_t *net_phase) {
    sm_row     *row;
    row_data_t *rd;

    sm_foreach_row(net_phase->matrix, row) {
        rd = sm_get(row_data_t *, row);
        if (rd->inverted) {
            (void) phase_node_invert(rd->node);
        }
    }

    if (keep_mapped) {
        map_remove_inverter(network, 0);
    } else {
        (void) network_sweep(network);
        add_inv_network(network);
    }
}
