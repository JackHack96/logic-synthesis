#include "sis.h"
#include "ntbdd_int.h"


static void bdd_do_build();

static node_t *bdd_do_build_rec();

/* 
 * Builds a multi-output network from an array of BDDs. po_names gives the correspondance
 * between output function names and BDD functions. var_names gives the correspondance 
 * between input variable names and variable ids.
 */
network_t *ntbdd_bdd_array_to_network(fn_array, po_names, var_names)
        array_t *fn_array;
        array_t *po_names;
        array_t *var_names;
{
    network_t *network;
    bdd_t     *fn;
    st_table  *pi_table;
    st_table  *bdd_to_node_table;  /* acts as a visited table */
    int       i;
    char      *po_name;

    /*
     * The number of functions and output names must match.
     */
    assert(array_n(fn_array) == array_n(po_names));

    /*
     * Allocate the necessary structures.
     */
    network           = network_alloc();
    pi_table          = st_init_table(st_ptrcmp, st_ptrhash);
    bdd_to_node_table = st_init_table(st_ptrcmp, st_ptrhash);

    /*
     * For each BDD function, construct the necessary logic.  Note that nodes will be shared.
     */
    for (i = 0; i < array_n(fn_array); i++) {
        fn = array_fetch(bdd_t * , fn_array, i);
        if (fn == NIL(bdd_t)) {
            continue;
        }
        po_name = array_fetch(
        char *, po_names, i);
        bdd_do_build(network, fn, po_name, var_names, pi_table, bdd_to_node_table);
    }

    /*
     * Free the arrays, do a network check, and return.
     */
    st_free_table(pi_table);
    st_free_table(bdd_to_node_table);
    assert(network_check(network));
    return network;
}

/*
 * Convenient interface to ntbdd_bdd_array_to_network.
 */
network_t *ntbdd_bdd_single_to_network(fn, po_name, var_names)
        bdd_t *fn;
        char *po_name;
        array_t *var_names;
{
    network_t *network;
    array_t   *fn_array;
    array_t   *po_names;

    /*
     * Create the necessary one element arrays for call to ntbdd_bdd_array_to_network.
     */
    fn_array = array_alloc(bdd_t * , 1);
    array_insert(bdd_t * , fn_array, 0, fn);
    po_names = array_alloc(
    char *, 1);
    array_insert(
    char *, po_names, 0, po_name);

    /*
     * Create network.
     */
    network = ntbdd_bdd_array_to_network(fn_array, po_names, var_names);

    /*
     * Free the arrays, and return the network.
     */
    array_free(fn_array);
    array_free(po_names);
    return network;
}


/* INTERNAL INTERFACE */

static void bdd_do_build(network, fn, po_name, var_names, pi_table, bdd_to_node_table)
        network_t *network;
        bdd_t *fn;
        char *po_name;
        array_t *var_names;
        st_table *pi_table;
        st_table *bdd_to_node_table;
{
    int     phase;
    node_t  *node, *po;
    boolean status_0, status_1;


    status_0 = bdd_is_tautology(fn, 0);
    status_1 = bdd_is_tautology(fn, 1);

    /*
     * Handle constants specially.
     */
    if (status_0 || status_1) {
        phase = (status_1) ? 1 : 0;
        node  = node_constant(phase);
    } else {
        node = bdd_do_build_rec(network, fn, pi_table, var_names, bdd_to_node_table);
    }
    network_add_node(network, node);
    po = network_add_primary_output(network, node);
    network_change_node_name(network, po, util_strsav(po_name));
}

static node_t *bdd_do_build_rec(network, fn, pi_table, var_names, bdd_to_node_table)
        network_t *network;
        bdd_t *fn;
        st_table *pi_table;
        array_t *var_names;
        st_table *bdd_to_node_table;
{
    int            i, active;
    bdd_variableId top_var_id;
    bdd_node       *node_regular;
    bdd_t          *fn_regular;
    bdd_t          *sub_fn[2];
    int            is_constant[2];
    int            phase[2];
    node_t         *node, *pi, *and_node[2], *input_node[2];
    node_t         *pi_literal[2];
    boolean        status_0, status_1;
    boolean        is_complemented;


    /*
     * The fn should not be zero or one.
     */
    status_0 = bdd_is_tautology(fn, 0);
    status_1 = bdd_is_tautology(fn, 1);
    assert(!status_0 && !status_1);

    /*
     * We want to process the regularized version of fn.
     */
    node_regular = bdd_get_node(fn, &is_complemented);
    if (is_complemented) {
        fn_regular = bdd_not(fn);
    } else {
        fn_regular = bdd_dup(fn);
    }

    /*
     * Check if this bdd_node has already been visited.  If so, return a literal in the proper phase.
     */
    if (st_lookup(bdd_to_node_table, (char *) node_regular, (char **) &node)) {
        if (is_complemented) {
            return node_literal(node, 0);
        } else {
            return node_literal(node, 1);
        }
    }

    /*
     * If top_var is found in the pi_table (i.e. it has already been visited), then
     * simply set pi and continue.  Otherwise, make top_var a primary input of the network,
     * and add it to the pi_table.
     */
    top_var_id = bdd_top_var_id(fn);
    if (!st_lookup(pi_table, (char *) top_var_id, (char **) &pi)) {
        pi = node_alloc();
        pi->name = util_strsav(array_fetch(
        char *, var_names, top_var_id));
        network_add_primary_input(network, pi);
        st_insert(pi_table, (char *) top_var_id, (char *) pi);
    }

    /*
     * Set up the branches for special cases of constants.  Note that bdd_then and bdd_else take into
     * account that fn may be a complemented pointer.  The effect of bdd_then and bdd_else is to
     * recursively push the negated pointers all the way down to the constants.  Thus, the code below
     * never has to complement an internal SIS network node.
     */
    sub_fn[0] = bdd_else(fn_regular);
    sub_fn[1] = bdd_then(fn_regular);
    bdd_free(fn_regular);
    for (i = 0; i < 2; i++) {
        pi_literal[i]  = node_literal(pi, i);
        is_constant[i] = 0;  /* initialize */
        status_0 = bdd_is_tautology(sub_fn[i], 0);
        status_1 = bdd_is_tautology(sub_fn[i], 1);
        if (status_0 || status_1) {
            is_constant[i] = 1;
            phase[i]       = (status_1) ? 1 : 0;
        }
    }

    if (is_constant[0] && is_constant[1]) {
        /*
         * Both then and else functions are constants.  fn is a single variable bdd.
         * Return pi in the proper phase, and free the other pi_literal.  Free the BDDs created.
         */
        for (i = 0; i < 2; i++) {
            bdd_free(sub_fn[i]);
        }
        assert(phase[0] != phase[1]);
        if ((!is_complemented && phase[1]) || (is_complemented && !phase[1])) {  /* logical XOR */
            node_free(pi_literal[0]);
            return pi_literal[1];
        } else {
            node_free(pi_literal[1]);
            return pi_literal[0];
        }
    } else if (is_constant[0] || is_constant[1]) {
        /*
         * One of then and else functions is constant.  active denotes which branch function is not a constant.
         * If active==1, then the then fn is not a constant.  If active==0, then the else fn is not a constant.
         * input_node is the node representing the function of the non-constant function.  and_node is the function
         * of the top variable, with proper phase depending on if active is the then or else function, ANDed with the
         * active function.  node_or is the and_node ORed with the constant function.
         */
        active = is_constant[0] ? 1 : 0;
        input_node[active] = bdd_do_build_rec(network, sub_fn[active], pi_table, var_names, bdd_to_node_table);
        /* phase[1 - active] gives the value of the constant function */
        if (phase[1 - active]) {
            /*
             * The constant function is 1.  pi_literal[1 - active] gives the literal for top variable corresponding
             * to the constant branch.  Implicitly making the simplification of the form: xf+x' ==> f+x'
             */
            node = node_or(input_node[active], pi_literal[1 - active]);
        } else {
            /*
             * The constant function is 0.  AND of 0 and anything is 0, so don't need to OR in anything.
             */
            node = node_and(input_node[active], pi_literal[active]);
        }
        node_free(input_node[active]);
    } else {
        /*
         * Neither branch function is constant.  Create the function for each branch function, and AND it with
         * the proper phase of the top variable.
         */
        for (i = 0; i < 2; i++) {
            input_node[i] = bdd_do_build_rec(network, sub_fn[i], pi_table, var_names, bdd_to_node_table);
            and_node[i]   = node_and(input_node[i], pi_literal[i]);
            node_free(input_node[i]);
        }
        node = node_or(and_node[0], and_node[1]);
        node_free(and_node[0]);
        node_free(and_node[1]);
    }

    /*
     * Free the literal nodes associated with the top variable of the BDD fn, and free the BDDs created.
     */
    for (i = 0; i < 2; i++) {
        node_free(pi_literal[i]);
        bdd_free(sub_fn[i]);
    }

    /*
     * Add to the network the (single) node representing the function positive fn.  Add the positive
     * version to the visited table.  Return the node in the proper phase.
     */
    network_add_node(network, node);
    assert(!st_insert(bdd_to_node_table, (char *) node_regular, (char *) node));
    if (is_complemented) {
        return node_literal(node, 0);
    } else {
        return node_literal(node, 1);
    }
}

