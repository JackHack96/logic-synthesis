
#include "sis.h"
#include "speed_int.h"
#include <math.h>

static void nsp_print_bdd();
static void sp_compute_improvement();
static void nsp_create_relevant_data();
static void nsp_eval_best_transform();
static void nsp_compute_achievable_saving();
static int nsp_trivial_mapped_network();
static int nsp_eval_selection();

#define NSP_BDD_LIMIT 100000
#define NSP_MAXSTR 256
#define ACCEPTABLE_FRACTION 0.2	     /* Fraction of predicted impr */
#define MIN_ACCEPTABLE_SAVING 0.01   /* Limit to quit binary search */

#define IS_ZERO(v) (ABS((v)) < NSP_EPSILON)

/* Some data structure to carry information about what scopes to generate */
struct create_flag_struct {
    int create_clp;
    int create_fanout;
    int create_dual;
};

static nsp_num_char; /* Records the number of characters printed on a line */
static array_t *fi_nodes;
static array_t *ip_slacks;

/*
 * Compute the area cost of applying a transformation. 
 * Only the transformations that save more than epsilon (determined) are
 * considered....
 */
void
new_speed_compute_weight(network,
			 speed_param, wght_table, epsilonp)
network_t *network;
speed_global_t *speed_param;
st_table *wght_table;	/* Returns information pertinent to the collapsing */
double *epsilonp;
{
    lsGen gen;
    int pass = 0;
    node_t *np, *node, *fanin;
    sp_weight_t *wght;
    struct create_flag_struct create_flag;
    int num_crit_po, need_another_pass;
    delay_time_t slack, min_slack, new_min_slack;
    double new_thresh, new_crit_slack, min_po_slack;
    double impr, sl, min_epsilon, max_epsilon, slack_diff;
    array_t *new_crit_po_array;
    st_table *new_crit_table;
    st_generator *stgen;

    *epsilonp = 0;

    new_crit_table = NIL(st_table);
    new_crit_po_array = NIL(array_t);
    /* Initialize the static arrays used in nsp_compute_achievable..() */
    fi_nodes = array_alloc(node_t *, 0);
    ip_slacks = array_alloc(double, 0);

    /* Set the types of networks that are to be created */
    create_flag.create_fanout = 
           (sp_num_local_trans_of_type(speed_param, FAN) > 0 &&
	   speed_param->model == DELAY_MODEL_MAPPED); 
    create_flag.create_dual = 
          (sp_num_local_trans_of_type(speed_param, DUAL) > 0);
    create_flag.create_clp = 
          (sp_num_local_trans_of_type(speed_param, CLP) > 0);
    min_po_slack = min_slack.rise = min_slack.fall = POS_LARGE;
    foreach_primary_output(network, gen, node){
	slack = delay_slack_time(node);
	min_slack.rise = MIN(min_slack.rise, slack.rise);
	min_slack.fall = MIN(min_slack.fall, slack.fall);
	min_po_slack = MIN(min_po_slack, MIN(slack.rise, slack.fall));
    }

    if (min_po_slack > NSP_EPSILON){
	/*
	 * Timing constraints are met: nothing needs to be done 
	 * Since the epsilon_p is zero,  new_speed_recur() will
	 * end without any further work
	 */
	return;
    }

    do {
	pass++;
 	if (new_crit_table != NIL(st_table)){
	    st_free_table(new_crit_table);
 	}
 	new_crit_table = st_init_table(st_ptrcmp, st_ptrhash);
	nsp_num_char = 0;

	if (speed_param->debug){
	    (void)fprintf(sisout, "\nPass %d: Computing weigths, crit_slack = %.2f\n",
			  pass, speed_param->crit_slack); 
	}

	/* Process all the critcal nodes not yet visited */
	foreach_node(network, gen, np) {
	    if (np->type != PRIMARY_OUTPUT) {
		if (speed_critical(np, speed_param) &&
		    !st_is_member(wght_table, (char *)np) ) {
		    wght = ALLOC(sp_weight_t, 1);
		    /* Generate scope for transformations */
		    nsp_create_relevant_data(network, np, speed_param,
					     wght, &create_flag);
		    st_insert(wght_table, (char *)np, (char *)wght);
		    st_insert(new_crit_table, (char *)np, (char *)wght);

		    /*
		     * Now apply transformations and compute the
		     * improvement that is possible at np 
		     */ 
		    nsp_eval_best_transform(np, wght, speed_param);
		}
	    }
	}

	if (speed_param->debug){
	    (void)fprintf(sisout, "\nPass %d: Computed weigthts of %d nodes\n",
			  pass, st_count(new_crit_table));
	}
	
	/*
	 * Now compute the savings that can be achieved.
	 */
	nsp_compute_achievable_saving(network, speed_param, wght_table);
	
	/*
	 * Now find the minimum guaranteed saving at the PO nodes
	 */
	min_epsilon = POS_LARGE;
	max_epsilon = NEG_LARGE;
	new_min_slack.rise = new_min_slack.fall = POS_LARGE;
	foreach_primary_output(network, gen, node){
	    slack = delay_slack_time(node);
	    if (speed_critical(node, speed_param)){
		fanin = node_get_fanin(node, 0);
		if (node_type(fanin) == INTERNAL){
		    assert(st_lookup(wght_table, (char *)fanin, (char **)&wght));
		    impr = wght->epsilon;
		} else {
		    /* PI is connected to the PO: no improvement possible */
		    impr = 0.0;
		}
		min_epsilon = MIN(min_epsilon, impr);
		max_epsilon = MAX(max_epsilon, impr);
		new_min_slack.rise = MIN(new_min_slack.rise,
					 (slack.rise + impr));
		new_min_slack.fall = MIN(new_min_slack.fall,
					 (slack.fall + impr));
		sl = MIN(slack.rise, slack.fall);
		if (speed_param->debug > 1){
		    (void)fprintf(sisout, "PO %s : new slack = %.2f + %.2f = %.2f\n",
				  node_name(node), sl, impr, sl+impr);
		}
	    }
	}
	new_crit_slack = MIN(new_min_slack.rise, new_min_slack.fall);
	new_thresh = new_crit_slack - min_po_slack;

	/* Adjust the new slack for floating point comparisons */
	new_min_slack.rise -= NSP_EPSILON;
	new_min_slack.fall -= NSP_EPSILON;
	if (speed_param->debug) {
	    (void)fprintf(sisout, "Guaranteed saving = %.2f\n", new_thresh);
	}
	new_thresh = MAX(0.0, new_thresh);
	
	/*
	 * If this improvement is acheived, then there are other outputs
	 * that would be more critical. Find these and modify the threshold
	 * appropriately
	 */
	if (new_crit_po_array != NIL(array_t)){
	    array_free(new_crit_po_array);
	}
	new_crit_po_array = array_alloc(node_t *, 0);
	num_crit_po = 0;
	foreach_primary_output(network, gen, node){
	    slack = delay_slack_time(node);
	    if (speed_critical(node, speed_param)){
		num_crit_po++;
	    } else if ((slack.rise < new_min_slack.rise ||
		        slack.fall < new_min_slack.fall)){
		array_insert_last(node_t *, new_crit_po_array, node);
		num_crit_po++;
	    }
	}
	if (speed_param->debug){
	    (void)fprintf(sisout, "%d (%d new) critical PO's: thresh = %.2f\n",
		       num_crit_po, array_n(new_crit_po_array), new_thresh);
	}

	/* Set the new threshold */
	*epsilonp = new_thresh;
	
	/*
	 * Need to make at most two passes, Second pass needed only if the
	 * new_thresh exceeds the original threshold used for the first pass 
	 */
	need_another_pass = ((pass < 2) && (new_thresh > speed_param->thresh));
	if (need_another_pass) {
	    speed_param->crit_slack = new_crit_slack;
	}
    } while (need_another_pass);
    
    if (speed_param->debug > 2){
	st_foreach_item(wght_table, stgen, (char **)&np, (char **)&wght){
 	    (void)fprintf(sisout, "At %s %d D=%.2f A=%.2f (da=%.2f oa=%.2f)\n",
			  np->name, wght->best_technique,
			  SP_IMPROVEMENT(wght), SP_COST(wght),
			  wght->dup_area, wght->orig_area);
	}
    }
    array_free(new_crit_po_array);
    st_free_table(new_crit_table);
    array_free(fi_nodes);
    array_free(ip_slacks);
    /*  Reset the caches that were used during local transformations */
    nsp_free_buf_param();
}

/*
 * Routine to generate the networks on which the local transformations are
 * carried out and some data that may be corrupted by subsequent delay
 * traces
 */
static void
nsp_create_relevant_data(network, np, speed_param, wght, flags)
network_t *network;
node_t *np;
speed_global_t *speed_param;
sp_weight_t *wght;
struct create_flag_struct *flags;
{
    lib_gate_t *gate;
    int sp_num_models = array_n(speed_param->local_trans);
    delay_pin_t *pin_delay;
    double auto_load;

    /* Initialize some data that is always required */
    wght->clp_netw = wght->fanout_netw = wght->dual_netw = wght->best_netw
        = NIL(network_t); 
    wght->best_technique = -1;
    wght->select_flag = FALSE;
    wght->improvement = NIL(double);
    wght->area_cost = NIL(double);
    wght->dup_area = 0.0;
    wght->orig_area = 0.0;

#ifdef SIS
    /* Do not consider the internal nodes that are part of latches */
    if ((gate = lib_gate_of(np)) != NIL(lib_gate_t) &&
	lib_gate_type(gate) != COMBINATIONAL) {
	return;
    }
#endif /* SIS */
    
    /* For the case of TREE based selection strategy, we restrict the
       optimizations only to the root of the trees. By doing this, the
       computation cost of the transforms is reduced. The restriction
       is carried out by making the stored network == NIL() */
    if (speed_param->region_flag == ONLY_TREE){
	if (node_num_fanout(np) <= 1 && !new_speed_is_fanout_po(np)){
	    return;
	}
    }

    /* Now generate the relevant networks */
    if (flags->create_clp){
	wght->clp_netw = sp_get_network_to_collapse(np, speed_param,
						    &(wght->dup_area));
    }
    auto_load = delay_get_default_parameter(network, DELAY_WIRE_LOAD_SLOPE);
    auto_load = (auto_load == DELAY_NOT_SET ? 0.0 : auto_load);
    if (flags->create_fanout){
	wght->fanout_netw = 
	buf_get_fanout_network(np, speed_param);
	wght->cfi = -1;
	wght->cfi = sp_buf_get_crit_fanin(np, speed_param->model);
	if (np->type == INTERNAL){
	    pin_delay = get_pin_delay(np, wght->cfi, speed_param->model);
	    wght->cfi_load = pin_delay->load + auto_load;
	} else {
	    wght->cfi_load = delay_load(np);
	}	
	delay_wire_required_time(np, wght->cfi, speed_param->model,
				 &wght->cfi_req);  
    }
    if (flags->create_dual){
	wght->dual_netw = nsp_get_dual_network(np, speed_param);
    }
    
    wght->orig_area = sp_get_netw_area(wght->clp_netw);
    wght->arrival_time = delay_arrival_time(np);
    wght->slack = delay_slack_time(np);
    wght->load = delay_load(np);
    wght->crit_slack = speed_param->crit_slack;
    
    wght->can_invert = 1 - new_speed_is_fanout_po(np);
    wght->improvement = ALLOC(double, sp_num_models);
    wght->area_cost = ALLOC(double, sp_num_models);

    return;
}

/* 
 * Evaluate the local transformations on the appropriate scopes and
 * determine the best transform. Also, remove the networks that are
 * not required (since the best transform does not use them) to save space 
 */
static void
nsp_eval_best_transform(np, wght, speed_param)
node_t *np;
sp_weight_t *wght;
speed_global_t *speed_param;
{
    int i, j;
    double best_imp, cur_impr, best_area, cur_area;
    int sp_num_models = array_n(speed_param->local_trans);
    network_t *opt_netw;
    sp_xform_t *local_trans;
	
    best_imp = NEG_LARGE;
    for(i = 0; i < sp_num_models; i++){
	local_trans = sp_local_trans_from_index(speed_param, i);
	if (local_trans->on_flag){
	    j = speed_param->debug;
	    speed_param->debug = FALSE;
	    opt_netw = NSP_OPTIMIZE(wght, local_trans, speed_param);
	    speed_param->debug = j;
	    
	    if (opt_netw == NIL(network_t)) {
		cur_area = POS_LARGE;
		cur_impr = NEG_LARGE;
	    } else {
		sp_compute_improvement(np, opt_netw, wght, speed_param,
				       i, local_trans);
		
		/* Accept the first  or one that betters the previous*/
		cur_area = wght->area_cost[i];
		cur_impr = wght->improvement[i];

		if (best_imp == NEG_LARGE) {
		    best_imp = cur_impr; best_area = cur_area;
		    wght->best_technique = i;
		} else if (IS_ZERO(cur_impr-best_imp)){
		    /* Select the technique with the smaller area penalty*/
		    if (cur_area < best_area){
			wght->best_technique = i;
			best_area = cur_area;
		    }
		} else if (cur_impr > NSP_EPSILON){
		    if (speed_param->transform_flag == BEST_BENEFIT) {
			if (cur_impr > best_imp){
			    best_imp = cur_impr; best_area = cur_area;
			    wght->best_technique = i;
			}
		    } else {
			/* Use the BENEFIT/COST metric for selection */
			if (best_imp < NSP_EPSILON){
			    best_imp = cur_impr; best_area = cur_area;
			    wght->best_technique = i;
			} else if (!IS_ZERO(cur_area) && !IS_ZERO(best_area)) {
			    if ((cur_area < 0.0 && best_area > 0.0) ||
				(cur_area > 0.0 && best_area > 0.0 &&
				 (cur_impr/cur_area) > (best_imp/best_area)) ||
				(cur_area < 0.0 && best_area < 0.0 &&
				 (cur_impr/cur_area) < (best_imp/best_area))) {
				best_imp = cur_impr; best_area = cur_area;
				wght->best_technique = i;
			    }
			} else if ((IS_ZERO(cur_area) && best_area > 0.0) ||
				   (IS_ZERO(best_area) && cur_area < 0.0)){
			    best_imp = cur_impr; best_area = cur_area;
			    wght->best_technique = i;
			}
		    }
		}
	    }
	    
	    if(speed_param->debug){
		fputc((cur_impr > 0 ? '+' : '-'), sisout);
		(void)fflush(sisout);
		nsp_num_char++;
	    }
	    sp_network_free(opt_netw);
	}
    }
    if (speed_param->debug){
	fputc('|', sisout); nsp_num_char++;
	if (nsp_num_char > 70){
	    fputc('\n', sisout);
	    nsp_num_char = 0;
	}
    }
    if (speed_param->debug > 1){
	(void)fprintf(sisout, "At %s (%d) D=%.2f A=%.2f (da=%.2f oa=%.2f)\n",
		      np->name, wght->best_technique,
		      SP_IMPROVEMENT(wght), SP_COST(wght),
		      wght->dup_area, wght->orig_area);
    }
    /* If there is no improvement set the stuff appropriately */
    if (best_imp < NSP_EPSILON){
	wght->best_technique = -1;
	/* Delete the networks --- should never need them */
	sp_network_free(wght->clp_netw);
	sp_network_free(wght->fanout_netw);
    }
    /* Also remove the networks that are not required */
    if (wght->best_technique >= 0 && SP_IMPROVEMENT(wght) > NSP_EPSILON){
	local_trans = sp_local_trans_from_index(speed_param,
						wght->best_technique); 
	if (local_trans->type != CLP) sp_network_free(wght->clp_netw);
	if (local_trans->type != FAN) sp_network_free(wght->fanout_netw);
	if (local_trans->type != DUAL) sp_network_free(wght->dual_netw);
    }
    return;
}
/*
 * The network has been optimized according to the "local_trans->optimize_func"
 * Use the appropriate delay_model to compute the arrival time of the optimized
 * network and compute the delay saving and the area penalty, The current
 * technique is numbered "num"....
 *
 * Assume that the delay trace has been done by the optimization routine
 *
 */

static void
sp_compute_improvement(node, opt_netw, wght, speed_param, num,
		       local_trans)
node_t *node;                /* The node at which the transform is applied */
network_t *opt_netw;	     /* The optimized network */
sp_weight_t *wght;	     /* The structure stored with "node" */
speed_global_t *speed_param; /* GLobals */
int num;		     /* Index of the transformation !! */
sp_xform_t *local_trans;     /* The local transform structure */
{
    node_t *root, *fanin, *po;
    delay_pin_t *pin_delay;
    double impr, load_diff, auto_load;
    delay_time_t t, drive, po_impr, po_arr, pi_req;

    if (opt_netw == NIL(network_t)){ /* Ignore it !!! Should not be called */
	return;
    }

    auto_load = delay_get_default_parameter(node_network(node),
					    DELAY_WIRE_LOAD_SLOPE);
    auto_load = (auto_load == DELAY_NOT_SET ? 0.0 : auto_load);

    impr = 0.0 - NSP_EPSILON; /* By default no improvement possible */

    if (local_trans->type == FAN){
	/*
	 * Get the change in required time at input of root node
	 * The node under consideration may be a PI, so be careful 
	 */
	if (node->type == PRIMARY_INPUT){
	    fanin = network_get_pi(opt_netw, 0);
	    delay_wire_required_time(fanin, 0, speed_param->model, &pi_req);
	    load_diff = delay_compute_fo_load(fanin, speed_param->model) - wght->cfi_load;
	} else {
	    root = node_get_fanout(network_get_pi(opt_netw, 0), 0);
	    fanin = node_get_fanin(root, wght->cfi);
	    delay_wire_required_time(root, wght->cfi, speed_param->model, &pi_req);
	    pin_delay = get_pin_delay(root, wght->cfi, speed_param->model);
	    load_diff = (auto_load + pin_delay->load) - wght->cfi_load;
	}
	/*
	 * Adjust the required time based on the difference in loading 
	 */
	assert(delay_get_pi_drive(fanin, &drive));
	pi_req.rise -= (drive.rise * load_diff);
	pi_req.fall -= (drive.fall * load_diff);
	impr = MIN((pi_req.rise - wght->cfi_req.rise),
		   (pi_req.fall - wght->cfi_req.fall));
    } else if (local_trans->type == DUAL) {
	/* Check the improvement in the slack of the network */
	fail("Need to find the slack impr on dualizing\n");
    } else {
	/* 
	 * Determine if something sensible has been done ---
	 * In case the nodes were part of a fanout tree the "mapped"
	 * transform is not the right one (since it is the required time that
	 * is of significance) so set the improvement to be zero !!!
	 */
	if (nsp_trivial_mapped_network(opt_netw)){
	    impr = -1.0; /* set < 0 just to be sure it is not considered */
	} else {
	/* Find out the decrease in arrival time at o/p of node */
	    po = network_get_po(opt_netw, 0);
	    po_arr = (*local_trans->arr_func)(po, speed_param);
	    root = node_get_fanin(po, 0);
	    if (node_num_fanin(root) == 1 &&  wght->can_invert && 
		speed_param->model != DELAY_MODEL_MAPPED){
		t = nsp_compute_delay_saving(po, root, speed_param->model);
		po_arr.rise -= t.rise;
		po_arr.fall -= t.fall;
	    }
	    po_impr.rise = wght->arrival_time.rise - po_arr.rise;
	    po_impr.fall = wght->arrival_time.fall - po_arr.fall;
	    impr = MIN(po_impr.rise, po_impr.fall);
	}
    }
    /*
     * Ignoring interaction with other cutset members --- this is the
     * amount of saving that is the result of this transformation
     */
    wght->improvement[num] = impr;

    /* 
     * Compute the area penalty that results from applying the transformation
     */
    if (local_trans->type == FAN){
	wght->area_cost[num] = sp_get_netw_area(opt_netw);
    } else {
	wght->area_cost[num] = (sp_get_netw_area(opt_netw)
				+ wght->dup_area) - wght->orig_area;
    }
}

/* 
 * Routine to compute the saving that can be achieved at a node (the delta
 * values). Assumes that the computation of local improvement has been made
 */
static void 
nsp_compute_achievable_saving(network, speed_param, wght_table)
network_t *network;
speed_global_t *speed_param;
st_table *wght_table;
{
    int i, j;
    array_t *nodevec;
    node_t *node, *fanin;
    double local_impr, inp_eps, min_ip_slack, sl;
    delay_time_t req, arr, slack;
    sp_weight_t *wght, *ip_wght;
    int all_fanins_pi;
    int crit_ip_count; /* Number of fanins of a node that are critical */
    
    nodevec = network_dfs(network);
    for (i = 0; i < array_n(nodevec); i++){
	node = array_fetch(node_t *, nodevec, i);
	if (st_lookup(wght_table, (char *)node, (char **)&wght)){
	    inp_eps = POS_LARGE;
	    all_fanins_pi = TRUE;
	    if (speed_param->debug > 1) {
		(void)fprintf(sisout, "EPS %s (%s) --",
			      node_name(node), node_long_name(node));
	    }
	    min_ip_slack = POS_LARGE;
	    crit_ip_count = 0;
	    foreach_fanin(node, j, fanin){
		if (st_lookup(wght_table, (char *)fanin, (char **)&ip_wght)){
		    
		    assert(delay_wire_required_time(node, j, speed_param->model, &req));
		    arr = delay_arrival_time(fanin);
		    slack.rise = req.rise - arr.rise;
		    slack.fall = req.fall - arr.fall;
		    sl = MIN(slack.rise, slack.fall);
		    if (sl < speed_param->crit_slack - NSP_EPSILON){
			min_ip_slack = MIN(min_ip_slack, sl);
			all_fanins_pi = FALSE;
			array_insert(double, ip_slacks, crit_ip_count, sl);
			array_insert(node_t *, fi_nodes, crit_ip_count, fanin);
			crit_ip_count++;
		    } 
		    
		}
	    }
	    for (j = 0; j < crit_ip_count; j++){
		sl = array_fetch(double, ip_slacks, j);
		fanin = array_fetch(node_t *, fi_nodes, j);
		st_lookup(wght_table, (char *)fanin, (char **)&ip_wght);
		
		inp_eps = MIN(inp_eps,
			      (ip_wght->epsilon + sl - min_ip_slack));
		if (speed_param->debug > 1){
		    (void)fprintf(sisout," (%5.2f+%5.2f)%s",
				  ip_wght->epsilon, sl - min_ip_slack,
				  fanin->name);
		}
	    }
	    
	    if (all_fanins_pi){
		inp_eps = 0.0;
	    } else if (inp_eps >= POS_LARGE){
		fail("error in the computation of improvement\n");
	    }
	    local_impr = SP_IMPROVEMENT(wght);
	    if (local_impr >= inp_eps){
		wght->epsilon = local_impr;
		wght->select_flag = TRUE;
	    } else {
		wght->epsilon = inp_eps;
	    }
	    if (speed_param->debug > 1){
		(void)fprintf(sisout, "  AND (%.2f) = %5.2f\n",
			      local_impr, wght->epsilon);
	    }
	}
    }
    array_free(nodevec);
}
/* 
 * Determine if the network is the result of collapsing just chains of
 * inverters
 */
static int 
nsp_trivial_mapped_network(network)
network_t *network;
{
    lsGen gen;
    node_t *node;
    int is_trivial = TRUE;

    if (network_num_internal(network) == 0) return 1;
    
    foreach_node(network, gen, node){
	if (node->type == INTERNAL && node_num_fanin(node) > 1){
	    is_trivial = FALSE;
	    lsFinish(gen);
	    break;
	}
    }
    return is_trivial;
}
/* Meant for specific use of printing bdd's generated during the
 * procedures for selection of sites to apply local transformations
 */
static void
nsp_print_bdd(f, nodevec, num_lit)
bdd_t *f;
array_t *nodevec;
{
    char *name;
    int i, index;
    node_t *temp;
    network_t *pr_net;
    array_t *names_array;

    names_array = array_alloc(char *, num_lit);
    index = 0;
    for (i = array_n(nodevec); i-- > 0; ){
	temp = array_fetch(node_t *, nodevec, i);
	if (temp->type != PRIMARY_OUTPUT) {
	    array_insert(char *, names_array, index, util_strsav(temp->name));
	    index++;
	}
    }
    pr_net = ntbdd_bdd_single_to_network(f, "bdd_out", names_array);
    com_execute(&pr_net, "collapse");
    com_execute(&pr_net, "simplify");
    com_execute(&pr_net, "print");
    network_free(pr_net);
    for (i = array_n(names_array); i-- > 0; ){
	name = array_fetch(char *, names_array, i);
	FREE(name);
    }
    array_free(names_array);
}

 /* Compute the difference between the most critical slack and the next 
  * Returns 0 in case there is no valid slack difference */
int
nsp_first_slack_diff(network, slack_diff)
network_t *network;
double *slack_diff;
{
    lsGen gen;
    node_t *node;
    delay_time_t slack, min_slack, diff;

    *slack_diff = 0.0;
    if (network_num_po(network) == 1) return 0;

    min_slack.rise = min_slack.fall = POS_LARGE;
    foreach_primary_output(network, gen, node){
	slack = delay_slack_time(node);
	min_slack.rise = MIN(min_slack.rise, slack.rise);
	min_slack.fall = MIN(min_slack.fall, slack.fall);
    }
    diff.rise = diff.fall = POS_LARGE;
    foreach_primary_output(network, gen, node){
	slack = delay_slack_time(node);
	if (slack.rise > (min_slack.rise + NSP_EPSILON)){
	    diff.rise = MIN(diff.rise, slack.rise - min_slack.rise);
	}
	if (slack.fall > (min_slack.fall + NSP_EPSILON)){
	    diff.fall = MIN(diff.fall, slack.fall - min_slack.fall);
	}
    }
    if (diff.rise == POS_LARGE || diff.fall == POS_LARGE){
	return 0;
    } else {
	*slack_diff = MIN(diff.rise, diff.fall);
	return 1;
    }
}

/*
 * Returns 1 if the edge between node and fanin is critical
 */
int
nsp_critical_edge(node, fanin_index, model, crit_slack)
node_t *node;
int fanin_index;
delay_model_t model;
double crit_slack;
{
    node_t *fanin;
    delay_time_t req, arr, slack;

    if (!delay_wire_required_time(node, fanin_index, model, &req)){
	return 0;
    }
    fanin = node_get_fanin(node, fanin_index);
    arr = delay_arrival_time(fanin);
    slack.rise = req.rise - arr.rise;
    slack.fall = req.fall - arr.fall;
    if (slack.rise < (crit_slack - NSP_EPSILON) ||
	slack.fall < (crit_slack - NSP_EPSILON)){
	return 1;
    } 
    return 0;
}


 /*
  * Routine to compute the shortest path (Modification on the routine
  * mvbr_shortestpath() by Yoshinori Watanabe (in GYOCRO). All the comments
  * are Yoshi's ....
  */
static void nsp_dfs_bdd();
static int get_post_visit();
static void set_post_visit();
static int is_f_visited();
static enum st_retval free_starray();
static enum st_retval free_stbddt();

/* 
 * nsp_best_selection() finds the shortest path in a BDD f that
 * ends with a 1 terminal, where an edge has a weight 'wtt' if it
 * is a then edge and has a weight 'wte' if it is an else edge.
 * This routine does not check if f is NIL or is a constant
 * terminal BDD.  This routine should not be used for these
 * cases.
 */ 

array_t *
nsp_best_selection(f, node_and_id, wtt, wte, cost)
bdd_t *f;
st_table *node_and_id;
int *wtt; /* Array of weigths for the then edges for each variable */
int  wte; /* The weight of the "then" edges */
int *cost;
{
    st_table *id_table, *order;
    int num_nodes, i, k;
    int *length, *phase;
    bdd_t **parent, *v, *w0, *w1, *dup_f;
    char *value, *id;
    node_t *np;
    int then_weight;
    array_t *solution_array;
    unsigned int var_id;
    int terminal1_id;

    *cost = 0;

    solution_array = array_alloc(node_t *, 0);
    if(bdd_is_tautology(f, 0)){
	array_free(solution_array);
	return NIL(array_t);
    } else if (bdd_is_tautology(f, 1)){
	return solution_array;
    } 
    /* id_table gives a post_visit number of a sub-BDD g of f.
     * Specifically, as the key being the bdd_node of g, the table
     * contains an array[0, 1] of integers, where array[0] gives
     * the post visit number of g with complemented edge and array[1]
     * gives the one for the non-complemented edge.  In either case,
     * if only one of them appears as a sub-BDD in f, then the number
     * for the other is set to -1.
     */
    id_table = st_init_table(st_ptrcmp, st_ptrhash);
    /* order contains a single sub-BDD 'g' with its post_visit number
     * as the key.
     */
    order = st_init_table(st_numcmp, st_numhash);

    /* fill in id_table and order.  We duplicate f so that we can
     * free up the BDD's stored in order at the end.
     */
    dup_f = bdd_dup(f);
    nsp_dfs_bdd(dup_f, id_table, order, 0, &num_nodes);

    /* num_nodes is the number of nodes visited in the dfs. */
    length = ALLOC(int, num_nodes);
    parent = ALLOC(bdd_t *, num_nodes);
    phase = ALLOC(int, num_nodes);

    /* length[i] gives the length of the shortest path of the
     * BDD node with the post_visit number i from the top node of f.
     * parent[i] gives the parent BDD of the BDD node with
     * the post_visit number i in the path.  phase[i] is 1 if the
     * BDD node with  id = i is a then child of parent[i] and
     * 0 if an else child.  terminal is an array of post_visit number
     * of BDDs of 1 terminal nodes in f.
     */
    for(i=0; i<num_nodes-1; i++){
	length[i] = INFINITY;
    }
    length[i] = 0;
    parent[i] = NIL(bdd_t);

    /* Each sub-BDD in f visited in DFS is processed in topological
     * order, which is the reverse post_visit order.
     * The first node to be processed is f.
     */
    for(i=num_nodes-1; i>=0; i--){
	(void)st_lookup(order, (char *)i, &value);
	v = (bdd_t *)value;

	/* Skip if v is a leaf. */
	if(bdd_is_tautology(v, 0) || bdd_is_tautology(v, 1))  continue;

	/* Labeling the then child */
	w1 = bdd_then(v);
	if(bdd_is_tautology(w1, 0) == 0){  /* Skip if w is 0 terminal. */
	    k = get_post_visit(id_table, w1);
	    var_id = bdd_top_var_id(v);
	    then_weight = wtt[var_id];
	    if(length[k] > (then_weight+length[i])){
		length[k] = then_weight+length[i];
		parent[k] = v;
		phase[k] = 1;
	    }
	    if(bdd_is_tautology(w1, 1))  terminal1_id = k;
	}
	bdd_free(w1);

	/* Labeling the else child */
	w0 = bdd_else(v);
	if(bdd_is_tautology(w0, 0) == 0){
	    k = get_post_visit(id_table, w0);
	    if(length[k] > (wte+length[i])){
		length[k] = wte + length[i];
		parent[k] = v;
		phase[k] = 0;
	    } 
	    if(bdd_is_tautology(w0, 1))  terminal1_id = k;
	}
	bdd_free(w0);
    }

    /* At this point, the sub_BDD of 1 terminal stored in order
     * has the shortest path to the root f using parent[].
     */
    k = terminal1_id;
    (void)st_lookup(order, (char *)k, &value);
    v = (bdd_t *)value;

    /* Now follow the reverse pointers */
    while(parent[k] != NIL(bdd_t)){
	v = parent[k];
	var_id = bdd_top_var_id(v);
	if (phase[k] == 1){
	    assert(st_lookup(node_and_id, (char *)var_id, (char **)&np));
	    array_insert_last(node_t *, solution_array, np);
	    *cost += wtt[var_id];
	}
	k = get_post_visit(id_table, v);
    }

    /* cleanup */
    FREE(length); FREE(phase);  FREE(parent);
    st_foreach(id_table, free_starray, NIL(char));
    st_foreach(order, free_stbddt, NIL(char));
    st_free_table(id_table);  st_free_table(order);

    return solution_array;
}


/* 
 * nsp_dfs_bdd() recursively performs a depth first search over nodes
 * in BDD 'f' and assigns post_visit numbers for all the nodes except
 * 0 terminal nodes.  The post_visit number is a number assigned to a
 * node after all of its children have been visited.  The number
 * starts with 'start_num'.  Once a node that is not a 0 terminal is
 * assigned a post_visit number, then the number is set in a hash_table
 * 'id_table' with key as the pointer to the node.  Also, the pointer
 * to the node is stored in a hash_table 'order' with key as its post_
 * visit number. *total_num_ptr is returned as the number of the
 * nodes in f except the 0 terminal nodes plus start_num.
 */
static void
nsp_dfs_bdd(f, id_table, order, start_num, total_num_ptr)
bdd_t *f;
st_table *id_table, *order;
int start_num, *total_num_ptr;
{
    int total; 
    bdd_t *v0, *v1;

    if(is_f_visited(id_table, f)){
	*total_num_ptr = start_num;
	return;
    }
    if(bdd_is_tautology(f, 0)){
	*total_num_ptr = start_num;
	return;
    }
    if(bdd_is_tautology(f, 1)){
	set_post_visit(id_table, f, start_num);
	(void)st_insert(order, (char *)start_num, (char *)f);
	*total_num_ptr = start_num + 1;
	return;
    }

    v1 = bdd_then(f);
    nsp_dfs_bdd(v1, id_table, order, start_num, &total);

    start_num = total;
    v0 = bdd_else(f);
    nsp_dfs_bdd(v0, id_table, order, start_num, &total);

    /* The post_visit number of f is total */
    set_post_visit(id_table, f, total);
    (void)st_insert(order, (char *)total, (char *)f);
    *total_num_ptr = total + 1;

    return;
}

static enum st_retval
free_starray(key, value, arg)
char *key, *value, *arg;
{
    FREE(value);
    return ST_DELETE;
}

static enum st_retval
free_stbddt(key, value, arg)
char *key, *value, *arg;
{
    bdd_t *b;

    b = (bdd_t *)value;
    bdd_free(b);
    return ST_DELETE;
}

static void
set_post_visit(id_table, f, n)
st_table *id_table;
bdd_t *f;
int n;
{
    int complement, index, *A;
    bdd_node *F;
    char *value;

    F = bdd_get_node(f, &complement);
    index = (complement) ? 0 : 1;
    if(st_lookup(id_table, (char *)F, &value)){
	A = (int *)value;
	if(A[index] != -1)  A[index] = n;
    }
    else{
	A = ALLOC(int, 2);
	A[0] = A[1] = -1;
	A[index] = n;
	(void)st_insert(id_table, (char *)F, (char *)A);
    }
}

static int
get_post_visit(id_table, f)
st_table *id_table;
bdd_t *f;
{
    int complement, index, *A;
    bdd_node *F;
    char *value;

    F = bdd_get_node(f, &complement);
    index = (complement) ? 0 : 1;
    (void)st_lookup(id_table, (char *)F, &value);
    A = (int *)value;
    return A[index];
}

static int
is_f_visited(id_table, f)
st_table *id_table;
bdd_t *f;
{
    int complement, index, *A;
    bdd_node *F;
    char *value;

    F = bdd_get_node(f, &complement);
    index = (complement) ? 0 : 1;
    if(st_lookup(id_table, (char *)F, &value)){
	A = (int *)value;
	if(A[index] != -1) return 1;
    }
    return 0;
}



typedef struct valid_data_struct {
    int select;		    /* set if selection increases the epsilon */
    double saving;	    /* Amount of saving desired at this node */
    bdd_t *func;	    /* The partial "valid_set" func  upto this node */
    int num_fi;	            /* The number of fanins that are traversed */
    array_t *fi;	    /* The fanins that need to be traversed */
} vdata_t;

/*
 * Select the transformations to apply on the given circuit.
 * Generates the vaild_set_functions() directly for every output.
 */
array_t *
new_speed_select_xform(network_in, speed_param, clp_table, target_impr)
network_t	*network_in;
speed_global_t  *speed_param;
st_table *clp_table;  /* Table that lists the improvement possible at nodes */
double target_impr;   /* The improvement that we promised we could deliver */
{
    lsGen gen;
    int i, j, k, index, sel_length, cost, area_cost, max_area_cost;
    int first, last, num_tries, crit_ip_count, more_to_come;
    node_t *node, *np, *temp, *fanin;
    bdd_manager *manager;
    st_generator *stgen;
    int neg_impr, found_good_set;
    int end_of_prop;            /* Flag for termination of propagation */
    int num_lit, area, *cost_array;   /* Array of costs for BDD variable */
    array_t *sol_array;    /* The set of transforms selected !!! */
    st_table *node_to_bdd, *node_and_id, *visited;
    vdata_t *valid_data, *vdata, *ip_vdata;
    st_table *valid_table;
    sp_weight_t *wght, *ip_wght;
    double sl, ip_sav, offset, *saving_array;
    double min_slack, thresh, target, min_ip_slack;
    delay_time_t slack, arr, req;
    delay_model_t model = speed_param->model;
    bdd_t *ip_func, *var, *c, *f, *f_prime, *offending_set, *bad_f;
    array_t *nodevec, *temp_array, *topo_order, *fi_nodes, *ip_slacks;
    array_t *init_sol = NIL(array_t), *best_sol = NIL(array_t);

    /* Initialization */
    num_lit = network_num_internal(network_in) + network_num_pi(network_in);
    
    /* Form a hash table from a node pointer to a row index */
    index = 0;
    manager = bdd_start(num_lit);
    node_to_bdd = st_init_table(st_ptrcmp, st_ptrhash);
    node_and_id = st_init_table(st_ptrcmp, st_ptrhash);
    
    /* Create the required number of variables: outputs ordered last */
    min_slack = POS_LARGE;
    nodevec = network_dfs_from_input(network_in);
    for (i = array_n(nodevec); i-- > 0; ){
	node = array_fetch(node_t *, nodevec, i);
	if (node->type != PRIMARY_OUTPUT) {
	    (void)st_insert(node_and_id, (char *)node, (char *)index);
	    (void)st_insert(node_and_id, (char *)index, (char *)node);
	    var = bdd_get_variable(manager, index);
	    (void)st_insert(node_to_bdd, (char *)node, (char *)var);
	    index++;
	} else {
	    slack = delay_slack_time(node);
	    min_slack = MIN(min_slack, MIN(slack.rise, slack.fall));
	}
    }
    
    /* Allocate the structures used to compute the "valid_set" functions */
    valid_data = ALLOC(vdata_t, num_lit);
    for (i = 0; i < num_lit; i++){
	valid_data[i].saving = NEG_LARGE;
	valid_data[i].func = NIL(bdd_t);
	valid_data[i].select = FALSE;
	valid_data[i].fi = array_alloc(node_t *, 0);
    }
    
    /* Generate the cost array used in finding best selection set */
    cost_array = ALLOC(int, num_lit);
    saving_array = ALLOC(double, num_lit);
    for (i = num_lit; i-- > 0; cost_array[i] = POS_LARGE); /* Initialization */
    for (i = num_lit; i-- > 0; saving_array[i] = NEG_LARGE);
    max_area_cost = NEG_LARGE;
    st_foreach_item(clp_table, stgen, (char **)&np, (char **)&wght){
	area_cost = (int)ceil(SP_COST(wght));
	max_area_cost = MAX(area_cost, max_area_cost);
    }
    
    st_foreach_item(clp_table, stgen, (char **)&np, (char **)&wght){
	assert(st_lookup(node_and_id, (char *)np, (char **)&j));
	/* Node with variableID = j has a weight of the transform */
	area_cost = (int)ceil(SP_COST(wght));
	if (speed_param->objective == TRANSFORM_BASED){
	    /* Choose the fewest number of transformations: Ties broken
	     * based on the choice that has smaller area increase
	     */
	    cost_array[j] = area_cost + max_area_cost * num_lit;
	} else {
	    /* Choose the set that requires min area increase: Ties broken
	     * by selection the choice that involves fewer transformations
	     */
	    cost_array[j] = 1 + num_lit * area_cost;
	}
    }
    
    /* Some temporary storage required during traversal of critical fanins */
    fi_nodes = array_alloc(node_t *, 0);
    ip_slacks = array_alloc(double, 0);

    thresh = target_impr;
    bad_f = bdd_zero(manager); /* Set of unacceptable selections */
    
    do {
	/*
	 * Initialization of the selection fn and nodes that are visited
	 * for this choice of threshold
	 */
	f = bdd_one(manager);
	visited = st_init_table(st_ptrcmp, st_ptrhash);

	/* Visit all the critical outputs and generate selection functions */
	foreach_primary_output(network_in, gen, node) {
	    np = node_get_fanin(node, 0);	/* Get the internal node */
	    if (!speed_critical(node, speed_param) ||
		st_is_member(visited, (char *)np) ) {
		continue; /* not critical */
	    }
	    (void)st_insert(visited, (char *)np, NIL(char));
	    /* 
	     * First determine the amount of saving at the PO 
	     */
	    slack = delay_slack_time(node);
	    target = (min_slack+thresh) - MIN(slack.rise,slack.fall) - NSP_EPSILON;
	    if (target < NSP_EPSILON){
		if (speed_param->debug > 1){
		    (void)fprintf(sisout, "Ignoring PO %s\n", node->name);
		}
		continue;
	    }
	    if (speed_param->debug > 1){
		(void)fprintf(sisout, "Target improvement for %s = %.2f\n", 
			      node->name, target);
	    }
	    
	    /*
	     * Find the nodes in topo order that are valid for this PO...
	     */
	    temp_array = array_alloc(node_t *, 0);
	    array_insert_last(node_t *, temp_array, np);
	    valid_table = st_init_table(st_ptrcmp, st_ptrhash);
	    assert(st_lookup(node_and_id, (char *)np, (char **)&index));
	    valid_data[index].saving = target;
	    (void)st_insert(valid_table, (char *)np, (char *)(valid_data+index));
	    
	    /* BFS traversal of the critical fanin subnetwork */
	    first = 0; more_to_come = TRUE;
	    while (more_to_come) {
		more_to_come = FALSE;
		last = array_n(temp_array);
		for (i = first; i < last; i++){
		    temp = array_fetch(node_t *, temp_array, i);
		    foreach_fanin(temp, j, fanin) {
			if (/* fanin->type == INTERNAL && */ /* PI also possible*/
			    !st_is_member(valid_table, (char *)fanin) &&
			    st_is_member(clp_table, (char *)fanin) &&
			    nsp_critical_edge(temp, j, model, speed_param->crit_slack)){
			    /* Fanin lies on the critical region... */
			    assert(st_lookup(node_and_id, (char *)fanin, (char **)&index));
			    (void)st_insert(valid_table, (char *)fanin,
					    (char *)(valid_data+index));
			    array_insert_last(node_t *, temp_array, fanin);
			    more_to_come = TRUE;
			}
		    }
		}
		first = last;
	    }
	    
	    /* Combine nodes in the critical fanin with transitive order */
	    topo_order = array_alloc(node_t *, 0);
	    for (i = 0; i < array_n(nodevec); i++){
		temp = array_fetch(node_t *, nodevec, i);
		if (st_is_member(valid_table, (char *)temp)){
		    array_insert_last(node_t *, topo_order, temp);
		}
	    }
	    /*
	     * Starting from the PO, compute the saving that is desired at each
	     * node. Also determine which fanins need to be traversed
	     */
	    for ( i = 0; i < array_n(topo_order); i++){
		temp = array_fetch(node_t *, topo_order, i);
		/* get the saving desired at this node */
		assert(st_lookup(valid_table, (char *)temp, (char **)&vdata));
		assert(st_lookup(clp_table, (char *)temp, (char **)&wght));
		if (speed_param->debug > 2){
		    (void)fprintf(sisout,"SAVING %s = %.2f: ", temp->name, vdata->saving);
		}
		
		vdata->num_fi = 0;
		if (vdata->saving < NSP_EPSILON){
		    if (speed_param->debug > 2) (void)fprintf(sisout,"\n");
		    continue;
		}
		
		if (vdata->saving <= SP_IMPROVEMENT(wght)){
		    if (speed_param->debug > 2) (void)fprintf(sisout,"SEL ");
		    vdata->select = TRUE;
		}
		
		/* traverse the critical fanins --- compute slack offsets */
		crit_ip_count = 0; min_ip_slack = POS_LARGE;
		foreach_fanin(temp, j, fanin){
		    if (st_is_member(valid_table, (char *)fanin)){
			assert(delay_wire_required_time(temp, j, speed_param->model, &req));
			arr = delay_arrival_time(fanin);
			slack.rise = req.rise - arr.rise;
			slack.fall = req.fall - arr.fall;
			sl = MIN(slack.rise,slack.fall);
			if (sl < speed_param->crit_slack - NSP_EPSILON){
			    min_ip_slack = MIN(min_ip_slack, sl);
			    array_insert(double, ip_slacks, crit_ip_count, sl);
			    array_insert(node_t *, fi_nodes, crit_ip_count, fanin);
			    crit_ip_count++;
			} 
		    }
		}
		if (speed_param->debug > 2){
		    (void)fprintf(sisout,"%d CRIT ", crit_ip_count);
		}
		/* Determine if all the inputs have potential savings */
		end_of_prop = FALSE;
		for (j = 0; j < crit_ip_count; j++){
		    fanin = array_fetch(node_t *, fi_nodes, j);
		    assert(st_lookup(clp_table, (char *)fanin, (char **)&ip_wght));
		    sl = array_fetch(double, ip_slacks, j);
		    offset = (sl -  min_ip_slack);
		    if (offset + ip_wght->epsilon < vdata->saving){
			end_of_prop = TRUE;
		    }
		}

		if (end_of_prop){
		    /* end_of_prop  => vdata->select */
		    assert(vdata->select);
		    if (speed_param->debug > 2){
			(void)fprintf(sisout, " --- DEAD END\n");
		    }
		    continue;
		}
		
		/* Now propogate the saving towards the inputs */
		for (j = 0; j < crit_ip_count; j++){
		    fanin = array_fetch(node_t *, fi_nodes, j);
		    assert(st_lookup(clp_table, (char *)fanin, (char **)&ip_wght));
		    assert(st_lookup(valid_table, (char *)fanin, (char **)&ip_vdata));
		    sl = array_fetch(double, ip_slacks, j);
		    offset = (sl -  min_ip_slack);
		    if ((ip_wght->epsilon > NSP_EPSILON) &&
			(vdata->saving - offset) <= ip_wght->epsilon){
			/* Saving will be achieved in the transitive fanin */
			ip_sav =  vdata->saving - offset;
			if (ip_sav < NSP_EPSILON){
			    /* Go no further --- slack accounts for saving */
			    if (speed_param->debug > 2){
				(void)fprintf(sisout, " %s(OFF)", fanin->name);
			    }
			} else {
			    ip_vdata->saving = MAX(ip_vdata->saving, ip_sav);
			    array_insert(node_t *, vdata->fi, vdata->num_fi, fanin);
			    vdata->num_fi++;
			    if (speed_param->debug > 2){
				(void)fprintf(sisout," %s(%.2f)", fanin->name, ip_sav);
			    }
			}
		    } else {
			/* No saving is possible or required along this fanin */
			ip_vdata->saving = NEG_LARGE;
			if (speed_param->debug > 2){
			    (void)fprintf(sisout," %s(END)", fanin->name);
			}
		    }
		}
		if (speed_param->debug > 2) (void)fprintf(sisout,"\n");
		/* Completed processing of one node in the TOPO order */
	    }
	    
	    /* 
	     * Now build up the "valid_set" function, starting from the inputs
	     */
	    for (i = array_n(topo_order); i-- > 0; ){
		temp = array_fetch(node_t *, topo_order, i);
		assert(st_lookup(valid_table, (char *)temp, (char **)&vdata));
		if (vdata->saving < NSP_EPSILON){
		    continue;
		}
		if (vdata->num_fi > 0){
		    ip_func = bdd_one(manager);
		    for (k = 0; k < vdata->num_fi; k++){
			fanin = array_fetch(node_t *, vdata->fi, k);
			assert(st_lookup(valid_table, (char *)fanin, (char **)&ip_vdata));
			c = bdd_and(ip_func, ip_vdata->func, 1, 1);
			bdd_free(ip_func);
			ip_func = c;
		    }
		} else {
		    ip_func = bdd_zero(manager);
		}
		
		if (vdata->select){
		    assert(st_lookup(node_to_bdd, (char *)temp, (char **)&var));
		    assert(st_lookup(node_and_id, (char *)temp, (char **)&index));
		    saving_array[index] = MAX(saving_array[index], vdata->saving);
		    vdata->func = bdd_or(var, ip_func, 1, 1);
		} else {
		    assert(vdata->num_fi > 0);
		    assert(ip_func != bdd_zero(manager));
		    vdata->func = ip_func;
		}
	    }
	    
	    /* Combine the "valid_set" function for all the outputs */
	    assert(st_lookup(valid_table, (char *)np, (char **)&vdata));
	    
	    if (speed_param->debug > 3){
		(void)fprintf(sisout, "SELECTION FUNCTION for %s\n", np->name);
		nsp_print_bdd(vdata->func, nodevec, index);
	    }
	    
	    f_prime = bdd_and(f, vdata->func, 1, 1);
	    bdd_free(f);
	    f = f_prime;
	    
	    /* Reinitialize all  vdata_t structures used for the next pass */
	    st_foreach_item(valid_table, stgen, (char **)&temp, (char **)&vdata){
		vdata->saving = NEG_LARGE;
		vdata->select = FALSE;
		if (vdata->func) {
		    bdd_free(vdata->func); vdata->func = NIL(bdd_t);
		}
		vdata->num_fi = 0;
	    }
	    array_free(temp_array);
	    array_free(topo_order);
	    st_free_table(valid_table);
	}

	/* Remove the rejected solutions !!! no need to evaluate them again */
	f_prime = bdd_and(f, bad_f, 1, 0);
	bdd_free(f);
	f = f_prime;

	best_sol = NIL(array_t);
	if (bdd_is_tautology(f, 0)) {
	    /* If no soultions  are left then we are finished */
	    init_sol = array_alloc(node_t *, 0);
	    break;
	} else {
	    /* Now find the min cost solution of the function f*/
	    sol_array = nsp_best_selection(f, node_and_id, cost_array, 0,
					   &area);
	    
	    if (sol_array == NIL(array_t)){
		(void)fprintf(sisout, "ERROR: no solution found\n");
		return NIL(array_t);
	    } else {
		init_sol = array_dup(sol_array);
	    }
	}

	if (speed_param->debug > 1){
	    (void)fprintf(sisout, "SOLUTION (%d node) = ", array_n(sol_array));
	    for ( i = 0; i < array_n(sol_array); i++){
		temp = array_fetch(node_t *, sol_array, i);
		(void)fprintf(sisout, " %s ", temp->name);
	    }
	    (void)fprintf(sisout, "\n");
	}
	
	/* Evaluate to see if other solutions need to be generated */
	num_tries = 1; 
	found_good_set = FALSE;
	sel_length = array_n(sol_array);
	while (TRUE) {
	    if (! nsp_eval_selection(network_in, speed_param, clp_table,
				     thresh, sol_array, area,
				     &best_sol, &found_good_set, &neg_impr)
		|| (num_tries > speed_param->max_num_cuts)) {
		if (speed_param->debug > 1 && !found_good_set){
		    (void)fprintf(sisout, "\tREJECTING %.3f AFTER %d SETS\n",
			      thresh, num_tries);
		}		    
		if (sol_array != NIL(array_t)) {
		    array_free(sol_array);
		}
		sol_array = NIL(array_t);
		break;
	    }
	    /* Remove the current selection from the set of acceptable solns */
	    cost = 0;
	    offending_set = bdd_one(manager);
	    if (speed_param->debug > 2){
		(void)fprintf(sisout, "Just evaluated ## ");
	    }
	    for ( i = 0; i < sel_length; i++){
		temp = array_fetch(node_t *, sol_array, i);
		assert(st_lookup(node_to_bdd, (char *)temp, (char **)&var));
		assert(st_lookup(node_and_id, (char *)temp, (char **)&index));
		c = bdd_and(offending_set, var, 1, 1);
		bdd_free(offending_set);
		offending_set = c;
		cost += cost_array[index];
		if (speed_param->debug > 2){
		    (void)fprintf(sisout, " %s", temp->name);
		}
	    }
	    if (speed_param->debug > 2){
		(void)fprintf(sisout, " ## Cost = %d\n", cost);
	    }
	    c = bdd_and(f, offending_set, 1, 0);
	    bdd_free(f);
	    f = c;
	    if (neg_impr) {
		/* Add to the set of selectiosn that can be avoided !!! */
		c = bdd_or(bad_f, offending_set, 1, 1);
		bdd_free(bad_f);
		bad_f = c;
	    }
	    
	    bdd_free(offending_set);

	    /* Get the next best selection !!! */
	    array_free(sol_array);
	    sol_array = nsp_best_selection(f, node_and_id, cost_array, 0,
					   &area); 
	    sel_length = (sol_array == NIL(array_t) ? 0 : array_n(sol_array));
	    num_tries++;
	}
/*
	if (speed_param->debug > 2){
	    (void)fprintf(sisout, "\nVALID SELECTION FUNCTION\n\n");
	    nsp_print_bdd(f, nodevec);
	}
*/

	bdd_free(f);
	st_free_table(visited);

	if (! found_good_set){
	    thresh /= 2;
	    if (speed_param->debug > 1){
		(void)fprintf(sisout, "REDUCING THRESHOLD TO %.2f\n", thresh);
	    }
	}
    } while (! found_good_set && (thresh > MIN_ACCEPTABLE_SAVING));
    
    /* Termination */
    st_free_table(node_to_bdd);
    st_free_table(node_and_id);
    for ( i = 0; i < num_lit; i++){
	array_free(valid_data[i].fi);
    }
    FREE(valid_data);
    array_free(nodevec);
    array_free(fi_nodes);
    array_free(ip_slacks);
    FREE(cost_array);
    FREE(saving_array);
    bdd_end(manager);
    
    if (best_sol == NIL(array_t)){
	if (speed_param->debug){
	    (void)fprintf(sisout, "REVERTING TO ORIGINAL SOLUTION\n");
	}
	sol_array = array_dup(init_sol);
    } else {
	sol_array = best_sol;
    }
    array_free(init_sol);

    return sol_array;
}

/* 
 * Determine if the selected set of transformations interacts severely
 * so as to offset the advantages that would incur from implemeting them
 * Returns 0 if the selection terminates the search for more solutions !!!
 */
static int
nsp_eval_selection(network, speed_param, clp_table, thresh,
		   orig_sol_array, area, best_array, found_good_set, neg_impr)
network_t *network;
speed_global_t *speed_param;
st_table *clp_table;
double thresh;	         /* The impr that we expect without interactions */
array_t *orig_sol_array;
int area;                /* Cost of the current selection */
array_t **best_array;
int *found_good_set;
int *neg_impr;
{
    int i, j, index, check_failed;
    node_t *temp;
    sp_weight_t *wght;
    network_t *dup_network;
    char *key, *value;
    st_generator *stgen;
    st_table *equiv_name_table, *select_table;
    array_t *new_roots;
    array_t *sol_array;	/* Actual solution array !!! */
    sp_xform_t *local_trans;
    double diff, desired, old_value, new_value, sl;
    static double best_area, best_delay;

    if (orig_sol_array == NIL(array_t)) return 0;

    /* No interaction occurs when the "unit" delay model is used */
    if (speed_param->model == DELAY_MODEL_UNIT) {
	*found_good_set = TRUE;
	*neg_impr = FALSE;
	*best_array = array_dup(orig_sol_array);
	return 0;
    }

    /* Set the value that one feels is acceptable */
    if (speed_param->req_times_set){
 	(void)sp_minimum_slack(network, &old_value);
	desired = old_value + thresh * ACCEPTABLE_FRACTION;
    } else {
	(void)delay_latest_output(network, &old_value);
	desired = old_value - thresh * ACCEPTABLE_FRACTION;
    }

    dup_network = speed_network_dup(network);

    sol_array = array_dup(orig_sol_array);
    sp_expand_selection(speed_param, sol_array, clp_table, &select_table);
    speed_reorder_cutset(speed_param, clp_table, &sol_array);

    /* Remove the old configurations */
    for ( i = 0; i < array_n(sol_array); i++){
	temp = array_fetch(node_t *, sol_array, i);
	assert(st_lookup(clp_table, (char *)temp, (char **)&wght));
	index = wght->best_technique;
	local_trans = sp_local_trans_from_index(speed_param, index);

	/* Ensure that the network is around !!! */
	if (wght->best_netw == NIL(network_t)){
	    sl = speed_param->crit_slack;
	    speed_param->crit_slack = wght->crit_slack;
	    j = speed_param->debug;
	    speed_param->debug = FALSE;
	    wght->best_netw = NSP_OPTIMIZE(wght, local_trans, speed_param);
	    speed_param->debug = j;
	    speed_param->crit_slack = sl;
	}

	sp_delete_network(speed_param, dup_network, temp, 
			  NSP_NETWORK(wght,local_trans),
			  select_table, clp_table, NIL(st_table));
    }
    st_free_table(select_table);

    /* Insert the new ones */
    new_roots = array_alloc(node_t *, 0);
    equiv_name_table = st_init_table(strcmp, st_strhash);
    for(i = 0; i < array_n(sol_array); i++ ){
	temp = array_fetch(node_t *, sol_array, i);
	assert(st_lookup(clp_table, (char *)temp, (char **)&wght));
	index = wght->best_technique;
	local_trans = sp_local_trans_from_index(speed_param, index);

        sp_append_network(speed_param, dup_network, wght->best_netw,
			  local_trans, equiv_name_table, new_roots);
    }
    array_free(new_roots);
    st_foreach_item(equiv_name_table, stgen, &key, &value){
	FREE(key); FREE(value);
    }
    st_free_table(equiv_name_table);

    /* Evaluate the difference */
    delay_trace(dup_network, speed_param->model);

    *neg_impr = check_failed = FALSE;
    if (speed_param->req_times_set){
 	(void)sp_minimum_slack(dup_network, &new_value);
	diff = new_value - old_value;
	if (new_value < desired) check_failed = TRUE;
	if (new_value < old_value) *neg_impr = TRUE;
    } else {
	(void)delay_latest_output(dup_network, &new_value);
	diff = old_value - new_value ;
	if (new_value > desired) check_failed = TRUE;
	if (new_value > old_value) *neg_impr = TRUE;
    }

    network_free(dup_network);
    
    if (speed_param->debug > 1){
	(void)fprintf(sisout, "%s check; IMPROVEMENT = %.3f, area = %d\n", 
		      (check_failed ? "FAILED" : "PASSED"), diff, area);
    }
    
    /* First one... Initialize the area and delay */
    if (*best_array == NIL(array_t)){
	best_area = INFINITY; /* Don't use POS_LARGE (scaling problems) */
	best_delay = POS_LARGE;
	*best_array = array_dup(orig_sol_array);
    }
    
    /* Record the best one so far !!! */
    if (check_failed && !(*found_good_set)) {
	/* Choose the MIN area solution */
	if (area < best_area){
	    best_area = area;
	    best_delay = diff;
	    array_free(*best_array);
	    *best_array = array_dup(orig_sol_array);
	    if (speed_param->debug > 1) {
		(void)fprintf(sisout, "Accepting (1) D= %.3f, cost = %d\n",
			      diff, area);
	    }
	}
    } else if (!check_failed){
	/* Choose best bang/buck !!! */
	assert(diff > 0);
	if (!(*found_good_set) || ((diff/area) > (best_delay/best_area))) {
	    best_area = area;
	    best_delay = diff;
	    array_free(*best_array);
	    *best_array = array_dup(orig_sol_array);
	    *found_good_set = TRUE;
	    if (speed_param->debug) {
		(void)fprintf(sisout, "Accepting (2) D= %.3f, cost = %d\n",
			      diff, area);
	    }
	}
    } else {
	/* Ignore this one */
    }

    array_free(sol_array);
    return 1;
}

#undef NSP_MAXSTR







