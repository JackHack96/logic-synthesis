
/* 
 * $Log: re_initial.c,v $
 * Revision 1.1.1.1  2004/02/07 10:14:48  pchong
 * imported
 *
 * Revision 1.5  1993/05/06  01:25:21  sis
 * *** empty log message ***
 *
 * Revision 1.4  22/.0/.1  .0:.5:.3  sis
 *  Updates for the Alpha port.
 * 
 * Revision 1.3  1992/05/06  19:00:16  sis
 * SIS release 1.1
 *
 * Revision 1.2  1992/04/17  22:13:47  sis
 * *** empty log message ***
 *
 * Revision 1.1  92/01/08  17:45:43  sis
 * Initial revision
 * 
 */

#ifdef SIS
#include "sis.h"
#include "retime_int.h"

/*
 * UPDATING INITIAL STATE AFTER RETIMING 
 *
 * Use algorithm by Touati & Brayton TRANSCAD January 93
 *
 * May fail if the initial state is not reachable from itself.
 * In that case, use "add_reset" before retiming and "remove_reset" after retiming.
 * 
 * "*can_init" = 1 if update was possible, edges annotated with new reset state 
 * "*can_init" = 0 if update not possible  (graph is not modified)
 *
 * need to extract a sequence ending into the reset state of sufficient length
 * need to be able to simulate a node forward 
 * need to be able to extract inputs vectors 
 */

typedef struct {
    re_node **current;		       /* top of stack pointer */
    re_node **top;		       /* max value for top of stack pointer */
    re_node **bottom;		       /* stack of ready nodes implemented as an array */
    int *in_stack;		       /* flag: if 1, the node is already in the stack */
} re_stack_t;


static int make_retiming_negative();
static int min_n_input_latch();
static int re_stack_is_empty();
static int simulate_node_function();
static re_node *re_stack_pop();
static re_stack_t *re_stack_init();
static void re_stack_free();
static void re_stack_push();
static void realloc_initial_values();
static void retime_get_input_order();
static void retime_node_forward();
static void retime_primary_input_forward();
static void retime_primary_output_forward();
static void set_initial_state();
static void simulate_forward();
static st_table *retime_get_latch_corresp();
static st_table *retime_swap_latches();


 /* ARGSUSED */

int retime_update_init_states(network, graph, r_vec, should_init, can_init)
network_t *network;    /* Network to get STG from */
re_graph *graph;       /* graph representation of network for retiming */
int *r_vec;	       /* solution vector: r_vec[i] corresponds to node = array_fetch(re_node *, graph->nodes, i) */
int should_init;       /* currently unused: always try */
int *can_init;	       /* Flag to specify if all went OK */
{
    int j, i, n_shift;		 /* = max(r(v)-r(host)) */
    int *retiming;		 /* Saved vector of retiming values */
    st_table *reset_ancestor;	 /* maps latch to its value in ancestor state */
    st_table *orig_reset_ancestor;	
    st_table *latch_table;	 /* mapes latches in orig and dup networks */
    array_t *input_seq;		 /* sequence of (PI inputs) in reverse temporal order */
    st_table *input_order;	/* maps external PI in 'network' to small integers: their order of occurrence in 'graph' */
    st_generator *stgen;
    latch_t *latch;
    char *input;
    int value;
    network_t *dup_network;
    
    *can_init = TRUE;
    if (network == NIL(network_t)) {
    *can_init = FALSE;
    return 2;		       /* Set initial states to Dont Cares */
    }

    retiming = ALLOC(int, re_num_nodes(graph));
    for ( i = re_num_nodes(graph); i-- > 0; ){
    retiming[i] = r_vec[i];
    }
    dup_network = network_dup(network);
    n_shift = make_retiming_negative(graph, retiming);
    assert(n_shift >= 0);
    input_order = st_init_table(st_ptrcmp, st_ptrhash);
    retime_get_input_order(dup_network, graph, input_order, /* node_t's */ 0);
    reset_ancestor = st_init_table(st_ptrcmp, st_ptrhash);
    latch_table = retime_get_latch_corresp(network, dup_network);
    input_seq = array_alloc(char *, 0);

    if (seqbdd_extract_input_sequence(dup_network, n_shift, input_order, input_seq, reset_ancestor)) {
    *can_init = FALSE;
    } else {
    if (retime_debug) {
        (void)fprintf(sisout, "RESET ANCESTOR CHANGES \n");
        st_foreach_item_int(reset_ancestor, stgen, (char **)&latch, &value){
        (void)fprintf(sisout, "%s %d -> %d\n",
            node_name(latch_get_output(latch)), value,
            latch_get_initial_value(latch));
        }
    }
    orig_reset_ancestor = retime_swap_latches(reset_ancestor, latch_table);
    set_initial_state(graph, orig_reset_ancestor);
    if (retime_debug){
        (void)fprintf(sisout, "ON INPUTS\n");
        for ( i = array_n(input_seq); i--> 0; ){
        input = array_fetch(char *, input_seq, i);
        for (j = 0; j < st_count(input_order); j++){
            (void)fprintf(sisout, "%d", (int)input[j]);
        }
        (void)fprintf(sisout, "\n");
        }
    }
    simulate_forward(graph, retiming, input_seq);
    st_free_table(orig_reset_ancestor);
    }

    /* cleanup */
    st_free_table(input_order);
    st_free_table(latch_table);
    st_free_table(reset_ancestor);
    for (i = 0; i < array_n(input_seq); i++) {
    input = array_fetch(char *, input_seq, i);
    FREE(input);
    }
    FREE(retiming);
    array_free(input_seq);
    network_free(dup_network);

    return (*can_init);
}


 /* INTERNAL INTERFACE */


/*
 * Expects that initially the PIPO nodes have a retiming value of 0. This is checked.
 * computes n_shift = MAX_i(retiming[i]) and subtract this value to all entries in 'retiming'.
 */

static int
make_retiming_negative(graph, retiming)
re_graph *graph;	    /* graph representation of network for retiming */
int *retiming;		    /* solution vector */
{
    int i;
    int n_shift;		       /* = max(r(v)-r(host)) */
    int n_nodes = re_num_nodes(graph);
    int max_value;
    re_node *pi, *po;

    re_foreach_primary_input(graph, i, pi) {
    assert(retiming[pi->id] == 0);
    }
    re_foreach_primary_output(graph, i, po) {
    assert(retiming[po->id] == 0);
    }
    max_value = 0;
    for (i = n_nodes; i-- > 0; ) {
    if (max_value < retiming[i]) max_value = retiming[i];
    }
    n_shift = max_value;
    for (i = n_nodes; i-- > 0; )
      retiming[i] -= n_shift;

    return n_shift;
}


/*
 * stores in 'input_order' a map between external PI of 'network'
 * and the ordering of primary inputs in graph.
 * If 'use_re_nodes_flag' is set, stores the correspondance between
 * the external PI in 'graph' rather than external PI in 'network'.
 * Just to make sure the two are consistent.
 */

static void
retime_get_input_order(network, graph, input_order, use_re_node_flag)
network_t *network;
re_graph *graph;
st_table *input_order;
int use_re_node_flag; /* if 1, map re_node to integers; if 0, map node_t to integers */
{
    int i;
    re_node *node;
    node_t *old_pi, *pi;

    re_foreach_primary_input(graph, i, node) {
    if (use_re_node_flag) {
        (void) st_insert(input_order, (char *) node, (char *) i);
    } else {
        old_pi = node->node;
        pi = network_find_node(network, node_long_name(old_pi));
        assert(pi != NIL(node_t));
        assert(pi->type == PRIMARY_INPUT && network_is_real_pi(network, pi));
        (void) st_insert(input_order, (char *) pi, (char *) i);
    }
    }
}


/*
 * Copies the initial values from the latches into the initial_states field.
 * It is an error if the value is not specified.
 * NOTE: It frees the latch entries associated with the edges.
 */

static void
set_initial_state(graph, latch_values)
re_graph *graph;	    /* graph representation of network for retiming */
st_table *latch_values;
{
    re_edge *edge;
    latch_t *latch;
    int edge_index, i, init_val;

    re_foreach_edge(graph, edge_index, edge) {
    if (re_ignore_edge(edge)) continue;
    if (edge->weight == 0) continue;
    assert(edge->weight > 0);
    if (edge->num_val_alloc > 0) FREE(edge->initial_values);
    edge->initial_values = ALLOC(int, edge->weight);
    edge->num_val_alloc = edge->weight;
    for (i = 0; i < edge->weight; i++) {
        latch = edge->latches[i];
        assert(st_lookup_int(latch_values, (char *) latch, &init_val));
        edge->initial_values[i] = init_val;
    }
    FREE(edge->latches);
    edge->latches = NIL(latch_t *);	/* For garbage collection routines */
    }
}


/*
 * The heart of the algorithm.
 * Uses a stack. Pushes on the stack all the nodes that are ready to be retimed.
 * A node is ready to be retimed if it still needs to be retimed and it has latches on all its inputs.
 * A node still needs to be retimed if retiming[node_id] < 0.
 * We need to be careful not to put the same node twice on the stack, or we will get stack overflow.
 * For that,we use the special array 'on_stack' that checks for this condition.
 * After a node has fired, it may enable its outputs: we check to see if any output needs to be
 * put on the stack.
 * The algorithm completes when the stack is empty. For consistency, we check that the retiming
 * is complete then (all entries of 'retiming' are 0).
 */

static void
simulate_forward(graph, retiming, input_seq)
re_graph *graph;	    /* graph representation of network for retiming */
int *retiming;		    /* solution vector (integer version) */
array_t *input_seq;	    /* array of char* pointing to input sequences from ancestor to reset */
{
    int i, input_position, local_shift;
    re_node *node, *output;
    re_edge *outedge;
    int n_nodes;
    re_stack_t *stack;
    st_table *pi_order;		       /* correspondance between 'graph' PI and indices in 'input_seq' */

    pi_order = st_init_table(st_numcmp, st_numhash);
    retime_get_input_order(NIL(network_t), graph, pi_order, /* re_node */ 1);
    stack = re_stack_init(graph);

    n_nodes = re_num_nodes(graph);
    for (i = 0; i < n_nodes; i++) {
    node = array_fetch(re_node *, graph->nodes, i);
    assert(node->id == i);
    if (retiming[i] < 0) re_stack_push(stack, node);
    }


    while (! re_stack_is_empty(stack)) {
    /* retime the node at the top of the stack */
    node = re_stack_pop(stack);
    local_shift = MIN(min_n_input_latch(node), - retiming[node->id]);
    assert(local_shift > 0);
    switch (node->type) {
      case RE_PRIMARY_INPUT:
        assert(array_n(input_seq) == local_shift);
        assert(st_lookup_int(pi_order, (char *) node, &input_position));
        retime_primary_input_forward(node, input_position, input_seq);
        break;
      case RE_PRIMARY_OUTPUT:
        retime_primary_output_forward(node, local_shift);
        break;
      case RE_INTERNAL:
        retime_node_forward(node, local_shift);
        break;
        default:
        fail("node of type RE_IGNORE encountered");
    }
    retiming[node->id] += local_shift;

    /*
     * propagate readiness to node outputs
     */
    re_foreach_fanout(node, i, outedge) {
        if (re_ignore_edge(outedge)) continue;
        output = outedge->sink;
        if (retiming[output->id] < 0) re_stack_push(stack, output);
    }
    }

    /* check that the work has been done and cleanup */
    for (i = 0; i < n_nodes; i++) {
    assert(retiming[i] == 0);
    }
    re_stack_free(stack);
    st_free_table(pi_order);
}


/*
 * Stack manipulation routines for retiming
 */

static re_stack_t *
re_stack_init(graph)
re_graph *graph;
{
    int i;
    int n_nodes;
    re_stack_t *stack;

    stack = ALLOC(re_stack_t, 1);
    n_nodes = re_num_nodes(graph);
    stack->bottom = ALLOC(re_node *, n_nodes);
    stack->top = stack->bottom + n_nodes;
    stack->current = stack->bottom;
    stack->in_stack = ALLOC(int, n_nodes);
    for (i = 0; i < n_nodes; i++) {
    stack->in_stack[i] = 0;
    }
    return stack;
}

static void
re_stack_free(stack)
re_stack_t *stack;
{
    FREE(stack->bottom);
    FREE(stack->in_stack);
    FREE(stack);
}

/*
 * Does not always push. Checks whether the node needs to be pushed on the stack or not.
 * Supposes that the node needs retiming. This should be checked by the caller.
 */

static void
re_stack_push(stack, node)
re_stack_t *stack;
re_node *node;
{
    int n_input_latches;

    if (stack->in_stack[node->id]) return;
    n_input_latches = min_n_input_latch(node);
    if (n_input_latches <= 0) return;
    assert(stack->current < stack->top);
    *(stack->current++) = node;
    stack->in_stack[node->id] = 1;
}

static re_node *
re_stack_pop(stack)
re_stack_t *stack;
{
    re_node *node;

    assert(stack->current > stack->bottom);
    node = *(--stack->current);
    stack->in_stack[node->id] = 0;
    return node;
}

static int
re_stack_is_empty(stack)
re_stack_t *stack;
{
    return (stack->current == stack->bottom);
}


 /* yes iff currently a latch is present on each fanin edge */

static int
min_n_input_latch(node)
re_node *node;
{
    int i;
    re_edge *backedge;
    int min_weight = (int) INFINITY;

    re_foreach_fanin(node, i, backedge) {
    if (re_ignore_edge(backedge)) continue;
    if (min_weight > backedge->weight)
      min_weight = backedge->weight;
    }
    return min_weight;
}


/*
 * array_t *input_seq
 * char *one_seq = array_fetch(char *, input_seq, i)
 * EXPECTS "one_seq" TO BE ORDERED IN THE SAME ORDER AS
 * PRIMARY INPUTS OF GRAPH (guaranteed by 'retime_get_input_order')
 */

static void
retime_primary_input_forward(node, input_index, input_seq)
re_node *node;		    /* node being retimed */
int input_index;
array_t *input_seq;
{
    int i, j;
    re_edge *edge;
    int n_shift = array_n(input_seq);

    if (n_shift == 0) return;
    re_foreach_fanout(node, i, edge) {
    if (re_ignore_edge(edge)) continue;
    realloc_initial_values(edge, n_shift);
    }
    re_foreach_fanout(node, j, edge) {
    if (re_ignore_edge(edge)) continue;
    for (i = 0; i < n_shift; i++) {
        char *input = array_fetch(char *, input_seq, i);
        edge->initial_values[i] = (int)(input[input_index]);
    }
    }
}

/*
 * ASSERTION: the body of the loops are not called when new_size == 0.
 */

static void
realloc_initial_values(edge, size_incr)
re_edge *edge;
int size_incr;
{
    int i;
    int old_size, new_size;
    int *new_init_values;

    if (size_incr == 0) return;
    old_size = edge->weight;
    new_size = edge->weight + size_incr;

    assert(old_size >= 0 && new_size >= 0);
    new_init_values = (new_size > 0) ? ALLOC(int, new_size) : NIL(int);
    if (size_incr > 0) {
    for (i = 0; i < edge->weight; i++) {
        new_init_values[i + size_incr] = edge->initial_values[i];
    }
    } else {			       /* size_incr < 0 */
    for (i = 0; i < edge->weight + size_incr; i++) {
      new_init_values[i] = edge->initial_values[i];
      }
    }
    if (old_size > 0) FREE(edge->initial_values);
    edge->initial_values = new_init_values;
    edge->weight = edge->num_val_alloc = new_size;
}


static void
retime_primary_output_forward(node, n_shift)
re_node *node;		    /* node being retimed */
int n_shift;
{
    int i;
    re_edge *edge;

    re_foreach_fanin(node, i, edge) {
    if (re_ignore_edge(edge)) continue;
    realloc_initial_values(edge, 0 - n_shift);
    }
}


static void
retime_node_forward(node, n_shift)
re_node *node;		    /* node being retimed */
int n_shift;
{
    int i, j;
    re_edge *edge;
    int n_inputs = array_n(node->fanins);
    int **input_values = ALLOC(int *, n_shift);
    int *output_values = ALLOC(int, n_shift);

    for (i = 0; i < n_shift; i++) {
    input_values[i] = ALLOC(int, n_inputs);
    }

    for (i = 0; i < n_shift; i++) {
    re_foreach_fanin(node, j, edge) {
        if (re_ignore_edge(edge)) continue;
        input_values[i][edge->sink_fanin_id] = edge->initial_values[i + edge->weight - n_shift];
    }
    }
    re_foreach_fanin(node, j, edge) {
    if (re_ignore_edge(edge)) continue;
    realloc_initial_values(edge, 0 - n_shift);
    }
    re_foreach_fanout(node, j, edge) {
    if (re_ignore_edge(edge)) continue;
    realloc_initial_values(edge, 0 + n_shift);
    }

    for (i = 0; i < n_shift; i++) {
    output_values[i] = simulate_node_function(node->node, input_values[i]);
    }

    for (i = 0; i < n_shift; i++) {
    re_foreach_fanout(node, j, edge) {
        if (re_ignore_edge(edge)) continue;
        edge->initial_values[i] = output_values[i];
    }
    }

    for (i = 0; i < n_shift; i++) {
    FREE(input_values[i]);
    }
    FREE(input_values);
    FREE(output_values);
}

static int and_table[3][2] = {
    {0, 0}, /* first arg is 0 */
    {0, 1}, /* first arg is 1 */
    {0, 1}, /* first arg is 2 */
};

static int or_table[3][2] = {
    {0, 1}, /* first arg is 0 */
    {1, 1}, /* first arg is 1 */
    {0, 1}, /* first arg is 2 */
};


static int
simulate_node_function(node, in_values)
node_t *node;
int *in_values;
{
    int i, j, num_in;
    node_cube_t cube;
    int num_cubes = node_num_cube(node);
    int out_value = 2;
    node_function_t node_fn = node_function(node);

    if (node_fn == NODE_0) return 0;
    if (node_fn == NODE_1) return 1;
    num_in = node_num_fanin(node);
    for (i = 0; i < num_cubes; i++) {
    int local_out_value = 2;
    cube = node_get_cube(node, i);
    for (j = 0; j < num_in; j++) {
        int literal = node_get_literal(cube, j);
        switch (literal) {
          case ZERO:
        local_out_value = and_table[local_out_value][1 - in_values[j]];
        break;
          case ONE:
        local_out_value = and_table[local_out_value][in_values[j]];
        break;
          case TWO:
        break;
        default:
        fail("bad literal");
        /* NOTREACHED */
        }
        if (local_out_value == 0) break;
    }
    assert(local_out_value != 2);
    out_value = or_table[out_value][local_out_value];
    if (out_value == 1) break;
    }
    return out_value;
}
/*
 * Store corresponding latches in the two networks
 */
static st_table *
retime_get_latch_corresp(net1, net2)
network_t *net1, *net2;
{
    lsGen gen;
    node_t *po1, *po2;
    latch_t *l1, *l2;
    st_table *latch_table;

    latch_table = st_init_table(st_ptrcmp, st_ptrhash);

    foreach_latch(net1, gen, l1){
    po1 = latch_get_input(l1);
    assert((po2 = network_find_node(net2, node_long_name(po1))) != NIL(node_t));
    assert((l2 = latch_from_node(po2)) != NIL(latch_t));
    (void)st_insert(latch_table, (char *)l1, (char *)l2);
    }
    foreach_latch(net2, gen, l2){
    po2 = latch_get_input(l2);
    assert((po1 = network_find_node(net1, node_long_name(po2))) != NIL(node_t));
    assert((l1 = latch_from_node(po1)) != NIL(latch_t));
    (void)st_insert(latch_table, (char *)l2, (char *)l1);
    }

    return latch_table;
}

static st_table *
retime_swap_latches(ancestor, latch_table)
st_table *ancestor, *latch_table;
{
    int value;
    st_generator *stgen;
    latch_t *l1, *l2;
    st_table *orig_ancestor;

    orig_ancestor = st_init_table(st_ptrcmp, st_ptrhash);
    st_foreach_item_int(ancestor, stgen, (char **)&l1, &value){
    assert(st_lookup(latch_table, (char *)l1, (char **)&l2));
    st_insert(orig_ancestor, (char *)l2, (char *)value);
    }
    return orig_ancestor;
}
#endif /* SIS */
