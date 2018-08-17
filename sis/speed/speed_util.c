
#include <stdio.h>
#include "sis.h"
#include "../include/speed_int.h"

static st_table *level_table; /* For the levelize routines */
int level_cmp();

/*
 * Routine to form a node with the set as the i'th 
 * cube of the node. 
 */
node_t *
speed_dec_node_cube(f, i)
        node_t *f;
        int i;
{
    node_cube_t cube;
    node_t      *c, *fanin, *tlit, *t;
    int         j;

    c    = node_constant(1);
    cube = node_get_cube(f, i);
    foreach_fanin(f, j, fanin)
    {
        switch (node_get_literal(cube, j)) {
            case ZERO: tlit = node_literal(fanin, 0);
                t           = node_and(c, tlit);
                node_free(tlit);
                node_free(c);
                c = t;
                break;
            case ONE: tlit = node_literal(fanin, 1);
                t          = node_and(c, tlit);
                node_free(tlit);
                node_free(c);
                c = t;
        }
    }

    return c;
}

/*
 * Deletes the cubes with inputs whose arrival
 * times are greater than the threshold.
 */
void
speed_del_critical_cubes(node, speed_param, thresh)
        node_t *node;
        speed_global_t *speed_param;
        double thresh;
{
    node_t       *cube, *new, *new_node, *fanin;
    int          i, j, crit_flag;
    delay_time_t time;
    double       t;

    new    = node_constant(0);
    for (i = 0; i < node_num_cube(node); i++) {
        crit_flag = FALSE;
        cube      = speed_dec_node_cube(node, i);
        foreach_fanin(cube, j, fanin)
        {
            speed_delay_arrival_time(fanin, speed_param, &time);
            t = D_MAX(time.rise, time.fall);
            if (t > thresh) {
                crit_flag = TRUE;
            }
        }
        if (!crit_flag) {
            new_node = node_or(new, cube);
            node_free(new);
            new = new_node;
        }
        node_free(cube);
    }
    node_replace(node, new);
}

/*
 * name_to_node -- returns the pointer to the fanin
 * of a node with a given name .
 */

node_t *
name_to_node(node, name)
        node_t *node;
        char *name;
{
    node_t *fanin;
    int    i;

    foreach_fanin(node, i, fanin)
    {
        if (strcmp(node_long_name(fanin), name) == 0) {
            return fanin;
        }
    }
    return NIL(node_t);
}

int
speed_print_level(network, node_array, thresh, crit_only, flag)
        network_t *network;
        array_t *node_array;
        double thresh;        /* Threshold specifying the critical path */
        int crit_only;        /* == 1 => print only critical nodes */
        int flag;        /* == 1 => print all levels, == 0 => silent mode */
{
    lsGen        gen;
    node_t       *p;
    char         *dummy;
    st_table     *node_table;
    delay_time_t slack;
    int          i, j, level, new_level;
    array_t      *nodevec, *temp_array;

    level_table = st_init_table(st_ptrcmp, st_ptrhash);

    foreach_node(network, gen, p)
    {
        (void) st_insert(level_table, (char *) p, (char *) 0);
    }

    foreach_primary_output(network, gen, p)
    {
        (void) assign_level(p);
    }

    nodevec = network_dfs_from_input(network);
    array_sort(nodevec, level_cmp);

    if (node_array == NIL(array_t)) {
        /* All nodes can be printed */
        node_table = level_table;
    } else {
        /* Get the nodes to be printed */
        node_table = st_init_table(st_ptrcmp, st_ptrhash);
        for (i     = 0; i < array_n(node_array); i++) {
            p = array_fetch(node_t * , node_array, i);
            (void) st_insert(node_table, (char *) p, NIL(
            char));
            temp_array = network_tfi(p, INFINITY);
            for (j     = array_n(temp_array); j-- > 0;) {
                p = array_fetch(node_t * , temp_array, j);
                (void) st_insert(node_table, (char *) p, NIL(
                char));
            }
            array_free(temp_array);
        }
    }

    level = 0;
    if (flag)
        (void) fprintf(sisout, "%3d:", level);
    for (i = 0; i < array_n(nodevec); i++) {
        p = array_fetch(node_t * , nodevec, i);
        if (p->type != PRIMARY_OUTPUT && st_is_member(node_table, (char *) p)) {
            slack = delay_slack_time(p);
            if (crit_only &&
                slack.rise > thresh && slack.fall > thresh)
                continue;
            if (st_lookup(level_table, (char *) p, &dummy)) {
                new_level = (int) dummy;
                if (new_level > level) {
                    level = new_level;
                    if (flag)
                        (void) fprintf(sisout, "\n%3d:", level);
                }
                if (flag)
                    (void) fprintf(sisout, " %s", node_name(p));
            } else {
                if (flag)
                    (void) fprintf(sisout, "Node %s not assigned a level",
                                   node_name(p));
            }
        }
    }
    if (flag) (void) fprintf(sisout, " \n");
    array_free(nodevec);
    st_free_table(level_table);
    if (node_array != NIL(array_t)) st_free_table(node_table);

    return level;
}

int
assign_level(node)
        node_t *node;
{
    node_t *fanin;
    int    i, level, new_level;
    char   *dummy;

    (void) st_lookup(level_table, (char *) node, &dummy);
    new_level = (int) dummy;
    if (node->type == PRIMARY_INPUT || new_level > 0) {
        return new_level;
    } else {
        level = 0;
        foreach_fanin(node, i, fanin)
        {
            level = MAX(level, assign_level(fanin));
        }
    }
    level++;
    (void) st_insert(level_table, (char *) node, (char *) level);

    return level;
}

int
level_cmp(p1, p2)
        char **p1, **p2;
{
    int  level1, level2;
    char *dummy;

    (void) st_lookup(level_table, *p1, &dummy);
    level1 = (int) dummy;
    (void) st_lookup(level_table, *p2, &dummy);
    level2 = (int) dummy;
    return (level1 - level2);
}


st_table *
speed_levelize_crit(network, speed_param, max_levelp)
        network_t *network;
        speed_global_t *speed_param;
        int *max_levelp;    /* RETURN: the maximum level in the circuit */
{
    array_t  *nodevec;
    lsGen    gen;
    node_t   *p;
    int      i, level, new_level;
    char     *dummy;
    st_table *table;

    level_table = st_init_table(st_ptrcmp, st_ptrhash);
    table       = st_init_table(st_ptrcmp, st_ptrhash);

    foreach_node(network, gen, p)
    {
        (void) st_insert(level_table, (char *) p, (char *) 0);
    }

    foreach_primary_output(network, gen, p)
    {
        (void) assign_level(p);
    }

    nodevec = network_dfs_from_input(network);
    array_sort(nodevec, level_cmp);

    level  = -1;
    for (i = 0; i < array_n(nodevec); i++) {
        p = array_fetch(node_t * , nodevec, i);
        if ((p->type != PRIMARY_OUTPUT) && (speed_critical(p, speed_param))) {
            if (st_lookup(level_table, (char *) p, &dummy)) {
                new_level = (int) dummy;
                if (new_level > level) {
                    level = new_level;
                }
                (void) st_insert(table, (char *) p, (char *) level);
            } else {
                (void) fprintf(siserr, "Node %s not assigned a level\n",
                               node_name(p));
            }
        }
    }
    array_free(nodevec);
    st_free_table(level_table);

    *max_levelp = level;
    return table;
}

/*
 * Compute the area of a network
 */
double
sp_get_netw_area(network)
        network_t *network;
{
    lsGen  gen;
    int    mapped_flag;
    double area;
    node_t *node;

    if (network == NIL(network_t)) return 0.0;

    area        = 0;
    mapped_flag = lib_network_is_mapped(network);
    foreach_node(network, gen, node)
    {
        if (node->type == INTERNAL) {
            if (mapped_flag) {
                area += lib_gate_area(lib_gate_of(node));
            } else {
                area += factor_num_literal(node);
            }
        }
    }
    return area;
}

void
speed_get_stats(network, library, model, delay, area)
        network_t *network;
        library_t *library;
        delay_model_t model;
        double *delay;
        double *area;
{
    node_t    *np;
    lsGen     gen;
    network_t *new_net;

    assert(delay_trace(network, model));
    (void) delay_latest_output(network, delay);

    /*
     * Add inverters and map the network
     */

    *area = 0.0;
    new_net = network_dup(network);
    add_inv_network(new_net);
    new_net = map_network(new_net, library, 1.0, 1, 3);
    foreach_node(new_net, gen, np)
    {
        *area += lib_get_gate_area(np);
    }
    network_free(new_net);
}

void
set_speed_thresh(network, speed_param)
        network_t *network;
        speed_global_t *speed_param;
{
    delay_time_t time;
    double       my_thresh, min_t;
    node_t       *po;
    lsGen        gen;

    my_thresh = POS_LARGE;
    foreach_primary_output(network, gen, po)
    {
        time      = delay_slack_time(po);
        min_t     = D_MIN(time.rise, time.fall);
        my_thresh = D_MIN(my_thresh, min_t);
    }
    if (my_thresh > 1.0e-6 /* For floating point comparisons */ ) {
        /*
         * All output nodes meet the required
         * time costraints.
         */
        speed_param->crit_slack = -1.0;
    } else {
        /*
         * Set the threshold for the epsilon
         * network to be within thresh of the most
         * most critical node.
         */
        speed_param->crit_slack = my_thresh + speed_param->thresh;
    }
}

void
speed_delete_single_fanin_node(network, node)
        network_t *network;
        node_t *node;
{
    int     i;
    node_t  *fo;
    lsGen   gen;
    array_t *array;

    if (node_num_fanin(node) <= 1) {
        array = array_alloc(node_t * , 0);
        foreach_fanout(node, gen, fo)
        {
            array_insert_last(node_t * , array, fo);
        }
        /*
         * Now collapse the node into its fanouts.
         */
        for (i = 0; i < array_n(array); i++) {
            fo = array_fetch(node_t * , array, i);
            (void) node_collapse(fo, node);
        }
        /*
         * Free the node if it has no fanouts.
         */
        if (node_num_fanout(node) == 0)
            speed_network_delete_node(network, node);
        array_free(array);
    }
}

void
speed_network_delete_node(network, node)
        network_t *network;
        node_t *node;
{
    int     i;
    node_t  *fanin;
    array_t *nodevec;

    nodevec = array_alloc(node_t * , 0);

    foreach_fanin(node, i, fanin)
    {
        if (node_num_fanout(fanin) == 1) {
            array_insert_last(node_t * , nodevec, fanin);
        }
    }

    network_delete_node(network, node);

    for (i = 0; i < array_n(nodevec); i++) {
        fanin = array_fetch(node_t * , nodevec, i);
        speed_network_delete_node(network, fanin);
    }

    array_free(nodevec);
}

/* For the purposes of buffering we may need buffers from the library */
lib_gate_t *
sp_lib_get_buffer(lib)
        library_t *lib;
{
    network_t   *network;
    lib_class_t *class;
    lib_gate_t  *gate;

    if (lsLength(lib->patterns) > 0) { /* Should be a macro */
        /* The patterns were generated */
        network = read_eqn_string("f = a;");
        assert(network != NIL(network_t));
        class = lib_get_class(network, lib);
        network_free(network);
        if (class == NIL(lib_class_t)) {
            gate = NIL(lib_gate_t);
        } else {
            gate = sp_get_gate(class, 0);
        }
    } else {
        gate = lib_get_gate(lib, MIN_AREA_BUF_NAME);
    }
    return gate;
}

/*
 * Get the min size iverter from the library description
 */
lib_gate_t *
sp_lib_get_inv(lib)
        library_t *lib;
{
    network_t   *network;
    lib_class_t *class;
    lib_gate_t  *gate;

    if (lsLength(lib->patterns) > 0) { /* Should be a macro */
        /* The patterns were generated */
        network = read_eqn_string("f = a';");
        assert(network != NIL(network_t));
        class = lib_get_class(network, lib);
        network_free(network);
        if (class == NIL(lib_class_t)) {
            gate = NIL(lib_gate_t);
        } else {
            gate = sp_get_gate(class, 0);
        }
    } else {
        gate = lib_get_gate(lib, MIN_AREA_BUF_NAME);
    }
    return gate;
}

lib_gate_t *
sp_get_gate(class, which)
        lib_class_t *class;
        int which;        /* 0 = smallest, 1 = biggest */
{
    lib_gate_t *gate;
    lsGen      gen;
    char       *dummy;

    if (class == NIL(lib_class_t)) return NIL(lib_gate_t);

    gen = lib_gen_gates(class);
    if (lsNext(gen, &dummy, LS_NH) != LS_OK) {
        fail("Error, No inverters in library\n");
    }
    gate = (lib_gate_t *) dummy;
    while (lsNext(gen, &dummy, LS_NH) == LS_OK) {
        if (which) {
            if (lib_gate_area((lib_gate_t *) dummy) > lib_gate_area(gate))
                gate = (lib_gate_t *) dummy;
        } else {
            if (lib_gate_area((lib_gate_t *) dummy) < lib_gate_area(gate))
                gate = (lib_gate_t *) dummy;
        }
    }
    (void) lsFinish(gen);

    return gate;
}
