#include "sis.h"
#include "../include/ntbdd_int.h"


static bdd_t *construct_local_bdd();

static bdd_t *sop();

static bdd_t *product();

static bdd_t *get_node_bdd();

/* 
 * Build the logic function at that node in terms of its inputs.
 * The ordering of the inputs should be specified in "leaves".
 */
bdd_t *ntbdd_node_to_local_bdd(node, manager, leaves)
        node_t *node;
        bdd_manager *manager;
        st_table *leaves;
{
    int    i;
    node_t *fanin;

    /*
     * Make sure that each fanin is in the leaves table.
     */
    foreach_fanin(node, i, fanin)
    {
        assert(st_lookup(leaves, (char *) fanin, NIL(
        char *)));
    }

    return construct_local_bdd(node, manager, leaves);
}

/*
 * ntbdd_node_to_bdd - construct a bdd from a node
 *
 * The manager is provided by the caller
 * The BDD variables are the nodes in leaves. 
 * The table "leaves" specifies the ordering.
 * return the bdd ...
 * The BDD is always stored in the node.
 *
 * If a BDD is already there with the same manager,
 * it is considered valid even if it was computed with different leaves.
 * If this happens, then the user is not using managers correctly.
 *
 * If node has a BDD(node) field with same manager, take that.
 * Otherwise, build the bdd at the node from the bdd's of its inputs 
 * the BDD's of the inputs are computed recursively 
 */
bdd_t *ntbdd_node_to_bdd(node, manager, leaves)
        node_t *node;
        bdd_manager *manager;
        st_table *leaves;
{
    bdd_variableId varid;
    bdd_t          *ext_f, *fanin_bdd, *single_var_f;
    node_t         *fanin_node;
    int            nliterals;


    /*
     * Handle the cases where node already has a BDD or where node is in the leaves table.
     */
    ext_f = ntbdd_at_node(node);
    if (ext_f == NIL(bdd_t)) {
        /*
         * node does not have a BDD.  If node is in the leaves table,
         * then get the single variable BDD for node, and return it.
         * If node is not in the leaves table, drop down for further processing.
         */
        if (st_lookup_int(leaves, (char *) node, (int *) &varid)) {
            single_var_f = bdd_get_variable(manager, varid);
            ntbdd_set_at_node(node, single_var_f);
            return single_var_f;
        }

    } else {
        /*
         * The node already has a BDD.
         * If node is in the leaves table, then check that ext_f points to the
         * single varible BDD representing the node variable.  If so, just
         * return ext_f; if not, return a pointer to the single variable BDD.
         */
        if (st_lookup_int(leaves, (char *) node, (int *) &varid)) {
            single_var_f = bdd_get_variable(manager, varid);
            if (bdd_equal(ext_f, single_var_f)) {
                bdd_free(single_var_f);
                return ext_f;
            } else {
                ntbdd_set_at_node(node, single_var_f);  /* automatically frees existing bdd at node */
                return single_var_f;
            }
        } else {
            /*
             * node is not in the leaves table.
             * If the bdd at this node is already of this bdd manager
             * (which presumably means that it has been assigned before)
             * then just take that value as the correct value.  Otherwise,
             * drop down to create a BDD for this node in terms of leaves.
             */
            if (bdd_get_manager(ext_f) == manager) {
                return ext_f;
            }
        }
    }


    /*
     * Treat constants specially.
     */
    nliterals = node_num_fanin(node);
    if (nliterals == 0) {
        switch (node_function(node)) {
            case NODE_0:ext_f = bdd_zero(manager);
                break;
            case NODE_1:ext_f = bdd_one(manager);
                break;
            default:fail("variable not listed in the leaves table");
                break;
        }
        ntbdd_set_at_node(node, ext_f);
        return ext_f;
    }

    /*
     * If there is a fanin and this is a primary output
     * (e.g. this is one of those special primary output nodes)
     * duplicate the bdd at the fanin of this node and return that.
     * Make a new one in any case, in case they
     * want to call demons for any changes ...
     */
    if (nliterals == 1 && node->type == PRIMARY_OUTPUT) {
        fanin_node = node_get_fanin(node, 0);
        fanin_bdd  = ntbdd_node_to_bdd(fanin_node, manager, leaves);
        ext_f      = bdd_dup(fanin_bdd);
        ntbdd_set_at_node(node, ext_f);
        return ext_f;
    }

    /*
     * General case: get a bdd for the sum of products at this node
     */
    ext_f = sop(node, manager, leaves, GLOBAL);

    ntbdd_set_at_node(node, ext_f);
    return ext_f;
}

/*
 * Build the node as a function of its direct inputs.
 * Assumes all direct inputs are in 'leaves'.
 * If node has no fanin, its function is itself.
 * In that case, it should be in the 'leaves' table.
 */
static bdd_t *construct_local_bdd(node, manager, leaves)
        node_t *node;
        bdd_manager *manager;
        st_table *leaves;
{
    bdd_variableId varid;
    bdd_t          *ext_f;
    int            nliterals;
    node_t         *fanin;

    /*
     * Treat constants specially.
     */
    nliterals = node_num_fanin(node);
    if (nliterals == 0) {
        switch (node_function(node)) {
            case NODE_0:ext_f = bdd_zero(manager);
                break;
            case NODE_1:ext_f = bdd_one(manager);
                break;
            default:assert(st_lookup_int(leaves, (char *) node, (int *) &varid));
                ext_f = bdd_get_variable(manager, varid);
                break;
        }
    } else if (nliterals == 1 && node->type == PRIMARY_OUTPUT) {
        /*
         * If there is a fanin and this is a primary output
         * (e.g. this is one of those special primary output nodes)
         * get the variable associated with the input and return the corresponding BDD.
         * Note that we do not return the function of the fanin, but just the
         * variable of the fanin.
         */
        fanin = node_get_fanin(node, 0);
        assert(st_lookup_int(leaves, (char *) fanin, (int *) &varid));
        ext_f = bdd_get_variable(manager, varid);
    } else {
        /*
         * General case: get a bdd for the sum of products at this node
         */
        ext_f = sop(node, manager, leaves, LOCAL);
    }

    return ext_f;
}

/*
 * sop - get the bdd for a sum of products
 *
 * The node is made up of a sum of products.  Foreach product (cube),
 * OR together the cubes in the node to make the final bdd for the node.
 * Start by taking the one node as the base-case.
 */
static bdd_t *
sop(node, manager, leaves, bdd_type)
        node_t *node;
        bdd_manager *manager;
        st_table *leaves;
        ntbdd_type_t bdd_type;
{
    int   ncubes;
    int   row;
    bdd_t *current_bdd, *temp_bdd, *temp2_bdd;

    ncubes = node_num_cube(node);

    /*
     *    current = or(current, tmp)
     */
    current_bdd = bdd_zero(manager);
    for (row    = 0; row < ncubes; row++) {
        temp_bdd  = product(node, manager, row, leaves, bdd_type);
        temp2_bdd = bdd_or(current_bdd, temp_bdd, 1, 1);
        bdd_free(temp_bdd);
        bdd_free(current_bdd);
        current_bdd = temp2_bdd;
    }

    return current_bdd;
}

/*
 * product - take the product of all of the elements in the cube
 * return the bdd of the product
 */
static bdd_t *
product(node, manager, row, leaves, bdd_type)
        node_t *node;
        bdd_manager *manager;
        int row;
        st_table *leaves;
        ntbdd_type_t bdd_type;
{
    int            nliterals;
    int            literal;
    node_cube_t    cube;
    node_t         *fanin;
    bdd_t          *fanin_bdd, *temp_bdd, *temp2_bdd, *current_bdd;
    node_literal_t literal_type;

    nliterals = node_num_fanin(node);
    cube      = node_get_cube(node, row);

    current_bdd  = bdd_one(manager);
    for (literal = 0; literal < nliterals; literal++) {
        literal_type = node_get_literal(cube, literal);
        switch (literal_type) {
            case ZERO:fanin = node_get_fanin(node, literal);
                fanin_bdd   = get_node_bdd(fanin, manager, leaves, bdd_type);
                temp_bdd    = bdd_not(fanin_bdd);  /* take complement */
                if (bdd_type == LOCAL) {
                    /*
                     * For the LOCAL case, fanin_bdd is not attached to the fanin node.  Thus,
                     * we must free it so that we don't leave any bdd_t's dangling.  In the
                     * GLOBAL case, the fanin_bdd is attached to the fanin node, and we leave
                     * it their for future reference.  (Same comment applies to case ONE.)
                     */
                    bdd_free(fanin_bdd);
                }
                break;
            case ONE:fanin = node_get_fanin(node, literal);
                fanin_bdd  = get_node_bdd(fanin, manager, leaves, bdd_type);
                temp_bdd   = bdd_dup(fanin_bdd);  /* just duplicate, so we have same flow as case ZERO */
                if (bdd_type == LOCAL) {
                    bdd_free(fanin_bdd);
                }
                break;
            default:
                /* do nothing for case TWO (i.e. when variable is a don't care) */
                continue;
        }

        /*
         *    current = and(current, tmp)
         */
        temp2_bdd = bdd_and(current_bdd, temp_bdd, 1, 1);
        bdd_free(temp_bdd);
        bdd_free(current_bdd);
        current_bdd = temp2_bdd;
    }

    return current_bdd;
}

/*
 * get_node_bdd - If bdd_type is LOCAL, then just call bdd_get_variable.  
 * If bdd_type is GLOBAL, then call ntbdd_node_to_bdd.  The distinction
 * is that we don't want to store any BDDs created in making a local BDD.
 * On the other hand, in creating a global BDD, we store any BDDs created 
 * at the respective node.
 */
static bdd_t *
get_node_bdd(node, manager, leaves, bdd_type)
        node_t *node;
        bdd_manager *manager;
        st_table *leaves;
        ntbdd_type_t bdd_type;
{
    bdd_t          *bdd;
    bdd_variableId varid;

    if (bdd_type == LOCAL) {
        if (st_lookup_int(leaves, (char *) node, (int *) &varid)) {
            bdd = bdd_get_variable(manager, varid);
        }
    } else if (bdd_type == GLOBAL) {
        bdd = ntbdd_node_to_bdd(node, manager, leaves);
    } else {
        fail("get_node_bdd: unknown bdd_type");
    }

    return bdd;
}
