/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/speed/sp_buffer.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:50 $
 *
 */
#include "sis.h"
#include "speed_int.h"
#include "buffer_int.h"

#define BUF_GET_PERFORMANCE(network,node,value,area) \
node = (required_times_set ? sp_minimum_slack((network), &dummy) :\
               delay_latest_output((network), &dummy)); \
value =  (required_times_set ? delay_slack_time((node)) :\
               delay_arrival_time((node))); \
area = sp_get_netw_area(network)

static delay_time_t large_req_time  = {POS_LARGE, POS_LARGE};
static void sp_already_buffered();
int sp_satisfy_max_load();


/*
 * exported routine --- needed to set some of the global variables
 * before calling the appropriate routine.
 * Returns 0 if network is not changed, 1 if changed
 */
int
buffer_network(network, limit, trace, thresh, single_pass, do_decomp)
network_t *network;
int limit;
int trace;
double thresh;
int single_pass;
int do_decomp;
{
    int changed;
    buf_global_t buf_param;

    (void)buffer_fill_options(&buf_param, 0, NIL(char *));
    buf_param.limit = limit;
    buf_param.trace = trace;
    buf_param.thresh = thresh;
    buf_param.single_pass = single_pass;
    buf_param.do_decomp = do_decomp;

    buf_set_buffers(network, 0, &buf_param);
            
    /* Annotate the delay data */
    if (!delay_trace(network, buf_param.model)){
	(void)fprintf(sisout,"%s\n", error_string());
	return 1;
    }
    
    changed = speed_buffer_network(network, &buf_param);
    buf_free_buffers(&buf_param);

    changed = (changed == 0 ? 0 : 1);
    return changed;
}


/*
 * Buffers the nodes in the network. For now it will select the node
 * on the critical path with the largest fanout -- buffer it -- recompute
 * the delays and keep going till it meets timing constraints or 
 * cannot improve....
 */
int
speed_buffer_network(network, buf_param)
network_t *network;
buf_global_t *buf_param;
{
    lsGen gen;
    int changed, changed_some, i, flag, required_times_set, loop_count;
    int loop_again;
    array_t *nodevec;
    st_table *buffered_nodes;
    double dummy, orig_area, area;
    node_t *node, *best, *last, *orig;
    delay_time_t prev, delay, initial, final;
    delay_model_t model = buf_param->model;

    changed = FALSE;
    loop_count = 1;

    (void)delay_trace(network, model);
    /*
     * The flag "requird_times_set" is used to figure out whether to stop
     * when timing constraints are met (if required times at po are specified)
     * or when there is no improvement in delay
     */
    required_times_set = sp_po_req_times_set(network);

    loop_again = TRUE;
    while (loop_again){		/* There is scope for improvement */
	loop_again = FALSE;
	changed_some = FALSE;
	buffered_nodes = st_init_table(st_ptrcmp, st_ptrhash);
	if (buf_param->trace) 
	    (void)fprintf(misout,"\nLOOP NUMBER %d\n---------------\n",
		    loop_count++);

	/* Initially set the gates to be unbuffered */
	foreach_node(network, gen, node){
	    BUFFER(node)->type  = NONE;
	}
	/* Initial circuit performance */
	BUF_GET_PERFORMANCE(network, orig, initial, orig_area);
	prev = initial;

	/*
	 * Starting from the output keep on buffering the nodes that lie
	 * on the critical path
	 */
	do {
	    flag = FALSE;
	    set_buf_thresh(network, buf_param);
	    nodevec = network_dfs_from_input(network);
	    for (i = 0; i < array_n(nodevec); i++){
		best = array_fetch(node_t *, nodevec, i);
		if (best->type != PRIMARY_OUTPUT && buf_critical(best, buf_param)
		    && (!st_is_member(buffered_nodes, (char *)best)) ){
		    if (speed_buffer_node(network, best, buf_param, required_times_set)){
			flag = TRUE;
			changed = TRUE;
			changed_some = TRUE;
			sp_already_buffered(buffered_nodes, best);
			(void)delay_trace(network, model);
			set_buf_thresh(network, buf_param);

			/* 
			 * We should have gotton an improvement in ckt perf */
			BUF_GET_PERFORMANCE(network, last, delay, area);
			if (buf_param->trace){
 			    (void)fprintf(misout,
				"\t%s %.2f -> %.2f Area %.0f -> %.0f CRIT_PO %s -> %s\n",
				(required_times_set ? "Slack" : "Delay"),
				(required_times_set ? BUF_MIN_D(prev) :
						      BUF_MAX_D(prev)),
				(required_times_set ? BUF_MIN_D(delay) :
						      BUF_MAX_D(delay)),
				orig_area, area,
				orig->name, last->name);
			}
			if ((required_times_set && (BUF_MIN_D(delay) <
						    BUF_MIN_D(prev))) ||
			    (!required_times_set && (BUF_MAX_D(delay) >
						     BUF_MAX_D(prev))) ) {
			    (void)fprintf(sisout, "FAILED CHECK: Performance worsened when buffering %s\n", best->name);
			}
			prev = delay;
			orig = last;
			orig_area = area;
			break;
		    }
		}
	    }
	    array_free(nodevec);
	} while (flag);

	/* Do an area recovery pass as well::: This is not really correct
	   so bypass it for the time being till a correct recovery
	   procedure is found  */

	if (speed_buffer_recover_area(network, buf_param)){
	    BUF_GET_PERFORMANCE(network, last, delay, area);
	    if (buf_param->trace){
		(void)fprintf(misout,
			      "\t%s %.2f -> %.2f Area %.0f -> %.0f after Area Recovery\n",
			      (required_times_set ? "Slack" : "Delay"),
			      (required_times_set ? BUF_MIN_D(prev) :
			       BUF_MAX_D(prev)),
			      (required_times_set ? BUF_MIN_D(delay) :
			       BUF_MAX_D(delay)), orig_area, area);
	    }
	    prev = delay;
	    orig = last;
	    orig_area = area;
	}

	/*
	 * Completed one sweep through the network ... Should we
	 * go on ?? "loop_again" only if the delay/slack has been improved
	 * "delay" is the metric of performance that has been achieved
	 */
	if (required_times_set){
	    last = sp_minimum_slack(network, &dummy);
	    final = delay_slack_time(last);
	} else {
	    last = delay_latest_output(network, &dummy);
	    final = delay_arrival_time(last);
	}

	if (!buf_param->single_pass && changed_some){
	    /* Need to continue looping only if there is a substantial change */
	    if (required_times_set){
		/* 
		loop_again = ((final.rise-initial.rise) > V_SMALL &&
			(final.fall-initial.fall) > V_SMALL);
		*/
		loop_again = REQ_IMPR(final,initial);
	    } else {
	        /* 
		  loop_again = ((initial.rise-final.rise) > V_SMALL &&
		  (initial.fall-final.fall) > V_SMALL);
		  */
		loop_again = REQ_IMPR(initial, final);
	    }
	}
	st_free_table(buffered_nodes);

    }

    /*
    changed += sp_satisfy_max_load(network);
    */
    return changed;
}

/*
 * Gets the fanout set of node. If there is only one inverter in
 * the fanout set then returns its fanouts too. If more than one inverter
 * then it returns only the immediate fanouts of node
 */
static buf_alg_input_t *
sp_get_fanout_set(node)
node_t *node;
{
    int pin, pin1;
    lsGen gen, gen1;
    node_t *fo, *fo1;
    buf_alg_input_t *alg_input;
    int i=0, number_inv;

    alg_input = ALLOC(buf_alg_input_t, 1);
    alg_input->root = node;
    alg_input->node = node;
    alg_input->inv_node = NIL(node_t);
    number_inv = alg_input->num_pos = alg_input->num_neg = 0;

    foreach_fanout(node, gen, fo){
	if (node_function(fo) == NODE_INV){
	    alg_input->num_neg += node_num_fanout(fo);
	    alg_input->inv_node = fo;
	    number_inv++;
	} else {
	    (alg_input->num_pos)++;
	}
    }

    if (number_inv > 1){
	alg_input->inv_node = NIL(node_t);
	alg_input->num_neg = 0;
	alg_input->num_pos += number_inv;
    }

    /* Check for discrepency, i.e. there are no fanouts of inv_node */
    if (alg_input->inv_node != NIL(node_t) && alg_input->num_neg == 0){
	alg_input->inv_node = NIL(node_t);
	number_inv = 2; /* This avoids generating the fanouts of the inverter */
	(void)fprintf(siserr,"CHECK THIS --- inverter has no fanouts\n");
    }


    /* Allocate and fill in the data regarding the fanout destinations */
    if (number_inv > 1){
	/* get only the positive fanout signals */
	alg_input->fanouts = ALLOC(sp_fanout_t, alg_input->num_pos+number_inv);
    } else {
	alg_input->fanouts = ALLOC(sp_fanout_t, (alg_input->num_pos)+(
		alg_input->num_neg));
    }

    foreach_fanout_pin(node, gen, fo, pin){
	if (node_function(fo) == NODE_INV && number_inv <= 1){
	    foreach_fanout_pin(fo, gen1, fo1, pin1){
		alg_input->fanouts[i].fanout = fo1;
		alg_input->fanouts[i].pin = pin1;
		alg_input->fanouts[i].phase = PHASE_INVERTING;
		i++;
	    }
	} else {
	    alg_input->fanouts[i].fanout = fo;
	    alg_input->fanouts[i].pin = pin;
	    alg_input->fanouts[i].phase = PHASE_NONINVERTING;
	    i++;
	}
    }

    return alg_input;
}

static void
sp_already_buffered(table, node)
st_table *table;
node_t *node;
{
    lsGen gen;
    node_t *fo;
    
    foreach_fanout(node, gen, fo){
	if (BUFFER(fo)->type != NONE){
	    BUFFER(fo)->type = NONE;
	    if (!st_is_member(table, (char *)fo)){
		/* Recursively add the nodes that have been added herein */
		sp_already_buffered(table, fo);
	    }
	}
    }
    (void)st_insert(table, (char *)node, NIL(char));
}


/*
 * Optimal buffer selection for the nodes in nodevec based on 
 * the phases and required times of the signals 
 */

int
speed_buffer_array_of_nodes(network, buf_param, nodevec)
network_t *network;
buf_global_t *buf_param;
array_t *nodevec;
{
    lsGen gen;
    node_t *node, *fo;
    int changed;
    int i, do_delay_trace, count;

    count = 0;
    changed = FALSE;	
    do_delay_trace = FALSE;

    for (i = 0; i < array_n(nodevec); i++){
	node = array_fetch(node_t *, nodevec, i);
	if (node->type != PRIMARY_OUTPUT){
	    foreach_fanout(node, gen, fo){
		if (st_is_member(buf_param->glbuftable, (char *)fo)){
		    /* Need to update the req time data */
		    do_delay_trace = TRUE;
		    (void)lsFinish(gen);
		    break;
		}
	    }
	    if (do_delay_trace){
		/* Update the delay data via a trace */
		do_delay_trace = FALSE;
		(void)delay_trace(network, buf_param->model);
		count++;
		st_free_table(buf_param->glbuftable);
		buf_param->glbuftable = st_init_table(st_ptrcmp, st_ptrhash);
	    }
	    if (speed_buffer_node(network, node, buf_param, 0 /*unconstrained */)){
		changed = TRUE;
		(void)st_insert(buf_param->glbuftable, (char *)node, NIL(char));
	    }
	}
    }
    if (buf_param->debug){
	(void)fprintf(misout,"INFO: %d delay traces were required\n", count);
    }

    return changed;
}

/*
 * Finds the amount by which the the required time at the current node 
 * needs to be increased. Basically the negative of the min slack !!!
 * for teh case when the required times are set (constrained == TRUE)
 * Also determines the max_load that can be tolerated at crit fin...
 */
delay_time_t 
sp_buf_get_target(node, buf_param, constrained, max_loadp)
node_t *node;
buf_global_t *buf_param;
int constrained;
double *max_loadp;
{
    lsGen gen;
    double diff, load;
    node_t *fin, *fo, *crit_fin;
    int i, pin, cfi, crit_fin_index;
    delay_time_t wire_req, min_fo_slack, min_slack, slack, arr, req_diff;

    *max_loadp = POS_LARGE;    /* Default */
    req_diff = large_req_time; /* Default */
    min_slack = large_req_time;
    crit_fin_index = -1;

    /* Special case for PI node */
    if (node->type == PRIMARY_INPUT) {
	load = delay_get_parameter(node, DELAY_MAX_INPUT_LOAD);
	if (load != DELAY_NOT_SET) {
	    *max_loadp = load;
	}
	if (constrained) {
	    min_slack = delay_slack_time(node);
	    req_diff.rise = (0.0 - min_slack.rise);
	    req_diff.fall = (0.0 - min_slack.fall);
	}
	return req_diff;
    }

    /* Get the smallest slack at all the inputs */
    foreach_fanin(node, i, fin){
	if (buf_critical(fin, buf_param)){
	    assert(delay_wire_required_time(node, i, buf_param->model, &wire_req));
	    arr = delay_arrival_time(fin);
	    slack.rise = wire_req.rise - arr.rise;
	    slack.fall = wire_req.fall - arr.fall;

	    if (BUF_MIN_D(slack) < BUF_MIN_D(min_slack)){
		crit_fin_index = i;
	    }
	    min_slack.rise = MIN(slack.rise, min_slack.rise);
	    min_slack.fall = MIN(slack.fall, min_slack.fall);
	}
    }

    if (crit_fin_index == -1){
	/* The node "node" is not on critical path: no buffering req */
	req_diff.rise = req_diff.fall = 0.0;
    } else {
	cfi = BUFFER(node)->cfi;
 	assert(crit_fin_index == cfi);
	if (constrained){
	    req_diff.rise = (0.0 - min_slack.rise);
	    req_diff.fall = (0.0 - min_slack.fall);
	}
	/*
	 * Compute the limit on the load of the new gate !!!!
	 * Proportional to the difference in the slacks of the fanouts
	 */
	crit_fin = node_get_fanin(node, crit_fin_index);
	min_fo_slack = large_req_time;
	foreach_fanout_pin(crit_fin, gen, fo, pin){
	    if (fo == node) continue;
	    delay_wire_required_time(fo, pin, buf_param->model, &wire_req);
	    arr = delay_arrival_time(crit_fin);
	    slack.rise = wire_req.rise - arr.rise;
	    slack.fall = wire_req.fall - arr.fall;
	    min_fo_slack.rise = MIN(slack.rise, min_fo_slack.rise);
	    min_fo_slack.fall = MIN(slack.fall, min_fo_slack.fall);
	}
	/* Just a pessimistic view (to account for rise and fall asymmetry) */
	diff = BUF_MIN_D(min_fo_slack) - BUF_MIN_D(min_slack);
	if (diff > 0){
	  load = diff / (BUF_MAX_D(BUFFER(node)->prev_dr));  
	} else {
	  load = V_SMALL;  
	}
	*max_loadp = sp_get_pin_load(node, buf_param->model) + load;
    }

    return req_diff;
}

/*
 * Buffers the fanouts if fanout count is greater than limit 
 * Returns 1 if the fanout structure at the node was changed
 */
int
speed_buffer_node(network, node, buf_param, constrained)
network_t *network;
node_t *node;
buf_global_t *buf_param;
int constrained;	/* == 1 => req_times set at outputs */
{
    buf_alg_input_t *fanout_data;
    delay_time_t req_diff;
    int i, n;
    double max_load;
    int level = 0, changed = 0;

    /*
     * Set the required time at the input of the gate/buffer in the
     * original netw. The setting is done in the BUFFER field of "node"
     */
    sp_buf_annotate_gate(node, buf_param);

    req_diff = sp_buf_get_target(node, buf_param, constrained, &max_load);

    if (buf_param->debug){
	(void)fprintf(misout,"Buffering %s node %s\n\t (gate = %s, %s-diff = %6.2f:%-6.2f)\n",
	(node->type == INTERNAL ? "INTERNAL" : "PRIMARY_INPUT"),
	node_long_name(node), sp_name_of_impl(node),
	(constrained ? "CONSTRAINED": "UNCONSTRAINED"),
	req_diff.rise, req_diff.fall);
    }

    fanout_data = sp_get_fanout_set(node);
    fanout_data->max_ip_load = max_load;
    n = fanout_data->num_pos + fanout_data->num_neg;

    if (n < buf_param->limit){
	FREE(fanout_data->fanouts);
	FREE(fanout_data);
	return changed;
    }

    /* Also need to set the appropriate data for the inv_node */
    if (fanout_data->inv_node != NIL(node_t)){
	sp_buf_annotate_gate(fanout_data->inv_node, buf_param);
    }

    for (i = 0; i < n; i++){
	sp_buf_req_time(fanout_data->fanouts[i].fanout, buf_param, 
	    fanout_data->fanouts[i].pin, &(fanout_data->fanouts[i].req),
	    &(fanout_data->fanouts[i].load));
    }
    changed = sp_buffer_recur(network, buf_param, fanout_data, req_diff, &level);

    FREE(fanout_data->fanouts);
    FREE(fanout_data);

    return changed;
}

/*
 * Routine that returns 1 if the load at the output of "node" exceeds the
 * "max_load" specified by the parameters of "gate"
 */
int
sp_max_load_violation(node)
node_t *node;
{
    int i;
    double load, limit;
    lib_gate_t *gate;

    if (node == NIL(node_t) || (gate = lib_gate_of(node)) == NIL(lib_gate_t))
        return 0;

    /* Node is mapped: The gate must have the max_load parameter */
    load = delay_load(node);
    for ( i = node_num_fanin(node); i-- > 0; ){
	limit = delay_get_load_limit(gate->delay_info[i]);
	if (load > limit) return 1;
    }
    return 0;
}

/*
 * Traverses the outputs and makes sure that all nodes satisfy the
 * limitation specified by the "max_load" limitation of its gate
 * Returns 1 if the routine changed the network...
 */
/* ARGSUSED */
int
sp_satisfy_max_load(network, buf_param)
network_t *network;
buf_global_t *buf_param;
{
    int i, changed = 0;
    node_t *node;
    array_t *nodevec, *redo_nodes;

    redo_nodes = array_alloc(node_t *, 0);
    nodevec = network_dfs_from_input(network);

    for ( i = 0; i < array_n(nodevec); i++){
	node = array_fetch(node_t *, nodevec, i);
	if (node_type(node) == INTERNAL && sp_max_load_violation(node)){
	    array_insert_last(node_t *, redo_nodes, node);
	}
    }
    if (array_n(redo_nodes) > 0){
	(void)fprintf(sisout, "Ensuring that max_load constraints are met\n");
	for (i = 0; i < array_n(redo_nodes); i++){
	    node = array_fetch(node_t *, redo_nodes, i);
	    (void)fprintf(sisout, "Node %s\n", node_name(node));
	}
    }

    array_free(redo_nodes);
    array_free(nodevec);

    return changed;
}

static int speed_do_area_recover();

 /* Run the area recovery --- Visit all nodes from the outputs to the
  * inputs. In case the area can be reduced without worsening performance,
  * downsize the gate
  */
int
speed_buffer_recover_area(network, buf_param)
network_t *network;
buf_global_t *buf_param;
{
    int i, changed, false;
    node_t *node, *temp;
    double min_slack;
    array_t *nodevec;
    
    changed = FALSE;
    if (lib_network_is_mapped(network)) {
	nodevec = network_dfs_from_input(network);
	(void)sp_minimum_slack(network, &min_slack);

	for (i = 0; i < array_n(nodevec); i++){
	    node = array_fetch(node_t *, nodevec, i);
	    if (node->type == INTERNAL) {
		if (speed_do_area_recover(network, node, buf_param, min_slack)){
		    changed = TRUE;
		    (void)delay_trace(network, buf_param->model);
		    (void)sp_minimum_slack(network, &min_slack);
		}
	    }
	}
	
	array_free(nodevec);
    }
    return changed;
}

typedef struct buf_cost_struct{
    delay_time_t slack;
    double area;
    } buf_cost_t;
		
#define BUF_EQUAL_DOUBLE(a,b) (ABS((a)-(b)) < V_SMALL)

/* Routine to check if the slack is acceptable and area is smaller */
static int
buf_cur_version_is_better(cost1, cost2, min_slack)
buf_cost_t *cost1;
buf_cost_t *cost2;
double min_slack;
{
  double slack1 = BUF_MIN_D(cost1->slack);
  double slack2 = BUF_MIN_D(cost2->slack);
  double diff   = slack1 - slack2;

  if (BUF_EQUAL_DOUBLE(slack1, min_slack)) slack1 = min_slack;
  if (BUF_EQUAL_DOUBLE(slack2, min_slack)) slack2 = min_slack;
  if (BUF_EQUAL_DOUBLE(diff, 0.0))   diff   = 0.0;

  if (slack2 < min_slack /* 0 */) {
    return (diff > 0) ? 1 : 0;
  } else if (slack1 < min_slack) {
    return 0;
  } else {
    return (cost1->area < cost2->area) ? 1 : 0;
  }
}
#undef BUF_EQUAL_DOUBLE

/* Utility routines to recompute arrival time data when gates/loads are
 * changed
 */ 

static delay_time_t
buf_get_reloaded_arrival_time(node, load_diff)
node_t *node;
double load_diff;
{
  int i, ninputs;
  node_t *fanin;
  lib_gate_t *gate;
  double load;
  network_t *net = node_network(node);
  delay_time_t arrival, drive, t;
  delay_time_t **arrival_times_p, *arrival_times;

  load = delay_load(node) + load_diff;
  assert(node->type != PRIMARY_OUTPUT);
  if (node->type == PRIMARY_INPUT) {
      arrival.rise = arrival.fall = 0.0;
      if (!delay_get_pi_arrival_time(node, &arrival)){
	  if ((t.rise = delay_get_default_parameter(net,
                     DELAY_DEFAULT_ARRIVAL_RISE)) != DELAY_NOT_SET &&
              (t.fall = delay_get_default_parameter(net,
                     DELAY_DEFAULT_ARRIVAL_FALL)) != DELAY_NOT_SET) {
	      arrival = t;
	  }
      }
      drive.rise = drive.fall = 0.0;
      if (!delay_get_pi_drive(node, &drive)){
	  if ((t.rise = delay_get_default_parameter(net,
                     DELAY_DEFAULT_DRIVE_RISE)) != DELAY_NOT_SET &&
              (t.fall = delay_get_default_parameter(net,
                     DELAY_DEFAULT_DRIVE_FALL)) != DELAY_NOT_SET) {
	      drive = t;
	  }
      }
      arrival.rise += drive.rise * load;
      arrival.fall += drive.fall * load;
  } else {
      gate = lib_gate_of(node);
      ninputs = lib_gate_num_in(gate);
      arrival_times = ALLOC(delay_time_t, ninputs);
      arrival_times_p = ALLOC(delay_time_t *, ninputs);
      foreach_fanin(node, i, fanin){
	  arrival_times[i] = delay_arrival_time(fanin);
	  arrival_times_p[i] = &(arrival_times[i]);
      }
      arrival = delay_map_simulate(ninputs, arrival_times_p,
				   gate->delay_info, load); 
      FREE(arrival_times);
      FREE(arrival_times_p);
  }
  return arrival;
}

delay_time_t
buf_get_new_arrival_time(node, new_gate, min_slack, input_worsened)
node_t *node;
lib_gate_t *new_gate;
double min_slack;
int *input_worsened;
{
    int i, ninputs;
    node_t *fanin;
    delay_time_t *arrival_times;
    delay_time_t **arrival_times_p;
    lib_gate_t *orig_gate;
    delay_time_t arrival, sl, req, new_ip_req, new_ip_slack;
    delay_pin_t *pin_delay;
    double load, load_diff;
    
    orig_gate = lib_gate_of(node);
    *input_worsened = FALSE;
    if (new_gate == orig_gate) {
	return delay_arrival_time(node);
    }
    ninputs = lib_gate_num_in(new_gate);
    arrival_times = ALLOC(delay_time_t, ninputs);
    arrival_times_p = ALLOC(delay_time_t *, ninputs);
    load = delay_load(node);
    for (i = 0; i < ninputs; i++) {
	pin_delay = (delay_pin_t *)(new_gate->delay_info[i]);
	load_diff = delay_get_load(new_gate->delay_info[i]) -
	                 delay_get_load(orig_gate->delay_info[i]);
	fanin = node_get_fanin(node, i);
	arrival_times[i] = buf_get_reloaded_arrival_time(fanin, load_diff);
	arrival_times_p[i] = &(arrival_times[i]);
	
	/* Also make sure that this input does not become more critical */
	new_ip_req = delay_required_time(node);
	sp_subtract_delay(pin_delay->phase, pin_delay->block,
			  pin_delay->drive, load, &new_ip_req);
	req = delay_required_time(fanin);
	MIN_DELAY(req,req,new_ip_req); /* "req" is the new req time at fanin */
	new_ip_slack.rise = req.rise - arrival_times[i].rise;
	new_ip_slack.fall = req.fall - arrival_times[i].fall;
	
	if (BUF_MIN_D(new_ip_slack) < min_slack){
	    *input_worsened = TRUE;
	}
    }
    arrival = delay_map_simulate(ninputs, arrival_times_p,
				 new_gate->delay_info, load); 
    FREE(arrival_times);
    FREE(arrival_times_p);
    return arrival;
    
}
 /* Evaluate the different versions of the gate and select the smallest one
  * that does not worsen performance
  */
static int
speed_do_area_recover(network, node, buf_param, min_slack)
network_t *network;
node_t *node;
buf_global_t *buf_param;
double min_slack;
{
    lsGen gen;
    int i, best_index, input_restricted;
    buf_cost_t best_cost, cost;
    array_t *cost_array;
    lib_gate_t *orig_gate, *gate;
    delay_time_t required, arrival;
    
    /* Get the slack at the output of the node for the different versions*/
    required = delay_required_time(node);
    orig_gate = lib_gate_of(node);
    gen = lib_gen_gates(lib_gate_class(orig_gate));
    cost_array = array_alloc(buf_cost_t, 0);
    while (lsNext(gen, (char **)&gate, LS_NH) == LS_OK){
	arrival = buf_get_new_arrival_time(node, gate, min_slack, &input_restricted);
	if (input_restricted) {
	    /* Do not consider this one at all. Repowering leads to
	     * making other paths more critical (i.e. it is restricted by
	     * the loading at the inputs
	     */
	    cost.slack.rise = cost.slack.fall = NEG_LARGE;
	    cost.area = POS_LARGE;
	} else {
	    cost.slack.rise = required.rise - arrival.rise;
	    cost.slack.fall = required.fall - arrival.fall;
	    cost.area = lib_gate_area(gate);
	}
	array_insert_last(buf_cost_t, cost_array, cost);
    }
    lsFinish(gen);

    best_index = -1;
    best_cost.slack.rise = best_cost.slack.fall = NEG_LARGE;
    best_cost.area = POS_LARGE;
    for (i = 0; i < array_n(cost_array); i++) {
	cost = array_fetch(buf_cost_t, cost_array, i);
	if (buf_cur_version_is_better(&cost, &best_cost, min_slack)) {
	    best_cost = cost;
	    best_index = i;
	}
    }
    assert(best_index >= 0);
    gate = buf_get_gate_version(orig_gate, best_index);
    sp_replace_lib_gate(node, orig_gate, gate);
    array_free(cost_array);
    return (gate != orig_gate);	/* Change has been made */
}

