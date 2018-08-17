
#include "sis.h"
#include "../include/pld_int.h"

/*	for order_nodes and p_alap_order_nodes	*/
typedef struct fanin_record_defn {
    int    max_level;
    node_t *node;
} fanin_record_t;

extern void p_rootAdd();

extern void p_levelAdd();

extern int p_maxAdd();

extern void p_recOrder();

extern enum st_retval p_cleanFRTable();

extern enum st_retval p_cleanRTable();

extern void p_recFree();

extern int p_fanin_record_sort();

extern enum st_retval p_addPI();

array_t *
p_alap_order_nodes(node_vec)
        array_t *node_vec;
{
    node_t         *dummy_node, *t_node, *tt_node, *root, *node;
    int            i;
    char           *dummy;
    st_table       *root_table, *level_table;
    array_t        *order_list, *pi_list;
    fanin_record_t *pi_record;

    dummy_node = node_constant(0);
    for (i     = 0; i < array_n(node_vec); i++) {
        node = array_fetch(node_t * , node_vec, i);
        if (node_function(node) != NODE_PI) {
            if (node_function(node) == NODE_PO) {
                node = node_get_fanin(node, 0);
            }
            t_node     = dummy_node;
            tt_node    = node_literal(node, 1);
            dummy_node = node_or(t_node, tt_node);
            node_free(t_node);
            node_free(tt_node);
        }
    }

    root_table = st_init_table(st_ptrcmp, st_ptrhash);
    p_rootAdd(dummy_node, root_table);

    level_table = st_init_table(st_ptrcmp, st_ptrhash);
    (void) st_lookup(root_table, (char *) dummy_node, &dummy);
    root = (node_t *) dummy;
    (void) st_insert(level_table, (char *) root, (char *) 0);
    p_levelAdd(root, level_table, root_table);

    pi_list = array_alloc(fanin_record_t * , 0);
    st_foreach(level_table, p_addPI, (char *) pi_list);

    array_sort(pi_list, p_fanin_record_sort);
    order_list = array_alloc(node_t * , 0);
    for (i = 0; i < array_n(pi_list); i++) {
        pi_record = array_fetch(fanin_record_t * , pi_list, i);
        array_insert_last(node_t * , order_list, pi_record->node);
        FREE(pi_record);
    }
    array_free(pi_list);
    st_free_table(level_table);
    st_foreach(root_table, p_cleanRTable, (char *) root_table);
    st_free_table(root_table);
    node_free(dummy_node);
    return (order_list);
}

enum st_retval
p_addPI(key, value, arg)
        char *key, *value, *arg;
{
    node_t         *node;
    int            level;
    array_t        *pi_list;
    fanin_record_t *pi_record;

    node = (node_t *) key;

    if ((node_num_fanin(node) == 0) && (node_function(node) != NODE_0) &&
        (node_function(node) != NODE_1)) {
        level     = (int) value;
        pi_record = ALLOC(fanin_record_t, 1);
        pi_record->node      = node;
        pi_record->max_level = level;
        pi_list = (array_t *) arg;
        array_insert_last(fanin_record_t * , pi_list, pi_record);
    }
    return ST_CONTINUE;
}

array_t *
pld_shuffle(list)
        array_t *list;

{
    array_t *newlist, *mark;
    int     i, list_size, next_index, count, j, index, rand_num;
    node_t  *new_node;

    mark = array_alloc(
    int, 0);
    newlist = array_alloc(node_t * , 0);

    list_size = array_n(list);
    for (i    = 0; i < list_size; i++) {
        array_insert(
        int, mark, i, 0);
    }

    for (i = 0; i < list_size; i++) {
        /*NOSTRICT*/
        rand_num   = rand() % 32768;
        /* taking random numbers from 0 - 2**15
        this should be portable across most machines	*/
        next_index = (rand_num / 32768.0) * (list_size - i);
        /* normalizing it and multiplying by the range	*/
        count      = -1;
        for (j     = 0; j < list_size; j++) {
            if (array_fetch(int, mark, j) == 0) count++;
            if (count == next_index) {
                array_insert(
                int, mark, j, 1);
                index = j;
                break;
            }
        }
        new_node   = array_fetch(node_t * , list, index);
        array_insert(node_t * , newlist, i, new_node);
    }
    array_free(mark);
    array_free(list);
    return (newlist);
}


array_t *
pld_order_nodes(node_vec, pi_only)
        array_t *node_vec;
        int pi_only;
{
    node_t   *dummy_node, *t_node, *tt_node, *root, *node;
    int      i;
    char     *dummy;
    st_table *root_table, *level_table, *max_table, *fanin_table;
    array_t  *order_list;

    dummy_node = node_constant(0);
    for (i     = 0; i < array_n(node_vec); i++) {
        node       = array_fetch(node_t * , node_vec, i);
        t_node     = dummy_node;
        tt_node    = node_literal(node, 1);
        dummy_node = node_or(t_node, tt_node);
        node_free(t_node);
        node_free(tt_node);
    }

    root_table = st_init_table(st_ptrcmp, st_ptrhash);
    p_rootAdd(dummy_node, root_table);

    level_table = st_init_table(st_ptrcmp, st_ptrhash);
    (void) st_lookup(root_table, (char *) dummy_node, &dummy);
    root = (node_t *) dummy;
    (void) st_insert(level_table, (char *) root, (char *) 0);
    p_levelAdd(root, level_table, root_table);

    max_table = st_init_table(st_ptrcmp, st_ptrhash);
    (void) p_maxAdd(root, max_table, level_table, root_table);

    order_list  = array_alloc(node_t * , 0);
    fanin_table = st_init_table(st_ptrcmp, st_ptrhash);
    p_recOrder(root, max_table, fanin_table, root_table, order_list,
               pi_only);
    node_free(dummy_node);
    st_free_table(max_table);
    st_free_table(level_table);
    st_foreach(root_table, p_cleanRTable, (char *) root_table);
    st_free_table(root_table);
    st_foreach(fanin_table, p_cleanFRTable, dummy);
    st_free_table(fanin_table);
    return (order_list);
}

void
p_rootAdd(node, root_table)
        node_t *node;
        st_table *root_table;
{
    char    *dummy;
    array_t *factor_array;
    node_t  *root, *fanin;
    int     i;

    if (node_num_fanin(node) == 0) {
        return;
    }
    if (!st_lookup(root_table, (char *) node, &dummy)) {
        factor_array = factor_to_nodes(node);
        root         = array_fetch(node_t * , factor_array, 0);
        array_free(factor_array);
        (void) st_insert(root_table, (char *) node, (char *) root);
        foreach_fanin(node, i, fanin)
        {
            p_rootAdd(fanin, root_table);
        }
    }
    return;
}

void
p_levelAdd(node, level_table, root_table)
        node_t *node;
        st_table *level_table, *root_table;
{
    char   *dummy;
    int    i, current_level, old_level;
    node_t *fanin;

    (void) st_lookup(level_table, (char *) node, &dummy);
    current_level = (int) dummy;
    foreach_fanin(node, i, fanin)
    {
        if (st_lookup(root_table, (char *) fanin, &dummy)) {
            fanin = (node_t *) dummy;
        }
        if (st_lookup(level_table, (char *) fanin, &dummy)) {
            old_level = (int) dummy;
            if (old_level <= current_level) {
                (void) st_insert(level_table, (char *) fanin,
                                 (char *) (current_level + 1));
            }
        } else {
            (void) st_insert(level_table, (char *) fanin,
                             (char *) (current_level + 1));
        }
    }

    foreach_fanin(node, i, fanin)
    {
        if (st_lookup(root_table, (char *) fanin, &dummy)) {
            fanin = (node_t *) dummy;
        }
        p_levelAdd(fanin, level_table, root_table);
    }
}

int
p_maxAdd(node, max_table, level_table, root_table)
        node_t *node;
        st_table *max_table, *level_table, *root_table;
{
    char   *dummy;
    int    i, max, fanin_max;
    node_t *fanin;

    (void) st_lookup(level_table, (char *) node, &dummy);
    max = (int) dummy;
    foreach_fanin(node, i, fanin)
    {
        if (st_lookup(root_table, (char *) fanin, &dummy)) {
            fanin = (node_t *) dummy;
        }
        if (st_lookup(max_table, (char *) fanin, &dummy)) {
            fanin_max = (int) dummy;
        } else {
            fanin_max = p_maxAdd(fanin, max_table, level_table, root_table);
        }
        max = MAX(max, fanin_max);
    }
    (void) st_insert(max_table, (char *) node, (char *) max);
    return max;
}

void
p_recOrder(node, max_table, fanin_table, root_table, order_list, pi_only)
        node_t *node;
        st_table *max_table, *fanin_table, *root_table;
        array_t *order_list;
        int pi_only;
{
    array_t        *fanin_record_list;
    int            i;
    char           *dummy;
    node_t         *fanin;
    fanin_record_t *fanin_record;
    st_table       *iv_table;

    if (node_num_fanin(node) == 0) {
        if ((node_function(node) != NODE_0) &&
            (node_function(node) != NODE_1)) {
            array_insert_last(node_t * , order_list, node);
            return;
        }
    }

    fanin_record_list = array_alloc(fanin_record_t * , 0);
    iv_table          = st_init_table(st_ptrcmp, st_ptrhash);
    foreach_fanin(node, i, fanin)
    {
        if (!pi_only) {
            if (st_lookup(root_table, (char *) fanin, &dummy)) {
                (void) st_insert(iv_table, dummy, (char *) fanin);
                fanin = (node_t *) dummy;
            }
        }
        if (st_lookup(fanin_table, (char *) fanin, &dummy)) {
            fanin_record = (fanin_record_t *) dummy;
        } else {
            fanin_record = ALLOC(fanin_record_t, 1);
            (void) st_lookup(max_table, (char *) fanin, &dummy);
            fanin_record->max_level = (int) dummy;
            fanin_record->node      = fanin;
        }
        array_insert_last(fanin_record_t * , fanin_record_list, fanin_record);
    }

    array_sort(fanin_record_list, p_fanin_record_sort);

    for (i = 0; i < array_n(fanin_record_list); i++) {
        fanin_record = array_fetch(fanin_record_t * , fanin_record_list, i);
        if (!st_lookup(fanin_table, (char *) (fanin_record->node), &dummy)) {
            p_recOrder(fanin_record->node, max_table, fanin_table, root_table,
                       order_list, pi_only);
            if (!pi_only) {
                if (st_lookup(iv_table, (char *) fanin_record->node,
                              &dummy)) {
                    array_insert_last(node_t * , order_list,
                                      (node_t *) dummy);
                }
            }
            (void) st_insert(fanin_table, (char *) (fanin_record->node),
                             (char *) fanin_record);
        } else {
            if (fanin_record != (fanin_record_t *) dummy) {
                FREE(fanin_record);
            }
        }
    }
    array_free(fanin_record_list);
    st_free_table(iv_table);
    return;
}

/*ARGSUSED*/
enum st_retval
p_cleanRTable(key, value, arg)
        char *key, *value, *arg;
{
    st_table *root_table;
    node_t   *node;

    root_table = (st_table *) arg;
    node       = (node_t *) value;
    p_recFree(node, root_table);
    return ST_CONTINUE;
}

void
p_recFree(node, root_table)
        node_t *node;
        st_table *root_table;
{
    char    *dummy;
    node_t  *fanin;
    int     i;
    array_t *fanin_list;

    if (st_lookup(root_table, (char *) node, &dummy)) {
        return;
    }
    if (node_num_fanin(node) == 0) {
        return;
    }
    fanin_list = array_alloc(node_t * , 0);
    foreach_fanin(node, i, fanin)
    {
        array_insert_last(node_t * , fanin_list, fanin);
    }
    node_free(node);
    for (i = 0; i < array_n(fanin_list); i++) {
        node = array_fetch(node_t * , fanin_list, i);
        p_recFree(node, root_table);
    }
    array_free(fanin_list);
    return;
}
int
p_fanin_record_sort(key1, key2)
        char **key1, **key2;
{
    fanin_record_t *fanin_record1, *fanin_record2;
    int            level1, level2;

    fanin_record1 = *((fanin_record_t **) key1);
    fanin_record2 = *((fanin_record_t **) key2);
    level1        = fanin_record1->max_level;
    level2        = fanin_record2->max_level;
    if (level1 < level2) return 1;
    else if (level1 > level2) return -1;
    else return 0;
}

/*ARGSUSED*/
enum st_retval
p_cleanFRTable(key, value, arg)
        char *key, *value, *arg;
{
    fanin_record_t *fanin_record;

    fanin_record = (fanin_record_t *) value;
    FREE(fanin_record);
    return (ST_CONTINUE);
}
