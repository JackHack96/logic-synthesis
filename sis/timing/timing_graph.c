
#ifdef SIS
#include "timing_int.h"

/* Debug global variables */
static int lnum;
static double mmd;

/* function definition
    name:      tmg_network_to_graph()
    args:      network, model, algorithm (for CLOCK_OPTIMIZATION or 
               CLOCK_VERIFICATION)
    job:       converts the network to a latch to latch graph
    return value: graph_t *
    calls: tmg_alloc_graph(),
           delay_trace(),
	   tmg_alloc_node, g_add_vertex(), tmg_latch_get_phase(), tmg_get_latch_type(),
	   tmg_init_pi_arrival_time(),
	   tmg_build_graph(),
           tmg_update_K_edge(), set_mmd()
*/ 

graph_t *
tmg_network_to_graph(network, model, algorithm)
network_t *network;
delay_model_t model;
algorithm_type_t algorithm;
{
    node_t *pi, *node;
    graph_t *latch_graph;
    vertex_t *vertex;
    latch_t *latch;
    lsGen gen;

    mmd = 0;
    lnum = 1;

    assert(algorithm == OPTIMAL_CLOCK || algorithm == CLOCK_VERIFY);
    latch_graph = g_alloc();
    latch_graph->user_data = (gGeneric )tmg_alloc_graph(algorithm, network);
    if (tmg_get_phase_inv() && (array_n(CL(latch_graph)) > 1)) {
	(void)fprintf(sisout, "Error more than one clock phase\n");
	(void)fprintf(sisout, "-I flag assumes a two phase clocking scheme\n");
	(void)fprintf(sisout, "with inverted phases\n");
	tmg_free_graph_structure(latch_graph);
	return NIL(graph_t);
    }
    
    delay_trace(network, model);
    foreach_node(network, gen, node) {
	tmg_delay_alloc(node);
	DIRTY(node) = TRUE;
    }

    /* Enumerate latch to latch paths */
    /* ------------------------------ */

    foreach_primary_input (network, gen, pi) {
	if (clock_get_by_name(network, node_long_name(pi)) == 
	    NIL(sis_clock_t)) {

	    /* skip search for PI which is clock */
	    /* --------------------------------- */

	    if (network_is_real_pi(network, pi)) {
		if (HOST(latch_graph) == NIL(vertex_t)) {
		    vertex = g_add_vertex(latch_graph);
		    vertex->user_data = (gGeneric )tmg_alloc_node(algorithm);
		    ID(vertex) = 0;
		    TYPE(vertex) = NNODE;
		    HOST(latch_graph) = vertex;
		} else {
		    vertex = HOST(latch_graph);
		}
	    } else { /* node is a fake pi */
		     /* ----------------- */
		latch = latch_from_node(pi);
		assert(latch != NIL(latch_t));
		if ((vertex = MVER(latch)) == NIL(vertex_t)) {
		    vertex = g_add_vertex(latch_graph);
		    UNDEF(latch) = (char *)vertex;
		    vertex->user_data = (gGeneric )tmg_alloc_node(algorithm);
		    ID(vertex) = lnum;
		    lnum++;
		    TYPE(vertex) = LATCH;
		    LTYPE(vertex) = tmg_get_latch_type(latch);
		    NODE(vertex)->latch = latch;
		    if ((PHASE(vertex) = 
			 tmg_latch_get_phase(latch,CL(latch_graph))) 
			== NOT_SET) {
			tmg_free_graph_structure(latch_graph);
			return NIL(graph_t);
		    }
		}
	    }
	    tmg_init_pi_arrival_time(pi, model);
	    DIRTY(pi) = FALSE;
	    if (!tmg_build_graph(pi, latch_graph, vertex, model, algorithm)) {
		tmg_free_graph_structure(latch_graph);
		return NIL(graph_t);
	    }
	}
    }

    foreach_node (network, gen, node) {
	tmg_delay_free(node);
    }

    /* Sanity check for connectivity */
    /* ----------------------------- */

    if (debug_type == LGRAPH || debug_type == ALL) {
	(void)g_check(latch_graph);
    }
    
    /* Update the K for each edge */
    /* -------------------------- */

    if (!tmg_update_K_edge(latch_graph)) {
	tmg_free_graph_structure(latch_graph);
	return NIL(graph_t);
    }

    foreach_latch(network, gen, latch) {
	UNDEF(latch) = NIL(char);
    }
    return latch_graph;
}

/* function definition 
   name:    tmg_build_graph()
   args:    node, graph, source_vertex
   job:     from a source finds paths to all latches reachable from it
   return value: -
   calls: network_tfo(), get_pin_delay(), 
          tmg_alloc_node(),
          tmg_alloc_edge(),
	  node_get_delay(),
	  tmg_get_latch_type()
*/ 
int
tmg_build_graph(pi, latch_graph, source, model, algorithm)
node_t *pi;
graph_t *latch_graph;
vertex_t *source;
delay_model_t model;
algorithm_type_t algorithm;
{
    array_t *dfs_array;
    node_t *node, *fi;
    delay_time_t delay;
    delay_pin_t *pin_delay;
    latch_t *latch;
    vertex_t *sink;
    edge_t *edge;
    int i, j;
    
    dfs_array = network_tfo(pi, network_num_internal(pi->network));
    for (i = array_n(dfs_array)-1; i > -1; i--) {
	node = array_fetch(node_t *, dfs_array, i);
	maxdr(node) = -INFTY;
	maxdf(node) = -INFTY;
	mindr(node) = INFTY;
	mindf(node) = INFTY;

	if (node_function(node) != NODE_PO) {
	    foreach_fanin(node, j, fi) {
		if (!DIRTY(fi)) {
		    pin_delay = get_pin_delay (node, j, model);
		    delay.rise = pin_delay->drive.rise * delay_load(node);
		    delay.fall = pin_delay->drive.fall * delay_load(node);
		    if (node_function(node) != NODE_PI) {
			delay.rise += pin_delay->block.rise;
			delay.fall += pin_delay->block.fall;
	            } 
		    if (pin_delay->phase == PHASE_INVERTING ||
			pin_delay->phase == PHASE_NEITHER) {
			maxdr(node) = MAX(maxdr(node), maxdf(fi) + 
					  delay.rise);
			maxdf(node) = MAX(maxdf(node), maxdr(fi) + 
					  delay.fall);
			mindr(node) = MIN(mindr(node), mindf(fi) + 
					  delay.rise);
			mindf(node) = MIN(mindf(node), mindr(fi) + 
					  delay.fall);
		    } 
		    
		    if (pin_delay->phase == PHASE_NONINVERTING ||
			pin_delay->phase == PHASE_NEITHER) {
			maxdr(node) = MAX(maxdr(node), maxdr(fi) + 
					  delay.rise);
			maxdf(node) = MAX(maxdf(node), maxdf(fi) + 
					  delay.fall);
			mindr(node) = MIN(mindr(node), mindr(fi) + 
					  delay.rise);
			mindf(node) = MIN(mindf(node), mindf(fi) + 
					  delay.fall);
		    } 
		}
	    }
	    DIRTY(node) = FALSE;
	} else {
	    fi = node_get_fanin(node, 0);
	    if (!network_is_real_po(node->network, node)) {
		latch = latch_from_node(node);
		assert(latch != NIL(latch_t));
		if ((sink = MVER(latch)) == NIL(vertex_t)) {
		    sink = g_add_vertex(latch_graph);
		    UNDEF(latch) = (char *)sink;
		    sink->user_data = (gGeneric )tmg_alloc_node(algorithm);
		    ID(sink) = lnum;
		    lnum++;
		    TYPE(sink) = LATCH;
		    LTYPE(sink) = tmg_get_latch_type(latch);
		    NODE(sink)->latch = latch;
		    if ((PHASE(sink) = 
			 tmg_latch_get_phase(latch,CL(latch_graph)))
			== NOT_SET) {
			return FALSE;
		    }
		    
		}
		/* Edge guaranteed to be unique: 
		   sink appears only once in array */ 
		/* ------------------------------- */
		    
		if ((TYPE(source) == LATCH && TYPE(sink) == LATCH) &&
		    PHASE(source) == PHASE(sink) && 
		    (LTYPE(source) == LSH || LTYPE(source) == LSL) &&
		    LTYPE(source) == LTYPE(sink)) {
		    
		    /* A bug in SIS for some reason creates edges.
		       Have sent bug report to be fixed          */
		    /* ----------------------------------------- */

		    tmg_report_bug(source, sink);
		    
		} else {
		    if (!g_get_edge(source, sink, &edge)) {
			edge->user_data = (gGeneric) tmg_alloc_edge();
		    }
		    mmd = MAX(mmd, maxdf(fi));
		    mmd = MAX(mmd, maxdr(fi));
		    De(edge) = MAX(De(edge), maxdr(fi));
		    De(edge) = MAX(De(edge), maxdf(fi));
		    de(edge) = MIN(de(edge), mindr(fi));
		    de(edge) = MIN(de(edge), mindf(fi));
		    assert(De(edge) >= de(edge)); /* Sanity check */
		}
	    }
	}
    }
    for (i = 0; i < array_n(dfs_array); i++) {
	node = array_fetch(node_t *, dfs_array, i);
	DIRTY(node) = TRUE;
    }
    array_free(dfs_array);
    return TRUE;
}

/* function definition 
    name:     tmg_update_K_edge()
    args:     edge
    job:      stores the value of K in edge->user_data->K
    return value:  none
    calls:    tmg_latch_get_clock(),
              
*/

int
tmg_update_K_edge(g)
graph_t *g;
{
    lsGen gen;
    edge_t *edge;
    vertex_t *from, *to;
    sis_clock_t *clock1, *clock2, *clock;
    array_t *clock_array;
    int i;

    clock_array = GRAPH(g)->clock_order;
    foreach_edge(g, gen, edge) {
	from = g_e_source(edge);
	to = g_e_dest(edge);
	
	if ((TYPE(from) == NNODE) || (TYPE(to) == NNODE)) {
	    Ke(edge) = 0;
	} else {
	    clock1 = tmg_latch_get_clock(NODE(from)->latch);
	    clock2 = tmg_latch_get_clock(NODE(to)->latch);
	    if (clock1 == NIL(sis_clock_t) || clock2 == NIL(sis_clock_t)) {
		return 0;
	    }
	    if (clock1 == clock2) {
		if (LTYPE(from) == LTYPE(to)) {
		    Ke(edge) = 1;
		} else {
		    if (LTYPE(from) == LSH && LTYPE(to) == LSL) {
			Ke(edge) = 1;
		    } else {
			Ke(edge) = 0;
		    }
		}
	    } else {
		for (i = 0; i < array_n(clock_array); i++) {
		    clock = array_fetch(sis_clock_t *, clock_array, i);
		    if (clock == clock1) {
			Ke(edge) = 0;
			break;
		    } else if (clock == clock2) {
			Ke(edge) = 1;
			break;
		    }
		}
	    }
	} 
    }
    return 1;
}

/* function definition 
    name:     tmg_print_latch_graph()
    args:     graph
    job:      debug option to print the latch graph
    return value: -
    calls: -
*/ 
tmg_print_latch_graph(g)
graph_t *g;
{
    vertex_t *v, *f, *t;
    edge_t *e;
    lsGen gen, gen1;
    
    foreach_vertex(g, gen, v){
	foreach_in_edge(v, gen1, e){
	    f = g_e_source(e);
	    if (TYPE(f) == NNODE) {
		fprintf(sisout, "PI ->(%f- %f) %d\n", de(e), De(e), Ke(e));
	    } else {
		fprintf(sisout, "%d ->(%f - %f) %d\n", ID(f), 
			de(e), De(e), Ke(e));
	    }
	}
	if (TYPE(v) == NNODE) {
	    (void)fprintf(sisout, "HOST ->\n");
	} else {
	    fprintf(sisout, " %d  (phase %d)\n", ID(v), PHASE(v));
	}
	
	foreach_out_edge(v, gen1, e){
	    t = g_e_dest(e);
	    if (TYPE(t) == NNODE) {
		fprintf(sisout, "(%f - %f)->PO %d\n", de(e), De(e), Ke(e));
	    } else {
		fprintf(sisout, "(%f - %f)->%d %d\n", de(e), De(e),
			ID(t), Ke(e));
	    }
	}
    }
}

/* function definition 
    name:      tmg_init_pi_arrival_time()
    args:      node, model
    job:       initialize the arrival times in the latch graph
    return value: -
    calls: delay_get_pi_arrival_time()
*/ 
tmg_init_pi_arrival_time(node, model)
node_t *node;
delay_model_t model;
{
    latch_t *latch;
    lib_gate_t *gate;
    delay_time_t delay, arrival;
    delay_pin_t *pin_delay;
    
    if (network_is_real_pi(node->network, node)) {
	pin_delay = get_pin_delay(node, 0, model);
	
	if (!delay_get_pi_arrival_time(node, &arrival)) {
	    arrival.rise = 0.0;
	    arrival.fall = 0.0;
	}
	
	delay.rise = pin_delay->drive.rise * delay_load(node);
	delay.fall = pin_delay->drive.fall * delay_load(node);
	arrival.rise += delay.rise;
	arrival.fall += delay.fall;
	maxdr(node) = mindr(node) = arrival.rise;
	maxdf(node) = mindf(node) = arrival.fall;
    } else {
	latch = latch_from_node(node);
	assert(latch != NIL(latch_t));
	if (model == DELAY_MODEL_LIBRARY) {
	    gate = latch_get_gate(latch);
	    assert(gate != NIL(lib_gate_t));
	    delay.rise = gate->clock_delay->block.rise +
	    gate->clock_delay->drive.rise * delay_load(node);
	    delay.fall = gate->clock_delay->block.fall +
	    gate->clock_delay->drive.fall * delay_load(node);
	    maxdr(node) = mindr(node) = delay.rise;
	    maxdf(node) = mindf(node) = delay.fall;
	} else {
	    pin_delay = get_pin_delay(node, 0, model);
	    delay.rise = pin_delay->drive.rise * delay_load(node);
	    delay.fall = pin_delay->drive.fall * delay_load(node);
	    maxdr(node) = mindr(node) = delay.rise;
	    maxdf(node) = mindf(node) = delay.fall;
	} 
    }
}

/* havent figured this one yet- occurs in one huge example 
   The problem doesnt occur if you read library, map and do 
   c_opt. But should you read library, map, spit out mapped circuit
   then re-read and do c_opt, this error creeps up. Possibly a 
   subtle bug in the map package!! */

tmg_report_bug(source, sink)
vertex_t *source, *sink;
{
    
    (void)fprintf(sisout, "Error, path between latches on same phase\n");
    (void)fprintf(sisout, "\t \t input \t output\n");
    (void)fprintf(sisout, "source \t %s \t %s \n", 
		  node_long_name(latch_get_input(NODE(source)->latch)),
		  node_long_name(latch_get_output(NODE(source)->latch)));
    (void)fprintf(sisout, "sink \t %s \t %s \n", 
		  node_long_name(latch_get_input(NODE(sink)->latch)),
		  node_long_name(latch_get_output(NODE(sink)->latch)));
    
}


/* function definition 
    name: tmg_delay_alloc(), tmg_delay_free()
    args: node
    job: allocate/free data structure from node->undef1 used for storing
         long / short paths
    return value: void
    calls: -
*/ 
static void
tmg_delay_alloc(node)
node_t *node;
{
    UNDEF(node) = (char *)ALLOC(my_delay_t, 1);
}

static void
tmg_delay_free(node)
node_t *node;
{
    my_delay_t *md;
    
    md = MDEL(node);
    FREE(md);
}
#endif /* SIS */
