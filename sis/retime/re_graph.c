/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/retime/re_graph.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:48 $
 *
 */
#ifdef SIS
#include "sis.h"
#include "retime_int.h"

#define EPS 1.0e-9
#define FUDGE 1.0

/*
 * Routine that encapsulates the retiming procedures for the user
 * that wants to use the retiming algorithm on a graph
 */

int
retime_graph_interface(graph, area_r, delay_r, retime_tol, d_clk, new_c)
re_graph *graph;
double area_r, delay_r;
double retime_tol, d_clk, *new_c;
{
    int flag, can_init;

    flag = retime_graph(NIL(network_t), graph, area_r, delay_r, retime_tol, 
	    d_clk, new_c, 1, 0, 0, &can_init);
    return flag;
}

/*
 * Uses a binary search to determine the clock cycle that is feasible.
 * Will use either nanni's algorithm (default) or lieserson's algorithm
 * If the min_reg parameter == 1: minimize the register count as well
 *
 * The respective algorithms get a copy of the graph and the clock period to
 * attempt and they return a status (feasible or not) and the value of the
 * retiming vector. This is used to compute the initial states. The initial
 * states are computed only if ----
 *      should_init == 1 && number of latches <= MAX_LATCH_TO_INITIALIZE
 *			OR if
 *	should_init == 2 (can be set by the -I option)
 */
int
retime_graph(network, graph, area_r, delay_r, retime_tol, 
	d_clk, new_c, lp_flag, min_reg, should_init, can_initialize)
network_t *network;
re_graph *graph;
double area_r, delay_r, retime_tol, d_clk, *new_c;
int lp_flag;	/* == 1 => use the lieserson, otherwise nanni */
int min_reg;	/* == 1 => minimize number of registers */
int should_init;/* == 1 => compute initial states, == 2 => at all cost */
int *can_initialize;/*  RETURN: can the network be initialized */
{
    int i, j, status, n;
    int no_err_flag, changed;
    int make_legal_check, make_legal;
    int *sol, *best_sol;	/* The solution to the retiming problem */
    double current_c, max_fail_c, best_c, initial_c, c, attempting_c;
    re_node *node;
    re_edge *edge;
    re_graph *new_graph;

    best_c = re_cycle_delay(graph, delay_r);
    sol = ALLOC(int, re_num_nodes(graph)*2); /* Big enuff vector */
    /*
     * First of all make the edge weights positive --- We could have started
     * with a graph with -ve weights due to R&R procedures
     */
    make_legal = FALSE;
    re_foreach_edge(graph, i, edge){
	if (edge->weight < 0) make_legal = TRUE;
    }
    if (make_legal){
	if (retime_debug > 40){
	    (void)fprintf(sisout,"Making the circuit legal\n");
	}
	status = retime_lies_routine(graph, best_c + FUDGE, sol);
	if (status == 0){
	    /*
	     * Infeasible solution --- should never happen if the negative
	     * weigths were obtained by a proper procedure
	     */
	    error_append("Cannot get rid of the negative latches\n");
	    return -1;
	}
	make_legal_check = FALSE;
	re_foreach_edge(graph, i, edge){
	    if (edge->weight < 0) make_legal_check = TRUE;
	}
	/* Final check --- just in case something got overlooked */
	assert(make_legal_check == FALSE);
    }

    /* Now we have a legal retiming graph -- try to optimize the delay */
    best_c = re_cycle_delay(graph, delay_r);
    initial_c = best_c;
    if (!min_reg && best_c <= d_clk) {
	/* Current network meets the constraints */
	*new_c = best_c;
	(void)fprintf(sisout,"Current network meets cycle time constraint\n");
	return TRUE;
    }

    max_fail_c = d_clk;
    best_sol = NIL(int);
    n = network_num_latch(network);

    /*
     * Now work to improve the graph by retiming
     */
    (void)fprintf(sisout,"initial cycle delay         = %-5.2f\n", best_c);
    (void)fprintf(sisout,"initial number of registers = %-d\n", n);
    (void)fprintf(sisout,"initial logic cost          = %-5.2f\n",
	    re_sum_node_area(graph));
    (void)fprintf(sisout,"initial register cost       = %-5.2f\n\n", n * area_r);

    attempting_c = d_clk;
    
loop: new_graph = re_graph_dup(graph);
    c = attempting_c - delay_r;
    if (min_reg){
	status = retime_min_register(new_graph, c, sol);
    } else if (lp_flag) {
	status = retime_lies_routine(new_graph, c, sol);
    } else {
	status = retime_nanni_routine(new_graph, c, sol);
    }

    if (status == 0) {
	/*
	 * The value of attempting_clk is infeasible
	 */
	if ((best_c - attempting_c) > retime_tol) {
	    max_fail_c = attempting_c;
	    (void)fprintf(sisout,"Failed at %-5.2f",attempting_c);
	    attempting_c  =  0.5 *(best_c  + attempting_c);
	    (void)fprintf(sisout," : Now attempting %-5.2f\n", attempting_c);
	    re_graph_free(new_graph);
	    goto loop;
	} else {
	    /* There is no feasible soln better than "best_c - TOL" */
	    (void)fprintf(sisout,"Quitting binary search at %-5.2f\n", attempting_c);
	    goto exit_after_init;
	}
    }

    /*
     * Success has been achieved -- accept the current soln 
     */

    if (!retime_check_graph(new_graph)){
	re_graph_free(new_graph);
	FREE(sol);
	return 0;
    }

    current_c = re_cycle_delay(new_graph, delay_r);
    (void)fprintf(sisout,"Success at %-5.2f, Delay is %-5.2f\n",
	    attempting_c, current_c);

    /*
     * Just a check to make sure that retiming computations were OK 
     */
    if (current_c > (attempting_c + EPS)){  /* Floating point comaparison */
	(void)fprintf(sisout,"SERIOUS INTERNAL ERROR: The cycle after retiming %4.1f exceeds initial %4.1f\n", current_c, attempting_c);
	re_graph_free(new_graph);
	*new_c = best_c;
	FREE(sol);
	return 0;
    }

    /* Store the best values of the retiming and the cycle time */
    best_c = current_c;
    if (best_sol == NIL(int)){
	best_sol = ALLOC(int, re_num_nodes(graph));
    }
    for (i = re_num_nodes(graph); i-- > 0; ){
	best_sol[i] = sol[i];
    }

    /*
     * Even thought we succedded -- are we close enuff to the desired clock
     */
    if (current_c > (d_clk + retime_tol)) {
	if (current_c > (max_fail_c + retime_tol)){
	    attempting_c = 0.5 *(max_fail_c + current_c);
	    (void)fprintf(sisout,"Success: Now attempting %-5.2f\n", attempting_c);
	    re_graph_free(new_graph);
	    goto loop;
	}
    }

    /* 
     * We have achieved the constraint specified or no
     * further improvement is in sight ---- 
     */

exit_after_init:
    /*
     * If there was a feasible retiming (best_sol != NIL(int) then
     * call the routine to update the initial values on the edges
     */
    *new_c = best_c;
    *can_initialize = TRUE;
    no_err_flag = TRUE; changed = FALSE;
    (void)fprintf(sisout, "\n");
    if ( best_sol != NIL(int)){
	if (should_init){
	    if (!min_reg)  {
		/* Timing should have improved */
		assert(best_c < initial_c); 
	    }
	    no_err_flag = retime_update_init_states(network, graph, best_sol,
		    should_init, can_initialize);
	}
	if (should_init == 0 || no_err_flag == 2) {
	    /*
	     * Either initial state computation was suppresed (-i option)
	     * or the number of latches was too many ....
	     * Just move the latches and set initial states to 3 (UNKNOWN)
	     */
	    no_err_flag = TRUE; *can_initialize = TRUE;
	    (void)fprintf(sisout, 
		    "NEW INITIAL STATE FOR ALL LATCHES IS 3 (UNKNOWN)\n");
	    re_foreach_node(graph, i, node){
		if (re_ignore_node(node)) continue;
		if (node->type == RE_INTERNAL){
		    retime_single_node(node, best_sol[i]);
		} else {
		    assert(best_sol[i] == 0);
		}
	    }
	    re_foreach_edge(graph, i, edge){
		if (re_ignore_edge(edge)) continue;
		if (edge->weight > 0){
		    if (edge->num_val_alloc > 0) FREE(edge->initial_values);
		    edge->initial_values = ALLOC(int, edge->weight);
		    edge->num_val_alloc = edge->weight;
		    for (j = edge->weight; j-- > 0; ){
			edge->initial_values[j] = 3 /* UNKNOWN */;
		    }
		}
	    }
	}
	for (i = re_num_nodes(graph); i-- > 0; ){
	    if (best_sol[i] != 0){
		changed = TRUE;
		break;
	    }
	}
	FREE(best_sol);
    }

    FREE(sol);
    re_graph_free(new_graph);

    if (!no_err_flag) return -1;
    if (!changed && !make_legal) {
	return FALSE;
    }
    return TRUE;
}

#undef EPS
#endif /* SIS */
