/*
        $Revision: 1.1.1.1 $
        $Date: 2004/02/07 10:14:59 $
*/
#ifdef SIS
/* Write a network in Jeff Burch's verifier format.
 */
#include "astg_core.h"
#include "astg_int.h"
#include "bwd_int.h"
#include "sis.h"
#include <ctype.h>

static int is_garbage_node(node_t *node) {
    node_t *fanout, *pi;
    lsGen  gen;

    /* check if it is a "garbage" node, i.e. it does not feed any latch */
    pi = NIL(node_t);
    foreach_fanout(node, gen, fanout) {
        if (node_type(fanout) == PRIMARY_OUTPUT) {
            pi = network_latch_end(fanout);
            if (pi != NIL(node_t) && node_type(pi) == PRIMARY_INPUT) {
                break;
            }
        }
    }
    return (pi == NIL(node_t));
}

static char *make_legal_name(char *name) {
    static char buf[512];
    int         i, len;

    len = strlen(name);
    if (len >= (sizeof(buf) / sizeof(char))) {
        fprintf(siserr, "name too long ?\n");
        return "";
    }

    strcpy(buf, name);
    for (i = 0; i < len; i++) {
        if (!isalnum(buf[i])) {
            buf[i] = '_';
        }
    }

    return buf;
}

static node_t *get_named_node(node_t *node) {
    node_t *fanout, *pi, *result;
    lsGen  gen;

    /* look if any first level fanout is a latch input, and if so
     * return its latch output (which is a PI)
     */
    pi = NIL(node_t);
    foreach_fanout(node, gen, fanout) {
        if (node_type(fanout) == PRIMARY_OUTPUT) {
            pi = network_latch_end(fanout);
            if (pi != NIL(node_t) && node_type(pi) == PRIMARY_INPUT) {
                break;
            }
        }
    }

    if (pi != NIL(node_t)) {
        result = pi;
        if (astg_debug_flag > 1) {
            (void) fprintf(siserr, "using %s for node %s\n", node_long_name(result),
                           node_long_name(node));
        }
    } else {
        result = node;
    }
    return result;
}

static char *get_gate_name(node_t *node) {
    char *name;

    name = lib_gate_name(lib_gate_of(node));

    if (!strncmp(name, "NAND", 3)) {
        return ("orgate");
    }
    if (!strncmp(name, "NOR", 2)) {
        return ("andgate");
    }
    if (!strncmp(name, "INV", 3)) {
        return ("inverter");
    }
    if (!strncmp(name, "DELAY", 5)) {
        return ("buffer");
    }

    return NIL(char);
}

void bwd_write_burch(FILE *file, network_t *network, double min_delay_factor, int write_spec) {
    node_t         *node, *pi, *po, *fanin, *fanin1, *n, *n1;
    lsGen          gen;
    delay_time_t   delay;
    int            max_delay, min_delay, max_hazard;
    int            i, j, nin;
    pset           p, last;
    char           *gate_name, *n_name;
    network_t      *collapsed_network;
    array_t        *node_vec, *in_value;
    astg_retval    status;
    astg_graph     *astg;
    astg_scode     state, bit;
    astg_signal    *sig_p;
    char           buf[80];
    st_table       *inputs, *outputs;
    astg_generator sgen;

    if (!lib_network_is_mapped(network)) {
        fprintf(siserr, "network not mapped, cannot write netlist\n");
        return;
    }

    /* initialize the node values */
    astg  = astg_current(network);
    state = (astg_scode) 0;
    if (astg == NIL(astg_graph)) {
        fprintf(siserr, "cannot find the STG\n");
        return;
    }
    status = astg_initial_state(astg, &state);
    if (status != ASTG_OK && status != ASTG_NOT_USC) {
        fprintf(siserr, "cannot find the initial state\n");
        return;
    }
    node_vec = network_dfs(network);
    in_value = array_alloc(int, 0);

    foreach_primary_input(network, gen, pi) {
        sig_p = astg_find_named_signal(astg, bwd_po_name(node_long_name(pi)));
        if (sig_p == NIL(astg_signal)) {
            fprintf(siserr, "cannot find signal for %s\n", node_long_name(pi));
            return;
        }
        bit = astg_state_bit(sig_p);
        if (bit & state) {
            array_insert_last(int, in_value, 1);
        } else {
            array_insert_last(int, in_value, 0);
        }
    }

    (void) simulate_network(network, node_vec, in_value);
    array_free(node_vec);
    array_free(in_value);

    /* initialize the input and output symbol tables */
    inputs  = st_init_table(strcmp, st_strhash);
    outputs = st_init_table(strcmp, st_strhash);
    astg_foreach_signal(astg, sgen, sig_p) {
        if (astg_signal_type(sig_p) == ASTG_DUMMY_SIG) {
            continue;
        }
        if (astg_signal_type(sig_p) == ASTG_OUTPUT_SIG) {
            st_insert(outputs, sig_p->name, NIL(char));
        } else {
            st_insert(inputs, sig_p->name, NIL(char));
        }
    }

    if (write_spec) {
        /* print out the specification */
        collapsed_network = network_dup(network);
        (void) network_collapse(collapsed_network);
        node_vec = network_dfs(collapsed_network);
        in_value = array_alloc(int, 0);

        foreach_primary_input(collapsed_network, gen, pi) {
            sig_p = astg_find_named_signal(astg, bwd_po_name(node_long_name(pi)));
            if (sig_p == NIL(astg_signal)) {
                fprintf(siserr, "cannot find signal for %s\n", node_long_name(pi));
                return;
            }
            bit = astg_state_bit(sig_p);
            if (bit & state) {
                array_insert_last(int, in_value, 1);
            } else {
                array_insert_last(int, in_value, 0);
            }
        }

        (void) simulate_network(collapsed_network, node_vec, in_value);
        array_free(node_vec);
        array_free(in_value);

        (void) fprintf(file, "\n(setq %s-spec\n",
                       make_legal_name(network_name(network)));
        (void) fprintf(file, " (teval\n");
        (void) fprintf(file, "   (compose\n");
        foreach_node(collapsed_network, gen, node) {
            if (is_garbage_node(node)) {
                continue;
            }

            nin = node_num_fanin(node);
            if (nin == 0) {
                (void) fprintf(siserr, "warning: constant node function %s\n",
                               node_long_name(node));
                continue;
            }

            n      = get_named_node(node);
            n_name = node_long_name(n);
            if (node->type == INTERNAL) {
                /* PI... no hazard */
                (void) fprintf(file, "    (no-hazard gate ( ");
            } else {
                (void) fprintf(file, "    (gate ( ");
            }
            foreach_fanin(node, i, fanin) {
                n = get_named_node(fanin);
                if (!strcmp(node_long_name(n), n_name)) {
                    continue;
                }
                if ((int) n->simulation) {
                    (void) fprintf(file, "(%s %d) ", make_legal_name(node_long_name(n)),
                                   n->simulation);
                } else {
                    (void) fprintf(file, "%s ", make_legal_name(node_long_name(n)));
                }
            }
            (void) fprintf(file, ")");
            n = get_named_node(node);
            if ((int) n->simulation) {
                (void) fprintf(file, "(%s %d)\n", make_legal_name(node_long_name(n)),
                               n->simulation);
            } else {
                (void) fprintf(file, "%s\n", make_legal_name(node_long_name(n)));
            }
            (void) fprintf(file, ";       insert here min del for %s\n",
                           node_long_name(n));
            (void) fprintf(file, "       (0 infty)\n");
            if (node->F->count > 1) {
                (void) fprintf(file, "         (logior\n");
            }
            foreach_set(node->F, last, p) {
                if (set_ord(p) < 2 * nin - 1) {
                    (void) fprintf(file, "         (logand ");
                } else {
                    (void) fprintf(file, "                 ");
                }
                for (i = 0; i < nin; i++) {
                    switch (GETINPUT(p, i)) {
                        case ZERO:fanin = node_get_fanin(node, i);
                            (void) fprintf(file, " (lognot %s)", node_long_name(fanin));
                            break;
                        case ONE:fanin = node_get_fanin(node, i);
                            (void) fprintf(file, " %s", node_long_name(fanin));
                            break;
                        default:break;
                    }
                }
                if (set_ord(p) < 2 * nin - 1) {
                    (void) fprintf(file, ")\n");
                } else {
                    (void) fprintf(file, "\n");
                }
            }
            if (node->F->count > 1) {
                (void) fprintf(file, "         )\n");
            }
            (void) fprintf(file, "    )\n");
        }
        (void) fprintf(file, ")))\n");
        network_free(collapsed_network);
    } else {
        /* initialize the delays */
        foreach_primary_input(network, gen, pi) {
            delay_set_parameter(pi, DELAY_ARRIVAL_RISE, (double) 0);
            delay_set_parameter(pi, DELAY_ARRIVAL_FALL, (double) 0);
        }
        foreach_primary_output(network, gen, po) {
            delay_set_parameter(po, DELAY_OUTPUT_LOAD, (double) 1.0);
        }

        assert(delay_trace(network, DELAY_MODEL_LIBRARY));

        /* print out the implementation */
        (void) fprintf(file, "(setq %s-impl\n",
                       make_legal_name(network_name(network)));
        (void) fprintf(file, " (project '(");
        foreach_primary_input(network, gen, node) {
            (void) fprintf(file, " %s", make_legal_name(node_long_name(node)));
        }
        (void) fprintf(file, " Phi)\n");

        (void) fprintf(file, "   (compose\n");

        foreach_node(network, gen, node) {
            if (node->type != INTERNAL ||
                (gate_name = get_gate_name(node)) == NIL(char)) {
                continue;
            }

            nin = node_num_fanin(node);
            if (nin == 0) {
                (void) fprintf(siserr, "warning: constant node function %s\n",
                               node_long_name(node));
                continue;
            }

            /* get the min and max delays through the gate */
            max_delay = 0;
            min_delay = INFINITY;
            foreach_fanin(node, i, fanin) {
                delay     = delay_node_pin(node, i, DELAY_MODEL_LIBRARY);
                max_delay = MAX(max_delay, delay.rise);
                max_delay = MAX(max_delay, delay.fall);
                min_delay = MIN(min_delay, delay.rise);
                min_delay = MIN(min_delay, delay.fall);
            }
            min_delay *= min_delay_factor;

            (void) fprintf(file, "    (chaos %s ", gate_name);

            if (nin == 1) {
                /* one fanin, no input node list */
                fanin = node_get_fanin(node, 0);
                assert(fanin != NIL(node_t));

                n = get_named_node(fanin);
                if ((int) n->simulation) {
                    (void) fprintf(file, "(%s %d)", make_legal_name(node_long_name(n)),
                                   (int) n->simulation);
                } else {
                    (void) fprintf(file, "%s", make_legal_name(node_long_name(n)));
                }
                if (astg_debug_flag == 1) {
                    (void) fprintf(siserr, "%s ", make_legal_name(node_long_name(n)));
                }
            } else {
                /* else, run through its fanin list*/
                (void) fprintf(file, "( ");
                foreach_fanin(node, i, fanin) {
                    n      = get_named_node(fanin);
                    n_name = node_long_name(n);
                    /* avoid duplicate names (it happens...) */
                    for (j = i + 1; j < nin; j++) {
                        fanin1 = node_get_fanin(node, j);
                        n1     = get_named_node(fanin1);
                        if (!strcmp(node_long_name(n1), n_name)) {
                            break;
                        }
                    }
                    if (j < nin) {
                        continue;
                    }

                    if ((int) n->simulation) {
                        (void) fprintf(file, "(-%s %d) ", make_legal_name(node_long_name(n)),
                                       (int) n->simulation);
                    } else {
                        (void) fprintf(file, "-%s ", make_legal_name(node_long_name(n)));
                    }
                    if (astg_debug_flag == 1) {
                        (void) fprintf(siserr, "%s ", make_legal_name(node_long_name(n)));
                    }
                }
                (void) fprintf(file, ")");
            }

            /* check if the node is not internal, and if so don't allow hazards */
            n      = get_named_node(node);
            n_name = make_legal_name(node_long_name(n));
            if (st_is_member(inputs, n_name)) {
                /* no hazards allowed here ! */
                max_hazard = -1;
                strcpy(buf, "infty");
            } else if (st_is_member(outputs, n_name)) {
                /* no hazards allowed here ! */
                max_hazard = -1;
                sprintf(buf, "%d", max_delay);
            } else {
                /* internal node: do not care about hazards */
                max_hazard = max_delay;
                sprintf(buf, "%d", max_delay);
            }

            if ((int) n->simulation) {
                (void) fprintf(file, " (%s %d) (%d %d))\n",
                               make_legal_name(node_long_name(n)), n->simulation,
                               min_delay, max_delay);
            } else {
                (void) fprintf(file, " %s (%d %d))\n",
                               make_legal_name(node_long_name(n)), min_delay, max_delay);
            }
            if (astg_debug_flag == 1) {
                (void) fprintf(siserr, "%s\n", n_name);
            }
        }
        (void) fprintf(file, ")))\n");
    }

    st_free_table(inputs);
    st_free_table(outputs);
}

#endif /* SIS */
