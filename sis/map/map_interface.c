
/* file @(#)map_interface.c	1.7 */
/* last modified on 7/31/91 at 13:01:20 */
#include "fanout_int.h"
#include "lib_int.h"
#include "map_delay.h"
#include "map_int.h"
#include "map_macros.h"
#include "sis.h"
#include <math.h>

#include "map_interface_static.h"

/* EXTERNAL INTERFACE */

/* takes a network and maps it. Ignores the state of the network (ANNOTATED,
 * MAPPED, etc...) */

network_t *complete_map_interface(network_t *network, bin_global_t *options) {
    int            ok_status;
    network_t      *result;
    network_type_t type;
    library_t      *library = options->library;

    error_init();

    if (!map_check_library(network, options)) {
        return NIL(network_t);
    }

    options->network_type = type = map_get_network_type(network);
    switch (type) {
        case RAW_NETWORK:break;
        case ANNOTATED_NETWORK:
        case MAPPED_NETWORK:unmap_network(network, options);
            break;
        case EMPTY_NETWORK:return NIL(network_t);
        default:error_append("map_interface: bad network type\n");
            return NIL(network_t);
    }

    /* remove unnecessary nodes */
    network_csweep(network);

    /* initialize things properly for delay */
    map_alloc_delay_info(network, options);

    /* initialize the network */
    if (!map_premap_network(network, options)) {
        error_append("map_interface: can't premap the network\n");
        map_free_delay_info();
        return NIL(network_t);
    }

    /* perform tree mapping */
    if (options->n_iterations > 0) {
        if (options->new_mode != 1 && !FP_EQUAL(options->new_mode_value, 1.0)) {
            error_append(
                    "map_interface: can't iterate any other option than '-n 1'\n");
            map_free_delay_info();
            return NIL(network_t);
        }
        options->fanout_optimize   = 1;
        options->area_recover      = 0;
        options->fanout_iterate    = 1;
        options->opt_single_fanout = 1;
        options->fanout_log_on     = 1;
        while (options->n_iterations > 0) {
            (void) bin_delay_tree_match(network, library, *options);
            map_remove_unnecessary_annotations(network);
            map_report_data_annotated(network, options);
            options->ignore_polarity = 0;
            fanout_optimization(network, *options);
            map_remove_unnecessary_annotations(network);
            map_report_data_annotated(network, options);
            fanout_log_cleanup_network(network);
            options->n_iterations--;
        }
        options->area_recover      = 1;
        options->fanout_iterate    = 0;
        options->opt_single_fanout = 1;
        options->fanout_log_on     = 0;
        (void) bin_delay_tree_match(network, library, *options);
        map_remove_unnecessary_annotations(network);
        map_report_data_annotated(network, options);
        fanout_optimization(network, *options);
    } else {
        if (options->new_mode == 0) { /* status == 1 iff everything is OK */
            ok_status = tree_map(network, library, *options);
        } else if (FP_EQUAL(options->new_mode_value, 1.0)) {
            ok_status = bin_delay_tree_match(network, library, *options);
        } else {
            ok_status = bin_delay_area_tree_match(network, library, *options);
        }
        if (!ok_status) {
            error_append("map_interface: can't treemap the network\n");
            map_free_delay_info();
            return NIL(network_t);
        }
        if (options->fanout_optimize)
            fanout_optimization(network, *options);
    }
    result                = build_network_from_annotations(network, options);
    map_free_delay_info();
    return result;
}

/* this routine takes an annotated network and maps it */
/* it also performs a basic consistency check for robustness */

network_t *build_mapped_network_interface(network_t *network, bin_global_t *options) {
    network_type_t type;

    error_init();
    options->network_type = type = map_get_network_type(network);
    if (type != ANNOTATED_NETWORK) {
        error_append("build_annotated_network:  expects an ANNOTATED network\n");
        return NIL(network_t);
    }
    return build_network_from_annotations(network, options);
}

/* takes a RAW or an ANNOTATED network and returns an ANNOTATED network */
/* only do tree mapping */

network_t *tree_map_interface(network_t *network, bin_global_t *options) {
    int            ok_status;
    network_type_t type;
    library_t      *library = options->library;

    error_init();
    options->network_type = type = map_get_network_type(network);
    switch (type) {
        case RAW_NETWORK:
        case ANNOTATED_NETWORK:break;
        case MAPPED_NETWORK:error_append("map_interface: network is already mapped\n");
            return NIL(network_t);
        case EMPTY_NETWORK:return NIL(network_t);
        default:error_append("map_interface: bad network type\n");
            return NIL(network_t);
    }

    /* initialize things properly for delay */
    map_alloc_delay_info(network, options);

    /* initialize the network */
    if (type == RAW_NETWORK) {
        if (!map_premap_network(network, options)) {
            error_append("map_interface: can't premap the network\n");
            return NIL(network_t);
        }
    }

    /* if ANNOTATED, only authorize '-n 1' for the moment: the others won't work
     */
    if (type == ANNOTATED_NETWORK) {
        if (options->new_mode == 0 ||
            (options->new_mode == 1 && !FP_EQUAL(options->new_mode_value, 1.0))) {
            if (!options->no_warning)
                (void) fprintf(
                        sisout,
                        "WARNING: only '_tree_map -n 1' supported after fanout opt\n");
            options->new_mode       = 1;
            options->new_mode_value = 1.0;
        }
    }

    /* do the tree mapping */
    if (options->new_mode == 0) { /* status == 1 iff everything is OK */
        ok_status = tree_map(network, library, *options);
    } else if (FP_EQUAL(options->new_mode_value, 1.0)) {
        ok_status = bin_delay_tree_match(network, library, *options);
    } else {
        ok_status = bin_delay_area_tree_match(network, library, *options);
    }
    if (!ok_status) {
        error_append("map_interface: can't treemap the network\n");
        return NIL(network_t);
    }

    /* cleanup annotations */
    map_remove_unnecessary_annotations(network);
    map_report_data_annotated(network, options);

    /* free delay info storage */
    map_free_delay_info();

    return network;
}

/* takes an ANNOTATED network and returns an ANNOTATED network */
/* performs fanout optimization and, if specified, area recovery */

network_t *fanout_opt_interface(network_t *network, bin_global_t *options) {
    network_type_t type;

    error_init();
    type = map_get_network_type(network);
    if (type == EMPTY_NETWORK)
        return NIL(network_t);
    if (type != ANNOTATED_NETWORK) {
        error_append("fanout_opt_interface: expects an annotated network\n");
        return NIL(network_t);
    }

    /* initialize things properly for delay */
    map_alloc_delay_info(network, options);

    fanout_optimization(network, *options);
    map_remove_unnecessary_annotations(network);
    map_report_data_annotated(network, options);

    /* free delay info storage */
    map_free_delay_info();

    return network;
}

/* returns the type of a network */

network_type_t map_get_network_type(network_t *network) {
    int    is_mapped     = 1;
    int    is_annotated  = 1;
    int    all_constants = 1;
    lsGen  gen;
    node_t *node;
#ifdef SIS
    latch_t *latch;
#endif /* SIS */

    if (network == NIL(network_t))
        return EMPTY_NETWORK;
    /* Hack: if all the internal nodes in a network are constants, return
       RAW_NETWORK. */
    foreach_node(network, gen, node) {
        if (node->type == INTERNAL) {
            if (node_num_fanin(node) == 0)
                continue; /* handle the case of constants */
            all_constants = 0;
            if (MAP(node) == 0) {
                is_mapped = is_annotated = 0;
                (void) lsFinish(gen);
                break;
            } else if (MAP(node)->gate == NIL(lib_gate_t)) {
                is_mapped = 0;
            } else if (MAP(node)->gate != NIL(lib_gate_t) &&
                       MAP(node)->save_binding == NIL(node_t *)) {
                is_annotated = 0;
            } else if (MAP(node)->gate != NIL(lib_gate_t) &&
                       MAP(node)->save_binding != NIL(node_t *)) {
                is_mapped = 0;
            }
        }
    }
    if (all_constants)
        return RAW_NETWORK;
#ifdef SIS
    foreach_latch(network, gen, latch) {
        if (latch_get_gate(latch) == NIL(lib_gate_t)) {
            is_mapped = 0;
        }
        node = latch_get_input(latch);
        if (node_num_fanin(node) == 0 ||
            lib_gate_of(node_get_fanin(node, 0)) == NIL(lib_gate_t)) {
            is_annotated = 0;
        }
    }
#endif /* SIS */
    if (is_mapped)
        return MAPPED_NETWORK;
    if (is_annotated)
        return ANNOTATED_NETWORK;
    return RAW_NETWORK;
}

/* INTERNAL INTERFACE */

/* decompose into 2-input NAND gates if needed */
/* add inverters if required */
/* and premap with 2-input NAND gates */

network_t *map_premap_network(network_t *network, bin_global_t *options) {
    library_t *library = options->library;

    /* If not 'raw' mapping, get it into 2-input nand gate form */
    if (!options->raw) {
        map_prep(network, library);
    } else {
        add_inv_network(network);
    }

    /* make sure it is the correct form */
    if (!map_check_form(network, library->nand_flag)) {
        return NIL(network_t);
    }

    /* add redundant inverters: should be consistent with the library */
    if (library->add_inverter) {
        map_add_inverter(network, /* add_at_pipo */ 1);
    }

    /* make sure there's at most 1 0-cell and 1 1-cell */
    patch_constant_cells(network);

    /* pre-map the circuit */
    if (!do_tree_premap(network, library)) {
        return NIL(network_t);
    }
    return network;
}

/* takes an ANNOTATED network, builds it, removes inverters if necessary */
/* and report stats if required */

static network_t *build_network_from_annotations(network_t *network, bin_global_t *options) {
    network_t *new_network = map_build_network(network);

    if (options->remove_inverters) {
        if (options->print_stat) {
            map_remove_inverter(new_network, map_report_data_mapped);
        } else {
            map_remove_inverter(new_network, (VoidFn) 0);
        }
    }
    if (options->print_stat)
        map_report_data_mapped(new_network);
    return new_network;
}

/* traverse the network from PO to PI through the virtual network */
/* and record all the nodes encountered there */
/* unmap the others */

static void map_remove_unnecessary_annotations(network_t *network) {
    lsGen    gen;
    node_t   *node, *fanin, *output;
    st_table *visited = st_init_table(st_ptrcmp, st_ptrhash);

    foreach_primary_output(network, gen, output) {
        st_insert(visited, (char *) output, NIL(char));
        fanin = map_po_get_fanin(output);
        visit_mapped_nodes(fanin, visited);
    }
    /* all nodes that would not appear in the mapping are reset */
    foreach_node(network, gen, node) {
        if (st_lookup(visited, (char *) node, NIL(char *)))
            continue;
        MAP(node)->gate    = NIL(lib_gate_t);
        MAP(node)->ninputs = 0;
        FREE(MAP(node)->save_binding);
        MAP(node)->save_binding = NIL(node_t *);
        gate_link_delete_all(node);
    }
    st_free_table(visited);
}

static void visit_mapped_nodes(node_t *node, st_table *visited) {
    int             i;
    node_function_t node_fn;
    char            **slot;

    if (st_find_or_add(visited, (char *) node, (char ***) &slot))
        return;
    *slot = NIL(char);
    node_fn = node_function(node);
    if (node_fn == NODE_PI || node_fn == NODE_0 || node_fn == NODE_1)
        return;
    assert(MAP(node)->save_binding != NIL(node_t * ));
    for (i = 0; i < MAP(node)->ninputs; i++) {
        visit_mapped_nodes(MAP(node)->save_binding[i], visited);
    }
}

void map_report_data_mapped(network_t *network) {
    node_t *node;
    lsGen  gen;

    assert(delay_trace(network, DELAY_MODEL_LIBRARY));

    foreach_primary_output(network, gen, node) {
        MAP(node)->map_arrival = delay_arrival_time(node);
    }
    map_report_data(network);
}

/* the network should have been cleaned up before calling this routine */
/* every gate is considered part of the final circuit */

static void map_report_data_annotated(network_t *network, bin_global_t *options) {
    if (!options->print_stat)
        return;
    map_compute_annotated_arrival_times(network, options);
    if (options->verbose > 1) {
        int     i;
        node_t  *node;
        array_t *nodevec = network_dfs(network);

        for (i = 0; i < array_n(nodevec); i++) {
            node = array_fetch(node_t *, nodevec, i);
            map_report_node_data(node);
        }
        array_free(nodevec);
    }
    map_report_data(network);
}

static void map_report_data(network_t *network) {
    node_t       *node;
    lsGen        gen;
    double       area = 0.0;
    int          n_failing_outputs;
    delay_time_t arrival, required, slack;
    delay_time_t max_slack, min_slack, sum_slack, max_arrival;

    n_failing_outputs = 0;
    max_slack         = MINUS_INFINITY;
    min_slack         = PLUS_INFINITY;
    sum_slack         = ZERO_DELAY;
    max_arrival       = MINUS_INFINITY;

    foreach_primary_output(network, gen, node) {
        arrival  = MAP(node)->map_arrival;
        required = pipo_get_po_required(node);
        SETSUB(slack, required, arrival);
        if (GETMIN(slack) < 0) {
            n_failing_outputs++;
            sum_slack.rise += (slack.rise < 0) ? slack.rise : 0.0;
            sum_slack.fall += (slack.fall < 0) ? slack.fall : 0.0;
        }
        SETMAX(max_slack, max_slack, slack);
        SETMIN(min_slack, min_slack, slack);
        SETMAX(max_arrival, max_arrival, arrival);
    }

    foreach_node(network, gen, node) {
        if (MAP(node)->gate != NIL(lib_gate_t)) {
            area += lib_gate_area(MAP(node)->gate);
        }
    }
    (void) fprintf(sisout, "# of outputs:          %d\n", network_num_po(network));
    (void) fprintf(sisout, "total gate area:       %2.2f\n", area);
    (void) fprintf(sisout, "maximum arrival time: (%2.2f,%2.2f)\n",
                   max_arrival.rise, max_arrival.fall);
    (void) fprintf(sisout, "maximum po slack:     (%2.2f,%2.2f)\n", max_slack.rise,
                   max_slack.fall);
    (void) fprintf(sisout, "minimum po slack:     (%2.2f,%2.2f)\n", min_slack.rise,
                   min_slack.fall);
    (void) fprintf(sisout, "total neg slack:      (%2.2f,%2.2f)\n", sum_slack.rise,
                   sum_slack.fall);
    (void) fprintf(sisout, "# of failing outputs:  %d\n", n_failing_outputs);
}

/* ARGSUSED */
static void map_compute_annotated_arrival_times(network_t *network, bin_global_t *options) {
    lsGen    gen;
    node_t   *output, *fanin;
    st_table *visited = st_init_table(st_ptrcmp, st_ptrhash);

    map_compute_loads(network);
    visited = st_init_table(st_ptrcmp, st_ptrhash);
    foreach_primary_output(network, gen, output) {
        fanin = map_po_get_fanin(output);
        virtual_delay_arrival_times_rec(visited, fanin);
        MAP(output)->map_arrival = MAP(fanin)->map_arrival;
    }
    st_free_table(visited);
}

static void map_compute_loads(network_t *network) {
    int     i, j;
    lsGen   gen;
    node_t  *fanin, *node;
    array_t *nodevec = network_dfs_from_input(network);

    foreach_node(network, gen, node) { MAP(node)->load = 0.0; }
    for (i = 0; i < array_n(nodevec); i++) {
        node = array_fetch(node_t *, nodevec, i);
        if (node->type == PRIMARY_OUTPUT) {
            MAP(node)->load = pipo_get_po_load(node);
            fanin = map_po_get_fanin(node);
            MAP(fanin)->load += MAP(node)->load;
        } else if (MAP(node)->gate == NIL(lib_gate_t)) {
            /* do nothing */
        } else if (strcmp(MAP(node)->gate->name, "**wire**") == 0) {
            fanin = MAP(node)->save_binding[0];
            MAP(fanin)->load += MAP(node)->load;
        } else {
            for (j = 0; j < MAP(node)->ninputs; j++) {
                fanin = MAP(node)->save_binding[j];
                MAP(fanin)->load += delay_get_load(MAP(node)->gate->delay_info[j]);
            }
        }
    }
    array_free(nodevec);
}

/* when calling the mapper on a mapped or annotated network directly */
/* without using _fanout_opt or _tree_map the network is cleaned up of any */
/* existing mapping info and the mapping is redone from scratch */
/* actually nothing to do: map_alloc will simply free the MAP entry */
/* if there is one */

/* ARGSUSED */
static void unmap_network(network_t *network, bin_global_t *options) { options->network_type = RAW_NETWORK; }

void map_report_node_data(node_t *node) {
    int          i;
    node_t       *fanin;
    delay_time_t arrival;

    if (node->type == INTERNAL && MAP(node)->gate == NIL(lib_gate_t))
        return;
    (void) fprintf(sisout, "node %s ", node->name);
    arrival = MAP(node)->map_arrival;
    (void) fprintf(sisout, "l(%2.3f) a(%2.3f,%2.3f) ", MAP(node)->load,
                   arrival.rise, arrival.fall);
    switch (node->type) {
        case PRIMARY_INPUT:(void) fprintf(sisout, "PI\n");
            break;
        case PRIMARY_OUTPUT:
            if (node_num_fanin(node) > 0) {
                fanin = map_po_get_fanin(node);
                (void) fprintf(sisout, "PO -> %s\n", fanin->name);
            } else {
                (void) fprintf(sisout, "PO\n");
            }
            break;
        case INTERNAL:(void) fprintf(sisout, "%s -> ", MAP(node)->gate->name);
            for (i = 0; i < MAP(node)->ninputs; i++) {
                fanin = MAP(node)->save_binding[i];
                (void) fprintf(sisout, "%s ", fanin->name);
            }
            (void) fprintf(sisout, "\n");
            break;
        default:;
    }
}

/* check library for completeness */
static int map_check_library(network_t *network, bin_global_t *options) {
    library_t  *library = options->library;
    lib_gate_t *gate;

#ifdef SIS
    latch_t       *latch;
    lib_gate_t    *d_ff_rising, *d_ff_falling;
    lib_gate_t    *d_latch_high, *d_latch_low;
    lib_gate_t    *d_asynch;
    lsGen         gen;
    latch_synch_t default_type;
#endif /* SIS */

    gate = choose_smallest_gate(library, "f=!a;");
    if (gate == NIL(lib_gate_t)) {
        error_append("library_error: no inverter in the library\n");
        return 0;
    }
    gate = choose_smallest_gate(library, "f=!(a*b);");
    if (gate == NIL(lib_gate_t)) {
        gate = choose_smallest_gate(library, "f=!(a+b);");
        if (gate == NIL(lib_gate_t)) {
            gate = choose_smallest_gate(library, "f=a*b;");
            if (gate == NIL(lib_gate_t)) {
                gate = choose_smallest_gate(library, "f=a+b;");
                if (gate == NIL(lib_gate_t)) {
                    error_append("library error: no 2-input nand in the library\n");
                    return 0;
                }
            }
        }
    }
    gate = choose_smallest_gate(library, "f=0;");
    if (gate == NIL(lib_gate_t)) {
        error_append("library error: no constant 0 cell in the library\n");
        return 0;
    }
    gate = choose_smallest_gate(library, "f=1;");
    if (gate == NIL(lib_gate_t)) {
        error_append("library error: no constant 1 cell in the library\n");
        return 0;
    }

#ifdef SIS
    d_ff_rising  = lib_choose_smallest_latch(library, "q=d;", RISING_EDGE);
    d_ff_falling = lib_choose_smallest_latch(library, "q=d;", FALLING_EDGE);
    d_latch_high = lib_choose_smallest_latch(library, "q=d;", ACTIVE_HIGH);
    d_latch_low  = lib_choose_smallest_latch(library, "q=d;", ACTIVE_LOW);
    d_asynch     = lib_choose_smallest_latch(library, "q=d;", ASYNCH);

    if (d_ff_rising != NIL(lib_gate_t))
        default_type = RISING_EDGE;
    else if (d_ff_falling != NIL(lib_gate_t))
        default_type = FALLING_EDGE;
    else if (d_latch_high != NIL(lib_gate_t))
        default_type = ACTIVE_HIGH;
    else if (d_latch_low != NIL(lib_gate_t))
        default_type = ACTIVE_LOW;
    else if (d_asynch != NIL(lib_gate_t))
        default_type = ASYNCH;
    else
        default_type = UNKNOWN;

    /* check if there are appropriate FFs/latches in the library */
    foreach_latch(network, gen, latch) {
        switch (latch_get_type(latch)) {
            case UNKNOWN:
                if (default_type == UNKNOWN) {
                    error_append("library error: no D-type flip-flops/latches/delays in "
                                 "the library\n");
                    goto error_return;
                } else {
                    if (!options->no_warning) {
                        (void) fprintf(
                                siserr, "warning: unknown latch type at node '%s' (%s assumed)\n",
                                node_name(latch_get_input(latch)),
                                (default_type == RISING_EDGE)
                                ? "RISING_EDGE"
                                : (default_type == FALLING_EDGE)
                                  ? "FALLING_EDGE"
                                  : (default_type == ACTIVE_HIGH)
                                    ? "ACTIVE_HIGH"
                                    : (default_type == ACTIVE_LOW) ? "ACTIVE_LOW"
                                                                   : "ASYNCH");
                    }
                    latch_set_type(latch, default_type);
                }
                break;

            case RISING_EDGE:
            case FALLING_EDGE:
                if (d_ff_rising == NIL(lib_gate_t) && d_ff_falling == NIL(lib_gate_t)) {
                    error_append("library error: no D-type edge-triggered flip-flops in "
                                 "the library\n");
                    goto error_return;
                }
                break;

            case ACTIVE_HIGH:
            case ACTIVE_LOW:
                if (d_latch_high == NIL(lib_gate_t) && d_latch_low == NIL(lib_gate_t)) {
                    error_append(
                            "library error: no D-type transparent latches in the library\n");
                    goto error_return;
                }
                break;

            case ASYNCH:
                if (d_asynch == NIL(lib_gate_t)) {
                    if (!options->no_warning) {
                        (void) fprintf(siserr, "warning: no asynch delay in the library\n");
                    }
                }
                break;

            default:error_append("library_error: impossible latch type?\n");
                goto error_return;
        }
    }
#endif /* SIS */

    return 1;

#ifdef SIS
    error_return:
    LS_ASSERT(lsFinish(gen));
    return 0;
#endif /* SIS */
}
