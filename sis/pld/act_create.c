#include "pld_int.h"
#include "sis.h"

int MAXOPTIMAL;

/*	constructing the local and global ACTs
 */

/*	used in applyCreate	*/
typedef struct tree_entry_defn {
    int            buf;
    ACT_VERTEX_PTR dag;
}          tree_t;

void p_actCreate4Set(array_t *node_vec, array_t *node_list, int locality,
                     int order_style, float mode, network_t *network,
                     array_t *delay_values, st_table *cost_table);

ACT_VERTEX_PTR p_actCreateStep(node_t *current_node, int level, array_t *node_list, int locality);

ACT_VERTEX_PTR p_optimalDag(node_t *current_node, array_t *node_list, float mode,
                            network_t *network, array_t *delay_values, st_table *cost_table);

void p_permute(node_t *current_node, array_t *list, int n, int *best_cost_ptr,
               double *best_cost_and_delay_ptr, array_t **pbest_list,
               float mode, network_t *network, array_t *delay_values, st_table *cost_table);

void p_applyCreate(node_t *node, array_t *order_list);

void act_print_array(array_t *list);

ACT_VERTEX_PTR p_treeNodeDag(node_t *node, st_table *tree_table, array_t *order_list);

ACT_VERTEX_PTR p_terminalDag(int value, int index_size);

void p_actCreate4Set(array_t *node_vec, array_t *node_list, int locality,
                     int order_style, float mode, network_t *network,
                     array_t *delay_values, st_table *cost_table) {
    node_t  *current_node;
    int     i, j, num_fanin, input_size, com_local;
    ACT_PTR act;
    //ACT_VERTEX_PTR p_optimalDag(), p_actCreateStep(), actReduce();

    input_size = array_n(node_vec);
    if (node_list == NIL(array_t)) {
        com_local = 1;
    } else {
        com_local = 0;
    }
    for (i     = 0; i < input_size; i++) {
        current_node = array_fetch(node_t *, node_vec, i);
        if (node_function(current_node) != NODE_PO) {
            if (locality) {
                if (com_local) {
                    node_list = array_alloc(node_t *, 0);
                    num_fanin = node_num_fanin(current_node);
                    for (j    = 0; j < num_fanin; j++) {
                        array_insert(node_t *, node_list, j,
                                     node_get_fanin(current_node, j));
                    }
                }
                act = ALLOC(ACT, 1);
                act->node_name = util_strsav(node_name(current_node));

                switch (order_style) {
                    case OPTIMAL:
                        act->root = p_optimalDag(current_node, node_list, mode, network, delay_values, cost_table);
                        break;
                    case RANDOM:node_list = pld_shuffle(node_list);
                        act->root = p_actCreateStep(current_node, 0, node_list, 1);
                        break;
                    default: /* Fanin */
                        act->root = p_actCreateStep(current_node, 0, node_list, 1);
                        break;
                }

                act->root      = actReduce(act->root);
                act->node_list = node_list;
                ACT_SET(current_node)->LOCAL_ACT = ALLOC(ACT_ENTRY, 1);
                ACT_SET(current_node)->LOCAL_ACT->act = act;
                ACT_SET(current_node)->LOCAL_ACT->order_style =
                        (num_fanin <= MAXOPTIMAL) ? order_style : FANIN;
            } else {
                p_applyCreate(current_node, node_list);
                ACT_SET(current_node)->GLOBAL_ACT->order_style = order_style;
            }
        }
    }
}

ACT_VERTEX_PTR p_actCreateStep(node_t *current_node, int level, array_t *node_list, int locality) {
    int             index_size;
    ACT_VERTEX_PTR  u;
    node_function_t func;
    node_t          *p, *q, *r, *low_node, *high_node, *t_node;

    index_size = array_n(node_list);
    u          = ALLOC(ACT_VERTEX, 1);
    u->id          = 0;
    u->mark        = 0;
    u->index_size  = index_size;
    u->node        = NIL(node_t);
    u->name        = NIL(char);
    u->multiple_fo = 0;
    u->cost        = 0;
    u->mapped      = 0;
    func = node_function(current_node);
    if (func == NODE_0 || func == NODE_1) {

        u->value = (func == NODE_0) ? 0 : 1;
        u->low   = NIL(ACT_VERTEX);
        u->high  = NIL(ACT_VERTEX);
        u->index = index_size;
        return (u);
    } else {
        u->value = NO_VALUE;
        level--;
        do {
            level++;
            if (level >= array_n(node_list)) {
                u->value = 1;
                u->low   = NIL(ACT_VERTEX);
                u->high  = NIL(ACT_VERTEX);
                u->index = index_size;
                return (u);
            }
            t_node = array_fetch(node_t *, node_list, level);
            if (node_get_fanin_index(current_node, t_node) != -1) {
                node_algebraic_cofactor(current_node, t_node, &p, &q, &r);
                low_node  = node_or(q, r);
                high_node = node_or(p, r);
                simplify_node(low_node, SIM_METHOD_SNOCOMP, SIM_DCTYPE_NONE,
                              SIM_FILTER_NONE, SIM_ACCEPT_SOP_LITS);
                simplify_node(high_node, SIM_METHOD_SNOCOMP, SIM_DCTYPE_NONE,
                              SIM_FILTER_NONE, SIM_ACCEPT_SOP_LITS);
                node_free(p);
                node_free(q);
                node_free(r);
                if (my_node_equal(low_node, high_node)) {
                    node_free(low_node);
                    node_free(high_node);
                } else
                    break;
            }
        } while (1);

        u->index = level;
        level++;
        u->low = p_actCreateStep(low_node, level, node_list, locality);
        node_free(low_node);
        u->high = p_actCreateStep(high_node, level, node_list, locality);
        node_free(high_node);
        return (u);
    }
}

/*	optimal ACT using brute force, only for local ACT
 */

ACT_VERTEX_PTR p_optimalDag(node_t *current_node, array_t *node_list, float mode,
                            network_t *network, array_t *delay_values, st_table *cost_table) {
    int     best_cost, n, j;
    double  best_cost_and_delay; /* best_delay added later- July 5, 90 -- Rajeev*/
    array_t *best_list;
    node_t  *t_node;

    /*	brute force optimal is only done for fanin size <= MAXOPTIMAL
            otherwise the fanin ordered ACT is generated	*/

    if (array_n(node_list) > MAXOPTIMAL) {
        (void) fprintf(
                sisout,
                "Optimal ordering too expensive for node %s, fanin ordering chosen\n",
                node_name(current_node));
        return (p_actCreateStep(current_node, 0, node_list, 1));
    } else {
        best_cost           = HICOST;
        best_cost_and_delay = (double) HICOST;
        best_list           = array_alloc(node_t *, 0);
        n                   = array_n(node_list);
        p_permute(current_node, node_list, n, &best_cost, &best_cost_and_delay,
                  &best_list, mode, network, delay_values, cost_table);
        for (j = 0; j < n; j++) {
            t_node = array_fetch(node_t *, best_list, j);
            array_insert(node_t *, node_list, j, t_node);
        }
        array_free(best_list);

        /* remove       */
        /* Rajeev put the next comment */
        /*                (void) fprintf(sisout, " Final Check: ");
        for(j = 0; j<n; j++){
                (void) fprintf(sisout,
                " %s ", node_name(array_fetch(node_t *, node_list, j)));
                (void) fprintf(sisout, "\n");
        } */

        /*  end remove */

        return (p_actCreateStep(current_node, 0, node_list, 1));
    }
}

/*	permutation routine from kruse
 */

void p_permute(node_t *current_node, array_t *list, int n, int *best_cost_ptr,
               double *best_cost_and_delay_ptr, array_t **pbest_list,
               float mode, network_t *network, array_t *delay_values,
               st_table *cost_table) {
    int            c, cost;
    double         arrival_time, this_cost_and_delay;
    node_t         *t, *t1;
    ACT_VERTEX_PTR dag;
    COST_STRUCT    *cost_node;
    array_t        *best_list;

    best_list = *pbest_list;
    c         = 1;
    if (n > 2) {
        /* changed - July 5, 90 */
        /*----------------------*/
        /* if (*best_cost_ptr <= 1) ; */

        if ((mode == AREA) && (*best_cost_ptr <= 1));

        else
            p_permute(current_node, list, n - 1, best_cost_ptr,
                      best_cost_and_delay_ptr, pbest_list, mode, network,
                      delay_values, cost_table);
    } else {
        act_print_array(list);
        dag = p_actCreateStep(current_node, 0, list, 1);
        dag = actReduce(dag);
        /* changing the cost function */
        dag->my_type = ORDERED;
        WHICH_ACT = ORDERED;
        dag->node = current_node;
        put_node_names_in_act(dag, list);
        if (mode == AREA) {
            cost_node = make_tree_and_map(current_node, dag);
        } else {
            cost_node    = make_tree_and_map_delay(current_node, dag, network,
                                                   delay_values, cost_table, mode);
            arrival_time = cost_node->arrival_time;
        }
        cost      = cost_node->cost;

        /*		cost = dag->id + 1; */
        if (mode == AREA) {
            if (cost < *best_cost_ptr) {
                *best_cost_ptr = cost;
                array_free(best_list);
                best_list = array_dup(list);
                *pbest_list = best_list;
            }
        } else {
            this_cost_and_delay = ((double) (1.00 - mode)) * ((double) cost) +
                                  ((double) mode) * arrival_time;
            if (this_cost_and_delay < *best_cost_and_delay_ptr) {
                *best_cost_and_delay_ptr = this_cost_and_delay;
                array_free(best_list);
                best_list = array_dup(list);
                *pbest_list = best_list;
            }
        }
        p_dagDestroy(dag);
        /*		act_act_free(current_node); */
        FREE(cost_node);
    }

    while (c < n) {
        if ((n % 2) == 1) {
            t  = array_fetch(node_t *, list, n - 1);
            t1 = array_fetch(node_t *, list, 0);
            array_insert(node_t *, list, n - 1, t1);
            array_insert(node_t *, list, 0, t);
        } else {
            t  = array_fetch(node_t *, list, n - 1);
            t1 = array_fetch(node_t *, list, c - 1);
            array_insert(node_t *, list, n - 1, t1);
            array_insert(node_t *, list, c - 1, t);
        }
        c++;
        if (n > 2) {
            /* changed - July 5, 90 */
            /*----------------------*/
            /* if (*best_cost_ptr <= 1) ; */

            if ((mode == AREA) && (*best_cost_ptr <= 1));

            else {
                p_permute(current_node, list, n - 1, best_cost_ptr,
                          best_cost_and_delay_ptr, pbest_list, mode, network,
                          delay_values, cost_table);
            }
        } else {
            act_print_array(list);
            dag = p_actCreateStep(current_node, 0, list, 1);
            dag = actReduce(dag);
            dag->my_type = ORDERED;
            WHICH_ACT = ORDERED;
            dag->node = current_node;
            put_node_names_in_act(dag, list);
            if (mode == AREA) {
                cost_node = make_tree_and_map(current_node, dag);
            } else {
                cost_node    = make_tree_and_map_delay(current_node, dag, network,
                                                       delay_values, cost_table, mode);
                arrival_time = cost_node->arrival_time;
            }
            cost      = cost_node->cost;
            /*	cost = dag->id + 1; */
            if (mode == AREA) {
                if (cost < *best_cost_ptr) {
                    *best_cost_ptr = cost;
                    array_free(best_list);
                    best_list = array_dup(list);
                    *pbest_list = best_list;
                }
            } else {
                this_cost_and_delay = ((double) (1.00 - mode)) * ((double) cost) +
                                      ((double) mode) * arrival_time;
                if (this_cost_and_delay < *best_cost_and_delay_ptr) {
                    *best_cost_and_delay_ptr = this_cost_and_delay;
                    array_free(best_list);
                    best_list = array_dup(list);
                    *pbest_list = best_list;
                }
            }
            /* act_act_free(current_node); */
            p_dagDestroy(dag);
            FREE(cost_node);
        }
    }
}

void p_applyCreate(node_t *node, array_t *order_list) {
    ACT_VERTEX_PTR term_0, term_1, u, p_terminalDag(), p_rootCopy();
    ACT_VERTEX_PTR p_rootComplement(), p_treeNodeDag();
    enum st_retval p_freeTree();
    int            i, tree_size, index_size;
    array_t        *factor_tree;
    st_table       *tree_table;
    node_t         *act_tree_node, *t_node, *fanin_node, *root;
    tree_t         *tree_entry;
    char           dummy;
    ACT_PTR        act;
    ACT_ENTRY_PTR  act_entry;

    if (ACT_SET(node)->GLOBAL_ACT != NIL(ACT_ENTRY)) {
        return;
    }
    index_size = array_n(order_list);
    if (node_num_fanin(node) == 0) {
        if (node_function(node) == NODE_0) {
            u = p_terminalDag(0, index_size);
        } else if (node_function(node) == NODE_1) {
            u = p_terminalDag(1, index_size);
        } else {
            term_0 = ALLOC(ACT_VERTEX, 1);
            term_0->id         = 0;
            term_0->mark       = 0;
            term_0->index      = index_size;
            term_0->index_size = index_size;
            term_0->value      = 0;
            term_0->low        = NIL(act_t);
            term_0->high       = NIL(act_t);
            term_0->node       = NIL(node_t);
            term_0->name       = NIL(char);

            term_1 = ALLOC(ACT_VERTEX, 1);
            term_1->id         = 1;
            term_1->mark       = 0;
            term_1->index      = index_size;
            term_1->index_size = index_size;
            term_1->value      = 1;
            term_1->low        = NIL(act_t);
            term_1->high       = NIL(act_t);
            term_1->node       = NIL(node_t);
            term_1->name       = NIL(char);

            u = ALLOC(ACT_VERTEX, 1);
            u->id   = 2;
            u->mark = 0;
            for (i        = 0; i < index_size; i++) {
                t_node = array_fetch(node_t *, order_list, i);
                if (t_node == node) {
                    u->index = i;
                    u->node  = t_node;
                    break;
                }
            }
            u->index_size = index_size;
            u->value      = NO_VALUE;
            u->low        = term_0;
            u->high       = term_1;
            u->name       = NIL(char);
        }
    } else {
        factor_quick(node);
        factor_tree = factor_to_nodes(node);
        tree_size   = array_n(factor_tree);
        tree_table  = st_init_table(st_ptrcmp, st_ptrhash);
        if (tree_size > 1) {
            for (i = 1; i < tree_size; i++) {
                act_tree_node = array_fetch(node_t *, factor_tree, i);
                tree_entry    = ALLOC(tree_t, 1);
                switch (node_function(act_tree_node)) {
                    case NODE_BUF:tree_entry->buf = 1;
                        fanin_node = node_get_fanin(act_tree_node, 0);
                        if (ACT_SET(fanin_node)->GLOBAL_ACT == NIL(ACT_ENTRY)) {
                            p_applyCreate(fanin_node, order_list);
                        }
                        tree_entry->dag = ACT_SET(fanin_node)->GLOBAL_ACT->act->root;
                        break;

                    case NODE_INV:tree_entry->buf = 2;
                        fanin_node = node_get_fanin(act_tree_node, 0);
                        if (ACT_SET(fanin_node)->GLOBAL_ACT == NIL(ACT_ENTRY)) {
                            p_applyCreate(fanin_node, order_list);
                        }
                        tree_entry->dag = ACT_SET(fanin_node)->GLOBAL_ACT->act->root;
                        break;

                    default:tree_entry->dag = NIL(act_t);
                        tree_entry->buf     = 0;
                        break;
                }
                (void) st_insert(tree_table, (char *) act_tree_node, (char *) tree_entry);
            }
        }

        root = array_fetch(node_t *, factor_tree, 0);
        switch (node_function(root)) {
            case NODE_BUF:fanin_node = node_get_fanin(root, 0);
                if (ACT_SET(fanin_node)->GLOBAL_ACT == NIL(ACT_ENTRY)) {
                    p_applyCreate(fanin_node, order_list);
                }
                u = p_rootCopy(ACT_SET(fanin_node)->GLOBAL_ACT->act->root);
                break;
            case NODE_INV:fanin_node = node_get_fanin(root, 0);
                if (ACT_SET(fanin_node)->GLOBAL_ACT == NIL(ACT_ENTRY)) {
                    p_applyCreate(fanin_node, order_list);
                }
                u = p_rootComplement(ACT_SET(fanin_node)->GLOBAL_ACT->act->root);
                break;
            default:u = p_treeNodeDag(root, tree_table, order_list);
                break;
        }
        (void) st_foreach(tree_table, p_freeTree, &dummy);
        array_free(factor_tree);
        (void) st_free_table(tree_table);
        node_free(root);
    }

    act = ALLOC(ACT, 1);
    act->root      = u;
    act->node_list = order_list;
    act->node_name = util_strsav(node_name(node));

    act_entry = ALLOC(ACT_ENTRY, 1);
    act_entry->act         = act;
    act_entry->order_style = HEURISTIC;

    ACT_SET(node)->GLOBAL_ACT = act_entry;
    return;
}

/*ARGSUSED*/
enum st_retval p_freeTree(char *key, char *value, char *arg) {
    tree_t *tree_entry;
    node_t *node;

    tree_entry = (tree_t *) value;
    if (tree_entry->buf == 0) {
        p_dagDestroy(tree_entry->dag);
    }
    node = (node_t *) key;
    node_free(node);
    FREE(tree_entry);
    return (ST_CONTINUE);
}

ACT_VERTEX_PTR p_treeNodeDag(node_t *node, st_table *tree_table, array_t *order_list) {
    node_t         *fanin_node;
    tree_t         *tree_entry;
    ACT_VERTEX_PTR fanin_dag, temp_dag, current_dag, apply();
    input_phase_t  phase;
    char           *dummy;
    int            i, op, num_fanin, index_size, phase_comp, phase_num;

    index_size = array_n(order_list);
    switch (node_function(node)) {
        case NODE_AND:op = AND;
            current_dag = p_terminalDag(1, index_size);
            break;
        case NODE_OR:op = OR;
            current_dag = p_terminalDag(0, index_size);
            break;
    }

    num_fanin = node_num_fanin(node);
    for (i    = 0; i < num_fanin; i++) {
        phase_comp = 0;
        fanin_node = node_get_fanin(node, i);
        if (st_lookup(tree_table, (char *) fanin_node, &dummy)) {
            tree_entry = (tree_t *) dummy;
            if (tree_entry->dag == NIL(act_t)) {
                tree_entry->dag = p_treeNodeDag(fanin_node, tree_table, order_list);
            }
            fanin_dag      = tree_entry->dag;
            if (tree_entry->buf == 2)
                phase_comp = 1;
        } else {
            if (ACT_SET(fanin_node)->GLOBAL_ACT == NIL(ACT_ENTRY)) {
                p_applyCreate(fanin_node, order_list);
            }
            fanin_dag = ACT_SET(fanin_node)->GLOBAL_ACT->act->root;
        }

        phase       = node_input_phase(node, fanin_node);
        temp_dag    = current_dag;
        switch (phase) {
            case POS_UNATE:
                if (phase_comp) {
                    phase_num = P10;
                } else
                    phase_num = P11;
                break;

            case NEG_UNATE:
                if (phase_comp) {
                    phase_num = P11;
                } else
                    phase_num = P10;
                break;
        }
        current_dag = apply(current_dag, fanin_dag, op + phase_num);
        p_dagDestroy(temp_dag);
    }
    return (current_dag);
}

ACT_VERTEX_PTR p_terminalDag(int value, int index_size) {
    ACT_VERTEX_PTR u;

    u = ALLOC(ACT_VERTEX, 1);
    u->id         = 0;
    u->mark       = 0;
    u->index      = index_size;
    u->index_size = index_size;
    u->value      = value;
    u->low        = NIL(act_t);
    u->high       = NIL(act_t);
    u->node       = node_constant(value);
    u->name       = NIL(char);
    return (u);
}

void act_print_array(array_t *list) {
    int    i;
    node_t *n;

    if (ACT_DEBUG) {
        (void) printf("creating bdd for order ");
        for (i = 0; i < array_n(list); i++) {
            n = array_fetch(node_t *, list, i);
            (void) printf("%s", node_long_name(n));
        }
        (void) printf("\n");
    }
}
