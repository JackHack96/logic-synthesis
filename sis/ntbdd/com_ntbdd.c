#include <setjmp.h>
#include <signal.h>
#include "sis.h"
#include "ntbdd_int.h"


static int com_bdd_stats();

static int com_bdd_test();

static int com_bdd_create();

static int com_bdd_print();

static int com_bdd_size();

static int com_bdd_implies();

static int com_bdd_cofactor();

static int com_bdd_compose();

static int com_bdd_verify();

static int com_bdd_smooth();

static jmp_buf timeout_env;
static int     timeout_value;
static void timeout_handle(x)
        int x;
{
    longjmp(timeout_env, 1);
}


/*
 *    int
 *    function(network)
 *    network_t **network;	- value/return
 */
static struct {
    char *name;

    int (*function)();

    boolean changes_network;
}              table[] = {
        {"_bdd_stats",    com_bdd_stats,    FALSE},
        {"_bdd_test",     com_bdd_test,     FALSE},
        {"_bdd_create",   com_bdd_create,   FALSE},
        {"_bdd_print",    com_bdd_print,    FALSE},
        {"_bdd_size",     com_bdd_size,     FALSE},
        {"_bdd_implies",  com_bdd_implies,  FALSE},
        {"_bdd_cofactor", com_bdd_cofactor, TRUE},
        {"_bdd_compose",  com_bdd_compose,  FALSE},
        {"_bdd_verify",   com_bdd_verify,   FALSE},
        {"_bdd_smooth",   com_bdd_smooth,   FALSE},
};

/*
 *    init_ntbdd - the ntbdd package initialization
 */
void
init_ntbdd() {
    int i;

#if defined(DEBUG)
    static string version = "v3.0";

    (void) fprintf(miserr, "init_ntbdd for version %s of the bdd package\n", version);
#endif

    for (i = 0; i < (sizeof(table) / sizeof(table[0])); i++) {
        com_add_command(table[i].name, table[i].function, table[i].changes_network);
    }
    node_register_daemon(DAEMON_ALLOC, bdd_alloc_demon);
    node_register_daemon(DAEMON_FREE, bdd_free_demon);

#if defined(NEVER)
    errProgramName("sis");
#endif
}

/*
 *    end_ntbdd - the ntbdd package uninitialization
 */
void
end_ntbdd() {}

/*
 *    All the functions below here follow the sis-command calling convention
 *
 *    int
 *    function(network)
 *    network_t **network;	- value/return
 *
 *    Each was registered above via com_add_command
 */

static int
com_bdd_stats(network)
        network_t **network;    /* value/return */
{
    int         i;
    lsGen       gen;
    node_t      *node;
    bdd_t       *fn;
    bdd_stats   stats;
    bdd_manager *manager;
    st_table    *visited = st_init_table(st_ptrcmp, st_ptrhash);

    (void) fprintf(misout, "bdd's for network %s\n", network_name(*network));
    i = 0;
    foreach_node(*network, gen, node)
    {
        fn = ntbdd_at_node(node);
        if (fn != NIL(bdd_t)) {
            /*
             * If the manager of the BDD has not been visited yet, then print out its
             * stats, and insert it into the visited table.
             */
            manager = bdd_get_manager(fn);
            if (!st_lookup(visited, (char *) manager, NIL(char *))) {
                bdd_get_stats(manager, &stats);
                fprintf(misout, "bdd manager #%d ----------\n", i++);
                bdd_print_stats(stats, misout);
                st_insert(visited, (char *) manager, NIL(
                char));
            }
        }
    }
    st_free_table(visited);
    if (i == 0) {
        (void) fprintf(misout, "no bdd manager associated with the network\n");
    }
    return 0;
}

/*
 * Convert BDD function to a network and print it.
 */
static void
convert_bdd_to_ntwk_and_print(fn, po_name, var_names)
        bdd_t *fn;
        char *po_name;
        array_t *var_names;
{
    network_t *network;

    /*
     * Create network, print it, free it.
     */
    network = ntbdd_bdd_single_to_network(fn, po_name, var_names);
    com_execute(&network, "print");
    network_free(network);
}

/*
 * Exercise the ntbdd and bdd package.
 */
static int
com_bdd_test(network, argc, argv)
        network_t **network;    /* value/return */
        int argc;
        char **argv;
{
    lsGen        gen;
    node_t       *pi;
    node_t       *po;
    node_t       *a_pi, *a_po, *a_node, *node;
    bdd_manager  *mgr;
    bdd_t        *node_fn, *pi_fn, *po_fn;
    st_table     *leaves;
    array_t      *roots;
    array_t      *smoothing_vars;
    bdd_t        *f, *pi_bdd;
    bdd_t        *fns[8];
    int          i, index;
    int          num_pi;
    bdd_t        *one, *zero;
    array_t      *var_names, *var_bdds;
    st_generator *sym_table_gen;
    bdd_gen      *gen_bdd;
    bdd_node     *node_bdd;
    array_t      *lit_cube;
    bdd_literal  literal;
    array_t      *node_list;
    array_t      *po_names;
    array_t      *fn_array;
    network_t    *bdd_to_ntwk;
    int          status;
    int          verbose;
    int          c;
    var_set_t    *po_var_set;
    double       num_onset;

    verbose = 0;
    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "v")) != EOF) {
        switch (c) {
            case 'v':verbose = 1;
                break;
            default:goto usage;
        }
    }
    if (argc != util_optind) goto usage;

    /*
     * Create a manager. The current network must have at least one primary input to run the
     * BDD tests.
     */
    num_pi = network_num_pi(*network);
    if (num_pi == 0) {
        (void) fprintf(miserr, "current network doesn't have any primary inputs\n");
        return 1;
    }
    mgr = ntbdd_start_manager(num_pi);

    /*
     * Test the constants of the manager.
     */
    (void) fprintf(misout, "Constants\n");
    one  = bdd_one(mgr);
    zero = bdd_zero(mgr);
    bdd_print(one);
    bdd_print(zero);
    bdd_free(one);
    bdd_free(zero);

    /*
     * The leaves of the BDD's will be the PI's of the network.
     * The roots for which we want to build BDD's are the PO's of the network.
     */
    leaves = st_init_table(st_ptrcmp, st_ptrhash);
    foreach_primary_input(*network, gen, pi)
    {
        st_insert(leaves, (char *) pi, (char *) -1);
    }
    roots = array_alloc(node_t * , 0);
    foreach_primary_output(*network, gen, po)
    {
        array_insert_last(node_t * , roots, po);
    }

    /*
     * Get an ordering for the leaves.
     */
    node_list = order_dfs(roots, leaves, 0);
    array_free(node_list);  /* node_list not needed */

    /*
     * Create an array of all the names of nodes in leaves for use in
     * creating networks from BDD's. Print out each name with its corresponding
     * index.  Also, create an array of all the single variable BDDs for
     * the nodes in leaves.
     */
    var_names = array_alloc(
    char *, st_count(leaves));
    var_bdds = array_alloc(bdd_t * , st_count(leaves));
    st_foreach_item_int(leaves, sym_table_gen, (char **) &node, &index)
    {
        array_insert(
        char *, var_names, index, node->name);
        pi_bdd = bdd_get_variable(mgr, index);
        array_insert(bdd_t * , var_bdds, index, pi_bdd);
    }
    (void) fprintf(misout, "Nodes in leaves table, with variable index: \n");
    for (i = 0; i < array_n(var_names); i++) {
        (void) fprintf(misout, " index = %d, name = %s\n", i, array_fetch(
        char *, var_names, i));
    }

    /*
     * Check that the composition of the operations: network to BDD, and BDD to network, is the
     * identity operation.
     */
    po_names = array_alloc(
    char *, 0);
    fn_array    = array_alloc(bdd_t * , 0);
    for (i      = 0; i < array_n(roots); i++) {
        po = array_fetch(node_t * , roots, i);
        f  = ntbdd_node_to_bdd(po, mgr, leaves);
        array_insert(bdd_t * , fn_array, i, f);
        array_insert(
        char *, po_names, i, node_long_name(po));
    }
    bdd_to_ntwk = ntbdd_bdd_array_to_network(fn_array, po_names, var_names);
    status      = network_verify(*network, bdd_to_ntwk, 1);
    network_free(bdd_to_ntwk);
    if (status == 0) {
        (void) fprintf(miserr, "%s", error_string());
        (void) fprintf(miserr, "Networks are not equivalent.\n");
    } else {
        (void) fprintf(miserr, "\nComposition of ntwk -> bdd -> ntwk is the identity.\n");
    }

    /*
     * Pick up a few nodes to play with,
     */
    a_po   = network_get_po(*network, 0);
    a_po   = node_get_fanin(a_po, 0);  /* actually, get node which feeds the PO */
    a_pi   = network_get_pi(*network, 0);
    a_node = NIL(node_t);
    foreach_node(*network, gen, node)
    {
        if (node->type == PRIMARY_INPUT) continue;
        if (node->type == PRIMARY_OUTPUT) continue;
        a_node = node;
    }

    if (a_node == NIL(node_t)) {
        a_node = a_pi;
    }
    (void) fprintf(misout, "\nPrimary input: %s\n", node_long_name(a_pi));
    (void) fprintf(misout, "Primary output: %s\n", node_long_name(a_po));
    (void) fprintf(misout, "An internal node: %s\n", node_long_name(a_node));

    /*
     * Test contruction.  First get the BDD for a PO.
     */
    (void) fprintf(misout, "\nFunction and BDD for node: %s\n", node_long_name(a_po));
    f     = ntbdd_node_to_bdd(a_po, mgr, leaves);
    po_fn = bdd_dup(f);
    convert_bdd_to_ntwk_and_print(po_fn, node_long_name(a_po), var_names);
    bdd_print(po_fn);

    /*
     * Traverse the BDD node by node.
     */
    (void) fprintf(misout, "\nforeach_bdd_node of BDD for node: %s\n", node_long_name(a_po));
    foreach_bdd_node(po_fn, gen_bdd, node_bdd)
    {
        /* note that node_bdd has already been REGULARized */
        (void) fprintf(misout, " address = %#x\n", (int) node_bdd);
    }

    /*
     * Print out all the cubes of the BDD, found by traversing all the paths from the
     * root to the constant 1.
     */
    (void) fprintf(misout, "\nforeach_bdd_cube of BDD for node: %s\n", node_long_name(a_po));
    foreach_bdd_cube(po_fn, gen_bdd, lit_cube)
    {
        for (i = 0; i < num_pi; i++) {
            literal = array_fetch(bdd_literal, lit_cube, i);
            (void) fprintf(misout, "%d", literal);
        }
        (void) fprintf(misout, "\n");
    }

    /*
     * Print out the variable support of po_fn.
     */
    po_var_set = bdd_get_support(po_fn);
    (void) fprintf(misout, "Variable support for node %s has cardinality %d\n",
                   node_long_name(a_po), var_set_n_elts(po_var_set));
    var_set_print(misout, po_var_set);
    var_set_free(po_var_set);

    /*
     * Print out the number of minterms in the onset of po_fn.
     */
    num_onset = bdd_count_onset(po_fn, var_bdds);
    (void) fprintf(misout, "Number of minterms in onset of node %s = %f\n", node_long_name(a_po), num_onset);

    /*
     * Build the BDD for a_node (could be PI, INTERNAL, or INTERNAL driving a PO).
     */
    (void) fprintf(misout, "\nFunction and BDD for node: %s\n", node_long_name(a_node));
    f       = ntbdd_node_to_bdd(a_node, mgr, leaves);
    node_fn = bdd_dup(f);
    convert_bdd_to_ntwk_and_print(node_fn, node_long_name(a_node), var_names);
    bdd_print(node_fn);

    /*
     * Create a new BDD variable.
     */
    (void) fprintf(misout, "make a new variable\n");
    f = bdd_create_variable(mgr);
    bdd_print(f);
    bdd_free(f);

    pi_fn = ntbdd_node_to_bdd(a_pi, mgr, leaves); /* make bdd for pi */

    /*
     * Test cofactor, compose, smooth, and_smooth, minimize, is_cube.
     */
    (void) fprintf(misout, "cofactor(%s, %s)\n", node_long_name(a_po), node_long_name(a_pi));
    f = bdd_cofactor(po_fn, pi_fn);
    convert_bdd_to_ntwk_and_print(f, "cofactor", var_names);
    bdd_print(f);
    bdd_free(f);

    (void) fprintf(misout, "compose(%s, %s, %s)\n", node_long_name(a_po), node_long_name(a_pi), node_long_name(a_node));
    f = bdd_compose(po_fn, pi_fn, node_fn);
    convert_bdd_to_ntwk_and_print(f, "compose", var_names);
    bdd_print(f);
    bdd_free(f);

    (void) fprintf(misout, "smooth(%s, %s)\n", node_long_name(a_po), node_long_name(a_pi));
    smoothing_vars = array_alloc(bdd_t * , 0);
    array_insert_last(bdd_t * , smoothing_vars, pi_fn);
    f = bdd_smooth(po_fn, smoothing_vars);
    convert_bdd_to_ntwk_and_print(f, "smooth", var_names);
    bdd_print(f);
    bdd_free(f);
    array_free(smoothing_vars);

    (void) fprintf(misout, "and_smooth(%s, %s)\n", node_long_name(a_po), node_long_name(a_node));
    smoothing_vars = array_alloc(bdd_t * , 0);
    foreach_primary_input(*network, gen, pi)
    {
        f = ntbdd_node_to_bdd(pi, mgr, leaves);
        array_insert_last(bdd_t * , smoothing_vars, f);
    }
    f              = bdd_and_smooth(po_fn, node_fn, smoothing_vars);  /* smoothing out all the variables */
    convert_bdd_to_ntwk_and_print(f, "and_smooth", var_names);
    bdd_print(f);
    bdd_free(f);
    array_free(smoothing_vars);

    (void) fprintf(misout, "minimize(%s, %s)\n", node_long_name(a_po), node_long_name(a_pi));
    f = bdd_minimize_with_params(po_fn, pi_fn, BDD_MIN_OSM, TRUE, FALSE, TRUE);
    convert_bdd_to_ntwk_and_print(f, "minimize", var_names);
    bdd_print(f);
    bdd_free(f);
    f = bdd_between(po_fn, node_fn);
    bdd_free(f);

    /* This line has been commented out because the Long BDD package doesn't
       define bdd_is_cube */

/*  (void) fprintf(misout, "is_cube(%s) = %s\n", node_long_name(a_po), (bdd_is_cube(po_fn)) ? "TRUE" : "FALSE"); */

    /*
     * Test other BDD functions.
     */
    fns[0] = bdd_dup(node_fn);             /* a */
    fns[1] = bdd_not(fns[0]);             /* a' */
    fns[2] = bdd_dup(po_fn);             /* b */
    fns[3] = bdd_not(fns[2]);             /* b' */
    fns[4] = bdd_and(fns[0], fns[2], 1, 1);     /* a b */
    fns[5] = bdd_or(fns[0], fns[2], 0, 0);     /* a' + b' */
    fns[6] = bdd_not(fns[5]);             /* (a' + b')' = a b */
    if (!bdd_equal(fns[6], fns[4])) {
        (void) fprintf(miserr, "ERROR: De Morgan law violated by BDD package\n");
    }
    fns[7] = bdd_or(fns[4], fns[5], 1, 1);         /* a b + a' + b' = 1 */
    if (!bdd_is_tautology(fns[7], 1)) {
        convert_bdd_to_ntwk_and_print(fns[7], "fn_7", var_names);
        (void) fprintf(miserr, "ERROR: fn should have been a tautology\n");
    }
    if (!bdd_leq(fns[4], fns[0], 1, 1)) {
        convert_bdd_to_ntwk_and_print(fns[4], "fn_4", var_names);
        convert_bdd_to_ntwk_and_print(fns[0], "fn_0", var_names);
        (void) fprintf(miserr, "ERROR: first fn should have implied the second one\n");
    }
    if (verbose) {
        for (i = 0; i < 8; i++) {
            (void) fprintf(misout, "function %d:\n", i);
            convert_bdd_to_ntwk_and_print(fns[i], "bdd_out", var_names);
        }
    }

    /*
     * Free the BDD's created.
     */
    for (i = 0; i < 8; i++) {
        bdd_free(fns[i]);
    }
    bdd_free(po_fn);
    bdd_free(node_fn);
    for (i = 0; i < array_n(var_bdds); i++) {
        pi_bdd = array_fetch(bdd_t * , var_bdds, i);
        bdd_free(pi_bdd);
    }

    /*
     * Kill manager.
     */
    ntbdd_end_manager(mgr);

    /*
     * Free everything else.
     */
    array_free(fn_array);
    array_free(po_names);
    array_free(var_names);
    array_free(roots);
    array_free(var_bdds);

    return 0;

    usage:
    (void) fprintf(miserr, "usage: _bdd_test [-v]\n");
    return 1;
}

static int
com_bdd_create(network, argc, argv)
        network_t **network;
        int argc;
        char **argv;
{
    bdd_manager  *manager;
    array_t      *node_vec;
    int          i, j, c, count;
    ntbdd_type_t locality;
    node_t       *node, *fanin, *pi;
    long         time;
    array_t * (*ordering)();
    st_table *leaves;
    lsGen    gen;
    bdd_t    *fn;
    array_t  *node_list;

    ordering      = order_dfs;
    locality      = GLOBAL;
    timeout_value = 0;

    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "lot:")) != EOF) {
        switch (c) {
            case 'l':locality = LOCAL;
                break;
            case 'o':
                if (strcmp("dfs", util_optarg) == 0) {
                    ordering = order_dfs;
                } else if (strcmp("random", util_optarg) == 0) {
                    ordering = order_random;
                } else {
                    (void) fprintf(miserr, "unknown ordering method: %s\n", util_optarg);
                    goto usage;
                }
                break;
            case 't':timeout_value = atoi(util_optarg);
                if (timeout_value < 0 || timeout_value > 3600 * 24 * 365) {
                    goto usage;
                }
                break;
            default:goto usage;
        }
    }

    /* nodes for which to create BDD's */
    node_vec = com_get_nodes(*network, argc - util_optind + 1, argv + util_optind - 1);
    if (array_n(node_vec) == 0) goto usage;

    if (timeout_value > 0) {
        (void) signal(SIGALRM, timeout_handle);
        (void) alarm(timeout_value);
        if (setjmp(timeout_env) > 0) {
            fprintf(misout, "timeout occurred after %d seconds\n", timeout_value);
            return 1;
        }
    }

    (void) fprintf(misout, "ordering nodes\n");
    leaves = st_init_table(st_ptrcmp, st_ptrhash);

    /* if local, pick up some ordering */
    if (locality == LOCAL) {
        count  = 0;
        for (i = 0; i < array_n(node_vec); i++) {
            node = array_fetch(node_t * , node_vec, i);
            if (node->type == PRIMARY_INPUT) {
                /*
                 * If the node is a PI, and it has not been inserted into the table yet, then insert it.
                 */
                if (!st_is_member(leaves, (char *) node)) {
                    if (!st_insert(leaves, (char *) node, (char *) count)) count++;
                }
            } else {
                foreach_fanin(node, j, fanin)
                {  /* st_insert returns 0 if no entry was found */
                    if (!st_is_member(leaves, (char *) fanin)) {
                        if (!st_insert(leaves, (char *) fanin, (char *) count)) count++;
                    }
                }
            }
        }
    } else {
        foreach_primary_input(*network, gen, pi)
        {
            st_insert(leaves, (char *) pi, (char *) -1);
        }
        node_list = (*ordering)(node_vec, leaves, 0);
        if (node_list != NIL(array_t)) {
            array_free(node_list);  /* node_list not needed */
        }
    }

    (void) fprintf(misout, "constructing BDDs\n");
    time    = util_cpu_time();
    manager = ntbdd_start_manager(st_count(leaves));
    if (locality == LOCAL) {
        /*
         * Create the local BDD for each node in node_vec.  Then immediately
         * free the BDD.  This just makes sure we don't bomb anything; the
         * user cannot get to the local BDD he asked to be created.
         */
        for (i = 0; i < array_n(node_vec); i++) {
            node = array_fetch(node_t * , node_vec, i);
            fn   = ntbdd_node_to_local_bdd(node, manager, leaves);
            bdd_free(fn);
        }
    } else {  /* GLOBAL */
        for (i = 0; i < array_n(node_vec); i++) {
            node = array_fetch(node_t * , node_vec, i);
            ntbdd_node_to_bdd(node, manager, leaves);
        }
    }
    (void) fprintf(misout, "CPU time used: %g sec\n", (double) (util_cpu_time() - time) / 1000);

    /* we want global BDDs to persist, so don't kill the manager */

    array_free(node_vec);
    st_free_table(leaves);
    return (0);

    usage:
    (void) fprintf(miserr, "usage: bdd_create [-l] [-o ordering_style] [-t time_limit] node1 node2 ...\n");
    (void) fprintf(miserr, "	-l: build the bdd local to the specified nodes; not stored\n");
    (void) fprintf(miserr, "	ordering_style is one of the following: dfs, random\n");
    return 1;
}

static int
com_bdd_print(network, argc, argv)
        network_t **network;
        int argc;
        char **argv;
{
    array_t *node_vec;
    int     n, i;
    node_t  *node;
    bdd_t   *bdd;

    node_vec = com_get_nodes(*network, argc, argv);

    n = array_n(node_vec);
    if (n == 0) {
        array_free(node_vec);
        (void) fprintf(miserr, "usage: bdd_print n1 n2 ...\n");
        return (1);
    }

    for (i = 0; i < n; i++) {
        node = array_fetch(node_t * , node_vec, i);
        bdd  = ntbdd_at_node(node);
        if ((bdd != NIL(bdd_t)) && (node->type != PRIMARY_OUTPUT)) {
            /* Primary outputs will be effectively caught by the fanins of the primary outputs. */
            (void) fprintf(misout, "node: %s\n", node_name(node));
            bdd_print(bdd);
        }
    }
    array_free(node_vec);
    return (0);
}

static int
com_bdd_size(network, argc, argv)
        network_t **network;
        int argc;
        char **argv;
{
    array_t *node_vec;
    int     i, n;
    node_t  *node;
    bdd_t   *bdd;

    node_vec = com_get_nodes(*network, argc, argv);

    n = array_n(node_vec);
    if (n == 0) {
        array_free(node_vec);
        (void) fprintf(miserr, "usage: bdd_size n1 n2 ...\n");
        return (1);
    }
    for (i = 0; i < n; i++) {
        node = array_fetch(node_t * , node_vec, i);
        bdd  = ntbdd_at_node(node);
        if ((bdd != NIL(bdd_t)) && (node->type != PRIMARY_OUTPUT)) {
            /* Primary outputs will be effectively caught by the fanins of the primary outputs. */
            (void) fprintf(misout, "node: %s, size %d\n", node_name(node), bdd_size(bdd));
        }
    }
    array_free(node_vec);
    return (0);
}

static int
com_bdd_implies(network, argc, argv)
        network_t **network;
        int argc;
        char **argv;
{
    array_t *node_vec;
    node_t  *n1, *n2;
    int     p1, p2;
    bdd_t   *n1_bdd, *n2_bdd;

    if (argc != 5) {
        if (argc == 3) {
            p1 = 1;
            p2 = 1;
        } else {
            usage:
            (void) fprintf(miserr, "usage: bdd_implies n1 n2 [phase1 phase2]\n");
            (void) fprintf(miserr, "\tIf phase1 & phase2 are left out they are assumed both 1\n");
            return (1);
        }
    } else {
        p1 = atoi(argv[3]);
        p2 = atoi(argv[4]);
    }
    if ((p1 != 0 && p1 != 1) || (p2 != 0 && p2 != 1)) {
        goto usage;
    }
    node_vec = com_get_nodes(*network, 3, argv);
    if (array_n(node_vec) != 2) {
        array_free(node_vec);
        goto usage;
    }
    n1 = array_fetch(node_t * , node_vec, 0);
    n2 = array_fetch(node_t * , node_vec, 1);

    array_free(node_vec);
    if (n1 == NIL(node_t) || n2 == NIL(node_t)) {
        goto usage;
    }
    n1_bdd = ntbdd_at_node(n1);
    n2_bdd = ntbdd_at_node(n2);
    (void) fprintf(misout, "%s set to %d %s ", node_name(n1), p1,
                   ((bdd_leq(n1_bdd, n2_bdd, p1, p2) == 1) ? "forces" : "does not force"));
    (void) fprintf(misout, "%s to %d\n", node_name(n2), p2);
    return (0);
}

static int
com_bdd_cofactor(network, argc, argv)
        network_t **network;
        int argc;
        char **argv;
{
    array_t *node_vec;
    node_t  *n1, *n2, *pi;
    bdd_t   *bdd, *n1_bdd, *n2_bdd;
    int     c;
    int     status           = 0;
    int     print_as_network = 1;
    array_t *var_names;
    int     index;
    lsGen   gen;

    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "b")) != EOF) {
        switch (c) {
            case 'b':print_as_network = 0;
                break;
            default:goto usage;
        }
    }
    node_vec  = com_get_nodes(*network, argc - util_optind + 1, argv + util_optind - 1);
    if (argc - util_optind + 1 != 3 || array_n(node_vec) != 2) {
        status = 1;
    } else {
        n1 = array_fetch(node_t * , node_vec, 0);
        n2 = array_fetch(node_t * , node_vec, 1);

        if (n1 == NIL(node_t) || n2 == NIL(node_t)) {
            status = 1;
        } else {
            n1_bdd = ntbdd_at_node(n1);
            n2_bdd = ntbdd_at_node(n2);
            if (!((n1_bdd == NIL(bdd_t)) || (n2_bdd == NIL(bdd_t)))) {
                bdd = bdd_cofactor(n1_bdd, n2_bdd);
                if (print_as_network) {
                    var_names = array_alloc(
                    char *, network_num_pi(*network));
                    foreach_primary_input(*network, gen, pi)
                    {
                        index = bdd_top_var_id(BDD(pi));
                        array_insert(
                        char *, var_names, index, pi->name);
                    }
                    convert_bdd_to_ntwk_and_print(bdd, "cofactor", var_names);
                } else {
                    bdd_print(bdd);
                }
                bdd_free(bdd);
            }
        }
    }
    array_free(node_vec);
    return status;

    usage:
    (void) fprintf(miserr, "\
_bdd_cofactor [-b] node_fn node_factor\n\
               -b: print as BDD\n");
    return 1;
}

static int
com_bdd_smooth(network, argc, argv)
        network_t **network;
        int argc;
        char **argv;
{
    array_t *node_vec;
    node_t  *n1, *node, *pi;
    int     status           = 0;
    array_t *smoothing_vars;
    bdd_t   *bdd, *n1_bdd, *result_bdd;
    int     c, n, i;
    int     print_as_network = 1;
    array_t *var_names;
    int     index;
    lsGen   gen;

    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "b")) != EOF) {
        switch (c) {
            case 'b':print_as_network = 0;
                break;
            default:goto usage;
        }
    }

    node_vec = com_get_nodes(*network, argc - util_optind + 1, argv + util_optind - 1);
    n        = array_n(node_vec);
    if (n < 2) {
        (void) array_free(node_vec);
        goto usage;
    }

    n1 = array_fetch(node_t * , node_vec, 0);
    if (n1 == NIL(node_t)) {
        (void) fprintf(miserr, "%s: nil node\n", argv[0]);
        return (1);
    } else {
        n1_bdd = ntbdd_at_node(n1);
        if (n1_bdd == NIL(bdd_t)) {
            (void) fprintf(miserr, "%s: node does not have a BDD\n", argv[0]);
            return (1);
        }
    }

    smoothing_vars = array_alloc(bdd_t * , 0);
    for (i         = 1; i < n; i++) {
        node = array_fetch(node_t * , node_vec, i);
        if (n1 == NIL(node_t)) {
            (void) fprintf(miserr, "%s: nil node\n", argv[0]);
            return (1);
        }
        bdd = ntbdd_at_node(node);
        if (bdd == NIL(bdd_t)) {
            (void) fprintf(miserr, "%s: node does not have a BDD\n", argv[0]);
            return (1);
        }
        array_insert_last(bdd_t * , smoothing_vars, bdd);
    }
    result_bdd     = bdd_smooth(n1_bdd, smoothing_vars);


    if (print_as_network) {
        var_names = array_alloc(
        char *, network_num_pi(*network));
        foreach_primary_input(*network, gen, pi)
        {
            index = bdd_top_var_id(BDD(pi));
            array_insert(
            char *, var_names, index, pi->name);
        }
        convert_bdd_to_ntwk_and_print(result_bdd, "smooth", var_names);
    } else {
        bdd_print(result_bdd);
    }

    bdd_free(result_bdd);
    array_free(smoothing_vars);
    array_free(node_vec);
    return (status);

    usage:
    (void) fprintf(miserr, "\
_bdd_smooth [-b] node_fn node_var1 node_var2 ...\n\
               -b: print as BDD\n");
    return 1;

}

static int
com_bdd_compose(network, argc, argv)
        network_t **network;
        int argc;
        char **argv;
{
    array_t *node_vec;
    node_t  *n1, *n2, *n3;
    bdd_t   *bdd;
    int     status;

    node_vec = com_get_nodes(*network, argc, argv);
    if (argc != 4 || array_n(node_vec) != 3) {
        status = 1;
    } else {
        n1 = array_fetch(node_t * , node_vec, 0);
        n2 = array_fetch(node_t * , node_vec, 1);
        n3 = array_fetch(node_t * , node_vec, 2);

        if (n1 == NIL(node_t) || n2 == NIL(node_t) || n3 == NIL(node_t)) {
            status = 1;
        } else {
            bdd = bdd_compose(ntbdd_at_node(n1), ntbdd_at_node(n2), ntbdd_at_node(n3));
            bdd_print(bdd);
            bdd_free(bdd);
            status = 0;
        }
    }
    array_free(node_vec);
    return (status);
}

static /* boolean */ int readit();

static int
com_bdd_verify(network, argc, argv)
        network_t **network;
        int argc;
        char **argv;
{
    FILE      *f1, *f2;
    network_t *net1, *net2;
    int       status;
    int       i;

    if (argc != 3) {
        usage:
        (void) fprintf(miserr, "usage: %s blif_file1 blif_file2\n", argv[0]);
        return (1);
    }
    f1 = com_open_file(argv[1], "r", NIL(
    char *), 0);
    if (f1 == NIL(FILE)) {
        goto usage;
    }
    f2 = com_open_file(argv[2], "r", NIL(
    char *), 0);
    if (f2 == NIL(FILE)) {
        (void) fclose(f1);
        goto usage;
    }

    net1 = net2 = NIL(network_t);
    error_init();

    if (readit(f1, argv[1], &net1) != 1 || readit(f2, argv[2], &net2) != 1) {
        (void) fprintf(miserr, "%s: error reading network\n", argv[0]);
        if (error_string()[0] != '\0') {
            (void) fprintf(miserr, "%s", error_string());
        }
        status = 1;
    } else {
        i = ntbdd_verify_network(net1, net2, DFS_ORDER, ALL_TOGETHER);
        (void) fprintf(misout, "verification %s\n", (i == 0) ? "fails" : "succeeds");
        status = 0;
    }
    network_free(net1);
    network_free(net2);
    (void) fclose(f1);
    (void) fclose(f2);
    return (status);
}

static /* boolean */ int
readit(file, filename, network)
        FILE *file;
        char *filename;
        network_t **network;    /* return */
{
    error_init();
    read_register_filename(filename);
    return (read_blif(file, network));
}

