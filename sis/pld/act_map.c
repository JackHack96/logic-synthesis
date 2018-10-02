/*  April 4, 1991 - added act_is_or_used */
/*  May 7, 1991   - added init_param to make routines look cleaner
                  - fixed bdnet file generation
                  - moved some stuff over to act_util.c  */
/* Aug 18, 1991   - fixed another bdnet generation bug - driving a node by
                    two outputs */

#include "pld_int.h"
#include "sis.h"

static int map_act_delay();

array_t
           *multiple_fo_array; /* holds the nodes that have multiple_fo in the act */
array_t    *num_mux_struct_array; /* holds the cost of each multiple_fo act */
array_t    *my_node_array;
array_t    *vertex_node_array; /* for node and its root bdd */
array_t
           *vertex_name_array; /* for vertex's association with name of correct node */
int        WHICH_ACT;
ACT_VERTEX *PRESENT_ACT; /* for temporary storage */
int        MARK_VALUE, MARK_COMPLEMENT_VALUE;
int        num_or_patterns; /* number of blocks that use the OR_gate */
static int instance_num;
extern int act_is_or_used;

network_t *act_map_network(network_t *network, act_init_param_t *init_param,
                           st_table *cost_table) {
    node_t          *node;
    node_function_t node_fun;
    COST_STRUCT     *cost_node;
    char            *name;
    int             i, size;
    int             cost_network;
    array_t         *nodevec;
    array_t         *delay_values; /* for delay values */

    if (init_param->DISJOINT_DECOMP)
        decomp_disj_network(network);
    if (init_param->mode != AREA) {
        delay_values = act_read_delay(init_param->delayfile);
    }

    nodevec = network_dfs(network);
    size    = array_n(nodevec);
    for (i  = 0; i < size; i++) {
        node     = array_fetch(node_t *, nodevec, i);
        node_fun = node_function(node);
        if ((node_fun != NODE_PI) && (node_fun != NODE_PO)) { /* node is INTERNAL */
            if ((node_fun == NODE_0) || (node_fun == NODE_1)) {
                cost_node = ALLOC(COST_STRUCT, 1);
                cost_node->node                  = node;
                cost_node->arrival_time          = (double) 0.0;
                cost_node->cost_and_arrival_time = (double) (-1.0);
                cost_node->cost                  = 0;
                cost_node->act                   = NIL(ACT_VERTEX);
            } else {
                if (init_param->mode == AREA) {
                    cost_node = act_evaluate_map_cost(node, init_param, NIL(network_t),
                                                      NIL(array_t), NIL(st_table));
                } else {
                    cost_node = act_evaluate_map_cost(node, init_param, network,
                                                      delay_values, cost_table);
                }
            }
            name = util_strsav(node_long_name(node));
            assert(!st_insert(cost_table, name, (char *) cost_node));
        } /* if INTERNAL */
        else {
            /* set the arrival time of PI's if DELAY mode */
            /*--------------------------------------------*/
            if ((node_fun == NODE_PI) && (init_param->mode != AREA))
                (void) act_set_pi_arrival_time_node(node, cost_table);
        }
    }
    array_free(nodevec);

    if (init_param->mode == AREA) {
        iterative_improvement(network, cost_table, init_param);
        if (init_param->LAST_GASP) {
            network = act_network_remap(network, cost_table);
        }
        cost_network = print_network(network, cost_table, -1);
    }
    if (ACT_STATISTICS)
        (void) printf(" block count: %d\n", cost_network);
    assert(network_check(network));
    if (init_param->BREAK) {
        network = act_break_network(network, cost_table);
        if ((ACT_STATISTICS) || (ACT_DEBUG))
            (void) printf(" number of blocks using OR gate = %d\n", num_or_patterns);
        /* if (ACT_DEBUG) act_final_check_network(network, cost_table); */
    }
    if (init_param->mode != AREA) {
        if (init_param->BREAK) {
            act_delay_iterative_improvement(network, cost_table, delay_values,
                                            init_param);
        }
        array_free(delay_values);
    }
    return network;
}

/*******************************************************************************************************************
 improve the network using repeated partial collapse and decomp till NUM_ITER
are done, or no further improvement. then do a quick-phase to determine which
nodes should be realized as complemented.
********************************************************************************************************************/

static void iterative_improvement(network_t *network, st_table *cost_table,
                                  act_init_param_t *init_param) {
    if (init_param->NUM_ITER > 0)
        improve_network(network, cost_table, init_param);
    if (init_param->QUICK_PHASE)
        act_quick_phase(network, cost_table, init_param);
    if (ACT_DEBUG)
        assert(network_check(network));
}

/****************************************************************************
act_evaluate_map_cost() - summary
It maps the given node onto the actel architecture. Preferably,
   the network given could be the one with every node having fanin less than
   equal to 6- the maximum limit upto which the act generated for each node is
   optimal. Then for the node, the act is generated;
   at multiple fanout points in the act,
   the act is broken and a set of acts is
   constructed. For each small act(a tree), the pattern_match algorithm is
   used to compute the cost of the act in terms of the mux structure.
COMMENTS: 1) network, delay_values, cost_table are all NIL if init_param->mode
is AREA. These are needed only if init_param->mode is DELAY. 2) Returns
COST_STRUCT * corresponding to the node, but does not insert it in the
cost_table.
*****************************************************************************/

COST_STRUCT *act_evaluate_map_cost(node_t *node, act_init_param_t *init_param,
                                   network_t *network, array_t *delay_values,
                                   st_table *cost_table) {
    ACT_VERTEX_PTR act_of_node, act1_of_node, act2_of_node;
    COST_STRUCT    *cost_node, *cost_node1, *cost_node2;

    if (ACT_DEBUG)
        node_print(stdout, node);
    if ((node_num_literal(node) == node_num_cube(node)) ||
        (node_num_cube(node) == 1)) {
        act_of_node = my_create_act(node, init_param->mode, cost_table, network,
                                    delay_values);
        act_of_node->my_type = ORDERED;
        WHICH_ACT = ORDERED;
        act_of_node->node = node;
        if (init_param->mode == AREA)
            cost_node = make_tree_and_map(node, act_of_node);
        else
            cost_node =
                    make_tree_and_map_delay(node, act_of_node, network, delay_values,
                                            cost_table, init_param->mode);
        act_act_free(node);
        return cost_node;
    }

    switch (init_param->HEURISTIC_NUM) {
        case 1:
            act_of_node = my_create_act(node, init_param->mode, cost_table, network,
                                        delay_values);
            WHICH_ACT   = act_of_node->my_type   = ORDERED;
            act_of_node->node = node;
            if (init_param->mode == AREA)
                cost_node = make_tree_and_map(node, act_of_node);
            else
                cost_node =
                        make_tree_and_map_delay(node, act_of_node, network, delay_values,
                                                cost_table, init_param->mode);
            act_act_free(node);
            return cost_node;
        case 2:make_act(node);
            ACT_SET(node)->LOCAL_ACT->act->node_list = NIL(array_t);
            act_of_node = ACT_SET(node)->LOCAL_ACT->act->root;
            WHICH_ACT   = act_of_node->my_type   = UNORDERED;
            act_of_node->node = node;
            if (init_param->mode == AREA)
                cost_node = make_tree_and_map(node, act_of_node);
            else
                cost_node =
                        make_tree_and_map_delay(node, act_of_node, network, delay_values,
                                                cost_table, init_param->mode);
            act_act_free(node);
            return cost_node;
        case 3:
            if ((node_num_fanin(node) <= MAXOPTIMAL) ||
                (node_num_literal(node) == node_num_fanin(node))) {
                act_of_node = my_create_act(node, init_param->mode, cost_table, network,
                                            delay_values);
                WHICH_ACT   = act_of_node->my_type = ORDERED;
                act_of_node->node = node;
            } else {
                make_act(node);
                ACT_SET(node)->LOCAL_ACT->act->node_list = NIL(array_t);
                act_of_node = ACT_SET(node)->LOCAL_ACT->act->root;
                WHICH_ACT   = act_of_node->my_type = UNORDERED;
                act_of_node->node = node;
            }
            if (init_param->mode == AREA)
                cost_node = make_tree_and_map(node, act_of_node);
            else
                cost_node =
                        make_tree_and_map_delay(node, act_of_node, network, delay_values,
                                                cost_table, init_param->mode);
            act_act_free(node);
            return cost_node;
        case 4:make_act(node);
            ACT_SET(node)->LOCAL_ACT->act->node_list = NIL(array_t);
            act2_of_node = ACT_SET(node)->LOCAL_ACT->act->root;
            WHICH_ACT    = act2_of_node->my_type = UNORDERED;
            act2_of_node->node = node;
            if (init_param->mode == AREA)
                cost_node2 = make_tree_and_map(node, act2_of_node);
            else
                cost_node2 =
                        make_tree_and_map_delay(node, act2_of_node, network, delay_values,
                                                cost_table, init_param->mode);
            act_act_free(node);
            if (cost_node2->cost <= 1) {
                return cost_node2;
            }
            act1_of_node = my_create_act(node, init_param->mode, cost_table, network,
                                         delay_values);
            WHICH_ACT    = act1_of_node->my_type = ORDERED;
            act1_of_node->node = node;
            if (init_param->mode == AREA)
                cost_node1 = make_tree_and_map(node, act1_of_node);
            else
                cost_node1 =
                        make_tree_and_map_delay(node, act1_of_node, network, delay_values,
                                                cost_table, init_param->mode);
            act_act_free(node);
            if (init_param->mode == AREA) {
                if (cost_node1->cost <= cost_node2->cost) {
                    (void) free_cost_struct(cost_node2);
                    return cost_node1;
                }
                (void) free_cost_struct(cost_node1);
                return cost_node2;
            } else {
                if (cost_node1->arrival_time <= cost_node2->arrival_time) {
                    (void) free_cost_struct(cost_node2);
                    return cost_node1;
                }
                (void) free_cost_struct(cost_node1);
                return cost_node2;
            }
        default:
            (void) printf(
                    "Error: act_map_evaluate(): %d is not a valid heuristic number\n",
                    init_param->HEURISTIC_NUM);
            exit(1);
    }
    return (NIL(COST_STRUCT));
}

/************************************************************************************
  Given a node and its subject_graph (ACT, or OACT), snap at multiple fanout
points and maps each tree using the pattern-set.
*************************************************************************************/

COST_STRUCT *make_tree_and_map(node_t *node, ACT_VERTEX_PTR act_of_node) {
    int            num_acts;
    int            i;
    int            num_mux_struct;
    int            TOTAL_MUX_STRUCT; /* cost of the network */
    COST_STRUCT    *cost_node;
    ACT_VERTEX_PTR act;

    TOTAL_MUX_STRUCT = 0;
    cost_node        = ALLOC(COST_STRUCT, 1);

    act_init_multiple_fo_array(act_of_node); /* insert the root */
    act_initialize_act_area(act_of_node, multiple_fo_array);
    /* initialize the required structures */
    /* if (ACT_DEBUG) my_traverse_act(act_of_node);
if (ACT_DEBUG) print_multiple_fo_array(multiple_fo_array); */
    num_acts = array_n(multiple_fo_array);
    /* changed the following traversal , now  bottom-up */
    /*--------------------------------------------------*/
    for (i   = num_acts - 1; i >= 0; i--) {
        act            = array_fetch(ACT_VERTEX_PTR, multiple_fo_array, i);
        PRESENT_ACT    = act;
        num_mux_struct = map_act(act);
        TOTAL_MUX_STRUCT += num_mux_struct;
    }
    if (ACT_DEBUG) {
        (void) printf(
                "total mux_structures used for the node %s = %d, arrival_time = %f\n",
                node_long_name(node), TOTAL_MUX_STRUCT, act_of_node->arrival_time);
    }
    array_free(multiple_fo_array);
    cost_node->cost         = TOTAL_MUX_STRUCT;
    cost_node->arrival_time = act_of_node->arrival_time;
    cost_node->node         = node;
    cost_node->act          = act_of_node;
    return cost_node;
}

/***************************************************************************************
 Creates OACT for a given node. If the node has <= MAXOPTIMAL fanins, optimal
act (with min. number of vertices would be constructed. COMMENTS: If mode is
AREA, cost_table is actually NIL. Not using it then.
****************************************************************************************/

ACT_VERTEX_PTR
my_create_act(node_t *node, float mode, st_table *cost_table,
              network_t *network, array_t *delay_values) {
    array_t *OR_literal_order();
    array_t *single_cube_order();
    array_t        *nodevec;
    array_t        *order_list;
    array_t        *decomp_array;
    node_t         *n, *fanin;
    node_t         *node_of_cube;
    node_cube_t    cube;
    int            size;
    int            num_cube_n, num_literal_n;
    int            i;
    ACT_VERTEX_PTR act;
    int            act_constructed; /* in case number of cubes = 2 */
    int            num_cubes;
    int            num_literals;
    int            num_fanins;
    array_t        *cube_order_list, *order_list_new;
    extern ACT_VERTEX *p_act_construct();

    act_constructed = 0;

    /* if mode is not AREA, arrival times of inputs determine the order_list.  */
    /*----------------------------------------------------------------------*/
    if (mode != AREA) {
        nodevec = array_alloc(node_t *, 0);
        array_insert_last(node_t *, nodevec, node);
        if (node_num_fanin(node) <= MAXOPTIMAL) {
            p_actCreate4Set(nodevec, NIL(array_t), 1, OPTIMAL, mode, network,
                            delay_values, cost_table);
            act = ACT_SET(node)->LOCAL_ACT->act->root;
            if (ACT_SET(node)->LOCAL_ACT->act->node_list == NIL(array_t)) {
                (void) printf(" Error: my_create_act(): optimal order list not\
					returned\n");
                exit(1);
            }
            put_node_names_in_act(ACT_SET(node)->LOCAL_ACT->act->root,
                                  ACT_SET(node)->LOCAL_ACT->act->node_list);
        } else {
            order_list = act_order_for_delay(node, cost_table);
            act        = (ACT_VERTEX *) p_act_construct(node, order_list, 1);
            put_node_names_in_act(ACT_SET(node)->LOCAL_ACT->act->root, order_list);
            array_free(order_list);
            ACT_SET(node)->LOCAL_ACT->act->node_list = NIL(array_t);
        }
        array_free(nodevec);
        return act;
    }
    /* mode = AREA: minimizing the number of basic blocks now.
if the number of cubes is 1, then make sure that the bottom ones are
not inverted. If the number is 2 and the possibility that there
is a factor of type (a+b), then order a,b towards the end(bottom) */
    /*-------------------------------------------------------------------*/
    num_cubes    = node_num_cube(node);
    if (num_cubes == 0) {
        nodevec = array_alloc(node_t *, 0);
        array_insert_last(node_t *, nodevec, node);
        p_actCreate4Set(nodevec, NIL(array_t), 1, OPTIMAL, (float) 0.0,
                        NIL(network_t), NIL(array_t), NIL(st_table));
        act = ACT_SET(node)->LOCAL_ACT->act->root;
        array_free(nodevec);
        return act;
    }
    num_literals = node_num_literal(node);
    num_fanins   = node_num_fanin(node);
    if (num_cubes == num_literals) {
        if (num_fanins == num_literals) {
            order_list                               = OR_literal_order(node);
            act                                      = (ACT_VERTEX *) p_act_construct(node, order_list, 1);
            put_node_names_in_act(ACT_SET(node)->LOCAL_ACT->act->root, order_list);
            array_free(order_list);
            ACT_SET(node)->LOCAL_ACT->act->node_list = NIL(array_t);
            return act;
        } else {
            act = my_create_act_general(node);
            return act;
        }
    }

    /* take into account the fact that OR gate is there for single cube*/
    /*-----------------------------------------------------------------*/
    if (num_cubes == 1) {
        if (num_fanins == num_literals) {
            order_list = single_cube_order(node);
            act        = p_act_construct(node, order_list, 1);
            put_node_names_in_act(ACT_SET(node)->LOCAL_ACT->act->root, order_list);
            array_free(order_list);
            ACT_SET(node)->LOCAL_ACT->act->node_list = NIL(array_t);
            return act;
        } else {
            act = my_create_act_general(node);
            return act;
        }
    }

    /* if num_literals = num_fanin_node, then it means it is a
disjoint support tree, then the ordering is obtained by
concatenating the optimal ordering of the individual cubes.
ordering of the individual cubes is carried on in the same way as
ordering of the nodes with a single cube. case like f = ab + c'    */
    /*-----------------------------------------------------------------*/
    if (num_literals == num_fanins) {
        order_list = array_alloc(node_t *, 0);
        for (i     = num_cubes - 1; i >= 0; i--) {
            cube            = node_get_cube(node, i);
            node_of_cube    = pld_make_node_from_cube(node, cube);
            cube_order_list = single_cube_order(node_of_cube);
            node_free(node_of_cube);
            order_list_new = array_join(order_list, cube_order_list);
            array_free(order_list);
            array_free(cube_order_list);
            order_list = array_dup(order_list_new);
            array_free(order_list_new);
        }
        act        = p_act_construct(node, order_list, 1);
        put_node_names_in_act(ACT_SET(node)->LOCAL_ACT->act->root, order_list);
        array_free(order_list);
        ACT_SET(node)->LOCAL_ACT->act->node_list = NIL(array_t);
        return act;
    }

    /* if num_cubes == 2, then check for a special condition -  basically a filter
   */
    /*-----------------------------------------------------------------------------*/
    if (num_cubes == 2) {
        decomp_array = decomp_good(node);
        size         = array_n(decomp_array);
        if (size == 2) {
            n             = array_fetch(node_t *, decomp_array, 1);
            num_cube_n    = node_num_cube(n);
            num_literal_n = node_num_literal(n);
            if ((num_cube_n == num_literal_n) && (all_fanins_positive(n))) {
                order_list                               = array_alloc(node_t *, 0);
                act_put_nodes(array_fetch(node_t *, decomp_array, 0), n, order_list);
                for (i = num_literal_n - 1; i >= 0; i--) {
                    fanin = node_get_fanin(n, i);
                    array_insert_last(node_t *, order_list, fanin);
                }
                act_constructed = 1;
                act             = p_act_construct(node, order_list, 1);
                put_node_names_in_act(ACT_SET(node)->LOCAL_ACT->act->root, order_list);
                array_free(order_list);
                ACT_SET(node)->LOCAL_ACT->act->node_list = NIL(array_t);
            }
        }
        for (i = 0; i < size; i++) {
            node_free(array_fetch(node_t *, decomp_array, i));
        }
        array_free(decomp_array);
        if (act_constructed)
            return act;
    } /* if num_cubes == 2 */

    /* create general act */
    /*--------------------*/
    act = my_create_act_general(node);
    return act;
}

static ACT_VERTEX_PTR my_create_act_general(node_t *node) {
    array_t        *nodevec;
    ACT_VERTEX_PTR act;
    array_t        *order_list, *order_list1;
    array_t        *pld_order_nodes(), *act_get_corr_fanin_nodes();
    extern ACT_VERTEX *p_act_construct();
    node_t         *po1, *node1;
    network_t      *network1;

    /* if number of fanins is leq than MAXOPTIMAL, construct an optimal ACT for
   * delay */
    /*--------------------------------------------------------------------------------*/
    if (node_num_fanin(node) <= MAXOPTIMAL) {
        nodevec = array_alloc(node_t *, 0);
        array_insert_last(node_t *, nodevec, node);
        p_actCreate4Set(nodevec, NIL(array_t), 1, OPTIMAL, (float) 0.0,
                        NIL(network_t), NIL(array_t), NIL(st_table));
        act = ACT_SET(node)->LOCAL_ACT->act->root;
        if (ACT_SET(node)->LOCAL_ACT->act->node_list == NIL(array_t)) {
            (void) printf(" Error: my_create_act_general(): optimal order list not\
				returned\n");
            exit(1);
        }
        put_node_names_in_act(ACT_SET(node)->LOCAL_ACT->act->root,
                              ACT_SET(node)->LOCAL_ACT->act->node_list);
        array_free(nodevec);
        return act;
    }
    /* create a network in terms of fanins only */

    network1 = network_create_from_node(node);
    po1      = network_get_po(network1, 0);
    node1    = node_get_fanin(po1, 0);
    nodevec  = array_alloc(node_t *, 0);
    array_insert_last(node_t *, nodevec, node1);
    order_list1 = pld_order_nodes(nodevec, 1); /* just PI */
    order_list  = act_get_corr_fanin_nodes(node, order_list1);
    array_free(order_list1);
    array_free(nodevec);
    network_free(network1);
    /* order_list = pld_order_nodes(nodevec, 0); */ /* include internal nodes also
                                                   */

    act = p_act_construct(node, order_list, 1); /* local ACT */
    put_node_names_in_act(ACT_SET(node)->LOCAL_ACT->act->root, order_list);
    array_free(order_list);
    ACT_SET(node)->LOCAL_ACT->act->node_list = NIL(array_t);
    return act;
}

/******************************************************************************
  given a node, checks if all the fanins have a positive phase.
  Returns 1 if so, else returns 0.
******************************************************************************/

static int all_fanins_positive(node_t *node) {

    int           num_fanin;
    node_t        *fanin;
    input_phase_t phase;
    int           i;

    num_fanin = node_num_fanin(node);
    for (i    = num_fanin - 1; i >= 0; i--) {
        fanin = node_get_fanin(node, i);
        phase = node_input_phase(node, fanin);
        if (phase != POS_UNATE)
            return 0;
    }
    return 1;
}

/*************************************************************
  Initializes the array for the multiple_fo_vertices in the
  act for node under consideration. The first entry is the
  root itself, although it is not multiple fan_out vertex.
************************************************************/

void act_init_multiple_fo_array(ACT_VERTEX_PTR act_of_node) {
    multiple_fo_array = array_alloc(ACT_VERTEX_PTR, 0);
    array_insert_last(ACT_VERTEX_PTR, multiple_fo_array, act_of_node);
}

/***************************************************************************
   Annotates the act vertex with the name of the corresponding index-node.
****************************************************************************/
void put_node_names_in_act(ACT_VERTEX_PTR vertex, array_t *list) {
    node_t *node;

    if (vertex->mark == 0)
        vertex->mark = 1;
    else
        vertex->mark = 0;

    if (vertex->value != 4)
        return;
    node = array_fetch(node_t *, list, vertex->index);
    vertex->name = node_long_name(node);
    if (vertex->mark != vertex->low->mark) /* low child already visited */
        put_node_names_in_act(vertex->low, list);
    if (vertex->mark != vertex->high->mark)
        put_node_names_in_act(vertex->high, list);
}

/***********************************************************************
   Returns the cost of mapping a act vertex onto the patterns
   The cost of 1 mux structure is 1, hence in all configurations the mux
   is equally costly. This results in a very simple method of mapping.
   The function looks at the root of the tree_act, looks at two levels below
   and calculates the cost of the children there, if any, and adds one
   to the cost
***********************************************************************/
int map_act(ACT_VERTEX_PTR vertex) {

    int TOTAL_PATTERNS = 12;
    int pat_num;
    int cost[12]; /* for each of the 12 patterns derived from the mux struct */
    int vlow_cost;
    int vhigh_cost;
    int vlowlow_cost;
    int vlowhigh_cost;
    int vhighlow_cost;
    int vhighhigh_cost;
    int best_pat_num;

    /* if cost already computed, don't compute again */
    /*-----------------------------------------------*/
    if (vertex->mapped)
        return vertex->cost;

    /* if multiple fanout (but not the root of the present multiple_fo vertex)
return 0, but do not set any value, as this vertex would be/ or was
visited earlier as a root. Basically zero is not its true cost. So do not set
vertex->mapped = 1 also */

    if ((vertex->multiple_fo != 0) && (vertex != PRESENT_ACT)) {
        vertex->cost = 0;
        return 0;
    }

    vertex->mapped = 1;

    if (vertex->value != 4) {
        vertex->cost = 0;
        return 0;
    }

    /* if a simple input, cost = 0 */
    /*-----------------------------*/
    if ((vertex->low->value == 0) && (vertex->high->value == 1)) {
        vertex->cost = 0;
        return 0;
    }

    /* if or gate not used, just used first 4 patterns */
    /*-------------------------------------------------*/
    if (!act_is_or_used)
        TOTAL_PATTERNS = 4;

    /* initialize cost vector to a very high value for all the patterns*/
    /*-----------------------------------------------------------------*/
    for (pat_num = 0; pat_num < TOTAL_PATTERNS; pat_num++) {
        cost[pat_num] = MAXINT;
    }

    /***********recursively calculate relevant costs *************/

    if (vertex->low->multiple_fo != 0)
        vlow_cost = 0;
    else
        vlow_cost = map_act(vertex->low);

    if (vertex->high->multiple_fo != 0)
        vhigh_cost = 0;
    else
        vhigh_cost = map_act(vertex->high);

    if (vertex->low->value == 4) {
        if (vertex->low->multiple_fo != 0)
            vlowlow_cost = vlowhigh_cost = MAXINT;
        else {
            if (vertex->low->low->multiple_fo != 0)
                vlowlow_cost = 0;
            else
                vlowlow_cost  = map_act(vertex->low->low);
            if (vertex->low->high->multiple_fo != 0)
                vlowhigh_cost = 0;
            else
                vlowhigh_cost = map_act(vertex->low->high);
        }
    }

    if (vertex->high->value == 4) {
        if (vertex->high->multiple_fo != 0)
            vhighlow_cost = vhighhigh_cost = MAXINT;
        else {
            if (vertex->high->low->multiple_fo != 0)
                vhighlow_cost = 0;
            else
                vhighlow_cost  = map_act(vertex->high->low);
            if (vertex->high->high->multiple_fo != 0)
                vhighhigh_cost = 0;
            else
                vhighhigh_cost = map_act(vertex->high->high);
        }
    }

    /**********enumerate possible cases************************/

    /* l.c. (left child) + r.c.(right child) + 1 */
    /*-------------------------------------------*/
    cost[0] = vlow_cost + vhigh_cost + 1;

    if (vertex->low->value == 4)
        cost[1] = vlowlow_cost + vlowhigh_cost + vhigh_cost + 1;

    if (vertex->high->value == 4)
        cost[2] = vlow_cost + vhighlow_cost + vhighhigh_cost + 1;

    if ((vertex->low->value == 4) && (vertex->high->value == 4))
        cost[3] = vlowlow_cost + vlowhigh_cost + vhighlow_cost + vhighhigh_cost + 1;
    if (!act_is_or_used) {
        /************find minimum cost************************/
        /*---------------------------------------------------*/
        best_pat_num = minimum_cost_index(cost, TOTAL_PATTERNS);
        vertex->cost        = cost[best_pat_num];
        vertex->pattern_num = best_pat_num;
        /* if (ACT_DEBUG)
(void) printf("vertex id = %d, index = %d, cost = %d, name = %s,
pattern_num = %d\n", vertex->id, vertex->index, vertex->cost,
vertex->name, vertex->pattern_num);*/
        return vertex->cost;
    }

    /* OR patterns - pretty involved -> for more explanation, see doc. */
    /*-----------------------------------------------------------------*/

    if ((OR_pattern(vertex)) && (vertex->low->low->multiple_fo == 0) &&
        (vertex->low->low->value == 4))
        cost[4] =
                map_act(vertex->low->low->low) + map_act(vertex->low->low->high) + 1;

    if ((OR_pattern(vertex->high)) && (vertex->high->multiple_fo == 0)) {
        if (vertex->low->value == 0)
            cost[5] =
                    map_act(vertex->high->high) + map_act(vertex->high->low->low) + 1;
        else {
            if ((vertex->low->value == 1) && (vertex->high->high->value == 0) &&
                (vertex->high->low->low->value == 1))
                cost[6] = 1;
        }
    }

    if ((OR_pattern(vertex->low)) && (vertex->low->multiple_fo == 0)) {
        if (vertex->high->value == 0)
            cost[7] = map_act(vertex->low->high) + map_act(vertex->low->low->low) + 1;
    }

    /************find minimum cost************************/
    best_pat_num = minimum_cost_index(cost, TOTAL_PATTERNS);
    vertex->cost        = cost[best_pat_num];
    vertex->pattern_num = best_pat_num;
    /* if (ACT_DEBUG)
(void) printf("vertex id = %d, index = %d, cost = %d, name = %s,
pattern_num = %d\n",
           vertex->id, vertex->index, vertex->cost, vertex->name,
vertex->pattern_num);*/
    return vertex->cost;
}

/**************************************************************************
   if an OR pattern (or NOR pattern) is rooted at the root, return 1, else
   return 0
***************************************************************************/
static int OR_pattern(ACT_VERTEX_PTR root) {
    if ((root->value == 4) && (root->low->multiple_fo == 0) &&
        (root->low->value == 4) && (root->high == root->low->high))
        return 1;

    return 0;
}

/************************************************************************
 returns the index of the pattern which results in the minimum cost
 at the present node.
*************************************************************************/
static int minimum_cost_index(int cost[], int num_entries) {

    int i;
    int min_index = 0;

    for (i = 1; i < num_entries; i++) {
        if (cost[i] < cost[min_index])
            min_index = i;
    }
    return min_index;
}

static void improve_network(network_t *network, st_table *cost_table,
                            act_init_param_t *init_param) {

    int         nogain;
    network_t   *dup_net;
    node_t      *dup_node;
    lsGen       genfo;
    node_t      *node, *fanout;
    /* node_t *fanout_simpl; */
    COST_STRUCT ***focost;
    int         **focost_ind;
    char        *dummy;
    char        buf[BUFSIZE]; /* to hold the system command */
    int         i, j;
    array_t     *nodevec;
    st_table    *node2num_table;
    int         size_net;
    int         clust_num;
    int         numnodes; /*number of internal nodes */

    int **M;             /* coeff. for Integer Programming formulation */
    int *num_ones;       /* number of ones in a row of M[][] */
    int *g;              /*gain for a cluster*/
    int *collapsed_node; /* for a cluster, stores the id of the
                  collapsed node */

    int *variable; /* to hold the value of the variables after IP */

    int iteration_gain; /* tells the gain at the end of present iteration */
    int gain;           /* gain for the node being collapsed */
    int decomp_gain;    /* gain for network_decomposition at an iteration */
    int collapse_gain;  /*gain from the collapse operation */
    int old_cost;       /* old_cost of a fan_out node */
    int cost_network;   /* cost of the network in terms of mux_structs*/

    int num_fo; /* number of fan_outs of a ndoe */
    int clust_num_add1;

    char            *lindopathname;
    char            *name, *name1;
    char            *infile, *outfile;
    node_function_t node_fun;
    int             num_fanouts;
    st_table        *fo_table;
    int             iteration_num;
    int             sfd;

    nogain        = FALSE;
    iteration_num = 0;

    while (nogain == FALSE) {

        cost_network = print_network(network, cost_table, iteration_num);

        /* decomposing big nodes in the network if it is profitable*/
        /*---------------------------------------------------------*/

        decomp_big_nodes(network, cost_table, &decomp_gain, init_param);

        /* check */
        /*-------*/
        assert(network_check(network));

        if (ACT_DEBUG && (decomp_gain > 0))
            (void) print_network(network, cost_table, iteration_num);
        iteration_gain = decomp_gain;
        iteration_num++;
        if (iteration_num > init_param->NUM_ITER)
            return;
        nogain    = TRUE;
        clust_num = -1;

        /* check if collapsing is desired. If not, continue if gain sufficient */
        /*---------------------------------------------------------------------*/

        if (init_param->FANIN_COLLAPSE == 0) {
            if (iteration_gain >= (init_param->GAIN_FACTOR * cost_network)) {
                nogain = FALSE;
            }
            continue;
        }

        /* search for lindo in the path name. If not found, do not
do collapsing */
        /*--------------------------------------------------------*/
        lindopathname = util_path_search("lindo");
        if (lindopathname == NIL(char)) {
            if (ACT_DEBUG) {
                (void) printf("YOU DO NOT HAVE LINDO INTEGER PROGRAMMING PACKAGE\n");
                (void) printf("\tIN YOUR PATH.\n");
                (void) printf("CONTINUING WITHOUT LINDO.....\n");
                (void) printf("PLEASE OBTAIN LINDO FROM\n");

                (void) printf("The Scientific Press, 540 University Ave.\n");
                (void) printf("Palo Alto, CA 94301, USA\n");
                (void) printf("TEL : (415) 322-5221\n");
            }
            /* Adding this on Jan. 24, 91 */
            /*----------------------------*/
            collapse_gain =
                    act_partial_collapse_without_lindo(network, cost_table, init_param);
            iteration_gain = collapse_gain + decomp_gain;

            if (iteration_gain >= (init_param->GAIN_FACTOR * cost_network)) {
                nogain = FALSE;
            }
            continue;
        }
        FREE(lindopathname);

        /*Initialization */
        /*--------------*/
        numnodes = network_num_internal(network) + network_num_po(network) +
                   network_num_pi(network);

        focost         = ALLOC(COST_STRUCT **, numnodes);
        focost_ind     = ALLOC(int *, numnodes);
        M              = ALLOC(int *, numnodes);
        num_ones       = ALLOC(int, numnodes);
        g              = ALLOC(int, numnodes);
        collapsed_node = ALLOC(int, numnodes);
        variable       = ALLOC(int, numnodes);

        for (i = 0; i < numnodes; i++) {
            focost[i]     = ALLOC(COST_STRUCT *, numnodes);
            M[i]          = ALLOC(int, numnodes);
            focost_ind[i] = ALLOC(int, numnodes);
            for (j      = 0; j < numnodes; j++) {
                M[i][j]          = 0;
                focost_ind[i][j] = 0;
            }
            num_ones[i] = 0;
        }

        node2num_table = st_init_table(st_ptrcmp, st_ptrhash);

        nodevec  = network_dfs(network);
        size_net = array_n(nodevec);
        /* establish a link from node to its number */
        /*------------------------------------------*/
        for (i   = 0; i < size_net; i++) {
            node = array_fetch(node_t *, nodevec, i);
            (void) st_insert(node2num_table, node_long_name(node), (char *) i);
        }

        for (i = 0; i < size_net; i++) {
            node     = array_fetch(node_t *, nodevec, i);
            node_fun = node_function(node);
            if ((node_fun == NODE_PI) || (node_fun == NODE_PO))
                continue;
            if (node_num_fanin(node) > init_param->FANIN_COLLAPSE)
                continue;
            if (is_anyfo_PO(node)) {
                continue;
            }
            name     = node_long_name(node);
            dup_net  = network_dup(network);
            dup_node = network_find_node(dup_net, name);
            if (dup_node == NIL(node_t)) {
                (void) printf("Error:dup_node not there %s\n", name);
                exit(1);
            }
            name1 = node_long_name(node);
            assert(st_lookup(cost_table, name1, &dummy));

            /* compute the gain if the node "dup_node" is collapsed into each of its
fanouts and is then removed from the network. The cost is
stored in the focost for each of the fanouts of the node "dup_node" */
            /*------------------------------------------------------------------------*/
            gain           = ((COST_STRUCT *) dummy)->cost;
            num_fo         = 0;
            clust_num_add1 = clust_num + 1;

            /* for each fanout, recompute the cost */
            /*-------------------------------------*/
            foreach_fanout(dup_node, genfo, fanout) {
                (void) node_collapse(fanout, dup_node);
                focost[clust_num_add1][num_fo] = act_evaluate_map_cost(
                        fanout, init_param, NIL(network_t), NIL(array_t), NIL(st_table));
                /* put the names in act vertices now */
                /*-----------------------------------*/
                act_partial_collapse_update_act_fields(network, fanout,
                                                       focost[clust_num_add1][num_fo]);
                focost_ind[clust_num_add1][num_fo] = 1;
                /* free the act structure of the fanout node, it has been copied into
         * the focost */
                /*-------------------------------------------------------------------------------*/
                assert(st_lookup(cost_table, node_long_name(fanout), &dummy));
                old_cost = ((COST_STRUCT *) dummy)->cost;
                gain     = gain + old_cost - (focost[clust_num_add1][num_fo]->cost);
                num_fo++;
            }

            if (gain > 0) {
                clust_num++;

                /* assign proper value to g and M for the cluster "clust_num" */
                /*------------------------------------------------------------*/
                collapsed_node[clust_num] = i;
                g[clust_num]              = gain;
                M[i][clust_num]           = 1; /* node (ie. with id i) in clust_num */
                num_ones[i]++;
                foreach_fanout(node, genfo, fanout) {
                    assert(st_lookup(node2num_table, node_long_name(fanout), &dummy));
                    M[(int) dummy][clust_num] = 1;
                    num_ones[(int) dummy]++;
                }
            } /* if (gain > 0) */

                /* if gain <= 0, free the storage in focost associated with node
"dup_node" */

            else {
                for (j = 0; j < num_fo; j++) {
                    (void) free_cost_struct(focost[clust_num_add1][j]);
                    focost_ind[clust_num_add1][j] = 0;
                }
                assert(st_lookup(cost_table, name1, &dummy));
            }
            /* free the duplicated node and the network */
            /*-------------------------------------------*/
            network_delete_node(dup_net, dup_node);
            network_free(dup_net);

        } /* for i = .. */

        /* if no cluster (ie with gain > 0) can be formed check here */
        /*-----------------------------------------------------------*/
        if (clust_num < 0) {
            if (ACT_DEBUG)
                (void) printf("***no improvement in collapse***\n");
            for (i = 0; i < numnodes; i++) {
                for (j = 0; j < numnodes; j++) {
                    if (focost_ind[i][j] == 1)
                        free_cost_struct(focost[i][j]);
                }
                FREE(focost[i]);
                FREE(focost_ind[i]);
                FREE(M[i]);
            }
            FREE(focost);
            FREE(focost_ind);
            FREE(variable);
            FREE(collapsed_node);
            FREE(g);
            FREE(num_ones);
            FREE(M);
            array_free(nodevec);
            st_free_table(node2num_table);
            if (iteration_gain < (init_param->GAIN_FACTOR * cost_network)) {
                if (ACT_DEBUG) {
                    (void) printf("****no significant improvement in iteration_gain***\n");
                    (void) printf("cost_network before iter. = %d, iteration_gain = %d\n",
                                  cost_network, iteration_gain);
                }
                return;
            }
            nogain = FALSE;
            continue;
        } /* if clust_num < 0 */

        /* else formulate the IP - using LINDO  */
        /*---------------------------------*/

        infile  = formulate_IP(nodevec, M, g, num_ones, clust_num, iteration_num);
        outfile = ALLOC(char, 100);
        (void) sprintf(outfile, "/usr/tmp/actel.lindo.out%d.XXXXXX", iteration_num);
        sfd = mkstemp(outfile);
        if (sfd != -1) {
            close(sfd);
            (void) sprintf(buf, "lindo < %s > %s", infile, outfile);

            (void) system(buf);

            /* read the solution and change the  network */
            /*-------------------------------------------*/
            read_LINDO_file(outfile, variable, &collapse_gain);
        }

        FREE(infile);
        FREE(outfile);
        iteration_gain = collapse_gain + decomp_gain;

        /* update the cost of affected nodes in the new network */
        /*------------------------------------------------------*/
        for (j = 0; j <= clust_num; j++) {
            if (variable[j] == 0)
                continue;
            node        = array_fetch(node_t *, nodevec, collapsed_node[j]);
            num_fanouts = node_num_fanout(node);
            fo_table    = st_init_table(strcmp, st_strhash);
            foreach_fanout(node, genfo, fanout) {
                /* get the correct focost[][] corresponding to fanout */
                /*----------------------------------------------------*/
                for (num_fo = 0; num_fo < num_fanouts; num_fo++) {
                    if (focost[j][num_fo]->node == fanout)
                        break;
                    assert(num_fo != (num_fanouts - 1));
                }
                assert(!st_insert(fo_table, node_long_name(fanout), (char *) num_fo));
            }
            foreach_fanout(node, genfo, fanout) {
                (void) node_collapse(fanout, node);
                /* fanout_simpl = node_simplify(fanout, NIL (node_t),
NODE_SIM_ESPRESSO); node_replace(fanout, fanout_simpl);
*/
                name1 = node_long_name(fanout);
                name  = ALLOC(char, (strlen(name1) + 1)); /* name needed */
                (void) strcpy(name, name1);
                assert(st_delete(cost_table, &name1, &dummy));
                (void) free_cost_struct((COST_STRUCT *) dummy);
                FREE(name1);

                /* checking */
                /*----------*/
                if (node_long_name(node) == NIL(char)) {
                    (void) printf("Error:improve_names---\n");
                    exit(1);
                }
                /* this focost is in the cost_table, unavailable for free.
Therefore set the indicator to 0. */
                /*-------------------------------------------------------*/
                assert(st_lookup_int(fo_table, node_long_name(fanout), &num_fo));
                focost_ind[j][num_fo] = 0;
                assert(!st_insert(cost_table, name, (char *) focost[j][num_fo]));
            } /* foreach_fanout */

            st_free_table(fo_table);
            /* delete the collapsed node */
            /*---------------------------*/
            name1 = node_long_name(node);
            /* delete the act storage from the struct */
            /*----------------------------------------*/
            assert(st_delete(cost_table, &name1, &dummy));
            (void) free_cost_struct((COST_STRUCT *) dummy);
            FREE(name1);

            /* checking */
            /*----------*/
            if (node_long_name(node) == NIL(char)) {
                (void) printf("Error:improve_names---\n");
                exit(1);
            }

            network_delete_node(network, node);

        } /* for j = loop */

        /* free storage */
        /*--------------*/
        for (i = 0; i < numnodes; i++) {
            for (j = 0; j < numnodes; j++)
                if (focost_ind[i][j] == 1)
                    free_cost_struct(focost[i][j]);
            FREE(focost[i]);
            FREE(focost_ind[i]);
            FREE(M[i]);
        }
        FREE(focost);
        FREE(focost_ind);
        FREE(M);
        FREE(variable);
        FREE(collapsed_node);
        FREE(g);
        FREE(num_ones);
        array_free(nodevec);
        st_free_table(node2num_table);
        if (iteration_gain >= (init_param->GAIN_FACTOR * cost_network))
            nogain = FALSE;
        if (ACT_DEBUG)
            (void) print_network(network, cost_table, iteration_num);

    } /* while (nogain == FALSE) */

    /* return network; */
}

/********************************************************************************
  Generates a file for the program lindo and solves the integer program,
  The solution of the integer program generates nodes that should be collapsed.
*********************************************************************************/
static char *formulate_IP(array_t *nodevec, int **M, int g[], int num_ones[],
                          int clust_num, int iteration_num) {

    int  i, j;
    int  num_constr;
    FILE *lindoFile;
    int  FLAG;
    int  termsPerLine = 5;
    int  numTerm;
    char *filename;
    int  net_size;
    int  fd;

    if (ACT_DEBUG)
        (void) printf("formulating IP - creating lindo file *****\n");
    filename = ALLOC(char, 100);
    (void) sprintf(filename, "/usr/tmp/actel.lindo.in%d.XXXXXX", iteration_num);
    fd = mkstemp(filename);
    if (fd == -1) {
        fprintf(stderr, "failed to open lindo file\n");
    }
    lindoFile = fdopen(fd, "w");

    /* generate objective function : MAX g[0]x0 + .... g[clust_num]xclust_num */
    /*------------------------------------------------------------------------*/
    (void) fprintf(lindoFile, "MAX ");
    for (numTerm = 0, i = 0; i <= clust_num; ++numTerm, ++i) {
        if (i == 0)
            (void) fprintf(lindoFile, "%d x0 ", g[0]);
        else {
            if (numTerm < termsPerLine)
                (void) fprintf(lindoFile, "+ %d x%d ", g[i], i);
            else {
                numTerm = 0;
                (void) fprintf(lindoFile, "\n + %d x%d ", g[i], i);
            }
        }
    }
    (void) fprintf(lindoFile, "\n");

    /* Generate constraints for every node in the network, only if at least
two entries for the corresponding row is 1(as told by num_ones.
constraint for a node i is of the form mi,1 x1 + mi,2 x2 + .... <= 1 */
    /*----------------------------------------------------------------------*/
    net_size   = array_n(nodevec);
    num_constr = 0;
    for (i     = 0; i < net_size; i++) {
        /* if num_ones = 1, obviously constraint would be satisfied - so do
not put the constraint at all */
        /*------------------------------------------------------------------*/
        if (num_ones[i] < 2)
            continue;
        num_constr++;
        if (num_constr == 1)
            (void) fprintf(lindoFile, "ST\n");

        numTerm = 0;
        FLAG    = FALSE;
        for (j  = 0; j <= clust_num; j++) {
            if (M[i][j] == 0)
                continue;
            if (FLAG == FALSE) {
                FLAG = TRUE;
                ++numTerm;
                (void) fprintf(lindoFile, "x%d ", j);
                continue;
            }
            if (numTerm < termsPerLine) {
                ++numTerm;
                (void) fprintf(lindoFile, "+ x%d ", j);
                continue;
            }
            numTerm = 1;
            (void) fprintf(lindoFile, "\n + x%d ", j);
        }
        (void) fprintf(lindoFile, "<= 1\n");
    } /* for i = .. */
    (void) fprintf(lindoFile, "END\n");

    /* declaration of integrality - integrality would confirm 0-1 IP for lindo */
    /*-------------------------------------------------------------------------*/

    for (j = 0; j <= clust_num; j++)
        (void) fprintf(lindoFile, "INTEGER x%d\n", j);

    (void) fprintf(lindoFile, "GO\n");
    (void) fprintf(lindoFile, "QUIT\n");
    (void) fclose(lindoFile);
    return filename;
}

/***************************************************************************
  reads the solution generated by the lindo from the file. Sets the
  corresponding "variable" values 1. Also reads the gain that resulted
  from the collapse operation for the network, stores it in iteration_gain.
    -"IP OPTIMUM" means that optimum solution was found.
    -"NO FEASIBLE" indicates that no feasible solution exists.
         (This normally should not be the case).
    -If none of the above two found, then the only other possibility is
      "NEW INTEGER", which means optimal answer could not be found,
      but some answer was found. So live with that, buddy. For this,
      however, the file is scanned again.
****************************************************************************/
static void read_LINDO_file(char *filename, int *variable,
                            int *iteration_gain) {
    FILE  *lindoFile;
    float f_gain;
    int   variable_id;
    float result;
    char  word[BUFSIZE];
    int   FLAG;

    FLAG = 0;

    lindoFile = fopen(filename, "r");
    while (TRUE) {
        if (fscanf(lindoFile, "%s", word) == EOF) {
            if (FLAG == 1) {
                FLAG = 2;
                break;
            } else {
                (void) printf(" sudden end of file lindo.out\n");
                exit(1);
            }
        }

        if (strcmp(word, "NO") == 0) {
            (void) fscanf(lindoFile, "%s", word);
            if (strcmp(word, "FEASIBLE") == 0) {
                (void) printf(" NO FEASIBLE SOLUTION OBTAINED\n");
                exit(1);
            }
            (void) printf("Error:NO %s in lindo.out\n", word);
            exit(1);
        }
        if (strcmp(word, "NEW") == 0) {
            (void) fscanf(lindoFile, "%s", word);
            if (strcmp(word, "INTEGER") == 0) {
                FLAG = 1;
            } else {
                (void) printf("Error:NEW %s in lindo.out\n", word);
                exit(1);
            }
            continue;
        }
        /* searching for "IP OPTIMUM" - to get the value of the objective function*/
        /*------------------------------------------------------------------------*/
        if (strcmp(word, "IP") == 0) {
            (void) fscanf(lindoFile, "%s", word);
            if (strcmp(word, "OPTIMUM") == 0)
                break;
            else {
                (void) printf("Error:unaccounted apperance of IP in lindo file\n");
                exit(1);
            }
        }
    } /* while */

    /* Optimum solution was not found, but do with the best possible */
    /*---------------------------------------------------------------*/
    if (FLAG == 2) {
        (void) fclose(lindoFile);
        lindoFile = fopen(filename, "r");
        partial_scan_lindoFile(lindoFile);
    }

    /* iteration_gain = value of the objective function */
    /*--------------------------------------------------*/
    while (TRUE) {
        (void) fscanf(lindoFile, "%s", word);
        if (strcmp(word, "1)") == 0) {
            (void) fscanf(lindoFile, "%f", &f_gain);
            *iteration_gain = (int) f_gain;
            if (ACT_DEBUG)
                (void) printf("total gain in IP = %d\n", *iteration_gain);
            break;
        }
    }
    /* "REDUCED COST" is a precursor of the solution */
    /*-----------------------------------------------*/
    while (TRUE) {
        (void) fscanf(lindoFile, "%s", word);
        if (strcmp(word, "REDUCED") == 0) {
            (void) fscanf(lindoFile, "%s", word);
            break;
        }
    }

    /* now get the result */
    /*--------------------*/
    for (; fscanf(lindoFile, "%s", word) != EOF;) {

        /* if "ROW" appears solution set section ends */
        /*--------------------------------------------*/
        if (strcmp(word, "ROW") == 0)
            break;
        if (word[0] == 'X') {
            (void) sscanf(word, "X%d", &variable_id);
            (void) fscanf(lindoFile, "%f", &result);
            variable[variable_id] = (int) result;
        }
    }
    (void) fclose(lindoFile);
}

/***********************************************************************************
  now it is known that the solution is not Optimum; so search for the "NEW
INTEGER" in the file. As soon as it is found(that is, the first instance)
return.
*************************************************************************************/
static void partial_scan_lindoFile(FILE *lindoFile) {
    char word[BUFSIZE];

    while (TRUE) {
        (void) fscanf(lindoFile, "%s", word);
        if (strcmp(word, "NEW") == 0) {
            (void) fscanf(lindoFile, "%s", word);
            if (strcmp(word, "INTEGER") == 0)
                break;
        }
    } /* while */
}

/*************************************************************************
  return 1 if any fanout of the node is a Primary output. Else return 0.
*************************************************************************/
int is_anyfo_PO(node_t *node) {
    lsGen           genfo;
    node_t          *fanout;
    node_function_t node_fun;

    foreach_fanout(node, genfo, fanout) {
        node_fun = node_function(fanout);
        if (node_fun == NODE_PO) {
            (void) lsFinish(genfo);
            return 1;
        }
    }
    return 0;
}

void print_multiple_fo_array(array_t *multiple_fo_array) {
    int            nsize;
    int            i;
    ACT_VERTEX_PTR v;

    (void) printf("---- printing multiple_fo_vertices---");
    nsize  = array_n(multiple_fo_array);
    for (i = 0; i < nsize; i++) {
        v = array_fetch(ACT_VERTEX_PTR, multiple_fo_array, i);
        (void) printf(" index = %d, id = %d, num_fanouts = %d\n ", v->index, v->id,
                      v->multiple_fo + 1);
    }
    (void) printf("\n");
}

int print_network(network_t *network, st_table *cost_table, int iteration_num) {
    lsGen           gen;
    node_t          *node;
    char            *dummy;
    int             TOTAL_COST;
    char            *name;
    node_function_t node_fun;
    /* int pattern_type; */ /* statistics about how many patterns of what type */

    /* pattern_type = 0; */
    TOTAL_COST = 0;

    if (ACT_DEBUG)
        (void) printf("printing network %s\n", network_name(network));
    foreach_node(network, gen, node) {
        node_fun = node_function(node);
        if ((node_fun == NODE_PI) || (node_fun == NODE_PO))
            continue;
        if (ACT_DEBUG)
            node_print(stdout, node);
        name = node_long_name(node);
        assert(st_lookup(cost_table, name, &dummy));
        if (ACT_DEBUG)
            (void) printf("cost = %d\n\n", ((COST_STRUCT *) dummy)->cost);
        TOTAL_COST += ((COST_STRUCT *) dummy)->cost;
        /* pattern_type += ((COST_STRUCT *) dummy)->pattern_type; */
    }

    if (ACT_DEBUG) {
        if (iteration_num < 0)
            (void) printf("**** total cost of network after all iterations is %d\n***",
                          TOTAL_COST);
        else
            (void) printf("**** total cost of network after iteration %d is %d\n***",
                          iteration_num, TOTAL_COST);
        /* (void) printf("**** usage of OR patterns = %d\n***",
pattern_type); */
    }

    return TOTAL_COST;
}

/*************************************************************************
  Decompose nodes of the network. If decomposition results in a
  lesser cost node, accept the decomposition, and replace the node by
  the decomposition.
  think about how to decompose the nodes in the network. Should I use the
  mux structure and do a resub. this is because it possibly means that the
  decomposition would be in terms of a mux structure => Boolean div not
  implemented in misII.
**************************************************************************/

static void decomp_big_nodes(network_t *network, st_table *cost_table,
                             int *gain, act_init_param_t *init_param) {
    array_t         *nodevec;
    int             size_net;
    int             i;
    node_t          *node;
    int             num_fanin, num_cube;
    node_function_t node_fun;
    int             FLAG_DECOMP;

    *gain = 0;
    FLAG_DECOMP = 1;

    nodevec  = network_dfs(network);
    size_net = array_n(nodevec);
    for (i   = 0; i < size_net; i++) {

        node     = array_fetch(node_t *, nodevec, i);
        node_fun = node_function(node);
        if ((node_fun == NODE_PI) || (node_fun == NODE_PO))
            continue;

        /* if the num_fanin is less than the limit, continue */
        /*---------------------------------------------------*/
        num_fanin = node_num_fanin(node);
        if (num_fanin < init_param->DECOMP_FANIN)
            continue;

        /*  if the node requires upto 2 muxes, do not split it.
                Reason being that if split, it would form 2 nodes
                (at least) and hence the cost would be >= 2 always */
        /*-----------------------------------------------------*/
        if (cost_of_node(node, cost_table) <= 2)
            continue;

        /* if constructed optimally, do not decompose */
        /*--------------------------------------------*/
        num_cube = node_num_cube(node);
        if ((num_cube == 1) ||
            (node_num_literal(node) ==
             num_cube) /* earlier it was num_fanin - changed Dec.17, 92 */
                )
            continue;

        /* using act_node_remap for decomposition */
        /*----------------------------------------*/
        (*gain) += act_node_remap(network, node, cost_table, FLAG_DECOMP,
                                  init_param->HEURISTIC_NUM);

    } /* for i = loop*/

    if (ACT_DEBUG)
        (void) printf("---Gain in decomp_big_nodes = %d\n", *gain);
    array_free(nodevec);
    /*  return network; */
}

/***************************************************************************
  given the node, finds the cost of the node from the cost table.
***************************************************************************/

int cost_of_node(node_t *node, st_table *cost_table) {
    char *dummy;

    assert(st_lookup(cost_table, node_long_name(node), &dummy));
    return ((COST_STRUCT *) dummy)->cost;
}

/***********************************************************************
   This procedure generates act ordering st. a function which has all the
   cubes of single literal, is implemented with minimum no. of mux
   structures. First separates the NEG and POS phase fanins. Rest is
   complicated!!!!

*************************************************************************/

array_t *OR_literal_order(node_t *node) {

    array_t       *order_list, *order_rev;
    array_t       *pos_list, *neg_list;
    int           size_pos, size_neg;
    int           FLAG_POS, FLAG_NEG;
    int           pointer_pos, pointer_neg;
    int           num_fanin;
    node_t        *fanin, *n;
    input_phase_t phase;
    int           i, j, stage, position;

    num_fanin  = node_num_fanin(node);
    order_list = array_alloc(node_t *, 0);
    order_rev  = array_alloc(node_t *, 0);
    pos_list   = array_alloc(node_t *, 0);
    neg_list   = array_alloc(node_t *, 0);

    /* separate the fanins with positive phase and negative phase */
    /*------------------------------------------------------------*/
    for (i = num_fanin - 1; i >= 0; i--) {
        fanin = node_get_fanin(node, i);
        phase = node_input_phase(node, fanin);
        if (phase == NEG_UNATE)
            array_insert_last(node_t *, neg_list, fanin);
        else if (phase == POS_UNATE)
            array_insert_last(node_t *, pos_list, fanin);
    }

    size_pos = array_n(pos_list);
    size_neg = array_n(neg_list);

    /* Get the optimal ordering now for STRUCT module - look at the paper for
   * details*/
    /*-------------------------------------------------------------------------------*/

    pointer_neg = 0;
    pointer_pos = 0;
    FLAG_NEG    = OFF;
    FLAG_POS    = OFF;
    stage       = 0;

    for (i = 0; i < num_fanin; i++) {
        switch (stage) {
            case 1:
                if (pointer_neg >= size_neg) {
                    FLAG_NEG = ON;
                    break;
                }
                n = array_fetch(node_t *, neg_list, pointer_neg);
                array_insert_last(node_t *, order_rev, n);
                pointer_neg++;
                break;
            case 0:;
            case 2:;
            case 3:
                if (pointer_pos >= size_pos) {
                    FLAG_POS = ON;
                    break;
                }
                n = array_fetch(node_t *, pos_list, pointer_pos);
                array_insert_last(node_t *, order_rev, n);
                pointer_pos++;
                break;
        } /* switch */

        /* if all neg phase fanins finished, put positive now */
        /*----------------------------------------------------*/
        if (FLAG_NEG == ON) {
            for (j = i; j < num_fanin; j++) {
                if (pointer_pos >= size_pos) {
                    (void) printf(" Error: pointer_pos = %d??\n", pointer_pos);
                    exit(1);
                }
                n = array_fetch(node_t *, pos_list, pointer_pos);
                array_insert_last(node_t *, order_rev, n);
                pointer_pos++;
            }
            break; /* from i loop */
        }

        /* if all  positive phased fanins finished, put negative now */
        /*-----------------------------------------------------------*/
        if (FLAG_POS == ON) {
            for (j = i; j < num_fanin; j++) {
                if (pointer_neg >= size_neg) {
                    (void) printf("Error: pointer_neg = %d??\n", pointer_neg);
                    exit(1);
                }
                n = array_fetch(node_t *, neg_list, pointer_neg);
                array_insert_last(node_t *, order_rev, n);
                pointer_neg++;
            }
            break; /* from i loop */
        }

        /* for first mux structure, there are 4 inputs that can be assigned,
for the rest, just 3, because the rest of them have as their inputs
the outputs from the previous muxes */
        /*------------------------------------------------------------------*/
        if ((stage % 3) == 0)
            stage = 1;
        else
            stage = stage + 1;

    } /* for */

    /*  order_rev has everything in reverse order,as it was constructed
bottom_up. Reverse it to get order_list */
    /*------------------------------------------------------------------*/
    for (i = 0; i < num_fanin; i++) {
        position = num_fanin - (i + 1);
        n        = array_fetch(node_t *, order_rev, position);
        array_insert_last(node_t *, order_list, n);
    }

    /* check */
    /*-------*/
    if (array_n(order_list) != num_fanin) {
        (void) printf("single_cube_order():something is binate for node %s\n",
                      node_long_name(node));
        exit(1);
    }

    /* free the lists */
    /*----------------*/
    array_free(order_rev);
    array_free(pos_list);
    array_free(neg_list);

    return order_list;
}

/***********************************************************************
   This procedure generates act ordering st. a function which has a
   single cube  is implemented with minimum no. of mux
   structures. First separates the NEG and POS phase fanins. Rest is
   complicated!!!!

*************************************************************************/

array_t *single_cube_order(node_t *node) {

    array_t       *order_list, *order_rev;
    array_t       *pos_list, *neg_list;
    int           size_pos, size_neg;
    int           FLAG_POS, FLAG_NEG;
    int           pointer_pos, pointer_neg;
    int           num_fanin;
    node_t        *fanin, *n;
    input_phase_t phase;
    int           i, j, stage, position;

    num_fanin  = node_num_fanin(node);
    order_list = array_alloc(node_t *, 0);
    order_rev  = array_alloc(node_t *, 0);
    pos_list   = array_alloc(node_t *, 0);
    neg_list   = array_alloc(node_t *, 0);

    for (i = num_fanin - 1; i >= 0; i--) {
        fanin = node_get_fanin(node, i);
        phase = node_input_phase(node, fanin);
        if (phase == NEG_UNATE)
            array_insert_last(node_t *, neg_list, fanin);
        else if (phase == POS_UNATE)
            array_insert_last(node_t *, pos_list, fanin);
    }

    size_pos    = array_n(pos_list);
    size_neg    = array_n(neg_list);
    pointer_neg = 0;
    pointer_pos = 0;
    FLAG_NEG    = OFF;
    FLAG_POS    = OFF;
    stage       = 0;

    for (i = 0; i < num_fanin; i++) {
        switch (stage) {
            case 2:
            case 3:
                if (pointer_neg >= size_neg) {
                    FLAG_NEG = ON;
                    break;
                }
                n = array_fetch(node_t *, neg_list, pointer_neg);
                array_insert_last(node_t *, order_rev, n);
                pointer_neg++;
                break;
            case 0:;
            case 1:
                if (pointer_pos >= size_pos) {
                    FLAG_POS = ON;
                    break;
                }
                n = array_fetch(node_t *, pos_list, pointer_pos);
                array_insert_last(node_t *, order_rev, n);
                pointer_pos++;
                break;
        } /* switch */

        /* if all neg phase fanins finished, put positive now */
        /*----------------------------------------------------*/
        if (FLAG_NEG == ON) {
            for (j = i; j < num_fanin; j++) {
                if (pointer_pos >= size_pos) {
                    (void) printf(" pointer_pos = %d??\n", pointer_pos);
                    exit(1);
                }
                n = array_fetch(node_t *, pos_list, pointer_pos);
                array_insert_last(node_t *, order_rev, n);
                pointer_pos++;
            }
            break; /* from i loop */
        }

        /* if all  positive phased fanins finished, put negative now */
        /*-----------------------------------------------------------*/
        if (FLAG_POS == ON) {
            for (j = i; j < num_fanin; j++) {
                if (pointer_neg >= size_neg) {
                    (void) printf(" pointer_neg = %d??\n", pointer_neg);
                    exit(1);
                }
                n = array_fetch(node_t *, neg_list, pointer_neg);
                array_insert_last(node_t *, order_rev, n);
                pointer_neg++;
            }
            break; /* from i loop */
        }

        /* for first mux structure, there are 4 inputs that can be assigned,
for the rest, just 3  */
        /*----------------------------------------------------------------*/
        if ((stage % 3) == 0)
            stage = 1;
        else
            stage = stage + 1;

    } /* for */

    /*  order_rev has everything in reverse order,as it was constructed
bottom_up. Reverse it to get order_list */
    /*-----------------------------------------------------------------*/
    for (i = 0; i < num_fanin; i++) {
        position = num_fanin - (i + 1);
        n        = array_fetch(node_t *, order_rev, position);
        array_insert_last(node_t *, order_list, n);
    }

    /* check */
    /*-------*/
    if (array_n(order_list) != num_fanin) {
        (void) printf("single_cube_order():something is binate for node %s\n",
                      node_long_name(node));
        exit(1);
    }

    /* free the lists */
    /*----------------*/
    array_free(order_rev);
    array_free(pos_list);
    array_free(neg_list);

    return order_list;
}

/****************************************************************************************
  Checks if it is beneficial to implement the node in negative phase.
"Beneficial" is gotten by recomputing the cost of the node and the fanouts of
the node. Greedily selects the node, if it improves the cost.
******************************************************************************************/
static void act_quick_phase(network_t *network, st_table *cost_table,
                            act_init_param_t *init_param) {

    array_t         *cost_array, *nodevec;
    COST_STRUCT     *cost_struc;
    int             i, j;
    int             size, TOTAL_GAIN, gain, new_cost, old_cost;
    lsGen           gen;
    node_t          *node, *fanout;
    char            *name, *dummy;
    node_function_t node_fun;

    if (ACT_DEBUG)
        (void) printf("-------entering act_quick_phase--------\n");
    nodevec = network_dfs(network);
    size    = array_n(nodevec);

    TOTAL_GAIN = 0;
    for (i     = 0; i < size; i++) {

        node = array_fetch(node_t *, nodevec, i);
        if (ACT_DEBUG)
            node_print(stdout, node);

        /* if node cannot be inverted, get the next node */
        /*-----------------------------------------------*/
        node_fun = node_function(node);
        if ((node_fun == NODE_PI) || (node_fun == NODE_PO))
            continue;
        if (is_anyfo_PO(node))
            continue;

        /* invert the node, if possible */
        /*------------------------------*/
        if (!node_invert(node)) {
            continue;
        }

        /* checking */
        /*----------*/
        assert(st_lookup(cost_table, node_long_name(node), &dummy));

        /* evaluate the cost of the inverted node, store in cost_array */
        /*-------------------------------------------------------------*/
        gain       = 0;
        cost_array = array_alloc(COST_STRUCT *, 0);

        old_cost   = ((COST_STRUCT *) dummy)->cost;
        cost_struc = act_evaluate_map_cost(node, init_param, NIL(network_t),
                                           NIL(array_t), NIL(st_table));
        array_insert_last(COST_STRUCT *, cost_array, cost_struc);
        new_cost = cost_struc->cost;
        gain     = old_cost - new_cost;

        /* evaluate the cost of all the fanouts of the node */
        /*--------------------------------------------------*/
        if (ACT_DEBUG)
            (void) printf(" num_fo = %d\n", node_num_fanout(node));
        foreach_fanout(node, gen, fanout) {
            assert(st_lookup(cost_table, node_long_name(fanout), &dummy));
            old_cost   = ((COST_STRUCT *) dummy)->cost;
            cost_struc = act_evaluate_map_cost(fanout, init_param, NIL(network_t),
                                               NIL(array_t), NIL(st_table));
            array_insert_last(COST_STRUCT *, cost_array, cost_struc);
            new_cost = cost_struc->cost;
            gain     = gain + old_cost - new_cost;
        }

        if (ACT_DEBUG)
            (void) printf(" gain from node %s = %d\n", node_long_name(node), gain);

        /* if it is beneficial to invert the node, change the corresponding entries
               of the cost_table */
        /*-------------------------------------------------------------------------*/
        if (gain > 0) {
            TOTAL_GAIN += gain;
            name = node_long_name(node);
            assert(st_delete(cost_table, &name, &dummy));
            free_cost_struct((COST_STRUCT *) dummy);
            cost_struc = array_fetch(COST_STRUCT *, cost_array, 0);
            assert(!st_insert(cost_table, name, (char *) cost_struc));
            j = 1;
            foreach_fanout(node, gen, fanout) {
                name = node_long_name(fanout);
                assert(st_delete(cost_table, &name, &dummy));
                free_cost_struct((COST_STRUCT *) dummy);
                cost_struc = array_fetch(COST_STRUCT *, cost_array, j);
                j++;
                assert(!st_insert(cost_table, name, (char *) cost_struc));
            }
        } /* if gain > 0 */

            /* if it is not beneficial, invert the node back, since earlier
       inversion was done in place */
            /*-------------------------------------------------------------*/
        else {
            /* if gain <= 0, then do not change the network. Restore it first */
            /*----------------------------------------------------------------*/
            (void) node_invert(node);

            for (j = array_n(cost_array) - 1; j >= 0; j--) {
                cost_struc = array_fetch(COST_STRUCT *, cost_array, j);
                free_cost_struct((COST_STRUCT *) cost_struc);
            }
        } /* else */

        array_free(cost_array);
    } /* for i = */
    if (ACT_DEBUG)
        (void) printf("*******TOTAL_GAIN from act_quick_phase = %d******\n",
                      TOTAL_GAIN);
    array_free(nodevec);
    /* return network; */
}

void free_cost_struct(COST_STRUCT *cost_node) {
    if (cost_node->cost != 0) {
        if (ACT_DEBUG)
            act_check(cost_node->act);
    }
    my_free_act(cost_node->act);
    FREE(cost_node);
}

/*ARGSUSED*/
void free_cost_table(st_table *table) {
    char *arg;
    st_foreach(table, free_table_entry, arg);
}

/*ARGSUSED */
static enum st_retval free_table_entry(char *key, char *value, char *arg) {
    (void) free_cost_struct((COST_STRUCT *) value);
    FREE(key);
    return ST_DELETE;
}

/*ARGSUSED*/
void free_cost_table_without_freeing_key(st_table *table) {
    char *arg;
    st_foreach(table, free_table_entry_without_freeing_key, arg);
}

/*ARGSUSED */
enum st_retval free_table_entry_without_freeing_key(char *key, char *value,
                                                    char *arg) {
    (void) free_cost_struct((COST_STRUCT *) value);
    return ST_DELETE;
}

/************************************************************************************
  This function returns a node that implements mux function, i.e.
                (control' left + control right).
*************************************************************************************/
node_t *act_mux_node(node_t *control, node_t *left, node_t *right) {
    node_t *not_control, *and1, *and2, *answer, *answer_simpl;

    not_control = node_not(control);
    and1        = node_and(not_control, left);
    and2        = node_and(control, right);
    answer      = node_or(and1, and2);
    node_free(not_control);
    node_free(and1);
    node_free(and2);
    answer_simpl = node_simplify(answer, NIL(node_t), NODE_SIM_ESPRESSO);
    node_free(answer);
    return answer_simpl;
}

/********************************************************************************
  This function returns the function implemented by the Actel basic block.
  The naming convention of the inputs is the same as reported in the
  Actel paper.
*********************************************************************************/
static node_t *basic_block_node(node_t *A0, node_t *A1, node_t *SA, node_t *B0,
                                node_t *B1, node_t *SB, node_t *S0, node_t *S1,
                                st_table *cost_table) {
    node_t *out1, *out2, *or_control, *out, *out_simpl;

    (void) fprintf(BDNET_FILE,
                   "\n INSTANCE \"BASIC_BLOCK\":physical NAME = INST%d;\n",
                   instance_num);
    instance_num++;

    print_node_with_string(A0, "A0", cost_table);
    print_node_with_string(A1, "A1", cost_table);
    print_node_with_string(SA, "SA", cost_table);
    print_node_with_string(B0, "B0", cost_table);
    print_node_with_string(B1, "B1", cost_table);
    print_node_with_string(SB, "SB", cost_table);
    print_node_with_string(S0, "S0", cost_table);
    print_node_with_string(S1, "S1", cost_table);

    out1       = act_mux_node(SA, A0, A1);
    out2       = act_mux_node(SB, B0, B1);
    or_control = node_or(S0, S1);
    out        = act_mux_node(or_control, out1, out2);
    node_free(out1);
    node_free(out2);
    node_free(or_control);
    out_simpl = node_simplify(out, NIL(node_t), NODE_SIM_ESPRESSO);
    node_free(out);
    return out_simpl;
}

static void print_output_node_with_string(node_t *node, char *name) {
    node_function_t node_fun;

    (void) fprintf(BDNET_FILE, "        \"%s\"           :         \"", name);
    node_fun = node_function(node);
    switch (node_fun) {
        case NODE_0:(void) fprintf(BDNET_FILE, "GND");
            break;
        case NODE_1:(void) fprintf(BDNET_FILE, "Vdd");
            break;
        default:node_print_rhs(BDNET_FILE, node);
    }
    (void) fprintf(BDNET_FILE, "\";\n");
}

static void print_node_with_string(node_t *node, char *name,
                                   st_table *cost_table) {
    node_t          *fanin;
    char            *fanin_fn_type;
    node_function_t node_fun;

    (void) fprintf(BDNET_FILE, "        \"%s\"           :         \"", name);

    /* hack */
    /*------*/
    node_fun = node_function(node);
    switch (node_fun) {
        case NODE_0:fanin_fn_type = "GND";
            (void) fprintf(BDNET_FILE, "%s", fanin_fn_type);
            break;
        case NODE_1:fanin_fn_type = "Vdd";
            (void) fprintf(BDNET_FILE, "%s", fanin_fn_type);
            break;
        default:fanin     = node_get_fanin(node, 0);
            fanin_fn_type = act_get_node_fn_type(fanin, cost_table);
            if (fanin_fn_type == NIL(char))
                (void) fprintf(BDNET_FILE, "%s", node_long_name(fanin));
            else
                (void) fprintf(BDNET_FILE, "%s", fanin_fn_type);
    }
    (void) fprintf(BDNET_FILE, "\";\n");
}

/********************************************************************************
  Given a network and the cost_table, breaks up each node into nodes that can be
  realized by one block of STRUCT module.
*********************************************************************************/
network_t *act_break_network(network_t *network, st_table *table) {
    array_t     *nodevec;
    int         size;
    int         i, i1;
    int         is_one; /* = 1 if the node has non-zero cost */
    node_t      *node;
    COST_STRUCT *cost_struct;
    char        *dummy;

    num_or_patterns = 0;
    instance_num    = 0; /* keeps track of the instance numbers in the bdnet file */
    nodevec         = network_dfs(network);
    size            = array_n(nodevec);
    /* setting up the node_function_info */
    /*-----------------------------------*/
    for (i          = 0; i < size; i++) {
        node = array_fetch(node_t *, nodevec, i);
        if (node->type != INTERNAL)
            continue;
        assert(st_lookup(table, node_long_name(node), &dummy));
        cost_struct = (COST_STRUCT *) dummy;
        cost_struct->fn_type = act_get_node_fn_type(node, table);
        /* fn_type = act_get_node_fn_type(node, table); */

        /* check - just to be sure */
        /*-------------------------*/
        i1 = (cost_struct->fn_type == NIL(char));
        if (cost_struct->cost != 0)
            assert(i1);
    }

    init_bdnet(network, table); /* print the beginning remarks */

    for (i = 0; i < size; i++) {
        node = array_fetch(node_t *, nodevec, i);
        if (node->type != INTERNAL)
            continue;
        /* possibly change the node and the bdd (act) there */
        /*--------------------------------------------------*/
        is_one = act_break_node(node, network, table);
        if (is_one) {
            (void) fprintf(BDNET_FILE,
                           "        \"out\"          :          \"%s\"; \n\n",
                           node_long_name(node));
        }
    }
    (void) fprintf(BDNET_FILE, "ENDMODEL;\n");

    assert(network_check(network));
    array_free(nodevec);
    return network;
}

/*---------------------------------------------------------------------------
  Given the node, breaks it up into many nodes, each  with cost 1.
  Based on the bdd at that node. Network is changed. New nodes may be added
  and the functionality of the node may change. Also, breaks the bdd (act)
  in the cost_node and puts their appropriate parts in the node. After the
  bdd is broken, all the new bdd's are trees (not even leaf dag's). All the
  multiple fo points are duplicated. Care has to be taken for all the nodes
  so that name at the bdd's are correct.
----------------------------------------------------------------------------*/
void act_break_node(node_t *node, network_t *network, st_table *table) {
    int             j, size_my_node_array1;
    node_t          *node1, *node_new, *node_returned;
    char            *dummy, *node_new_name;
    COST_STRUCT     *cost_struct, *cost_node;
    node_function_t node_fun;
    VERTEX_NODE     *vertex_name_struct, *vertex_node;

    node_fun = node_function(node);
    if ((node_fun == NODE_PI) || (node_fun == NODE_PO))
        return 0;
    assert(st_lookup(table, node_long_name(node), &dummy));
    cost_struct = (COST_STRUCT *) dummy;
    if (cost_struct->cost == 0)
        return 0;
    PRESENT_ACT = cost_struct->act;
    /* added Aug 7 '90 */
    /*-----------------*/
    if (ACT_DEBUG)
        act_check(cost_struct->act);
    set_mark_act(PRESENT_ACT);

    my_node_array     = array_alloc(node_t *, 0);
    vertex_node_array = array_alloc(VERTEX_NODE *, 0);
    vertex_name_array = array_alloc(VERTEX_NODE *, 0);

    node_returned = act_get_function(network, node, PRESENT_ACT, table);
    node_free(node_returned);
    size_my_node_array1 = array_n(my_node_array) - 1;
    if (array_n(my_node_array) == 0);
    else {
        /* add in the network new nodes resulting from breaking the node */
        /*---------------------------------------------------------------*/
        /* with change made Aug 18, 1991,  adding the nodes is done earlier.*/
        /*------------------------------------------------------------------*/

        for (j = 0; j < size_my_node_array1; j++) {
            node_new    = array_fetch(node_t *, my_node_array, j);
            vertex_node = array_fetch(VERTEX_NODE *, vertex_node_array, j);
            assert(node_new == vertex_node->node);
            /* network_add_node(network, node_new); */
            cost_node     = act_allocate_cost_node(node_new, 1, -1.0, -1.0, -1.0, NO, 0.0,
                                                   0.0, vertex_node->vertex);
            node_new_name = util_strsav(node_long_name(node_new));
            assert(!st_insert(table, node_new_name, (char *) cost_node));
            /* free the storage associated with the act (bdd) */
            /*------------------------------------------------------*/
            set_mark_act(cost_node->act);
            free_nodes_in_act_and_change_multiple_fo(cost_node->act);

            FREE(vertex_node); /* is this the place to free it? */
        }
        node1 = array_fetch(node_t *, my_node_array, size_my_node_array1);
        node_replace(node, node1);
        /* change the cost of node to 1 */
        /*------------------------------*/
        cost_struct->cost = 1;
        /* added later */
        /*-------------*/
        vertex_node =
                array_fetch(VERTEX_NODE *, vertex_node_array, size_my_node_array1);
        FREE(vertex_node);
    }
    /* all the multiple fo vertices had a node in the node field. Free that node
   */
    /*---------------------------------------------------------------------------*/
    set_mark_act(PRESENT_ACT);
    free_nodes_in_act_and_change_multiple_fo(PRESENT_ACT);

    /* associate the vertex with the node - for name: only after all the
             corresponding nodes have been inserted into the network */
    /*-----------------------------------------------------------------*/
    for (j = 0; j < array_n(vertex_name_array); j++) {
        vertex_name_struct = array_fetch(VERTEX_NODE *, vertex_name_array, j);
        vertex_name_struct->vertex->name = node_long_name(vertex_name_struct->node);
        FREE(vertex_name_struct);
    }

    array_free(my_node_array);
    array_free(vertex_name_array);
    array_free(vertex_node_array);
    return 1;
}

void set_mark_act(ACT_VERTEX_PTR PRESENT_ACT) {
    MARK_VALUE                = PRESENT_ACT->mark;
    if (MARK_VALUE == 1)
        MARK_COMPLEMENT_VALUE = 0;
    else
        MARK_COMPLEMENT_VALUE = 1;
}

/**************************************************************************************
  Given a node and its corresponding ACT(OACT), breaks the node into nodes, each
  new node realizing a function that can be implemented by one basic block. Puts
all these nodes in the network. Original node is changed.
***************************************************************************************/
static node_t *act_get_function(network_t *network, node_t *node,
                                ACT_VERTEX_PTR vertex, st_table *table) {

    node_t      *node1, *node2;
    node_t      *A0, *A1, *SA, *B0, *B1, *SB, *S0, *S1;
    ACT_VERTEX  *v_low, *v_high, *v_lowlow, *v_lowhigh, *v_highlow, *v_highhigh;
    ACT_VERTEX  *v_lowlowlow, *v_lowlowhigh, *v_highlowlow;
    VERTEX_NODE *vertex_node;

    /* vertex->node corresponds to the node at the vertex - only for the
multiple_fanout vertex */
    /*-----------------------------------------------------------------*/

    if (vertex->value == 0) {
        vertex->mark = MARK_COMPLEMENT_VALUE;
        return node_constant(0);
    }
    if (vertex->value == 1) {
        vertex->mark = MARK_COMPLEMENT_VALUE;
        return node_constant(1);
    }

    if (vertex->mark == MARK_COMPLEMENT_VALUE) {
        if (vertex->multiple_fo == 0) {
            (void) printf("error: act_get_function()\n");
            exit(1);
        }
        return vertex->node;
    }

    /* added April 4, 1991*/
    /*-------------------*/
    if (!act_is_or_used) {
        assert(vertex->pattern_num < 4);
    }

    /* even multiple fo vertices will be mapped when they are visited first time
   */
    /*---------------------------------------------------------------------------*/
    vertex->mark = MARK_COMPLEMENT_VALUE;

    if ((vertex->low->value == 0) && (vertex->high->value == 1)) {
        node1 = get_node_literal_of_vertex(vertex, network);
        if (vertex->multiple_fo != 0)
            vertex->node = node1;
        return node1;
    }

    if (vertex->pattern_num > 3)
        num_or_patterns++;

    switch (vertex->pattern_num) {

        case 0:

            A0 = node_constant(0);
            A1 = act_get_function(network, node, vertex->low, table);
            SA = node_constant(1);

            B0 = A0;
            B1 = act_get_function(network, node, vertex->high, table);
            SB = SA;

            S0 = get_node_literal_of_vertex(vertex, network);
            S1 = A0;

            node1 = basic_block_node(A0, A1, SA, B0, B1, SB, S0, S1, table);
            if (PRESENT_ACT != vertex) {
                network_add_node(network, node1);
            }

            /* need v_low etc because act_change_vertex may change the
pointer of vertex and free_node_if_possible needs
original vertex->low (i.e. v_low)    	     	 */
            /*-------------------------------------------------------*/
            v_low  = vertex->low;
            v_high = vertex->high;
            act_change_vertex_child(vertex, v_low, LOW, A1);
            act_change_vertex_child(vertex, v_high, HIGH, B1);

            free_node_if_possible(A1, v_low);
            free_node_if_possible(B1, v_high);

            node_free(A0);
            node_free(SA);
            node_free(S0);

            node2 = node_literal(node1, 1);
            if (vertex != PRESENT_ACT)
                print_output_node_with_string(node2, "out");
            if (vertex->multiple_fo != 0)
                vertex->node = node2;
            array_insert_last(node_t *, my_node_array, node1);

            vertex_node = act_allocate_vertex_node_struct(vertex, node1);
            array_insert_last(VERTEX_NODE *, vertex_node_array, vertex_node);

            return node2;

        case 1:

            vertex->low->mark = MARK_COMPLEMENT_VALUE;
            A0 = act_get_function(network, node, vertex->low->low, table);
            A1 = act_get_function(network, node, vertex->low->high, table);
            SA = get_node_literal_of_vertex(vertex->low, network);

            B0 = node_constant(0);
            B1 = act_get_function(network, node, vertex->high, table);
            SB = node_constant(1);

            S0 = get_node_literal_of_vertex(vertex, network);
            S1 = B0;

            node1 = basic_block_node(A0, A1, SA, B0, B1, SB, S0, S1, table);
            if (PRESENT_ACT != vertex) {
                network_add_node(network, node1);
            }

            v_lowlow  = vertex->low->low;
            v_lowhigh = vertex->low->high;
            v_high    = vertex->high;
            act_change_vertex_child(vertex->low, v_lowlow, LOW, A0);
            act_change_vertex_child(vertex->low, v_lowhigh, HIGH, A1);
            act_change_vertex_child(vertex, v_high, HIGH, B1);

            free_node_if_possible(A0, v_lowlow);
            free_node_if_possible(A1, v_lowhigh);
            free_node_if_possible(B1, v_high);

            node_free(B0);
            node_free(SB);
            node_free(S0);
            node_free(SA);

            node2 = node_literal(node1, 1);
            if (vertex != PRESENT_ACT)
                print_output_node_with_string(node2, "out");
            if (vertex->multiple_fo != 0)
                vertex->node = node2;
            array_insert_last(node_t *, my_node_array, node1);

            vertex_node = act_allocate_vertex_node_struct(vertex, node1);
            array_insert_last(VERTEX_NODE *, vertex_node_array, vertex_node);

            return node2;

        case 2:vertex->high->mark = MARK_COMPLEMENT_VALUE;
            A0 = node_constant(0);
            A1 = act_get_function(network, node, vertex->low, table);
            SA = node_constant(1);

            B0 = act_get_function(network, node, vertex->high->low, table);
            B1 = act_get_function(network, node, vertex->high->high, table);
            SB = get_node_literal_of_vertex(vertex->high, network);

            S0 = get_node_literal_of_vertex(vertex, network);
            S1 = A0;

            node1 = basic_block_node(A0, A1, SA, B0, B1, SB, S0, S1, table);
            if (PRESENT_ACT != vertex) {
                network_add_node(network, node1);
            }

            v_highlow  = vertex->high->low;
            v_highhigh = vertex->high->high;
            v_low      = vertex->low;
            act_change_vertex_child(vertex, v_low, LOW, A1);
            act_change_vertex_child(vertex->high, v_highlow, LOW, B0);
            act_change_vertex_child(vertex->high, v_highhigh, HIGH, B1);

            free_node_if_possible(A1, v_low);
            free_node_if_possible(B0, v_highlow);
            free_node_if_possible(B1, v_highhigh);

            node_free(A0);
            node_free(SA);
            node_free(S0);
            node_free(SB);

            node2 = node_literal(node1, 1);
            if (vertex != PRESENT_ACT)
                print_output_node_with_string(node2, "out");
            if (vertex->multiple_fo != 0)
                vertex->node = node2;
            array_insert_last(node_t *, my_node_array, node1);

            vertex_node = act_allocate_vertex_node_struct(vertex, node1);
            array_insert_last(VERTEX_NODE *, vertex_node_array, vertex_node);

            return node2;

        case 3:vertex->low->mark = MARK_COMPLEMENT_VALUE;
            vertex->high->mark   = MARK_COMPLEMENT_VALUE;

            A0 = act_get_function(network, node, vertex->low->low, table);
            A1 = act_get_function(network, node, vertex->low->high, table);
            SA = get_node_literal_of_vertex(vertex->low, network);

            B0 = act_get_function(network, node, vertex->high->low, table);
            B1 = act_get_function(network, node, vertex->high->high, table);
            SB = get_node_literal_of_vertex(vertex->high, network);

            S0 = get_node_literal_of_vertex(vertex, network);
            S1 = node_constant(0);

            node1 = basic_block_node(A0, A1, SA, B0, B1, SB, S0, S1, table);
            if (PRESENT_ACT != vertex) {
                network_add_node(network, node1);
            }
            v_lowlow   = vertex->low->low;
            v_lowhigh  = vertex->low->high;
            v_highlow  = vertex->high->low;
            v_highhigh = vertex->high->high;
            act_change_vertex_child(vertex->low, v_lowlow, LOW, A0);
            act_change_vertex_child(vertex->low, v_lowhigh, HIGH, A1);
            act_change_vertex_child(vertex->high, v_highlow, LOW, B0);
            act_change_vertex_child(vertex->high, v_highhigh, HIGH, B1);

            free_node_if_possible(A0, v_lowlow);
            free_node_if_possible(A1, v_lowhigh);
            free_node_if_possible(B0, v_highlow);
            free_node_if_possible(B1, v_highhigh);

            node_free(S1);
            node_free(SA);
            node_free(SB);
            node_free(S0);

            node2 = node_literal(node1, 1);
            if (vertex != PRESENT_ACT)
                print_output_node_with_string(node2, "out");
            if (vertex->multiple_fo != 0)
                vertex->node = node2;
            array_insert_last(node_t *, my_node_array, node1);

            vertex_node = act_allocate_vertex_node_struct(vertex, node1);
            array_insert_last(VERTEX_NODE *, vertex_node_array, vertex_node);

            return node2;

        case 4:vertex->low->low->mark = MARK_COMPLEMENT_VALUE;
            vertex->low->mark         = MARK_COMPLEMENT_VALUE;

            A0 = act_get_function(network, node, vertex->low->low->low, table);
            A1 = act_get_function(network, node, vertex->low->low->high, table);
            SA = get_node_literal_of_vertex(vertex->low->low, network);

            B0 = node_constant(0);
            B1 = act_get_function(network, node, vertex->high, table);
            SB = node_constant(1);

            S0 = get_node_literal_of_vertex(vertex, network);
            S1 = get_node_literal_of_vertex(vertex->low, network);

            node1 = basic_block_node(A0, A1, SA, B0, B1, SB, S0, S1, table);
            if (PRESENT_ACT != vertex) {
                network_add_node(network, node1);
            }

            v_lowlowlow  = vertex->low->low->low;
            v_lowlowhigh = vertex->low->low->high;
            v_high       = vertex->high;
            act_change_vertex_child(vertex->low->low, v_lowlowlow, LOW, A0);
            act_change_vertex_child(vertex->low->low, v_lowlowhigh, HIGH, A1);
            act_change_vertex_child(vertex, v_high, HIGH, B1);
            act_change_vertex_child(vertex->low, v_high, HIGH, B1);

            free_node_if_possible(A0, v_lowlowlow);
            free_node_if_possible(A1, v_lowlowhigh);
            free_node_if_possible(B1, v_high);

            node_free(SA);
            node_free(B0);
            node_free(SB);
            node_free(S0);
            node_free(S1);

            node2 = node_literal(node1, 1);
            if (vertex != PRESENT_ACT)
                print_output_node_with_string(node2, "out");
            if (vertex->multiple_fo != 0)
                vertex->node = node2;
            array_insert_last(node_t *, my_node_array, node1);

            vertex_node = act_allocate_vertex_node_struct(vertex, node1);
            array_insert_last(VERTEX_NODE *, vertex_node_array, vertex_node);

            return node2;

        case 5:vertex->high->mark   = MARK_COMPLEMENT_VALUE;
            vertex->high->low->mark = MARK_COMPLEMENT_VALUE;

            A0 = node_constant(0);
            A1 = act_get_function(network, node, vertex->high->low->low, table);
            SA = get_node_literal_of_vertex(vertex, network);

            B0 = A0;
            B1 = act_get_function(network, node, vertex->high->high, table);
            SB = SA;

            S0 = get_node_literal_of_vertex(vertex->high, network);
            S1 = get_node_literal_of_vertex(vertex->high->low, network);

            node1 = basic_block_node(A0, A1, SA, B0, B1, SB, S0, S1, table);
            if (PRESENT_ACT != vertex) {
                network_add_node(network, node1);
            }

            v_highlowlow = vertex->high->low->low;
            v_highhigh   = vertex->high->high;
            act_change_vertex_child(vertex->high->low, v_highlowlow, LOW, A1);
            act_change_vertex_child(vertex->high, v_highhigh, HIGH, B1);
            act_change_vertex_child(vertex->high->low, v_highhigh, HIGH, B1);
            act_change_vertex_child(vertex, vertex->low, LOW, NIL(node_t));

            free_node_if_possible(A1, v_highlowlow);
            free_node_if_possible(B1, v_highhigh);

            node_free(A0);
            node_free(SA);
            node_free(S0);
            node_free(S1);

            node2 = node_literal(node1, 1);
            if (vertex != PRESENT_ACT)
                print_output_node_with_string(node2, "out");
            if (vertex->multiple_fo != 0)
                vertex->node = node2;
            array_insert_last(node_t *, my_node_array, node1);

            vertex_node = act_allocate_vertex_node_struct(vertex, node1);
            array_insert_last(VERTEX_NODE *, vertex_node_array, vertex_node);

            return node2;

        case 6:

            vertex->high->mark      = MARK_COMPLEMENT_VALUE;
            vertex->high->low->mark = MARK_COMPLEMENT_VALUE;

            A0 = node_constant(0);
            A1 = node_constant(1);
            SA = A1;

            B0 = A1;
            B1 = A0;
            SB = get_node_literal_of_vertex(vertex, network);

            S0 = get_node_literal_of_vertex(vertex->high, network);
            S1 = get_node_literal_of_vertex(vertex->high->low, network);

            node1 = basic_block_node(A0, A1, SA, B0, B1, SB, S0, S1, table);
            if (PRESENT_ACT != vertex) {
                network_add_node(network, node1);
            }

            v_highhigh = vertex->high->high;
            act_change_vertex_child(vertex, vertex->low, LOW, NIL(node_t));
            act_change_vertex_child(vertex->high->low, vertex->high->low->low, LOW,
                                    NIL(node_t));
            act_change_vertex_child(vertex->high, v_highhigh, HIGH, NIL(node_t));
            act_change_vertex_child(vertex->high->low, v_highhigh, HIGH, NIL(node_t));

            node_free(A0);
            node_free(A1);
            node_free(SB);
            node_free(S0);
            node_free(S1);

            node2 = node_literal(node1, 1);
            if (vertex != PRESENT_ACT)
                print_output_node_with_string(node2, "out");
            if (vertex->multiple_fo != 0)
                vertex->node = node2;
            array_insert_last(node_t *, my_node_array, node1);

            vertex_node = act_allocate_vertex_node_struct(vertex, node1);
            array_insert_last(VERTEX_NODE *, vertex_node_array, vertex_node);

            return node2;

        case 7:vertex->low->mark   = MARK_COMPLEMENT_VALUE;
            vertex->low->low->mark = MARK_COMPLEMENT_VALUE;

            A0 = act_get_function(network, node, vertex->low->low->low, table);
            A1 = node_constant(0);
            SA = get_node_literal_of_vertex(vertex, network);

            B0 = act_get_function(network, node, vertex->low->high, table);
            B1 = A1;
            SB = SA;

            S0 = get_node_literal_of_vertex(vertex->low, network);
            S1 = get_node_literal_of_vertex(vertex->low->low, network);

            node1 = basic_block_node(A0, A1, SA, B0, B1, SB, S0, S1, table);
            if (PRESENT_ACT != vertex) {
                network_add_node(network, node1);
            }
            v_lowlowlow = vertex->low->low->low;
            v_lowhigh   = vertex->low->high;
            act_change_vertex_child(vertex->low->low, v_lowlowlow, LOW, A0);
            act_change_vertex_child(vertex->low->low, v_lowhigh, HIGH, B0);
            act_change_vertex_child(vertex->low, v_lowhigh, HIGH, B0);
            act_change_vertex_child(vertex, vertex->high, HIGH, NIL(node_t));

            free_node_if_possible(A0, v_lowlowlow);
            free_node_if_possible(B0, v_lowhigh);

            node_free(A1);
            node_free(SA);
            node_free(S0);
            node_free(S1);

            node2 = node_literal(node1, 1);
            if (vertex != PRESENT_ACT)
                print_output_node_with_string(node2, "out");
            if (vertex->multiple_fo != 0)
                vertex->node = node2;
            array_insert_last(node_t *, my_node_array, node1);

            vertex_node = act_allocate_vertex_node_struct(vertex, node1);
            array_insert_last(VERTEX_NODE *, vertex_node_array, vertex_node);

            return node2;
        default:
            (void) printf(" act_get_function(): unknown pattern_num %d found\n",
                          vertex->pattern_num);
            exit(1);
    } /* switch */
    return NIL(node_t);
}

/*-------------------------------------------------------------------
  Given a bdd vertex, returns the literal of the node of the network
  that corresponds to it. Gets the node by name.
  COMMENT: differs from get_node_of_vertex().
---------------------------------------------------------------------*/
node_t *get_node_literal_of_vertex(ACT_VERTEX_PTR vertex, network_t *network) {
    node_t *node;

    node = network_find_node(network, vertex->name);
    if (node == NIL(node_t)) {
        (void) printf(" get_node_literal_of_network(): node is NULL\n");
        exit(1);
    }
    return (node_literal(node, 1));
}

/***********************************************************************************
   If the vertex is just an input, then the node cannot be freed. If the vertex
is a multiple_fo point, then also it cannot be freed. This is because these
nodes are needed in the network.
************************************************************************************/
void free_node_if_possible(node_t *node, ACT_VERTEX_PTR vertex) {
    node_function_t node_fn;

    node_fn = node_function(node);
    if ((node_fn == NODE_0) || (node_fn == NODE_1)) {
        node_free(node);
        return;
    }
    /* should be after the node_fn check, because 0, 1 vertices
do not store anything */
    /*---------------------------------------------------------*/
    if (vertex->multiple_fo != 0)
        return;
    node_free(node);
}

void act_act_free(node_t *node) {
    void local_actFree();

    ACT_SET(node)->LOCAL_ACT->act->root = NIL(ACT_VERTEX);
    local_actFree(node);
}

/*-----------------------------------------------------------
  Makes the multiple_fo field of all vertices 0.
  Also frees the node associated with the non-terminal
  multiple fanout vertices.
-------------------------------------------------------------*/
static void free_nodes_in_act_and_change_multiple_fo(ACT_VERTEX_PTR vertex) {
    if (vertex->mark == MARK_COMPLEMENT_VALUE)
        return;
    vertex->mark = MARK_COMPLEMENT_VALUE;
    if (vertex->value != 4) {
        vertex->multiple_fo = 0;
        return;
    }
    if (vertex->multiple_fo != 0) {
        node_free(vertex->node);
        vertex->multiple_fo = 0;
    }
    free_nodes_in_act_and_change_multiple_fo(vertex->low);
    free_nodes_in_act_and_change_multiple_fo(vertex->high);
}

/*********************************************************************
   writes input and output of the network in the bdnet file.
**********************************************************************/

static void init_bdnet(network_t *network, st_table *table) {

    lsGen  genpi, genpo;
    node_t *pi, *po, *po_fanin;
    char   *pi_name, *po_name, *po_fanin_fn_type;

    (void) fprintf(BDNET_FILE, "MODEL \"%s\";\n\n", network_name(network));

    (void) fprintf(BDNET_FILE, "TECHNOLOGY scmos;\n");
    (void) fprintf(BDNET_FILE, "VIEWTYPE SYMBOLIC;\n");
    (void) fprintf(BDNET_FILE, "EDITSTYLE SYMBOLIC;\n\n");
    /* writing inputs */
    /*----------------*/
    foreach_primary_input(network, genpi, pi) {
        pi_name = node_long_name(pi);
        (void) fprintf(BDNET_FILE, "INPUT \t\"%s\"	:	\"%s\";\n", pi_name,
                       pi_name);
    }
    (void) fprintf(BDNET_FILE, "\n");

    /* writing outputs */
    /*-----------------*/
    foreach_primary_output(network, genpo, po) {
        po_name  = node_long_name(po);
        po_fanin = node_get_fanin(po, 0);

        /* hack for bdnet */
        /*----------------*/
        /* if po_fanin has 0 cost, then print the transitive input (or Vdd, GND)
that influences it */
        /*---------------------------------------------------------------------*/
        po_fanin_fn_type = act_get_node_fn_type(po_fanin, table);
        if (po_fanin_fn_type == NIL(char))
            (void) fprintf(BDNET_FILE, "OUTPUT \t\"%s\"	:	\"%s\";\n", po_name,
                           node_long_name(po_fanin));
        else
            (void) fprintf(BDNET_FILE, "OUTPUT \t\"%s\"	:	\"%s\";\n", po_name,
                           po_fanin_fn_type);
    }
    (void) fprintf(BDNET_FILE, "\n");
}

/*-----------------------------------------------------------------
  This is a "hack" for generating bdnet file. If a node has NODE_0
  (NODE_1) function, returns "GND" (Vdd"). If it is a NODE_BUF
  (single input), then returns the name of the farthest
   transitive fanin node   which is either NODE_0, NODE_1,
   or NODE_BUF, appropriately. It may happen that the cost of the
   node is 0, but it is not of type NODE_0 or NODE_1 (it may be of
   the type a + a'), then also GND or Vdd may be returned. Actually
   it will never be of type GND.
------------------------------------------------------------------*/
static char *act_get_node_fn_type(node_t *node, st_table *cost_table) {
    node_t          *fanin;
    char            *dummy;
    COST_STRUCT     *cost_node;
    node_function_t node_fun;
    ACT_VERTEX_PTR  vertex;

    node_fun = node_function(node);
    switch (node_fun) {
        case NODE_0:return "GND";
        case NODE_1:return "Vdd";
        case NODE_BUF:fanin = node_get_fanin(node, 0);
            if (fanin->type == PRIMARY_INPUT)
                return node_long_name(fanin);
            assert(st_lookup(cost_table, node_long_name(fanin), &dummy));
            cost_node = (COST_STRUCT *) dummy;
            if (cost_node->fn_type == NIL(char))
                return node_long_name(fanin);
            else
                return cost_node->fn_type;
        default:
            if (node->type != INTERNAL)
                return NIL(char); /* may not be needed */
            /* change made on Aug. 18, 1991 when pointed out by Andre' */
            /*---------------------------------------------------------*/
            if (!st_lookup(cost_table, node_long_name(node), &dummy))
                return NIL(char);
            cost_node = (COST_STRUCT *) dummy;
            if (cost_node->cost == 0) {
                vertex = cost_node->act;
                assert(vertex != NIL(ACT_VERTEX));
                assert(vertex->value < 2);
                if (vertex->value == 0)
                    return "GND";
                return "Vdd";
            }
            return NIL(char);
    }
}

network_t *act_network_remap(network_t *network, st_table *cost_table) {
    array_t *nodevec;
    node_t  *node;
    int     i, size;
    int     gain;
    int     FLAG_DECOMP   = 0;
    int     HEURISTIC_NUM = 0; /* does not matter */

    gain    = 0;
    nodevec = network_dfs(network);
    size    = array_n(nodevec);
    for (i  = 0; i < size; i++) {
        node = array_fetch(node_t *, nodevec, i);
        gain +=
                act_node_remap(network, node, cost_table, FLAG_DECOMP, HEURISTIC_NUM);
    }
    if (ACT_STATISTICS)
        (void) printf("total gain after remapping = %d\n", gain);
    array_free(nodevec);
    return network;
}

/**********************************************************************************************
  Calls the same procedures for a node to obtain a good mapping of the node.
  It accepts the solution if it is better than the earlier one.
  Can be called either for the DECOMPOSITION or for simply remapping
(LAST_GASP). FLAG_DECOMP  is 1 if the routine is called from decomp_big_nodes,
is 0 if it is called from act_network_remap.
**********************************************************************************************/
int act_node_remap(network_t *network, node_t *node, st_table *cost_table,
                   int FLAG_DECOMP, int HEURISTIC_NUM_ORIG) {

    node_t           *dup_node, *node1, *node1_dup;
    network_t        *network1;
    int              cost_node_original;
    int              cost_network1;
    st_table         *node_cost_table1, *table1;
    int              num_cubes, gain, num_nodes2, i;
    act_init_param_t *init_param;
    node_function_t  node_fun;
    array_t          *nodevec1;
    char             *name, *name1, *dummy;
    COST_STRUCT      *cost_struct1;

    node_fun = node_function(node);
    if ((node_fun == NODE_PI) || (node_fun == NODE_PO))
        return 0;

    cost_node_original = cost_of_node(node, cost_table);
    if (ACT_DEBUG)
        (void) printf("original cost of node %s = %d\n", node_long_name(node),
                      cost_node_original);

    /* could be made 2, but may be the cost can be made 1 - takes care of f = a +
   * a' */
    /*-------------------------------------------------------------------------------*/
    if (cost_node_original <= 1)
        return 0;

    /* if the subject-graph for the node is optimal, do not remap */
    /*------------------------------------------------------------*/
    num_cubes = node_num_cube(node);
    if ((num_cubes == 1) || (num_cubes == node_num_literal(node)))
        return 0;

    dup_node = node_dup(node);
    network1 = network_create_from_node(dup_node);

    decomp_good_network(network1);
    if ((FLAG_DECOMP == 1) && (network_num_internal(network1) == 1)) {
        network_free(network1);
        node_free(dup_node);
        return 0;
    }
    if (FLAG_DECOMP == 0)
        decomp_tech_network(network1, 2, 2);

    init_param            = ALLOC(act_init_param_t, 1);
    if (FLAG_DECOMP == 0) {
        init_param->NUM_ITER        = 2;
        init_param->GAIN_FACTOR     = 0.001;
        init_param->FANIN_COLLAPSE  = 4;
        init_param->DECOMP_FANIN    = 4;
        init_param->QUICK_PHASE     = 1;
        init_param->DISJOINT_DECOMP = 0;
        init_param->HEURISTIC_NUM   = 4;
    } else {
        init_param->NUM_ITER        = 0;
        init_param->GAIN_FACTOR     = 0.001;
        init_param->FANIN_COLLAPSE  = 0;
        init_param->DECOMP_FANIN    = 0;
        init_param->QUICK_PHASE     = 0;
        init_param->DISJOINT_DECOMP = 0;
        init_param->HEURISTIC_NUM   = HEURISTIC_NUM_ORIG;
    }
    init_param->mode      = AREA;
    init_param->LAST_GASP = 0;
    init_param->BREAK     = 0;

    node_cost_table1 = st_init_table(strcmp, st_strhash);
    network1         = act_map_network(network1, init_param, node_cost_table1);
    FREE(init_param);

    cost_network1 = print_network(network1, node_cost_table1, -1);
    if (ACT_DEBUG)
        (void) printf(" the original cost of node = %d, cost after remapping = %d\n",
                      cost_node_original, cost_network1);
    gain = cost_node_original - cost_network1;
    if (gain <= 0) {
        free_cost_table(node_cost_table1);
        st_free_table(node_cost_table1);
        network_free(network1);
        node_free(dup_node);
        return 0;
    }
    /* replace the node in the original network by the network1
better not to call pld_replace_node_by_network, since we
need to re-place the cost_struct also                    */
    /*----------------------------------------------------------*/
    nodevec1 = network_dfs(network1);
    table1   = st_init_table(st_ptrcmp, st_ptrhash);
    /* set correspondence between p.i. and ... */
    /*-----------------------------------------*/
    pld_remap_init_corr(table1, network, network1);
    num_nodes2 = array_n(nodevec1) - 2;
    for (i     = 0; i < num_nodes2; i++) {
        node1 = array_fetch(node_t *, nodevec1, i);
        if (node1->type != INTERNAL)
            continue;
        node1_dup = pld_remap_get_node(node1, table1);
        node1_dup->name       = NIL(char); /* possibility of leak? */
        node1_dup->short_name = NIL(char);
        network_add_node(network, node1_dup);
        if (ACT_DEBUG)
            (void) node_print(sisout, node1_dup);
        /* lookup the cost_struct1 for node1 and put it in the
cost_table */
        /*---------------------------------------------------*/
        name1 = util_strsav(node_long_name(node1_dup));
        assert(st_lookup(node_cost_table1, node_long_name(node1), &dummy));
        cost_struct1 = (COST_STRUCT *) dummy;
        act_remap_update_act_fields(table1, network1, node1_dup, cost_struct1);
        assert(!st_insert(cost_table, name1, (char *) cost_struct1));
        assert(!st_insert(table1, (char *) node1, (char *) node1_dup));
        /* if (ACT_DEBUG)
(void) printf("inserting node %s (%s) in table\n",
node_long_name(node1_dup), node_name(node1_dup)); */
    }
    /* the last node in the array in primary output - do not want that */
    /*-----------------------------------------------------------------*/
    node1      = array_fetch(node_t *, nodevec1, num_nodes2);
    assert(node1->type == INTERNAL);
    node1_dup = pld_remap_get_node(node1, table1);
    node_replace(node, node1_dup);
    name  = node_long_name(node);
    name1 = util_strsav(name);
    /* delete the entry of the node from the cost_table */
    /*--------------------------------------------------*/
    assert(st_delete(cost_table, &name, &dummy));
    (void) free_cost_struct((COST_STRUCT *) dummy);
    FREE(name);
    /* look up the cost of node1, put it in the cost_table */
    /*-----------------------------------------------------*/
    assert(st_lookup(node_cost_table1, node_long_name(node1), &dummy));
    cost_struct1 = (COST_STRUCT *) dummy;
    act_remap_update_act_fields(table1, network1, node, cost_struct1);
    assert(!st_insert(cost_table, name1, (char *) cost_struct1));
    assert(!st_insert(table1, (char *) node1, (char *) node));
    /* if (ACT_DEBUG) {
(void) printf("inserting node %s (%s) in table\n", node_long_name(node),
           node_name(node));
(void) node_print(sisout, node);
} */
    st_free_table(node_cost_table1);
    st_free_table(table1);
    array_free(nodevec1);
    network_free(network1);
    node_free(dup_node);
    return gain;
}

/************************************************************************************************
  Given a node and its subject_graph (ACT, or OACT), snap at multiple fanout
points and maps each tree using the pattern-set. The mapping is for a
combination of delay and area depending upon value of mode. init_param->mode = 1
(DELAY) means mapping is done only for delay. init_param->mode -> 0 : for AREA.
  However this routine is not called for init_param->mode = 0 (AREA) (instead
make_tree_and_map() is used.) REMARK: The arrival time at the output of the node
takes into account the fanout of the node.
*************************************************************************************************/

COST_STRUCT *make_tree_and_map_delay(node_t *node, ACT_VERTEX_PTR act_of_node,
                                     network_t *network, array_t *delay_values,
                                     st_table *cost_table, float mode) {
    int            num_acts;
    int            i;
    int            num_mux_struct;
    int            TOTAL_MUX_STRUCT;                  /* cost of the network */
    /* int 		pattern_type; */ /* how many patterns which use OR gate
                                          */
    COST_STRUCT    *cost_node;
    ACT_VERTEX_PTR act;
    double         correction;

    /* pattern_type = 0; */
    TOTAL_MUX_STRUCT = 0;
    cost_node        = ALLOC(COST_STRUCT, 1);

    act_init_multiple_fo_array(act_of_node); /* insert the root */
    act_initialize_act_delay(act_of_node);
    make_multiple_fo_array_delay(act_of_node, multiple_fo_array);
    /* if (ACT_DEBUG) my_traverse_act(act_of_node);
if (ACT_DEBUG) print_multiple_fo_array(multiple_fo_array); */
    num_acts = array_n(multiple_fo_array);
    /* changed the following traversal , now  bottom-up */
    /*--------------------------------------------------*/
    for (i   = num_acts - 1; i >= 0; i--) {
        act         = array_fetch(ACT_VERTEX_PTR, multiple_fo_array, i);
        PRESENT_ACT = act;
        num_mux_struct =
                map_act_delay(act, network, delay_values, cost_table, mode);
        TOTAL_MUX_STRUCT += num_mux_struct;
    }
    if (ACT_DEBUG)
        (void) printf("number of total mux_structures used for the node %s = %d, "
                      "arrival_time = %f\n",
                      node_long_name(node), TOTAL_MUX_STRUCT,
                      act_of_node->arrival_time);

    array_free(multiple_fo_array);
    cost_node->cost = TOTAL_MUX_STRUCT;
    /* this arrival time was computed with the assumption of one fanout.
Now correct it */
    /*-----------------------------------------------------------------*/
    correction = act_get_node_delay_correction(node, delay_values, 1,
                                               node_num_fanout(node));
    cost_node->arrival_time = act_of_node->arrival_time + correction;
    act_invalidate_cost_and_arrival_time(cost_node);
    cost_node->cost_and_arrival_time = act_cost_delay(cost_node, mode);
    cost_node->node                  = node;
    cost_node->act                   = act_of_node;
    /* cost_node->pattern_type = pattern_type; */
    return cost_node;
}

/*-------------------------------------------------------------------
  Given a bdd vertex, maps it in a non-area init_param->mode.
--------------------------------------------------------------------*/
static int map_act_delay(ACT_VERTEX_PTR vertex, network_t *network,
                         array_t *delay_values, st_table *cost_table,
                         float mode) {
    int    TOTAL_PATTERNS = 12; /* right now using just 8 */
    int    pat_num;
    int    cost[12]; /* for each of the 12 patterns, this is the area cost in terms
of STRUCT blocks */
    double delay[12]; /* same as cost, but now for delay */
    int    vlow_cost, vhigh_cost, vlowlow_cost, vlowhigh_cost, vhighlow_cost,
           vhighhigh_cost;
    double vlow_delay, vhigh_delay, vlowlow_delay, vlowhigh_delay, vhighlow_delay,
           vhighhigh_delay;
    int    best_pat_num;
    node_t *node; /* corresponding to a vertex of bdd (act), it gives the node */
    node_t *nlow, *nhigh, *nlowlow, *nhighlow;
    double prop_delay, max_input_arrival, max_input_int_arrival;

    if (vertex->value != 4) {
        vertex->mapped       = 1;
        vertex->arrival_time = (double) 0.0;
        vertex->cost         = 0;
        return 0;
    }

    /* if multiple fanout (but not the root of the present multiple_fo vertex)
return 0, but do not set any value, as this vertex would be/ or was
visited earlier as a root. Basically zero is not its true cost. So do not set
vertex->mapped = 1 also */
    /*--------------------------------------------------------------------------*/
    if ((vertex->multiple_fo != 0) && (vertex != PRESENT_ACT))
        return 0; /* because area not to be counted */

    /* if cost already computed for a vertex in the tree rooted by "PRESENT_ACT",
   * don't compute again */
    /*------------------------------------------------------------------------------------------------*/
    if (vertex->mapped)
        return vertex->cost;

    vertex->mapped = 1;

    /* if a simple input, cost = 0 */
    /*-----------------------------*/
    if ((vertex->low->value == 0) && (vertex->high->value == 1)) {
        node = get_node_of_vertex(vertex, network);
        vertex->arrival_time = act_get_arrival_time(node, cost_table);
        vertex->cost         = 0;
        return 0;
    }

    /* initialize cost vector to a very high value for all the patterns*/
    /*-----------------------------------------------------------------*/
    for (pat_num = 0; pat_num < TOTAL_PATTERNS; pat_num++) {
        cost[pat_num]  = MAXINT;
        delay[pat_num] = (double) MAXINT;
    }

    /***********recursively calculate relevant costs *************/

    if (vertex->low->multiple_fo != 0) {
        vlow_cost  = 0;
        vlow_delay = vertex->low->arrival_time;
    } else {
        (void) map_act_delay(vertex->low, network, delay_values, cost_table, mode);
        vlow_cost  = vertex->low->cost;
        vlow_delay = vertex->low->arrival_time;
    }

    if (vertex->high->multiple_fo != 0) {
        vhigh_cost  = 0;
        vhigh_delay = vertex->high->arrival_time;
    } else {
        (void) map_act_delay(vertex->high, network, delay_values, cost_table, mode);
        vhigh_cost  = vertex->high->cost;
        vhigh_delay = vertex->high->arrival_time;
    }

    if (vertex->low->value == 4) {
        if (vertex->low->multiple_fo != 0) {
            vlowlow_cost  = vlowhigh_cost  = MAXINT;
            vlowlow_delay = vlowhigh_delay =
                    (double) MAXINT; /* because do not want to change tree-mapping */
        } else {
            if (vertex->low->low->multiple_fo != 0)
                vlowlow_cost = 0;
            else
                vlowlow_cost = map_act_delay(vertex->low->low, network, delay_values,
                                             cost_table, mode);
            vlowlow_delay =
                    vertex->low->low
                          ->arrival_time; /* use the delay value previously set */
            if (vertex->low->high->multiple_fo != 0)
                vlowhigh_cost = 0;
            else
                vlowhigh_cost = map_act_delay(vertex->low->high, network, delay_values,
                                              cost_table, mode);
            vlowhigh_delay    = vertex->low->high->arrival_time;
        }
    } /* if */

    if (vertex->high->value == 4) {
        if (vertex->high->multiple_fo != 0) {
            vhighlow_cost  = vhighhigh_cost  = MAXINT;
            vhighlow_delay = vhighhigh_delay = (double) MAXINT;
        } else {
            if (vertex->high->low->multiple_fo != 0)
                vhighlow_cost = 0;
            else
                vhighlow_cost  = map_act_delay(vertex->high->low, network, delay_values,
                                               cost_table, mode);
            vhighlow_delay     = vertex->high->low->arrival_time;
            if (vertex->high->high->multiple_fo != 0)
                vhighhigh_cost = 0;
            else
                vhighhigh_cost = map_act_delay(vertex->high->high, network,
                                               delay_values, cost_table, mode);
            vhighhigh_delay    = vertex->high->high->arrival_time;
        }
    }

    node       = get_node_of_vertex(vertex, network);
    if (vertex->low->value == 4)
        nlow   = get_node_of_vertex(vertex->low, network);
    if (vertex->high->value == 4)
        nhigh  = get_node_of_vertex(vertex->high, network);
    prop_delay = act_get_bddfanout_delay(vertex, delay_values, cost_table);

    /**********enumerate possible cases************************/

    /* l.c. (left child) + r.c.(right child) + 1 */
    /*-------------------------------------------*/

    cost[0] = vlow_cost + vhigh_cost + 1;
    max_input_arrival = my_max(my_max(vlow_delay, vhigh_delay),
                               act_get_arrival_time(node, cost_table));
    delay[0] = max_input_arrival + prop_delay;

    if (vertex->low->value == 4) {
        cost[1] = vlowlow_cost + vlowhigh_cost + vhigh_cost + 1;
        max_input_int_arrival =
                my_max(my_max(vlowlow_delay, vlowhigh_delay), vhigh_delay);
        max_input_arrival = my_max(my_max(act_get_arrival_time(node, cost_table),
                                          act_get_arrival_time(nlow, cost_table)),
                                   max_input_int_arrival);
        delay[1] = max_input_arrival + prop_delay;
    }
    if (vertex->high->value == 4) {
        cost[2] = vlow_cost + vhighlow_cost + vhighhigh_cost + 1;
        max_input_int_arrival =
                my_max(my_max(vhighlow_delay, vhighhigh_delay), vlow_delay);
        max_input_arrival = my_max(my_max(act_get_arrival_time(node, cost_table),
                                          act_get_arrival_time(nhigh, cost_table)),
                                   max_input_int_arrival);
        delay[2] = max_input_arrival + prop_delay;
    }

    if ((vertex->low->value == 4) && (vertex->high->value == 4)) {
        cost[3] = vlowlow_cost + vlowhigh_cost + vhighlow_cost + vhighhigh_cost + 1;
        max_input_int_arrival = my_max(my_max(vlowlow_delay, vlowhigh_delay),
                                       my_max(vhighlow_delay, vhighhigh_delay));
        max_input_arrival     = my_max(
                my_max(act_get_arrival_time(node, cost_table),
                       act_get_arrival_time(nlow, cost_table)),
                my_max(act_get_arrival_time(nhigh, cost_table), max_input_int_arrival));
        delay[3] = max_input_arrival + prop_delay;
    }

    /* OR patterns - pretty involved -> for more explanation, see doc. */
    /*-----------------------------------------------------------------*/

    if ((OR_pattern(vertex)) && (vertex->low->low->multiple_fo == 0) &&
        (vertex->low->low->value == 4)) {
        /* important to do this way, otherwise if low->low->low multiple
fo vertex, may get a wrong answer. */
        nlowlow = get_node_of_vertex(vertex->low->low, network);
        cost[4] = map_act_delay(vertex->low->low->low, network, delay_values,
                                cost_table, mode) +
                  map_act_delay(vertex->low->low->high, network, delay_values,
                                cost_table, mode) +
                  1; /* vertex->high is a multiple-parent vertex */
        max_input_int_arrival = my_max(my_max(vertex->low->low->low->arrival_time,
                                              vertex->low->low->high->arrival_time),
                                       vhigh_delay);
        max_input_arrival     = my_max(my_max(act_get_arrival_time(node, cost_table),
                                              act_get_arrival_time(nlow, cost_table)),
                                       my_max(act_get_arrival_time(nlowlow, cost_table),
                                              max_input_int_arrival));
        delay[4] = max_input_arrival + prop_delay;
    }

    if ((OR_pattern(vertex->high)) && (vertex->high->multiple_fo == 0)) {
        if (vertex->low->value == 0) {
            nhighlow = get_node_of_vertex(vertex->high->low, network);
            cost[5] = map_act_delay(vertex->high->high, network, delay_values,
                                    cost_table, mode) +
                      map_act_delay(vertex->high->low->low, network, delay_values,
                                    cost_table, mode) +
                      1;
            max_input_int_arrival = my_max(vertex->high->high->arrival_time,
                                           vertex->high->low->low->arrival_time);
            max_input_arrival =
                    my_max(my_max(act_get_arrival_time(node, cost_table),
                                  act_get_arrival_time(nhigh, cost_table)),
                           my_max(act_get_arrival_time(nhighlow, cost_table),
                                  max_input_int_arrival));
            delay[5] = max_input_arrival + prop_delay;
        } else {
            if ((vertex->low->value == 1) && (vertex->high->high->value == 0) &&
                (vertex->high->low->low->value == 1)) {
                nhighlow = get_node_of_vertex(vertex->high->low, network);
                cost[6] = 1;
                max_input_arrival =
                        my_max(my_max(act_get_arrival_time(node, cost_table),
                                      act_get_arrival_time(nhigh, cost_table)),
                               act_get_arrival_time(nhighlow, cost_table));
                delay[6] = max_input_arrival + prop_delay;
            }
        }
    }

    if ((OR_pattern(vertex->low)) && (vertex->low->multiple_fo == 0)) {
        if (vertex->high->value == 0) {
            nlowlow = get_node_of_vertex(vertex->low->low, network);
            cost[7] = map_act_delay(vertex->low->high, network, delay_values,
                                    cost_table, mode) +
                      map_act_delay(vertex->low->low->low, network, delay_values,
                                    cost_table, mode) +
                      1;
            max_input_int_arrival = my_max(vertex->low->high->arrival_time,
                                           vertex->low->low->low->arrival_time);
            max_input_arrival =
                    my_max(my_max(act_get_arrival_time(node, cost_table),
                                  act_get_arrival_time(nlow, cost_table)),
                           my_max(act_get_arrival_time(nlowlow, cost_table),
                                  max_input_int_arrival));
            delay[7] = max_input_arrival + prop_delay;
        }
    }

    /************find minimum cost************************/
    best_pat_num = minimum_costdelay_index(cost, delay, TOTAL_PATTERNS, mode);
    vertex->cost         = cost[best_pat_num];
    vertex->arrival_time = delay[best_pat_num];
    vertex->pattern_num  = best_pat_num;
    /* if (ACT_DEBUG)
(void) printf("vertex id = %d, index = %d, cost = %d, name = %s,
arrival_time = %f, pattern_num = %d\n", vertex->id, vertex->index,
vertex->cost, vertex->name, vertex->arrival_time, vertex->pattern_num); */
    return vertex->cost;
}

static int minimum_costdelay_index(int cost[], double delay[], int num_entries,
                                   float mode) {

    int    i;
    int    min_index = 0;
    double *cost_and_delay;

    /* generate a cost_and_delay array that takes into account cost and delay */
    /*------------------------------------------------------------------------*/
    cost_and_delay = ALLOC(double, num_entries);
    for (i         = 0; i < num_entries; i++) {
        cost_and_delay[i] =
                ((double) (1.00 - mode)) * (double) cost[i] + ((double) mode) * delay[i];
    }

    for (i = 1; i < num_entries; i++) {
        if (cost_and_delay[i] < cost_and_delay[min_index])
            min_index = i;
    }

    FREE(cost_and_delay);
    return min_index;
}

/*-------------------------------------------------------------------
  Given a bdd vertex, returns the node of the network
  that corresponds to it. Gets the node by name.
---------------------------------------------------------------------*/
static node_t *get_node_of_vertex(ACT_VERTEX_PTR vertex, network_t *network) {
    node_t *node;

    node = network_find_node(network, vertex->name);
    if (node == NIL(node_t)) {
        (void) printf(" get_node_literal_of_network(): node is NULL\n");
        exit(1);
    }
    return node;
}

array_t *act_get_corr_fanin_nodes(node_t *node, array_t *order_list1) {
    network_t *network;
    int       i;
    array_t   *order_list;
    node_t    *n1, *n;

    network    = node->network;
    order_list = array_alloc(node_t *, 0);
    for (i     = 0; i < array_n(order_list1); i++) {
        n1 = array_fetch(node_t *, order_list1, i);
        assert(n1->type == PRIMARY_INPUT);
        n = network_find_node(network, node_long_name(n1));
        assert(n != NIL(node_t));
        array_insert_last(node_t *, order_list, n);
    }
    assert(array_n(order_list) == node_num_fanin(node));
    return order_list;
}

/* not implemented yet */
/*ARGSUSED*/
void act_delay_iterative_improvement(network_t *network, st_table *cost_table,
                                     array_t *delay_values,
                                     act_init_param_t *init_param) {
    //TODO: what's this?!
}
