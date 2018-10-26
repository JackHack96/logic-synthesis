
/*--------------------------------------------------------------------------
|   Auxiliary routines for the power estimate package:
|
|       power_free_info()
|       power_print_info()
|
|       power_network_dfs()
|       power_network_print()
|       power_lines_in_set()
|
| Copyright (c) Mar 94 - Jose' Monteiro      jcm@rle-vlsi.mit.edu
| Massachusetts Institute of Technology
+-------------------------------------------------------------------------*/

#include "power_int.h"
#include "sis.h"

int power_free_info() {
    node_t       *node;
    power_info_t *info;
    st_generator *gen;

    if (power_info_table == NIL(st_table))
        return 0;

    st_foreach_item(power_info_table, gen, (char **) &node, (char **) &info) {
        FREE(info);
    }
    st_free_table(power_info_table);
    power_info_table = NIL(st_table);

    return 0;
}

int power_print_info(network_t *network) {
    node_t       *node;
    power_info_t *info;
    double       PIs_power, total_power;
    lsGen        gen;

    if (power_info_table == NIL(st_table)) {
        fprintf(siserr, "Power for this network not estimated yet!\n");
        return 1;
    }

    total_power = PIs_power = 0.0;
    foreach_node(network, gen, node) {
        if (node_function(node) == NODE_PO)
            continue;

        if (!st_lookup(power_info_table, (char *) node, (char **) &info)) {
            power_free_info();
            fprintf(siserr, "Power for this network not estimated yet!\n");
            return 1;
        }

        fprintf(sisout, "Node %-5s\tCap. = %d\tSwitch Prob. = %.2f\tPower = %.1f\n",
                node->name, info->cap_factor, info->switching_prob,
                250.0 * info->cap_factor * info->switching_prob * CAPACITANCE);
        total_power +=
                250.0 * info->cap_factor * info->switching_prob * CAPACITANCE;
        if (node_function(node) == NODE_PI)
            PIs_power +=
                    250.0 * info->cap_factor * info->switching_prob * CAPACITANCE;
    }
    fprintf(sisout, "Total Power:\t%f\nPIs Power:\t%f\n", total_power, PIs_power);

    return 0;
}

array_t *power_network_dfs(network_t *net) {
    st_table *input, *output;
    array_t  *result, *temp;
    node_t   *node, *pi, *po;
    int      i;
    char     *value;
    lsGen    gen;

    input  = st_init_table(st_ptrcmp, st_ptrhash);
    output = st_init_table(st_ptrcmp, st_ptrhash);
    result = array_alloc(node_t *, 0);

    /* Put the PI nodes in the array first */
    foreach_primary_input(net, gen, pi) {
        array_insert_last(node_t *, result, pi);
        st_insert(input, (char *) pi, (char *) 0);
    }
    foreach_primary_output(net, gen, po) {
        st_insert(output, (char *) po, (char *) 0);
    }

    temp = network_dfs(net);

    /* Now put the intermediate nodes */
    for (i = 0; i < array_n(temp); i++) {
        node = array_fetch(node_t *, temp, i);
        if (!st_lookup(input, (char *) node, &value) &&
            !st_lookup(output, (char *) node, &value))
            array_insert_last(node_t *, result, node);
    }

    /* Now put the PO nodes */
    foreach_primary_output(net, gen, po) {
        array_insert_last(node_t *, result, po);
    }

    array_free(temp);
    st_free_table(input);
    st_free_table(output);

    return result;
}

void power_network_print(network_t *net) {
    array_t *node_vec;
    node_t  *node;
    int     i;

    node_vec = power_network_dfs(net);
    for (i   = 0; i < array_n(node_vec); i++) {
        node = array_fetch(node_t *, node_vec, i);
        node_print(sisout, node);
    }
    array_free(node_vec);
}

pset power_lines_in_set(pset set, int setSize) {
    pset inclSets = set_full(setSize);
    int  i, j, mask;

    for (i = 0; i < setSize; i++)
        for (j = 0, mask = 1; j < power_setSize; j++, mask <<= 1)
            if (mask & i) {
                if (!is_in_set(set, 2 * j + 1)) {
                    set_remove(inclSets, i);
                    break;
                }
            } else if (!is_in_set(set, 2 * j)) {
                set_remove(inclSets, i);
                break;
            }

    return inclSets;
}
