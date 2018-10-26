
#include "sis.h"

static char *gen_unique_name();

static node_t *copy_node();

static int add_node_by_name();

int network_append(network_t *network1, network_t *network2_copy) {
    network_t *network2;
    node_t    *fanin, *node, *node1, *node2;
    lsGen     gen;
    int       i, error;

    error = 0;

    /*
     *  we copy network2 because when we 'copy_node' we corrupt the
     *  fanout pointer list of network2
     */
    network2 = network_dup(network2_copy);

#ifdef SIS
    /* We don't append STG information, so we invalidate the STG */
    stg_free(network1->stg);
#endif

    /* each node of network1 will be replaced with itself (unless changed) */
    foreach_node(network1, gen, node1) { node1->copy = node1; }

    /* copy nodes from network2 */
    foreach_node(network2, gen, node2) {
        if (!add_node_by_name(network1, node2)) {
            error = 1;
        }
    }

    /* reset_io */
    foreach_node(network1, gen, node) {
        foreach_fanin(node, i, fanin) { node->fanin[i] = fanin->copy; }
    }

    /* patch any fanin which happens to point to a PO node */
    foreach_node(network1, gen, node1) {
        foreach_fanin(node1, i, fanin) {
            if (fanin->type == PRIMARY_OUTPUT) {
                node1->fanin[i] = fanin->fanin[0];
            }
        }
    }

    foreach_node(network1, gen, node) {
        if (node->type == PRIMARY_INPUT && node->copy != node) {
            network_delete_node_gen(network1, gen);
        }
    }

    /* reset the fanout pointers */
    foreach_node(network1, gen, node1) {
        LS_ASSERT(lsDestroy(node1->fanout, free));
        node1->fanout = lsCreate();
    }
    foreach_node(network1, gen, node1) { fanin_add_fanout(node1); }

#ifdef SIS
    /* Update the latch list and the latch table */
    copy_latch_info(network2->latch, network1->latch, network1->latch_table);
#endif

    /* make sure the resulting network is acyclic */
    if (!network_is_acyclic(network1)) {
        error = 1;
    }

    network_free(network2);
    return !error;
}

static node_t *copy_node(network_t *network1, node_t *node2, int name_hack) {
    node_t *new_node;
    char   *new_name;

    new_node = node_dup(node2); /* even duplicates fanin */

    if (name_hack) {
        new_name = gen_unique_name(network1, new_node);
        FREE(new_node->name);
        new_node->name = new_name;
    }
    network_add_node(network1, new_node);
    node2->copy    = new_node;
    new_node->copy = new_node;
    return new_node;
}

static char *gen_unique_name(network_t *network, node_t *node) {
    char new_name[1024];
    int  count;

    count = 0;
    do {
        (void) sprintf(new_name, "%s-%d", node->name, count++);
    } while (network_find_node(network, new_name) != 0);

    return util_strsav(new_name);
}

static int add_node_by_name(network_t *network1, node_t *node2) {
    node_t *temp, *node1;
    char   errmsg[1024];

    node1 = network_find_node(network1, node2->name);
    if (node1 == 0) {
        (void) copy_node(network1, node2, 0);

    } else if (node2->type == PRIMARY_INPUT) {
        node2->copy = node1;

    } else {
        temp = copy_node(network1, node2, 1);
        if (node1->type == PRIMARY_INPUT) {
            node1->copy = temp;
            network_swap_names(network1, node1, temp);
        } else {
            (void) sprintf(errmsg, "network_append: node '%s' already driven\n",
                           node2->name);
            error_append(errmsg);
            return 0;
        }
    }

    return 1;
}
