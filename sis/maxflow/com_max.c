
#include "sis.h"
#include "maxflow_int.h"

#define MAXLINE 200

/* ARGSUSED */
com_maxflow(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    maxflow_debug = 1 - maxflow_debug;
}

/* ARGSUSED */
com_test_mf(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
   int i,j;
   mf_graph_t *graph;
   mf_cutset_t *cutset;
   mf_node_t *node;
   mf_edge_t *edge;
   array_t *from, *to, *flow;

   /*
    * Build up a dummy network and print out the
    * cutset and the flows 
    */
    
    graph = mf_alloc_graph();

    /* read in the network */

    mf_read_node( graph, "mf_source", 1);
    mf_read_node( graph, "mf_sink", 2);
    mf_read_node( graph, "a", 0);
    mf_read_node( graph, "b", 0);
    mf_read_node( graph, "c", 0);
    mf_read_node( graph, "d", 0);
    mf_read_node( graph, "e", 0);
    mf_read_node( graph, "f", 0);
    mf_read_node( graph, "g", 0);

    /* Now the capacities */
    mf_read_edge( graph, "mf_source","a", 2);
    mf_read_edge( graph, "mf_source","b", 2);
    mf_read_edge( graph, "a","c", 3);
    mf_read_edge( graph, "b","c", 1);
    mf_read_edge( graph, "b","d", 2);
    mf_read_edge( graph, "c","f", 4);
    mf_read_edge( graph, "d","e", 1);
    mf_read_edge( graph, "e","mf_sink", 2);
    mf_read_edge( graph, "f","e", 2);
    mf_read_edge( graph, "f","g", 3);
    mf_read_edge( graph, "g","mf_sink", 1);

    mf_display_graph(sisout,graph);
    maxflow( graph, 0);

    cutset = mf_get_cutset( graph, &from, &to, &flow);

    (void) fprintf( sisout,"    From       to       flow\n");
    for ( i = 0; i < array_n(from); i++){
	(void) fprintf( sisout, " %-10s%-10s  %3d\n",
	    array_fetch( char *, from, i),
	    array_fetch( char *, to, i),
	    array_fetch( int , flow, i));
    }
    mf_free_cutset( cutset);

    (void)fprintf(sisout,"Cutset has %d arcs\n", mf_sizeof_cutset(graph));
    mf_foreach_node(graph, i, node){
	mf_foreach_fanin(node, j, edge){
	    if ( mf_is_edge_on_mincut(edge)){
		(void)fprintf(sisout,"%s -> %s : Cap = %d, flow = %d\n",
		mf_node_name(mf_tail_of_edge(edge)),
		mf_node_name(mf_head_of_edge(edge)),
		mf_get_edge_capacity(edge),
		mf_get_edge_flow(edge));
	    }
	}
    }
    (void)mf_remove_node( graph, "d");
    (void)mf_reread_edge(graph,"g","mf_sink",3);
    mf_change_node_type(graph,mf_get_source_node(graph),0);
    mf_change_node_type(graph, mf_get_node(graph,"a"),1);
    (void)fprintf(sisout,"After modfication \n");
    mf_display_graph(sisout, graph);
    maxflow(graph,0);
    mf_foreach_node(graph, i, node){
	mf_foreach_fanin(node, j, edge){
	    if ( mf_is_edge_on_mincut(edge)){
		(void)fprintf(sisout,"%s -> %s : Cap = %d, flow = %d\n",
		mf_node_name(mf_tail_of_edge(edge)),
		mf_node_name(mf_head_of_edge(edge)),
		mf_get_edge_capacity(edge),
		mf_get_edge_flow(edge));
	    }
	}
    }

    mf_free_graph( graph);
    array_free( from);
    array_free( to);
    array_free( flow);
}


/* ARGSUSED */
com__run_maxflow(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
   int i,j, cap;
   FILE *input;
   char s[MAXLINE];
   char from[MF_MAXSTR], to[MF_MAXSTR];
   mf_graph_t *graph;
   mf_cutset_t *cutset;
   array_t *from_array, *to_array, *flow;

   if ( argc < 2 ) goto run_mf_usage;

    graph = mf_alloc_graph();
   input = fopen(argv[1],"r");
   if ( input == NIL(FILE)) return 1;

   j = 0;
   while( fgets(s, MAXLINE, input) != NULL){
	if ( strcmp(s,"") == 0) continue;
	j++;
	if ( j == 1){
	    /* get and create the source and sink */
	    i = sscanf(s,"Source:%s Sink:%s", from, to);
	    if ( i != 2){
		error_append("Improper Input format\n");
		goto abort;
	    }
	    mf_read_node(graph, from, 1); /* source */

	    mf_read_node(graph, to, 2); /* sink */
	    continue;
	} else if ( j == 2){
	    continue;
	} else {
	    i = sscanf(s,"%s %s %d", from, to, &cap);
	    if ( i != 3) {
		error_append("Improper input format\n");
		goto abort;
	    }
	    if ( mf_get_node(graph, from) == NIL(mf_node_t)){
		mf_read_node(graph, from, 0);
	    }
	    if ( mf_get_node(graph, to) == NIL(mf_node_t)){
		mf_read_node(graph, to, 0);
	    }
	    mf_read_edge(graph, from, to, cap);
	}
   }

    maxflow( graph, 0);

    cutset = mf_get_cutset( graph, &from_array, &to_array, &flow);

    (void) fprintf( sisout,"Following maxflow/mincut found:\n");
    (void) fprintf( sisout,"    From       to       flow\n");
    for ( i = 0; i < array_n(from_array); i++){
	(void) fprintf( sisout, " %-10s%-10s  %3d\n",
	    array_fetch( char *, from_array, i),
	    array_fetch( char *, to_array, i),
	    array_fetch( int , flow, i));
    }
    mf_free_cutset( cutset);

    (void) fclose(input);
    mf_free_graph( graph);
    array_free( from_array);
    array_free( to_array);
    array_free( flow);

    return 0;
run_mf_usage:
    (void)fprintf(sisout,"Usage: _run_maxflow input_file \n");
    return 1;
abort:
    (void)fprintf(sisout,"%s\n", error_string());
    (void) fclose(input);
    mf_free_graph(graph);
    return 1;
}

init_maxflow()
{
    maxflow_debug = FALSE;

    com_add_command("_maxflow", com_maxflow, 0);
    com_add_command("_test_mf", com_test_mf, 0);
    com_add_command("_run_maxflow", com__run_maxflow, 0);
}

end_maxflow()
{
}
