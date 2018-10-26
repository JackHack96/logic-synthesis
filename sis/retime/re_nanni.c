
#ifdef SIS
#include "sis.h"
#include "retime_int.h"

static int retime_set_outputs();
static st_table *retime_get_po_from_array();
static array_t *retime_get_slow_nodes();
static int re_translate_retiming_vector();
static array_t *retime_get_input_reachable();

/*
 * Use static arrays to cut down the allocation and freeing of the arrays
 * used to store information during the delay tracing
 */
static bool *static_valid_table;
static double *static_delay_table;

static void
retime_allocate_delay_data(n)
int n;
{
    static_valid_table = ALLOC(bool, n);
    static_delay_table = ALLOC(double, n);
}

static void
retime_free_delay_data()
{
    FREE(static_valid_table);
    FREE(static_delay_table);
}

/* 
 * Carry out the retiming procedure outlined in nanni's paper....
 * return 1 if success is achieved --
 */

int
retime_nanni_routine(graph, d_clk, retiming)
re_graph *graph;
double d_clk;
int *retiming;		/* Array storing the retiming vector */
{
    int i, j, n;
    re_node *re_no;
    array_t *M;

    /* Initialization */
    n = re_num_nodes(graph);
    for ( i = n; i-- > 0; ){
	retiming[i] = 0;
    }
    retime_allocate_delay_data(n);

    /* Iteration according to Nanni's paper (and Saxe's thesis) */
    for (i = n; i-- > 0; ){
	M = retime_get_slow_nodes(graph, d_clk);
	if ( array_n(M) == 0){
	    array_free(M);
	    re_translate_retiming_vector(graph, retiming);
	    retime_free_delay_data();
	    return TRUE;
	}

	for( j = array_n(M); j-- > 0; ){
	    re_no = array_fetch( re_node *, M, j);
	    retime_single_node(re_no, 1);
	    retiming[re_no->id] += 1;
	}
	if (retime_set_outputs(graph, M, retiming) == FALSE){
	    array_free(M);
	    retime_free_delay_data();
	    return FALSE;
	}
	array_free(M);
    }
    /* No feasible retiming exists for d_clk */
    retime_free_delay_data();
    return FALSE;
}
/*
 * Delays all outputs and speedup all inputs --
 * returns FALSE if  this is not possible without violating 
 * path weights
 */

static int
retime_set_outputs(graph, M, retiming)
re_graph *graph;
array_t *M;
int *retiming;
{
    int i;
    array_t *S;
    re_node *node;
    st_table *table;

    if ( (table = retime_get_po_from_array(M)) != NIL(st_table)){
	/* 
	 * Retime all the o/p not in M, i.e. those o/p which have not
	 * been retimed. This is required since at this stage at least
	 * one o/p has been retimed
	 */
	re_foreach_primary_output( graph, i, node){
	    if ( !st_is_member(table, (char *)node) ){
		retime_single_node(node, 1);
		retiming[node->id] += 1;
	    }
	}

	/*
	 * Now to find the vertices,v, such that a zero weight
	 * path exists from i/p to v. These are in the array S
	 */
	S = retime_get_input_reachable(graph);
	for ( i = array_n(S); i-- > 0; ){
	    node = array_fetch( re_node *, S, i);
	    if ( (node->type == RE_PRIMARY_OUTPUT) &&
		     (st_is_member(table, (char *)node))){
		array_free(S);
		st_free_table(table);
		return FALSE;
	    }
	    /* Retime the node */
	    retime_single_node( node, 1);
	    retiming[node->id] += 1;
	}
	array_free(S);
	(void)st_free_table(table);
    }
    return TRUE;
}

/*
 * Just gets the primary outputs from the array 
 */
static st_table *
retime_get_po_from_array(M)
array_t *M;
{
    int i;
    re_node *node;
    st_table *table;

    table = NIL(st_table);
    for (i = array_n(M); i-- > 0; ){
	node = array_fetch( re_node *, M, i);
	if ( node->type == RE_PRIMARY_OUTPUT) {
	    if ( table == NIL(st_table)){
		table = st_init_table( st_ptrcmp, st_ptrhash);
	    }
	    (void)st_insert(table, (char *)node, "");
	}
    }

    return table;
}

/*
 * Gets the nodes in re_graph that are "slow" w.r.t. the given clock "clk"
 * 
 * Uses the arrival time data for the primary input and the required times
 * for the outputs while doing this computation ...
 */
static array_t *
retime_get_slow_nodes(graph, clk)
re_graph *graph;
double clk;
{
    int i, n = re_num_nodes(graph);
    array_t *M;
    re_node *node;
    double offset;

    for (i = n ; i-- > 0; ) {
	static_valid_table[i] = FALSE;
	static_delay_table[i] = 0.0;
    }

    /* primary input nodes need not be evaluated */
    re_foreach_node(graph, i, node){
	if (re_num_fanins(node) == 0){
	    static_valid_table[i] = TRUE;
	    if (node->user_time > RETIME_TEST_NOT_SET) {
		static_delay_table[i] = node->user_time;
	    }
	}
    }

    /* compute delays */
    for (i = n; i-- > 0; ) {
	if (!static_valid_table[i]) {
	    node = array_fetch(re_node *, graph->nodes, i);
	    re_evaluate_delay(node, static_valid_table, static_delay_table);
	}
    }

    /* find the nodes with arrival time > clk */
    M = array_alloc(re_node *,0);
    for (i = n; i-- > 0; ) {
	offset = 0.0;
	node = array_fetch(re_node *, graph->nodes, i);
	if (node->type == RE_PRIMARY_OUTPUT &&
		node->user_time > RETIME_TEST_NOT_SET){
	    offset = node->user_time;
	} 
	if ((static_delay_table[i] - offset) > clk) {
	    array_insert_last(re_node *, M, node);
	}
    }

    /* return */
    return(M);
}

/*
 * Get the nodes reachable from pi with 0 weight paths
 */

static array_t *
retime_get_input_reachable(graph)
re_graph *graph;
{
    array_t *S;
    re_edge *edge;
    re_node *node;
    st_table *table;
    int i, j, first, last, more_to_come;

    /*
     * Perform bfs to get these nodes 
     */
    S = array_alloc( re_node *, 0);
    table = st_init_table( st_ptrcmp, st_ptrhash);
    re_foreach_primary_input( graph, i, node){
	(void)st_insert(table, (char *)node, "");
	array_insert_last( re_node *, S, node);
    }
    
    first = 0;
    more_to_come = TRUE;
    while ( more_to_come){
	last = array_n(S);
	more_to_come = FALSE;
	for( i = first; i < array_n(S); i++){
	    node = array_fetch( re_node *, S, i);
	    re_foreach_fanout( node, j, edge){
		if (re_ignore_edge(edge)) continue;
		if ( (edge->weight == 0) && 
			(!st_is_member(table,(char *)(edge->sink)))){
		    (void)st_insert(table, (char *)(edge->sink), "");
		    array_insert_last( re_node *, S, edge->sink);
		    more_to_come = TRUE;
		}
	    }
	}
	first = last;
    }
    st_free_table(table);

    return(S);
}

static int
re_translate_retiming_vector(graph, r)
re_graph *graph;
int *r;
{
    re_node *re_no;
    int i, max_po = 0, max_pi = 0, n = re_num_nodes(graph);

    re_foreach_node(graph, i, re_no){
	if (re_no->type == RE_PRIMARY_INPUT){
	    max_pi = MAX(max_pi, r[i]);
	} else if (re_no->type == RE_PRIMARY_OUTPUT){
	    max_po = MAX(max_po, r[i]);
	}
    }
    assert(max_pi == max_po);
    for (i = n; i-- > 0; ){
	re_no = array_fetch(re_node *, graph->nodes, i);
	r[i] -= max_pi;
	retime_single_node(re_no, -max_pi);
    }
}
#endif /* SIS */
