
#ifdef SIS

#include "timing_int.h"

/* Buch of local global variables  */
static double Uset_up;
static double Uhold;
static double Umin_sep;
static double Umax_sep;
static int    algorithm_flag;
static int    phase_inv;

static int tmg_clock_compare();

int timing_network_check(network_t *network, delay_model_t model) {
    node_t      *gate, *clock_pi;
    sis_clock_t *clock;
    latch_t     *latch;
    lsGen       gen;

    if (network_num_internal(network) == 0 || network_num_latch(network) == 0) {
        (void) fprintf(sisout, "Serious error: no memory elements\n");
        return FALSE;
    }
    if (model == DELAY_MODEL_LIBRARY) {
        if (!lib_network_is_mapped(network)) {
            (void) fprintf(sisout, "Serious error: not all nodes are mapped\n");
            return FALSE;
        }
    }

    foreach_latch(network, gen, latch) {
        gate = latch_get_control(latch);
        if (gate == NIL(node_t)) {
            (void) fprintf(sisout, "Serious error: latch without clock \n");
            return FALSE;
        }
        clock_pi = node_get_fanin(gate, 0);
        if (node_function(clock_pi) != NODE_PI) {
            (void) fprintf(sisout, "Serious error: logic preceeding latch\n");
            return FALSE;
        }
        clock = clock_get_by_name(clock_pi->network, node_long_name(clock_pi));
        if (clock == NIL(sis_clock_t)) {
            (void) fprintf(sisout, "Serious error: unable to locate clock\n");
            (void) fprintf(sisout, "to latch \n");
            return FALSE;
        }
    }
    return TRUE;
}

/* function definition
    name:     tmg_alloc_graph()
    args:     algorithm (OPTIMAL_CLOCK or CLOCK_VERIFY), network
    job:      allocate data structures
    return value: l_graph_t *
    calls: tmg_determine_clock_order()
*/
l_graph_t *tmg_alloc_graph(algorithm_type_t algorithm, network_t *network) {
    l_graph_t *g;

    g = ALLOC(l_graph_t, 1);
    g->clock_order = tmg_determine_clock_order(network);
    g->host        = NIL(vertex_t);
    return g;
}

/* function definition
    name:     tmg_alloc_node()
    args:     -
    job:      allocate structure to store with node->user_data in
              the latch_graph
    return value: (l_node_t *)
    calls:    -
*/
l_node_t *tmg_alloc_node(algorithm_type_t algorithm) {
    l_node_t    *node;
    copt_node_t *copt;
    cv_node_t   *cv;

    node = ALLOC(l_node_t, 1);
    node->pio        = -1;
    node->num        = -1;
    node->latch_type = -1;
    node->latch      = NIL(latch_t);
    node->phase      = -1;
    node->copt       = NIL(copt_node_t);
    node->cv         = NIL(cv_node_t);
    if (algorithm == OPTIMAL_CLOCK) {
        copt = ALLOC(copt_node_t, 1);
        copt->w     = 0.0;
        copt->prevw = 0.0;
        copt->r     = 0;
        copt->dirty = FALSE;
        node->copt  = copt;
    } else if (algorithm == CLOCK_VERIFY) {
        cv = ALLOC(cv_node_t, 1);
        cv->A    = cv->a = cv->D = cv->d = 0.0;
        node->cv = cv;
    }
    return node;
}

/* function definition
    name:     tmg_alloc_edge()
    args:     -
    job:      allocate structure to store with edge->user_data in latch_graph
    return value: (l_edge_t *)
    calls:    -
*/

l_edge_t *tmg_alloc_edge() {
    l_edge_t *edge;

    edge = ALLOC(l_edge_t, 1);
    edge->Dmax = -INFTY;
    edge->Dmin = INFTY;
    edge->K    = -1;
    return edge;
}

/* function definition
    name:     tmg_get_latch_type
    args:     latch
    job:      finds if latch is a flip-flop or level sensitive
    return value: int (LS for level-sensitive and FF for flip-flop)
    calls:   -
*/
int tmg_get_latch_type(latch_t *latch) {
    switch (latch_get_type(latch)) {
        case FALLING_EDGE:return FFF;
        case RISING_EDGE:return FFR;
        case ACTIVE_HIGH:return LSH;
        case ACTIVE_LOW:return LSL;
        default:fprintf(sisout, "Warning latch type unknown\n");
            fprintf(sisout, "Assuming edge-triggered on falling edge\n");
            return FFF;
    }
}

/* function defipnition
    name:     g_get_edge()
    args:     source, sink, &edge
    job:      check if edge is present
    return value: 1 if edge is present - edge returned in edge, else
                  0 if no edge - allocate new edge and returned in edge
    calls:    -
*/

int g_get_edge(vertex_t *from, vertex_t *to, register edge_t **edge) {
    lsList in_edges;
    lsGen  gen1;

    in_edges = g_get_in_edges(to);

    for (gen1 = lsStart(in_edges);
         lsNext(gen1, (lsGeneric *) edge, LS_NH) == LS_OK ||
         ((void) lsFinish(gen1), 0);) {
        if (from == g_e_source(*edge)) {
            lsFinish(gen1);
            return 1;
        }
    }
    *edge = g_add_edge(from, to);
    return FALSE;
}

/* function definition
    name:     tmg_determine_clock_order()
    args:     network
    job:      construct an array of clock signals according to the
              ordering (e1 <= e1 <= ...en)
    return value:  (array_t *)
    calls:
*/

array_t *tmg_determine_clock_order(network_t *network) {
    sis_clock_t  *clock;
    lsGen        gen;
    int          no_sort;
    array_t      *clock_order;
    clock_edge_t transition;
    int          tmg_clock_compare(), i;
    double       e1;

    clock_order = array_alloc(sis_clock_t *, 0);
    no_sort     = FALSE;
    foreach_clock(network, gen, clock) {
        array_insert_last(sis_clock_t *, clock_order, clock);
        transition.clock      = clock;
        transition.transition = FALL_TRANSITION;
        e1 = clock_get_parameter(transition, CLOCK_NOMINAL_POSITION);
        if (e1 == CLOCK_NOT_SET) { /* clock edges not set!! */
            (void) fprintf(sisout, "Warning clock phases not set\n");
            (void) fprintf(sisout, "Arbitrary selection\n");
            no_sort = TRUE;
        }
    }

    /* Compute the clock ordering by sorting the clock edges-
       tmg_clock_compare() */
    /* --------------- */

    if (!no_sort) {
        array_sort(clock_order, tmg_clock_compare);
    }
    if (debug_type == ALL || debug_type == LGRAPH) {
        for (i = 0; i < array_n(clock_order); i++) {
            clock = array_fetch(sis_clock_t *, clock_order, i);
            (void) fprintf(sisout, "%s\n", clock_name(clock));
        }
    }
    return clock_order;
}

/* function definition
    name:     tmg_latch_get_clock()
    args:     latch_t *latch
    job:      returns the clock signal on the gate to trhe latch
    return value:    sis_clock_t *
    calls: -
*/
sis_clock_t *tmg_latch_get_clock(latch_t *latch) {
    node_t      *gate, *clock_pi;
    sis_clock_t *clock;

    gate     = latch_get_control(latch);
    clock_pi = node_get_fanin(gate, 0);
    clock    = clock_get_by_name(clock_pi->network, node_long_name(clock_pi));
    return clock;
}

/* Sorts the clock phases */
/* function definition
    name:     tmg_clock_compare()
    args:     sis_clock_t * clock1,*clock2
    job:      used in array_sort to sort the clock signals according
              to falling edges
    return value: int
    calls:
*/
static int tmg_clock_compare(char **k1, char **k2) {
    sis_clock_t  *clock1, *clock2;
    clock_edge_t transition;
    double       e1, e2;

    clock1 = (sis_clock_t *) (*k1);
    clock2 = (sis_clock_t *) (*k2);
    transition.clock      = clock1;
    transition.transition = FALL_TRANSITION;
    e1 = clock_get_parameter(transition, CLOCK_NOMINAL_POSITION);

    transition.clock      = clock2;
    transition.transition = FALL_TRANSITION;
    e2 = clock_get_parameter(transition, CLOCK_NOMINAL_POSITION);
    if (e1 - e2 < 0) {
        return -1;
    } else if (e1 - e2 > 0) {
        return 1;
    } else {
        return 0;
    }
}

/* function definition
    name:     tmg_latch_get_phase()
    args:     latch_t *l, graph_t *g
    job:      returns an int from 1, 2, ... num_phases giving the phase of
   control signal return value: int calls:
*/
int tmg_latch_get_phase(latch_t *l, array_t *a) {
    int i;

    for (i = 0; i < array_n(a); i++) {
        if (array_fetch(sis_clock_t *, a, i) == tmg_latch_get_clock(l))
            return (i + 1);
    }

    /* NOT REACHED */
    (void) fprintf(sisout, "Serious error: latch without clock signal!\n");
    return NOT_SET;
}

tmg_set_set_up(double s) { Uset_up = s; }

tmg_set_hold(double s) { Uhold = s; }

double tmg_get_set_up(vertex_t *v) {
    lib_gate_t   *gate;
    latch_time_t **lt;

    if ((gate = latch_get_gate(NLAT(v))) != NIL(lib_gate_t)) {
        lt = lib_gate_latch_time(gate);
        return lt[0]->setup;
    }
    return Uset_up;
}

double tmg_get_hold(vertex_t *v) {
    lib_gate_t   *gate;
    latch_time_t **lt;

    if ((gate = latch_get_gate(NLAT(v))) != NIL(lib_gate_t)) {
        lt = lib_gate_latch_time(gate);
        return lt[0]->hold;
    }

    return Uhold;
}

tmg_set_min_sep(double s) { Umin_sep = s; }

tmg_set_max_sep(double s) { Umax_sep = s; }

double 
tmg_get_min_sep (void) { return Umin_sep; }

double 
tmg_get_max_sep (void) { return Umax_sep; }

/* function definition
    name:     tmg_alloc_cedge()
    args:      -
    job:      allocates memory for an edge in the constraint graph
    return value: c_edge_t *
    calls:
*/
c_edge_t *tmg_alloc_cedge() {
    c_edge_t *edge;

    edge = ALLOC(c_edge_t, 1);
    edge->c_1        = st_init_table(st_numcmp, st_numhash);
    edge->c_2        = NIL(double);
    edge->evaluate_c = NOT_SET;
    edge->fixed_w    = INFTY;
    edge->ignore     = FALSE;
    edge->w          = INFTY;
    edge->duty       = 2;
    return edge;
}

static void tmg_cg_free_g(gGeneric c) {
    c_graph_t *g;
    g = (c_graph_t *) c;

    FREE(g->phase_list);
    FREE(g);
}

static void tmg_cg_free_v(gGeneric c) {
    c_node_t *n;

    n = (c_node_t *) c;
    FREE(n);
}

static void tmg_cg_free_e(gGeneric c) {
    c_edge_t     *e;
    st_generator *sgen;
    char         *dummy;
    double       *value;

    e = (c_edge_t *) c;

    st_foreach_item(e->c_1, sgen, (char **) &dummy, (char **) &value) {
        FREE(value);
    }
    st_free_table(e->c_1);
    if (e->c_2 != NIL(double)) {
        FREE(e->c_2);
    }
    FREE(e);
}

/* function definition
    name:     tmg_constraint_graph_free()
    args:     graph *g
    job:      frees memory associated with the clock graph
    return value: -
    calls: tmg_cg_free_g(), tmg_cg_free_v(), tmg_cg_free_e()
*/
void tmg_constraint_graph_free(graph_t *g) {
    int     i;
    phase_t *p;

    for (i = 0; i < NUM_PHASE(g); i++) {
        p = PHASE_LIST(g)[i];
        FREE(p);
    }
    g_free(g, tmg_cg_free_g, tmg_cg_free_v, tmg_cg_free_e);
}

int tmg_set_gen_algorithm_flag(int value) { algorithm_flag = value; }

int 
tmg_get_gen_algorithm_flag (void) { return algorithm_flag; }

tmg_set_phase_inv(int flag) { phase_inv = flag; }

int 
tmg_get_phase_inv (void) { return phase_inv; }

c_graph_t *tmg_alloc_cgraph() {
    c_graph_t *c;

    c = ALLOC(c_graph_t, 1);
    return c;
}

void tmg_alloc_phase_vertices(graph_t *cg, graph_t *lg) {
    phase_t **list;
    int     i;

    list   = ALLOC(phase_t *, array_n(CL(lg)));
    for (i = 0; i < array_n(CL(lg)); i++) {
        list[i] = ALLOC(phase_t, 1);
        list[i]->rise            = g_add_vertex(cg);
        list[i]->rise->user_data = (gGeneric) ALLOC(c_node_t, 1);
        list[i]->fall            = g_add_vertex(cg);
        list[i]->fall->user_data = (gGeneric) ALLOC(c_node_t, 1);
        PID(list[i]->rise) = i + 1;
        PID(list[i]->fall) = -(i + 1);
    }
    PHASE_LIST(cg) = list;
    NUM_PHASE(cg)  = array_n(CL(lg));
}

/* function definition
    name:     f_free_g(), f_free_v(), f_free_e()
    args:     graph,       vertex,      edge
    job:      free associated data in ->user_data
    return value: -
    calls: -
*/
static void tmg_lg_free_g(gGeneric c) {
    l_graph_t *g;

    g = (l_graph_t *) c;
    array_free(g->clock_order);
    FREE(g);
}

static void tmg_lg_free_v(gGeneric c) {
    l_node_t *n;
    n = (l_node_t *) c;
    if (n->copt != NIL(copt_node_t)) {
        FREE(n->copt);
    }
    if (n->cv != NIL(cv_node_t)) {
        FREE(n->cv);
    }
    FREE(n);
}

static void tmg_lg_free_e(gGeneric c) {
    l_edge_t *e;
    e = (l_edge_t *) c;
    FREE(e);
}

void tmg_free_graph_structure(graph_t *latch_graph) {
    g_free(latch_graph, tmg_lg_free_g, tmg_lg_free_v, tmg_lg_free_e);
}

double tmg_max_clock_skew(vertex_t *u) { return (double) (0); }

double tmg_min_clock_skew(vertex_t *u) { return (double) (0); }

#endif /* SIS */
