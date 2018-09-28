
/* file @(#)libutil.c	1.8 */
/* last modified on 9/16/91 at 17:07:18 */
#include "lib_int.h"
#include "map_int.h"
#include "sis.h"

library_t *lib_current_library;

library_t *lib_get_library() { return lib_current_library; }

lsGen lib_gen_classes(library)library_t *library;
{ return lsStart(library->classes); }

lsGen lib_gen_gates(class)lib_class_t *class;
{ return lsStart(class->gates); }

lib_gate_t *lib_get_gate(library, name)library_t *library;
                                       char *name;
{
    lib_gate_t *gate;
    lsGen      gen;

    lsForeachItem(library->gates, gen, gate) {
        if (strcmp(gate->name, name) == 0) {
            LS_ASSERT(lsFinish(gen));
            return gate;
        }
    }
    return NIL(lib_gate_t);
}

/* for backward compatibility */
lib_class_t *lib_get_class(network, library)network_t *network;
                                            library_t *library;
{
#ifdef SIS
    return lib_get_class_by_type(network, library, COMBINATIONAL);
#else
    return lib_get_class_by_type(network, library);
#endif
}

/*
 *  lib_class_t access functions
 */

char *lib_class_name(class)lib_class_t *class;
{
    lib_gate_t *gate;
    char       *dummy;

    if (class == 0)
        return NIL(char);
    if (lsFirstItem(class->gates, &dummy, LS_NH) == LS_OK) {
        gate = (lib_gate_t *) dummy;
        return gate->name;
    }
    return NIL(char);
}

lib_class_t *lib_class_dual(class)lib_class_t *class;
{ return class->dual_class; }

network_t *lib_class_network(class)lib_class_t *class;
{ return class->network; }

/*
 *  lib_gate_t access functions
 */

char *lib_gate_name(gate)lib_gate_t *gate;
{
    if (gate == 0)
        return NIL(char);
    return gate->name;
}

char *lib_gate_pin_name(gate, pin, inflag)lib_gate_t *gate;
                                          int pin;
                                          int inflag;
{
    node_t *p;

    if (gate == 0)
        return NIL(char);
    switch (inflag) {
        case 0:p = network_get_po(gate->network, pin);
            break;
        case 1:p = network_get_pi(gate->network, pin);
            break;
#ifdef SIS
        case 2:return (gate->control_name);
#endif
        default:p = NIL(node_t);
            break;
    }
    return p ? p->name : NIL(char);
}

double lib_gate_area(gate)lib_gate_t *gate;
{
    if (gate == 0)
        return 0.0;
    return gate->area;
}

lib_class_t *lib_gate_class(gate)lib_gate_t *gate;
{
    if (gate == 0)
        return NIL(lib_class_t);
    return gate->class_p;
}

int lib_gate_num_in(gate)lib_gate_t *gate;
{
    if (gate == 0)
        return -1;
    return network_num_pi(gate->network);
}

int lib_gate_num_out(gate)lib_gate_t *gate;
{
    if (gate == 0)
        return -1;
    return network_num_po(gate->network);
}

/*
 *  misc functions
 */

lib_gate_t *lib_gate_of(node)node_t *node;
{
    if (MAP(node) == 0)
        return NIL(lib_gate_t);
    return MAP(node)->gate;
}

char *lib_get_pin_delay(node, pin)node_t *node;
                                  int pin;
{
    if (MAP(node) == 0)
        return 0;
    if (MAP(node)->gate == 0)
        return 0;
    /* ### should check for out of bounds ? */
    return MAP(node)->gate->delay_info[pin];
}

int lib_network_is_mapped(network)network_t *network;
{
    network_type_t type;

    type = map_get_network_type(network);
    return (type == MAPPED_NETWORK) ? 1 : 0;
}

/*
 *  backwards compatibility
 */

char *lib_get_gate_name(node)node_t *node;
{ return lib_gate_name(lib_gate_of(node)); }

char *lib_get_pin_name(node, pin)node_t *node;
                                 int pin;
{ return lib_gate_pin_name(lib_gate_of(node), pin, /* inflag */ 1); }

char *lib_get_out_pin_name(node, pin)node_t *node;
                                     int pin;
{ return lib_gate_pin_name(lib_gate_of(node), pin, /* inflag */ 0); }

double lib_get_gate_area(node)node_t *node;
{ return lib_gate_area(lib_gate_of(node)); }

/*
 *  set the gate of a node -- assumes single-output gate
 */

int lib_set_gate(user_node, gate, formals, actuals, nin)node_t *user_node;
                                                        lib_gate_t *gate;
                                                        char **formals;
                                                        node_t **actuals;
                                                        int nin;
{
    register int i, j;
    node_t       *node, *fanin, **new_fanin;
    node_t       *new_po;
    char         errmsg[1024];
#ifdef SIS
    node_t  *new_node; /* sequential support */
    latch_t *latch;   /* sequential support */
#endif
    int ninputs;

#ifdef SIS
    if (node_type(user_node) != PRIMARY_INPUT) {
#endif
        if (nin != network_num_pi(gate->network)) {
            error_append("lib_set_gate: number of inputs on gate is incorrect\n");
            return 0;
        }
        /*
          if (network_num_po(gate->network) != 1) {
          error_append("lib_set_gate: number of outputs on gate is incorrect\n");
          return 0;
          }
          */

        new_fanin = ALLOC(node_t *, nin);
        node      = node_get_fanin(network_get_po(gate->network, 0), 0);
        foreach_fanin(node, i, fanin) {
            new_fanin[i] = NIL(node_t);
            for (j = 0; j < nin; j++) {
                if (strcmp(fanin->name, formals[j]) == 0) {
                    new_fanin[i] = actuals[j];
                    break;
                }
            }
            if (new_fanin[i] == 0) {
                (void) sprintf(errmsg, "lib_set_gate: pin '%s' not in formals list\n",
                               fanin->name);
                error_append(errmsg);
                return 0;
            }
        }

        node_replace_internal(user_node, new_fanin, nin, sf_save(node->F));
        map_alloc(user_node);
        MAP(user_node)->gate = gate;
        return 1;
#ifdef SIS
    }
    /* sequential support : create a new PO, a new node, a new
     * latch structure and set the new node to the given latch gate.
     * input, output, type and gate fields of the latch structure
     * are filled here.   The rest of the fields must be filled
     * by the calling program.
     */

    /* note that the feedback variable is not included in nin
     */
    ninputs = nin + ((lib_gate_latch_pin(gate) != -1) ? 1 : 0);
    if (ninputs != network_num_pi(gate->network)) {
        error_append("lib_set_gate: number of inputs on gate is incorrect\n");
        return 0;
    }
    /*
     * the user_node is a PI
     * new_node -> [ PO, PI(user_node) ]
     */
    new_node = node_alloc();
    network_add_node(node_network(user_node), new_node);
    new_po = network_add_primary_output(node_network(user_node), new_node);
    network_create_latch(node_network(user_node), &latch, new_po, user_node);
    latch_set_gate(latch, gate);
    latch_set_type(latch, lib_gate_type(gate));
    /* now set the new_node func to that of the given latch */
    new_fanin = ALLOC(node_t *, ninputs);
    node      = node_get_fanin(network_get_po(gate->network, 0), 0);
    foreach_fanin(node, i, fanin) {
        new_fanin[i] = NIL(node_t);
        for (j = 0; j < ninputs; j++) {
            if (strcmp(fanin->name, formals[j]) == 0) {
                new_fanin[i] = actuals[j];
                break;
            }
        }
        if (new_fanin[i] == 0) {
            if (IS_LATCH_END(fanin)) {
                new_fanin[i] = user_node;
            } else {
                (void) sprintf(errmsg, "lib_set_gate: pin '%s' not in formals list\n",
                               fanin->name);
                error_append(errmsg);
                return 0;
            }
        }
    }

    node_replace_internal(new_node, new_fanin, ninputs, sf_save(node->F));
    map_alloc(new_node);
    MAP(new_node)->gate = gate;
    return 1;
#endif
}

void lib_free(library)library_t *library;
{
    lsGen       gen;
    lib_gate_t  *gate;
    lib_class_t *class;
    prim_t      *prim;
    int         i;

    lsForeachItem(library->gates, gen, gate) {
        for (i = network_num_pi(gate->network) - 1; i >= 0; i--) {
            FREE(gate->delay_info[i]);
        }
        FREE(gate->delay_info);
#ifdef SIS
        if (gate->latch_time_info != NIL(latch_time_t *)) {
            for (i = network_num_pi(gate->network) - 1; i >= 0; i--) {
                if (gate->latch_time_info[i] != NIL(latch_time_t)) {
                    FREE(gate->latch_time_info[i]);
                }
            }
        }
        FREE(gate->latch_time_info);
        if (gate->clock_delay != 0) {
            FREE(gate->clock_delay);
        }
#endif
        FREE(gate->name);
        FREE(gate);
    }
    LS_ASSERT(lsDestroy(library->gates, (void (*)()) 0));

    lsForeachItem(library->classes, gen, class) {
        network_free(class->network);
        LS_ASSERT(lsDestroy(class->gates, (void (*)()) 0));
        FREE(class->name);
        FREE(class);
    }
    LS_ASSERT(lsDestroy(library->classes, (void (*)()) 0));

    lsForeachItem(library->patterns, gen, prim) { prim_free(prim); }
    LS_ASSERT(lsDestroy(library->patterns, (void (*)()) 0));
    FREE(library);
}

void lib_dump(fp, library, detail)FILE *fp;
                                  library_t *library;
                                  int detail;
{
    lsGen       gen;
    prim_t      *prim;
    lib_class_t *class;

    lsForeachItem(library->classes, gen, class) { lib_dump_class(fp, class); }

    (void) fprintf(fp, "\n");
    lsForeachItem(library->patterns, gen, prim) {
        map_prim_dump(fp, prim, detail);
    }
}

void lib_dump_class(fp, class)FILE *fp;
                              lib_class_t *class;
{
    lsGen      gen, gen1;
    lib_gate_t *gate;
    node_t     *node;
    char       *dual_name;

    (void) fprintf(fp, "class: %s\n", lib_class_name(class));

    lsForeachItem(class->gates, gen, gate) {
        dual_name     = lib_class_name(class->dual_class);
        if (dual_name == 0)
            dual_name = "<none>";

        (void) fprintf(fp, "    gate: %-10s  area: %5.2f  dual_class: %s\n",
                       lib_gate_name(gate), lib_gate_area(gate), dual_name);

        foreach_primary_output(gate->network, gen1, node) {
            (void) fprintf(fp, "      ");
            (void) node_print_negative(fp, node_get_fanin(node, 0));
        }

        assert(gate->class_p == class);
    }
    (void) fprintf(fp, "\n");
}

/* sequential support */
lib_class_t *
#ifdef SIS
lib_get_class_by_type(network, library, type)
#else
lib_get_class_by_type(network, library)
#endif
        network_t *network;
        library_t *library;
#ifdef SIS
        latch_synch_t type;
#endif
{
    lsGen       gen;
    lib_class_t *class_p;
    lib_gate_t  *gate;
    network_t   *net1;
    node_t      *node, *node1;
    prim_t      *prim;
    char        *dummy;

    if (library == NIL(library_t)) {
        return NIL(lib_class_t);
    }
    /* Special hack as usual for constants */
    if (network_num_pi(network) == 0) {
        node = node_get_fanin(network_get_po(network, 0), 0);
        lsForeachItem(library->classes, gen, class_p) {
            if (network_num_pi(class_p->network) == 0) {
                node1 = node_get_fanin(network_get_po(class_p->network, 0), 0);
                if (node_equal_by_name(node1, node)) {
                    LS_ASSERT(lsFinish(gen));
                    return class_p;
                }
            }
        }
        return NIL(lib_class_t);
    }

    net1 = network_dup(network);
    decomp_quick_network(net1);
    if (library->nand_flag) {
        decomp_tech_network(net1, /* and_limit */ 0, /* or_limit */ 2);
    } else {
        decomp_tech_network(net1, /* and_limit */ 2, /* or_limit */ 0);
    }
    add_inv_network(net1);
    if (library->add_inverter) {
        map_add_inverter(net1, /* add_at_pipo */ 0);
    }

    /* HACK: take care of buffers
     * (buffer is represented by PI->INV->INV->PO in the library)
     * (net1 looks like PI->PO)
     */
#ifdef SIS
    if (type == COMBINATIONAL && network_num_pi(net1) == 1 &&
        network_num_internal(net1) == 0) {
#else
        if (network_num_pi(net1) == 1 && network_num_internal(net1) == 0) {
#endif
        node_t *po, *pi, *node1, *node2;

        po    = network_get_po(net1, 0);
        pi    = node_get_fanin(po, 0);
        node1 = node_alloc();
        node_replace(node1, node_literal(pi, 0));
        network_add_node(net1, node1);
        node2 = node_alloc();
        node_replace(node2, node_literal(node1, 0));
        network_add_node(net1, node2);
        assert(node_patch_fanin(po, pi, node2));
    }
    map_setup_network(net1); /* needed for lib_pattern_matches() */

    class_p = 0;
    lsForeachItem(library->patterns, gen, prim) {
#ifdef SIS
        if (prim->latch_type == type) {
#endif
            if (lib_pattern_matches(net1, prim, /* ignore type */ 1)) {
                LS_ASSERT(lsFirstItem(prim->gates, &dummy, LS_NH));
                gate = (lib_gate_t *) dummy;
                if (strcmp(gate->name, "**wire**") != 0) {
                    class_p = gate->class_p;
                    LS_ASSERT(lsFinish(gen));
                    break;
                }
            }
#ifdef SIS
        }
#endif
    }
    network_free(net1);
    return class_p;
}

#ifdef SIS

lib_gate_t *lib_choose_smallest_latch(library_t *library, char *string, latch_synch_t latch_type) {
    network_t   *network;
    lib_class_t *class;
    lib_gate_t  *gate, *best_gate;
    lsGen       gen;
    double      best_area;

    network = read_eqn_string(string);
    if (network == 0)
        return 0;
    class = lib_get_class_by_type(network, library, latch_type);
    network_free(network);
    if (class == 0)
        return 0;

    best_area = INFINITY;
    best_gate = 0;
    gen       = lib_gen_gates(class);
    while (lsNext(gen, (char **) &gate, LS_NH) == LS_OK) {
        if (lib_gate_area(gate) < best_area) {
            best_area = lib_gate_area(gate);
            best_gate = gate;
        }
    }
    LS_ASSERT(lsFinish(gen));
    return best_gate;
}

/* print params assoc. with a gate */
void lib_dump_gate(gate)lib_gate_t *gate;
{
    int i;

    (void) fprintf(sisout, "%s : type=", gate->name);
    switch (gate->type) {
        case (int) COMBINATIONAL:(void) fprintf(sisout, "comb");
            break;
        case (int) ACTIVE_HIGH:(void) fprintf(sisout, "ah");
            break;
        case (int) ACTIVE_LOW:(void) fprintf(sisout, "al");
            break;
        case (int) RISING_EDGE:(void) fprintf(sisout, "re");
            break;
        case (int) FALLING_EDGE:(void) fprintf(sisout, "fe");
            break;
        case (int) ASYNCH:(void) fprintf(sisout, "as");
            break;
        default:(void) fprintf(sisout, "unknown");
            break;
    }
    (void) fprintf(sisout, ", latch_pin # = %d, clock = %s\n", gate->latch_pin,
                   (gate->control_name != 0) ? gate->control_name : "NIL");
    if (gate->latch_time_info != 0) {
        for (i = 0; i < network_num_pi(gate->network); i++) {
            if (gate->latch_time_info[i] != 0) {
                (void) fprintf(sisout, "  pin %d : setup = %4.2f, hold = %4.2f\n", i,
                               gate->latch_time_info[i]->setup,
                               gate->latch_time_info[i]->hold);
            }
        }
    }
    if (gate->clock_delay != 0) {
        (void) fprintf(sisout,
                       "  clock delay = %4.2f %4.2f %4.2f %4.2f %4.2f %4.2f\n",
                       gate->clock_delay->block.rise, gate->clock_delay->block.fall,
                       gate->clock_delay->drive.rise, gate->clock_delay->drive.fall,
                       gate->clock_delay->load, gate->clock_delay->max_load);
    }
}
#endif /* SIS */
