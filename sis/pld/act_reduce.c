
#include "sis.h"
#include "../include/pld_int.h"

/*	reduce, strictly follows bryants paper	
*/

extern int p_compare2key();

ACT_VERTEX_PTR
actReduce(fn_graph)
        ACT_VERTEX_PTR fn_graph;

{
    int            nextid, i, j, list_size, q_size, num_var;
    ACT_VERTEX_PTR u, result;
    KEY            oldkey, key;
    array_t        *Q, *current_list, *subgraph;
    Q_NODE_PTR     q_entry;


    subgraph         = array_alloc(ACT_VERTEX_PTR, 0);
    num_var          = fn_graph->index_size;
    /* create the index_lists */
    index_list_array = array_alloc(array_t * , 0);
    for (i = 0; i <= num_var; i++) {
        current_list = array_alloc(ACT_VERTEX_PTR, 0);
        array_insert(array_t * , index_list_array, i, current_list);
    }

    /* add all the vertices in fn_graph to the lists corresponding
           to their indices.
    */
    traverse(fn_graph, p_addLists);

    nextid = -1;
    for (i = num_var; i >= 0; i--) {
        Q            = array_alloc(Q_NODE_PTR, 0);
        current_list = array_fetch(array_t * , index_list_array, i);
        list_size    = array_n(current_list);
        for (j       = 0; j < list_size; j++) {
            u = array_fetch(ACT_VERTEX_PTR, current_list, j);
            if (i == num_var) {    /* add to the Q 	*/

                q_entry = ALLOC(Q_NODE, 1);
                q_entry->key.low  = u->value;
                q_entry->key.high = u->value;
                q_entry->v        = u;
                array_insert_last(Q_NODE_PTR, Q, q_entry);
            } else {
                if (u->low->id == u->high->id) {
                    u->id   = u->low->id;
                    u->mark = DELETE;
                } else {
                    q_entry = ALLOC(Q_NODE, 1);
                    q_entry->key.low  = u->low->id;
                    q_entry->key.high = u->high->id;
                    q_entry->v        = u;
                    array_insert_last(Q_NODE_PTR, Q, q_entry);
                }
            }
        }

        array_sort(Q, p_compare2key);

        oldkey.low  = -1;
        oldkey.high = -1;

        q_size = array_n(Q);
        for (j = 0; j < q_size; j++) {
            q_entry = array_fetch(Q_NODE_PTR, Q, j);
            key     = q_entry->key;
            u       = q_entry->v;
            if ((key.low == oldkey.low) && (key.high == oldkey.high)) {
                u->id   = nextid;
                u->mark = DELETE;
            } else {
                nextid++;
                u->id       = nextid;
                array_insert(ACT_VERTEX_PTR, subgraph, nextid, u);
                if (i != num_var) {
                    u->low  = array_fetch(ACT_VERTEX_PTR,
                                          subgraph, u->low->id);
                    u->high = array_fetch(ACT_VERTEX_PTR,
                                          subgraph, u->high->id);
                }
                oldkey.low  = key.low;
                oldkey.high = key.high;
            }
        }

        /* Free storage associated with Q */
        for (j = 0; j < q_size; j++) {
            q_entry = array_fetch(Q_NODE_PTR, Q, j);
            FREE(q_entry);
        }
        array_free(Q);
    }

    result = array_fetch(ACT_VERTEX_PTR, subgraph, fn_graph->id);
    array_free(subgraph);

    /*	free the storage associated with the unwanted nodes
         in the fn_graph and the index lists
    */

    for (i = num_var; i >= 0; i--) {
        current_list = array_fetch(array_t * , index_list_array, i);
        list_size    = array_n(current_list);
        for (j       = 0; j < list_size; j++) {
            u = array_fetch(ACT_VERTEX_PTR, current_list, j);
            if (u->mark == DELETE) {
                FREE(u);
            }
        }
        array_free(current_list);
    }

    array_free(index_list_array);
    return (result);
}

void
p_addLists(v)
        ACT_VERTEX_PTR v;
{
    int     index;
    array_t *current_list;

    index        = v->index;
    current_list = array_fetch(array_t * , index_list_array, index);
    array_insert_last(ACT_VERTEX_PTR, current_list, v);
}

int
p_compare2key(q1, q2)
        Q_NODE_PTR *q1, *q2;

{
    KEY key1, key2;

    key1 = (*q1)->key;
    key2 = (*q2)->key;

    if (key1.low < key2.low)
        return (-1);
    else if (key1.low > key2.low)
        return (1);
    else if (key1.high < key2.high)
        return (-1);
    else if (key1.high > key2.high)
        return (1);
    else return (0);
}

