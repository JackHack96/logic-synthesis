
/**********************************************************************/
/*           This is the input processor for maxflow routines         */
/*                                                                    */
/*           Author: Hi-Keung Tony Ma                                 */
/*           last update : 03/28/1988                                 */
/**********************************************************************/

#include "sis.h"
#include "maxflow_int.h"

mfgptr
mf_alloc_graph()
{
    mfgptr graph;

    if (!(graph = MF_ALLOC(1, mf_graph_t)))
        mf_error("Memory allocation failure", "mf_alloc_graph");
    graph->node_table = st_init_table(strcmp, st_strhash);
    return(graph);
}/* end of mf_alloc_graph */

/*
 * read a node structure into the graph  
 */
void
mf_read_node(graph, node_name, type)
mfgptr graph;
char *node_name;
int type; /* source = 1, sink = 2, fictitious = -1,  others = 0 */
{
    mfnptr n;
    char *name;
    void reallocate_graph_node();

    if (st_is_member(graph->node_table, node_name))
        mf_error("Node defined twice", "mf_read_node");

    /* allocate new node from free storage */
    if (graph->num_of_node == graph->max_num_of_nptr) {
        reallocate_graph_node(graph);
    }
    if (!(n = MF_ALLOC(1, mf_node_t))) mf_error("Memory allocation failure", "mf_read_node");
    graph->nlist[graph->num_of_node] = n;
    graph->num_of_node++;

    /* initialize node entries */

    n->name = name = util_strsav(node_name);
    (void)st_insert(graph->node_table, name ,(char *)n);
    n->in_edge = NIL(mfeptr);

    /* check node type */
    if (type == 1) {
        if (graph->source_node)
            mf_error("Multiple declaration of source node", "mf_read_node");
        else graph->source_node = n;
    }
    else if (type == 2) {
        if (graph->sink_node)
            mf_error("Multiple declaration of sink node", "mf_read_node");
        else graph->sink_node = n;
    }
    else if (type == -1) n->flag |= FICTITIOUS;
}/* end of mf_read_node */

int
mf_remove_node(graph, name)
mf_graph_t *graph;
char *name;
{
    int i, j, k;
    char *dummy;
    mf_node_t *node, *new_node;
    mf_edge_t *edge, *new_edge;

    if ((node = mf_get_node(graph, name)) == NIL(mf_node_t)){
	return 0;
    }
    mf_foreach_fanin(node, i, edge){
	new_node = mf_tail_of_edge(edge);
	mf_foreach_fanout(new_node, j, new_edge){
	   if(new_edge == edge ) {
		--(new_node->nout);
	       break;
	   }
	}
	for (k = j+1; k <= new_node->nout; k++){
	    new_node->out_edge[k-1] = new_node->out_edge[k];
	}
	FREE(edge);
    }
    mf_foreach_fanout(node, i, edge){
	new_node = mf_head_of_edge(edge);
	mf_foreach_fanin(new_node, j, new_edge){
	   if(new_edge == edge ) {
		--(new_node->nin);
	       break;
	   }
	}
	for (k = j+1; k <= new_node->nin; k++){
	    new_node->in_edge[k-1] = new_node->in_edge[k];
	}
	FREE(edge);
    }
    FREE(node->in_edge);
    FREE(node->out_edge);

    (void)st_delete(graph->node_table, &name, &dummy);
    mf_foreach_node(graph, i, new_node){
	if (new_node == node ) break;
    }
    for (k = i+1; k < graph->num_of_node; k++){
	graph->nlist[k-1] = graph->nlist[k];
    }
    --(graph->num_of_node);
    FREE(node->name);
    FREE(node);

    return 1;
}

mf_change_node_type(graph, node, type)
mf_graph_t *graph;
mf_node_t *node;
int type; /* source = 1, sink = 2,  others = 0 */
{
    if (type == 1){
	graph->source_node = node;
    }
    if (type == 2){
	graph->sink_node = node;
    }
    if (type == 0 && mf_get_sink_node(graph) == node ){
	graph->sink_node = NIL(mf_node_t);
    }
    if (type == 0 && mf_get_source_node(graph) == node ){
	graph->source_node = NIL(mf_node_t);
    }
}

/* read and get edges structures */
void
mf_read_edge(graph, node1, node2, capacity)
mfgptr graph;
char *node1, *node2;
int capacity;
{ 
    mfeptr e;
    mfnptr n1, n2;
    void reallocate_out_edge();

    if (capacity < 0) mf_error("Negative capacity assigned","mf_read_edge");

    if ((n1 = mf_get_node(graph, node1)) == NIL(mf_node_t)){
        mf_error("Node undefined", "mf_read_edge");
    }
    if (n1->nout == n1->max_nout) reallocate_out_edge(n1);

    if ((n2 = mf_get_node(graph, node2)) == NIL(mf_node_t)){
        mf_error("Node undefined", "mf_read_edge");
    }

    if (n1 == n2) mf_error("Self-Loop is not allowed", "mf_read_edge");

    if (!(e = MF_ALLOC(1, mf_edge_t)))
        mf_error("Memory allocation failure", "mf_read_edge");
    e->inode = n1;
    e->onode = n2;
    e->capacity = capacity;
    n1->out_edge[n1->nout] = e;
    n1->nout++;
    n2->nin++;
}/* end of mf_read_edge */

/* 
 * reread or create an edge structure 
 */
int
mf_reread_edge(graph, node1, node2, capacity)
mfgptr graph;
char *node1, *node2;
int capacity;
{ 
    int i;
    mfeptr e;
    mfnptr n1, n2;
    void reallocate_out_edge();

    if (capacity < 0) mf_error("Negative capacity assigned","mf_reread_edge");

    if ((n1 = mf_get_node(graph, node1)) == NIL(mf_node_t)){
        mf_error("Node undefined", "mf_reread_edge");
    }
    if ((n2 = mf_get_node(graph, node2)) == NIL(mf_node_t)){
        mf_error("Node undefined", "mf_reread_edge");
    }
    if (n1 == n2) mf_error("Self-Loop is not allowed", "mf_reread_edge");
    mf_foreach_fanout(n1, i, e){
	if (mf_head_of_edge(e) == n2){
	    e->capacity = capacity;
	    return 0;
	}
    }

    if (n1->nout == n1->max_nout) reallocate_out_edge(n1);
    if (!(e = MF_ALLOC(1, mf_edge_t)))
        mf_error("Memory allocation failure", "mf_reread_edge");
    e->inode = n1;
    e->onode = n2;
    e->capacity = capacity;
    n1->out_edge[n1->nout] = e;
    n1->nout++;
    n2->nin++;
    return 1;
}/* end of mf_reread_edge */

/*
* print mf_error message and die
*/
void
mf_error(msg1, msg2)
char *msg1, *msg2;
{
    (void) fprintf(siserr,"\n");
    (void) fprintf(siserr,"%s in routine %s\n",msg1, msg2);
    abort();
}/* end of mf_error */


void
reallocate_graph_node(graph)
mfgptr graph;
{
    mfnptr *ntemp;
    int i;

    graph->max_num_of_nptr += 50;
    if (!(ntemp = MF_ALLOC(graph->max_num_of_nptr, mfnptr)) &&
	    graph->max_num_of_nptr)
        mf_error("Memory allocation failure", "reallocate_graph_node");
    for (i = 0; i < graph->num_of_node; i++) {
        ntemp[i] = graph->nlist[i];
    }
    if (graph->nlist != NIL(mfnptr)) free((char *)graph->nlist);
    graph->nlist = ntemp;
}/* end of reallocate_graph_node */

void
reallocate_out_edge(node)
mfnptr node;
{
    int i;
    mfeptr *etemp;

    node->max_nout += 10;
      if (!(etemp = MF_ALLOC(node->max_nout, mfeptr)) && node->max_nout)
        mf_error("Memory allocation failure", "reallocate_out_edge");
    for (i = 0; i < node->nout; i++) {
        etemp[i] = node->out_edge[i];
    }
    if (node->out_edge != NIL(mfeptr)) FREE(node->out_edge);
    node->out_edge = etemp;
}/* end of reallocate_out_edge */


char *
MF_calloc(a, b)
int a, b;
{
    int i, size;
    char *p;

    size = a * b;
    p = ALLOC(char, size);
    for(i = 0; i < size; i++){
	*(p+i) = 0;
    }

    return p;
}

void
mf_free_cutset(cutset)
mfcptr cutset;
{
    FREE(cutset->from_node);
    FREE(cutset->to_node);
    FREE(cutset->capacity);
    FREE(cutset);
}

void 
mf_free_node(node)
mfnptr node;
{
    int i;

    for (i = 0; i < node->nout; i++){
        FREE(node->out_edge[i]);
    }
    FREE(node->in_edge);
    FREE(node->out_edge);
    FREE(node->name);

    FREE(node);
}

void
mf_free_graph(graph)
mfgptr graph;
{
    int i;

    for (i = 0; i < graph->num_of_node;i++){
        mf_free_node(graph->nlist[i]);
    }
    FREE(graph->nlist);
    st_free_table(graph->node_table);
    FREE(graph);
}
