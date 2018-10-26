
#ifdef SIS

#include "retime_int.h"
#include "sis.h"

static int re_graph_dfs_recur();

static int retime_node_num_fanout();

static double retime_get_gate_load();

static double retime_get_user_constraint();

/*
 * For the node in the network, add a node in the retime graph...
 * Inherit the delay data as well as the default arrival times at the
 * primary inputs and req times at p/o
 */
void re_graph_add_node(re_graph *graph, node_t *node, int use_mapped, lib_gate_t *d_latch, st_table *node_to_id_table) {
    double     load;
    int        node_id;
    re_node    *re_no;
    lib_gate_t *gate;

    /* Allocation & initialization */
    re_no = retime_alloc_node();
    re_no->node = node;
    node_id = re_num_nodes(graph);
    array_insert_last(re_node *, graph->nodes, re_no);
    re_no->id = node_id;

    (void) st_insert(node_to_id_table, (char *) node, (char *) node_id);

    /* HERE --- add the user-time data from the p/i and p/o nodes */
    if (node->type == PRIMARY_INPUT) {
        re_no->type       = RE_PRIMARY_INPUT;
        re_no->user_time  = retime_get_user_constraint(node);
        re_no->final_area = re_no->final_delay = 0.0;
        array_insert_last(re_node *, graph->primary_inputs, re_no);
        return;
    } else if (node->type == PRIMARY_OUTPUT) {
        re_no->type       = RE_PRIMARY_OUTPUT;
        re_no->user_time  = retime_get_user_constraint(node);
        re_no->final_area = re_no->final_delay = 0.0;
        array_insert_last(re_node *, graph->primary_outputs, re_no);
        return;
    }

    re_no->type      = RE_INTERNAL;
    re_no->user_time = RETIME_USER_NOT_SET;

    /* Special casing for constant nodes */
    if (node_num_fanin(node) == 0) {
        re_no->final_area  = 0.0;
        re_no->final_delay = 0.0;
    }

    if (use_mapped) {
        /*
         * Get the gate from the node
         * Fill in the field for the node
         */
        gate = lib_gate_of(node);

        re_no->final_area = lib_gate_area(gate);
        load = retime_get_gate_load(node, d_latch);
        re_no->final_delay = retime_simulate_gate(gate, load);

    } else {
        /*
         * Put the area as the number of literals,
         * delay according to the unit fanout model
         */
        re_no->final_area  = node_num_literal(node);
        re_no->final_delay = 1.0 + 0.2 * retime_node_num_fanout(node);
    }
}

/*
 * gets the load driven by the node.. we only consider the actual nodes
 * being driven since the latches may be replaced or moved by retiming
 */
static double retime_get_gate_load(node_t *node, lib_gate_t *d_latch) {
    int         pin;
    lsGen       gen;
    node_t      *fo, *end;
    double      load, po_load;
    delay_pin_t *pin_delay;

    load = 0.0;
    foreach_fanout_pin(node, gen, fo, pin) {
        end = retime_network_latch_end(fo, d_latch);
        if (end == NIL(node_t)) {
            if (node_type(fo) == INTERNAL) {
                assert(lib_gate_of(fo) != NIL(lib_gate_t));
                pin_delay = (delay_pin_t *) lib_get_pin_delay(fo, pin);
                load += pin_delay->load;
            } else {
                /* Primary output node -- add its load if specified */
                if (delay_get_po_load(fo, &po_load))
                    load += po_load;
            }
        } else {
            /* the fanout is a latch --- Recursively compute its load */
            load += retime_get_gate_load(end, d_latch);
        }
    }
    return load;
}

/*
 * Recursively computes the number of fanouts of a node.
 * Traverses latches to find the true number of fanouts
 */
static int retime_node_num_fanout(node_t *node) {
    int    count;
    lsGen  gen;
    node_t *fo, *end;

    count = 0;
    foreach_fanout(node, gen, fo) {
        if ((end = retime_network_latch_end(fo, NIL(lib_gate_t))) == NIL(node_t)) {
            /* the fanout is not the input of a latch */
            count++;
        } else {
            /* Recursively use the end of the latch to compute fanouts */
            count += retime_node_num_fanout(end);
        }
    }
    return count;
}

int retime_get_dff_info(network_t *network, double *area_r, double *delay_r, latch_synch_t type) {
    double      d;
    lib_gate_t  *d_latch;
    delay_pin_t *pin_delay;

    if (lib_network_is_mapped(network)) {
        d_latch = lib_choose_smallest_latch(lib_get_library(), "f=a;", type);
        if (d_latch == NIL(lib_gate_t)) {
            error_append("Appropriate D-FF not in the library\n");
            return 1;
        }
        *area_r = lib_gate_area(d_latch);
        pin_delay = (delay_pin_t *) (d_latch->delay_info[0]);
        d         = retime_simulate_gate(d_latch, pin_delay->load);
        if (d > 0)
            *delay_r = d;
    }
    return 0;
}

double retime_simulate_gate(lib_gate_t *gate, double load) {
    int          i, j, n;
    delay_time_t t, **a_t;
    double       max_time, delay;

    n      = lib_gate_num_in(gate);
    a_t    = ALLOC(delay_time_t *, n);
    for (i = 0; i < n; i++) {
        a_t[i] = ALLOC(delay_time_t, 1);
    }

    delay = NEG_LARGE;

    for (i = 0; i < n; i++) {
        for (j       = 0; j < n; j++) {
            a_t[j]->rise = a_t[j]->fall = NEG_LARGE;
        }
        a_t[i]->rise = a_t[i]->fall = 0.0;

        t        = delay_map_simulate(n, a_t, gate->delay_info, load);
        max_time = MAX(t.rise, t.fall);
        delay    = MAX(max_time, delay);
    }
    for (i = 0; i < n; i++) {
        FREE(a_t[i]);
    }
    FREE(a_t);

    return delay;
}

/*
 * In some cases the edges may be generated twice. So, take care to see if an
 * edge with the same properties exists. If so, then do nothing...
 */
re_edge *re_graph_add_edge(re_graph *graph, node_t *from, node_t *to, int weight, double breadth, int index,
                           st_table *node_to_id_table) {
    re_edge *edge;
    re_node *from_node, *to_node;
    int     i, from_index, to_index;

    assert(st_lookup_int(node_to_id_table, (char *) from, &from_index));
    assert(st_lookup_int(node_to_id_table, (char *) to, &to_index));
    from_node = re_get_node(graph, from_index);
    to_node   = re_get_node(graph, to_index);

    re_foreach_fanout(from_node, i, edge) {
        if (edge->sink == to_node && edge->weight == weight &&
            edge->sink_fanin_id == index) {
            /* Edge with specified characteristics present --- do nothing */
            return edge;
        }
    }
    /* No edge present with same characteristics --- go on and create one */
    edge = re_create_edge(graph, from_node, to_node, index, weight, breadth);

    return edge;
}

array_t *re_graph_dfs(re_graph *graph) {
    int      i;
    re_node  *p;
    st_table *visited;
    array_t  *node_vec;

    visited  = st_init_table(st_ptrcmp, st_ptrhash);
    node_vec = array_alloc(re_node *, 0);

    re_foreach_primary_output(graph, i, p) {
        if (!re_graph_dfs_recur(p, node_vec, visited, 1, INFINITY, 0)) {
            fail("re_graph_dfs_recur: graph contains a zero weight cycle\n");
        }
    }

    st_free_table(visited);
    return node_vec;
}

array_t *re_graph_dfs_from_input(re_graph *graph) {
    int      i;
    re_node  *p;
    st_table *visited;
    array_t  *node_vec;

    visited  = st_init_table(st_ptrcmp, st_ptrhash);
    node_vec = array_alloc(re_node *, 0);

    re_foreach_primary_input(graph, i, p) {
        if (!re_graph_dfs_recur(p, node_vec, visited, 0, INFINITY, 0)) {
            fail("re_graph_dfs_recur: graph contains a 0 weight cycle\n");
        }
    }

    st_free_table(visited);
    return node_vec;
}

static int re_graph_dfs_recur(re_node *node, array_t *node_vec, st_table *visited, int dir, int level, int weight) {
    int     i, j;
    char    *value;
    re_node *re_no;
    re_edge *fanin, *fanout;

    if (level > 0) {

        if (st_lookup(visited, (char *) node, &value)) {
            /* if value of the node i== weight, then a
            zero weight cycle */
            return (int) value != weight;

        } else {
            /* add this node to the active path */
            value = (char *) weight;
            (void) st_insert(visited, (char *) node, value);

            /* avoid recursion if level-1 wouldn't add anything anyways */
            if (level > 1) {
                if (dir) {
                    re_foreach_fanin(node, i, fanin) {
                        if (re_ignore_edge(fanin))
                            continue;
                        re_no = fanin->source;
                        if (!re_graph_dfs_recur(re_no, node_vec, visited, dir, level - 1,
                                                weight + fanin->weight)) {
                            return 0;
                        }
                    }
                } else {
                    re_foreach_fanout(node, j, fanout) {
                        if (re_ignore_edge(fanout))
                            continue;
                        re_no = fanout->sink;
                        if (!re_graph_dfs_recur(re_no, node_vec, visited, dir, level - 1,
                                                weight + fanout->weight)) {
                            return 0;
                        }
                    }
                }
            }

            /* take this node off of the active path */
            value = (char *) (-1);
            (void) st_insert(visited, (char *) node, value);

            /* add node to list */
            array_insert_last(re_node *, node_vec, node);
        }
    }
    return 1;
}

re_node *retime_alloc_node() {
    re_node *re_no;

    re_no = ALLOC(re_node, 1);
    re_no->fanins           = array_alloc(re_edge *, 0);
    re_no->fanouts          = array_alloc(re_edge *, 0);
    re_no->scaled_delay     = RETIME_NOT_SET;
    re_no->scaled_user_time = 0;

    return (re_no);
}

int re_retime_node(re_node *node, int r) {
    int     i;
    re_edge *edge;

    if (node->type == RE_INTERNAL) {
        re_foreach_fanin(node, i, edge) {
            if (!re_ignore_edge(edge)) {
                edge->weight += r;
            }
        }
        re_foreach_fanout(node, i, edge) {
            if (!re_ignore_edge(edge)) {
                edge->weight -= r;
            }
        }
        return 0;
    } else {
        return 1;
    }
}

int retime_check_graph(re_graph *graph) {
    int     i, j;
    re_node *node;
    re_edge *edge;

    re_foreach_node(graph, i, node) {
        if (node->type == RE_PRIMARY_INPUT) {
            if (re_num_fanins(node) != 0) {
                error_append("E: PI with non-zero fanins detected\n");
                return 0;
            }
        }
        if (node->type == RE_PRIMARY_OUTPUT) {
            if (re_num_fanouts(node) != 0) {
                /* Added edges may be added to model register sharing */
                re_foreach_fanout(node, j, edge) {
                    if (re_ignore_edge(edge))
                        continue;
                    error_append("E: PO with non-zero fanouts detected\n");
                    return 0;
                }
            }
        }
    }

    re_foreach_edge(graph, i, edge) {
        if (re_ignore_edge(edge))
            continue;
        if (edge->weight < 0) {
            error_append("E: edge with negative weights encountered\n");
            return 0;
        }
    }
    return 1;
}

/*
 * Retimes a single node by an amount "lag"
 */
void retime_single_node(re_node *node, int lag) {
    int     i;
    re_edge *edge;

    if (lag == 0)
        return;
    re_foreach_fanout(node, i, edge) {
        if (re_ignore_edge(edge))
            continue;
        edge->weight -= lag;
        if (edge->latches != NIL(latch_t *)) {
            FREE(edge->latches);
            edge->latches = NIL(latch_t *);
        }
    }
    re_foreach_fanin(node, i, edge) {
        if (re_ignore_edge(edge))
            continue;
        edge->weight += lag;
        if (edge->latches != NIL(latch_t *)) {
            FREE(edge->latches);
            edge->latches = NIL(latch_t *);
        }
    }
}

void retime_dump_graph(FILE *fp, re_graph *graph) {
    int     i, j;
    re_node *re_no;
    re_edge *re_ed;

    (void) fprintf(fp, "Inputs --\n\t");
    re_foreach_primary_input(graph, i, re_no) {
        (void) fprintf(fp, "%s  ", node_name(re_no->node));
    }
    (void) fprintf(fp, "\nOutputs --\n\t");
    re_foreach_primary_output(graph, i, re_no) {
        (void) fprintf(fp, "%s  ", node_name(re_no->node));
    }
    (void) fprintf(fp, "\n\nFanin(weight) --> Node -->Fanouts(weight)\n\n");
    re_foreach_node(graph, i, re_no) {
        if (re_no->type != RE_PRIMARY_INPUT) {
            re_foreach_fanin(re_no, j, re_ed) {
                (void) fprintf(fp, "%s(%d)  ", node_name(re_ed->source->node),
                               re_ed->weight);
            }
            (void) fprintf(fp, " --> %s -->  ", node_name(re_no->node));
            re_foreach_fanout(re_no, j, re_ed) {
                (void) fprintf(fp, "%s(%d)  ", node_name(re_ed->sink->node),
                               re_ed->weight);
            }
            (void) fprintf(sisout, "\n\t Delay: %-5.2f , Area: %-5.0f\n",
                           re_no->final_delay, re_no->final_area);
        }
    }
}

/*
 * Get the user specified constraint for the PI/PO node
 */
static double retime_get_user_constraint(node_t *node) {
    int          flag;
    delay_time_t t;
    clock_edge_t edge;
    double       constraint = RETIME_USER_NOT_SET;

    if (node->type == INTERNAL)
        return constraint;

    if (delay_get_synch_edge(node, &edge, &flag)) {
        if (node->type == PRIMARY_INPUT && delay_get_pi_arrival_time(node, &t)) {
            /* Take the latest arrival time */
            if (flag == BEFORE_CLOCK_EDGE) {
                constraint = -1.0 * MIN(t.rise, t.fall);
            } else {
                constraint = MAX(t.rise, t.fall);
            }
        } else if (node->type == PRIMARY_OUTPUT &&
                   delay_get_po_required_time(node, &t)) {
            /* Take the earliest required time */
            if (flag == BEFORE_CLOCK_EDGE) {
                constraint = -1.0 * MAX(t.rise, t.fall);
            } else {
                constraint = MIN(t.rise, t.fall);
            }
        } else {
            /* Normally should not happen -- consider it unsynchronized */
        }
    } else {
        /* Not synchronized --- Assume no constraint on the signal */
    }

    return constraint;
}

/*
 * Determine if the network is fit to be retimed -- If so get the type and the
 * control information
 */
int retime_get_clock_data(network_t *network, int use_mapped, int *found_unknown_type, int *should_init,
                          latch_synch_t *old_type, double *delay_r, double *area_r) {
    lsGen         gen;
    latch_t       *latch;
    node_t        *control, *prev_control;
    latch_synch_t type;
    int           is_first = TRUE;
    int           all_twos;

    *found_unknown_type = FALSE;
    prev_control = NIL(node_t);
    *old_type = UNKNOWN;
    all_twos = TRUE; /* All latches have init_val (2)DONT_CARE or (3)UNSET */
    foreach_latch(network, gen, latch) {
        type = latch_get_type(latch);
        if (type == RISING_EDGE || type == FALLING_EDGE || type == UNKNOWN) {
            if (type != UNKNOWN) {
                if (is_first) {
                    *old_type = type;
                    is_first = FALSE;
                } else if (type != *old_type) {
                    error_append(
                            "ERROR: Synchronization types of clocked latches differ\n");
                    (void) lsFinish(gen);
                    return 1;
                }
                control = latch_get_control(latch);
                if (prev_control == NIL(node_t)) {
                    prev_control = control;
                } else if (prev_control != control) {
                    error_append("ERROR: Latches not clocked by same signal\n");
                    (void) lsFinish(gen);
                    return 1;
                }
            } else {
                *found_unknown_type = TRUE;
            }
            all_twos = (all_twos && (latch_get_initial_value(latch) >= 2));
        } else {
            error_append("Retiming not supported for TRANSPARENT latches\n");
            (void) lsFinish(gen);
            return 1;
        }
    }
    /* If all the latch values are twos do not compute initial states */
    if (all_twos)
        *should_init = 0;

    /*
     * Now that we know that the structure is OK -- also find the area
     * and delay thru a latch if it a mapped network
     */
    if (use_mapped) {
        /* get the area and delay corresponding to the D-FlopFlop */
        return (retime_get_dff_info(network, area_r, delay_r, *old_type));
    } else {
        return 0;
    }
}

/*
 * For convenience a single routine is provided to check if the
 * retiming is possible or not
 */
int retime_is_network_retimable(network_t *network) {
    int           flag;
    int           a, b;           /* Dummy variable for storing initialization data */
    latch_synch_t type; /* Dummy variable for storing latch type */
    double        d, e;        /* Dummy variable for storing latch data */

    flag = retime_get_clock_data(network, lib_network_is_mapped(network), &a, &b,
                                 &type, &d, &e);

    return (1 - flag);
}

#endif /* SIS */
