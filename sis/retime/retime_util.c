
#ifdef SIS
#include "sis.h"
#include "retime_int.h"

re_node *re_node_dup();
re_edge *re_edge_dup();

re_graph *
re_graph_alloc()
{
    re_graph *new_graph;

    new_graph = ALLOC(re_graph, 1);
    new_graph->nodes = array_alloc(re_node *, 0);
    new_graph->edges = array_alloc(re_edge *, 0);
    new_graph->primary_inputs = array_alloc(re_node *, 0);
    new_graph->primary_outputs = array_alloc(re_node *, 0);
    new_graph->control_name = NIL(char);
    new_graph->s_type = UNKNOWN;

    return new_graph;
}

void
re_graph_free(graph)
re_graph *graph;
{
    int i;
    re_node *node;
    re_edge *edge;

    for (i = re_num_edges(graph); i-- > 0; ) {
	edge = array_fetch(re_edge *, graph->edges, i);
	FREE(edge->latches);
	FREE(edge->initial_values);
	FREE(edge);
    }

    for (i = re_num_nodes(graph); i-- > 0; ) {
	node = array_fetch(re_node *, graph->nodes, i);
	(void) array_free(node->fanins);
	(void) array_free(node->fanouts);
	FREE(node);
    }

    (void) array_free(graph->nodes);
    (void) array_free(graph->edges);
    (void) array_free(graph->primary_inputs);
    (void) array_free(graph->primary_outputs);
    if (graph->control_name != NIL(char)) FREE(graph->control_name);

    FREE(graph);
}

re_graph *
re_graph_dup(graph)
re_graph *graph;
{
    int i, j;
    re_graph *new_graph;
    re_node *node, *new_node;
    re_edge *edge, *new_edge;
    st_table *edge_ref_table, *node_ref_table;

    new_graph = re_graph_alloc();
    edge_ref_table = st_init_table(st_numcmp, st_numhash);
    node_ref_table = st_init_table(st_numcmp, st_numhash);

    /* copy nodes */
    re_foreach_node(graph, i, node){
	if (re_ignore_node(node)) continue;

	new_node = re_node_dup(node);
	new_node->id = re_num_nodes(new_graph);

	if (new_node->type == RE_PRIMARY_INPUT)
	    (void)array_insert_last(re_node *, new_graph->primary_inputs,
		    new_node);
	else if (new_node->type == RE_PRIMARY_OUTPUT)
	    (void)array_insert_last(re_node *, new_graph->primary_outputs,
		    new_node);

	(void) array_insert_last(re_node *, new_graph->nodes, new_node);
	(void)st_insert(node_ref_table, (char *)node->id, (char *)new_node);
    }

    /* copy edges */
    re_foreach_edge(graph, i, edge){
	/* Ignore this edge if it connects to a RE_IGNORE node */
	if (re_ignore_edge(edge)) continue;
	new_edge = re_edge_dup(edge);
	new_edge->id = re_num_edges(new_graph);
	(void) array_insert(re_edge *, new_graph->edges, i, new_edge);
	(void)st_insert(edge_ref_table, (char *)edge->id, (char *)new_edge);
    }

    /* cross link fanins and fanouts */
    re_foreach_node(graph, i, node){
	if (re_ignore_node(node)) continue;
	assert(st_lookup(node_ref_table, (char *)node->id, (char **)&new_node));
	re_foreach_fanin(node, j, edge){
	    if (re_ignore_edge(edge)) continue;
	    assert(st_lookup(edge_ref_table, (char *)edge->id, (char **)&new_edge));
	    (void) array_insert_last(re_edge *, new_node->fanins, new_edge);
	}

	re_foreach_fanout(node, j, edge){
	    if (re_ignore_edge(edge)) continue;
	    assert(st_lookup(edge_ref_table, (char *)edge->id, (char **)&new_edge));
	    (void) array_insert_last(re_edge *, new_node->fanouts, new_edge);
	}
    }

    /* cross link sink and source */
    re_foreach_edge(graph, i, edge){
	/* Ignore this edge if it connects to a RE_IGNORE node */
	if (re_ignore_edge(edge)) continue;
	assert(st_lookup(edge_ref_table, (char *)edge->id, (char **)&new_edge));

	assert(st_lookup(node_ref_table, (char *)edge->sink->id, (char **)&new_node));
	new_edge->sink = new_node;

	assert(st_lookup(node_ref_table, (char *)edge->source->id, (char **)&new_node));
	new_edge->source = new_node;
    }

    new_graph->s_type = graph->s_type;
    if (graph->control_name != NIL(char)){
	new_graph->control_name = util_strsav(graph->control_name);
    }
    st_free_table(edge_ref_table);
    st_free_table(node_ref_table);
    return new_graph;
}

re_node *
re_get_node(graph, index)
re_graph *graph;
int index;
{
    if (index >= re_num_nodes(graph)) 
	return NIL(re_node);
    
    return array_fetch(re_node *, graph->nodes, index);
}

re_edge *
re_get_edge(graph, index)
re_graph *graph;
int index;
{
    if (index >= re_num_edges(graph)) 
	return NIL(re_edge);
    
    return array_fetch(re_edge *, graph->edges, index);
}

re_node *
re_get_primary_input(graph, index)
re_graph *graph;
int index;
{
    if (index >= re_num_primary_inputs(graph)) 
	return NIL(re_node);
    
    return array_fetch(re_node *, graph->primary_inputs, index);
}

re_node *
re_get_primary_output(graph, index)
re_graph *graph;
int index;
{
    if (index >= re_num_primary_outputs(graph)) 
	return NIL(re_node);
    
    return array_fetch(re_node *, graph->primary_outputs, index);
}

re_edge *
re_get_fanin(node, index)
re_node *node;
int index;
{
    if (index >= re_num_fanins(node))
	return NIL(re_edge);

    return array_fetch(re_edge *, node->fanins, index);
}

re_edge *
re_get_fanout(node, index)
re_node *node;
int index;
{
    if (index >= re_num_fanouts(node))
	return NIL(re_edge);

    return array_fetch(re_edge *, node->fanouts, index);
}

re_edge *
re_create_edge(graph, source, sink, index, weight, breadth)
re_graph *graph;
re_node *source, *sink;
int index;		/* The fanin index of the "sink" node */
int weight;
double breadth;
{
    re_edge *new_edge;

    /* create the edge next */
    new_edge = ALLOC(re_edge, 1);
    new_edge->id = re_num_edges(graph);
    new_edge->weight = weight;
    new_edge->breadth = breadth;
    new_edge->temp_breadth = breadth;
    new_edge->source = source;
    new_edge->sink = sink;
    new_edge->sink_fanin_id = index;
    new_edge->num_val_alloc = 0;
    new_edge->initial_values = NIL(int);
    new_edge->latches = NIL(latch_t *);
    (void) array_insert_last(re_edge *, graph->edges, new_edge);
    (void) array_insert_last(re_edge *, source->fanouts, new_edge);
    (void) array_insert_last(re_edge *, sink->fanins, new_edge);

    return new_edge;
}

re_edge *
re_edge_dup(edge)
re_edge *edge;
{
    int i;
    re_edge *new_edge;

    new_edge = ALLOC(re_edge, 1);
    new_edge->sink_fanin_id = edge->sink_fanin_id;
    new_edge->weight = edge->weight;
    new_edge->breadth = edge->breadth;
    new_edge->temp_breadth = edge->breadth;

    new_edge->num_val_alloc = edge->num_val_alloc;
    if (edge->num_val_alloc > 0 && edge->initial_values != NIL(int)){
	new_edge->initial_values = ALLOC(int, new_edge->num_val_alloc);
	for (i = edge->weight; i-- > 0; ){
	    (new_edge->initial_values)[i] = (edge->initial_values)[i];
	}
    } else {
	new_edge->initial_values = NIL(int);
    }
    if (edge->latches != NIL(latch_t *)){
	new_edge->latches = ALLOC(latch_t *, new_edge->weight);
	for (i = edge->weight; i-- > 0; ){
	    (new_edge->latches)[i] = (edge->latches)[i];
	}
    } else new_edge->latches = NIL(latch_t *);

    return new_edge;
}

re_node * 
re_node_dup(node)
re_node *node;
{
    re_node *new_node;

    new_node = ALLOC(re_node, 1);
    new_node->node = node->node;
    new_node->fanins = array_alloc(re_edge *, 0);
    new_node->fanouts = array_alloc(re_edge *, 0);
    new_node->type = node->type;
    new_node->final_area = node->final_area;
    new_node->final_delay = node->final_delay;
    new_node->user_time = node->user_time;
    new_node->scaled_delay = RETIME_NOT_SET;
    new_node->scaled_user_time = 0;

    return new_node;
}
#endif /* SIS */
