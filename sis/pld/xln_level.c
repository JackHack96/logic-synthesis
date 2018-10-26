/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/pld/xln_level.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:56 $
 *
 */
#include "pld_int.h"
#include "sis.h"

/*---------------------------------------------------------------------------
  Does a delay trace on the network. Then finds a node critical node
  which can be collapsed into some of its critical fanouts. This constitutes
  one pass. This process is repeated till no such collapsing is possible.

  xln_reduce_level_one_pass() collapses a critical node into critical fanouts
  even if it can't be collapsed in all the critical fanouts.
  xln_find_critical_collapsible_node() collapses a node only if it can be
  collapsed in all the critical fanouts. Also, right now,
  xln_reduce_level_one_pass() also does first a collapse of a node into a
  fanout that may be "slightly" infeasible right now, but which has a fanin
  that has a lower level (arrival time) and hence can be cofactored wrt that
  fanin. In future, xln_find_critical_collapsible_node() should also have this
  capability.

  Also needed is a way to move fanins that are not critical, so that the node
  may be collapsed into the fanout nodes.
----------------------------------------------------------------------------*/
void xln_reduce_levels(network_t **net, xln_init_param_t *init_param) {
    network_t *network, *network1;
    int       changed_network;
    double    max_level, max_level1;
    node_t    *node;

    network = *net;

    changed_network = 1;
    assert(delay_trace(network, DELAY_MODEL_UNIT));
    while (changed_network) {
        if (XLN_DEBUG) {
            node = delay_latest_output(network, &max_level);
            (void) printf("latest output %s (%s) -> level %f\n", node_long_name(node),
                          node_name(node), max_level);
        }
        if (init_param->traversal_method == 0) {
            changed_network = xln_one_pass_by_levels(network, (double) 0, init_param);
        } else {
            changed_network = xln_one_pass_topol(network, (double) 0, init_param);
        }
        /* try to make the fanins of critical nodes as few as possible
           0 for not doing an initial delay trace. Note that moving is
           done only if MOVE_FANINS is non-zero in init_param.         */
        /*-------------------------------------------------------------*/
        if (changed_network) {
            xln_network_move_fanins_for_delay(network, init_param, 0);
        }
    }
    /* try a cofactor based approach - new network is good if number of levels
       less or levels same, but nodes less */
    /*---------------------------------------------------------------------------*/
    network1        = xln_check_network_for_collapsing_delay(network, init_param);
    if (network1 == NIL(network_t))
        return;
    (void) delay_latest_output(network, &max_level);
    (void) delay_latest_output(network1, &max_level1);
    if ((max_level1 < max_level) ||
        ((max_level1 == max_level) &&
         (network_num_internal(network1) < network_num_internal(network)))) {
        network_free(network);
        network = network1;
    } else {
        network_free(network1);
    }
    *net = network;
}

/*---------------------------------------------------------------------------
  Tries to reduce levels by collapsing a critical node into its fanouts.
  Returns 1 if any change was done in the network.
  Disadvantage: a new node may become critical after an operation: not taken
  into account... Always dealing with the old info...
-----------------------------------------------------------------------------*/
int xln_one_pass_by_levels(network_t *network, double threshold, xln_init_param_t *init_param) {
    int     changed;
    node_t  *node;
    array_t *levelsvec, *levelvec;
    int     i, j, xln_level_width_compare_function();

    if (XLN_DEBUG)
        (void) printf("----------------------------------\n");
    changed   = 0;
    /* nodevec = network_dfs(network); */
    /* first process level with minimum number of nodes */
    /*--------------------------------------------------*/
    levelsvec = xln_array_of_critical_nodes_at_levels(network, threshold);
    array_sort(levelsvec, xln_level_width_compare_function);
    for (i = 0; i < array_n(levelsvec); i++) {
        levelvec = array_fetch(array_t *, levelsvec, i);
        if (XLN_DEBUG)
            (void) printf("number of nodes = %d\n", array_n(levelvec));
        for (j = 0; j < array_n(levelvec); j++) {
            node = array_fetch(node_t *, levelvec, j);
            if (xln_node_collapse_if_critical(node, threshold, init_param)) {
                changed = 1;
                if (node_num_fanout(node) == 0)
                    network_delete_node(network, node);
                /* (void) network_cleanup(network); */ /* to delete the node too */
                assert(delay_trace(network, DELAY_MODEL_UNIT));
            }
        }
        array_free(levelvec);
    }
    array_free(levelsvec);
    return changed;
}

int xln_one_pass_topol(network_t *network, double threshold, xln_init_param_t *init_param) {
    int     changed;
    node_t  *node;
    array_t *nodevec;
    int     i;

    changed = 0;
    nodevec = network_dfs(network);
    for (i  = 0; i < array_n(nodevec); i++) {
        node = array_fetch(node_t *, nodevec, i);
        if (xln_node_collapse_if_critical(node, threshold, init_param)) {
            changed = 1;
            if (node_num_fanout(node) == 0)
                network_delete_node(network, node);
            assert(delay_trace(network, DELAY_MODEL_UNIT));
        }
    }
    array_free(nodevec);
    return changed;
}

/*---------------------------------------------------------------------------
  If it is a critical node, tries to collapse it into fanouts that are
  critical "because of this node" (i.e. with arrival time 1 greater than
  node).
    heuristic = 2 collapses the node only if it can be collapsed into all
  critical fanouts. heuristic = 1 collapses the node into whatever fanouts
  it can. It also does cofactoring if the fanout is infeasible... May move
  fanins too.
    1 is returned if any collapses or fanin-moves were done, else returns 0.
  Note: init_param->heuristic = 1 or 2 are the only ones supported.
        Node is not deleted from the network.
----------------------------------------------------------------------------*/
int xln_node_collapse_if_critical(node_t *node, double threshold, xln_init_param_t *init_param) {
    node_t       *fanout, *fanin;
    int          flag, flag1, j, num_comp_fanin, diff;
    array_t      *fanoutvec, *fanoutvec1, *comp_fanin, *xln_composite_fanin();
    lsGen        gen;
    delay_time_t arrival_node;
    st_table     *table;
    int          changed_node;
    int          size;
    network_t    *network;

    /* if node not INTERNAL, or not critical, return */
    /*-----------------------------------------------*/
    if (node->type != INTERNAL)
        return 0;
    if (!xln_is_node_critical(node, threshold))
        return 0;

    assert(init_param->heuristic == 1 || init_param->heuristic == 2);
    network      = node->network;
    size         = init_param->support;
    arrival_node = delay_arrival_time(node);
    fanoutvec    = array_alloc(node_t *, 0);
    /* table to store for infeasible fanouts(after collapse), the critical fanin
     */
    /*---------------------------------------------------------------------------*/
    if (init_param->heuristic == 1) {
        table      = st_init_table(st_ptrcmp, st_ptrhash);
        fanoutvec1 = array_alloc(node_t *, 0);
        diff       = 0;
    }
    foreach_fanout(node, gen, fanout) {
        if (fanout->type == PRIMARY_OUTPUT)
            continue;
        /* if (!xln_is_node_critical(fanout, threshold)) continue; */ /*commented
                                                                     July 30 */

        /* if fanout critical because of node and composite fanins
           <= size, select the pair for collapsing                */
        /*--------------------------------------------------------*/
        if (delay_arrival_time(fanout).rise == arrival_node.rise + 1) {
            if (init_param->heuristic == 2) {
                if (xln_num_composite_fanin(node, fanout) <= size) {
                    array_insert_last(node_t *, fanoutvec, fanout);
                } else {
                    array_free(fanoutvec);
                    return 0;
                }
            } else {
                comp_fanin     = xln_composite_fanin(node, fanout);
                num_comp_fanin = array_n(comp_fanin);
                flag1          = 0;
                if ((num_comp_fanin <= size) ||
                    ((num_comp_fanin == size + 1) && (size != 2) &&
                     (flag1    = xln_is_just_one_fanin_critical(fanout, comp_fanin, size,
                                                                &fanin)))) {
                    array_insert_last(node_t *, fanoutvec, fanout);
                    if (flag1) {
                        assert(!st_insert(table, (char *) fanout, (char *) fanin));
                    }
                } else {
                    diff = MAX(diff, num_comp_fanin - size);
                    array_insert_last(node_t *, fanoutvec1, fanout);
                }
                array_free(comp_fanin);
            }
        }
    }

    /* collapse node into selected fanouts */
    /*-------------------------------------*/
    flag   = 0;
    for (j = 0; j < array_n(fanoutvec); j++) {
        fanout = array_fetch(node_t *, fanoutvec, j);
        node_collapse(fanout, node);
        if (XLN_DEBUG)
            (void) printf("--collapsing %s into %s\n", node_long_name(node),
                          node_long_name(fanout));
        flag = 1;
    }

    /* flag is 1 whenever collapsing was done */
    /*----------------------------------------*/

    if (init_param->heuristic == 2)
        return flag;

    if (flag) {
        xln_mux_decomp(fanoutvec, network, table, size);
    }
    st_free_table(table);
    array_free(fanoutvec);
    changed_node = 0;
    if (init_param->xln_move_struct.MOVE_FANINS) {
        if (xln_node_move_fanins_for_delay(
                node, init_param->support, init_param->xln_move_struct.MAX_FANINS,
                diff, init_param->xln_move_struct.bound_alphas)) {
            changed_node = xln_try_collapsing_node(node, fanoutvec1, init_param);
        }
    }
    array_free(fanoutvec1);
    return (flag || changed_node);
}

/*----------------------------------------------------------------------------
  If there is at most one node in comp_fanin that has level level_fanout - 2
  and none with level_fanout - 1, then return 1, else return 0.
-----------------------------------------------------------------------------*/
void xln_is_just_one_fanin_critical(node_t *fanout, array_t *comp_fanin, int size, node_t **fanin) {
    int          i, num, num_level_fanout_minus_2;
    double       level_fanout, level_fanout_minus_2, level_fanout_minus_1, level_node;
    delay_time_t arrival_fanout, arrival_node;
    node_t       *node;

    *fanin = NIL(node_t);
    arrival_fanout           = delay_arrival_time(fanout);
    level_fanout             = arrival_fanout.rise;
    level_fanout_minus_2     = level_fanout - 2.00000;
    level_fanout_minus_1     = level_fanout - 1.00000;
    num_level_fanout_minus_2 = 0;
    num                      = array_n(comp_fanin);
    for (i                   = 0; i < num; i++) {
        node         = array_fetch(node_t *, comp_fanin, i);
        arrival_node = delay_arrival_time(node);
        level_node   = arrival_node.rise;
        if (level_node == level_fanout_minus_2) {
            num_level_fanout_minus_2++;
            *fanin = node;
            if (num_level_fanout_minus_2 > 1)
                return 0;
        } else {
            if (level_node == level_fanout_minus_1)
                return 0;
        }
    }
    return 1;
}

/*------------------------------------------------------
  Using the data structure of delay package, tell if the
  node is critical.
-------------------------------------------------------*/
int xln_is_node_critical(node_t *node, double threshold) {

    delay_time_t slack;
    slack = delay_slack_time(node);
    assert(slack.rise == slack.fall);
    if (slack.rise <= threshold)
        return 1;
    return 0;
}

/*-----------------------------------------------------
  Using sophisticated delay model, tells if node is
  critical.
------------------------------------------------------*/
/* commented because pld_get_node_slack not defined here */
/*
xln_is_node_critical_sophis(node, threshold)
  node_t *node;
  double threshold;
{
  double pld_get_node_slack();

  if (pld_get_node_slack(node) <= threshold) return 1;
  return 0;
}
*/

int xln_num_composite_fanin(node_t *n1, node_t *n2) {
    int    is_n2_fanin_of_n1, is_n1_fanin_of_n2, i;
    int    num_composite_fanin;
    node_t *fanin;

    /* assert(n1->type == INTERNAL);
    assert(n2->type == INTERNAL); */
    is_n2_fanin_of_n1 = node_get_fanin_index(n1, n2);
    is_n1_fanin_of_n2 = node_get_fanin_index(n2, n1);

    num_composite_fanin = node_num_fanin(n1);
    foreach_fanin(n2, i, fanin) {
        if (node_get_fanin_index(n1, fanin) == (-1))
            num_composite_fanin++;
    }
    if ((is_n2_fanin_of_n1 >= 0) || (is_n1_fanin_of_n2 >= 0))
        num_composite_fanin--;
    return num_composite_fanin;
}

array_t *xln_composite_fanin(node_t *n1, node_t *n2) {
    array_t *faninvec;
    node_t  *fanin;
    int     i;

    assert(n1->type == INTERNAL);
    assert(n2->type == INTERNAL);

    faninvec = array_alloc(node_t *, 0);
    foreach_fanin(n1, i, fanin) {
        if (fanin != n2)
            array_insert_last(node_t *, faninvec, fanin);
    }
    foreach_fanin(n2, i, fanin) {
        if ((node_get_fanin_index(n1, fanin) == (-1)) && (fanin != n1))
            array_insert_last(node_t *, faninvec, fanin);
    }
    return faninvec;
}

void xln_mux_decomp(array_t *fanoutvec, network_t *network, st_table *table, int size) {
    int    num, num_fanin, i;
    node_t *fanout, *crit_fanin, *crit_fanin0, *crit_fanin1;
    node_t *pos, *neg, *rem, *p, *q, *p1, *q1, *temp1, *temp2, *new_fanout;

    num    = array_n(fanoutvec);
    for (i = 0; i < num; i++) {
        fanout    = array_fetch(node_t *, fanoutvec, i);
        num_fanin = node_num_fanin(fanout);
        if (num_fanin <= size)
            continue;
        /* fanout has size + 1 inputs, decompose it using
           the mux decomposition */
        /*----------------------------------------------*/
        assert(num_fanin == size + 1);
        assert(st_lookup(table, (char *) fanout, (char **) &crit_fanin));
        node_algebraic_cofactor(fanout, crit_fanin, &pos, &neg, &rem);
        p           = node_or(pos, rem);
        q           = node_or(neg, rem);
        crit_fanin0 = node_literal(crit_fanin, 0); /* complemented */
        crit_fanin1 = node_literal(crit_fanin, 1); /* uncomplemented */
        /* orig fanout = crit_fanin1 p + crit_fanin0 q */
        /*---------------------------------------------*/
        p1          = node_literal(p, 1);
        q1          = node_literal(q, 1);
        temp1       = node_and(p1, crit_fanin1);
        temp2       = node_and(q1, crit_fanin0);
        new_fanout  = node_or(temp1, temp2);
        node_free(crit_fanin0);
        node_free(crit_fanin1);
        node_free(pos);
        node_free(neg);
        node_free(rem);
        node_free(p1);
        node_free(q1);
        node_free(temp1);
        node_free(temp2);
        network_add_node(network, p);
        network_add_node(network, q);
        node_replace(fanout, new_fanout);
        if (XLN_DEBUG) {
            (void) printf("mux decomposing %s ", node_long_name(fanout));
            (void) printf("critical fanin %s\n", node_long_name(crit_fanin));
            node_print(sisout, p);
            node_print(sisout, q);
            node_print(sisout, fanout);
        }
    }
}

/*----------------------------------------------------------------------------
  Tries to collapse node into the fanouts in fanoutvec if they are feasible
  after collapsing. If the collapse cannot be done, tries to move the
  non-critical fanins of the fanout to get some benefit.
  Returns 1 if any collapse took place. Else returns 0.
-----------------------------------------------------------------------------*/
int xln_try_collapsing_node(node_t *node, array_t *fanoutvec, xln_init_param_t *init_param) {
    int    changed;
    int    i;
    node_t *fanout;
    int    num_comp, diff;

    changed = 0;
    for (i  = 0; i < array_n(fanoutvec); i++) {
        fanout   = array_fetch(node_t *, fanoutvec, i);
        num_comp = xln_num_composite_fanin(node, fanout);
        diff     = num_comp - init_param->support;
        if (diff <= 0) {
            changed = 1;
            node_collapse(node, fanout);
        } else {
            /* try to move fanins of fanout */
            /*------------------------------*/
            if (init_param->xln_move_struct.MOVE_FANINS) {
                if (diff == xln_node_move_fanins_for_delay(
                        fanout, init_param->support,
                        init_param->xln_move_struct.MAX_FANINS, diff,
                        init_param->xln_move_struct.bound_alphas)) {
                    changed = 1;
                    node_collapse(node, fanout);
                }
            }
        }
    }
    return changed;
}

/*---------------------------------------------------------------------------------
  If the network has small number of inputs, collapses the network. Then applies
  Roth-Karp decomposition and cofactoring techniques on the network and
evaluates them. Returns the network with lower number of levels. Returns NIL if
collapsing could not be done.
-----------------------------------------------------------------------------------*/
network_t *xln_check_network_for_collapsing_delay(network_t *network, xln_init_param_t *init_param) {
    network_t *network1, *network2;
    double    max_level1, max_level2;

    if (network_num_pi(network) > init_param->collapse_input_limit)
        return NIL(network_t);

    /* for the time being, do not handle support = 2 case */
    /*----------------------------------------------------*/
    if (init_param->support == 2)
        return NIL(network_t);

    network1 = network_dup(network);
    (void) network_collapse(network1);
    network2 = network_dup(network1);

    pld_simplify_network_without_dc(network1);
    (void) network_sweep(network1);
    assert(delay_trace(network1, DELAY_MODEL_UNIT));
    xln_cofactor_decomp_network(network1, init_param->support, DELAY);
    assert(delay_trace(network1, DELAY_MODEL_UNIT));

    karp_decomp_network(network2, init_param->support, 0, NIL(node_t));
    assert(xln_is_network_feasible(network2, init_param->support));
    assert(delay_trace(network2, DELAY_MODEL_UNIT));

    (void) delay_latest_output(network1, &max_level1);
    (void) delay_latest_output(network2, &max_level2);
    if (max_level1 < max_level2) {
        network_free(network2);
        return network1;
    }
    if (max_level1 == max_level2) {
        if (network_num_internal(network1) < network_num_internal(network2)) {
            network_free(network2);
            return network1;
        }
    }
    network_free(network1);
    return network2;
}

/*------------------------------------------------------------
  Cofactors each infeasible node of the network. The mode is
  either AREA or DELAY. DELAY mode made faster by levelling
  the network and treating each level at one step. Only
  after the whole level is processed, a delay trace is done
  if needed. Assumes initially that  a delay trace using
  DELAY_MODEL_UNIT has been done (if mode == DELAY).
--------------------------------------------------------------*/
void xln_cofactor_decomp_network(network_t *network, int support, float mode) {
    array_t *levelsvec, *levelvec;
    array_t *nodevec;
    int     i, j;
    int     changed; /* set to 1 for a level if a node at that level was decomposed */
    node_t  *node;

    if (mode == AREA) {
        changed = 0;
        nodevec = network_dfs(network);
        for (i  = 0; i < array_n(nodevec); i++) {
            node = array_fetch(node_t *, nodevec, i);
            if (xln_cofactor_decomp_node(node, support, AREA)) {
                changed = 1;
            }
        }
        array_free(nodevec);
        if (changed)
            (void) network_sweep(network);
        return;
    }

    /* mode == DELAY */
    /*---------------*/
    levelsvec = xln_array_of_levels(network);
    for (i    = 0; i < array_n(levelsvec); i++) {
        levelvec = array_fetch(array_t *, levelsvec, i);
        changed  = 0;
        for (j   = 0; j < array_n(levelvec); j++) {
            node = array_fetch(node_t *, levelvec, j);
            if (xln_cofactor_decomp_node(node, support, DELAY)) {
                changed = 1;
            }
        }
        array_free(levelvec);
        if (changed) {
            (void) network_sweep(network);
            assert(delay_trace(network, DELAY_MODEL_UNIT));
        }
    }
    array_free(levelsvec);
}

/*---------------------------------------------------------------------
  Arranges nodes of the network in an array of levels. Assumes that
  delay trace has been done on the network.
----------------------------------------------------------------------*/
array_t *xln_array_of_levels(network_t *network) {
    double       max_level;
    int          imax_level;
    array_t      *levelsvec, *levelvec;
    node_t       *node;
    delay_time_t arrival_node;
    int          level;
    lsGen        gen;
    int          i;

    (void) delay_latest_output(network, &max_level);
    imax_level     = (int) max_level;
    if (imax_level < 0)
        imax_level = 0;
    levelsvec      = array_alloc(array_t *, 0);
    for (i         = 0; i <= imax_level; i++) {
        levelvec = array_alloc(node_t *, 0);
        array_insert_last(array_t *, levelsvec, levelvec);
    }
    foreach_node(network, gen, node) {
        arrival_node = delay_arrival_time(node);
        level        = (int) arrival_node.rise;
        assert(level <= imax_level);
        /* for nodes with no inputs - Added July 5 '93 */
        /*---------------------------------------------*/
        if (level < 0)
            level = 0;
        levelvec  = array_fetch(array_t *, levelsvec, level);
        array_insert_last(node_t *, levelvec, node);
    }
    return levelsvec;
}

/*-----------------------------------------------------------------------
  Given a network, create an array of levels having critical nodes.
------------------------------------------------------------------------*/
array_t *xln_array_of_critical_nodes_at_levels(network_t *network, double threshold) {
    double       max_level;
    int          imax_level;
    array_t      *levelsvec, *levelvec;
    node_t       *node;
    delay_time_t arrival_node;
    int          level;
    lsGen        gen;
    int          i;

    (void) delay_latest_output(network, &max_level);
    imax_level     = (int) max_level;
    if (imax_level < 0)
        imax_level = 0;
    levelsvec      = array_alloc(array_t *, 0);
    for (i         = 0; i <= imax_level; i++) {
        levelvec = array_alloc(node_t *, 0);
        array_insert_last(array_t *, levelsvec, levelvec);
    }
    foreach_node(network, gen, node) {
        if (xln_is_node_critical(node, threshold)) {
            arrival_node = delay_arrival_time(node);
            level        = (int) arrival_node.rise;
            assert(level <= imax_level);
            /* for nodes with no inputs - Added July 5 '93 */
            /*---------------------------------------------*/
            if (level < 0)
                level = 0;
            levelvec  = array_fetch(array_t *, levelsvec, level);
            array_insert_last(node_t *, levelvec, node);
        }
    }
    return levelsvec;
}

int xln_level_width_compare_function(array_t **obj1, array_t **obj2) { return (array_n(*obj1) - array_n(*obj2)); }

/*----------------------------------------------------------------------
  Given a level of nodes, finds all fanouts critical because of some of
  them. Makes copies of all these nodes,
------------------------------------------------------------------------*/
