
#include "sis.h"
#include "../include/pld_int.h"

unionop(node1, node2
)
struct tree_node *node1;
struct tree_node *node2;
{

if (node1->parent != node1) {
(void) printf(" Node %d has a parent\n", node1->index);
exit(8);
}
if (node2->parent != node2) {
(void) printf( " Node %d has a parent\n", node2->index);
exit(8);
}

if (node1->num_child < node2->num_child)
make_son(node1, node2
);
else
make_son (node2, node1
);

}

/* make node1 the son of node2 */

make_son(node1, node2
)
struct tree_node *node1, *node2;
{
node1->
parent = node2;
node2->
num_child = node2->num_child + node1->num_child;
}

/* try changing the parent fields of all the nodes on the 
   path  done*/

struct tree_node *
find_tree(node)
        struct tree_node *node;
{
    struct tree_node *dummy_node;
    struct tree_node *node_on_path;

    node_on_path   = node;
    dummy_node     = node;
    while (dummy_node->parent != dummy_node)
        dummy_node = dummy_node->parent;
    while (node_on_path != dummy_node) {
        node_on_path = node_on_path->parent = dummy_node;
    }
    return dummy_node;

}

int
estimate_clb_no(network, size)
        network_t *network;
        int size;
{
    int       upper_bound;
    network_t *dup_network;

    dup_network = network_dup(network);
    (void) decomp_tech_network(dup_network, 2, 2);
    imp_part_network(dup_network, size, 0, 0); /* for not moving fanins */
    upper_bound = network_num_internal(dup_network);
    network_free(dup_network);
    (void) fprintf(sisout, "The upper bound on CLBs is %d\n", upper_bound);
    return upper_bound;
}

and_or_map(network, size
)
network_t *network;
int       size;
{
(void)
decomp_tech_network(network,
2, 2);
imp_part_network(network, size,
0, 0); /* for not moving fanins */
return 0;
}


int
estimate_net_no(network)
        network_t *network;
{
    node_t  *node;
    int     value = 0, i;
    array_t *order;

    order  = network_dfs(network);
    for (i = 0; i < array_n(order); i++) {
        node = array_fetch(node_t * , order, i);
        if ((node_function(node) != NODE_PI) &&
            (node_function(node) != NODE_PO)) {
            value += node_num_fanin(node);
        }
    }
    array_free(order);
    return value;
}

  

     
