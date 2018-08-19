/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/pld/xln_ULM_util.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:56 $
 *
 */
/*
 *  util.c
 *  ******
 *  functions contained in this file:
 *	change_edge_capacity()
 *	get_maxflow()
 *	get_maxflow_edge()
 *	graph2network_node()
 *	print_fanin()
 *	comp_ptr()
 *	count_intsec_union()
 *	print_table()
 *	print_array()
 *  Import descriptions:
 *	sis.h		: macros for misII
 *	ULM_int.h	: macros for universal logic module package
 */

#include "sis.h"
#include "pld_int.h"
/*
 *  change_edge_capacity()
 *	Change the capacity of an edge in the flow network
 *	Assumes this edge is from bottom node to top node both of which
 *	corresponds to the given node in the network
 */

void
change_edge_capacity(graph, node, capacity)

mf_graph_t	*graph;
node_t		*node;
int		capacity;

{
    char	name1[BUFSIZE];
    char	name2[BUFSIZE];

    (void) sprintf(name1, "%s_top", node_long_name(node));
    (void) sprintf(name2, "%s_bottom", node_long_name(node));
    (void) mf_reread_edge(graph, name2, name1, capacity);
}



/*
 *  get_maxflow()
 *	Get the value of maxflow for the given flow network
 */


int
get_maxflow(graph)

mf_graph_t	*graph;		/* pointer to graph */

{
    int		i;
    int		flow;
    int		save_flow;
    mf_edge_t	*edge;
    mf_node_t	*source;

    /* Print the input graph */

    maxflow(graph, OFF);

    /* Get the value of maxflow */
    flow = 0;
    source = mf_get_source_node(graph);
    mf_foreach_fanout(source, i, edge) {
	save_flow = flow;
	flow += mf_get_edge_flow(edge);

	/* Check if the overflow has occured */
	if (flow < save_flow) {
	    flow = MAXINT;
	    break;
	}
    }

    return(flow);
}




/*
 *  get_maxflow_edge()
 *	Get the maxflow network edge from the graph nodes
 *	RETURNS
 *	edge		: pointer to the network edge
 */


mf_edge_t *
get_maxflow_edge(graph, name1, name2)

mf_graph_t	*graph;	/* pointer to graph */
char		*name1;	/* name of the source node of an edge */
char		*name2;	/* name of the sink node of an edge */

{


    int		i;
    mf_edge_t	*edge;
    mf_node_t	*node1;
    mf_node_t	*node2;


    node1 = mf_get_node(graph, name1);
    node2 = mf_get_node(graph, name2);
    mf_foreach_fanout(node1, i, edge) {
	if (node2 == mf_head_of_edge(edge)) {
	    return(edge);
	}
    }
    return(NULL);


}





/*
 *  graph2network_node()
 *	Get the network node from the graph node
 *
 * 	RETURN
 *	node		: pointer to the network node
 *  Possible errors
 *  ---------------
 *	Assumes that the name of each graph_node has a modifier such as _top, 
 *	_bottom, _left, or _right in the end.
 *
 */


node_t *
graph2network_node(network, graph_node)
network_t	*network;	/* pointer to network */
mf_node_t	*graph_node;	/* pointer to a node in the graph */
{
    char	buf[BUFSIZE], *ptr;

    (void) strcpy(buf, mf_node_name(graph_node));
    ptr = strrchr(buf, '_');
    if (ptr) 
	*ptr = '\0';
    return(network_find_node(network, buf));
}

/*
 *  print_fanin()
 *	Print the names of the fanin nodes of a given node
 *  Arguments:
 *  ---------
 *	node		: node whose fanins are to be printed
 *
 */

void	print_fanin(node)

node_t	*node;

{

    node_t	*fanin;
    int		i;

    (void) printf("Fanins of node %s = ", node_long_name(node));
    foreach_fanin(node, i, fanin) {
	(void) printf("%s ", node_long_name(fanin));
    }
    (void) printf("\n");

}



/*
 *  comp_ptr()
 *	Compare function for array_sort
 *
 *  RETURNS
 *  -------
 *	-1,0,1		: if obj1 <,=,> obj2
 */

int	comp_ptr(obj1, obj2)

char	*obj1;
char	*obj2;

{

    int	value;

    value = (int)obj1 - (int)obj2;
    if (value < 0) {
	return(-1);
    } else if (value > 0) {
	return(1);
    } else {
	return(0);
    }
}




/*
 *  count_intsec_union()
 *	Count the cardinality of the intersection and union of two sets given
 *	that are stored in array_t.
 *  
 *	Assume that the arrays are sorted in advance
 */


void
count_intsec_union(array1, array2, num_intsec, num_union)

array_t		*array1, *array2;
int		*num_intsec, *num_union;

{

    int		i, j;
    char	*element1, *element2;


    *num_intsec = *num_union = 0;

    for(i = 0, j = 0; ;) {
	if (i > array_n(array1) - 1) {
	    *num_union += array_n(array2) - j;
	    break;
	}
	if (j > array_n(array2) - 1) {
	    *num_union += array_n(array1) - i;
	    break;
	}
	element1 = array_fetch(char *, array1, i);
	element2 = array_fetch(char *, array2, j);
	if (element1 < element2) {
	    ++i;
	} else if (element1 > element2) {
	    ++j;
	} else {
	    ++(*num_intsec);
	    ++i; ++j;
	}
	++(*num_union);
    }

}




/*
 *  Print an entry in the table (debug use)
 *
 */

enum st_retval
print_table_entry(key, value, arg)

char *key;
char *value;
char *arg;

{

    (void) printf("key = %s, value = %x arg = %s\n", key, value, arg);
    return(ST_CONTINUE);

}



void print_table(table)
st_table	*table;
{
    st_foreach(table, print_table_entry, (char *)NULL); 
}


void print_array(array, hash_table)
array_t		*array;
nodeindex_t	*hash_table;
{
    int		i;
    node_t	*node;

    for(i = 0; i < array_n(array); i++) {
	node = array_fetch(node_t *, array, i);
	(void) printf("%s(%d) ", node_long_name(node), 
		nodeindex_indexof(hash_table, node));
    }
    (void) printf("\n"); 
}

