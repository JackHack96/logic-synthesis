
#include "sis.h"
#include "pld_int.h"

/*	free up ACTs
*/

int
p_actRemove(network, argc, argv)
        network_t **network;
        int argc;
        char **argv;

{
    int     i, c, locality, input_size;
    array_t *node_vec;
    node_t  *current_node;

    locality = 0;        /* 	default GLOBAL	*/
    util_getopt_reset();
    while ((c  = util_getopt(argc, argv, "lg")) != EOF) {
        switch (c) {
            case 'l' : locality = 1;
                break;
            case 'g' : locality = 0;
                break;
            default: goto usage;
        }
    }
    node_vec   = com_get_nodes(*network, argc - util_optind + 1, argv + util_optind - 1);
    input_size = array_n(node_vec);
    for (i     = 0; i < input_size; i++) {
        current_node = array_fetch(node_t * , node_vec, i);
        p_actDestroy(&(ACT_SET(current_node)->act_num[locality]),
                     locality);
        ACT_SET(current_node)->act_num[locality] = NIL(ACT_ENTRY);
    }
    array_free(node_vec);
    return (0);    /* ok exit	*/

    usage:
    (void) fprintf(siserr, "usage: act_remove [-l] [-g] nodelist\n      -l          local\n      -g          global\n");
    return (1);      /* error exit   */
}

/* ARGSUSED */
void
p_actDestroy(act_set_entry_ptr, locality)
        ACT_ENTRY_PTR *act_set_entry_ptr;
        int locality;
{
    ACT_ENTRY_PTR act_set_entry;

    if (*act_set_entry_ptr == NULL) return;
    act_set_entry = *act_set_entry_ptr;
    if (act_set_entry != NIL(ACT_ENTRY)) {

        if (act_set_entry->act->root != NIL(ACT_VERTEX))
            p_dagDestroy(act_set_entry->act->root);
        if ((act_set_entry->act->node_list != NIL(array_t)) && locality)
            array_free(act_set_entry->act->node_list);
        FREE(act_set_entry->act->node_name);
        FREE(act_set_entry->act);
        FREE(act_set_entry);

    }
}

void
p_dagDestroy(dag)
        ACT_VERTEX_PTR dag;
{
    int            num_var, i, j, list_size;
    array_t        *current_list;
    ACT_VERTEX_PTR u;

    if (dag != NIL(ACT_VERTEX)) {
        index_list_array = array_alloc(array_t * , 0);
        num_var          = dag->index_size;
        for (i           = 0; i <= num_var; i++) {
            current_list = array_alloc(ACT_VERTEX_PTR, 0);
            array_insert(array_t * , index_list_array, i,
                         current_list);
        }

        traverse(dag, p_addLists);

        for (i = num_var; i >= 0; i--) {
            current_list = array_fetch(array_t * , index_list_array,
                                       i);
            list_size    = array_n(current_list);
            for (j       = 0; j < list_size; j++) {
                u = array_fetch(ACT_VERTEX_PTR, current_list, j);
                FREE(u);
            }
            array_free(current_list);
        }
        array_free(index_list_array);
    }
}
