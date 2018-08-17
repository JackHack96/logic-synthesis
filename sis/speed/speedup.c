
#include "sis.h"
#include "../include/speed_int.h"
#include <math.h>

static void speed_replace();

static int speed_resub_alge_node();

static int speed_resub_alge_network();

int sp_compare_cutset_nodes();        /* For order of collapsing */

bool
speed_critical(np, speed_param)
        node_t *np;
        speed_global_t *speed_param;
{
    delay_time_t time;

    time = delay_slack_time(np);
    if (time.rise < (speed_param->crit_slack - NSP_EPSILON) ||
        time.fall < (speed_param->crit_slack - NSP_EPSILON)) {
        return TRUE;
    } else {
        return FALSE;
    }
}


int
speed_is_fanout_po(node)
        node_t *node;
{
    lsGen  gen;
    node_t *fo;

    foreach_fanout(node, gen, fo)
    {
        if (node_function(fo) == NODE_PO) {
            (void) lsFinish(gen);
            return TRUE;
        }
    }
    return FALSE;
}


void
speed_up_network(network, speed_param)
        network_t *network;
        speed_global_t *speed_param;
{
    int      i;
    st_table *table;
    lsGen    gen, gen1;
    array_t  *mincut, *revised_order;
    node_t   *np, *fo, *fan, *fanin, *fanin_in;


    /* Build up the table of weights */
    set_speed_thresh(network, speed_param);
    table = speed_compute_weight(network, speed_param, NIL(array_t));

    /* compute the minimum weighted cutset of the critical paths */
    mincut = cutset(network, table);

    /* Make sure that nodes get collapsed in the order --- outputs to inputs */
    revised_order = sp_generate_revised_order(mincut, speed_param, NIL(st_table));
    array_free(mincut);
    mincut = revised_order;

    /*
     * partial_collapse & re-decompose each collapsed node. 
     */
    for (i = 0; i < array_n(mincut); i++) {
        np = array_fetch(node_t * , mincut, i);
        speed_absorb(np, speed_param);
    }
    for (i = 0; i < array_n(mincut); i++) {
        np = array_fetch(node_t * , mincut, i);

        if (np->type == INTERNAL) {
            if (speed_param->debug) {
                (void) fprintf(sisout, "\nSPEED DECOMPOSING NODE \t ");
                node_print(sisout, np);
            }
            speed_up_node(network, np, speed_param, 0);
        }
    }

    /* 
     * Due to the collapsing of the inverters, some
     * primary o/p may become buffers. Remove these.
     */
    foreach_primary_output(network, gen, fo)
    {
        fanin = node_get_fanin(fo, 0);
        if (node_function(fanin) == NODE_BUF) {
            fanin_in = node_get_fanin(fanin, 0);
            assert(node_patch_fanin(fo, fanin, fanin_in));
            foreach_fanout(fanin, gen1, fan)
            {
                (void) node_collapse(fan, fanin);
            }
            if (node_num_fanout(fanin) == 0) speed_network_delete_node(network, fanin);
        } else if (node_function(fanin) == NODE_INV) {
            foreach_fanout(fanin, gen1, fan)
            {
                (void) node_collapse(fan, fanin);
            }
            if (node_num_fanout(fanin) == 0) speed_network_delete_node(network, fanin);
        }
    }

    /*
     *Just a check:  If the routines are good this is not needed
     */
    /*
    if (network_ccleanup(network) ) {
	(void) fprintf(sisout, " Hey ! cleanup was done \n");
    }
    */

    array_free(mincut);
    st_free_table(table);
}


/*
 * Exported interface to the routine speed_up_node () that decomposes
 * a node in place in the network (assuming a valid trace)...
 */
void
speed_node_interface(network, node, coeff, model)
        network_t *network;
        node_t *node;
        double coeff;
        delay_model_t model;
{
    speed_global_t speed_param;

    (void) speed_fill_options(&speed_param, 0, NIL(
    char *));
    speed_param.coeff = coeff;
    speed_param.model = model;

    speed_set_delay_data(&speed_param, 0 /* No library acceleration */);
    speed_up_node(network, node, &speed_param, 0 /* Use fanin arrival time */);

    return;

}


void
speed_up_node(network, np, speed_param, delay_flag)
        network_t *network;
        node_t *np;
        speed_global_t *speed_param;
        int delay_flag;        /* 1=> use user field */
{
    array_t      *nodes;
    delay_time_t t;

    /*
     * Check if the node is fit to be decomposed.
     */
    if (np->type != INTERNAL || node_num_literal(np) == 0) {
        speed_single_level_update(np, speed_param, &t);
        return;
    }

    if (node_num_fanin(np) <= 2 && node_num_cube(np) <= 1) {
        speed_single_level_update(np, speed_param, &t);
        return;
    }

    nodes = speed_decomp(np, speed_param, delay_flag);

    if (nodes != NIL(array_t)) {
        speed_replace(network, np, nodes, speed_param);
    } else {
        fail("Speed_decomp returned a null array");
        exit(-1);
    }

    array_free(nodes);
}

int
speed_init_decomp(network, speed_param)
        network_t *network;
        speed_global_t *speed_param;
{
    node_t  *node, *temp;
    array_t *nodevec;
    int     i, n_attempts, del_crit_flag, debug_flag;

    /*
     * Check if the initial 2-input decomposition is required: In the case
     * when the mapped optimization is being used this is not rerquired
     */
    if (speed_param->new_mode && speed_param->model == DELAY_MODEL_MAPPED) {
        if (speed_param->interactive) {
            (void) fprintf(sisout, "INFO: For mapped network, 2-input decomp BYPASSED\n");
        }
        return 0;
    }
    if (speed_param->add_inv) {
        (void) add_inv_network(network);
    } else {
        (void) network_csweep(network);
    }

    assert(speed_delay_trace(network, speed_param));
    nodevec = network_dfs(network);

    /* Use only a single attempt at decomposing the node */
    n_attempts = speed_param->num_tries;
    speed_param->num_tries = 1;

    debug_flag = speed_param->debug;
    speed_param->debug = FALSE;
    del_crit_flag = speed_param->del_crit_cubes;
    speed_param->del_crit_cubes = TRUE;

    for (i = 0; i < array_n(nodevec); i++) {
        node = array_fetch(node_t * , nodevec, i);
        if (node->type == INTERNAL) {
            /*
             * Just for the case when redundant boolean networks are passed.
             * Make sure that there are no obvious obvious redundancies like
             * f = (a+a')+... -- Has happened in industrial examples !!!
             */
            temp = node_simplify(node, NIL(node_t), NODE_SIM_SIMPCOMP);
            node_replace(node, temp);
            speed_up_node(network, node, speed_param, 1);
        }
    }
    speed_param->debug          = debug_flag;
    speed_param->num_tries      = n_attempts;
    speed_param->del_crit_cubes = del_crit_flag;

    /* 
     * During the decomposition into 2-input NAND gates there is a
     * lot of possibility that nodes computing the same logic function
     * exist. Get rid of these by an algebraic resubstitution.
     */

    if (speed_param->area_reclaim)
        speed_resub_alge_network(network);

    if (speed_param->add_inv) {
        (void) add_inv_network(network);
    } else {
        (void) network_csweep(network);
    }

    array_free(nodevec);
    return 0;
}

static void
speed_replace(network, np, nodes, speed_param)
        network_t *network;
        node_t *np;
        array_t *nodes;
        speed_global_t *speed_param;
{
    int          i;
    char         *dummy;
    delay_time_t t;
    st_table     *table;
    array_t      *inv_array, *added_array, *added_tables;
    node_t       *temp, *root_node;

    if (array_n(nodes) > 3) {
        inv_array    = array_alloc(node_t * , 0);
        added_array  = array_alloc(node_t * , 0);
        added_tables = array_alloc(st_table * , 0);
        table        = st_init_table(st_ptrcmp, st_ptrhash);

        /*
         * Now add the nodes in the array. If the inverters
         * have to be removed, record their occurance and
         * collapse them later.
         */
        node_free(array_fetch(node_t * , nodes, 0));
        root_node = array_fetch(node_t * , nodes, 1);
        speed_delay_arrival_time(root_node, speed_param, &t);

        for (i = array_n(nodes) - 1; i > 1; i--) {
            temp = array_fetch(node_t * , nodes, i);
            (void) st_insert(table, (char *) temp, "");
        }
        for (i = array_n(nodes) - 1; i > 1; i--) {
            temp = array_fetch(node_t * , nodes, i);
            if (node_function(temp) != NODE_PI) {

                network_add_node(network, temp);
                (void) st_delete(table, (char **) &temp, &dummy);
                if (node_num_fanin(temp) < 2) {
                    array_insert_last(node_t * , inv_array, temp);
                } else if (speed_param->area_reclaim) {
                    /*
                     * Check if this node is already present -
                     * i.e try an algebraic resubstitution.
                     */
                    array_insert_last(node_t * , added_array, temp);
                    array_insert_last(st_table * , added_tables, st_copy(table));
                }
            } else {
                node_free(temp);
            }
        }

        node_replace(np, root_node);
        speed_set_arrival_time(np, t);
        st_free_table(table);

        for (i = 0; i < array_n(added_array); i++) {
            table = array_fetch(st_table * , added_tables, i);
            temp  = array_fetch(node_t * , added_array, i);
            if (speed_param->debug) {
                (void) fprintf(sisout, "Trying resub for");
                node_print(sisout, temp);
            }
            if (speed_resub_alge_node(network, temp, table)) {
                network_delete_node(network, temp);
            } else if (speed_param->debug) {
                (void) fprintf(sisout, "After resub");
                node_print(sisout, temp);
            }
            st_free_table(table);
        }
        array_free(added_array);
        array_free(added_tables);

        /*
         * Conditional removal of the inverters.
         */
        if (!speed_param->add_inv) {
            for (i = 0; i < array_n(inv_array); i++) {
                temp = array_fetch(node_t * , inv_array, i);
                speed_delete_single_fanin_node(network, temp);
            }
        }
        array_free(inv_array);

        if ((node_num_fanin(np) == 1) &&
            (speed_is_fanout_po(np) == FALSE)) {
            speed_delete_single_fanin_node(network, np);
        }
    } else {
        /*
         * If the node could be replaced by something this trivial
         * then one might as well simplify the node itself.
         * We still need to update the delay times
         */
        root_node = node_simplify(np, NIL(node_t), NODE_SIM_SIMPCOMP);
        node_replace(np, root_node);
        (void) speed_update_arrival_time(np, speed_param);
    }
}

/*
 * Substitute node f into other nodes in the network and not in "table".
 * The node "f" might have no fanouts if the substitution is made . In that case
 * the routine returns 1, otherwise if nothing happened it returns 0
 */
static int
speed_resub_alge_node(network, f, table)
        network_t *network;
        node_t *f;
        st_table *table;
{
    lsGen   gen;
    node_t  *np, *fo;
    int     i, changed = 0;
    array_t *target, *tl1;

    if (f->type == PRIMARY_INPUT || f->type == PRIMARY_OUTPUT) {
        return changed;
    }

    if (node_num_literal(f) < 1 || (node_num_fanout(f) > 1)) {
        return changed;
    }

    target = array_alloc(node_t * , 0);
    foreach_fanin(f, i, np)
    {
        tl1 = array_alloc(node_t * , 0);
        foreach_fanout(np, gen, fo)
        {
            if ((node_num_fanin(fo) <= 2) &&
                (table == NIL(st_table) || !st_is_member(table, (char *) fo))) {
                array_insert_last(node_t * , tl1, fo);
            }
        }
        array_append(target, tl1);
        array_free(tl1);
    }
    array_sort(target, node_compare_id);
    array_uniq(target, node_compare_id, (void (*)()) 0);

    for (i = 0; i < array_n(target); i++) {
        np = array_fetch(node_t * , target, i);
        /*
         * Only resubstitution candidates should have single fanout.
         * This way the delay (under the unit-fanout model) will
         * not worsen as a result of increased loading
         */
        if (node_num_fanout(np) < 2 && node_substitute(f, np, 0)) {
            /* See if the substituted node can be removed */
            if (node_num_fanin(f) <= 1) {
                foreach_fanout(f, gen, fo)
                {
                    (void) node_collapse(fo, f);
                }
                if (node_num_fanout(f) == 0) {
                    changed = 1;
                    break;
                }
            }
        }
    }
    array_free(target);

    return changed;
}

static st_table       *sort_info_table;
static speed_global_t *sort_speed_param;
/*
 * Order the nodes in the separator set so that the nodes closer to the
 * output are first in the array. That way, they will be collapsed earlier
 * and the problem of collapsing already collapsed nodes "should" disappear....
 */
array_t *
sp_generate_revised_order(array, speed_param, clp_table)
        array_t *array;
        speed_global_t *speed_param;
        st_table *clp_table;
{
    array_t *new_array;

    new_array        = array_dup(array);
    sort_info_table  = clp_table;
    sort_speed_param = speed_param;
    array_sort(new_array, sp_compare_cutset_nodes);
    return new_array;
}

int
sp_compare_cutset_nodes(ptr1, ptr2)
        char **ptr1;
        char **ptr2;
{
    node_t      *f;
    int         i, fanin_based_technique;
    node_t      *node1 = (node_t * ) * ptr1;
    node_t      *node2 = (node_t * ) * ptr2;
    sp_xform_t  *l1, *l2;
    sp_weight_t *wght1, *wght2;
    array_t     *array;
    network_t   *network;

    network = node_network(node1);
    assert(network == node_network(node2));

    if (sort_info_table == NIL(st_table)) {
        fanin_based_technique = TRUE;
    } else {
        assert(st_lookup(sort_info_table, (char *) node1, (char **) &wght1));
        assert(st_lookup(sort_info_table, (char *) node2, (char **) &wght2));
        l1 = sp_local_trans_from_index(sort_speed_param, wght1->best_technique);
        l2 = sp_local_trans_from_index(sort_speed_param, wght2->best_technique);
        if (l1->type != CLP && l2->type != CLP) {
            fanin_based_technique = FALSE;
        } else if (l1->type == CLP && l2->type == CLP) {
            fanin_based_technique = TRUE;
        } else {
            /* Place the FANOUT optimizations before the FANIN optimizations */
            if (l1->type) {
                return -1;
            } else {
                return 1;
            }
        }
    }
    /* Return -1 if node2 is in the tfi of node1 (and fanin_based methods
       in use) */
    array   = network_tfi(node1, INFINITY);
    for (i  = array_n(array); i-- > 0;) {
        f = array_fetch(node_t * , array, i);
        if (f == node2) {
            array_free(array);
            if (fanin_based_technique) {
                return -1;
            } else {
                return 1;
            }
        }
    }
    array_free(array);

    /* Return 1 if node1 is in the tfi of node2 */
    array  = network_tfi(node2, INFINITY);
    for (i = array_n(array); i-- > 0;) {
        f = array_fetch(node_t * , array, i);
        if (f == node1) {
            array_free(array);
            if (fanin_based_technique) {
                return 1;
            } else {
                return -1;
            }
        }
    }
    array_free(array);

    return 0;
}

/*
 *  Substitute each function in the network into all other 
 *  functions using algebraic division of the function and 
 *  its complement.
 */
static int
speed_resub_alge_network(network)
        network_t *network;
{
    lsGen  gen;
    node_t *np;
    bool   not_done;

    not_done = TRUE;
    while (not_done) {
        not_done = FALSE;
        foreach_node(network, gen, np)
        {
            if (speed_resub_alge_node(network, np, NIL(st_table))) {
                network_delete_node_gen(network, gen);
                not_done = TRUE;
            }
        }
    }
}
