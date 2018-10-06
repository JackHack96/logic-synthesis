

#ifdef SIS

#include "prl_util.h"
#include "sis.h"

/*
 *----------------------------------------------------------------------
 *
 * Prl_RemoveDcNetwork -- EXPORTED ROUTINE
 *
 * Removes the dc network associated to 'network'.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	'network->dc_network' is nil.
 *
 *----------------------------------------------------------------------
 */

void Prl_RemoveDcNetwork(network_t *network) {
    if (network->dc_network != NIL(network_t)) {
        network_free(network->dc_network);
        network->dc_network = NIL(network_t);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Prl_SetupCopyFields -- EXPORTED ROUTINE
 *
 * 1. Initializes the 'copy' field of all the nodes in 'network_from' to NIL.
 * 2. Initializes the 'copy' field of the PIs in 'network_from' to point to
 *    a node in 'network_from; that has the same name, if there is any.
 *    In case that node is a PO of 'network_from', its fanin is used instead.
 *    In case there are no homonyms, a dummy PI in 'network_to' is created.
 *
 * WARNING:
 *	Do not change the test of 'to_input->type == PRIMARY_INPUT'
 * 	to 'network_is_real_pi(to_network, to_input)' below.
 *      Otherwise 'remove_latches' will break. 'remove_latches'
 *	relies on the fact that 'from_network' has real PIs with
 *	the same name as the latch PIs of 'to_network'. It is used
 *	to replace a latch with an equivalent combinational logic circuit.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The 'copy' fields of nodes in 'from_network' are modified.
 *	Some PIs may have been added to 'to_network'.
 *
 *----------------------------------------------------------------------
 */

void Prl_SetupCopyFields(network_t *to_network, network_t *from_network) {
    lsGen  gen;
    node_t *from_node;
    node_t *from_input, *to_input;

    foreach_node(from_network, gen, from_node) { from_node->copy = NIL(node_t); }

    /* visit the PIs of 'source' */
    foreach_primary_input(from_network, gen, from_input) {

        /* find the homonym in 'target' */
        to_input = network_find_node(to_network, from_input->name);

        /* if names are meaningful and there is a matching node, just use it */
        if (network_is_real_pi(from_network, from_input) &&
            to_input != NIL(node_t)) {
            if (network_is_real_po(to_network, to_input)) {
                from_input->copy = node_get_fanin(to_input, 0);
            } else if (to_input->type == PRIMARY_INPUT) {
                from_input->copy = to_input;
            }
        }

        /* otherwise make one PI up */
        if (from_input->copy == NIL(node_t)) {
            from_input->copy = node_alloc();
            from_input->copy->name =
                    Prl_DisambiguateName(to_network, from_input->name, NIL(node_t));
            network_add_primary_input(to_network, from_input->copy);
        }
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Prl_DisambiguateName -- EXTERNAL ROUTINE
 *
 * From a string 'name', generates a string
 * that is not used as a node name in 'network'
 * by appending some suffix. If anywhere during the search
 * the node 'node' is found as a match, a copy of its name
 * is returned. If 'node' is NIL, a name is generated that is
 * guaranteed not to be used in 'network'.
 *
 * Results:
 *	Either a copy of the name of 'node', or a new name
 *      not used by any node in 'network'.
 *
 * Side effects;
 *	The result is a freshly allocated string.
 *
 *----------------------------------------------------------------------
 */

char *Prl_DisambiguateName(network_t *network, char *name, node_t *node) {
    char   *buffer;
    node_t *matching_node;

    name                  = util_strsav(name);
    while ((matching_node = network_find_node(network, name))) {
        if (matching_node == node)
            return name;
        buffer = ALLOC(char, strlen(name) + 5);
        (void) sprintf(buffer, "%s:dup", name);
        FREE(name);
        name = buffer;
    }
    return name;
}

/*
 *----------------------------------------------------------------------
 *
 * Prl_CopySubNetwork -- EXPORTED ROUTINE
 *
 * 'network' is the network to be augmented.
 * 'node' is a node from some other network whose tfi is to be copied in
 *'network'. The 'copy' fields of the nodes in 'node'->network are used to keep
 *track of what has been already copied. The 'copy' fields of the PIs of
 *'node'->network must have been initialized properly already (i.e.:
 *'CopySubnetwork' should only be called after 'Prl_SetupCopyFields').
 *
 * Results:
 *	A logically equivalent copy of 'node' in 'network'.
 *
 * Side Effects:
 *	'network' is modified; the transitive fanin of 'node'
 *      is copied over into 'network'.
 *	The 'copy' fields of nodes in 'node->network' are modified
 *	to point to their copies in 'network'.
 *
 *----------------------------------------------------------------------
 */

node_t *Prl_CopySubnetwork(network_t *network, node_t *node) {
    int    i;
    char   *name;
    node_t *fanin;

    if (node->copy == NIL(node_t)) {
        assert(node->type != PRIMARY_INPUT);
        node->copy = node_dup(node);
        foreach_fanin(node, i, fanin) {
            fanin->copy = Prl_CopySubnetwork(network, fanin);
            node->copy->fanin[i] = fanin->copy;
        }
        name = node->copy->name;
        if (name != NIL(char) && network_find_node(network, name) != NIL(node_t)) {
            node->copy->name = Prl_DisambiguateName(network, name, NIL(node_t));
            FREE(name);
        }
        network_add_node(network, node->copy);
    }
    return node->copy;
}

/*
 *----------------------------------------------------------------------
 *
 * Prl_StoreAsSingleOutputDcNetwork -- EXPORTED ROUTINE
 *
 * 'dc_network' has a single PO, expressing a global dc condition
 * valid for all POs of 'network'. The PIs of 'dc_network'
 * match by name the PIs of 'network'. This routine modifies
 * 'dc_network' to be an acceptable dc_network of 'network',
 * by inserting in 'dc_network' one PO per PO in 'network'.
 * The old 'network->dc_network', if any, is discarded.
 *
 * WARNING:
 *	'attach_dcnetwork_to_network' matches POs by node->name.
 *	For POs that correspond to latch inputs, that does not always work
 *	properly. Those internal POs may borrow their name from their inputs.
 *	The correct way of doing things would be:
 *	1. to modify 'attach_dcnetwork_to_network' to use 'io_name'
 *	2. to modify 'Prl_StoreAsSingleOutputDcNetwork' to use 'io_name'
 *         (be careful to guarantee unicity of PO names in the 'dc_network').
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	'network->dc_network' is generated.
 *
 * Note:
 *	'dc_network' will be freed when 'network' is.
 *
 *----------------------------------------------------------------------
 */

void Prl_StoreAsSingleOutputDcNetwork(network_t *network, network_t *dc_network) {
    lsGen  gen;
    node_t *node, *po, *oldpo, *fanin;
    node_t *pi;
    char   *name;
    int    i;

    if (dc_network == NIL(network_t))
        return;
    assert(network_num_po(dc_network) == 1);
    Prl_RemoveDcNetwork(network);

    oldpo = network_get_po(dc_network, 0);
    fanin = node_get_fanin(oldpo, 0);

    foreach_primary_output(network, gen, po) {
        node = node_literal(fanin, 1);
        node->name = util_strsav(po->name);
        network_add_node(dc_network, node);
        (void) network_add_primary_output(dc_network, node);
    }
    network_delete_node(dc_network, oldpo);
    network_sweep(dc_network);
    name = ALLOC(char, (int) (strlen(network_name(network)) + strlen(".dc") + 1));
    (void) sprintf(name, "%s%s", network_name(network), ".dc");
    network_set_name(dc_network, name);
    FREE(name);
    network->dc_network = dc_network;
}

/*
 *----------------------------------------------------------------------
 *
 * Prl_FreeBddArray -- EXPORTED ROUTINE
 *
 * Given an 'array_t *' of 'bdd_t *', frees all the bdds in the array
 * and then free the array itself.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	'bdd_array' and all its elements are freed.
 *
 *----------------------------------------------------------------------
 */

void Prl_FreeBddArray(array_t *bdd_array) {
    int   i;
    bdd_t *fn;

    for (i = 0; i < array_n(bdd_array); i++) {
        fn = array_fetch(bdd_t *, bdd_array, i);
        bdd_free(fn);
    }
    array_free(bdd_array);
}

/*
 *----------------------------------------------------------------------
 *
 * Prl_CleanupDcNetwork --
 *
 * Removes any PI that does not fanout anywhere.
 *
 * WARNING:
 * 	Should guarantee compatibility with 'attach_dcnetwork_to_network',
 *	which attach 'network' PO to 'dc_network' PO based on node->name only.
 *
 * Results:
 *	None.
 *
 * Side effects:
 * 	dc_network is modified in place.
 *
 *----------------------------------------------------------------------
 */

void Prl_CleanupDcNetwork(network_t *network) {
    int     i;
    lsGen   gen;
    node_t  *pi;
    array_t *nodes_to_be_removed;

    if (network->dc_network == NIL(network_t))
        return;
    nodes_to_be_removed = array_alloc(node_t *, 0);
    foreach_primary_input(network->dc_network, gen, pi) {
        if (node_num_fanout(pi) == 0) {
            array_insert_last(node_t *, nodes_to_be_removed, pi);
        }
    }
    for (i = 0; i < array_n(nodes_to_be_removed); i++) {
        pi = array_fetch(node_t *, nodes_to_be_removed, i);
        network_delete_node(network->dc_network, pi);
    }
    array_free(nodes_to_be_removed);
    network_sweep(network->dc_network);
}

/*
 *----------------------------------------------------------------------
 *
 * Prl_ReportElapsedTime  --  EXTERNAL ROUTINE
 *
 *----------------------------------------------------------------------
 */

void Prl_ReportElapsedTime(prl_options_t *options, char *comment {
    int    last_time = options->last_time;
    int    new_time  = util_cpu_time();
    double elapsed;
    double total;

    if (options->verbose == 0)
        return;
    (void) fprintf(misout, "use method %s\n", options->method_name);
    options->last_time = new_time;
    options->total_time += (new_time - last_time);
    elapsed = (double) (new_time - last_time) / 1000;
    total   = (double) (options->total_time) / 1000;
    (void) fprintf(misout, "*** [elapsed(%2.1f),total(%2.1f)] ***\n", elapsed,
                   total);
    if (comment != NIL(char))
        (void) fprintf(misout, "%s...\n", comment);
}

static bdd_t *array_bdd_and(bdd_manager *, array_t *);

/*
 *----------------------------------------------------------------------
 *
 * Prl_GetSimpleDc  --  EXTERNAL ROUTINE
 *
 * Results:
 *	Returns the bdd_and of all the don't care bdds.
 *
 *----------------------------------------------------------------------
 */

bdd_t *Prl_GetSimpleDc(seq_info_t *seq_info) {
    bdd_t *tmp1   = array_bdd_and(seq_info->manager, seq_info->next_state_dc);
    bdd_t *tmp2   = array_bdd_and(seq_info->manager, seq_info->external_output_dc);
    bdd_t *result = bdd_and(tmp1, tmp2, 1, 1);
    bdd_free(tmp1);
    bdd_free(tmp2);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * array_bdd_and  --  INTERNAL ROUTINE
 *
 * Results:
 *	Returns the bdd_and of an array of BDDs.
 *
 *----------------------------------------------------------------------
 */

static bdd_t *array_bdd_and(bdd_manager *manager, array_t *array) {
    int   i;
    bdd_t *tmp;
    bdd_t *result = bdd_one(manager);

    for (i = 0; i < array_n(array); i++) {
        tmp = bdd_and(result, array_fetch(bdd_t *, array, i), 1, 1);
        bdd_free(result);
        result = tmp;
    }
    return result;
}

static bdd_t *GetOneMinterm(bdd_manager *, bdd_t *, st_table *);

static void StoreVarIdsInTable(array_t *, st_table *);

/*
 *----------------------------------------------------------------------
 *
 * Prl_GetOneEdge -- EXTERNAL ROUTINE
 *
 * Get one edge contained in the transition condition 'from'.
 * An edge is given as a 'state' and an 'input' bdd_t*.
 * Both are returned as a minterm wrt their own variable set.
 * In case a cube would work, the zero branch is arbitrarily selected.
 *
 * Results:
 *	'state' and 'input' are modified to point towards a valid edge.
 *
 * Side effects:
 *	Assertion failure if 'from' is equal to 0.
 *
 *----------------------------------------------------------------------
 */

void Prl_GetOneEdge(bdd_t *from, seq_info_t *seq_info, bdd_t **state, bdd_t **input) {
    bdd_t    *minterm;
    st_table *var_table;

    assert(!bdd_is_tautology(from, 0));

    /* precompute the vars to be used */
    var_table = st_init_table(st_numcmp, st_numhash);
    StoreVarIdsInTable(seq_info->present_state_vars, var_table);
    StoreVarIdsInTable(seq_info->external_input_vars, var_table);
    minterm = GetOneMinterm(seq_info->manager, from, var_table);
    st_free_table(var_table);

    if (state != NIL(bdd_t *))
        *state = bdd_smooth(minterm, seq_info->external_input_vars);
    if (input != NIL(bdd_t *))
        *input = bdd_smooth(minterm, seq_info->present_state_vars);
}

/*
 *----------------------------------------------------------------------
 *
 * GetOneMinterm -- INTERNAL ROUTINE
 *
 * Returns a minterm from 'fn'.
 * 'fn' should only depend on variables contain in 'vars'.
 * 'vars' is a table of variable_id (small ints), not of bdd_t*'s.
 * WARNING: Relies on the fact that the ith bdd_literal in cube
 * corresponds to the variable of variable_id == i.
 *
 * Results:
 *	a bdd_t*, a minterm from 'fn' wrt to 'vars'.
 *
 * Side effects:
 *	Assertion failure if 'fn' is equal to 0.
 *
 *----------------------------------------------------------------------
 */

static bdd_t *GetOneMinterm(bdd_manager *manager, bdd_t *fn, st_table *vars) {
    int         i;
    bdd_t       *tmp;
    bdd_t       *bdd_lit;
    bdd_t       *minterm;
    bdd_gen     *gen;
    int         n_lits;
    bdd_literal *lits;
    array_t     *cube;

    assert(!bdd_is_tautology(fn, 0));

    /*
     * WARNING: need to free the bdd_gen immediately
     * otherwise problems with the BDD garbage collector.
     */

    minterm = bdd_one(manager);
    gen     = bdd_first_cube(fn, &cube);
    n_lits  = array_n(cube);
    lits    = ALLOC(bdd_literal, n_lits);
    for (i  = 0; i < n_lits; i++) {
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
            default: fail("unexpected literal in GetOneMinterm");
                break;
        }
        tmp = bdd_and(minterm, bdd_lit, 1, 1);
        bdd_free(minterm);
        bdd_free(bdd_lit);
        minterm = tmp;
    }
    return minterm;
}

/*
 *----------------------------------------------------------------------
 *
 * AddVaridsToTable -- INTERNAL ROUTINE
 *
 * Given 'vars', an array of bdd_t*'s representing variables
 * extract from each its variable identifier and store it
 * in the hash table 'table' as key. The associated values
 * are always 0, i.e. 'table' is used as a set.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	'table' is augmented with the varids of the variables in 'vars'.
 *
 *----------------------------------------------------------------------
 */

static void AddVaridsToTable(array_t *vars, st_table *table) {
    int   i;
    bdd_t *var;
    int   varid;

    for (i = 0; i < array_n(vars); i++) {
        var   = array_fetch(bdd_t *, vars, i);
        varid = bdd_top_var_id(var);
        st_insert(table, (char *) varid, NIL(char));
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Prl_GetPiToVarTable -- EXTERNAL ROUTINE
 *
 * a simple utility: computes a map between PIs and bdd_t *vars
 * corresponding to the PIs.
 *
 * Results:
 *	a hash table mapping node_t*'s to bdd_t*'s.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

st_table *Prl_GetPiToVarTable(seq_info_t *seq_info) {
    int      i;
    node_t   *pi;
    bdd_t    *var;
    st_table *pi_to_var_table;

    pi_to_var_table = st_init_table(st_ptrcmp, st_ptrhash);
    for (i          = 0; i < array_n(seq_info->input_nodes); i++) {
        pi  = array_fetch(node_t *, seq_info->input_nodes, i);
        var = array_fetch(bdd_t *, seq_info->input_vars, i);
        st_insert(pi_to_var_table, (char *) pi, (char *) var);
    }
    return pi_to_var_table;
}

/*
 *----------------------------------------------------------------------
 *
 * StoreVarIdsInTable -- INTERNAL ROUTINE
 *
 * a simple utility: extract the varids from the bdd_t*s
 * stored in array 'vars' and store them in 'table'.
 * 'table' is used as a set.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	'table' is augmented with the top varid of the bdds in 'vars'.
 *
 *----------------------------------------------------------------------
 */

static void StoreVarIdsInTable(array_t *vars, st_table *table) {
    int   i;
    bdd_t *var;
    int   varid;

    for (i = 0; i < array_n(vars); i++) {
        var   = array_fetch(bdd_t *, vars, i);
        varid = bdd_top_var_id(var);
        st_insert(table, (char *) varid, NIL(char));
    }
}

#endif /* SIS */
