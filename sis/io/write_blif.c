
#include "io_int.h"
#include "sis.h"

static void do_write_blif();

static void write_blif_nodes();

static void write_blif_delay();

#ifdef SIS

static void write_blif_latches();

static void write_codes();

static void write_blif_clocks();

static st_table *write_add_buffers();

#endif /* SIS */

static void write_gen_event();

void write_blif_for_bdsyn(FILE *fp, network_t *network) {
    io_fput_params("\\\n", 32000);
    do_write_blif(fp, network, 0, 0);
}

void write_blif(FILE *fp, network_t *network, int short_flag, int netlist_flag) {
    io_fput_params("\\\n", 78);
    do_write_blif(fp, network, short_flag, netlist_flag);
}

static void do_write_blif(register FILE *fp, network_t *network, int short_flag, int netlist_flag) {
    node_t    *p;
    lsGen     gen;
    node_t    *node, *po, *fanin, *pnode;
    node_t    *nodein;
    network_t *dc_net;
    char      *name;
    int       po_cnt;
#ifdef SIS
    graph_t      *stg;
    st_generator *stgen;
    node_t       *pi_node, *po_node, *buf_node;
    st_table     *added_nodes;
    latch_t      *latch;
#endif /* SIS */

    /*
     * HACK: To preserve the names of latches in the case when the latch
     * output is also a true output. The latch output may be referred to in
     * the dont care network and so we need that name... Add buffers between
     * latch output (PI's) and true PO's ... Later remove so that the
     * network is unchanged.
     */
#ifdef SIS
    added_nodes = write_add_buffers(network);
#endif /* SIS */

    io_fprintf_break(fp, ".model %s\n.inputs", network_name(network));
    /*
     * Don't print out clocks and latch outputs
     */
    foreach_primary_input(network, gen, p) {
#ifdef SIS
        if (network_is_real_pi(network, p) != 0 &&
            clock_get_by_name(network, node_long_name(p)) == 0) {
            io_fputc_break(fp, ' ');
            io_write_name(fp, p, short_flag);
        }
#else
        io_fputc_break(fp, ' ');
        io_write_name(fp, p, short_flag);
#endif /* SIS */
    }

    io_fputc_break(fp, '\n');
    io_fputs_break(fp, ".outputs");
    /*
     * Don't print out control outputs and latch_inputs
     */
    foreach_primary_output(network, gen, p) {
#ifdef SIS
        if (network_is_real_po(network, p) != 0) {
            io_fputc_break(fp, ' ');
            io_write_name(fp, p, short_flag);
        }
#else
        io_fputc_break(fp, ' ');
        io_write_name(fp, p, short_flag);
#endif /* SIS */
    }

    io_fputc_break(fp, '\n');

#ifdef SIS
    write_blif_clocks(fp, network, short_flag);
#endif

    write_blif_delay(fp, network, short_flag);

#ifdef SIS
    write_blif_latches(fp, network, short_flag, netlist_flag);

    stg = network_stg(network);
    if (stg != NIL(graph_t)) {
        io_fputs_break(fp, ".start_kiss\n");
        (void) write_kiss(fp, stg);
        io_fputs_break(fp, ".end_kiss\n.latch_order");

        foreach_latch(network, gen, latch) {
            io_fputc_break(fp, ' ');
            io_write_name(fp, latch_get_output(latch), short_flag);
        }
        io_fputc_break(fp, '\n');

        write_codes(fp, stg);
    }
#endif /* SIS */

    write_blif_nodes(fp, network, netlist_flag, short_flag);

    if (network->dc_network != NIL(network_t)) {
        io_fprintf_break(fp, ".exdc \n.inputs");
        dc_net = network_dup(network->dc_network);

/* Get correct names for the fake PO's corresponding to the latches. */
#ifdef SIS
        foreach_primary_output(network, gen, po) {
            if (network_is_real_po(network, po))
                continue;
            if (!(node = network_find_node(dc_net, po->name)))
                continue;
            fanin      = po->fanin[0];
            if (node_function(fanin) == NODE_PI) {
                nodein = node->fanin[0];
                network_delete_node(dc_net, node);
                if (node_num_fanout(nodein) == 1)
                    network_delete_node(dc_net, nodein);
                continue;
            }
            po_cnt     = io_rpo_fanout_count(fanin, &pnode);
            if (po_cnt != 0) {
                nodein = node->fanin[0];
                network_delete_node(dc_net, node);
                if (node_num_fanout(nodein) == 0)
                    network_delete_node(dc_net, nodein);
                continue;
            }

            /* Try to preserve as much dont-care as possible.
             * For now this is only possible for D-type latches. */
            if (netlist_flag) {
                switch (lib_gate_type(lib_gate_of(fanin))) {
                    case RISING_EDGE:
                    case FALLING_EDGE:
                    case ACTIVE_HIGH:
                    case ACTIVE_LOW:
                    case ASYNCH:
                        if (node_function(fanin) == NODE_BUF) {
                            /* D-type FF */
                            fanin = fanin->fanin[0];
                        } else {
                            /* we have no way of propagating the dont-care
                             * info for complex latches */
                            (void) fprintf(
                                    siserr,
                                    "warning: can't preserve dont-care info for complex latch %s\n",
                                    fanin->name);
                            nodein = node->fanin[0];
                            network_delete_node(dc_net, node);
                            if (node_num_fanout(nodein) == 0)
                                network_delete_node(dc_net, nodein);
                            continue;
                        }
                        break;
                }
            }
            name       = util_strsav(fanin->name);
            if (!network_find_node(dc_net, name)) {
                /* If the name were in the dc_network, that means that a
                       DC function for this latch was already found.  The user
                       can give two constructs ".latch a b" ".latch a c" to
                       create two latches, but can only give one DC set for these
                       two latches.  extract_seq_dc will produce a DC set for each
                       of these latches, but the DC set will be the same. */
                network_change_node_name(dc_net, node, name);
            } else {
                network_delete_node(dc_net, node);
            }
        }
        network_cleanup(dc_net);
#endif /* SIS */

        foreach_primary_input(dc_net, gen, p) {
#ifdef SIS
            if (network_is_real_pi(dc_net, p) != 0) {
                io_fputc_break(fp, ' ');
                io_write_name(fp, p, short_flag);
            }
#else
            io_fputc_break(fp, ' ');
            io_write_name(fp, p, short_flag);
#endif /* SIS */
        }

        io_fputc_break(fp, '\n');
        io_fputs_break(fp, ".outputs");
        foreach_primary_output(dc_net, gen, p) {
#ifdef SIS
            if (network_is_real_po(dc_net, p) != 0) {
                io_fputc_break(fp, ' ');
                io_write_name(fp, p, short_flag);
            }
#else
            io_fputc_break(fp, ' ');
            io_write_name(fp, p, short_flag);
#endif /* SIS */
        }
        io_fputc_break(fp, '\n');

        write_blif_nodes(fp, dc_net, netlist_flag, short_flag);
        network_free(dc_net);
    }
    io_fputs_break(fp, ".end\n");

#ifdef SIS
    /*
     * Remove the extra nodes added to preserve latch names in the DC network
     */
    st_foreach_item(added_nodes, stgen, (char **) &buf_node, (char **) &po_node) {
        pi_node = node_get_fanin(buf_node, 0);
        node_patch_fanin(po_node, buf_node, pi_node);
        network_delete_node(network, buf_node);
    }
    st_free_table(added_nodes);
#endif /* SIS */
}

static void write_blif_nodes(FILE *fp, network_t *network, int netlist_flag, int short_flag) {
    lsGen  gen;
    node_t *p;

    foreach_node(network, gen, p) {
        if (netlist_flag != 0 && lib_gate_of(p) != NIL(lib_gate_t)) {
#ifdef SIS
            if (io_lpo_fanout_count(p, NIL(node_t *)) == 0) {
                /*
                 * Avoid the dummy nodes due to the latches
                 */
                io_write_gate(fp, p, short_flag);
                io_fputc_break(fp, '\n');
            }
#else
            io_write_gate(fp, p, short_flag);
            io_fputc_break(fp, '\n');
#endif /* SIS */
        } else {
            io_write_node(fp, p, short_flag);
        }
    }
}

static void write_blif_delay(FILE *fp, network_t *network, int short_flag) {
    write_blif_slif_delay(fp, network, 0, short_flag);
}

#ifdef SIS

static void write_blif_latches(FILE *fp, network_t *network, int short_flag, int netlist_flag) {
    lsGen   gen;
    latch_t *latch;
    node_t  *node, *control;

    foreach_latch(network, gen, latch) {
        control = latch_get_control(latch);
        if (netlist_flag != 0 && latch_get_gate(latch) != NIL(lib_gate_t)) {
            node = node_get_fanin(latch_get_input(latch), 0);
            io_write_gate(fp, node, short_flag);
            io_fputc_break(fp, ' ');
            if (control != NIL(node_t)) {
                io_write_name(fp, control, short_flag);
            } else {
                io_fputs_break(fp, "NIL");
            }
            io_fputc_break(fp, ' ');
            io_fputc_break(fp, latch_get_initial_value(latch) + '0');
            io_fputc_break(fp, '\n');
        } else {
            io_fputs_break(fp, ".latch    ");
            io_write_name(fp, latch_get_input(latch), short_flag);
            io_fputc_break(fp, ' ');
            io_write_name(fp, latch_get_output(latch), short_flag);
            io_fputc_break(fp, ' ');
            switch (latch_get_type(latch)) {
                case UNKNOWN:break;
                case RISING_EDGE:io_fputs_break(fp, "re");
                    break;
                case FALLING_EDGE:io_fputs_break(fp, "fe");
                    break;
                case ACTIVE_HIGH:io_fputs_break(fp, "ah");
                    break;
                case ACTIVE_LOW:io_fputs_break(fp, "al");
                    break;
                case ASYNCH:io_fputs_break(fp, "as");
                    break;
            }
            io_fputc_break(fp, ' ');
            if (control != NIL(node_t)) {
                io_write_name(fp, control, short_flag);
            } else if (latch_get_type(latch) != UNKNOWN) {
                io_fputs_break(fp, "NIL");
            }
            io_fputc_break(fp, ' ');
            io_fputc_break(fp, latch_get_initial_value(latch) + '0');
            io_fputc_break(fp, '\n');
        }
    }
}

static void write_codes(FILE *fp, graph_t *stg) {
    vertex_t *state;
    lsGen    gen;
    char     *code, *name;

    stg_foreach_state(stg, gen, state) {
        if ((
                    code = stg_get_state_encoding(state)
            ) != NIL(char)) {
            name = stg_get_state_name(state);
            io_fputs_break(fp,
                           ".code ");
            io_fputs_break(fp, name
                          );
            io_fputc_break(fp,
                           ' ');
            io_fputs_break(fp, code
                          );
            io_fputc_break(fp,
                           '\n');
        }
    }
}

static void write_blif_clocks(FILE *fp, network_t *network, int short_flag) {
    int         first = 1;
    lsGen       gen;
    sis_clock_t *clock;
    node_t      *node;
    double      cycle;
    st_table    *done_table;
    int         trans;

    foreach_clock(network, gen, clock) {
        if (first != 0) {
            io_fputs_break(fp,
                           ".clock");
            first = 0;
        }
        io_fputc_break(fp,
                       ' ');
        assert((node = network_find_node(network, clock_name(clock))) !=
               NIL(node_t));
        io_write_name(fp, node, short_flag
                     );
/*
io_fputs_break(fp, clock_name(clock));
*/
    }
    if (first == 0) {
        io_fputc_break(fp,
                       '\n');
    }

    cycle = clock_get_cycletime(network);
    if (cycle > 0) {
        io_fprintf_break(fp,
                         ".cycle %.2lf\n", cycle);
    }

    done_table = st_init_table(st_ptrcmp, st_ptrhash);
    foreach_clock(network, gen, clock) {
        if (
                st_lookup_int(done_table,
                              (char *) clock, &trans) != 0) {
            switch (trans) {
                case 3:break;
                case 2:write_gen_event(fp, clock, RISE_TRANSITION, done_table, short_flag);
                    break;
                default:write_gen_event(fp, clock, FALL_TRANSITION, done_table, short_flag);
                    break;
            }
        } else {
            write_gen_event(fp, clock, RISE_TRANSITION, done_table, short_flag);
            (void)
                    st_lookup_int(done_table,
                                  (char *) clock, &trans);
            if (trans != 3) {
                write_gen_event(fp, clock, FALL_TRANSITION, done_table, short_flag);
            }
        }
    }
    st_free_table(done_table);
}

/*
 * Utility routine that will get the name of the node representing the clock
 * This is required since short names may be used during writing. So we
 * cannot use the names stored with the sis_clock_t structures
 */
static char *io_clock_name(sis_clock_t *clock, int short_flag) {
    node_t *node;

    node = network_find_node(clock->network, clock_name(clock));
    assert(node != NIL(node_t));
    return io_name(node, short_flag);
}

static void write_gen_event(FILE *fp, sis_clock_t *clock, int transition, st_table *done_table, int short_flag) {
    clock_edge_t edge, new_edge;
    lsGen        gen;
    char         *dummy;
    int          done;
    char         prefix;

    edge.transition = transition;
    edge.clock      = clock;

    if (clock_get_parameter(edge, CLOCK_NOMINAL_POSITION) < 0) {
        return;
    }
    io_fputs_break(fp, ".clock_event ");
    prefix = (edge.transition == RISE_TRANSITION) ? 'r' : 'f';
    io_fprintf_break(fp, "%.2lf (%c'%s %.2lf %.2lf)",
                     clock_get_parameter(edge, CLOCK_NOMINAL_POSITION), prefix,
                     io_clock_name(clock, short_flag),
                     clock_get_parameter(edge, CLOCK_LOWER_RANGE),
                     clock_get_parameter(edge, CLOCK_UPPER_RANGE));

    if (st_lookup(done_table, (char *) (edge.clock), &dummy)) {
        done = 3;
    } else {
        done = (edge.transition == RISE_TRANSITION) ? 1 : 2;
    }
    (void) st_insert(done_table, (char *) (edge.clock), (char *) done);

    gen = clock_gen_dependency(edge);
    while (lsNext(gen, &dummy, LS_NH) == LS_OK) {
        new_edge = *((clock_edge_t *) dummy);
        prefix   = new_edge.transition == RISE_TRANSITION ? 'r' : 'f';
        io_fprintf_break(fp, " (%c'%s %.2lf %.2lf)", prefix,
                         io_clock_name(new_edge.clock, short_flag),
                         clock_get_parameter(new_edge, CLOCK_LOWER_RANGE),
                         clock_get_parameter(new_edge, CLOCK_UPPER_RANGE));
        if (st_lookup(done_table, (char *) (new_edge.clock), &dummy)) {
            done = 3;
        } else {
            done = (new_edge.transition == RISE_TRANSITION) ? 1 : 2;
        }
        (void) st_insert(done_table, (char *) (new_edge.clock), (char *) done);
    }
    (void) lsFinish(gen);
    io_fputc_break(fp, '\n');
}

static st_table *write_add_buffers(network_t *network) {
    lsGen    gen, gen1;
    latch_t  *latch;
    node_t   *pi, *po, *node;
    st_table *added_nodes;

    added_nodes = st_init_table(st_ptrcmp, st_ptrhash);

    if (network->dc_network == NIL(network_t)) {
        return added_nodes;
    }

    foreach_latch(network, gen, latch) {
        pi = latch_get_output(latch);
        foreach_fanout(pi, gen1, po) {
            if (network_is_real_po(network, po)) {
                node = node_literal(pi, 1);
                network_add_node(network, node);
                node_patch_fanin(po, pi, node);
                (void) st_insert(added_nodes, (char *) node, (char *) po);
            }
        }
    }
    return added_nodes;
}

#endif /* SIS */
