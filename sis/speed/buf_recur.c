
#include "sis.h"
#include "speed_int.h"
#include "buffer_int.h"

static int buf_cumulative_cap();
static int buf_annotate_trans3();
static void buf_implement_trans3();
static delay_pin_t *sp_get_versions_of_node();

static delay_time_t large_req_time  = {POS_LARGE, POS_LARGE};

static delay_pin_t buffer_unit_fanout_delay_model = {
    /* block */   {1.0, 1.0},
    /* drive */   {0.2, 0.2},
    /* phase */   PHASE_NONINVERTING,
    /* load */    1.0,
    /* maxload */ INFINITY
    };

#define EPS 1.0e-6
/*
 * The basic recursive routine that decides the fate of node "node"......
 * It may 
 *	(1) redistribute the fanouts of node by adding a buffer
 *	(2) Try a balanced decomposition of the outputs..
 *	(3) Failing 2 it tries to decompose node into smaller gates
 */
int 
sp_buffer_recur(network, buf_param, fanout_data, req_diff, levelp)
network_t *network;
buf_global_t *buf_param;
buf_alg_input_t *fanout_data;
delay_time_t req_diff;  	/* This much needs to be saved */
int *levelp;			/* Iteration counter */
{
    delay_model_t model = buf_param->model;
    int num_inv = buf_param->num_inv;
    int num_buf = buf_param->num_buf;
    sp_buffer_t **buf_array = buf_param->buf_array;
    node_t *node = fanout_data->node; /* The root node */
    node_t *inv_node = fanout_data->inv_node;/* The inverter at its fanout */
    sp_fanout_t *fanouts = fanout_data->fanouts; /* data on the fanouts */
    int num_pos = fanout_data->num_pos; /* The number of positive fanouts */
    int num_neg = fanout_data->num_neg; /* The number of negative fanouts */

    lsGen gen;
    lib_gate_t *gate;
    delay_pin_t *pin_delay, *gate_array;
    sp_buffer_t *buf_b, *buf_B, *buf_gI;
    node_t *fo, *new_b, *new_B, *new_inv_node;
    buf_alg_input_t *new_fanout_data, *old_fanout_data;
    int do_unbalanced, use_noninv_buf, changed, current_level;
    int b, B, g_I, g, m_p, m_n, cfi;
    int temp, i, k, n, pin, num_gates, excess;
    int valid_config, met_target, old_pos, old_neg, new_pos, new_neg;
    int best_b, best_B, best_gI, best_g, best_mp, best_mn;
    double root_node_drive;
    double total_cap_pos, total_cap_neg;
    double auto_route = buf_param->auto_route;
    double best_load_op_g, load_op_b, load_op_B, load_op_gI, load_op_g;
    double load, load_b, load_B, load_gI, orig_load;
    double *cap_L_pos, *cap_L_neg, *cap_K_pos, *cap_K_neg;
    double *a_array, min_area_notmet, min_area, cur_area;
    double area_b, area_B, area_gI;
    delay_time_t work_diff, best_req_B, best_req_gI;
    delay_time_t early_pos, early_neg, late_pos, late_neg;
    delay_time_t req_op_g, best_req_op_g;
    delay_time_t orig_drive, a_saving, margin, margin_pos, margin_neg, best;
    delay_time_t req_time, target, orig_req;
    delay_time_t fo_req, req_b, req_B, req_g, req_gI;
    delay_time_t neg_min_req, pos_min_req;

    if (BUF_MAX_D(req_diff) < EPS ){
	    if (buf_param->debug){
		(void)fprintf(sisout,"REQUIRED TIME DATA DOES NOT WARRANT BUFFERING\n");
	    }
	    return 0;
    }

    changed = FALSE;
    n = num_pos+num_neg;
    work_diff = req_diff;
    current_level = ++(*levelp);

    if (fanout_data->root == fanout_data->node) {
	/* See if we need to repower */
	if (buf_param->mode & REPOWER_MASK){
	    gate_array = sp_get_versions_of_node(node, buf_param, &num_gates,
		    &a_array);
	} else {
	    /* Put in the current impl of node as the only entry in gate_array*/

	    num_gates = 1;
	    gate_array = ALLOC(delay_pin_t, num_gates);
	    a_array = ALLOC(double, num_gates);
	    cfi = BUFFER(node)->cfi;
	    gate = lib_gate_of(node);
	    if (gate == NIL(lib_gate_t)){
		gate_array[0] = buffer_unit_fanout_delay_model;
		a_array[0] = 0;
	    } else {
		gate_array[0] = *((delay_pin_t *)(gate->delay_info[cfi]));
		a_array[0] = lib_gate_area(gate);
	    }
	}
    } else {
	/* At this level we have inverter that we can size */
	gate_array = sp_get_versions_of_node(node, buf_param, &num_gates, &a_array);
    }

    if (n == 1 && current_level == 1){
	if (buf_param->debug > 10){
	    (void)fprintf(sisout,"********Iteration %d -- %d nodes -- aim to save %.3f:%.3f\n",
		    *levelp, n, req_diff.rise, req_diff.fall);
	    (void)fprintf(sisout,"\tSingle fanout node -- TERMINATE \n");
	}
	fo = node_get_fanout(node, 0);
	k = node_get_fanin_index(fo, node);
	sp_buf_req_time(fo, buf_param, k, &req_time, &total_cap_pos);
	changed = sp_replace_cell_strength(network, buf_param, fanout_data, gate_array,
	    num_gates, req_time, large_req_time, total_cap_pos, 0.0, &a_saving);
	FREE(gate_array);
	FREE(a_array);
	return changed;
    }

    /*
     * For ease of later operations sort the fanouts so that the signal
     * required the earliest is at the start -- sorted in decreasing 
     * criticality. Inverting signals appear before non-inverting ones
     */
    qsort((void *)fanouts, (unsigned)n, sizeof(sp_fanout_t), buf_compare_fanout);
    if (buf_param->debug > 10){
	(void)fprintf(sisout,"******Iteration %d -- %d nodes -- aim to save %.3f:%.3f\n",
		*levelp, n, req_diff.rise, req_diff.fall);
	(void)fprintf(sisout,"\tRequired time and Load distribution (max_ip_load = %.3f) \n", fanout_data->max_ip_load);
	buf_dump_fanout_data(sisout, fanouts, n);
    }

    /*
     * Compute the cumulative capacitances of grouping first (m-1)
     * non_inv signals as cap_K_pos, and last (n-m+1) signals as cap_L_pos....
     * Done analogously for the negatiive phase signals
     */
    do_unbalanced = buf_cumulative_cap(fanouts, buf_param, num_pos,
				       num_neg, &cap_K_pos, &cap_K_neg,
				       &cap_L_pos, &cap_L_neg);
    total_cap_pos = cap_K_pos[num_pos]; total_cap_neg = cap_K_neg[num_neg];

    /* Also record the earliest required times among the two sets */
    neg_min_req = pos_min_req = large_req_time;
    for(i = 0; i < num_pos; i++){
	MIN_DELAY(pos_min_req, pos_min_req, fanouts[i].req);
    }
    for(i = num_pos; i < n; i++){
	MIN_DELAY(neg_min_req, neg_min_req, fanouts[i].req);
    }


    /*
     * As the obvious step make sure that the required time is computed
     * based on the best possible match version of the gate implementing
     * node (This is done only for the first pass in the recursion). We
     * want to compare the result of buffering treansformations with this value
     */
    orig_req = BUFFER(node)->req_time;
    orig_drive = BUFFER(node)->prev_dr;
    orig_load = sp_get_pin_load(node, model);

    /* Adjust for delay due to limited drive of preceeding gate */
    sp_drive_adjustment(orig_drive, orig_load, &orig_req);

    /*
     * Set a target for the current recursion. This is the value of the
     * required time such that making the required time greater than this
     * will not effect the delay of the circuit
     */
    target.rise = orig_req.rise + req_diff.rise;
    target.fall = orig_req.fall + req_diff.fall;

    if (buf_param->debug > 10){
	(void)fprintf(sisout,"Initial req_time %6.3f:%-6.3f , Target %6.3f:%-6.3f\n",
		orig_req.rise, orig_req.fall, target.rise, target.fall);
    }
    if (current_level == 1 && (buf_param->mode & REPOWER_MASK)){
    /* first recursion call -- try replacements if allowed */
	if (buf_param->debug > 10){
	    (void)fprintf(sisout,"\tCOMPUTING BEST VERSION OF CELL\n");
	}

	temp = buf_param->do_decomp;
	buf_param->do_decomp = 0;    /* no decomp permitted here */
	changed = sp_replace_cell_strength(network, buf_param, fanout_data,
                gate_array, num_gates, pos_min_req, neg_min_req,
		total_cap_pos, total_cap_neg, &a_saving);
	buf_param->do_decomp = temp;

	orig_req.rise += a_saving.rise;
	orig_req.fall += a_saving.fall;
	work_diff.rise = req_diff.rise - a_saving.rise;
	work_diff.fall = req_diff.fall - a_saving.fall;
	if (buf_param->debug > 10)
	    (void)fprintf(sisout,"Required time after powering %6.3f:%-6.3f\n",
		    orig_req.rise, orig_req.fall);

	if (REQ_IMPR(orig_req, target)) {
	    /* 
	     * Mission accomplished  !!!!
	     * achieved the target -- return after freeing storage 
	     */
	    goto exit_recursion;
	}
    }

    /*
     * TRY the UNBALANCED TRANSFORMATION (trans3 according to the paper)
     *
     * Loop over all possible implementations of the root gate, inv,
     * inverting buffers b and B ------ refer to the DAC paper
     * Keep the best required time configuration. Then compare it to the
     * required time achieved by repowering alone....
     */
    met_target = FALSE;
    best_b = best_B = best_gI = best_g = best_mp = best_mn = -1;
    best.rise = best.fall = NEG_LARGE;
    min_area_notmet = min_area = POS_LARGE;
    if (do_unbalanced && (buf_param->mode & UNBALANCED_MASK)){
	/*
	 * unbalanced is permitted or warranted by the delay data . We need
	 * to make sure that the configurations are acceptable in terms of the
	 * fanout load that the gate can drive.
	 */
	for(g_I = 0; g_I < num_inv; g_I++){
	    for(m_p = 0; m_p <= num_pos; m_p++){ /* positive fanout partition*/
		early_pos = late_pos = large_req_time;
		for (i = 0; i < m_p; i++){
		    MIN_DELAY(early_pos, early_pos, fanouts[i].req);
		}
		for (i = m_p; i < num_pos; i++){
		    MIN_DELAY(late_pos, late_pos, fanouts[i].req);
		}
		for(b = 0; b < num_buf; b++){
		    load_op_b = 0.0;
		    buf_b = buf_array[b];
		    if (m_p == num_pos){   /* "b" drives no fanouts */
			req_b = large_req_time;
			load_b = 0.0; area_b = 0;
		    } else {
			load_b = buf_b->ip_load+auto_route;
			area_b = buf_b->area;
			req_b = late_pos;
			load_op_b = cap_L_pos[m_p];
			sp_subtract_delay(buf_b->phase, buf_b->block,
				buf_b->drive, load_op_b, &req_b);
		    }
		    for (m_n = 0; m_n <= num_neg; m_n++){/*negative partition*/
			early_neg = late_neg = large_req_time;
			for (i = 0; i < m_n; i++){
			    MIN_DELAY(early_neg, early_neg, fanouts[num_pos+i].req);
			}
			for (i = m_n; i < num_neg; i++){
			    MIN_DELAY(late_neg, late_neg, fanouts[num_pos+i].req);
			}
			for(B = 0; B < num_buf; B++){  /* choice of buffer B */
			    if (B >= num_inv && b < num_inv){
				/* Driving the negative partition by non-inv
				 * buffer requires that the positive partition
				 * be similarly driven by non-inv buffer */
				break;
			    }
			    load_op_B = 0.0;
			    buf_B = buf_array[B];
			    if ((m_n == num_neg && B >= num_inv) ||
				(m_n == num_neg && m_p == num_pos)) {
				req_B = large_req_time;
				load_B = 0.0; area_B = 0;
			    } else {
				load_B = buf_B->ip_load+auto_route;
				area_B = buf_B->area;
				if (B >= num_inv){
				    req_B = late_neg;
				} else {
				    MIN_DELAY(req_B, req_b, late_neg);
				    load_op_B = load_b;
				}
				load_op_B += cap_L_neg[m_n];
				sp_subtract_delay(buf_B->phase, buf_B->block,
				    buf_B->drive, load_op_B, &req_B);
			    }

			    /* Now compute the required time at input of "gI" */
			    load_op_gI = 0.0;
			    buf_gI = buf_array[g_I];
			    if (m_n == 0 && B < num_inv){
				/* gI is driving no signals in this case */
				req_gI = large_req_time;
				load_gI = 0.0;
				area_gI = 0;
			    } else{
				load_gI = buf_gI->ip_load+auto_route;
				area_gI = buf_gI->area;
				if (B >= num_inv){
				    load_op_gI += load_B;
				    MIN_DELAY(req_gI, req_B, early_neg);
				} else {
				    req_gI = early_neg;
				}
				load_op_gI += cap_K_neg[m_n];
				sp_subtract_delay(buf_gI->phase, buf_gI->block,
				    buf_gI->drive,load_op_gI, &req_gI);
			    }

			    for(g=0; g < num_gates; g++){ /* Versions of root */
				pin_delay = &(gate_array[g]);
				if (pin_delay->load >
				    fanout_data->max_ip_load){
				    /* load limit violated !!!  */
				    continue;
				}
				load_op_g = load_gI;
				if (b >= num_inv){
				    MIN_DELAY(req_g,req_b,req_gI);
				    load_op_g += load_b;
				} else {
				    MIN_DELAY(req_g,req_B,req_gI);
				    load_op_g += load_B;
				}
				if(m_p != 0){
				    MIN_DELAY(req_g, req_g, early_pos);
				}
				load_op_g += cap_K_pos[m_p];
				req_op_g = req_g; /* save the req time */

				sp_subtract_delay(pin_delay->phase, 
				    pin_delay->block, pin_delay->drive,
				    load_op_g, &req_g);
				/* Account for drive of previous stage */
				sp_drive_adjustment(orig_drive, pin_delay->load, &req_g);

				valid_config = !((B < num_inv) ^ (b < num_inv));
				cur_area = a_array[g]+area_b+area_B+area_gI;
				if (valid_config && REQ_IMPR(req_g,target) &&
					(cur_area < min_area)){
				    /* Keep the smallest area config */
				    met_target = TRUE;
				    min_area = cur_area;
				    best_b = b; best_B = B; best_gI = g_I;
				    best_g=g; best_mp = m_p; best_mn = m_n;
				    best_req_op_g = req_op_g;
				    best_load_op_g = load_op_g;
				} 
				if ( valid_config && !met_target){
				    if (REQ_IMPR(req_g, best)){
					best = req_g;
					min_area_notmet = cur_area;
					best_req_B=req_B, best_req_gI = req_gI;
					best_b = b; best_B = B; best_gI = g_I;
					best_g=g; best_mp = m_p; best_mn = m_n;
					best_req_op_g = req_op_g;
					best_load_op_g = load_op_g;
				    } else if (REQ_EQUAL(req_g, best) &&
					    (cur_area < min_area_notmet) ){
					min_area_notmet = cur_area;
					best_req_B=req_B, best_req_gI = req_gI;
					best_b = b; best_B = B; best_gI = g_I;
					best_g=g; best_mp = m_p; best_mn = m_n;
					best_req_op_g = req_op_g;
					best_load_op_g = load_op_g;
				    }
				}
			    }
			}
		    }
		    /* Dont try more values for "b" if no positive fanouts */
		    if (num_pos == 0) break;
		}
	    }
	    /* 
	     * If there are no negative fanout gates, no need to 
	     * try replacements for the possible choices of node gI
	     */
	    if (num_neg == 0) break;
	}
    }

    use_noninv_buf =  ((best_b >= num_inv) && (best_B >= num_inv));

    if ((buf_param->mode & UNBALANCED_MASK) && buf_param->debug > 10){
	if (do_unbalanced){
	    (void)fprintf(sisout,"Trans3 can get %6.3f:%-6.3f at %d fanin\n",
		    best.rise, best.fall, BUFFER(node)->cfi);
	} else {
	    (void)fprintf(sisout,"NO NEED TO DO THE UNBALANCED DECOMP\n");
	}
    }

    /* The computation was based on improving the req_time at cfi input
     * However, it is possible that the slack worsened at some other input.
     * If this happens set the best to be NEG_LARGE to reject the
     * transformation, thereby avoid worsening the circuit performance
     * Do not check when interfacing to the mapper (interactive == FALSE)
     */
    if (buf_param->interactive && !met_target && 
	REQ_IMPR(best,orig_req) && (node_num_fanin(node) > 1)){
	if (buf_failed_slack_test(node, buf_param, best_g, best_req_op_g, 
				  best_load_op_g)){
	    if (buf_param->debug > 10){
		(void)fprintf(sisout, "SLACK CHECK during unbalanced xform\n");
	    }	   
	    best.rise = best.fall = NEG_LARGE;
	}
    }
    /* 
     * NOTE: When unbalanced decomp is not desired then best = NEG_LARGE 
     * and also "met_target == FALSE"
     * Hence the condition being tested below is FALSE
     */
    if (met_target || REQ_IMPR(best, orig_req)){
	if (!met_target){
	    /*
	     * We should first check if a balanced decomp is the right thing
	     * one such case is when (num_pos == 0 && req_B >= req_gI)
	     */
	    pin_delay = &(gate_array[best_g]);
	    sp_drive_adjustment(pin_delay->drive,
		buf_array[best_gI]->ip_load + auto_route, &best_req_gI);
	    sp_drive_adjustment(pin_delay->drive,
		buf_array[best_B]->ip_load + auto_route, &best_req_B);
	    if ((buf_param->mode & BALANCED_MASK) &&  num_pos == 0
		    && !use_noninv_buf 
		    && ((best_req_gI.rise - best_req_B.rise < EPS)
			|| (best_req_gI.fall - best_req_B.fall < EPS))){
		changed += buf_evaluate_trans2(network, buf_param, fanout_data,
			gate_array, a_array, num_gates, cap_K_pos, cap_K_neg,
			work_diff, levelp);
		goto exit_recursion;
	    }
	}

	/*
	 * Have gotton some saving over naive buffering using trans3
	 * Generate the new configuration and then recur if required
	 */
	changed = 1;
	buf_implement_trans3(network, node, inv_node, fanouts, use_noninv_buf,
		num_pos, num_neg, best_mn, best_mp, &new_b, &new_B);
	new_inv_node = inv_node;
	if (new_B == inv_node){
	    new_inv_node = NIL(node_t);
	}
	(void)buf_annotate_trans3(network, buf_param, fanouts, use_noninv_buf,
	  node, new_inv_node, new_b, new_B, best_g, best_gI, best_b, best_B, 
	  &margin_pos, &margin_neg);

	if (met_target){
	    if (buf_param->debug > 10){
		(void)fprintf(sisout,"\t--- ACHIEVED DESIRED SAVING ---\n");
	    }
	    goto exit_recursion;
	}

	if (new_b == NIL(node_t) && new_B == NIL(node_t) 
		&& new_inv_node == NIL(node_t)){
	    /* trans3 has done the same job as simple repowering of root */
	    if (buf_param->debug > 10){
		(void)fprintf(sisout,"--- NO CHANGE IN FANOUT SET ---\n");
	    }
	    goto exit_recursion;
	}

	old_pos = best_mp;
	old_neg = best_mn;
	new_neg = num_pos - best_mp;
	new_pos = num_neg - best_mn;

	if (use_noninv_buf ? 
		(BUF_MAX_D(margin_pos) < EPS && BUF_MAX_D(margin_neg) < EPS) :
		(BUF_MAX_D(margin_pos)< EPS) ){
	    if (buf_param->debug > 10){
		(void)fprintf(sisout,"\t--- RECURSION ON NEW NODES WONT HELP ---\n");
	    }
	} else if (new_pos + new_neg > 0){
	    /*
	     * Recur on the sub_problems ---
	     * Set up the fanout array for the recursion branch 
	     * for the recently added nodes call the recursion routine
	     */
	    root_node_drive = BUF_MAX_D(gate_array[best_g].drive);

	    if (use_noninv_buf){
		new_pos = num_pos - best_mp;
		new_neg = num_neg - best_mn;
		/*
		 * Need to see if we want to recur on the positive and/or
		 * negative partions (based on margin_pos and margin_neg)
		 */
		if (new_pos > 0 && (margin_pos.rise > EPS && margin_pos.fall > EPS)){
		    buf_b = buf_array[best_B];
		    new_fanout_data = ALLOC(buf_alg_input_t, 1);
		    new_fanout_data->num_pos = new_pos;
		    new_fanout_data->num_neg = 0;
		    new_fanout_data->max_ip_load = buf_B->ip_load +
		            BUF_MIN_D(margin_pos)/root_node_drive;
		    new_fanout_data->root = fanout_data->root;
		    new_fanout_data->node = new_b;
		    new_fanout_data->inv_node = NIL(node_t);
		    new_fanout_data->fanouts = ALLOC(sp_fanout_t, new_pos);
		    for (i = 0; i < new_pos; i++){
			new_fanout_data->fanouts[i] = fanouts[best_mp+i];
		    }
		    changed += sp_buffer_recur(network, buf_param, new_fanout_data,
			     margin_pos, levelp);
		    FREE(new_fanout_data->fanouts);
		    FREE(new_fanout_data);
		} 
		if (new_neg > 0 && (margin_neg.rise > EPS && margin_neg.fall > EPS)){
		    buf_B = buf_array[best_B];
		    new_fanout_data = ALLOC(buf_alg_input_t, 1);
		    new_fanout_data->num_pos = new_neg;
		    new_fanout_data->num_neg = 0;
		    new_fanout_data->max_ip_load = buf_B->ip_load +
		            BUF_MIN_D(margin_neg)/root_node_drive;
		    new_fanout_data->node = new_B;
		    new_fanout_data->inv_node = NIL(node_t);
		    new_fanout_data->fanouts = ALLOC(sp_fanout_t, new_neg);
		    for (i = 0; i < new_neg; i++){
			new_fanout_data->fanouts[i] = 
				fanouts[num_pos+best_mn+i];
		    }
		    changed += sp_buffer_recur(network, buf_param, new_fanout_data,
			     margin_neg, levelp);
		    FREE(new_fanout_data->fanouts);
		    FREE(new_fanout_data);
		}
	    } else {
		/*
		 * Recur with "new_B" as the root of the fanout problem
		 */
		new_fanout_data = ALLOC(buf_alg_input_t, 1);
		    /*
		     * We have no margin to shoot for when no old positive
		     * signals are present. Set the margin as per the 
		     * saving yet to be accomplished
		     */
/*
		if (old_pos == 0){
		    req_g = BUFFER(node)->req_time;
		    sp_drive_adjustment(orig_drive,
			    sp_get_pin_load(node, model), &req_g);
		    margin.rise = target.rise - req_g.rise;
		    margin.fall = target.fall - req_g.fall;
		} else {
		    margin = margin_pos;
		}
*/
		new_fanout_data->num_pos = new_pos;
		new_fanout_data->num_neg = new_neg;
		new_fanout_data->max_ip_load = buf_B->ip_load +
		       BUF_MIN_D(margin_pos)/root_node_drive;
		new_fanout_data->node = new_B;
		new_fanout_data->inv_node = new_b;
		new_fanout_data->fanouts = ALLOC(sp_fanout_t, new_pos+new_neg);
		for (i = 0; i < new_pos; i++){
		    new_fanout_data->fanouts[i] = fanouts[num_pos+best_mn+i];
		    new_fanout_data->fanouts[i].phase = PHASE_NONINVERTING;
		}
		for (i = 0; i < new_neg; i++){
		    new_fanout_data->fanouts[new_pos+i]=fanouts[best_mp+i];
		    new_fanout_data->fanouts[new_pos+i].phase = PHASE_INVERTING;
		}
		changed += sp_buffer_recur(network, buf_param, new_fanout_data,
			 margin_pos, levelp);
		FREE(new_fanout_data->fanouts);
		FREE(new_fanout_data);
	    }
	} else if (buf_param->debug > 10) {
		(void)fprintf(sisout,"\t -- NO FANOUTS TO RECUR ON --\n");
	}

	/*
	 * The recursion branch for the original nodes -- called
	 * after the recursion of the added nodes is completed...
	 * Need to do different things based on whether non-invering
	 * buffers were used or not during the unbalanced decomposition
	 */
	load = load_gI = 0; req_gI = req_g = large_req_time;
	if (use_noninv_buf){
	    excess = (new_b != NIL(node_t));
	} else {
	    excess = (new_B != NIL(node_t));
	}
	old_fanout_data = ALLOC(buf_alg_input_t, 1);
	old_fanout_data->max_ip_load = fanout_data->max_ip_load;
	old_fanout_data->fanouts = ALLOC(sp_fanout_t, old_pos+old_neg+excess);
	for (i = 0; i < old_pos; i++){
	    old_fanout_data->fanouts[i] = fanouts[i];
	    load += old_fanout_data->fanouts[i].load;
	    MIN_DELAY(req_g, req_g, old_fanout_data->fanouts[i].req);
	}
	for (i = 0; i < old_neg; i++){
	    old_fanout_data->fanouts[old_pos+i] = fanouts[num_pos+i];
	    load_gI += old_fanout_data->fanouts[old_pos+i].load;
	    MIN_DELAY(req_gI, req_gI, old_fanout_data->fanouts[old_pos+i].req);
	}
	if (excess){
	    old_fanout_data->fanouts[old_pos+old_neg].pin = 0;
	    old_fanout_data->fanouts[old_pos+old_neg].phase = PHASE_NONINVERTING;
	    if (use_noninv_buf){
		old_fanout_data->fanouts[old_pos+old_neg].fanout = new_b;
		old_fanout_data->fanouts[old_pos+old_neg].load = 
			sp_get_pin_load(new_b, model) + auto_route;
		old_fanout_data->fanouts[old_pos+old_neg].req = BUFFER(new_b)->req_time;
		load += old_fanout_data->fanouts[old_pos+old_neg].load;
		MIN_DELAY(req_g, req_g, old_fanout_data->fanouts[old_pos+old_neg].req);
	    } else {
		old_fanout_data->fanouts[old_pos+old_neg].fanout = new_B;
		old_fanout_data->fanouts[old_pos+old_neg].load = 
			sp_get_pin_load(new_B, model) + auto_route;
		old_fanout_data->fanouts[old_pos+old_neg].req = BUFFER(new_B)->req_time;
		load += old_fanout_data->fanouts[old_pos+old_neg].load;
		MIN_DELAY(req_g, req_g, old_fanout_data->fanouts[old_pos+old_neg].req);
	    }
	}
	/*
	 * compute the amount of saving yet to be achieved... Some
	 * of it has been achieved by the application of trans3
	 */
	if (old_neg > 0){
	    pin_delay = get_pin_delay(new_inv_node, BUFFER(new_inv_node)->cfi, model);
	    sp_subtract_delay(pin_delay->phase, pin_delay->block,
		    pin_delay->drive, load_gI, &req_gI);
	    MIN_DELAY(req_g, req_g, req_gI);
	    load += pin_delay->load + auto_route;
	}
	pin_delay = get_pin_delay(node, BUFFER(node)->cfi, model);
	sp_subtract_delay(pin_delay->phase, pin_delay->block,
		pin_delay->drive, load, &req_g);
	BUFFER(node)->req_time = req_g;
	sp_drive_adjustment(orig_drive, pin_delay->load, &req_g);
	margin.rise = target.rise - req_g.rise;
	margin.fall = target.fall - req_g.fall;

	old_fanout_data->root = fanout_data->root;
	old_fanout_data->node = node;
	old_fanout_data->inv_node = new_inv_node;
	old_fanout_data->num_pos = old_pos+excess;
	old_fanout_data->num_neg = old_neg;

	/* Now recur on the root node and the appropriate fanout set */
	changed += sp_buffer_recur(network, buf_param, old_fanout_data, margin, levelp);
	FREE(old_fanout_data->fanouts);
	FREE(old_fanout_data);

	/* Figure the new required time at the input of "node" */
	req_g = BUFFER(node)->req_time;
	load = sp_get_pin_load(node, model);
	sp_drive_adjustment(orig_drive, load, &req_g);

	if (REQ_IMPR(req_g, target)){
	    /* trans3 has been able to meet the target required time */
	    goto exit_recursion;
	}
    } else if (buf_param->mode & BALANCED_MASK) {
	/*
	 * No saving from trans3 (the unbalanced transformation) -----
	 * Do a balanced decomp for the positive & negative signal sets 
	 */
	margin.rise = target.rise - orig_req.rise;
	margin.fall = target.fall - orig_req.fall;
	changed += buf_evaluate_trans2(network, buf_param, fanout_data,
		gate_array, a_array, num_gates, cap_K_pos, cap_K_neg,
		margin, levelp);
	/* Compute the required time at "node" input, account for prev drive */
	req_g = BUFFER(node)->req_time;
	load = sp_get_pin_load(node, model);
	sp_drive_adjustment(orig_drive, load, &req_g);

	if (REQ_IMPR(req_g, target)){
	    /* Balanced distrbution meets the target required time */
	    goto exit_recursion;
	}
    }

    /*
     * When we reach this stage we have tried both balanced & unbalanced
     * redistributions of the fanout set and still not met the target.
     * As a last resort we try to see if duplication of root will help
     */
    if (current_level == 1 && buf_param->do_decomp){
	total_cap_pos = 0.0;
	req_time = large_req_time;
	foreach_fanout_pin(node, gen, fo, pin){
	    if (BUFFER(fo)->type == NONE){
		sp_buf_req_time(fo, buf_param, pin, &fo_req, &load);
	    } else {
		fo_req = BUFFER(fo)->req_time;
		load = sp_get_pin_load(fo, model);
	    }
	    total_cap_pos += load;
	    MIN_DELAY(req_time, req_time, fo_req);
	}
	/* HERE */
	fanout_data->inv_node = NIL(node_t);
	changed += sp_replace_cell_strength(network, buf_param, fanout_data, gate_array,
	num_gates, req_time, large_req_time, total_cap_pos, 0.0, &a_saving);

	if (buf_param->debug > 10 && a_saving.rise > 0 && a_saving.fall > 0){
	    (void)fprintf(sisout,"Saved %6.3f:%-6.3f in the end for node %s\n",
		a_saving.rise, a_saving.fall, node_name(node));
	}
    }

    /* Garbage collection and end of the routine */
exit_recursion: 
    FREE(a_array);
    FREE(gate_array);
    FREE(cap_L_pos); FREE(cap_L_neg);
    FREE(cap_K_pos); FREE(cap_K_neg);

    return changed;
} 

/*
 * Compare fanouts -- use the minimum of the rise and fall for the time
 * being. In future it may sort by either rise or fall -- whichever is more 
 * critical 
 */
int
buf_compare_fanout(ptr1, ptr2)
char *ptr1, *ptr2;
{
    sp_fanout_t *n1 = (sp_fanout_t *)ptr1, *n2 = (sp_fanout_t *)ptr2;
    double d1, d2;

    d1 = MIN(n1->req.rise, n1->req.fall);
    d2 = MIN(n2->req.rise, n2->req.fall);

    if (n1->phase == PHASE_INVERTING && n2->phase == PHASE_NONINVERTING){
	return 1;
    } else if (n1->phase == PHASE_NONINVERTING && n2->phase == PHASE_INVERTING){
	return -1;
    } else {
	if (d1 < d2) return -1;
	else if (d1 > d2) return 1;
	else return 0;
    }
}

static int
buf_cumulative_cap(fanouts, buf_param, num_pos, num_neg, 
	cap_K_posp, cap_K_negp, cap_L_posp, cap_L_negp)
sp_fanout_t *fanouts;
buf_global_t *buf_param;
int num_pos, num_neg;
double **cap_K_posp, **cap_K_negp, **cap_L_posp, **cap_L_negp;
{
    int i, n, j;
    int do_unbalanced;
    delay_time_t t;
    double min_req_diff = buf_param->min_req_diff;
    double min_r, max_r, min_f, max_f;
    double total_cap_pos, total_cap_neg;
    double *cap_K_pos, *cap_K_neg, *cap_L_pos, *cap_L_neg;

    n = num_pos+num_neg;
    cap_K_pos = ALLOC(double, num_pos+1);
    cap_K_neg = ALLOC(double, num_neg+1);
    cap_L_pos = ALLOC(double, num_pos+1);
    cap_L_neg = ALLOC(double, num_neg+1);


    do_unbalanced = FALSE;
    cap_K_pos[0] = cap_K_neg[0] = 0.0;
    min_r = min_f = POS_LARGE;
    max_r = max_f = NEG_LARGE;
    for(i = 0; i < num_pos; i++){
	t = fanouts[i].req;
	cap_K_pos[i+1] = cap_K_pos[i] + fanouts[i].load;
	min_r = MIN(min_r, t.rise);
	min_f = MIN(min_f, t.fall);
	max_r = MAX(max_r, t.rise);
	max_f = MAX(max_f, t.fall);
    }
    if (((max_r - min_r) > min_req_diff) && ((max_f - min_f) > min_req_diff)){
	do_unbalanced = TRUE;
    }
    min_r = min_f = POS_LARGE;
    max_r = max_f = NEG_LARGE;
    for(i = num_pos, j = 0; i < n; i++, j++){
	t = fanouts[i].req;
	cap_K_neg[j+1] = cap_K_neg[j] + fanouts[i].load;
	min_r = MIN(min_r, t.rise);
	min_f = MIN(min_f, t.fall);
	max_r = MAX(max_r, t.rise);
	max_f = MAX(max_f, t.fall);
    }
    if (((max_r - min_r) > min_req_diff) && ((max_f - min_f) > min_req_diff)){
	do_unbalanced = TRUE;
    }

    total_cap_pos = cap_K_pos[num_pos];
    total_cap_neg = cap_K_neg[num_neg];
    for (i = 0; i <= num_pos; i++){
	cap_L_pos[i] = total_cap_pos - cap_K_pos[i];
    }
    for (i = 0; i <= num_neg; i++){
	cap_L_neg[i] = total_cap_neg - cap_K_neg[i];
    }

    *cap_K_posp = cap_K_pos;
    *cap_K_negp = cap_K_neg;
    *cap_L_posp = cap_L_pos;
    *cap_L_negp = cap_L_neg;

    return do_unbalanced;
}

void
buf_dump_fanout_data(fp, fanouts, n)
FILE *fp;
sp_fanout_t *fanouts;
int n;
{
    int i;
    
    for(i = 0; i < n; i++){
	(void)fprintf(fp,"%6.3f:%-6.3f @ (%s)%6.3f   ", 
		      fanouts[i].req.rise, fanouts[i].req.fall,
		      (fanouts[i].phase == PHASE_INVERTING ? "-" : "+"),
		      fanouts[i].load);
	if (i%2 && i != (n-1)) (void)fprintf(fp,"\n");
    }
    (void)fprintf(fp,"\n");
}

/*
 * Generate the configuration for the unbalalanced decomposition. This is one
 * two configurations
 * use_noninv_buf == FALSE then
 * 	node drives (node_inv and bew_B), new_B drives bew_B
 * else
 * 	node_drives (node_inv and new_b), node_inv drives new_B
 */

static void
buf_implement_trans3(network, node, node_inv, fanouts, use_noninv_buf,
	num_pos, num_neg, best_mn, best_mp, new_b, new_B)
network_t *network;
node_t *node, *node_inv;
sp_fanout_t *fanouts;
int use_noninv_buf;	/* if == 1 , use the non inverting buffers for b & B */
int num_pos, num_neg, best_mn, best_mp;
node_t **new_b, **new_B;
{
    node_t *fo;
    int i, need_b, need_B, B_has_fanouts;

    need_b = (best_mp != num_pos);
    B_has_fanouts = (best_mn != num_neg);
    need_B = B_has_fanouts || (!use_noninv_buf && need_b);

    *new_B = *new_b = NIL(node_t);
    if (need_B){
	if (node_inv != NIL(node_t) && best_mn == 0 && !use_noninv_buf){
	    /* All the negatative fanouts are driven by the gate B -- so we
	    can as well use the gate g_I (node_inv) for it */
	    *new_B = node_inv;
	} else {
	    if (use_noninv_buf){
		assert(node_inv != NIL(node_t));
		*new_B = node_literal(node_inv, 1);
	    } else {
		*new_B = node_literal(node, 0);
	    }
	    network_add_node(network, *new_B);
	}
	if (B_has_fanouts){
	    for (i = best_mn; i < num_neg; i++){
		fo = fanouts[num_pos + i].fanout;
		assert(node_patch_fanin(fo, node_inv, *new_B));
	    }
	}
    }
    if (need_b){
	if (use_noninv_buf){
	    *new_b = node_literal(node, 1);
	} else {
	    *new_b = node_literal(*new_B, 0);
	}
	network_add_node(network, *new_b);
	for (i = best_mp; i < num_pos; i++){
	    fo = fanouts[i].fanout;
	    assert(node_patch_fanin(fo, node, *new_b));
	}
    }
}

/*
 * Having implemented the nodes in trans3, we need to annotate them with the
 * proper data to continue the recursion. The BUF field in the data struct
 * needs to be updated as does the required time data
 * Returns the number of nodes violating the load limit...
 */
static int
buf_annotate_trans3(network, buf_param, fanouts, use_noninv_buf, node, node_inv, new_b,
	new_B, best_g, best_gI, best_b, best_B, margin_pos, margin_neg)
network_t *network;
buf_global_t *buf_param;
sp_fanout_t *fanouts;
int use_noninv_buf;	/* Use the non-inverting buffers for new_b and new_B */
node_t *node, *node_inv, *new_b, *new_B;
int best_g, best_gI, best_b, best_B;
delay_time_t *margin_pos, *margin_neg;
{
    int excess;
    pin_phase_t prev_ph;
    lib_gate_t *root_gate;
    double auto_route = buf_param->auto_route;
    double load, cap_b, cap_B, cap_gI, root_limit;
    sp_buffer_t *buf_b, *buf_B, *buf_g, *buf_gI;
    delay_time_t pos_branch_req, neg_branch_req;
    delay_time_t prev_dr, prev_bl;
    delay_time_t req_b, req_B, req_gI, req_g;
    int i, cfi, index, old_pos, new_pos, old_neg, new_neg, num_violations;

    old_pos = node_num_fanout(node);
    old_neg = buf_num_fanout(node_inv);
    new_pos = buf_num_fanout(new_b);
    new_neg = buf_num_fanout(new_B);

    excess =  (node_inv != NIL(node_t));
    excess += (use_noninv_buf ? (new_b != NIL(node_t)): (new_B != NIL(node_t)));
    old_pos -= excess;
    excess = ((!use_noninv_buf && new_b != NIL(node_t)) ? 1 : 0);
    new_neg -= excess;


    num_violations = 0;
    /*
     * Annotate the root node with the relevant data
     * NOTE: The prev_ph and prev_dr and cfi fields remain unchanged
     */
    cfi = BUFFER(node)->cfi;
    if (node->type == PRIMARY_INPUT) {
	prev_bl.rise = prev_bl.fall = 0.0;
	prev_dr.rise = prev_dr.fall = 0.0; /* This needs to be fixed !!!! */
	prev_ph = PHASE_NONINVERTING;
    } else if (BUFFER(node)->type == BUF){
	buf_g = buf_param->buf_array[best_g];
	BUFFER(node)->impl.buffer = buf_g; 
	prev_bl = buf_g->block;
	prev_dr = buf_g->drive;
	prev_ph = buf_g->phase;
	root_limit = buf_g->max_load;
	sp_implement_buffer_chain(network, node);
	BUFFER(node)->cfi = cfi;
    } else {
	/* Find number of gates in the class */
	root_gate = buf_get_gate_version(lib_gate_of(node), best_g);
	assert(root_gate != NIL(lib_gate_t));
	BUFFER(node)->impl.gate = root_gate; 
	sp_replace_lib_gate(node, lib_gate_of(node), root_gate);
	prev_dr = ((delay_pin_t *)(root_gate->delay_info[cfi]))->drive;
	prev_ph = ((delay_pin_t *)(root_gate->delay_info[cfi]))->phase;
	prev_bl = ((delay_pin_t *)(root_gate->delay_info[cfi]))->block;
	root_limit = delay_get_load_limit(root_gate->delay_info[cfi]);
    }

    /*
     * Now annotate (if required) the buffer "b" driving positive signals
     */
    cap_b = 0; req_b = large_req_time;
    if (new_b != NIL(node_t)){
	assert(use_noninv_buf ? 1 : (best_b >= 0 && new_B != NIL(node_t)));
	buf_b = buf_param->buf_array[best_b];
	BUFFER(new_b)->type = BUF;
	BUFFER(new_b)->impl.buffer = buf_b; 
	load = 0;
	for (i = 0; i < new_pos; i++){
	    index = old_pos+i;
	    load += fanouts[index].load;
	    MIN_DELAY(req_b, req_b, fanouts[index].req);
	}
	sp_subtract_delay(buf_b->phase, buf_b->block, buf_b->drive,
		load, &req_b);
	cap_b = buf_b->ip_load + auto_route;
	BUFFER(new_b)->req_time = req_b;
	BUFFER(new_b)->load = load;
	num_violations += (load > buf_b->max_load);
    }

    /*
     * Now annotate (if required) the buffer "B" driving negative signals
     * and maybe the gate "b" above
     */
    cap_B = 0; req_B = (use_noninv_buf ? large_req_time : req_b);
    if (new_B != NIL(node_t)){
	buf_B = buf_param->buf_array[best_B];
	BUFFER(new_B)->type = BUF;
	BUFFER(new_B)->impl.buffer = buf_B; 
	load = (use_noninv_buf ? 0 : cap_b);
	for (i = 0; i < new_neg; i++){
	    index = old_neg+i+(new_pos+old_pos);
	    load += fanouts[index].load;
	    MIN_DELAY(req_B, req_B, fanouts[index].req);
	}
	sp_subtract_delay(buf_B->phase, buf_B->block, buf_B->drive,
		load, &req_B);
	cap_B = buf_B->ip_load+auto_route;
	BUFFER(new_B)->load = load;
	BUFFER(new_B)->req_time = req_B;
	BUFFER(new_B)->prev_ph = prev_ph;
	BUFFER(new_B)->prev_dr = prev_dr;
	num_violations += (load > buf_B->max_load);
    }

    if (new_b != NIL(node_t)){
	if (use_noninv_buf){
	    BUFFER(new_b)->prev_dr = prev_dr;
	    BUFFER(new_b)->prev_ph = prev_ph;
	} else {
	    assert(new_B != NIL(node_t));
	    BUFFER(new_b)->prev_dr = buf_B->drive;
	    BUFFER(new_b)->prev_ph = buf_B->phase;
	}
    }
    /*
     * a part of the original signals were driven by the inverter "gI"
     * Put in the correct data regarding that 
     */
    cap_gI = 0; req_gI = neg_branch_req = large_req_time;
    if (node_inv != NIL(node_t)){
	buf_gI = buf_param->buf_array[best_gI];
	BUFFER(node_inv)->type = BUF;
	BUFFER(node_inv)->impl.buffer = buf_gI; 
	load = (use_noninv_buf ? cap_B : 0);
	for (i = 0; i < old_neg; i++){
	    index = i+(new_pos+old_pos);
	    load += fanouts[index].load;
	    MIN_DELAY(req_gI, req_gI, fanouts[index].req);
	}
	neg_branch_req = req_gI;
	if (use_noninv_buf){
	    MIN_DELAY(req_gI, req_gI, req_B);
	}
	sp_subtract_delay(buf_gI->phase, buf_gI->block, buf_gI->drive,
		load, &req_gI);
	cap_gI = buf_gI->ip_load+auto_route;
	BUFFER(node_inv)->load = load;
	BUFFER(node_inv)->req_time = req_gI;
	BUFFER(node_inv)->prev_ph = prev_ph;
	BUFFER(node_inv)->prev_dr = prev_dr;
	num_violations += (load > buf_gI->max_load);
    }

    /* Now compute the required time at the input of "node' */
    if (use_noninv_buf){
	pos_branch_req = large_req_time;
	MIN_DELAY(req_g, req_b, req_gI);
	load = cap_b + cap_gI;
    } else {
	pos_branch_req = req_gI;
	MIN_DELAY(req_g, req_B, req_gI);
	load = cap_B + cap_gI;
    }
    for (i = 0; i < old_pos; i++){
	load += fanouts[i].load;
	MIN_DELAY(req_g, req_g, fanouts[i].req);
	MIN_DELAY(pos_branch_req, pos_branch_req, fanouts[i].req);
    }
    sp_subtract_delay(prev_ph, prev_bl, prev_dr, load, &req_g);
    BUFFER(node)->load = load;
    BUFFER(node)->req_time = req_g;
    num_violations += (load > root_limit);

    /*
     * We also want to find the difference in the required time
     * at the input of the buffer "B" and the earliest required signal.
     * This tells us whether recurring on "new_B" as root will help
     * or not.... 
     * If margin < 0 then no need to recur further
     */
    if (use_noninv_buf){
	margin_pos->rise = pos_branch_req.rise - req_b.rise;
	margin_pos->fall = pos_branch_req.fall - req_b.fall;
	margin_neg->rise = neg_branch_req.rise - req_B.rise;
	margin_neg->fall = neg_branch_req.fall - req_B.fall;
    } else {
	margin_pos->rise = pos_branch_req.rise - req_B.rise;
	margin_pos->fall = pos_branch_req.fall - req_B.fall;
    }

    /*
     * Set the implementations to be the library gates 
     */
    if (node_inv != NIL(node_t)){
	sp_implement_buffer_chain(network, node_inv);
	BUFFER(node_inv)->cfi = 0;
    }
    if (new_b != NIL(node_t)){
	sp_implement_buffer_chain(network, new_b);
	BUFFER(new_b)->cfi = 0;
    }
    if (new_B != NIL(node_t)){
	sp_implement_buffer_chain(network, new_B);
	BUFFER(new_B)->cfi = 0;
    }

    if (buf_param->debug > 10){
	(void)fprintf(sisout,"Transformation 3 data ----\n");
	(void)fprintf(sisout,"\troot node %s driving %d -- t_r =%6.3f:%-6.3f\n",
	    sp_name_of_impl(node), old_pos, req_g.rise, req_g.fall); 
	(void)fprintf(sisout,"\tgI is %s driving %d -- t_r = %6.3f:%-6.3f\n",
	    sp_name_of_impl(node_inv), old_neg, req_gI.rise, req_gI.fall);
	(void)fprintf(sisout,"\tb is %s driving %d -- t_r = %6.3f:%-6.3f\n",
	    sp_name_of_impl(new_b), new_pos, req_b.rise, req_b.fall); 
	(void)fprintf(sisout,"\tB is %s driving %d -- t_r = %6.3f:%-6.3f\n",
	    sp_name_of_impl(new_B), new_neg, req_B.rise, req_B.fall);
	if (use_noninv_buf){
	    (void)fprintf(sisout,"\tAfter Trans3: margin_pos =  %6.3f:%-6.f margin_neg =  %6.3f:%-6.f\n",
		    margin_pos->rise, margin_pos->fall, margin_neg->rise,
		    margin_neg->fall);
	} else {
	    (void)fprintf(sisout,"\tAfter Trans3: margin =  %6.3f:%-6.3f\n",
		    margin_pos->rise, margin_pos->fall);
	}
	(void)fprintf(sisout,"NOTE: There are %d load_limit violations\n",
		num_violations);
    }
    return num_violations;
}

static delay_pin_t *
sp_get_versions_of_node(node, buf_param, num_gates, a_array)
node_t *node;
buf_global_t *buf_param;
int *num_gates;
double **a_array;
{
    char *dummy;
    int i, j, cfi;
    lsGen gate_gen;
    sp_buffer_t *small_buf;
    lib_gate_t *cur_root_gate;
    delay_model_t model = buf_param->model;
    delay_pin_t *pin_delay, *gate_array;

    /*
     * The array stores the possible implementations of the root node
     */
    if (node->type == PRIMARY_INPUT){
	*num_gates = 1;
	*a_array = ALLOC(double, *num_gates);
	gate_array = ALLOC(delay_pin_t, *num_gates);
	pin_delay = get_pin_delay(node, 0, model);
	gate_array[0] = *pin_delay;
	(*a_array)[0] = 0.0;
    } else if (BUFFER(node)->type == BUF){
	if (BUFFER(node)->impl.buffer->phase == PHASE_INVERTING){
	    *num_gates = buf_param->num_inv; j = 0;
	} else {
	    *num_gates = buf_param->num_buf - buf_param->num_inv;
	    j = buf_param->num_inv;
	}
	gate_array = ALLOC(delay_pin_t, *num_gates);
	*a_array = ALLOC(double, *num_gates);
	for (i = 0; i < *num_gates; i++, j++){
	    small_buf = buf_param->buf_array[j];
	    gate_array[i].phase = small_buf->phase; 
	    gate_array[i].drive = small_buf->drive;
	    gate_array[i].block = small_buf->block;
	    gate_array[i].load = small_buf->ip_load;
	    (*a_array)[i] = small_buf->area;
	}
    } else {
	/* Find number of gates in the class */
	*num_gates = 0;
	gate_gen = lib_gen_gates(lib_gate_class(lib_gate_of(node)));
	while (lsNext(gate_gen, &dummy, LS_NH) == LS_OK){
	    (*num_gates)++;
	}
	(void)lsFinish(gate_gen);
 	/* Get the delay/area data for each into the array */
	gate_gen = lib_gen_gates(lib_gate_class(lib_gate_of(node)));
	gate_array = ALLOC(delay_pin_t, *num_gates);
	*a_array = ALLOC(double, *num_gates);
	i = 0;
	cfi = BUFFER(node)->cfi;
	while (lsNext(gate_gen, &dummy, LS_NH) == LS_OK){
	    cur_root_gate = (lib_gate_t *)dummy;
	    gate_array[i] = *((delay_pin_t *)(cur_root_gate->delay_info[cfi]));
	    (*a_array)[i] = lib_gate_area(cur_root_gate);
	    i++;
	}
	(void)lsFinish(gate_gen);
    }
    return gate_array;
}

/*
 * During the recursuion, this routine returns 1 if the load constaraint
 * at the output of node is satisfied, 0 on violation
 */
int
buf_load_constraint_met(node)
node_t *node;
{
    int cfi;
    double limit, load;

    if (node == NIL(node_t)) return 1;

    load = BUFFER(node)->load;
    if (BUFFER(node)->type == GATE){
	cfi = BUFFER(node)->cfi;
	limit= delay_get_load_limit(BUFFER(node)->impl.gate->delay_info[cfi]);
    } else if (BUFFER(node)->type == BUF){
	limit = BUFFER(node)->impl.buffer->max_load;
    } else {
	return 1;
    }
    return (limit > load);
}
#undef EPS
