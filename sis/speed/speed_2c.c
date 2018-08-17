
/*
 * Based on the distribution of the arrival time information, evaluate the
 * 2-cube divisors and then extract the best ...
 */

#include "sis.h"
#include "../include/speed_int.h"

#define SCALE_2 100000
#define SCALE 100

#define CRITICAL_FRACTION 0.05
#define FUDGE 0.0001
/* Original value
#define CRITICAL_FRACTION 0.0001
*/

typedef struct sp_2c_kernel_info sp_2c_kernel_info_t;
struct sp_2c_kernel_info {
    node_t         *node;     /* Node which is being decomposed */
    int            use_cokernel;     /* If set, use the cokernel instead of 2c_div */
    double         d_cost;     /* The current value for the best divisor */
    double         a_saving;     /* Saving in literals resulting from extraction */
    double         thresh;       /* Separating the early from the late inputs */
    speed_global_t *globals; /* Reference to all the global information */
};

extern void node_evaluate();              /* Part of speed_net.c */
extern node_t *fx_2c_kernel_best();        /* Part of gen_2c_kernel.c */
static int sp_eval_2c_kernel();

int twocube_timeout_occured = 0; /* Flag describing whether timeout has occured */

int
speed_2c_decomp(network, node, speed_param, attempt_no)
        network_t *network;
        node_t *node;
        speed_global_t *speed_param;
        int attempt_no;    /* selects different decompositions */
{
    int                 i, j;
    delay_time_t        time;
    node_t              *np, *rem, *temp, *best;
    double              min_t, max_t;
    sp_2c_kernel_info_t *store;

    if (speed_param->debug) {
        (void) fprintf(sisout, "   Decomposing node \n");
        node_print(sisout, node);
        (void) fprintf(sisout, "\tInput arrival times ");
        j = 0;
        foreach_fanin(node, i, np)
        {
            speed_delay_arrival_time(np, speed_param, &time);
            if ((j % 3 == 0) && (j != node_num_fanin(node))) {
                (void) fprintf(sisout, "\n\t");
            }
            j++;
            (void) fprintf(sisout, "%s at %-5.2f, ", node_name(np), time.rise);
        }
        (void) fprintf(sisout, "\n");
    }


    if ((!twocube_timeout_occured) &&
        (temp = ex_find_divisor_quick(node)) != NIL(node_t)) {
        node_free(temp);

        /* Implies kernel exists: Now go ahead and generate them */
        store = ALLOC(sp_2c_kernel_info_t, 1);
        store->node         = node;
        store->d_cost       = POS_LARGE;
        store->a_saving     = NEG_LARGE;
        store->use_cokernel = -1;
        store->globals      = speed_param;

        /*
	 * Set the threshold to delete nodes with arrival times in the
	 * last .01% of the nodes-- (effectively the latest input) 
	 */
        max_t = NEG_LARGE;
        min_t = POS_LARGE;
        foreach_fanin(node, i, np)
        {
            speed_delay_arrival_time(np, speed_param, &time);
            max_t = D_MAX(max_t, D_MAX(time.rise, time.fall));
            min_t = D_MIN(min_t, D_MIN(time.rise, time.fall));
        }
        if ((max_t - min_t) < 1.0e-3) {
            store->thresh = POS_LARGE;
        } else {
            store->thresh =
                    max_t - (attempt_no * CRITICAL_FRACTION + FUDGE) * (max_t - min_t);
        }

        if ((best = fx_2c_kernel_best(node, sp_eval_2c_kernel, store)) !=
            NIL(node_t)) {
            /* Extract the best divisor */
            if (store->use_cokernel == 1) {
                /*
                 * Use the co-kernel corresponding to the best 2c_kernel
                 * Take care to see that the critical cubes are deleted
                 * from the divisor. Mirror the evaluation code !!!
                 */
                if (speed_param->del_crit_cubes) {
                    speed_del_critical_cubes(best, speed_param, store->thresh);
                }
                temp = node_div(store->node, best, &rem);
                assert(node_function(temp) != NODE_0);
                node_free(best);
                node_free(rem);
                best = temp;
            }
            if (speed_param->del_crit_cubes) {
                speed_del_critical_cubes(best, speed_param, store->thresh);
            }

            network_add_node(network, best);
            if (speed_param->debug) {
                (void) fprintf(sisout, "\tBest 2c-kernel is  ");
                node_print(sisout, best);
            }

            /* Substitute the divisor into the function */
            if (!node_substitute(node, best, 1)) {
                node_free(best);
                fail("Error substituting node during 2c_kernel decomp \n");
            }
            /*
             * Recursively decompose the divisor
             */
            if (!speed_2c_decomp(network, best, speed_param, attempt_no)) {
                error_append("Failed while decomposing extracted kernel");
                fail(error_string());
            }

            /*
	     * The arrival time at the remaining node should
	     * be valid, since speed_and_or_decomp returns
	     * the node with a valid delay
	     */

            if (!speed_2c_decomp(network, node, speed_param, attempt_no)) {
                error_append("Failed while decomposing extracted kernel");
                fail(error_string());
            }
        } else {
            /* No 2-cube divisor is appropriate */
            if (speed_param->debug) {
                (void) fprintf(sisout, "\tNo 2c-kernel appropriate -- AND_OR decomp\n");
            }
            if (!speed_and_or_decomp(network, node, speed_param)) {
                error_append("Failed AND_OR decomp of kernel-free function ");
                fail(error_string());
            }
            if (!speed_param->add_inv) {
                speed_delete_single_fanin_node(network, node);
            }
        }

        FREE(store);

    } else {
        /*
	 * If no relevant 2-cube divisor exits or the function of the
	 * is such that there is no point in looking for kernels the decomp
	 * into 2-input AND  gates according to the arrival times 
	 */

        if (speed_param->debug) {
            (void) fprintf(sisout, "\tNo 2-cube div -- doing AND_OR decomp\n");
        }
        if (!speed_and_or_decomp(network, node, speed_param)) {
            error_append("Failed to decomp 2-cube free node into AND gates");
            fail(error_string());
        }
        if (!speed_param->add_inv) {
            speed_delete_single_fanin_node(network, node);
        }
    }
    return 1;
}

/*
 * Routine to evaluate the cost of a kernel 
 * Also keeps the best divisor and its cost.
 */

static int
sp_eval_2c_kernel(ddivisor, node_2c, handlep, state)
        ddivisor_t *ddivisor;
        node_t *node_2c;
        lsHandle **handlep;
        char *state;
{
    sp_2c_kernel_info_t *store = (sp_2c_kernel_info_t *) state;

    node_t *new_cokernel, *rem;
    double k_t_cost, c_t_cost;       /* time cost of kern and co-kern */
    double k_a_cost, c_a_cost;;       /* area cost of kern and co-kern */

    if (twocube_timeout_occured) {
        /* Time is up !!! return 0  to stop generation */
        return 0;
    }
    if (store->globals->debug) {
        (void) fprintf(sisout, "2c_kernel ");
        node_print_rhs(sisout, node_2c);
    }
    /* Make the kernel free of any cubes containing critical signals */
    if (store->globals->del_crit_cubes) {
        speed_del_critical_cubes(node_2c, store->globals, store->thresh);
    }

    if (node_function(node_2c) == NODE_0 || node_num_fanin(node_2c) == 1) {
        /* Don't consider this kernel pair */
        return 1;
    }

    /*
     * Generate the cokernel, reduce it to non crit signals 
     * and -- (as large as possible) and evaluate the cost 
     */

    new_cokernel = node_div(store->node, node_2c, &rem);
    node_free(rem);
    /*
     * If the node itself is a kernel dont consider it as a potential divisor 
     */
    if (node_function(new_cokernel) == NODE_1) {
        node_free(new_cokernel);
        return 1;
    }
    if (store->globals->del_crit_cubes) {
        speed_del_critical_cubes(new_cokernel, store->globals, store->thresh);
    }

    if (node_function(new_cokernel) != NODE_0 && node_num_fanin(new_cokernel) > 1) {
        node_evaluate(node_2c, new_cokernel, store->globals,
                      &k_t_cost, &k_a_cost);
        node_evaluate(new_cokernel, node_2c, store->globals,
                      &c_t_cost, &c_a_cost);
    } else {
        node_evaluate(node_2c, new_cokernel, store->globals,
                      &k_t_cost, &k_a_cost);
        c_t_cost = POS_LARGE;
        c_a_cost = NEG_LARGE;
    }

    if (store->globals->debug) {
        (void) fprintf(sisout, "  RESULTS IN ");
        node_print_rhs(sisout, node_2c);
        (void) fprintf(sisout, " AND ");
        node_print_rhs(sisout, new_cokernel);
        (void) fprintf(sisout, "\n");
    }

    /* Keep best around: Ties broken by greater area_saving */
    if ((k_t_cost < store->d_cost) ||
        (k_t_cost == store->d_cost && k_a_cost > store->a_saving)) {
        *handlep = &(fx_get_div_handle(ddivisor));
        store->use_cokernel = 0;
        store->a_saving     = k_a_cost;
        store->d_cost       = k_t_cost;
    }
    /* Also check the co-kernels : Ideally these should have been generated
     * as a generator too, but this is good enuff !!!
     */
    if ((c_t_cost < store->d_cost) ||
        (c_t_cost == store->d_cost && c_a_cost > store->a_saving)) {
        *handlep = &(fx_get_div_handle(ddivisor));
        store->use_cokernel = 1;
        store->a_saving     = c_a_cost;
        store->d_cost       = c_t_cost;
    }

    node_free(new_cokernel);

    return 1;
}



