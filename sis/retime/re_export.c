
#ifdef SIS
#include "sis.h"
#include "retime_int.h"

int
re_min_fanin_weight(node)
re_node *node;
{
    int m, i;
    re_edge *edge;

    m = POS_LARGE;
    re_foreach_fanin(node, i, edge){
    if (re_ignore_edge(edge)) continue;
    m = MIN(m, edge->weight);
    }
    return m;
}

int
re_min_fanout_weight(node)
re_node *node;
{
    int m, i;
    re_edge *edge;

    m = POS_LARGE;
    re_foreach_fanout(node, i, edge){
    if (re_ignore_edge(edge)) continue;
    m = MIN(m, edge->weight);
    }
    return m;
}

int
re_max_fanin_weight(node)
re_node *node;
{
    int m, i;
    re_edge *edge;

    m = 0;
    re_foreach_fanin(node, i, edge){
    if (re_ignore_edge(edge)) continue;
    m = MAX(m, edge->weight);
    }
    return m;
}

int
re_max_fanout_weight(node)
re_node *node;
{
    int m, i;
    re_edge *edge;

    m = 0;
    re_foreach_fanout(node, i, edge){
    if (re_ignore_edge(edge)) continue;
    m = MAX(m, edge->weight);
    }
    return m;
}

int
re_sum_of_edge_weight(graph)
re_graph *graph;
{
    int i;
    int c;
    re_edge *edge;

    c = 0;
    re_foreach_edge(graph, i, edge){
    if (re_ignore_edge(edge)) continue;
    c += edge->weight;
    }

    /* return */
    return c;

}

int
re_effective_sum_edge_weight(graph)
re_graph *graph;
{
    int i, m, c;
    re_node *node;

    c = 0;
    re_foreach_node(graph, i, node){
    if (node->type == RE_IGNORE) continue;
    m = re_max_fanout_weight(node);
    c += m;
    }

    /* return */
    return c;
}

double
re_sum_node_area(graph)
re_graph *graph;
{
    int i;
    double c;
    re_node *node;

    c = 0.0;
    for (i = re_num_nodes(graph); i-- > 0; ) {
    node = array_fetch(re_node *, graph->nodes, i);
    c += node->final_area;
    }

    /* return */
    return c;
}

double
re_total_area(graph, area_r)
re_graph *graph;
double area_r;
{
    double n;
    n = re_sum_node_area(graph) + 	/* Area of nodes */
    area_r * re_effective_sum_edge_weight(graph); /* Register area */

    return n;
}

bool
re_node_retimable(node)
re_node *node;
{
    if (node->type != RE_INTERNAL) return FALSE;

    if (re_min_fanin_weight(node) == 0 && re_min_fanout_weight(node) == 0)
    return FALSE;
    
    return TRUE;
}

bool
re_node_forward_retimable(node)
re_node *node;
{
    if (node->type != RE_INTERNAL) return FALSE;

    if (re_min_fanin_weight(node) == 0) return FALSE;
    
    return TRUE;
}

bool
re_node_backward_retimable(node)
re_node *node;
{
    if (node->type != RE_INTERNAL) return FALSE;

    if (re_min_fanout_weight(node) == 0) return FALSE;
    
    return TRUE;
}
#endif /* SIS */
