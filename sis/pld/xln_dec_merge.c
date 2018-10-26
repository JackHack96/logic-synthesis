/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/pld/xln_dec_merge.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:56 $
 *
 */
#include "pld_int.h"
#include "sis.h"

/*---------------------------------------------------------------------
  Find pairs of nodes for extracting mergeable nodes. This is done as
  follows: first infeasible nodes are collected. Then, for each of
  the pairs of nodes, finds the affinity between the nodes. Greedily
  selects the best pairs and generates mergeable functions.
-----------------------------------------------------------------------*/
void xln_decomp_for_merging_network(network_t *network,
                                    xln_init_param_t *init_param) {
    array_t *nodevec;

    /* make an array of internal infeasible nodes */
    /*--------------------------------------------*/
    nodevec = xln_infeasible_nodes(network, init_param->support);

    /* depending on the heuristic, decide if you wish to do an
       all-cube approach or a pair of node approach.          */
    /*--------------------------------------------------------*/
    if (init_param->heuristic == ALL_CUBES) {
        xln_decomp_for_merging_ALL_CUBES(nodevec, init_param);
    } else {
        xln_decomp_for_merging_PAIR_NODES(nodevec, init_param);
    }
    (void) network_sweep(network);
    array_free(nodevec);
}

void xln_decomp_for_merging_ALL_CUBES(array_t *nodevec,
                                      xln_init_param_t *init_param) {
    array_t  *cube_node_vec;
    st_table *cubenode2node_table;
    st_table *node2cubenode_table;

    /* table stores for a cube_node, the node from which it was extracted */
    /*--------------------------------------------------------------------*/
    cubenode2node_table = st_init_table(st_ptrcmp, st_ptrhash);
    node2cubenode_table = st_init_table(st_ptrcmp, st_ptrhash);

    cube_node_vec = xln_fill_tables(cubenode2node_table, node2cubenode_table,
                                    nodevec, init_param);
    xln_decomp_for_merge_cube_nodes(cube_node_vec, cubenode2node_table,
                                    node2cubenode_table, init_param);

    xln_free_cube_nodes(cube_node_vec);
    array_free(cube_node_vec);
    xln_free_node2cubenode_table(node2cubenode_table);
    st_free_table(cubenode2node_table);
}

void xln_decomp_for_merging_PAIR_NODES(array_t *nodevec,
                                       xln_init_param_t *init_param) {
    array_t         *affinity_vec;
    array_t         *matched_vec;
    st_table        *table1, *table2;
    int             i, j;
    node_t          *node1, *node2;
    AFFINITY_STRUCT *affinity_struct;

    /* find affinity (number of common inputs) for each pair of
       nodes in nodevec                                              */
    /*---------------------------------------------------------------*/
    affinity_vec = array_alloc(AFFINITY_STRUCT *, 0);
    for (i       = 0; i < array_n(nodevec); i++) {
        node1  = array_fetch(node_t *, nodevec, i);
        for (j = i + 1; j < array_n(nodevec); j++) {
            node2           = array_fetch(node_t *, nodevec, j);
            affinity_struct = ALLOC(AFFINITY_STRUCT, 1);
            affinity_struct->node1  = node1;
            affinity_struct->node2  = node2;
            affinity_struct->common = xln_node_find_common_inputs(node1, node2);
            array_insert_last(AFFINITY_STRUCT *, affinity_vec, affinity_struct);
        }
    }
    array_sort(affinity_vec, pld_affinity_compare_function);

    /* store the matched-cubes in matched_vec, table1, table2 */
    /*--------------------------------------------------------*/
    matched_vec = array_alloc(AFFINITY_STRUCT *, 0);
    table1      = st_init_table(st_ptrcmp, st_ptrhash);
    table2      = st_init_table(st_ptrcmp, st_ptrhash);
    for (i      = 0; i < array_n(affinity_vec); i++) {
        affinity_struct = array_fetch(AFFINITY_STRUCT *, affinity_vec, i);
        node1           = affinity_struct->node1;
        node2           = affinity_struct->node2;

        /* check for already matched up cube either from node1 or node2*/
        /*-------------------------------------------------------------*/
        if ((st_is_member(table1, (char *) node1)) ||
            (st_is_member(table2, (char *) node2))) {
            xln_free_AFFINITY_STRUCT(affinity_struct);
            continue;
        }
        assert(!st_insert(table1, (char *) node1, (char *) node1));
        assert(!st_insert(table2, (char *) node2, (char *) node2));
        array_insert_last(AFFINITY_STRUCT *, matched_vec, affinity_struct);
    }
    array_free(affinity_vec);
    st_free_table(table1);
    st_free_table(table2);

    /* for each of the pairs, find mergeable functions, extract
       them. Change the nodes.                                  */
    /*----------------------------------------------------------*/
    for (i = 0; i < array_n(matched_vec); i++) {
        affinity_struct = array_fetch(AFFINITY_STRUCT *, matched_vec, i);
        /* assert(network_check(network)); */
        xln_decomp_for_merge_pair_of_nodes(affinity_struct->node1,
                                           affinity_struct->node2, init_param);
        xln_free_AFFINITY_STRUCT(affinity_struct);
    }
    /* assert(network_check(network)); */
    array_free(matched_vec);
}

/*-----------------------------------------------------------------------------
  Given two infeasible nodes n1 & n2, pair up cubes which have good chances of
  generating good mergeable functions. This is done by simply finding the
  number of common inputs to the two cubes. Then for these pairs, generate
  mergeable functions. Make these the nodes of the network. The
  cover-merge or merge routine will hopefully merge these nodes.
------------------------------------------------------------------------------*/
void xln_decomp_for_merge_pair_of_nodes(node_t *n1, node_t *n2,
                                        xln_init_param_t *init_param) {
    array_t  *cube_node_vec;
    array_t  *nodevec;
    st_table *cubenode2node_table;
    st_table *node2cubenode_table;

    /* table stores for a cube_node, the node from which it was extracted */
    /*--------------------------------------------------------------------*/
    cubenode2node_table = st_init_table(st_ptrcmp, st_ptrhash);
    node2cubenode_table = st_init_table(st_ptrcmp, st_ptrhash);

    nodevec = array_alloc(node_t *, 0);
    array_insert_last(node_t *, nodevec, n1);
    array_insert_last(node_t *, nodevec, n2);

    cube_node_vec = xln_fill_tables(cubenode2node_table, node2cubenode_table,
                                    nodevec, init_param);
    xln_decomp_for_merge_cube_nodes(cube_node_vec, cubenode2node_table,
                                    node2cubenode_table, init_param);

    xln_free_cube_nodes(cube_node_vec);
    array_free(cube_node_vec);
    array_free(nodevec);
    xln_free_node2cubenode_table(node2cubenode_table);
    st_free_table(cubenode2node_table);
}

/*-------------------------------------------------------------------------------
  Given an array of cube_nodes (cube_node_vec) and the nodes of the network to
  which they belong (in the table cubenode2node_table), pair up the cube_nodes
  and extract mergeable functions.
--------------------------------------------------------------------------------*/

void xln_decomp_for_merge_cube_nodes(array_t *cube_node_vec,
                                     st_table *cubenode2node_table,
                                     st_table *node2cubenode_table,
                                     xln_init_param_t *init_param) {
    array_t         *affinity_vec;
    int             i, j;
    node_t          *cube_node1, *cube_node2;
    node_t          *n1, *n2; /* nodes of the network to which cube_node1
           and cube_node2 belong */
    AFFINITY_STRUCT *affinity_struct;
    st_table        *table;
    array_t         *common;

    /* set up the affinity between all pairs of cube_nodes */
    /*-----------------------------------------------------*/
    affinity_vec = array_alloc(AFFINITY_STRUCT *, 0);
    for (i       = 0; i < array_n(cube_node_vec); i++) {
        cube_node1 = array_fetch(node_t *, cube_node_vec, i);
        for (j     = i + 1; j < array_n(cube_node_vec); j++) {
            cube_node2 = array_fetch(node_t *, cube_node_vec, j);

            /* if the two cubes belong to the same node and have number of
               total inputs <= init_param->support, do not pair them together.
               They can anyway be realized in one CLB (by bin-packing)       */
            /*---------------------------------------------------------------*/
            assert(st_lookup(cubenode2node_table, (char *) cube_node1, (char **) &n1));
            assert(st_lookup(cubenode2node_table, (char *) cube_node2, (char **) &n2));
            if ((n1 == n2) && (xln_num_composite_fanin(cube_node1, cube_node2) <=
                               init_param->support))
                continue;

            /* find the set of inputs common to two sub_cubes. If the number is
               lower than a limit, ignore the pair                           */
            /*---------------------------------------------------------------*/
            common = xln_node_find_common_inputs(cube_node1, cube_node2);
            if (array_n(common) < init_param->common_lower_bound) {
                array_free(common);
                continue;
            }
            affinity_struct = ALLOC(AFFINITY_STRUCT, 1);
            affinity_struct->node1  = cube_node1;
            affinity_struct->node2  = cube_node2;
            affinity_struct->common = common;
            array_insert_last(AFFINITY_STRUCT *, affinity_vec, affinity_struct);
        }
    }
    /* pair up the cubes to maximize the total affinity. This is done by greedily
       selecting the cube-pairs with maximum affinity. Care is taken not to
       repeat the cubes already matched up earlier in the process.              */
    /*--------------------------------------------------------------------------*/
    array_sort(affinity_vec, pld_affinity_compare_function);

    /* store the matched-cubes in table */
    /*----------------------------------*/

    table = st_init_table(st_ptrcmp, st_ptrhash);

    for (i = 0; i < array_n(affinity_vec); i++) {
        affinity_struct = array_fetch(AFFINITY_STRUCT *, affinity_vec, i);
        cube_node1      = affinity_struct->node1;
        cube_node2      = affinity_struct->node2;

        /* check for already matched up cube either from node1 or node2*/
        /*-------------------------------------------------------------*/
        if ((st_is_member(table, (char *) cube_node1)) ||
            (st_is_member(table, (char *) cube_node2))) {
            xln_free_AFFINITY_STRUCT(affinity_struct);
            continue;
        }
        assert(st_lookup(cubenode2node_table, (char *) cube_node1, (char **) &n1));
        assert(st_lookup(cubenode2node_table, (char *) cube_node2, (char **) &n2));
        assert(network_check(n1->network));
        assert(!st_insert(table, (char *) cube_node1, (char *) cube_node1));
        assert(!st_insert(table, (char *) cube_node2, (char *) cube_node2));

        /* extract mergeable functions from the nodes n1 & n2 with
           cubes in affinity_struct. May consume other cubes also. */
        /*---------------------------------------------------------*/
        xln_extract_mergeable_fns_from_cubes(n1, n2, affinity_struct, table,
                                             node2cubenode_table, init_param);
        xln_free_AFFINITY_STRUCT(affinity_struct);
    }
    array_free(affinity_vec);
    st_free_table(table);
}

array_t *xln_node_find_common_inputs(node_t *node1, node_t *node2) {
    array_t *common_fanins;
    int     j;
    node_t  *fanin;

    common_fanins = array_alloc(node_t *, 0);
    foreach_fanin(node1, j, fanin) {
        if (node_get_fanin_index(node2, fanin) >= 0) {
            array_insert_last(node_t *, common_fanins, fanin);
        }
    }
    return common_fanins;
}

/*---------------------------------------------------------------------------
  Given two cubes (in the form of node1 & node2 in affinity_struct), extract
  more nodes out of them based on the mergeability cost function.
  The information about common inputs is present in affinity_struct.
  At each step, a subset of common inputs is selected that is the largest
  possible (limited by init_param->common_inputs). Corresponding literals
  are extracted from the nodes as new nodes and substituted in n1 (or n2).
  Some other literals may be put in the new nodes subject to the max inputs
  and union information along with these new nodes.
----------------------------------------------------------------------------*/
void xln_extract_mergeable_fns_from_cubes(node_t *n1, node_t *n2,
                                          AFFINITY_STRUCT *affinity_struct,
                                          st_table *table,
                                          st_table *node2cubenode_table,
                                          xln_init_param_t *init_param) {
    int     common_ptr;  /* points to the array location from where the next
                common fanin will be chosen */
    node_t  *node1;   /* affinity_struct->node1 */
    node_t  *node2;   /* affinity_struct->node2 */
    array_t *common; /* common fanins of node1 and node2 */
    array_t *subset_common;
    /* subset of common */
    array_t *nc1; /* array storing the fanins of node1 not in common*/
    array_t *nc2; /* array storing the fanins of node2 not in common*/
    int     nc1_ptr;  /* points to that nc1 entry from where a fanin
             may be selected to be put in the extracted node */
    int     nc2_ptr;  /* points to that nc2 entry from where a fanin
             may be selected to be put in the extracted node */
    int     j, num_subset_common, temp, temp1, temp2;
    int     fanin_bound_more1, fanin_bound_more2, bound_union_more, lower, remember;
    node_t  *fanin, *sub_node1, *sub_node2;

    node1  = affinity_struct->node1;
    node2  = affinity_struct->node2;
    common = affinity_struct->common;

    /* get the non-common fanins of node1 and node2 */
    /*----------------------------------------------*/
    nc1     = pld_get_non_common_fanins(node1, common);
    nc2     = pld_get_non_common_fanins(node2, common);
    nc1_ptr = nc2_ptr = 0;

    common_ptr = 0;
    while (common_ptr < array_n(common)) {

        /* get as many common fanins as allowed by MAX_COMMON_FANIN and common array
         */
        /*---------------------------------------------------------------------------*/
        subset_common = array_alloc(node_t *, 0);
        for (j        = 0;
             (j < init_param->MAX_COMMON_FANIN && common_ptr < array_n(common));
             j++, common_ptr++) {
            fanin = array_fetch(node_t *, common, common_ptr);
            array_insert_last(node_t *, subset_common, fanin);
        }

        /* extract node corresponding to the subset_common */
        /*-------------------------------------------------*/
        sub_node1         = xln_extract_supercube_from_cube(node1, subset_common);
        sub_node2         = xln_extract_supercube_from_cube(node2, subset_common);
        num_subset_common = array_n(subset_common);

        /* find bounds on "number of more fanins permissible" because
           of the MAX_FANIN constraint.                              */
        /*-----------------------------------------------------------*/
        temp = init_param->MAX_FANIN - num_subset_common;
        if (temp > 0) {
            fanin_bound_more1 = fanin_bound_more2 = temp;
        } else {
            fanin_bound_more1 = fanin_bound_more2 = 0;
        }

        /* make these bounds tighter based on the number of REMAINING fanins
           not in the common fanins                                         */
        /*------------------------------------------------------------------*/
        temp1 = array_n(nc1) - nc1_ptr;
        temp2 = array_n(nc2) - nc2_ptr;

        /* checks */
        /*--------*/
        assert(temp1 >= 0);
        assert(temp2 >= 0);

        fanin_bound_more1 = min(fanin_bound_more1, temp1);
        fanin_bound_more2 = min(fanin_bound_more2, temp2);

        /* find bound based on the MAX_UNION_FANIN constraint */
        /*----------------------------------------------------*/
        bound_union_more = init_param->MAX_UNION_FANIN - num_subset_common;

        /* compare the bounds and make them tight appropriately */
        /*------------------------------------------------------*/
        if (bound_union_more < fanin_bound_more1 + fanin_bound_more2) {
            if (fanin_bound_more1 < fanin_bound_more2) {
                lower    = fanin_bound_more1;
                remember = 1;
            } else {
                lower    = fanin_bound_more2;
                remember = 2;
            }
            if (lower >= bound_union_more) {
                fanin_bound_more1 = bound_union_more / 2;
                fanin_bound_more2 = bound_union_more - fanin_bound_more1;
            } else {
                if (remember == 1) {
                    fanin_bound_more2 =
                            min(bound_union_more - fanin_bound_more1, fanin_bound_more2);
                } else {
                    fanin_bound_more1 =
                            min(bound_union_more - fanin_bound_more2, fanin_bound_more1);
                }
            }
        }

        /* change the sub_node1 and sub_node2 (add more literals) */
        /*--------------------------------------------------------*/
        xln_add_more_literals(&sub_node1, node1, fanin_bound_more1, nc1, nc1_ptr);
        xln_add_more_literals(&sub_node2, node2, fanin_bound_more2, nc2, nc2_ptr);
        nc1_ptr += fanin_bound_more1;
        nc2_ptr += fanin_bound_more2;

        /* try to cover other cubes from the function into sub_node1 */
        /*-----------------------------------------------------------*/
        xln_add_more_cubes(&sub_node1, node1, n1, node2cubenode_table, table);
        xln_add_more_cubes(&sub_node2, node2, n2, node2cubenode_table, table);

        /* put the new nodes in the network */
        /*----------------------------------*/
        network_add_node(n1->network, sub_node1);
        network_add_node(n1->network, sub_node2);
        if (XLN_DEBUG) {
            (void) printf("---added the following pair---\n");
            node_print(stdout, sub_node1);
            node_print(stdout, sub_node2);
        }
        assert(network_check(n1->network));

        /* returns void since it may happen that some earlier sub_node
           that contained all the literals of sub_node1 (sub_node2) got
           substituted in n1 (n2).                                     */
        /*-------------------------------------------------------------*/
        (void) node_substitute(n1, sub_node1, 0);
        (void) node_substitute(n2, sub_node2, 0);
        array_free(subset_common);
    }
    array_free(nc2);
    array_free(nc1);
}

/*--------------------------------------------------------------------------
  Given a node having just 1 cube, and a subset of its fanins, smooths
  away all the inputs not present in the subset and returns the resulting
  node. Node does not change.
---------------------------------------------------------------------------*/
node_t *xln_extract_supercube_from_cube(node_t *node, array_t *subset_common) {
    node_t        *n, *n1, *n2, *fanin;
    int           i;
    input_phase_t phase;

    n      = node_constant(1);
    for (i = 0; i < array_n(subset_common); i++) {
        fanin = array_fetch(node_t *, subset_common, i);
        phase = node_input_phase(node, fanin);
        switch (phase) {
            case POS_UNATE:n1 = node_literal(fanin, 1);
                break;
            case NEG_UNATE:n1 = node_literal(fanin, 0);
                break;
            default:assert(0);
        }
        n2    = node_and(n, n1);
        node_free(n);
        node_free(n1);
        n = n2;
    }
    return n;
}

/*--------------------------------------------------------------------------
  Adds more literals to the node psub_node. These literals are from the
  node n. The literals to be added correspond to the fanins at nc[*pnc_ptr]
  to nc[*pnc_ptr + fanin_bound_more - 1]. The phases of these literals are
  the phases in the node.
---------------------------------------------------------------------------*/
void xln_add_more_literals(node_t **psub_node, node_t *node,
                           int fanin_bound_more, array_t *nc, int nc_ptr) {
    int           final, i;
    node_t        *fanin, *n1, *n2;
    input_phase_t phase;

    final  = nc_ptr + fanin_bound_more;
    for (i = nc_ptr; i < final; i++) {
        fanin = array_fetch(node_t *, nc, i);
        phase = node_input_phase(node, fanin);
        switch (phase) {
            case POS_UNATE:n1 = node_literal(fanin, 1);
                break;
            case NEG_UNATE:n1 = node_literal(fanin, 0);
                break;
            default:assert(0);
        }
        n2    = node_and(n1, *psub_node);
        node_free(n1);
        node_free(*psub_node);
        *psub_node = n2;
    }
}

xln_free_AFFINITY_STRUCT(AFFINITY_STRUCT
*affinity_struct) {
array_free(affinity_struct
->common);
FREE(affinity_struct);
}

/*----------------------------------------------------------
  Sorting in decreasing order of common fanins.
-----------------------------------------------------------*/
int pld_affinity_compare_function(AFFINITY_STRUCT **p1, AFFINITY_STRUCT **p2) {
    int num1, num2;

    num1 = array_n((*p1)->common);
    num2 = array_n((*p2)->common);
    if (num1 > num2)
        return (-1);
    if (num1 < num2)
        return 1;
    return 0;
}

/*----------------------------------------------------------------
  Given a network, returns an array of infeasible internal nodes,
  i.e., the ones with fanin higher than support.
-----------------------------------------------------------------*/
array_t *xln_infeasible_nodes(network_t *network, int support) {
    array_t *nodevec;
    lsGen   gen;
    node_t  *node;

    /* make an array of internal infeasible nodes */
    /*--------------------------------------------*/
    nodevec = array_alloc(node_t *, 0);
    foreach_node(network, gen, node) {
        if (node->type != INTERNAL)
            continue;
        if (node_num_fanin(node) <= support)
            continue;
        array_insert_last(node_t *, nodevec, node);
    }
    return nodevec;
}

/*-----------------------------------------------------------------------
  Given a set of nodes in the nodevec (from the network). For each node
  in nodevec, generate all cubes, and for each cube, make a node from
  it (the node has the same function as the cube) and store it in the
  cube_node_vec (if the number of inputs of the node is larger than
  init_param->cube_support_lower_limit, and establish a correspondence
  from it to the node in the cubenode2node_table. Returns cube_node_vec.
------------------------------------------------------------------------*/
array_t *xln_fill_tables(st_table *cubenode2node_table,
                         st_table *node2cubenode_table, array_t *nodevec,
                         xln_init_param_t *init_param) {
    array_t     *cube_node_vec;
    array_t     *cubevec;
    node_t      *node, *cube_node;
    node_cube_t cube;
    int         i, j;

    cube_node_vec = array_alloc(node_t *, 0);

    for (i = 0; i < array_n(nodevec); i++) {
        node = array_fetch(node_t *, nodevec, i);
        if (node->type != INTERNAL)
            continue;
        cubevec = array_alloc(node_t *, 0);
        for (j  = node_num_cube(node) - 1; j >= 0; j--) {
            cube      = node_get_cube(node, j);
            cube_node = pld_make_node_from_cube(node, cube);
            array_insert_last(node_t *, cubevec, cube_node);
            if (node_num_fanin(cube_node) < init_param->cube_support_lower_bound)
                continue;
            array_insert_last(node_t *, cube_node_vec, cube_node);
            assert(!st_insert(cubenode2node_table, (char *) cube_node, (char *) node));
        }
        assert(!st_insert(node2cubenode_table, (char *) node, (char *) cubevec));
    }
    return cube_node_vec;
}

void xln_free_cube_nodes(array_t *cube_node_vec) {
    int    i;
    node_t *cube_node;

    for (i = 0; i < array_n(cube_node_vec); i++) {
        cube_node = array_fetch(node_t *, cube_node_vec, i);
        /* just checking */
        /*---------------*/
        assert(cube_node->network == NIL(network_t));
        node_free(cube_node);
    }
}

/*-------------------------------------------------------------------------
  Given a sub_cube (psub_node) of the cube_node (which is a cube of the
  node n), see if psub_node can "consume" other cubes of the "original"
  node n: this just means that if there is a cube in the original cover
  of the node whose set of inputs is a subset of the inputs of this node,
  OR the function of the psub_node with that cube.
  A condition to be satisfied is that the cube should be the same as cube
  cube_node: i.e., psubnode has the same function as the cube_node.
---------------------------------------------------------------------------*/
void xln_add_more_cubes(node_t **psub_node, node_t *cube_node, node_t *n,
                        st_table *node2cubenode_table, st_table *table) {
    node_t  *sub_node, *sub_node1, *cube;
    int     nin_sub_node, nin_cube_node, i;
    array_t *cubenode_vec;
    int     is_fanin_subset;

    sub_node      = *psub_node;
    nin_sub_node  = node_num_fanin(sub_node);
    nin_cube_node = node_num_fanin(cube_node);
    if (nin_sub_node < nin_cube_node)
        return;

    assert(nin_sub_node == nin_cube_node);
    assert(st_lookup(node2cubenode_table, (char *) n, (char **) &cubenode_vec));
    for (i = 0; i < array_n(cubenode_vec); i++) {
        cube = array_fetch(node_t *, cubenode_vec, i);

        /* the next check takes care of the cube_node and the other
           cube_node with which it is being matched.               */
        /*---------------------------------------------------------*/
        if (st_is_member(table, (char *) cube))
            continue;
        is_fanin_subset = pld_is_fanin_subset(cube, sub_node);
        if (is_fanin_subset) {
            sub_node1 = node_or(sub_node, cube);
            node_free(sub_node);
            sub_node = sub_node1;
            assert(!st_insert(table, (char *) cube, (char *) cube));
        }
    }
    *psub_node = sub_node;
}

void xln_free_node2cubenode_table(st_table *node2cubenode_table) {
    char         *key;
    st_generator *stgen;
    array_t      *cubevec;

    st_foreach_item(node2cubenode_table, stgen, &key, (char **) &cubevec) {
        array_free(cubevec);
    }
    st_free_table(node2cubenode_table);
}
