
#ifdef SIS

#include "sis.h"

static int report_error_trace();

static bdd_t *find_good_constraint();

static int node_table_entry_cmp();

static var_set_t **extract_support_info();

static void bdd_cube_print();

static void extract_support_info_rec();

static void node_table_entry_print();

extern void output_info_free();

static void use_cofactored_set();

static void seqbdd_get_one_edge();

static bdd_t *seqbdd_get_one_minterm();

static void bdd_add_varids_to_table();

static void free_bdd_array();

static void print_error_trace();

static st_table *get_pi_to_var_table();

static void print_error_input();

/* top level routine for the sequential verifier */
/* returns 0 iff OK, 1 if verification failed */

int seq_verify_interface(network_t *network1, network_t *network2, verif_options_t *options) {
    int           result;
    output_info_t output_info;
    range_data_t  *data;
    int           count1, count2;
    node_t        *input, *output;
    latch_t       *latch;
    int           value;
    lsGen         gen;

    options->does_verification = 1;
    output_info_init(&output_info);
    options->output_info           = &output_info;
    output_info.is_product_network = 1;
    count1 = 0;
    count2 = 0;
    foreach_primary_input(network1, gen, input) {
        if ((latch = latch_from_node(input)) == NIL(latch_t)) {
            if (!network_find_node(network2, input->name)) {
                fprintf(siserr, "input %s appears in network1 but not network2.\n",
                        input->name);
                fprintf(siserr, "Finite state machines are not equal.\n");
                return 1;
            }
            count1++;
        } else {
            value = latch_get_initial_value(latch);
            if ((value != 0) && (value != 1) && (value != 2)) {
                fprintf(
                        siserr,
                        "Latch with input %s and output %s is not properly initialized.\n",
                        node_name(latch_get_input(latch)),
                        node_name(latch_get_output(latch)));
                fprintf(siserr, "Its value should be 0, 1, or 2.\n");
                return 1;
            }
        }
    }
    foreach_primary_input(network2, gen, input) {
        if ((latch = latch_from_node(input)) == NIL(latch_t)) {
            count2++;
        } else {
            value = latch_get_initial_value(latch);
            if (value == 3) {
                fprintf(
                        siserr,
                        "Latch with input %s and output %s is not properly initialized.\n",
                        node_name(latch_get_input(latch)),
                        node_name(latch_get_output(latch)));
                fprintf(siserr, "Its value should be 0, 1, or 2 (don't care).\n");
                return 1;
            }
        }
    }
    if (count1 != count2) {
        fprintf(siserr, "The number of inputs in the networks are not equal.\n");
        fprintf(siserr, "Finite state machines are not equal.\n");
        return 1;
    }
    count1 = 0;
    count2 = 0;
    foreach_primary_output(network1, gen, output) {
        if (network_is_real_po(network1, output)) {
            if (!network_find_node(network2, output->name)) {
                fprintf(siserr, "output %s appears in network1 but not network2.\n",
                        output->name);
                fprintf(siserr, "Finite state machines are not equal.\n");
                return 1;
            }
            count1++;
        }
    }
    foreach_primary_output(network2, gen, output) {
        if (network_is_real_po(network2, output)) {
            count2++;
        }
    }
    if (count1 != count2) {
        fprintf(siserr, "The number of outputs in the networks are not equal.\n");
        fprintf(siserr, "Finite state machines are not equal.\n");
        return 1;
    }
    if (count1 == 0) {
        fprintf(siserr, "Verification not performed: no outputs in the network.\n");
        return 1;
    }
    switch (options->type) {
        case PRODUCT_METHOD:
        case BULL_METHOD:output_info.generate_global_output = 0;
            break;
        case CONSISTENCY_METHOD:output_info.generate_global_output = 1;
            break;
        default: fail("unregistered range computation method");
    }
    extract_product_network_info(network1, network2, options);
    result = breadth_first_stg_traversal(&network2, &data, options);
    /*  if (output_info.init_node == NIL (node_t)) {
        return 1;
      }*/
    if (result == 0) {
        fprintf(siserr,
                "The finite state machines have the same sequential behavior.\n");
    } else {
        if (options->type != PRODUCT_METHOD) {
            (void) fprintf(sisout, "error trace only reported for product method\n");
        } else {
            report_error_trace(data, options);
        }
    }
    (*options->free_range_data)(data, options);
    output_info_free(&output_info);
    return result;
}

static int report_error_trace(range_data_t *range_data, verif_options_t *options) {
    int     output_index;
    bdd_t   *current_set;
    bdd_t   *new_current_set;
    bdd_t   *total_set;
    bdd_t   *new_total_set;
    bdd_t   *output_fn;
    bdd_t   *incorrect_support;
    bdd_t   *incorrect_state;
    bdd_t   *incorrect_input;
    bdd_t   *tmp;
    array_t *total_sets;
    array_t *input_trace;
    int     iter_count  = 0;
    int     error_found = 0;

    total_sets  = array_alloc(bdd_t *, 0);
    current_set = bdd_dup(range_data->init_state_fn);
    total_set   = bdd_zero(range_data->manager);

    for (;;) {
        if (options->does_verification) {
            if (!(*options->check_output)(current_set, range_data, &output_index,
                                          options)) {
                error_found = 1;
                break;
            }
        }
        if (!options->does_verification && iter_count >= options->n_iter)
            break;
        use_cofactored_set(&current_set, &total_set, bdd_cofactor);
        array_insert(bdd_t *, total_sets, iter_count, bdd_dup(total_set));
        if (options->does_verification && bdd_is_tautology(total_set, 1))
            break;
        range_data->total_set = total_set;
        new_current_set =
                (*options->compute_next_states)(current_set, range_data, options);
        bdd_free(current_set);
        current_set = new_current_set;
        iter_count++;
    }
    assert(error_found);
    output_fn         = array_fetch(bdd_t *, range_data->external_outputs, output_index);
    incorrect_support = bdd_and(current_set, output_fn, 1, 0);
    input_trace       = array_alloc(bdd_t *, 0);
    for (;;) {
        seqbdd_get_one_edge(incorrect_support, range_data, &incorrect_state,
                            &incorrect_input, options);
        bdd_free(incorrect_support);
        array_insert(bdd_t *, input_trace, iter_count, incorrect_input);
        if (iter_count == 0)
            break;
        iter_count--;
        tmp =
                (*options->compute_reverse_image)(incorrect_state, range_data, options);
        total_set         = array_fetch(bdd_t *, total_sets, iter_count);
        incorrect_support = bdd_and(tmp, total_set, 1, 1);
        bdd_free(tmp);
    }
    free_bdd_array(total_sets);
    print_error_trace(input_trace, range_data, options);
    free_bdd_array(input_trace);
    report_elapsed_time(options, "end error trace generation");
    bdd_free(current_set);
    return;
}

network_t *range_computation_interface(network_t *network, verif_options_t *options) {
    output_info_t output_info;
    range_data_t  *data;

    options->does_verification = 0;
    output_info_init(&output_info);
    options->output_info           = &output_info;
    output_info.is_product_network = 0;
    extract_network_info(network, options);
    if (output_info.init_node == NIL(node_t))
        return 0;
    (void) breadth_first_stg_traversal(&network, &data, options);
    (*options->free_range_data)(data, options);
    output_info_free(&output_info);
    return network;
}

/* INTERNAL INTERFACE */

/* returns 0 if it verifies, 1 otherwise */

/* WARNING: the output_fn is recomputed by some algorithms */
/* please do not replace its occurrences by an initialized local variable */
/* ultimately this should be changed to have a more coherent computation of
 * output_fn */
/* in particular, here the extraction of error pattern is wrong in case
 * output_fn */
/* is recomputed at each iteration */

int breadth_first_stg_traversal(network_t **network, range_data_t **data, verif_options_t *options) {
    int          i      = 0;
    int          result = 0;
    int          fn_size, output_size;
    bdd_t        *new_current_set;
    bdd_t        *unreached_states;
    range_data_t *range_data;
    bdd_t        *current_set;
    bdd_t        *total_set;
    int          index;
    node_t       *node;
    st_generator *gen;
    lsGen        listgen;
    array_t      *var_names;
    st_table     *leaves;
    network_t    *new_network;
    array_t      *nodes_array;
    char         *exdc_prefix;
    int          prefix_lenght;
    char         *name;

    range_data  = (*options->alloc_range_data)(*network, options);
    current_set = bdd_dup(range_data->init_state_fn);
    total_set   = bdd_zero(range_data->manager);
    leaves      = options->output_info->pi_ordering;

    for (;;) {
        if (options->does_verification) {
            if (!(*options->check_output)(current_set, range_data, NIL(int),
                                          options)) {
                result = 1;
                break;
            }
        }
        if (bdd_leq(current_set, total_set, 1, 1))
            break;
        if (!options->does_verification && i >= options->n_iter)
            break;
        use_cofactored_set(&current_set, &total_set, bdd_cofactor);
        if (options->does_verification && bdd_is_tautology(total_set, 1))
            break;
        range_data->total_set = total_set;
        new_current_set =
                (*options->compute_next_states)(current_set, range_data, options);
        bdd_free(current_set);
        current_set = new_current_set;
        i++;
    }
    if (!options->does_verification) {
        fprintf(sisout, "\n");
        fprintf(
                sisout,
                "number of latches = %d     depth = %d     states visited = %2.0f\n",
                network_num_latch(*network), i,
                bdd_count_onset(total_set, range_data->pi_inputs));
    }
    if (!options->does_verification && !options->keep_old_network) {
        var_names = array_alloc(char *, st_count(leaves));
        st_foreach_item_int(leaves, gen, (char **) &node, &index) {
            array_insert(char *, var_names, index, node->name);
        }
        unreached_states = bdd_not(total_set);
        new_network =
                ntbdd_bdd_single_to_network(unreached_states, "foo", var_names);
        nodes_array   = network_dfs(new_network);
        exdc_prefix   = "dc";
        prefix_lenght = strlen(exdc_prefix) + 1;
        for (i        = 0; i < array_n(nodes_array); i++) {
            node = array_fetch(node_t *, nodes_array, i);
            if (node_function(node) == NODE_PI)
                continue;
            if (node_function(node) == NODE_PO)
                continue;
            name = ALLOC(char, (int) (strlen(node->name) + prefix_lenght));
            (void) sprintf(name, "%s%s", exdc_prefix, node->name);
            network_change_node_name(new_network, node, name);
        }
        foreach_node(new_network, listgen, node) { node->bdd = (char *) NIL(bdd_t); }
        bdd_free(unreached_states);
        array_free(var_names);
        array_free(nodes_array);

        eliminate(new_network, 0, 10000);
        /* The old network is not freed here.  It is the responsibility of
           any routine calling breadth_first_traversal and asking NOT to keep
           the old network to free that network itself.  The reason is that
           a later call to ntbdd_end_manager will core dump if this network
           is freed now */
        /* network_free(*network); */
        *network = new_network;
    }
    bdd_free(current_set);
    *data = range_data;
    return result;
}

static bdd_t *find_good_constraint(bdd_t *current_set, bdd_t *total_set, BddFn cofactor_fn) {
    bdd_t *care_set = bdd_not(total_set);
    bdd_t *result   = (*cofactor_fn)(current_set, care_set);
    bdd_free(care_set);
    return result;
}

/* leaves: mapping of PI nodes to small integers -> BDD varid's */

void report_inconsistency(bdd_t *current_set, bdd_t *output_fn, st_table *leaves) {
    bdd_gen *gen;
    array_t *cube;
    bdd_t   *wrong_set = bdd_and(current_set, output_fn, 1, 0);

    assert(!bdd_is_tautology(wrong_set, 0));
    gen = bdd_first_cube(wrong_set, &cube);
    (void) fprintf(misout, "\nWARNING: verification failed\nfailing minterm ->\n");
    bdd_cube_print(cube, leaves);
    bdd_gen_free(gen);
}

static void use_cofactored_set(bdd_t **current_set, bdd_t **total_set, BddFn cofactor_fn) {
    bdd_t *new_total_set, *new_current_set;

    new_current_set = find_good_constraint(*current_set, *total_set, cofactor_fn);
    new_total_set   = bdd_or(*current_set, *total_set, 1, 1);
    bdd_free(*current_set);
    bdd_free(*total_set);
    *total_set   = new_total_set;
    *current_set = new_current_set;
}

/* "network" is the network to be augmented */
/* "node" is node from some other network */
/* "pi_table" maps PI names to nodes in "network" */
/* "node_table" table of nodes already copied and their corresponding node in
 * "network" */

node_t *network_copy_subnetwork(network_t *network, node_t *node, st_table *pi_table, st_table *node_table) {
    int    i;
    node_t *fanin;
    node_t *new_node;

    if (st_lookup(node_table, (char *) node, (char **) &new_node))
        return new_node;
    if (node->type == PRIMARY_INPUT) {
        node_t *pi;
        assert(st_lookup(pi_table, node->name, (char **) &pi));
        st_insert(node_table, (char *) node, (char *) pi);
        return pi;
    }
    if (node->type == PRIMARY_OUTPUT) {
        new_node = network_copy_subnetwork(network, node_get_fanin(node, 0),
                                           pi_table, node_table);
        if (new_node->type == PRIMARY_INPUT) {
            new_node = node_literal(new_node, 1);
            network_add_node(network, new_node);
            network_change_node_name(network, new_node, util_strsav(node->name));
        } else {
            network_change_node_name(network, new_node, util_strsav(node->name));
        }
        return network_add_primary_output(network, new_node);
    }
    new_node = node_dup(node);
    foreach_fanin(node, i, fanin) {
        node_t *new_fanin =
                       network_copy_subnetwork(network, fanin, pi_table, node_table);
        new_node->fanin[i] = new_fanin;
    }
    network_add_node(network, new_node);
    st_insert(node_table, (char *) node, (char *) new_node);
    return new_node;
}

void print_node_array(array_t *array) {
    int i;
    for (i = 0; i < array_n(array); i++) {
        node_t *node = array_fetch(node_t *, array, i);
        (void) fprintf(misout, "%s ", (node == NIL(node_t)) ? "---" : node->name);
    }
    (void) fprintf(misout, "\n");
}

typedef struct {
    node_t *node;
    int    value;
} node_table_entry_t;

static int node_table_entry_cmp(char *obj1, char *obj2) {
    int                diff;
    node_table_entry_t *a = (node_table_entry_t *) obj1;
    node_table_entry_t *b = (node_table_entry_t *) obj2;

    diff = a->value - b->value;
    if (diff == 0) {
        diff = strcmp(a->node->name, b->node->name);
    }
    return diff;
}

static void node_table_entry_print(node_table_entry_t *entry) {
    (void) fprintf(misout, "%s=>%d ", entry->node->name, entry->value);
}

void print_node_table(st_table *table) {
    int                i;
    node_t             *node;
    st_generator       *gen;
    node_table_entry_t entry;
    array_t            *array = array_alloc(node_table_entry_t, 0);

    st_foreach_item_int(table, gen, (char **) &node, &i) {
        entry.node  = node;
        entry.value = i;
        array_insert_last(node_table_entry_t, array, entry);
    }
    array_sort(array, node_table_entry_cmp);
    for (i = 0; i < array_n(array); i++) {
        entry = array_fetch(node_table_entry_t, array, i);
        node_table_entry_print(&entry);
    }
    (void) fprintf(misout, "\n");
    array_free(array);
}

/* skip over the NIL(node_t) entries */
/* if node does not have a BDD associated with it, should be registered in
 * leaves */
/* make one for it */

array_t *bdd_extract_var_array(bdd_manager *manager, array_t *node_list, st_table *leaves) {
    int     i;
    int     index;
    bdd_t   *var;
    node_t  *node;
    array_t *result = array_alloc(bdd_t *, array_n(node_list));

    for (i = 0; i < array_n(node_list); i++) {
        node = array_fetch(node_t *, node_list, i);
        if (node == NIL(node_t))
            continue;
        var = ntbdd_at_node(node);
        if (var == NIL(bdd_t)) {
            assert(st_lookup_int(leaves, (char *) node, &index));
            var = bdd_get_variable(manager, index);
            ntbdd_set_at_node(node, var);
        }
        array_insert_last(bdd_t *, result, var);
    }
    return result;
}

/* heuristic: order the po by minimum increase in total support size so far */
/* first: minimum support size; second, the one that increases the previous */
/* support by a minimum amount and so on... */
/* if use_good_heuristic is on, uses the heuristic in file ordering.c */

array_t *get_po_ordering(network_t *network, array_t *next_state_po, verif_options_t *options) {
    node_t     *node;
    int        i, index;
    set_info_t info;
    int        n_pi           = network_num_pi(network);
    int        n_po           = array_n(next_state_po);
    array_t    *index_ordering;
    array_t    *ordering      = array_alloc(node_t *, 0);
    var_set_t  **support_info = extract_support_info(network, next_state_po);

    info.n_vars = n_pi;
    info.n_sets = n_po;
    info.sets   = support_info;
    index_ordering = find_best_set_order(&info, options);
    assert(array_n(index_ordering) == n_po);
    for (i = 0; i < array_n(index_ordering); i++) {
        index = array_fetch(int, index_ordering, i);
        node  = array_fetch(node_t *, next_state_po, index);
        array_insert_last(node_t *, ordering, node);
    }
    array_free(index_ordering);
    for (i = 0; i < n_po; i++) {
        var_set_free(support_info[i]);
    }
    FREE(support_info);
    return ordering;
}

/* returns an array of n_po var_set_t containing */
/* in bitmap the info on which PI is in the support of which PO */

static var_set_t **extract_support_info(network_t *network, array_t *node_list) {
    int       i, uid;
    lsGen     gen;
    node_t    *input, *output;
    int       n_pi           = network_num_pi(network);
    int       n_po           = array_n(node_list);
    st_table  *pi_table      = st_init_table(st_ptrcmp, st_ptrhash);
    var_set_t **support_info = ALLOC(var_set_t *, n_po);

    for (i = 0; i < n_po; i++) {
        support_info[i] = var_set_new(n_pi);
    }
    uid = 0;
    foreach_primary_input(network, gen, input) {
        st_insert(pi_table, (char *) input, (char *) uid);
        uid++;
    }
    for (i = 0; i < array_n(node_list); i++) {
        st_table *visited = st_init_table(st_ptrcmp, st_ptrhash);
        output = array_fetch(node_t *, node_list, i);
        extract_support_info_rec(output, pi_table, visited, support_info[i]);
        st_free_table(visited);
    }
    st_free_table(pi_table);
    return support_info;
}

static void extract_support_info_rec(node_t *node, st_table *pi_table, st_table *visited, var_set_t *set) {
    int    i;
    node_t *fanin;
    int    uid;

    if (st_lookup(visited, (char *) node, NIL(char *)))
        return;
    st_insert(visited, (char *) node, NIL(char));
    if (node->type == PRIMARY_INPUT) {
        assert(st_lookup_int(pi_table, (char *) node, &uid));
        var_set_set_elt(set, uid);
    } else {
        foreach_fanin(node, i, fanin) {
            extract_support_info_rec(fanin, pi_table, visited, set);
        }
    }
}

/* starting with po_ordering, get new input variables needed for each po */
/* and after each po is done, add up the corresponding new_pi */

st_table *get_pi_ordering(network_t *network, array_t *po_ordering, array_t *new_pi) {
    lsGen        gen;
    int          i, node_index, node_id;
    node_t       *new_input, *node, *pi;
    var_set_t    *support;
    char         *key, *value;
    st_generator *table_gen;
    array_t      *old_pi;
    st_table     *result;
    array_t      *remaining_po, *other_pi;
    array_t      *pi_order      = array_alloc(node_t *, 0);
    st_table     *pi_table      = st_init_table(st_ptrcmp, st_ptrhash);
    var_set_t    **support_info = extract_support_info(network, po_ordering);
    var_set_t    *so_far        = var_set_new(network_num_pi(network));

    /* extract basic order of PI's out of the network */
    node_id = 0;
    foreach_primary_input(network, gen, node) {
        st_insert(pi_table, (char *) node, (char *) node_id);
        node_id++;
    }

    /* extract a good PI order, respecting the po_ordering */
    old_pi = order_nodes(po_ordering, 2);

    /* order the PI's wrt to the remaining PO's */
    remaining_po = get_remaining_po(network, po_ordering);
    other_pi     = order_nodes(remaining_po, 1);
    array_free(remaining_po);

    /* add the new_inputs and check for consistency */
    node_index = 0;
    for (i     = 0; i < array_n(po_ordering); i++) {
        new_input = array_fetch(node_t *, new_pi, i);
        support   = support_info[i];
        for (;;) {
            if (node_index >= array_n(old_pi))
                break;
            node = array_fetch(node_t *, old_pi, node_index);
            assert(st_lookup_int(pi_table, (char *) node, &node_id));
            if (!var_set_get_elt(support, node_id))
                break;
            assert(!var_set_get_elt(so_far, node_id));
            array_insert_last(node_t *, pi_order, node);
            st_delete(pi_table, (char **) &node, NIL(char *));
            node_index++;
        }
        array_insert_last(node_t *, pi_order, new_input);
        st_delete(pi_table, (char **) &new_input, NIL(char *));
        (void) var_set_or(so_far, so_far, support);
    }

    /* put the PI's only dependent on external PO's */
    for (i = 0; i < array_n(other_pi); i++) {
        node = array_fetch(node_t *, other_pi, i);
        if (st_delete(pi_table, (char **) &node, NIL(char *))) {
            array_insert_last(node_t *, pi_order, node);
        }
    }

    /* put at the end those PI's left over if any */
    st_foreach_item(pi_table, table_gen, &key, &value) {
        array_insert_last(node_t *, pi_order, (node_t *) key);
    }
    assert(network_num_pi(network) == array_n(pi_order));

    /* free stuff */
    array_free(other_pi);
    st_free_table(pi_table);
    var_set_free(so_far);
    if (old_pi != NIL(array_t))
        array_free(old_pi);
    for (i = 0; i < array_n(po_ordering); i++) {
        var_set_free(support_info[i]);
    }
    FREE(support_info);

    result = from_array_to_table(pi_order);
    array_free(pi_order);
    return result;
}

array_t *get_remaining_po(network_t *network, array_t *po_array) {
    int      i;
    lsGen    gen;
    node_t   *po;
    array_t  *remaining_po = array_alloc(node_t *, 0);
    st_table *po_table     = st_init_table(st_ptrcmp, st_ptrhash);

    for (i = 0; i < array_n(po_array); i++) {
        po = array_fetch(node_t *, po_array, i);
        st_insert(po_table, (char *) po, NIL(char));
    }
    foreach_primary_output(network, gen, po) {
        if (st_lookup(po_table, (char *) po, NIL(char *)))
            continue;
        array_insert_last(node_t *, remaining_po, po);
    }
    st_free_table(po_table);
    return remaining_po;
}

static output_info_t DEFAULT_OUTPUT_INFO = {0, 0, 0, 0, 0, 0, 0,
                                            0, 0, 0, {0, 0}, 0, 0, 0};

void output_info_init(output_info_t *info) { *info = DEFAULT_OUTPUT_INFO; }

void output_info_free(output_info_t *info) {
    if (info->org_pi)
        array_free(info->org_pi);
    if (info->extern_pi)
        array_free(info->extern_pi);
    if (info->pi_ordering)
        st_free_table(info->pi_ordering);
    if (info->po_ordering)
        array_free(info->po_ordering);
    if (info->new_pi)
        array_free(info->new_pi);
    if (info->xnor_nodes)
        array_free(info->xnor_nodes);
    if (info->name_table)
        st_free_table(info->name_table);
    if (info->transition_nodes)
        array_free(info->transition_nodes);
}

void report_elapsed_time(verif_options_t *options, char *comment) {
    int    last_time = options->last_time;
    int    new_time  = util_cpu_time();
    double elapsed;
    double total;

    options->last_time = new_time;
    options->total_time += (new_time - last_time);
    elapsed = (double) (new_time - last_time) / 1000;
    total   = (double) (options->total_time) / 1000;
    (void) fprintf(misout, "*** [elapsed(%2.1f),total(%2.1f)] ***\n", elapsed,
                   total);
    if (comment != NIL(char)) {
        (void) fprintf(misout, "%s...\n", comment);
    }
}

st_table *from_array_to_table(array_t *array) {
    int      i;
    node_t   *node;
    st_table *table = st_init_table(st_ptrcmp, st_ptrhash);

    for (i = 0; i < array_n(array); i++) {
        node = array_fetch(node_t *, array, i);
        st_insert(table, (char *) node, (char *) i);
    }
    return table;
}

static void bdd_cube_print(array_t *cube, st_table *leaves) {
    int          i;
    int          phase;
    int          index;
    node_t       *node;
    array_t      *names;
    st_generator *gen;
    char         *name;
    static char  cube_symbols[] = "01-";

    names = array_alloc(char *, 0);
    st_foreach_item_int(leaves, gen, (char **) &node, &index) {
        array_insert(char *, names, index, node_long_name(node));
    }
    assert(array_n(names) == array_n(cube));
    for (i = 0; i < array_n(cube); i++) {
        name = array_fetch(char *, names, i);
        (void) fprintf(misout, "%s ", name);
    }
    (void) fprintf(misout, "\n");
    array_free(names);
    for (i = 0; i < array_n(cube); i++) {
        phase = array_fetch(int, cube, i);
        (void) fprintf(misout, "%c", cube_symbols[phase]);
    }
    (void) fprintf(misout, "\n");
}

array_t *order_nodes(array_t *node_vec, int pi_only) {
    int          index;
    lsGen        gen;
    st_generator *stgen;
    node_t       *node, *pi;
    network_t    *network;
    st_table     *leaves;
    array_t      *result;

    if (array_n(node_vec) == 0)
        return NIL(array_t);
    node    = array_fetch(node_t *, node_vec, 0);
    network = node_network(node);
    leaves  = st_init_table(st_ptrcmp, st_ptrhash);
    foreach_primary_input(network, gen, pi) {
        (void) st_insert(leaves, (char *) pi, (char *) -1);
    }
    result = order_dfs(node_vec, leaves, (pi_only == 2) ? 1 : 0);
    if (pi_only) {
        array_free(result);
        result = array_alloc(node_t *, st_count(leaves));
        st_foreach_item_int(leaves, stgen, (char **) &node, &index) {
            if (index != -1) {
                array_insert(node_t *, result, index, node);
            }
        }
    }
    st_free_table(leaves);
    return result;
}

static void
seqbdd_get_one_edge(bdd_t *from, range_data_t *data, bdd_t **state, bdd_t **input, verif_options_t *options) {
    output_info_t *info = options->output_info;
    bdd_t         *minterm;
    st_table      *var_table;
    array_t       *external_input_vars;

    assert(!bdd_is_tautology(from, 0));

    external_input_vars =
            bdd_extract_var_array(data->manager, info->extern_pi, info->pi_ordering);
    /* precompute the vars to be used */
    var_table = st_init_table(st_numcmp, st_numhash);
    bdd_add_varids_to_table(data->input_vars, var_table);
    bdd_add_varids_to_table(external_input_vars, var_table);
    minterm = seqbdd_get_one_minterm(data->manager, from, var_table);
    st_free_table(var_table);

    if (state != NIL(bdd_t *))
        *state = bdd_smooth(minterm, external_input_vars);
    if (input != NIL(bdd_t *))
        *input = bdd_smooth(minterm, data->input_vars);
    bdd_free(minterm);
    array_free(external_input_vars);
}

/*
 * returns a minterm from fn
 * Function should only depend on variables contain in 'vars'.
 * 'vars' is a table of variable_id, not of bdd_t *'s.
 * WARNING: Relies on the fact that the ith bdd_literal in cube
 * corresponds to the variable of variable_id == i.
 */

static bdd_t *seqbdd_get_one_minterm(bdd_manager *manager, bdd_t *fn, st_table *vars) {
    int         i;
    bdd_t       *tmp;
    bdd_t       *bdd_lit;
    bdd_t       *minterm;
    bdd_gen     *gen;
    int         n_lits;
    bdd_literal *lits;
    array_t     *cube;

    assert(!bdd_is_tautology(fn, 0));

    /* need to free the bdd_gen immediately, otherwise problems with the BDD GC
     */
    minterm = bdd_one(manager);
    gen     = bdd_first_cube(fn, &cube);
    n_lits  = array_n(cube);

    lits   = ALLOC(bdd_literal, n_lits);
    for (i = 0; i < n_lits; i++) {
        lits[i] = array_fetch(bdd_literal, cube, i);
    }
    (void) bdd_gen_free(gen);

    for (i = 0; i < n_lits; i++) {
        if (!st_lookup(vars, (char *) i, NIL(char *))) {
            assert(lits[i] == 2);
            continue;
        }
        switch (lits[i]) {
            case 0:
            case 2:tmp  = bdd_get_variable(manager, i);
                bdd_lit = bdd_not(tmp);
                bdd_free(tmp);
                break;
            case 1:bdd_lit = bdd_get_variable(manager, i);
                break;
            default: fail("unexpected literal in get_one_minterm");
                break;
        }
        tmp = bdd_and(minterm, bdd_lit, 1, 1);
        bdd_free(minterm);
        bdd_free(bdd_lit);
        minterm = tmp;
    }
    FREE(lits);
    return minterm;
}

static void bdd_add_varids_to_table(array_t *vars, st_table *table) {
    int     i;
    int     varid;
    array_t *ids;

    ids    = bdd_get_varids(vars);
    for (i = 0; i < array_n(vars); i++) {
        varid = array_fetch(int, ids, i);
        st_insert(table, (char *) varid, NIL(char));
    }
    array_free(ids);
}

/* given an array of BDD's corresponding to external inputs */
/* prints out 0's and 1's in a format directly usable by simulate */
/* input_vars: in simulation order */

/* ARGSUSED */
static void print_error_trace(array_t *input_trace, range_data_t *data, verif_options_t *options) {
    int           i;
    lsGen         gen;
    node_t        *pi;
    bdd_t         *input;
    bdd_t         *var;
    array_t       *input_vars;
    st_table      *pi_to_var_table;
    output_info_t *info = options->output_info;
    char          *realf;
    FILE          *fp   = NULL;

    fp              = stdout;
    (void) fprintf(sisout, "\n");
    if (options->sim_file != NIL(char)) {
        fp = com_open_file(options->sim_file, "w", &realf, 0);
        if (fp != NULL) {
            (void) fprintf(sisout, "Simulation vectors saved in file %s\n", realf);
        } else {
            fp = stdout;
        }
    }
    /* first, compute an array: input_vars: external PI index -> BDD var */
    pi_to_var_table = get_pi_to_var_table(data->manager, info, data);
    input_vars      = array_alloc(bdd_t *, 0);
    (void) fprintf(fp, "Input nodes: ");
    for (i = 0; i < array_n(info->extern_pi); i++) {
        pi = array_fetch(node_t *, info->extern_pi, i);
        assert(st_lookup(pi_to_var_table, (char *) pi, (char **) &var));
        array_insert_last(bdd_t *, input_vars, var);
        (void) fprintf(fp, "%s ", node_name(pi));
    }
    (void) fprintf(fp, "\n");
    st_free_table(pi_to_var_table);

    /* then print minterm by minterm: 2->0 */
    for (i = 0; i < array_n(input_trace); i++) {
        input = array_fetch(bdd_t *, input_trace, i);
        (void) fprintf(fp, "sim ");
        print_error_input(fp, input, input_vars);
        (void) fprintf(fp, "\n");
    }
    array_free(input_vars);
}

/* vars in input_vars are already put in correct I/O order */

static void print_error_input(FILE *fp, bdd_t *input, array_t *input_vars) {
    int   i;
    int   value;
    bdd_t *var;

    for (i = 0; i < array_n(input_vars); i++) {
        var   = array_fetch(bdd_t *, input_vars, i);
        value = (bdd_leq(input, var, 1, 1)) ? 1 : 0;
        (void) fprintf(fp, "%d ", value);
    }
}

/*
 * simple utility: report the map between PRIMARY_INPUTS and bdd_t *var's corre
sponding to them
 */

static st_table *get_pi_to_var_table(bdd_manager *manager, output_info_t *info, range_data_t *data) {
    int          index;
    node_t       *pi;
    bdd_t        *var;
    st_table     *pi_to_var_table;
    st_table     *po_ordering;
    st_generator *gen;

    pi_to_var_table = st_init_table(st_ptrcmp, st_ptrhash);
    st_foreach_item_int(info->pi_ordering, gen, (char **) &pi, &index) {
        var = ntbdd_node_to_bdd(pi, manager, info->pi_ordering);
        st_insert(pi_to_var_table, (char *) pi, (char *) var);
    }
    return pi_to_var_table;
}

static void free_bdd_array(array_t *bdd_array) {
    int   i;
    bdd_t *fn;

    for (i = 0; i < array_n(bdd_array); i++) {
        fn = array_fetch(bdd_t *, bdd_array, i);
        bdd_free(fn);
    }
    array_free(bdd_array);
}

#endif /* SIS */
