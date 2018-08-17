
#include "sis.h"
#include "../include/pld_int.h"

/*--------------------------------------------------------------------------------
   Given a mapped network, in which each node can be mapped onto
   one basic block of MB architecture, prints out the value of the 
   arrival time of each of the outputs. Basically a forward delay trace. 
   Stores the arrival times in a table. Assumes that the arrival times of
   the primary inputs have been set by delay_set command, else assumes them to 
   be all 0.0
----------------------------------------------------------------------------------*/

void
act_delay_trace_forward(network, cost_table, delay_values)
        network_t *network;
        st_table *cost_table;
        array_t *delay_values;
{
    double          delaynum, total_delay, fanin_delay, max_fanin_delay, max_output_arrival_time;
    int             i, j, nsize, num_fanout;
    node_t          *node, *fanin, *slowest_po;
    array_t         *nodevec;
    node_function_t node_fun;
    COST_STRUCT     *cost_node;

    max_output_arrival_time = (double) (-1.0);
    slowest_po              = NIL(node_t);

    nodevec = network_dfs(network);
    nsize   = array_n(nodevec);

    for (i = 0; i < nsize; i++) {
        node     = array_fetch(node_t * , nodevec, i);
        node_fun = node_function(node);

        /* set Primary Input arrival time */
        /*--------------------------------*/
        if (node_fun == NODE_PI) {
            cost_node = act_set_pi_arrival_time_node(node, cost_table);
            if (ACT_DEBUG)
                (void) printf("---- arrival time of node %s = %f\n", node_long_name(node), cost_node->arrival_time);
            continue;
        }

        /* Primary Output's arrival time is same as its fanin */
        /*----------------------------------------------------*/
        if (node_fun == NODE_PO) {
            fanin       = node_get_fanin(node, 0);
            fanin_delay = act_get_arrival_time(fanin, cost_table);
            cost_node   = act_set_arrival_time(node, cost_table, fanin_delay);
            if (ACT_DEBUG)
                (void) printf("---- arrival time of node %s = %f\n", node_long_name(node), cost_node->arrival_time);
            if (cost_node->arrival_time > max_output_arrival_time) {
                max_output_arrival_time = cost_node->arrival_time;
                slowest_po              = node;
            }
            continue;
        }

        max_fanin_delay = (double) (0.0);

        foreach_fanin(node, j, fanin)
        {
            fanin_delay          = act_get_arrival_time(fanin, cost_table);
            if (fanin_delay < max_fanin_delay) continue;
            else max_fanin_delay = fanin_delay;
        }
        num_fanout      = node_num_fanout(node);
        if (num_fanout == 0) fail("Error: act_delay_trace(): node with 0 fanout found");
        delaynum    = act_delay_for_fanout(delay_values, num_fanout);
        total_delay = delaynum + max_fanin_delay;
        cost_node   = act_set_arrival_time(node, cost_table, total_delay);
        if (ACT_DEBUG)
            (void) printf("---- arrival time of node %s = %f\n", node_long_name(node), cost_node->arrival_time);

    }
    if ((ACT_DEBUG) || (ACT_STATISTICS))
        (void) printf("---- max arrival time is for node %s = %f\n", node_long_name(slowest_po),
                      max_output_arrival_time);

    array_free(nodevec);
}

/*--------------------------------------------------------------------------------
   Given a mapped network, in which each node can be mapped onto
   one basic block of MB architecture, prints out the value of the 
   required time of each of the outputs. Basically a delay trace. 
   Stores the required times in a table. Assumes that the required times of
   the primary outputs have been set by delay_set command. Sets the unset 
   required time of a po to the highest required time. If none of the required
   times were set, the required times of all outputs are set to max arrival time
   of some output.
----------------------------------------------------------------------------------*/
void
act_delay_trace_backward(network, cost_table, delay_values)
        network_t *network;
        st_table *cost_table;
        array_t *delay_values;
{
    double          delaynum, max_arrival_time;
    double          max_output_required_time, required_time, min_node_required_time, fanout_required_time;
    double          node_required_time;
    int             i, nsize, num_fanout;
    node_t          *node, *fanout, *po;
    array_t         *nodevec;
    node_function_t node_fun;
    COST_STRUCT     *cost_node;
    st_table        *po_table;  /* to keep track of all the po's that have their required times set. */
    int             flag_all_set, flag_some_not_set; /* indicate if the required times of po's were set in the first pass */
    lsGen           gen;

    po_table = st_init_table(strcmp, st_strhash);

    /* set the required times of all the primary_outputs first */
    /*---------------------------------------------------------*/
    flag_all_set             = 0;
    flag_some_not_set        = 0;
    max_output_required_time = (double) (-1.0);

    foreach_primary_output(network, gen, po)
    {
        required_time = delay_get_parameter(po, DELAY_REQUIRED_RISE);
        if (required_time == DELAY_NOT_SET) flag_some_not_set = 1;
        else {
            flag_all_set = 1;
            if (required_time > max_output_required_time) {
                max_output_required_time = required_time;
            }
            (void) act_set_required_time(po, cost_table, required_time);
        }

    }
    /* if flag_all_set is 0, this means none of the required times were specified */
    /* find the largest arrival time at some output and set all the
       required times to it.							*/
    /*----------------------------------------------------------------------------*/
    if (flag_all_set == 0) {
        max_arrival_time = act_get_max_arrival_time(network, cost_table);
        foreach_primary_output(network, gen, po)
        {
            (void) act_set_required_time(po, cost_table, max_arrival_time);
            assert(!st_insert(po_table, (char *) node_long_name(po), (char *) node_long_name(po)));
        }
    } else if (flag_some_not_set == 0); /* all po's required times have been set */
    else {
        /* set required times of not set outputs to the max_output_required_time */
        /*------------------------------------------------------------------------*/
        foreach_primary_output(network, gen, po)
        {
            if (!st_is_member(po_table, (char *) node_long_name(po)))
                (void) act_set_required_time(po, cost_table, max_output_required_time);
        }
    }
    st_free_table(po_table);

    nodevec = network_dfs_from_input(network);
    nsize   = array_n(nodevec);

    for (i = 0; i < nsize; i++) {
        node     = array_fetch(node_t * , nodevec, i);
        node_fun = node_function(node);
        if (node_fun == NODE_PO) continue;

        min_node_required_time = (double) (MAXINT);

        foreach_fanout(node, gen, fanout)
        {
            fanout_required_time        = act_get_required_time(fanout, cost_table);

            /* if the fanout is a PO, no delay through it basically.  */
            /*--------------------------------------------------------*/
            if (fanout->type == PRIMARY_OUTPUT) {
                node_required_time = fanout_required_time;
            } else {
                num_fanout = node_num_fanout(fanout);
                if (num_fanout == 0) fail("Error: act_delay_trace(): node with 0 fanout found");
                delaynum           = act_delay_for_fanout(delay_values, num_fanout);
                node_required_time = fanout_required_time - delaynum;
            }
            if (node_required_time >= min_node_required_time) continue;
            else min_node_required_time = node_required_time;
        }
        cost_node              = act_set_required_time(node, cost_table, min_node_required_time);
        if (ACT_DEBUG)
            (void) printf("---- arrival time of node %s = %f\n", node_long_name(node), cost_node->required_time);

    }
    array_free(nodevec);
}

double
act_get_arrival_time(node, cost_table)
        node_t *node;
        st_table *cost_table;
{
    COST_STRUCT *cost_node;

    assert(st_lookup(cost_table, node_long_name(node), (char **) &cost_node));
    return cost_node->arrival_time;
}

double
act_get_required_time(node, cost_table)
        node_t *node;
        st_table *cost_table;
{
    COST_STRUCT *cost_node;

    assert(st_lookup(cost_table, node_long_name(node), (char **) &cost_node));
    return cost_node->required_time;
}

/*****************************************************************************
  Sets (updates) the arrival_time of the node in the cost_table to arrival. 
  Allocates storage for a node if it is not found in the cost_table.
******************************************************************************/
COST_STRUCT *
act_set_arrival_time(node, cost_table, arrival)
        node_t *node;
        st_table *cost_table;
        double arrival;
{
    COST_STRUCT *cost_node;
    char        *name;

    if (st_lookup(cost_table, node_long_name(node), (char **) &cost_node)) {
        cost_node->arrival_time = arrival;
        if (ACT_DEBUG)
            (void) printf("act_set_arrival_time():node %s found in the cost_table...\n", node_long_name(node));
        if (ACT_DEBUG) (void) printf("setting the arrival time of %s to %f..\n", node_long_name(node), arrival);
        return cost_node;
    }
    /* cost_node not found. Allocate it and set the fields */
    /*-----------------------------------------------------*/
    name = ALLOC(
    char, strlen(node_long_name(node)) + 1);
    (void) strcpy(name, node_long_name(node));
    cost_node = ALLOC(COST_STRUCT, 1);
    cost_node->node                  = node;
    cost_node->arrival_time          = arrival;
    cost_node->required_time         = (double) (-1.0);
    cost_node->cost_and_arrival_time = (double) (-1.0);
    cost_node->cost                  = 0;
    cost_node->act                   = NIL(ACT_VERTEX);
    assert(!st_insert(cost_table, name, (char *) cost_node));
    if (ACT_DEBUG)
        (void) printf("act_set_arrival_time():node %s not found in the cost_table...\n", node_long_name(node));
    if (ACT_DEBUG) (void) printf("setting the arrival time of %s to %f..\n", node_long_name(node), arrival);
    return cost_node;

}

/*****************************************************************************
  Sets (updates) the required_time of the node in the cost_table to required.
  Allocates storage for a node if it is not found in the cost_table.
******************************************************************************/
COST_STRUCT *
act_set_required_time(node, cost_table, required)
        node_t *node;
        st_table *cost_table;
        double required;
{
    COST_STRUCT *cost_node;
    char        *name;

    if (st_lookup(cost_table, node_long_name(node), (char **) &cost_node)) {
        cost_node->required_time = required;
        if (ACT_DEBUG)
            (void) printf("act_set_required_time():node %s found in the cost_table...\n", node_long_name(node));
        if (ACT_DEBUG) (void) printf("setting the required time of %s to %f..\n", node_long_name(node), required);
        return cost_node;
    }
    /* cost_node not found. Allocate it and set the fields */
    /*-----------------------------------------------------*/
    name = ALLOC(
    char, strlen(node_long_name(node)) + 1);
    (void) strcpy(name, node_long_name(node));
    cost_node = ALLOC(COST_STRUCT, 1);
    cost_node->node                  = node;
    cost_node->arrival_time          = (double) (-1.0);
    cost_node->required_time         = required;
    cost_node->cost_and_arrival_time = (double) (-1.0);
    cost_node->cost                  = 0;
    cost_node->act                   = NIL(ACT_VERTEX);
    assert(!st_insert(cost_table, name, (char *) cost_node));
    if (ACT_DEBUG)
        (void) printf("act_set_required_time():node %s not found in the cost_table...\n", node_long_name(node));
    if (ACT_DEBUG) (void) printf("setting the required time of %s to %f..\n", node_long_name(node), required);
    return cost_node;

}

/*************************************************************
  Finds the fanout of the vertex in the bdd. Calculates 
  and returns the propagation delay, taking into account 
  the fanouts.
*************************************************************/

double
act_get_bddfanout_delay(vertex, delay_values, cost_table)
        ACT_VERTEX_PTR vertex;
        array_t *delay_values;
        st_table *cost_table;
{
    int    num_fanout;
    double delaynum;

    num_fanout = vertex->multiple_fo + 1;
    delaynum   = act_delay_for_fanout(delay_values, num_fanout);
    return delaynum;
}

/*------------------------------------------------------------
  Finds the fanout of the node. Calculates 
  the propagation delay delaynum1, taking into account 
  the actual_numfo's, calculates delaynum2, delay 
  for assumed_numfo's and returns the difference
--------------------------------------------------------------*/
double
act_get_node_delay_correction(node, delay_values, assumed_numfo, actual_numfo)
        node_t *node;
        array_t *delay_values;
        int assumed_numfo, actual_numfo;
{
    double delaynum1, delaynum2;
    int    diff;

    if ((actual_numfo == 0) || (assumed_numfo == 0)) return ((double) 0.0);

    diff = actual_numfo - assumed_numfo;
    if (diff == 0) return ((double) 0.0);

    delaynum1 = act_delay_for_fanout(delay_values, actual_numfo);
    delaynum2 = act_delay_for_fanout(delay_values, assumed_numfo);
    return (delaynum1 - delaynum2);
}

/*-----------------------------------------------------------------
  Gets the delay values set by the user for the primary inputs
  of the network, and stores them in the cost_table.
------------------------------------------------------------------*/
void
act_set_pi_arrival_time_network(network, cost_table)
        network_t *network;
        st_table *cost_table;
{
    node_t *pi;
    lsGen  gen;


    foreach_primary_input(network, gen, pi)
    {
        (void) act_set_pi_arrival_time_node(pi, cost_table);
    }
}

/*-----------------------------------------------------------------
  Gets the delay values set by the user for the primary input pi
  and stores them in the cost_table. If does not find the entry in 
  the cost table corresponding to pi, creates one. Returns the
  entry cost_node.
------------------------------------------------------------------*/
COST_STRUCT *
act_set_pi_arrival_time_node(pi, cost_table)
        node_t *pi;
        st_table *cost_table;
{
    double arrival_time;

    arrival_time = delay_get_parameter(pi, DELAY_ARRIVAL_RISE);
    if (arrival_time == DELAY_NOT_SET) arrival_time = (double) 0.0;
    return act_set_arrival_time(pi, cost_table, arrival_time);

}

void
act_invalidate_cost_and_arrival_time(cost_node)
        COST_STRUCT *cost_node;
{
    cost_node->cost_and_arrival_time = (double) (-1.0);
}

double
act_cost_delay(cost_node, mode)
        COST_STRUCT *cost_node;
        float mode;
{
    if (cost_node->cost_and_arrival_time < 0.0)
        return
                (double) (((double) (1.00 - mode)) * (double) cost_node->cost +
                          ((double) mode) * cost_node->arrival_time
                );
    else return cost_node->cost_and_arrival_time;
}

/*----------------------------------------------------------------------
   Returns the delay-value for a basic block if the number of fanouts
   for a signal is num_fanout. The delay_values store the delay-values
   upto a certain fanout number. If num_fanout is larger than that,
   a simple linear extrapolation is done.
------------------------------------------------------------------------*/
double
act_delay_for_fanout(delay_values, num_fanout)
        array_t *delay_values;
        int num_fanout;
{
    int    delay_size, delay_size1;
    double delaynum, delaynum1, delaynum2;

    delay_size  = array_n(delay_values);
    delay_size1 = delay_size - 1;

    if (num_fanout > (delay_size - 1)) {
        /* linear extrapolation from last two values in delay_values */
        /*-----------------------------------------------------------*/
        delaynum1 = array_fetch(
        double, delay_values, delay_size1);
        delaynum2 = array_fetch(
        double, delay_values, delay_size - 2);
        delaynum = delaynum1 + (delaynum1 - delaynum2) * (double) (num_fanout - delay_size1);
    } else {
        delaynum = array_fetch(
        double, delay_values, num_fanout);
    }
    return delaynum;
}

void
act_set_slack_network(network, cost_table)
        network_t *network;
        st_table *cost_table;
{
    lsGen  gen;
    node_t *node;

    foreach_node(network, gen, node)
    {
        (void) act_set_slack_node(node, cost_table);
    }
}

/*-------------------------------------------------------------------------
   Assuming that the cost_node for the node is there in the cost_table,
   computes the slack and stores it in the cost_node.
--------------------------------------------------------------------------*/
COST_STRUCT *
act_set_slack_node(node, cost_table)
        node_t *node;
        st_table *cost_table;
{
    COST_STRUCT *cost_node;

    assert(st_lookup(cost_table, node_long_name(node), (char **) &cost_node));
    cost_node->slack = cost_node->required_time - cost_node->arrival_time;
    return cost_node;
}

/*-------------------------------------------------------------------------
   Assuming that the cost_node for the node is there in the cost_table,
   returns the slack.
--------------------------------------------------------------------------*/
double
act_get_slack_node(node, cost_table)
        node_t *node;
        st_table *cost_table;
{
    COST_STRUCT *cost_node;

    assert(st_lookup(cost_table, node_long_name(node), (char **) &cost_node));
    return cost_node->slack;
}


/*------------------------------------------------------------------------
   Make cost_node's is_critical field YES if the node is critical.
   A node is defined critical if its slack is less than or equal to
   threshold_slack. Assumes that the slack of the node has been set 
   already in the cost_node's slack field. 
--------------------------------------------------------------------------*/
void
act_find_critical_nodes(network, cost_table, threshold_slack)
        network_t *network;
        st_table *cost_table;
        double threshold_slack;
{
    node_t      *node;
    lsGen       gen;
    COST_STRUCT *cost_node;
    int         num_critical_nodes;

    num_critical_nodes = 0;
    foreach_node(network, gen, node)
    {
        assert(st_lookup(cost_table, node_long_name(node), (char **) &cost_node));
        if (cost_node->slack <= threshold_slack) {
            cost_node->is_critical = YES;
            num_critical_nodes++;
        } else cost_node->is_critical = NO;
    }
    if (ACT_DEBUG)
        (void) printf(" threshold slack = %f, Number of critical nodes = %d\n",
                      threshold_slack, num_critical_nodes);
}

array_t *
act_compute_area_delay_weight_network_for_collapse(network, cost_table, mode)
        network_t *network;
        st_table *cost_table;
        float mode;
{
    lsGen       gen;
    node_t      *node;
    COST_STRUCT *cost_node;
    array_t     *c_pair_array, *c_node_pair_array;

    c_pair_array = array_alloc(COLLAPSIBLE_PAIR * , 0);
    foreach_node(network, gen, node)
    {
        if (node->type != INTERNAL) continue; /* important probably */
        assert(st_lookup(cost_table, node_long_name(node), (char **) &cost_node));
        cost_node->area_weight = act_compute_area_weight_node_for_collapse(node, cost_node);
        c_node_pair_array = act_compute_area_delay_weight_node_for_collapse(node, cost_node, cost_table, mode);
        if ((c_node_pair_array == NIL(array_t)) || (array_n(c_node_pair_array) == 0));
        else array_append(c_pair_array, c_node_pair_array);
        array_free(c_node_pair_array); /* should I free c_node_pair_array */
    }
    return c_pair_array;
}

/*----------------------------------------------------------------------------
  Given a node, for each of its fanout, it finds out the area_delay weight of 
  the pair. Returns an array of the collapsible pairs.
-----------------------------------------------------------------------------*/
array_t *
act_compute_area_delay_weight_node_for_collapse(node, cost_node, cost_table, mode)
        node_t *node;
        COST_STRUCT *cost_node;
        st_table *cost_table;
        float mode;
{
    node_t           *fanout;
    lsGen            gen;
    COST_STRUCT      *cost_fanout;
    array_t          *c_pair_array;
    double           small_arrival_at_input_of_fanout, largest_arrival_at_input_of_node, diff;
    COLLAPSIBLE_PAIR *c_pair;

    if (cost_node->is_critical == NO) return NIL(array_t);
    if (node->type != INTERNAL) return NIL(array_t);

    c_pair_array = array_alloc(COLLAPSIBLE_PAIR * , 0);
    foreach_fanout(node, gen, fanout)
    {
        if (fanout->type == PRIMARY_OUTPUT) continue;
        assert(st_lookup(cost_table, node_long_name(fanout), (char **) &cost_fanout));
        if (cost_fanout->is_critical == NO) continue;
        /* what happens to the next routine if fanout has just 1 input? */
        /*--------------------------------------------------------------*/
        small_arrival_at_input_of_fanout = act_smallest_fanin_arrival_time_at_fanout_except_node(fanout, node,
                                                                                                 cost_table);
        if (small_arrival_at_input_of_fanout >= (double) MAXINT) continue; /* is this OK */
        largest_arrival_at_input_of_node = act_largest_fanin_arrival_time_at_node(node, cost_table);
        diff                             = largest_arrival_at_input_of_node - small_arrival_at_input_of_fanout;
        if (diff <= 0.0) continue; /* check */

        /* assign a collapsible_pair to (node, fanout) pair, assign storage for their names */
        /*----------------------------------------------------------------------------------*/
        c_pair = act_allocate_collapsible_pair();
        c_pair->nodename   = util_strsav(node_long_name(node));
        c_pair->fanoutname = util_strsav(node_long_name(fanout));
        c_pair->weight     = ((double) (cost_node->area_weight * (1.00 - mode))) + diff * (mode); /* is this OK ? */
        array_insert_last(COLLAPSIBLE_PAIR * , c_pair_array, c_pair);

    }
    return c_pair_array;
}

/*-----------------------------------------------------------------
  Given a node and its cost_node, returns the area_cost if the node
  is to be collapsed into the fanout.
-----------------------------------------------------------------*/
double
act_compute_area_weight_node_for_collapse(node, cost_node)
        node_t *node;
        COST_STRUCT *cost_node;
{
    int num_fanout;

    if (node->type != INTERNAL) return ((double) (-1.0));
    num_fanout = node_num_fanout(node);
    assert(num_fanout);
    if (num_fanout == 1) return (double) 0.0;
    return ((double) (cost_node->cost));
}

/*-----------------------------------------------------------------
   Returns a new COLLAPSIBLE_PAIR, with weight 0.0.
------------------------------------------------------------------*/
COLLAPSIBLE_PAIR *
act_allocate_collapsible_pair() {
    COLLAPSIBLE_PAIR *c_pair;

    c_pair = ALLOC(COLLAPSIBLE_PAIR, 1);
    c_pair->weight = (double) 0.0;
    return c_pair;
}

double
act_smallest_fanin_arrival_time_at_fanout_except_node(fanout, node, cost_table)
        node_t *fanout, *node;
        st_table *cost_table;
{
    double      act_min;
    node_t      *fanin;
    int         i;
    COST_STRUCT *cost_fanin;

    act_min = (double) MAXINT;
    foreach_fanin(fanout, i, fanin)
    {
        if (fanin == node) continue;
        assert(st_lookup(cost_table, node_long_name(fanin), (char **) &cost_fanin));
        if (act_min > cost_fanin->arrival_time) act_min = cost_fanin->arrival_time;
    }
    return act_min;
}

double
act_largest_fanin_arrival_time_at_node(node, cost_table)
        node_t *node;
        st_table *cost_table;
{
    node_t      *fanin;
    double      act_max;
    int         i;
    COST_STRUCT *cost_fanin;

    act_max = (double) (-1.0);
    foreach_fanin(node, i, fanin)
    {
        assert(st_lookup(cost_table, node_long_name(fanin), (char **) &cost_fanin));
        if (act_max < cost_fanin->arrival_time) act_max = cost_fanin->arrival_time;
    }
    return act_max;
}

/*-------------------------------------------------------------------
  Small weight comes towards beginning of the array.
--------------------------------------------------------------------*/
int c_pair_compare_function(cp1, cp2)
        COLLAPSIBLE_PAIR **cp1, **cp2;
{

    double wt1, wt2;

    wt1 = (*cp1)->weight;
    wt2 = (*cp2)->weight;

    if (wt1 > wt2) return 1;
    if (wt2 > wt1) return (-1);
    return 0;
}

/*------------------------------------------------------------
   Looks at the primary outputs and find out the maximum
   arrival times of these.
------------------------------------------------------------*/
double act_get_max_arrival_time(network, cost_table)
        network_t *network;
        st_table *cost_table;
{
    lsGen  gen;
    node_t *po;
    double max_arrival_time, arrival_time;

    max_arrival_time = (double) (-1.0);
    foreach_primary_output(network, gen, po)
    {
        arrival_time = act_get_arrival_time(po, cost_table);
        if (max_arrival_time < arrival_time) max_arrival_time = arrival_time;
    }
    return max_arrival_time;
}

/*-----------------------------------------------------------------------
   Returns a hash table, which for every node has its topological index
   stored in it. Later free the entries of the hash table.
------------------------------------------------------------------------*/
st_table *
act_assign_topol_indices_network(network)
        network_t *network;
{
    array_t      *nodevec;
    int          size, i;
    node_t       *node;
    st_table     *topol_table;
    TOPOL_STRUCT *topol_node;

    topol_table = st_init_table(strcmp, st_strhash);

    nodevec = network_dfs(network);
    size    = array_n(nodevec);
    for (i  = 0; i < size; i++) {
        node       = array_fetch(node_t * , nodevec, i);
        topol_node = ALLOC(TOPOL_STRUCT, 1);
        /* nodename is stored instead of the node because node may have
       been replaced (physically) by another node. Hence name
       provides a reference, as the new node will have the same name */
        /*---------------------------------------------------------------*/
        topol_node->nodename = util_strsav(node_long_name(node));
        topol_node->index    = i;
        assert(!st_insert(topol_table, node_long_name(node), (char *) topol_node));
    }
    array_free(nodevec);
    return topol_table;
}


/*----------------------------------------------------------------------
  Given a node, returns in the topol_fanin_array, all the fanins of the
  node sorted in a topological order (from inputs).
-----------------------------------------------------------------------*/
array_t *
act_topol_sort_fanins(node, topol_table, network)
        node_t *node;
        st_table *topol_table;
        network_t *network;
{
    int          i, size;
    node_t       *fanin;
    array_t      *topol_fanin_array;
    array_t      *temp_array;
    TOPOL_STRUCT *topol_fanin;
    int act_topol_compare_function();

    topol_fanin_array = array_alloc(node_t * , 0);
    temp_array        = array_alloc(TOPOL_STRUCT * , 0);

    /* get the topol_fanin for all the fanins and sort them */
    /*------------------------------------------------------*/
    foreach_fanin(node, i, fanin)
    {
        assert(st_lookup(topol_table, node_long_name(fanin), (char **) &topol_fanin));
        array_insert_last(TOPOL_STRUCT * , temp_array, topol_fanin);
    }
    array_sort(temp_array, act_topol_compare_function);

    /* put the sorted fanins in the topol_fanin_array */
    /*------------------------------------------------*/
    size   = array_n(temp_array);
    for (i = 0; i < size; i++) {
        topol_fanin = array_fetch(TOPOL_STRUCT * , temp_array, i);
        fanin       = network_find_node(network, topol_fanin->nodename);
        array_insert_last(node_t * , topol_fanin_array, fanin);
    }
    array_free(temp_array);
    return topol_fanin_array;
}

/*-------------------------------------------------------------------
  Small index comes towards beginning of the array.
--------------------------------------------------------------------*/
int
act_topol_compare_function(ts1, ts2)
        TOPOL_STRUCT **ts1, **ts2;
{

    int index1, index2;

    index1 = (*ts1)->index;
    index2 = (*ts2)->index;

    if (index1 > index2) return 1;
    if (index2 > index1) return (-1);
    return 0;
}

void
act_delete_topol_table_entries(topol_table)
        st_table *topol_table;
{
    st_foreach(topol_table, act_delete_topol_table_entry, NIL(
    char));
}

enum st_retval
act_delete_topol_table_entry(key, value, arg)
        char *key, *value, *arg;
{
    free_topol_struct((TOPOL_STRUCT *) value);
    return ST_DELETE;
}

free_topol_struct(topol_node)
TOPOL_STRUCT *topol_node;
{
FREE(topol_node
->nodename);
FREE(topol_node);
}

