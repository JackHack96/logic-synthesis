/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/timing/timing_verify.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:15:00 $
 *
 */
#ifdef SIS
#include "timing_int.h"


/* function definition 
    name:     tmg_check_clocking_scheme()
    args:     graph, network
    job:      verify the clocking schedule
    return value: 1 on success, 0 on failiure
    calls:    tmg_get_clock_events()
*/ 

int
tmg_check_clocking_scheme(latch_graph, network)
graph_t *latch_graph;
network_t *network;
{
    clock_event_t *clock_event;
    int phase, phase1, converged, i, num_e, flag;
    double cycle, temp, max_arrival, min_arrival, shift;
    lsGen gen, gen1;
    edge_t *e;
    vertex_t *u, *w;
    
    cycle = clock_get_cycletime(network);
    if (cycle < 0) {
	(void)fprintf(sisout, "Clock cycle not set\n");
	(void)fprintf(sisout, "Serious error\n");
	return FALSE;
    }

    if ((clock_event = tmg_get_clock_events(latch_graph, network)) ==
	NIL(clock_event_t)) {
	(void)fprintf(sisout, "Serious error\n");
	return FALSE;
    }

    if (debug_type == VERIFY || debug_type == ALL) {
	(void)fprintf(sisout, "Clock events\n");
	for (i = 0; i < NUM_CLOCK(clock_event); i++) {
	    (void)fprintf(sisout, "Phase %d  ^ %.2lf  v %.2lf\n", 
			  i, RISE(clock_event)[i], FALL(clock_event)[i]);
	}
    }

    /* Initialize clock verification algorithm */
    /* --------------------------------------- */

    foreach_vertex (latch_graph, gen, u) {
	if (TYPE(u) == NNODE) {
	    ARR(u) = 0.0;
	    arr(u) = 0.0;
	    DEP(u) = 0.0;
	    dep(u) = 0.0;
	} else {
	    ARR(u) = -INFTY;
	    arr(u) = -INFTY;
	    DEP(u) = -INFTY;
	    dep(u) = -INFTY;
	}
    }

    num_e = lsLength(g_get_edges(latch_graph));

    /* Long path convergence */
    /* --------------------- */

    for (i = 0; i < num_e; i++) {
	converged = TRUE;
	foreach_vertex (latch_graph, gen, u) {
	    if (TYPE(u) == NNODE) {
		DEP(u) = 0.0;
		dep(u) = 0.0;
	    } else {
		phase = PHASE(u)-1;
		switch (LTYPE(u)) {
		case FFF:
		    DEP(u) = cycle;
		    dep(u) = cycle;
		    break;
		case FFR:
		    temp = RISE(clock_event)[phase] + 
		    SHIFT(clock_event)[phase];
		    DEP(u) = temp;
		    dep(u) = temp;
		    break;
		case LSH:
		    temp = RISE(clock_event)[phase] + 
		    SHIFT(clock_event)[phase];
		    if (ARR(u) < temp) {
			DEP(u) = temp;
		    } else {
			DEP(u) = ARR(u);
		    }
		    if (arr(u) < temp) {
			dep(u) = temp;
		    } else {
			dep(u) = arr(u);
		    }
		    break;
		case LSL:
		    temp = 0.0;
		    if (ARR(u) < temp) {
			DEP(u) = temp;
		    } else {
			DEP(u) = ARR(u);
		    }
		    if (arr(u) < temp) {
			dep(u) = temp;
		    } else {
			dep(u) = arr(u);
		    }
		    break;
		default: /* Should never be here */
		    break;
		}
	    }
	    if (debug_type == VERIFY || debug_type == ALL) {
		(void)fprintf(sisout, "D(%d) = %.2lf d(%d) = %.2lf\n", ID(u), 
			      DEP(u), ID(u), dep(u));
	    }
	}
	foreach_vertex (latch_graph, gen, u) {
	    if (TYPE(u) == NNODE) continue;
	    max_arrival = -INFTY;
	    min_arrival = INFTY;
	    phase = PHASE(u)-1;
	    if (debug_type == VERIFY || debug_type == ALL) {
		(void)fprintf(sisout, "A(%d) = MAX(", ID(u));
	    }
	    foreach_in_edge (u, gen1, e) {
		w = g_e_source(e);
		if (TYPE(w) != NNODE) {
		    phase1 = PHASE(w)-1;
		    shift = FALL(clock_event)[phase] - 
		    FALL(clock_event)[phase1] +	Ke(e) * cycle;
		} else {
		    shift = 0.0;
		}
		temp = De(e) + DEP(w) - shift;
		if (debug_type == VERIFY || debug_type == ALL) {
		    (void)fprintf(sisout, "%.2lf\t ", temp);
		}
		max_arrival = MAX(max_arrival, temp);
		temp = de(e) + dep(w) - shift;
		min_arrival = MIN(min_arrival, temp);
	    }
	    if (debug_type == VERIFY || debug_type == ALL) {
		(void)fprintf(sisout, ")\n");
	    }
	    assert(ARR(u) <= max_arrival); /* Sanity check */
	    assert(arr(u) <= min_arrival); /* Sanity check */
	    if (ARR(u) + EPS < max_arrival) {
		flag = TRUE;
		converged = FALSE;
		ARR(u) = max_arrival;
		if (LTYPE(u) == FFF || LTYPE(u) == LSH) {
		    if (ARR(u) > cycle - tmg_get_set_up(u) + EPS) {
			(void)fprintf(sisout, "ERROR: set-up violation at %d\n"
				      , ID(u));
			flag = FALSE;
		    }
		} else if (LTYPE(u) == FFR || LTYPE(u) == LSL) {
		    if (ARR(u) > RISE(clock_event)[phase] + 
			SHIFT(clock_event)[phase] - tmg_get_set_up(u) + EPS) {
			(void)fprintf(sisout, "ERROR: set-up violation at %d\n"
				      ,ID(u));
			flag = FALSE;
		    }
		}
		if (!flag) {
		    (void)fprintf(sisout, "Error: long path problem\n");
		    trace_erroneous_path(latch_graph, clock_event, cycle, u);
		    return FALSE; 
		}
	    }
	    if (arr(u) + EPS < min_arrival) {
		arr(u) = min_arrival;
		converged = FALSE;
	    }
	    if (i == 0) {
		if (LTYPE(u) == FFF || LTYPE(u) == LSH) {
		    if (arr(u) + EPS < tmg_get_hold(u)) {
			(void)fprintf(sisout, "ERROR: hold violation at %d\n",
				      ID(u)); 
			return FALSE;
		    }   
		} else if (LTYPE(u) == FFR || LTYPE(u) == LSL) {
		    if (arr(u) + EPS < RISE(clock_event)[phase] +
			SHIFT(clock_event)[phase] + tmg_get_hold(u)) {
			(void)fprintf(sisout, "ERROR: hold violation at %d\n",
				      ID(u)); 
			return FALSE;

		    }   
		} 
	    }
	}
	if (debug_type == VERIFY || debug_type == ALL) {
	    (void)fprintf(sisout, "A(%d) = %.2lf a(%d) = %.2lf\n", ID(u),
			      ARR(u), ID(u), arr(u));
	}
	if (converged) break;
    }
    if (!converged) {
	(void)fprintf(sisout, "Clock cycle too small (positive cycle)");
	(void)fprintf(sisout, "Convergence failed\n");
    }
    tmg_free_clock_event(clock_event);
    return converged;
}


trace_erroneous_path(latch_graph, clock_event, cycle, u)
graph_t *latch_graph;
clock_event_t *clock_event;
double cycle;
vertex_t *u;
{
    lsGen gen;
    vertex_t *v;
    double path;
    
    foreach_vertex (latch_graph, gen, v) {
	CV_DIRTY(v) = FALSE;
    }

    path = 0.0;
    (void)trace_recursive_path(latch_graph, clock_event, cycle, u, ARR(u),
			       &path);
}

int
trace_recursive_path(latch_graph, clock_event, cycle, u, A, path)
graph_t *latch_graph;
clock_event_t *clock_event;
double cycle;
vertex_t *u;
double A, *path;
{
    edge_t *edge;
    vertex_t *w;
    lsGen gen;
    int phase, phase1;
    double temp, new_A, shift;
    
    
    phase = PHASE(u) -1;
    
    if (TYPE(u) == NNODE) {
	if (ABS(A) < EPS) {
	    (void)fprintf(sisout, "HOST->\n");
	    (void)fprintf(sisout, "Start 0.00\n");
	    *path = 0.00;
	    return TRUE;
	} else {
	    return FALSE;
	}
    } else {
	switch(LTYPE(u)) {
	case FFF:
	    temp = cycle;
	    break;
	case FFR:
	    temp = RISE(clock_event)[phase] + SHIFT(clock_event)[phase];
	    break;
	case LSH:
	    temp = RISE(clock_event)[phase] + SHIFT(clock_event)[phase];
	    break;
	case LSL:
	    temp = 0.0;
	    break;
	default: /* Should never be here */
	    break;
	}
	if (ABS(A - temp) < EPS) {
	    (void)fprintf(sisout, "%d ->\n", ID(u));
	    (void)fprintf(sisout, "Start %.2lf\n", temp);
	    *path = temp;
	    return TRUE;
	}
    }

    if (CV_DIRTY(u) == TRUE) {
	return FALSE;
    }
    CV_DIRTY(u) = TRUE;

    foreach_in_edge (u, gen, edge) {
	w = g_e_source(edge);
	if (TYPE(w) != NNODE) {
	    phase1 = PHASE(w)-1;
	    shift = FALL(clock_event)[phase1] - 
	    FALL(clock_event)[phase] - Ke(edge) * cycle;
	} else {
	    shift = 0.0;
	}
	new_A = A - De(edge) - shift;
	if (trace_recursive_path(latch_graph, clock_event, cycle, w, new_A,
				 path)) {
	    *path += De(edge) + shift;
	    (void)fprintf(sisout, " %.2lf \t%d -> %d \t %.2lf\n", De(edge),
			  Ke(edge), ID(u), (*path));
	    return TRUE;
	}
    }
    CV_DIRTY(u) = FALSE;
    return FALSE;
}



    
/* function definition 
    name:     tmg_get_clock_events()
    args:     graph, network
    job:      make sure that the full clocking scheme is specified
              and fill up the clock_event data structure
    return value:   clock_event_t *
    calls:      -
*/ 

clock_event_t *
tmg_get_clock_events(latch_graph, network)
graph_t *latch_graph;
network_t *network;
{
    clock_event_t *clock_event;
    clock_edge_t transition;
    sis_clock_t *clock;
    double cycle;
    int i;
    
    cycle = clock_get_cycletime(network);
    clock_event = ALLOC(clock_event_t, 1);
    NUM_CLOCK(clock_event) = array_n(CL(latch_graph));
    RISE(clock_event) = ALLOC(double, NUM_CLOCK(clock_event));
    FALL(clock_event) = ALLOC(double, NUM_CLOCK(clock_event));
    SHIFT(clock_event) = ALLOC(double, NUM_CLOCK(clock_event));
    for (i = 0; i < NUM_CLOCK(clock_event); i++) {
	clock = array_fetch(sis_clock_t *, CL(latch_graph), i);
	transition.clock = clock;
	transition.transition = FALL_TRANSITION;
	FALL(clock_event)[i] = clock_get_parameter(transition, 
						   CLOCK_NOMINAL_POSITION);
	if (FALL(clock_event)[i] == CLOCK_NOT_SET) {
	    (void)fprintf(sisout, "Warning clock event not set\n");
	    (void)fprintf(sisout, "cannot verify utill specified!\n");
	    (void)fprintf(sisout, "aborting\n");
	    tmg_free_clock_event(clock_event);
	    return NIL(clock_event_t);
	}
	transition.transition = RISE_TRANSITION;
	RISE(clock_event)[i] = clock_get_parameter(transition, 
						   CLOCK_NOMINAL_POSITION);
	if (RISE(clock_event)[i] == CLOCK_NOT_SET) {
	    (void)fprintf(sisout, "Warning clock event not set\n");
	    (void)fprintf(sisout, "cannot verify utill specified!\n");
	    (void)fprintf(sisout, "aborting\n");
	    tmg_free_clock_event(clock_event);
	    return NIL(clock_event_t);
	}
	SHIFT(clock_event)[i] = cycle - FALL(clock_event)[i];
	if (SHIFT(clock_event)[i] < 0) {
          (void)fprintf(siserr, "Error: clock event outside of cycle time\n");
	  return NIL(clock_event_t);
  	}
    }
    return clock_event;
}



tmg_free_clock_event(clock_event)
clock_event_t *clock_event;
{
    FREE(RISE(clock_event));
    FREE(FALL(clock_event));
    FREE(SHIFT(clock_event));
    FREE(clock_event);
}
#endif /* SIS */
