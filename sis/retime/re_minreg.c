
#ifdef SIS

#include "retime_int.h"
#include "sis.h"

#define SCALE 1000

static void re_setup_lp_tableau();

static void re_adjust_user_delays();

static void re_add_register_sharing_node();

/*
 * Minimize the number of registers while trying to achieve the
 * cycle time of "c"... returns 1 if there is a feasible solution
 * The graph is modified according to the feasible retiming
 *
 * Since the delay values need not be integral.. We will scale the
 * values of delay by a factor SCALE and then floor the delay values
 */

int retime_min_register(re_graph *graph, double c, int *retiming) {
    wd_t     **WD;
    double   **a;
    re_node  *re_no, *new_node;
    st_table *table;
    int      r_host; /* Retiming at the host vertex */
    int      *iposv, *izrov;
    int      i, N, m, m1, m2, m3;
    int      status, flag, scaled_c, index, lag;

    /* Add the dummy node to the retime graph to model register sharing */
    re_add_register_sharing_node(graph);

    /*
     * Create a table of correspondence between nodes and variables in lp
     * The index "N-1" represents the host(source) vertex and the index
     * "N" represents the host (sink) vertex.
     */
    N     = 0;
    table = st_init_table(st_numcmp, st_numhash);
    re_foreach_node(graph, i, re_no) {
        if (re_no->type == RE_INTERNAL || re_no->type == RE_IGNORE) {
            (void) st_insert(table, (char *) N, (char *) re_no);
            re_no->lp_index = N;
            N++;
        } else if (re_no->user_time > RETIME_TEST_NOT_SET) {
            re_no->scaled_user_time = (int) ceil(re_no->user_time * SCALE);
        }
        /* Scale the delay values */
        re_no->scaled_delay = (int) ceil(re_no->final_delay * SCALE);
    }

    re_foreach_node(graph, i, re_no) {
        if (re_no->type == RE_PRIMARY_INPUT) {
            re_no->lp_index = N;
        } else if (re_no->type == RE_PRIMARY_OUTPUT) {
            re_no->lp_index = N + 1;
        }
    }
    N += 2; /* For the two host vertices */

    /* adjust the delays according to the user specified delays */
    re_adjust_user_delays(graph);

    /* Setup the W and D matrices */
    re_computeWD(graph, N, table, &WD);

    /* Generate the tableau for the linear programming */
    scaled_c = (int) ceil(SCALE * c);
    re_setup_lp_tableau(graph, N, WD, scaled_c, &a, &m, &m1, &m2, &m3, &r_host);

    for (i = N; i-- > 0;)
        FREE(WD[i]);
    FREE(WD);

    /* Solve the linear program */
    iposv = ALLOC(int, m + 1);
    izrov = ALLOC(int, N + 1);
    if (re_simplx(a, m, N, m1, m2, m3, &flag, izrov, iposv)) {
        fail(error_string());
    }

    if (!flag) {
        status = 1;
        if (retime_debug > 20) {
            (void) fprintf(sisout, "SIMPLEX -- optimum value = %f\n", a[1][1]);
        }

        for (i = re_num_nodes(graph); i-- > 0;)
            retiming[i] = 0;
        for (i = 1; i <= m; i++) {
            if (retime_debug > 60) {
                (void) fprintf(sisout, "%-4d: iposv(%d) = %6.3f -- %8s \n", i, iposv[i],
                               a[i + 1][1], (iposv[i] > N ? "SLACK" : "VARIABLE"));
            }
            /*
             * The indices are numbered 0 to (N-1) -- not 1 to N
             * We are interested only in the variables (not slack data)
             * and the variable indices N,N-1 are actually host. Since those
             * have a retiming equal to 0 and do not have an entry in the
             * symbol_table ignore it.
             */
            if (iposv[i] < N - 1) {
                index = iposv[i] - 1;
                assert(st_lookup(table, (char *) index, (char **) &new_node));
                retiming[new_node->id] = (int) ceil((double) (a[i + 1][1])) - r_host;
            }
        }

        if (retime_debug)
            (void) fprintf(sisout, "\tRetiming following nodes\n\t");
        re_foreach_node(graph, i, re_no) {
            if (re_no->type == RE_INTERNAL) {
                lag = retiming[re_no->id];
                if (retime_debug && lag != 0) {
                    (void) fprintf(sisout, "%s by %d  ", re_no->node->name, lag);
                }
                retime_single_node(re_no, lag);
            }
        }
        if (retime_debug)
            (void) fprintf(sisout, "\n");
    } else {
        status = 0;
        if (retime_debug) {
            (void) fprintf(
                    sisout, "LP HAS %s\n",
                    (flag == -1 ? "NO FEASIBLE SOLUTION" : "UNBOUNDED SOLUTION"));
        }
    }

    /* Garbage collection */
    for (i = m + 3; i-- > 0;)
        FREE(a[i]);
    FREE(a);
    FREE(iposv);
    FREE(izrov);
    st_free_table(table);

    return status;
}

/*
 * add nodes to appropriately model sharing of registers ---
 * Do this for multi-fanout nodes -- includeing primary inputs
 * This is tricky since the source (PI vertex) is actually a
 * bunch of vertices....
 */
static void re_add_register_sharing_node(re_graph *graph) {
    array_t *array;
    re_edge *re_ed, *edge;
    re_node *re_no, *new_node;
    int     i, j, max_weight, num_fan;

    /*
     * For each node with multiple fanout --
     * Add a dummy node for the correct modelling of register sharing
     */
    array = array_alloc(re_node *, 0);
    re_foreach_node(graph, i, re_no) {
        if (re_num_fanouts(re_no) > 1) {
            array_insert_last(re_node *, array, re_no);
        }
    }

    if (retime_debug > 40) {
        (void) fprintf(sisout, "%d nodes added to model register cost\n",
                       array_n(array));
    }

    /*
     * Now look over all the multi-fanout nodes and modify the breadth
     * as well as add edges from its fanouts to the dummy node
     */

    for (i = array_n(array); i-- > 0;) {
        re_no    = array_fetch(re_node *, array, i);
        new_node = retime_alloc_node();
        new_node->node       = NIL(node_t);
        new_node->id         = re_num_nodes(graph);
        new_node->type       = RE_IGNORE;
        new_node->final_area = new_node->final_delay = 0.0;

        /* First figure out the true fanout and  maximum weight */
        max_weight = NEG_LARGE;
        num_fan    = 0;
        re_foreach_fanout(re_no, j, re_ed) {
            if (re_ignore_edge(re_ed))
                continue;
            num_fan++;
            max_weight = MAX(max_weight, re_ed->weight);
            re_ed->temp_breadth = re_ed->breadth;
        }

        re_foreach_fanout(re_no, j, re_ed) {
            if (re_ignore_edge(re_ed))
                continue;

            /* Modify the breadth of the edge */
            re_ed->temp_breadth = re_ed->breadth / num_fan;

            /* Add an edge from the curent node to the dummy node */
            edge =
                    re_create_edge(graph, re_ed->sink, new_node, re_num_fanins(new_node),
                                   max_weight - re_ed->weight, 1.0);
            edge->temp_breadth = edge->breadth / num_fan;
        }
        /* Add the new_node as part of the graph */
        array_insert_last(re_node *, graph->nodes, new_node);
    }
    array_free(array);
}

/*
 * First figure the number of constraints
 * 2 equality constraint (r(host) = CONST) -- for appropraite scaling
 * num_edge constraints of the type r(u) - r(v) <= w(e) for edges u->v
 * ??? of type r(u) - r(v) <= W(u,v)-1 whenever D(u,v) > c
 * Since the rhs of simplex is always positive, we need see if
 * W(u,v)-1 is negative and if so, then the corresponding constraint has
 * to be multiplied by -1
 */
static void
re_setup_lp_tableau(re_graph *graph, int N, wd_t **WD, int c, double ***ap, int *mp, int *m1p, int *m2p, int *m3p,
                    int *trans) {
    int     i, j, k, t;
    int     m, m1, m2, m3;
    int     num_edges, cur_m1, cur_m2;
    re_edge *re_ed;
    re_node *re_no;
    double  **a;

    num_edges = re_num_edges(graph);
    m1        = num_edges; /* <= constraints */
    m2        = 0;         /* >= constraints */
    m3        = 2;         /* == constraints -- retiming at host */

    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {
            if (WD[i][j].w == POS_LARGE) /* No path fron i to j */
                continue;
            if (WD[i][j].d > c) { /* Need to add aconstraint */
                if (WD[i][j].w < 1)
                    m2++;
                else
                    m1++;
            }
        }
    }
    m      = m1 + m2 + m3;
    *trans = num_edges * re_sum_of_edge_weight(graph);

    /* Now allocate the a matrix --- for the tableau */
    a      = ALLOC(double *, m + 3);
    for (i = m + 3; i-- > 0;) {
        a[i] = ALLOC(double, N + 2);
        for (j = N + 2; j-- > 0;) {
            a[i][j] = 0.0;
        }
    }
    /* Now fillin the entries of the tableau */
    cur_m1 = 2;
    cur_m2 = m1 + 2;
    re_foreach_edge(graph, k, re_ed) {
        /* edge (i,j) */
        i            = re_ed->source->lp_index;
        j            = re_ed->sink->lp_index;
        /* Add the constraint r(i) - r(j) <= w(e) */
        a[cur_m1][i + 2] -= 1.0;
        a[cur_m1][j + 2] += 1.0;
        a[cur_m1][1] = re_ed->weight;
        cur_m1++;
    }
    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {
            if (WD[i][j].w == POS_LARGE)
                continue; /* No path from i to j */
            /*
             * The constraint: 0 <= (W(i,j)-1) - r(i) + r(j), if D(i,j) > c
             */
            if (WD[i][j].d > c) {
                t = WD[i][j].w - 1;
                if (t < 0) {
                    a[cur_m2][i + 2] += 1.0;
                    a[cur_m2][j + 2] -= 1.0;
                    a[cur_m2][1] -= (double) t;
                    cur_m2++;
                } else {
                    a[cur_m1][i + 2] -= 1.0;
                    a[cur_m1][j + 2] += 1.0;
                    a[cur_m1][1] += (double) t;
                    cur_m1++;
                }
            }
        }
    }

    /* Retiming at the host (index N and N-1) is set to 0 */
    a[m][1]         = *trans;
    a[m][N]         = -1.0; /* Source HOST */
    a[m + 1][1]     = *trans;
    a[m + 1][N + 1] = -1.0; /* Sink HOST */

    /*
     * Now we need to setup the cost function. For the ith node the
     * weight is sum_over_(i->?)(b(e)) - sum_over_(?->i)(b(e)).
     * Note: since the simplex solves a max problem we have inverted the
     * coefficient of the ith variable
     */
    re_foreach_node(graph, k, re_no) {
        i = re_no->lp_index;
        re_foreach_fanout(re_no, j, re_ed) { a[1][i + 2] += re_ed->temp_breadth; }
        re_foreach_fanin(re_no, j, re_ed) { a[1][i + 2] -= re_ed->temp_breadth; }
    }

    if (retime_debug > 40) {
        (void) fprintf(sisout, "Graph data  :: %d internal nodes, %d edges\n",
                       re_num_internals(graph), num_edges);
        (void) fprintf(sisout,
                       "Simplex data:: %d variable, %d=%d+%d+%d constraints\n", N, m,
                       m1, m2, m3);
        (void) fprintf(sisout, "Translation of the retiming = %d\n", *trans);
        (void) fprintf(sisout, "Objective function \n");
        for (i = 2; i <= N + 1; i++) {
            (void) fprintf(sisout, "%s%5.3f r%d ", (a[1][i] >= 0 ? "+" : "-"),
                           ABS(a[1][i]), i - 2);
        }
        (void) fprintf(sisout, "\n");
        /*
        for ( i = 0; i < m; i++){
            if (i == num_edges)
                (void)fprintf(sisout,"End of edge constraints\n");
            (void)fprintf(sisout,"Eqn %-5d is: ", i);
            sign = (i < m1 ? "<=" : (i >= (m1+m2) ? "=" : ">="));
            for (j = 0; j < N; j++){
            if (a[i+2][j+2] != 0) (void)fprintf(sisout,"%s%2.0f r%d ",
                (a[i+2][j+2] >= 0 ? "+":"-"), ABS(a[i+2][j+2]), j);
            }
            (void)fprintf(sisout," %s %6.2f\n", sign, a[i+2][1]);
        }
        */
    }

    *ap  = a;
    *mp  = m;
    *m1p = m1;
    *m2p = m2;
    *m3p = m3;

    return;
}

/*
 * If the PI/PO nodes haver user specified delays, then we modify
 * the delays of immidiate fanouts/fanins to account for the
 * user-specified delays... In effect the delay of the node is the
 * composite if the node's delay and the delay through a fictitious
 * node whose delay equals the user specified delay at PI/PO
 */
static void re_adjust_user_delays(re_graph *graph) {
    int     i, j;
    re_edge *re_ed;
    re_node *re_no;

    /* Do the PRIMARY INPUTS */
    /* Right now we account for PI arrival times in the WD computation */
    /*
    pipo_array = array_alloc(re_node *, 0);
    pipo_table = st_init_table(st_ptrcmp, st_ptrhash);
    re_foreach_primary_input(graph, i, re_no){
       if (re_no->user_time > RETIME_TEST_NOT_SET){
       re_foreach_fanout(re_no, j, re_ed){
           if (re_ed->sink->type == RE_INTERNAL &&
               !st_insert(pipo_table, (char *)(re_ed->sink), NIL(char))){
           array_insert_last(re_node *, pipo_array, re_ed->sink);
           }
       }
       }
    }
    for (i = array_n(pipo_array); i-- > 0; ){
    time = RETIME_USER_NOT_SET;
    re_no = array_fetch(re_node *, pipo_array, i);
    re_foreach_fanin(re_no, j, re_ed){
        if (re_ed->source->type == RE_PRIMARY_INPUT &&
            re_ed->source->user_time > RETIME_USER_NOT_SET){
        time = MAX(time, re_ed->source->scaled_user_time);
        }
    }
    assert(time > RETIME_TEST_NOT_SET);
    re_no->scaled_delay += time;
    }
    array_free(pipo_array);
    st_free_table(pipo_table);
    */

    /* Now do the PRIMARY OUTPUTS --- these have a single fanin */
    re_foreach_primary_output(graph, i, re_no) {
        if (re_no->user_time > RETIME_TEST_NOT_SET) {
            re_foreach_fanin(re_no, j, re_ed) {
                re_ed->source->scaled_delay -= re_no->scaled_user_time;
            }
        }
    }
}

#endif /* SIS */
