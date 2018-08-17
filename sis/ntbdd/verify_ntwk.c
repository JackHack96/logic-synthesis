#include "sis.h"
#include "../include/ntbdd_int.h"


static int same_orderp();

static void report_error();

static int verify_one_at_a_time();

static int verify_all_together();

/*
 *    ntbdd_verify_network - verify that the two networks are the same
 *
 *    Convert both networks into a bdd and then see if the
 *    bdd's for both networks are equivalent; bdd's are a
 *    canonical form, so this comparison is easy.
 *
 *    return {TRUE, FALSE} on {is, is not}.
 */
int ntbdd_verify_network(net1, net2, order_method, verify_method)
        network_t *net1;
        network_t *net2;
        order_method_t order_method;
        ntbdd_verify_method_t verify_method;
{
    int      i;
    lsGen    gen;
    int      state;
    node_t   *po, *ipo, *pi;
    st_table *leaves1, *leaves2;
    array_t  *po_list1, *po_list2, *pi_list1, *pi_list2;
    char     errmsg[128];

    /*
     *    If the two networks are exactly the same
     *    then return TRUE directly.
     */
    if (net1 == net2) return (TRUE);

    /*
     *    Clearly, if one is nil and the other one is
     *    not, then they are not the same.
     */
    if (net1 == NIL(network_t) || net2 == NIL(network_t)) return (FALSE);

    /*
     *    Create an array for both primary-output
     *    sets and fill those output sets.
     */
    po_list1 = array_alloc(node_t * , 0);
    foreach_primary_output(net1, gen, po)
    {
        array_insert_last(node_t * , po_list1, po);
    }

    po_list2 = array_alloc(node_t * , 0);
    foreach_primary_output(net2, gen, po)
    {
        array_insert_last(node_t * , po_list2, po);
    }

    /*
     * If they don't have the same output sets in the first place, don't even bother.
     */
    if (!same_orderp(po_list1, &po_list2)) {
        (void) sprintf(errmsg, "The name/number of outputs of the two networks don't match.\n");
        error_append(errmsg);
        array_free(po_list1);
        array_free(po_list2);
        return (FALSE);
    }

    /*
     * Take the nodes feeding the PRIMARY_OUTPUTS.
     */
    for (i = 0; i < array_n(po_list1); i++) {
        po  = array_fetch(node_t * , po_list1, i);
        ipo = node_get_fanin(po, 0);
        array_insert(node_t * , po_list1, i, ipo);

        po  = array_fetch(node_t * , po_list2, i);
        ipo = node_get_fanin(po, 0);
        array_insert(node_t * , po_list2, i, ipo);
    }

    pi_list1 = array_alloc(node_t * , 0);
    leaves1  = st_init_table(st_ptrcmp, st_ptrhash);
    foreach_primary_input(net1, gen, pi)
    {
        st_insert(leaves1, (char *) pi, (char *) -1);
        array_insert_last(node_t * , pi_list1, pi);
    }
    pi_list2 = array_alloc(node_t * , 0);
    leaves2  = st_init_table(st_ptrcmp, st_ptrhash);
    foreach_primary_input(net2, gen, pi)
    {
        st_insert(leaves2, (char *) pi, (char *) -1);
        array_insert_last(node_t * , pi_list2, pi);
    }

    if (verify_method == ONE_AT_A_TIME) {
        state = verify_one_at_a_time(po_list1, pi_list1, leaves1, po_list2, pi_list2, leaves2, order_method);
    } else if (verify_method == ALL_TOGETHER) {
        state = verify_all_together(po_list1, pi_list1, leaves1, po_list2, pi_list2, leaves2, order_method);
    } else {
        fail("ntbdd_verify_network: unknown verify method");
    }

    array_free(pi_list1);
    array_free(pi_list2);
    array_free(po_list1);
    array_free(po_list2);
    st_free_table(leaves1);
    st_free_table(leaves2);

    return (state);
}

/* 
 * Does the verification one output at a time, using different orderings each time.
 */
static int verify_one_at_a_time(po_list1, pi_list1, leaves1, po_list2, pi_list2, leaves2, order_method)
        array_t *po_list1;
        array_t *pi_list1;
        st_table *leaves1;
        array_t *po_list2;
        array_t *pi_list2;
        st_table *leaves2;
        order_method_t order_method;
{
    int     i;
    int     state;
    node_t  *po1, *po2;
    array_t *single_po1, *single_po2;

    for (i = 0; i < array_n(po_list1); i++) {
        single_po1 = array_alloc(node_t * , 0);
        single_po2 = array_alloc(node_t * , 0);
        po1        = array_fetch(node_t * , po_list1, i);
        po2        = array_fetch(node_t * , po_list2, i);
        array_insert_last(node_t * , single_po1, po1);
        array_insert_last(node_t * , single_po2, po2);
        /*
         * Note that verify_all_together creates and frees a BDD manager on each call.
         */
        state = verify_all_together(single_po1, pi_list1, leaves1, single_po2, pi_list2, leaves2, order_method);
        if (state == FALSE) {
            array_free(single_po1);
            array_free(single_po2);
            return FALSE;
        }
        array_free(single_po1);
        array_free(single_po2);
    }
    return TRUE;
}

/*
 * A node from pi_list1, and a node with the same name in pi_list2, must hash to the same variable ID
 * in leaves1 and leaves2, respectively.  This is so that BDD's f1 and f2 are built using exactly
 * the same BDD variables.
 *
 * Note: Only the first difference in outputs is found and reported.  TODO: report all differences?
 */
static int verify_all_together(po_list1, pi_list1, leaves1, po_list2, pi_list2, leaves2, order_method)
        array_t *po_list1;
        array_t *pi_list1;
        st_table *leaves1;
        array_t *po_list2;
        array_t *pi_list2;
        st_table *leaves2;
        order_method_t order_method;
{
    int         i, j;
    int         state;
    int         index;
    bdd_t       *f1, *f2;
    node_t      *n1, *n2;
    node_t      *pi1, *pi2;
    array_t     *node_list1;
    bdd_manager *manager;
    int         max_index;
    int         match_found;

    /*
     * Can't order both networks, because if orders differ, we won't be able
     * to do anything with the BDDs. Order the first and derive the order for the second.
     */
    if (order_method == DFS_ORDER) {
        node_list1 = order_dfs(po_list1, leaves1, 0);
        array_free(node_list1);  /* node_list1 not needed */
    } else if (order_method == RANDOM_ORDER) {
        node_list1 = order_random(po_list1, leaves1, 0);
        /* order_random always returns NIL(array_t); thus, no need to array_free */
    } else {
        fail("verify_all_together: unknown order method");
    }

    /*
     * We want to elegantly handle the case where a PI does not fanout.
     * Thus, first find the max index assigned to an input.  Only those
     * inputs which can be reached from po_list1 will be assigned an index
     * greater than -1.  Note that BDD variable IDs start at 0.
     */
    max_index = 0;
    for (i    = 0; i < array_n(pi_list1); i++) {
        pi1 = array_fetch(node_t * , pi_list1, i);
        assert(st_lookup_int(leaves1, (char *) pi1, &index));
        if (index > max_index) {
            max_index = index;
        }
    }
    max_index++;

    for (i = 0; i < array_n(pi_list1); i++) {
        pi1 = array_fetch(node_t * , pi_list1, i);
        assert(st_lookup_int(leaves1, (char *) pi1, &index));

        /*
         * Assign an index to those inputs which could not be reached
         * from the POs.  After assignment, increment max_index.
         */
        if (index < 0) {
            index = max_index++;
            st_insert(leaves1, (char *) pi1, (char *) index);
        }
    }

    /*
     * Based on the variable ordering for nodes in leaves1, assign a variable to the nodes
     * in leaves2.  Any node in pi_list2 which matches by name a node in pi_list1, is assigned
     * the same ordering value.  This is so that the BDD package treats the two nodes as if they
     * were the same. If a match is not found for a node in pi_list2, then assign the node
     * the next index value.
     * Note that even if the supports of network1 and network2 are disjoint, the functions
     * may still be equal (e.g. if they are the same constant function).
     */
    for (i = 0; i < array_n(pi_list2); i++) {
        pi2         = array_fetch(node_t * , pi_list2, i);
        match_found = FALSE;
        for (j      = 0; j < array_n(pi_list1); j++) {
            pi1 = array_fetch(node_t * , pi_list1, j);
            if (strcmp(node_long_name(pi2), node_long_name(pi1)) == 0) {
                /*
                 * A name match is found.  Assign pi2 the same index as pi1.
                 */
                match_found = TRUE;
                assert(st_lookup_int(leaves1, (char *) pi1, &index));
                st_insert(leaves2, (char *) pi2, (char *) index);
                break;
            }
        }

        /*
         * If no match was found, assign pi2 the next index value.  If a match was found, then
         * reset the match_found flag.
         * TODO: if ability to insert variables into order becomes available, then try to
         * interleave variables in network2 not in network1 into variable ordering.  Any change
         * to the variable ordering here should be reflected in the function report_error.
         */
        if (match_found == FALSE) {
            index = max_index++;
            st_insert(leaves2, (char *) pi2, (char *) index);
        } else {
            match_found = FALSE;
        }
    }

    /*
     * Both networks will be built in the same manager.  The important thing to remember is
     * that if 2 leaves have the same name, then they are considered equivalent in the manager,
     * because we assigned them the same variable ID.
     */
    manager = ntbdd_start_manager(max_index);
    state   = TRUE;
    for (i  = 0; i < array_n(po_list1); i++) {
        n1 = array_fetch(node_t * , po_list1, i);
        n2 = array_fetch(node_t * , po_list2, i);
        f1 = ntbdd_node_to_bdd(n1, manager, leaves1);
        f2 = ntbdd_node_to_bdd(n2, manager, leaves2);
        if (!bdd_equal(f1, f2)) {
            report_error(pi_list1, leaves1, pi_list2, leaves2, f1, f2, n1);
            state = FALSE;
        }
        ntbdd_free_at_node(n1); /* frees f1 & f2 */
        ntbdd_free_at_node(n2);
        if (state == FALSE) break;
    }
    ntbdd_end_manager(manager);
    return state;
}


/*
 * Orders the nodes in list2 so that the corresponding nodes in both lists
 * have the same names.
 */
static int
same_orderp(list1, list2p)
        array_t *list1;
        array_t **list2p;    /* return */
{
    int      i;
    int      index;
    char     *name;
    node_t   *node;
    array_t  *new_list;
    st_table *name_table;
    array_t  *list2 = *list2p;

    /*
     * Both arrays must have the same number of elements as a necessary condition
     * that they are the same.
     */
    if (array_n(list1) != array_n(list2)) return FALSE;

    /*
     * Hash each node name of list1 into its array position in list1.
     */
    name_table = st_init_table(strcmp, st_strhash);
    for (i     = 0; i < array_n(list1); i++) {
        node = array_fetch(node_t * , list1, i);
        st_insert(name_table, node_long_name(node), (char *) i);
    }

    /*
     * If node name from list2 is found in name table, then insert node into
     * new_list in the same position that it exists in list1.  Assumes that
     * 2 nodes in list2 don't have the same name.
     */
    new_list = array_alloc(node_t * , 0);
    for (i   = 0; i < array_n(list2); i++) {
        node = array_fetch(node_t * , list2, i);
        name = node_long_name(node);
        if (st_lookup_int(name_table, name, &index)) {
            array_insert(node_t * , new_list, index, node);
        } else {
            st_free_table(name_table);
            array_free(new_list);
            return FALSE;
        }
    }
    st_free_table(name_table);
    array_free(list2);
    *list2p = new_list;
    return (TRUE);
}

static void report_error(pi_list1, leaves1, pi_list2, leaves2, f1, f2, bad_output)
        array_t *pi_list1;
        st_table *leaves1;
        array_t *pi_list2;
        st_table *leaves2;
        bdd_t *f1;
        bdd_t *f2;
        node_t *bad_output;
{
    int     i;
    int     index;
    int     value;
    node_t  *pi;
    bdd_gen *gen;
    array_t *cube;
    char    errmsg[1024];
    int     max_index = -1;

    /*
     * Find the differences.  If there are no differences, then there is a program error.
     */
    bdd_t *diff = bdd_xor(f1, f2);
    assert(!bdd_is_tautology(diff, 0));

    gen = bdd_first_cube(diff, &cube);

    (void) sprintf(errmsg, "Networks differ on (at least) primary output %s\n", node_name(bad_output));
    error_append(errmsg);
    (void) sprintf(errmsg, "Incorrect input is:\n");
    error_append(errmsg);

    /*
     * First extract the variable value from "cube" for each variables in pi_list1.
     */
    for (i = 0; i < array_n(pi_list1); i++) {
        pi    = array_fetch(node_t * , pi_list1, i);
        assert(st_lookup_int(leaves1, (char *) pi, &index));
        if (index > max_index) {
            max_index = index;
        }
        value = array_fetch(
        int, cube, index);
        if (value == 2) {
            value = 0;
        }
        (void) sprintf(errmsg, "%d %s\n", value, node_name(pi));
        error_append(errmsg);
    }

    /*
     * For any node name in pi_list2 that is not in pi_list1, extract the variable value from "cube" for
     * that node.  Since all such nodes have a higher index value than any nodes in pi_list1, we can easily
     * detect such nodes: their index value is higher than the max index value in pi_list1.
     */
    for (i = 0; i < array_n(pi_list2); i++) {
        pi = array_fetch(node_t * , pi_list2, i);
        assert(st_lookup_int(leaves2, (char *) pi, &index));
        /*
         * If index is less than or equal to max_index, then this variable was already processed in pi_list1
         * above.
         */
        if (index <= max_index) {
            continue;
        }
        value = array_fetch(
        int, cube, index);
        if (value == 2) {
            value = 0;
        }
        (void) sprintf(errmsg, "%d %s\n", value, node_name(pi));
        error_append(errmsg);
    }

    /*
     * Do not free the generator before the last use of "cube"
     * because "cube" points somewhere in the generator.
     */
    bdd_gen_free(gen);
    (void) sprintf(errmsg, "\n");
    error_append(errmsg);
}

