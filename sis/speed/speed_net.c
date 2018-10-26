
/*
 *
 * Decomposes a node of a network by selecting 
 * divisors and recursively decomposes them.
 *
 */

#include <stdio.h>
#include "sis.h"
#include "speed_int.h"

#define SCALE_2 100000
#define SCALE 100

#define CRITICAL_FRACTION 0.05
#define FUDGE 0.0001
/* Original value
#define CRITICAL_FRACTION 0.0001
*/

int kernel_timeout_occured = 0; /* Flag stating that enough time has expired */
static void speed_scale_value();
static int evaluate_kernel();

typedef struct divisor_data divisor_data_t;
struct divisor_data {
    double alpha;
    double cost;
    node_t *best;
    double min_delay;
    double max_delay;
    double delay_scale;
    double min_area;
    double max_area;
    double area_scale;
    };

typedef struct divisor_struct divisor_t;
struct divisor_struct {
    node_t *node;
    double area;
    double delay;
    };

struct a_kern_node{
    node_t *node;
    array_t *cost_array;
    double area_max;
    double area_min;
    double delay_max;
    double delay_min;
    double thresh;
    speed_global_t *globals;
    };


int 
speed_decomp_network(network, node, speed_param, attempt_no)
network_t *network;
node_t *node;
speed_global_t *speed_param;
int attempt_no;	/* selects different decompositions */
{
    int i, j;
    delay_time_t time;
    node_t *np, *temp, *best;
    double min_t, max_t, range;
    struct a_kern_node *sorted;
    divisor_t *div;
    divisor_data_t *best_div;

    if (speed_param->debug) {
	(void) fprintf(sisout,"   Decomposing node \n");
	node_print(sisout, node);
	(void)fprintf(sisout,"\tInput arrival times ");
	j = 0;
	foreach_fanin(node, i, np){
	    speed_delay_arrival_time(np, speed_param, &time);
	    if ((j % 3 == 0) && (j != node_num_fanin(node))) {
		(void) fprintf(sisout,"\n\t");
	    }
	    j++;
	    (void) fprintf(sisout,"%s at %-5.2f, ", node_name(np), time.rise);
	}
	(void)fprintf(sisout,"\n");
    }

    temp = NIL(node_t);
    if ((!kernel_timeout_occured) &&
	(temp = ex_find_divisor_quick(node)) != NIL(node_t) ) {
        /* Implies kernel exists */
        sorted = ALLOC(struct a_kern_node, 1);
        sorted->node = node;
	sorted->cost_array = array_alloc(divisor_t *, 0);
        sorted->area_max = NEG_LARGE;
        sorted->area_min = POS_LARGE;
        sorted->delay_max = NEG_LARGE;
        sorted->delay_min = POS_LARGE;


        /*
	 * Set the threshold to delete nodes with arrival times in the
	 * last .01% of the nodes-- (effectively the latest input) 
	 */
        max_t = NEG_LARGE;
        min_t = POS_LARGE;
        foreach_fanin(node, i, np){
            speed_delay_arrival_time(np, speed_param, &time);
            max_t = D_MAX(max_t, D_MAX(time.rise, time.fall));
            min_t = D_MIN(min_t, D_MIN(time.rise, time.fall));
        }
        if ((max_t - min_t) < 1.0e-3) {
            sorted->thresh = POS_LARGE;
        } else {
            sorted->thresh = 
		max_t - (attempt_no*CRITICAL_FRACTION + FUDGE) * (max_t-min_t);
        }
	sorted->globals = speed_param;

        /*
	 * Evaluate the area and delay components for the kernels and
	 * co-kernels, as well as the intersections 
	 */

        ex_kernel_gen(node, evaluate_kernel, (char *)sorted);
        ex_subkernel_gen(node, evaluate_kernel, 0,(char *)sorted);

	/* Now get the numbers back and scale the two components 
	 * Also select the best divisor from a timing standpoint
	 */
	best_div = ALLOC(struct divisor_data, 1);
        best_div->alpha = speed_param->coeff;
	best_div->best = NIL(node_t);
	best_div->cost = POS_LARGE;
	best_div->min_delay = sorted->delay_min;
	best_div->max_delay = sorted->delay_max;
	best_div->min_area = (double)(sorted->area_min);
	best_div->max_area = (double)(sorted->area_max);

	range = sorted->delay_max - sorted->delay_min;
	best_div->delay_scale = SCALE_2 / (range > 0 ? range : 1);
	range = (double)sorted->area_max - (double)sorted->area_min;
	best_div->area_scale = SCALE / (range > 0 ? range : 1);

	if (speed_param->debug) {
	    (void) fprintf(sisout,"\t    Evaluating the cost of divisors\n");
	}
	for (i = 0; i < array_n(sorted->cost_array); i++){
	    div = array_fetch(divisor_t *,sorted->cost_array, i);
	    speed_scale_value(div, best_div, speed_param);
	}

	array_free(sorted->cost_array);
	FREE(sorted);

        if (best_div->best != NIL(node_t)){
            best = node_dup(best_div->best);
            /* 
	     * Free the storage for the best divisor 
	     */
            node_free(best_div->best);
            FREE(best_div);

            network_add_node(network, best);
            if (speed_param->debug){
                (void) fprintf(sisout,"\tBest divisor is  ");
                node_print(sisout, best);
            }

            /*
	     * Recursively speedup the rem and quotient 
	     */
            if (!node_substitute(node, best, 0)){
                node_free(best);
                fail("Error decomposing node during speedup \n");
            }
	    /* 
	     * Decomposing the divisor
	     */
            if (!speed_decomp_network(network, best, speed_param, attempt_no)){
                error_append("Failed while decomposing kernel  ");
                fail(error_string());
            }

            /*
	     * The arrival time at the remaining node should
	     * be valid, since speed_and_or_decomp returns
	     * the node with a valid delay
	     */

	    if(!speed_decomp_network(network, node, speed_param, attempt_no)){
		error_append("Failed after kernel extraction");
		fail(error_string());
	    }
        } else {
        /* Do an and-or decomp */
            FREE(sorted);
	    FREE(best_div);
            if (speed_param->debug) {
                (void) fprintf(sisout,"\tNo divisor appropriate -- AND_OR decomp\n");
            }
            if(!speed_and_or_decomp(network, node, speed_param)){
                error_append("Failed to decomp if no non_crit kernel ");
                fail(error_string());
            }
	    if (!speed_param->add_inv){
		speed_delete_single_fanin_node(network, node);
	    }
	}
    } else {

        /* If no kernels then decompose the network into 
        2-input and and or gates according to the arrival times    */

	if (speed_param->debug) {
	    (void) fprintf(sisout,"\tNo divisors -- doing AND_OR decomp\n");
	}
        if(!speed_and_or_decomp(network, node, speed_param)){
            error_append("Failed to decomp if no kernel ");
            fail(error_string());
        }
	if (!speed_param->add_inv){
	    speed_delete_single_fanin_node(network, node);
	}
    }
    if (temp != NIL(node_t)) node_free(temp);
    return 1;
}

/*
 * Routine to evaluate the cost of a kernel 
 * Also keeps the best divisor and its cost.
 */

static int 
evaluate_kernel(kernel, cokernel, state)
node_t *kernel;
node_t *cokernel;
char *state;
{
    struct a_kern_node *store;
    node_t *new_cokernel, *rem;
    divisor_t *k_data, *c_data;    /* To hold the area/delay components */
    void node_evaluate();
    double k_t_cost, c_t_cost;	   /* time cost of kern and co-kern */
    double k_a_cost, c_a_cost;;	   /* area cost of kern and co-kern */

    store = (struct a_kern_node *)state;
    
    if (kernel_timeout_occured) {
	/* Stop the generationof more kernel-cokernel pairs */
	node_free(kernel);
	node_free(cokernel);
	return 0;
    }
    /*
     * Make the kernel free of any cubes 
     * containing critical signals 
     */
    if (store->globals->del_crit_cubes){
	speed_del_critical_cubes(kernel, store->globals, store->thresh);
    }

    if (node_function(kernel) == NODE_0){
        /* Don't consider this kernel pair */
        node_free(kernel);
        node_free(cokernel);
        return 1;
    }

    /*
     * Generate the cokernel, reduce it to non crit signals 
     * and -- (as large as possible) and evaluate the cost 
     */

    new_cokernel = node_div(store->node, kernel, &rem);
    /*
     * If the node itself is a kernel dont consider it as
     * a potential divisor 
     */
    if (node_function(new_cokernel) == NODE_1){
         node_free(new_cokernel);
         node_free(rem);
         node_free(kernel);
         node_free(cokernel);
         return 1;
    }

    if (store->globals->del_crit_cubes){
	speed_del_critical_cubes(new_cokernel, store->globals, store->thresh);
    }
    if (node_function(new_cokernel) != NODE_0){
        node_evaluate(kernel, new_cokernel, store->globals, &k_t_cost, &k_a_cost);
        node_evaluate(new_cokernel, kernel, store->globals, &c_t_cost, &c_a_cost);
    }
    else{
        node_evaluate(kernel, new_cokernel, store->globals, &k_t_cost, &k_a_cost);
        c_t_cost = c_a_cost = POS_LARGE;
    }

    k_data = ALLOC(divisor_t, 1);
    k_data->node = kernel;
    k_data->delay = k_t_cost;
    k_data->area = k_a_cost;

    c_data = ALLOC(divisor_t, 1);
    c_data->node = new_cokernel;
    c_data->delay = c_t_cost;
    c_data->area = c_a_cost;

    array_insert_last(divisor_t *, store->cost_array, k_data);
    array_insert_last(divisor_t *, store->cost_array, c_data);

    /* Keep the range of costs of relevant  divisors updated
     * so that the scaling of the componenets is easy
     */
    if ((k_t_cost < POS_LARGE) || (k_a_cost < POS_LARGE) ) {
	store->delay_max = MAX(store->delay_max, k_t_cost);
	store->delay_min = MIN(store->delay_min, k_t_cost);
	store->area_max = MAX(store->area_max, k_a_cost);
	store->area_min = MIN(store->area_min, k_a_cost);
    }
    if ((c_t_cost < POS_LARGE) || (c_a_cost < POS_LARGE) ) {
	store->delay_max = MAX(store->delay_max, c_t_cost);
	store->delay_min = MIN(store->delay_min, c_t_cost);
	store->area_max = MAX(store->area_max, c_a_cost);
	store->area_min = MIN(store->area_min, c_a_cost);
    }

    node_free(rem);
    node_free(cokernel);

    return 1;
}


/*
 * Routine to evaluate a node . Nodes with smaller cost
 * are the ones that are preferred for speedup.
 *
 * C1 = n_lit(k)+n_lit(ck)+n_cubes(ck);
 * C2 = n_lit(k).n_cubes(ck) + n_lit(ck).num_cubes(k)
 * Area_saving = C2 - C1
 */

void 
node_evaluate(node, co_node, speed_param, delay, area)
node_t *node, *co_node;
speed_global_t *speed_param;
double *delay, *area;
{
    node_t *fanin;
    delay_time_t atime;
    int i, lit_saving, input_count;
    double mintime, maxtime;

    /* INITIALIZATIONS */
    maxtime = NEG_LARGE;
    mintime = POS_LARGE;

    /* For the literals find the arrival times */
    input_count = node_num_fanin(node);
    foreach_fanin(node, i, fanin){
        speed_delay_arrival_time(fanin, speed_param, &atime);
        maxtime = D_MAX(atime.rise, maxtime);
        maxtime = D_MAX(atime.fall, maxtime);
        mintime = D_MIN(atime.rise, mintime);
        mintime = D_MIN(atime.fall, mintime);
    }

    /*
     * Evaluate the literal saving resulting from
     * extracting this node                         
     */
    if (node_function(co_node) != NODE_0){
        lit_saving = node_num_cube(node) * node_num_literal(co_node) +
        	node_num_cube(co_node) * node_num_literal(node);
        lit_saving -= (node_num_literal(node) + node_num_literal(co_node) +
        node_num_cube(co_node));
    } else{
	lit_saving = 0;
    }

    /*
     * There is no point in extracting a node with
     * one input, so give it a high cost          
     */

    if (input_count <= 1){
	*delay = POS_LARGE;
	*area = POS_LARGE;
    } else {
        *delay =  (0.9 * maxtime) + (0.1 * mintime);
	*area = (double)lit_saving;
    }
}

static void
speed_scale_value(div, best_data, speed_param)
divisor_t *div;
divisor_data_t *best_data;
speed_global_t *speed_param;
{
    double area, delay, cost;

    delay = div->delay;
    area = div->area;

    if ((delay < POS_LARGE) && (area < POS_LARGE) ) {
	/*
	 * Scale the two components -- If the delay range is 0
	 * might as well select the best area saving.
	 */
	if (best_data->min_delay == best_data->max_delay) {
	    cost = 0 - area;
	} else {
	    cost = (delay - best_data->min_delay)* best_data->delay_scale -
		    best_data->alpha * (area - best_data->min_area)* 
		    best_data->area_scale;
	}

	if (speed_param->debug) {
	    /* Print the kernel and its cost components */
	    (void)fprintf(sisout,"\t\tt = %-5.2f ,a = %-3d for  ", delay, (int)area);
	    node_print_rhs(sisout, div->node);
	    (void)fprintf(sisout,"\n");
	}

	/* Keep the best around */
	if (cost < best_data->cost) {
	    best_data->cost = cost;
	    node_free(best_data->best);
	    best_data->best = div->node;
	} else {
	    node_free(div->node);
	}
    } else {
    	node_free(div->node);
    }
    FREE(div);
}
