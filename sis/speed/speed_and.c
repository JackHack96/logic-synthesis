
/************************************************
* Routine to decompose a node with one cube
* into an and tree based on the delay at its inputs 
**************************************************/
#include <stdio.h>
#include "sis.h"
#include "speed_int.h"

int
speed_and_decomp(network, node, speed_param, flag)
network_t *network;
node_t *node;
speed_global_t *speed_param;
int flag;             /* 1 => invert, 0 => no inversion */
{
    node_cube_t cube;
    node_t *new_node, *temp_node, *fanin, *speed_dec_node_cube();
    node_t *nlit, *mlit;
    double a_t, best_t, second_t; /* Arrival times of the inputs */
    int a_p, best_p = 1, second_p = 1;     /* Phase of the inputs */
    int i, n_lit = 0, first=0 , second=0 ; 
    node_literal_t literal;
    delay_time_t time, new_at;

    best_t = second_t = POS_LARGE;
    if (node_num_literal(node) == 0){
	if (flag){
	    new_node = node_not(node);
	    node_replace(node, new_node);
	}
	speed_single_level_update(node, speed_param, &new_at);
	return 1;
    }
    cube = node_get_cube(node, 0);

    /*
    if (speed_debug){
        (void) fprintf(sisout,"\t\tAND_DECOMPOSING node \t");
        node_print(sisout,node);
    }
    */

    /*
     * Select the two earliest signals
     */
    foreach_fanin(node, i, fanin){
	/*
        if (speed_debug ){
            (void) fprintf(sisout, "Node %s -- a_time = ", node_name(fanin));
        }
	*/
        literal = node_get_literal(cube, i);
        if ((literal == ONE) || (literal == ZERO)){
            n_lit++;
            speed_delay_arrival_time(fanin, speed_param, &time);
            a_t = MAX(time.rise, time.fall);
            a_p = (literal == ONE) ? 1 : 0;

	    /*
            if (speed_debug) (void) fprintf(sisout, "%5.2f\n", a_t);
	    */
            if (a_t < best_t){
                second = first; second_p = best_p; second_t = best_t;
                first = i; best_p = a_p; best_t = a_t;
            }
            else if (a_t < second_t){
                second = i; second_p = a_p; second_t = a_t;
            }
        }
    }

    if (n_lit > 2){
        /* Create a and node with the two inputs as the
        "first" and "second' variables */
        nlit = node_literal(node_get_fanin(node, first), best_p);
        mlit = node_literal(node_get_fanin(node, second), second_p);
        new_node = node_and(nlit, mlit);
        node_free(nlit); node_free(mlit);
        network_add_node(network, new_node);

	/*
	 * Update the arrival time for new_node 
	 */
	speed_single_level_update(new_node, speed_param, &new_at);

        /* Recursively decompose the node */
        if (node_substitute(node, new_node, 1) ){
	    if(!speed_and_decomp(network, node, speed_param, flag)){
		error_append("Failed in and_decomp ");
		fail(error_string());
	    }
        } else {
            error_append("substitute failed in speed_and_decomp\n");
            fail(error_string());
        }
    } else {
        if(flag ){
            /*
	     * Return the node after inverting it and 
	     * updating the arrival times. 
	     */
            if (n_lit > 1 ){
		/*
		 * Add an explicit inverter
		 */
                temp_node = node_dup(node);
                network_add_node(network, temp_node);
                nlit = node_literal(temp_node, 0);
                node_replace(node, nlit);
		speed_single_level_update(temp_node, speed_param, &new_at);
            }
            else {
                temp_node = node_not(node);
                node_replace(node, temp_node);
            }
        }
	speed_single_level_update(node, speed_param, &new_at);
    }

    return 1;
}
