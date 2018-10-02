
/***************************************************
 *
 *    Function:     cutset()
 *
 *    Author:        Kanwar Jit Singh
 *
 *****************************************************/

#include "maxflow_int.h"
#include "sis.h"

#define VLARGE (int)1000000
int maxflow_debug;

/* ARGSUSED */
enum st_retval speed_add_node_to_graph(char *node,

**cap,
char *store
)
{
mfgptr graph;
char   name[MF_MAXSTR], dup_name[MF_MAXSTR];

graph = (mfgptr) store;

(void)
sprintf(name,
"%s",
node_long_name((node_t
*)node));
(void)
sprintf(dup_name,
"%s_dup",
node_long_name((node_t
*)node));

mf_read_node(graph, name,
0);
mf_read_node(graph, dup_name,
0);

return
ST_CONTINUE;
}

array_t *cutset(network_t *network, st_table *node_table) {
    array_t *cut;
    cut = cutset_interface(network, node_table, VLARGE);
    return cut;
}

array_t *cutset_interface(network_t *network, st_table *node_table, int edge_weight) {
    st_table     *name_node_table;
    array_t      *array;
    mfgptr       graph;
    st_generator *gene;
    char         *key, *dummy;

    /* Create the flow network */
    graph = mf_create_flow_network(network, node_table, edge_weight,
                                   &name_node_table);

    /* Run the maxflow algorithm on the flow networek */
    maxflow(graph, 0);

    /* Get cutset and translate it into nodes of the network */
    array = mf_build_node_cutset(graph, name_node_table);

    /* FREE the memory in the name_node_table and the flow network */
    st_foreach_item(name_node_table, gene, &key, &dummy) { FREE(key); }
    st_free_table(name_node_table);
    mf_free_graph(graph);

    return array;
}

mf_graph_t *
mf_create_flow_network(network_t *network, st_table *node_table, int edge_weight, st_table **p_name_node_table) {
    array_t *df_array;
    mfgptr  graph;
    node_t * fi, *fo, *node;
    lsGen gen;
    char  *dummy, temp_name[MF_MAXSTR], name[MF_MAXSTR];
    int   j, i, capacity, terminal_flag;

    graph = mf_alloc_graph();
    *p_name_node_table = st_init_table(strcmp, st_strhash);

    /*
     * For each node in the network create
     * a node and also a duplicate
     */
    mf_read_node(graph, "maxflow_source", 1);
    mf_read_node(graph, "maxflow_sink", 2);
    st_foreach(node_table, speed_add_node_to_graph, (char *) graph);

    /*
     * For each node create an edge to each fanin
     * which is in node_table, assign capacity
     * to the duplicate.
     */
    df_array = network_dfs_from_input(network);
    for (i   = 0; i < array_n(df_array); i++) {
        node = array_fetch(node_t *, df_array, i);
        if (node->type == INTERNAL) {
            if (st_lookup(node_table, (char *) node, &dummy)) {
                capacity = (int) dummy;
                /* Add the edge between the node and its dup */
                (void) sprintf(name, "%s", node_long_name(node));
                (void) sprintf(temp_name, "%s_dup", node_long_name(node));
                mf_read_edge(graph, name, temp_name, capacity);

                /* Save the pointer referenced by name */
                (void) st_insert(*p_name_node_table, util_strsav(name), (char *) node);

                /* Add an edge to the source if no fanin is
                in the table or if the fanin is a primary i/p */
                terminal_flag = 1;
                foreach_fanin(node, j, fi) {
                    if (st_lookup(node_table, (char *) fi, &dummy)) {
                        (void) sprintf(temp_name, "%s_dup", node_long_name(fi));
                        mf_read_edge(graph, temp_name, name, edge_weight);
                        terminal_flag = 0;
                    }
                }
                if (terminal_flag) {
                    mf_read_edge(graph, "maxflow_source", name, edge_weight);
                }

                /* Add  an edge to the sink if no fanout is in table */
                terminal_flag = 1;
                foreach_fanout(node, gen, fo) {
                    if (st_lookup(node_table, (char *) fo, &dummy)) {
                        terminal_flag = 0;
                    }
                }
                if (terminal_flag) {
                    (void) sprintf(temp_name, "%s_dup", node_long_name(node));
                    mf_read_edge(graph, temp_name, "maxflow_sink", edge_weight);
                }
            }
        }
    }
    array_free(df_array);

    return graph;
}

array_t *mf_build_node_cutset(mf_graph_t *graph, st_table *name_node_table) {
    int  i;
    char *dummy;
    node_t * node;
    array_t *array;
    mfcptr  mf_cutset;

    /* Get the cutset from the flow network */
    if (!(mf_cutset = MF_ALLOC(1, mf_cutset_t)))
        mf_error("Memory allocation failure", "cutset");
    mf_cutset->graph = graph;
    get_cutset(mf_cutset);

    if (maxflow_debug) {
        (void) fprintf(sisout, "Cutset edges:");
        for (i = 0; i < mf_cutset->narcs; i++) {
            (void) fprintf(sisout, "  %s(%d)", (mf_cutset->from_node)[i],
                           (mf_cutset->capacity)[i]);
        }
        (void) fprintf(sisout, "\n");
    }

    array  = array_alloc(node_t *, 0);
    for (i = 0; i < mf_cutset->narcs; i++) {
        if (st_lookup(name_node_table, (mf_cutset->from_node)[i], &dummy)) {
            node = (node_t *) dummy;
            array_insert_last(node_t *, array, node);
        } else {
            (void) fprintf(sisout, "Unknown node %s encountered \n",
                           (mf_cutset->from_node)[i]);
        }
    }

    mf_free_cutset(mf_cutset);

    return array;
}
