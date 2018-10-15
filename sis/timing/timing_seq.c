/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/timing/timing_seq.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:15:00 $
 *
 */
#ifdef SIS
#include "timing_int.h"
    
/* function definition 
    name:     tmg_node_get_delay()
    args:     node, delay_model
    job:      return the max and min delay through the node.
              HACK to store the max delay in one entry and min delay in 
	      the other.
    return value: delay_time_t
    calls:  tmg_map_get_delay()
*/ 
delay_time_t 
tmg_node_get_delay(node, model)
node_t *node;
delay_model_t model;
{
    delay_time_t delay;
    
    if (node_function(node) == NODE_PI || node_function(node) == NODE_PO) {
	delay.rise = 0;
	delay.fall = 0;
	return delay;
    }
    if (model == DELAY_MODEL_UNIT) {
	delay.rise = 1 + 0.2 * node_num_fanout(node);
	delay.fall = delay.rise;
    } else {
	delay = tmg_map_get_delay(node);
    }
    return delay;
}
    
/* function definition 
    name:     tmg_map_get_delay()
    args:     node
    job:      get the mapped delay for the gate
    return value: (delay_time_t)
    calls:   -
*/ 
delay_time_t
tmg_map_get_delay(node)
node_t *node;
{
    lib_gate_t *gate;
    delay_time_t t, **a_t, delay;
    double max_time, min_time;
    int i, j, n;
    
    gate = lib_gate_of(node);
    n = lib_gate_num_in(gate);
    a_t = ALLOC(delay_time_t *, n);
    for(i = 0; i < n; i++){
	a_t[i] = ALLOC(delay_time_t, 1);
    }
    delay.rise = -INFTY;

    delay.fall = INFTY;
    for(i = 0; i < n; i++){
	for(j = 0; j < n; j++){
	    a_t[j]->rise = a_t[j]->fall = -INFTY;
	}
	a_t[i]->rise = a_t[i]->fall = 0.0;
	
	t = delay_map_simulate(n, a_t, gate->delay_info, (double)0);
	max_time = MAX(t.rise, t.fall);
	min_time = MIN(t.rise, t.fall);
	
	delay.rise = MAX(max_time, delay.rise);
	delay.fall = MIN(min_time, delay.fall);
    }
    
    for(i = 0; i < n; i++){
	FREE(a_t[i]);
    }
    FREE(a_t);
    return delay;
}
#endif /* SIS */
