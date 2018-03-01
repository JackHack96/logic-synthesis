/*
 * Revision Control Information
 *
 * $Source:
 * $Author:
 * $Revision:
 * $Date:
 *
 */
/*
 * The routines used for the Generalized Bypass Transformation.
 * Original code by Patrick McGeer,
 * Extensively modified/fixed by Heather Harkness
 * Modified to use the "list" package instead of the files mylist.[ch] by
 *          Kanwar Jit Singh
 */

#include "sis.h"
#include "speed_int.h"
#include "gbx_int.h"

#define GBX_MAXWEIGHT 0x7ffffffe

st_table *bypass_table;
st_table *gbx_node_table;
lsList bypasses;
delay_model_t gbx_delay_model;
void new_trace_bypass();
void newer_trace_bypass();
void extend_bypass();

#define foreach_list_item(list,gen,item) \
    for((gen) = lsStart((list)); \
	(lsNext((gen),(lsGeneric *)&(item),LS_NH) ==  LS_OK ? 1 : \
	 (lsFinish((gen)),0)) ; )

node_t *
node_my_dup(node)
node_t *node;
{
    node_t *new_node;

    new_node = node_dup(node);
    FREE(new_node->name);
    new_node->name = NIL(char);
    return new_node;
}    

node_t *
replace_fanin(node, fanin, replacement_fanin)
node_t *node, *fanin, *replacement_fanin;
{
    node_t *p, *q, *r;
    node_t *lit0, *lit1, *tmp0, *tmp1, *tmp2;
    node_t *result;

    if(node_get_fanin_index(node, fanin) == -1) return node_dup(node);

    node_algebraic_cofactor(node, fanin, &p, &q, &r);
    if(node_function(p) == NODE_0) {
	lit0 = node_literal(replacement_fanin, 0);
	tmp0 = node_and(q, lit0);
	result = node_or(tmp0, r);
	node_free(lit0); node_free(tmp0);
    } else if (node_function(q) == NODE_0) {
	lit1 = node_literal(replacement_fanin, 1);
	tmp1 = node_and(p, lit1);
	result = node_or(tmp1, r);
	node_free(lit1); node_free(tmp1);
    } else {
	lit1 = node_literal(replacement_fanin, 1);
	tmp1 = node_and(p, lit1);
	lit0 = node_literal(replacement_fanin, 0);
	tmp0 = node_and(q, lit0);
	tmp2 = node_or(tmp0, tmp1);
	node_free(lit1); node_free(tmp1);
	node_free(lit0); node_free(tmp0);
	result = node_or(tmp2, r); node_free(tmp2);
    }
    node_free(p); node_free(q); node_free(r);
    return result;
}

node_t *
add_opt_boolean_diff(innode, network, model)
node_t *innode;
network_t *network;
delay_model_t model;
{
    network_t *network_tmp; 
    speed_global_t speed_param;
    int i, j;
    array_t *nodevec;
    node_t *node, *newnode, *fanin, *orig, *last_node; 

    /* convert node to temporary network */ 
    network_tmp = network_create_from_node(innode);

    /* optimize the boolean diff network for delay */
    speed_fill_options(&speed_param, 0, NIL(char *));
    speed_param.model = model; 
    speed_init_decomp(network_tmp, &speed_param);     

    /* add optimized boolean diff to existing network */
    nodevec = network_dfs(network_tmp);
    for (i = 0; i < array_n(nodevec); i++) {
       node = array_fetch(node_t *, nodevec, i);
       if (node->type != INTERNAL) continue;
       newnode = node_dup(node);
       node->copy = newnode;

       /* Change the fanin pointers */
       foreach_fanin(newnode, j, fanin){
          if (fanin->type == PRIMARY_INPUT) {
             /* patch the fanin to original */
             if ((orig = name_to_node(innode, node_long_name(fanin))) != NIL(node_t)){
                newnode->fanin[j] = orig;
             }  
             else {
                fail("Failed to retrieve the original node");
             }
          }
          else {
             /* update the fanin pointer */
             newnode->fanin[j] = fanin->copy;
          }
       }
       network_add_node(network, newnode);
       last_node = newnode;
    }

    array_free(nodevec);
    network_free(network_tmp); 

    assert(network_check(network));
    return last_node;
}

int
take_bypass(bypass, network, model, mux_delay, actual_mux_delay)
bypass_t *bypass;
network_t *network;
delay_model_t model;
double mux_delay, actual_mux_delay;
{
    node_t *last_node, *node, *fanout, *fanin, *tmp, *tmp1, *tmp2;
    node_t *diff, *tmp3, *tmp4, *tmp5, *mux, *p, *q, *r, *dup_node;
    node_t *pathfan, *sub_node, *newdiff;
    int phase;
    lsGeneric dummy;
    lsList dup_nodes;
    lsGen gen, gen1, gen2;   
    int j;
    int nfanins;
    network_t *saved_net;

    /* Check that all nodes in the bypass are still in the network
       before attempting bypass.  If not, then punt.
       This is required because bypasses might overlap, and some 
       bypass nodes may have been removed by network_sweep when a 
       previous bypass was taken. */

    if (node_network(bypass->first_node) != network)
       return 0; 
    foreach_list_item(bypass->bypassed_nodes, gen, node) {
       if (node_network(node) != network) 
          return 0;
    }

/*    if ((mux_delay == 1) && (actual_mux_delay > 1)) {
       saved_net = network_dup(network); 
    }
*/

    if (print_bypass_mode) {
       printf("Taking Bypass\n");
       print_bypass(bypass); 
    }
 
    /* Form the Boolean difference as a big AND gate */
    
    last_node = bypass->first_node;
    diff = node_constant(1);
    last_node = bypass->first_node;
    foreach_list_item(bypass->bypassed_nodes, gen, node) {
	node_algebraic_cofactor(node, last_node, &p, &q, &r);
	tmp = node_xor(p, q);
	tmp3 = node_not(r);
	tmp1 = node_and(tmp, tmp3);
	tmp2 = node_and(diff, tmp1);
	node_free(diff); node_free(tmp); node_free(tmp1); node_free(tmp3);
	diff = tmp2;
	last_node = node;
    } 
   
    if (node_num_fanin(diff) <= 2) {
       newdiff = diff;
       network_add_node(network, newdiff);
    }
    else newdiff = add_opt_boolean_diff(diff, network, model);
    assert(network_check(network));

    /* Form the multiplexer.  Yes, this really and truly is what it takes
       to write a = b c + b'd in MIS, strange but true*/
    
    phase = (bypass->phase == POS_UNATE?1:0);
    tmp = node_literal(bypass->first_node, phase);
    tmp1 = node_literal(newdiff, 1); tmp2 = node_and(tmp, tmp1);
    tmp3 = node_literal(bypass->last_node, 1); tmp4 = node_literal(newdiff, 0);
    tmp5 = node_and(tmp3, tmp4); mux = node_or(tmp5, tmp2);
    node_free(tmp); node_free(tmp1); node_free(tmp2); node_free(tmp3);
    node_free(tmp4); node_free(tmp5);

    /* Make sure the MUX is in the network */
    
    network_add_node(network, mux);

 
    /* KMS the bypassed path. First duplicate all the nodes upto
       bypass->dupe_at, if needed, in preparation for the KMS Transform after
       the bypass. 
       Can't rely on dupe_at value set when bypass was found because subsequent
       taken bypasses may have created additional fanouts.  So check for fanouts
       again. */

    foreach_list_item(bypass->bypassed_nodes, gen, node) {
       if (node_num_fanout(node) > 1)
	  bypass->dupe_at = node;
    }

    if(bypass->dupe_at != NIL(node_t)) {
	dup_nodes = lsCreate();
	
	/* Make a list of all the nodes we'll duplicate, and add each dup'd
	   node to the network.  Note in particular that node will now
	   fanout to both the fanout of the original node and to the dup of
	   the fanout of the original node. */
	
	foreach_list_item(bypass->bypassed_nodes, gen, node) {
	    dup_node = node_my_dup(node);
	    lsNewEnd(dup_nodes, (lsGeneric)dup_node, LS_NH);
	    network_add_node(network, dup_node);
	    assert(network_check(network));
	}

	/* Walk down the list of nodes, substituting the dup node for the
	   original  on the bypass */

	gen1 = lsStart(dup_nodes);
	gen = lsStart(bypass->bypassed_nodes);

	/* Walk the list, doing the substitution */

	(void)lsNext(gen1, (lsGeneric *)&dup_node, LS_NH);
	do {
	    (void)lsNext(gen, (lsGeneric *)&node, LS_NH);
	    if (lsNext(gen1, (lsGeneric *) &pathfan, LS_NH) != LS_OK) {
		pathfan = mux;
	    }

	    /* Substitute dup_node for node in path_fan */
	    node_patch_fanin(pathfan, node, dup_node);
	    
	    assert(network_check(network));
	    
	    dup_node = pathfan;
	} while (pathfan != mux);
	lsFinish(gen); lsFinish(gen1);

	/* Now that we've duplicated the bypassed nodes, substitute the
	   path of duplicated nodes for the bypassed path (this is the
	   path of nodes that has been genuinely bypassed, now).  This is
	   important since (below) we're going to stick the constant on the
	   first node of the bypassed path, so we want to make sure we
	   get the right bypassed path, here.  Note we don't change
	   bypass->last_node, since we want to patch the fanouts of this
	   node to be fanned into by the mux, below */
	
	lsDestroy(bypass->bypassed_nodes, (void (*)())0);
	bypass->bypassed_nodes = dup_nodes;
    }

    /* Substitute the multiplexer into the network.  Important to do this
     AFTER we've dup'd off the bypassed path.  */

    if (!network_check(network)) {
       printf("%s\n", error_string());
     }  
    
    foreach_fanout(bypass->last_node, gen2, fanout) {
	if(fanout == mux) continue;
	node_patch_fanin(fanout, bypass->last_node, mux);
	assert(network_check(network));
    }

    /* OK.  Now that we've walked the list, duplicated as needed, we can
       stick the stuck-at-fault.  Do it to eliminate (if possible) the
       first bypassed node.  All this code is really just an intelligent
       selection of which fault to stick */
    
    gen = lsStart(bypass->bypassed_nodes);
    (void)lsNext(gen, (lsGeneric *)&node, LS_NH);
    node_algebraic_cofactor(node, bypass->first_node, &p, &q, &r);

    /* First try to set node to 1 */
    if(node_function(p) == NODE_1) sub_node =  node_constant(1);
    else if(node_function(q) == NODE_1) sub_node =  node_constant(0);

    /* If that doesn't work, see if we can eliminate all cubes containing
       bypass->first_node from node */
    else if(node_function(p) == NODE_0)  sub_node =  node_constant(1);  
    else if(node_function(q) == NODE_0)  sub_node =  node_constant(0);
    
    else sub_node = node_constant(1); /* Grrr.  Gotta do something */

    /* stick the constant in in place of bypass->first_node, and replace
       bypass->first_node in the network, then clean up */
    
    network_add_node(network, sub_node);
    node_patch_fanin(node, bypass->first_node, sub_node);
    node_free(p); node_free(q); node_free(r);
    network_sweep(network);

    /* If the actual mux delay is 2 and we're looking for last gasp bypasses
       with mux delay of 1, reject any bypasses which result in larger delay */
    if ((mux->network == network) && (mux_delay == 1) && (actual_mux_delay > 1)) {
       nfanins = 0;
       foreach_fanin(mux, j, fanin){
          if (fanin->network == network) 
             nfanins++;
       }
       if (nfanins > 2) {
          if (gbx_verbose_mode) 
             printf("Mux has more than 2 inputs\n");
/*        network_free(network);
          network = network_dup(saved_net); 
          network_free(saved_net);
          if (print_bypass_mode) printf("Bypass rejected\n");
          return 0;
*/  
       }
/*     else network_free(saved_net);  */
    }

    return 1;
}

bypass_t *
bypass_copy(bypass)
bypass_t *bypass;
{
    bypass_t *result;
    
    SAFE_ALLOC(result, bypass_t, 1);
    *result = *bypass;
    result->bypassed_nodes = lsCopy(bypass->bypassed_nodes, (lsGeneric(*)()) 0);
    return result;
}	


bypass_t *
new_bypass(node, fanout, weight, slack, phase)
node_t *fanout, *node;
double weight, slack;
input_phase_t phase;
{
    bypass_t *result;
    node_t *fanin;
    delay_time_t artime;
    int i;
    
    SAFE_ALLOC(result, bypass_t, 1);
    result->first_node = node;
    result->last_node = fanout;
    result->gain = weight;
    result->dupe_at = NIL(node_t);
    result->bypassed_nodes = lsCreate();
    result->phase = phase;
    result->slack  = slack;
    result->side_slack = slack;
    artime = delay_arrival_time(node);
    result->side_delay = 0.0;
    result->control_delay = max(artime.rise, artime.fall);
    foreach_fanin(fanout, i, fanin) {
	if(fanin == node) continue;
	artime = delay_arrival_time(fanin);
	if(max(artime.rise, artime.fall) > result->side_delay) {
	    result->side_delay = max(artime.rise, artime.fall);
	}
    }
    (void)lsNewEnd(result->bypassed_nodes, (lsGeneric)fanout, LS_NH);
    return result;
}
bypass_t *
bypass_dup(bypass)
bypass_t *bypass;
{
    bypass_t *result;
    SAFE_ALLOC(result, bypass_t, 1);
    *result = *bypass;
    result->bypassed_nodes = lsCopy(bypass->bypassed_nodes, (lsGeneric(*)())0);
    return result;
}
void
free_bypass(bypass)
bypass_t *bypass;
{
    lsDestroy(bypass->bypassed_nodes,  (void(*)())0);
    FREE(bypass);
}
int
bypass_is_extensible(bypass, fanout, node, weight)
node_t *fanout, *node;
bypass_t *bypass;
double weight;
{
    node_t *fanin;
    int i;
    delay_time_t artime, cartime;
    double control_delay, side_delay, side_slack;
    
    if(node_function(fanout) == NODE_PO) {
	return 0;
    }

    side_delay = 0;
    foreach_fanin(fanout, i, fanin) {
	if (fanin == node) {
           cartime = delay_arrival_time(fanin);
           control_delay = max(cartime.rise, cartime.fall);
        } 
        else {
	   artime = delay_arrival_time(fanin);
	   side_delay = max(side_delay, max(artime.rise, artime.fall));
        }
    }    
    side_slack = min(bypass->side_slack, control_delay - side_delay); 
    
    if (bypass->gain + weight <= side_slack) 
       return 1;
    else return 0;
}
void
bypass_add_node(bypass, fanout, node, weight, phase)
node_t *fanout, *node;
bypass_t *bypass;
double weight;
input_phase_t phase;
{
    node_bp_t *record;
    
    bypass->last_node = fanout;
    lsNewEnd(bypass->bypassed_nodes, (lsGeneric)fanout, LS_NH);
    if(node_num_fanout(node) > 1)
	bypass->dupe_at = node;

    bypass->gain += weight;

    st_lookup(gbx_node_table, (char *) fanout, (char **) &record);
/*    record->mark = 1;  */

    /* Set the phase.  This works since all phases must be POS or NEG_UNATE */
    
    bypass->phase = (bypass->phase==phase?POS_UNATE:NEG_UNATE);
    /* Update the slack */
    bypass->slack = min(bypass->slack, record->path_slack);
}
void
bypass_new_add_node(bypass, fanout, node)
node_t *fanout, *node;
bypass_t *bypass;
{
    node_bp_t *record;
    node_t *fanin;
    int i;
    delay_time_t artime, cartime;
    input_phase_t phase;
    double edgeweight, control_delay, side_delay;
    
    bypass->last_node = fanout;
    lsNewEnd(bypass->bypassed_nodes, (lsGeneric)fanout, LS_NH);
    if(node_num_fanout(node) > 1)
	bypass->dupe_at = node;

    side_delay = 0;
    foreach_fanin(fanout, i, fanin) {
	if (fanin == node) {
           cartime = delay_arrival_time(fanin);
           control_delay = max(cartime.rise, cartime.fall);
        } 
        else {
	   artime = delay_arrival_time(fanin);
	   side_delay =
	      max(side_delay, max(artime.rise, artime.fall));
        }
    }    
    bypass->side_slack = min(bypass->side_slack,  
                             control_delay - side_delay); 

    phase = node_input_phase(fanout, node);
    edgeweight = weight(fanout, node);

    bypass->gain += edgeweight;

    st_lookup(gbx_node_table, (char *) fanout, (char **) &record);
/*      record->mark = 1;   */

    /* Set the phase.  This works since all phases must be POS or NEG_UNATE */
    
    bypass->phase = (bypass->phase==phase?POS_UNATE:NEG_UNATE);
    /* Update the slack */
    bypass->slack = min(bypass->slack, record->path_slack);
}
void
register_bypass(bypass)
bypass_t *bypass;
{
    node_t *node;
    lsGen gen;
    node_bp_t *record;
     
    int unique_insertion_code =
	st_insert(bypass_table, (char *) bypass->last_node, (char *) bypass);
    (void)lsNewBegin(bypasses, (lsGeneric)bypass, LS_NH);
    assert(!unique_insertion_code);
   
    /* mark nodes on the new bypass */
    foreach_list_item(bypass->bypassed_nodes, gen, node) {
       st_lookup(gbx_node_table, (char *) node, (char **) &record);
       record->mark = 1;
    }
}

void
print_node_bp_record(rec, node)
node_bp_t *rec;
node_t *node;
{
    int i;
    node_t *fanin;

    printf("Printing record for node %s.  Slack %f mark %d.  Path fanin %s, off-path slack %f\n",
	   node_name(node), rec->slack, rec->mark,
	   rec->path_fanin == NIL(node_t)?"none":node_name(rec->path_fanin),
	   rec->path_slack);

    foreach_fanin(node, i, fanin) {
	printf("fanin %s slack %f weight %f phase %s\n", node_name(fanin),
	       rec->pin_slacks[i], rec->pin_weights[i],
	       (rec->input_phases[i] == BINATE)?"binate":
	       (rec->input_phases[i] == POS_UNATE)?"positive":
	       (rec->input_phases[i] == NEG_UNATE)?"negative":"unknown");
    }
}

void
destroy_bp_record(record)
node_bp_t *record;
{
    if(record->pin_slacks != NIL(double)) {
	FREE(record->pin_slacks);
	FREE(record->pin_weights);
	FREE(record->input_phases);
    }
    FREE(record);
}
    
node_bp_t *
new_node_bp_record(node)
node_t *node;
{
    node_bp_t *result;
    delay_time_t slack, rtime, atime, delay;
    double minslack;
    input_phase_t phase;
    int i, n;
    node_t *fanin;
    

    SAFE_ALLOC(result, node_bp_t, 1);
    result->mark = 0;
    slack = delay_slack_time(node);
    result->slack = min(slack.rise, slack.fall);
    result->path_fanin = NIL(node_t);
    if(node_function(node) == NODE_PI) {
	result->pin_slacks = result->pin_weights = NIL(double);
	result->input_phases = NIL(input_phase_t);
	return result;
    }
    n = node_num_fanin(node);
    SAFE_ALLOC(result->pin_slacks, double, n);
    SAFE_ALLOC(result->pin_weights, double, n);
    SAFE_ALLOC(result->input_phases, input_phase_t, n);
    minslack = result->path_slack = 1E29;
   
    for(i = 0; i < n; ++i) {
	delay = delay_node_pin(node, i, gbx_delay_model);
	fanin = node_get_fanin(node, i);
	atime = delay_arrival_time(fanin);
	rtime = delay_required_time(node);

	/* Error check due to iphase bug.  Take out when patch installed */
	if(node_function(node) != NODE_PO)
	    phase = node_input_phase(node, fanin);
	else phase = POS_UNATE;
	
	switch(phase) {
	case POS_UNATE:
	    rtime.rise -= delay.rise;
	    rtime.fall -= delay.fall;
	    break;
	case NEG_UNATE:
	    rtime.rise -= delay.fall;
	    rtime.fall -= delay.rise;
	    break;
	default:
	    rtime.rise = min(rtime.rise, rtime.fall);
	    delay.rise = max(delay.rise, delay.fall);
	    rtime.rise -= delay.rise;
	    rtime.fall = rtime.rise;
	}
	result->pin_slacks[i] = min(rtime.rise - atime.rise,
				    rtime.fall - atime.fall);
	result->pin_weights[i] = max(delay.rise, delay.fall);
	result->input_phases[i] = phase;

	/* Update the minimum slack and find the path fanin */
	
	if(result->pin_slacks[i] <= minslack) {
	    result->path_slack = minslack;
	    minslack = result->pin_slacks[i];
	    result->path_fanin = fanin;
	} else if(result->pin_slacks[i] < result->path_slack) {
	    result->path_slack = result->pin_slacks[i];
	}
    }
    /* Finally, it's the *extra* slack on the side-inputs we want */
    result->path_slack -= minslack;
    return result;
}
void
gbx_init_node_table(network, model)
network_t *network;
delay_model_t model;
{
    array_t *nodes;
    node_t *node;
    node_bp_t *node_record;
    int i;

    gbx_delay_model = model;
    (void) delay_trace(network, model);    
    nodes = network_dfs(network);
    gbx_node_table = st_init_table(st_ptrcmp, st_ptrhash);
    for(i = 0; i < array_n(nodes); ++i) {
	node = array_fetch(node_t *, nodes, i);
	node_record = new_node_bp_record(node);
	st_insert(gbx_node_table, (char *) node, (char *) node_record);
/*	print_node_bp_record(node_record, node);  */
    }
    array_free(nodes);
}
lsList
bypass_extension_fanouts(bypass, node)
node_t *node;
bypass_t *bypass;
{
    lsGen gen;
    node_t *fanout;
    lsList path_fanouts;
    
    assert(node_function(node) != NODE_PO);
    path_fanouts = lsCreate();
    foreach_fanout(node, gen, fanout) {
	if(bypass_is_extensible(bypass, fanout, node, weight(fanout, node))) {
	    (void)lsNewBegin(path_fanouts, (lsGeneric) fanout, LS_NH);
	}
    }
    if(lsLength(path_fanouts) == 0) {
	lsDestroy(path_fanouts,  (void(*)())0);
	return LS_NIL;
    } else {
	return path_fanouts;
    }
}

lsList
path_fanouts(node, epsilon)
node_t *node;
double epsilon;
{
    int i;
    lsGen gen;
    node_t *fanout;
    lsList path_fanouts;
    node_bp_t *fanout_record;
    input_phase_t phase;

    
    assert(node_function(node) != NODE_PO);
    path_fanouts = lsCreate();
    foreach_fanout(node, gen, fanout) {
	i = node_get_fanin_index(fanout, node);
	assert(st_lookup(gbx_node_table, (char *) fanout, (char **) &fanout_record));
	if(fanout_record->pin_slacks[i] > epsilon) continue;
	if(node_function(fanout) == NODE_PO) continue;
	if((phase = node_input_phase(fanout, node)) == BINATE) continue;
	if(phase == PHASE_UNKNOWN) continue;
	if(fanout_record->path_fanin != node) continue;
	if(fanout_record->path_slack == 0.0) continue;
	(void)lsNewBegin(path_fanouts, (lsGeneric) fanout, LS_NH);
    }
    return path_fanouts;
}

node_t *
path_fanout(node, epsilon)
node_t *node;
double epsilon;
{
    int fanout_found;
    lsGen gen;
    node_t *fanout, *path_fanout;
    node_bp_t *fanout_record;
    input_phase_t phase;
    
    assert(node_function(node) != NODE_PO);
    fanout_found = 0;
    foreach_fanout(node, gen, fanout) {
	assert(st_lookup(gbx_node_table, (char *) fanout, (char **) &fanout_record));
	if(fanout_record->path_fanin != node) return NIL(node_t);
	if(fanout_record->slack > epsilon) continue;
	if(node_function(fanout) == NODE_PO) return NIL(node_t);
	if(fanout_found) return NIL(node_t); /* two path fanouts */
	if((phase = node_input_phase(fanout, node)) == BINATE)
	    return NIL(node_t);
	if(phase == PHASE_UNKNOWN) return NIL(node_t);
	fanout_found = 1;
	path_fanout = fanout;
    }
    assert(fanout_found);
    return path_fanout;
}
	
double
retrieve_slack(fanout, node)
node_t *fanout, *node;
{
    int i;
    node_bp_t *record;

    assert(st_lookup(gbx_node_table, (char *) fanout, (char **) &record));
    i = node_get_fanin_index(fanout, node);
    return record->pin_slacks[i];
}
double
weight(fanout, node)
node_t *fanout, *node;
{
    int i;
    node_bp_t *record;

    assert(st_lookup(gbx_node_table, (char *) fanout, (char **) &record));
    i = node_get_fanin_index(fanout, node);
    return record->pin_weights[i];
}
input_phase_t
retrieve_phase(fanout, node)
node_t *fanout, *node;
{
    int i;
    node_bp_t *record;

    assert(st_lookup(gbx_node_table, (char *) fanout, (char **) &record));
    i = node_get_fanin_index(fanout, node);
    return record->input_phases[i];
}

lsList
new_find_bypass_nodes(network, epsilon, mux_delay, model)
network_t *network;
double epsilon;
double mux_delay;
delay_model_t model;
{
    array_t *nodes;
    int i, j;
    node_t *node, *fanout;
    node_bp_t *node_record;
    lsList bypass_fanouts;
    lsGen gen;
    lsGen gen1;
    
    bypass_table = st_init_table(st_ptrcmp, st_ptrhash);
    bypasses = lsCreate();
    
    nodes = network_dfs(network);
    for(i = 0; i < array_n(nodes); ++i) {
	/* Is this node critical?  */
	node = array_fetch(node_t *, nodes, i);
	st_lookup(gbx_node_table, (char *) node, (char **) &node_record);
	if(node_record->slack > epsilon) continue;
	if(node_function(node) == NODE_PO) continue;
	/* If it is, begin a list of the bypasses that begin with
	   this node */
	bypass_fanouts = lsCreate();
	foreach_fanout(node, gen1, fanout) {
	    st_lookup(gbx_node_table, (char *) fanout, (char **) &node_record);
	    if(node_record->slack > epsilon) continue;
	    j = node_get_fanin_index(fanout, node);
	    if(node_record->path_fanin != node) continue;
	    if(node_record->input_phases[j] == BINATE) continue;
	    if(node_record->input_phases[j] == PHASE_UNKNOWN) continue;
	    if(node_function(fanout) == NODE_PO) continue;
	    (void)lsNewBegin(bypass_fanouts, (lsGeneric) fanout, LS_NH);
	}
	/* Now try to extend each bypass */
	foreach_list_item(bypass_fanouts, gen, fanout) {
	    new_trace_bypass(fanout, node, epsilon, mux_delay, NIL(node_t),
               model);
	}
    }
    array_free(nodes);
    return bypasses;
}

void
new_trace_bypass(fanout, node, epsilon, mux_delay, last_bypass_ended, model)
node_t *fanout, *node, *last_bypass_ended;
double epsilon, mux_delay;
delay_model_t model;
{
    double slack, slackoffset, edgeweight;
    node_t *dummy_node, *fanin;
    int j;
    bypass_t *bypass, *old_bypass;
    input_phase_t phase;
    char reg_code;


    /* There is a theorem that this is a candidate bypass if and only if the
       bypass gain is <= the slack on all the side inputs - the slack on
       the original path input.  So compute this here and ensure that this
       invariant holds.  The way this works is simple.  Keep track of the
       minimum slack, subtracting off the bypass gain each time.  When we
       hit zero, we're done. */
    
    slackoffset = retrieve_slack(fanout, node);
    edgeweight = weight(fanout, node);
    slack = 1E29;
    foreach_fanin(fanout, j, fanin) {
       if (fanin == node) continue;
       slack = min(slack, retrieve_slack(fanout, fanin));
    }

    /* If we're using one of the unit delay models, then we have a 2-input 
       AND/OR node network.  In this case, we know KMS will eliminate the 
       first bypassed node, so we can allow a slack on the first node which 
       is one less than the minimum slack required on the other nodes.
       (We check the fanin number just in case this node is a "3-input,
       1-level" mux generated by a previous pass.) */
         
    if ((start_node_mode) &&
        (model == DELAY_MODEL_UNIT || model == DELAY_MODEL_UNIT_FANOUT) &&
        (node_num_fanin(fanout) <= 2)) {
       if (slack-slackoffset > 0) slack++;
       else return;      
    }
    else if (slack-slackoffset < edgeweight) return;
  

    /* OK, so we've bypassed at least one node...*/

    bypass = new_bypass(node, fanout, edgeweight, slack-slackoffset,
			retrieve_phase(fanout, node));

/*  st_lookup(gbx_node_table, fanout, &record);
    record->mark = 1;  */  
  
    fanout = path_fanout(node = fanout, epsilon);
    while((slack >= (bypass->gain + slackoffset)) &&  fanout != NIL(node_t)) {
	foreach_fanin(fanout, j, fanin) {
	    if(fanin == node) continue;
	    slack = min(slack, retrieve_slack(fanout, fanin));
	}
	phase = node_input_phase(fanout, node);
	edgeweight = weight(fanout, fanin);
	if(slack >= (bypass->gain + edgeweight + slackoffset))
	    bypass_add_node(bypass, fanout, node, edgeweight, phase);
	else break;
	fanout = path_fanout(node = fanout, epsilon);
    }
    bypass->gain -= mux_delay;
    
    /* Now.  Do we want to register this bypass?  Only if the gain is
       non-nil AND this bypass is not contained in the last one we did */

    /* Debugging print statement */
/*    printf("Bypass found\n"); print_bypass(bypass); */
    
    /* new code for the new tracer */
    
    reg_code = bypass->gain > 0;
    
    if(reg_code && (st_lookup(bypass_table, (char *) bypass->last_node, (char **) &old_bypass))) {
	if(bypass->gain > old_bypass->gain) {
	    reg_code = 1;

	    /* Delete old_bypass from the table.  Pass in the key as a dummy
	       since st may change it and I don't trust Moore that much.
	       Don't bother freeing the old_bypass since it's already on
	       the bypass list and will get freed in the terminal cleanup
	       anyway. */
	    
	    dummy_node = bypass->last_node;
	    st_delete(bypass_table, (char **) &dummy_node, (char **) &old_bypass);
	} else {
	    reg_code = 0;
	}
    }

    if(reg_code) {
	register_bypass(bypass);
    } else {
	free_bypass(bypass);
    }
	    
}
lsList
newer_find_bypass_nodes(network, epsilon,  mux_delay, model)
network_t *network;
double epsilon;
double mux_delay;
delay_model_t model;
{
    array_t *nodes;
    int i, j;
    node_t *node, *fanout;
    node_bp_t *node_record;
    lsList bypass_fanouts;
    lsGen gen;
    lsGen gen1;
    
    bypass_table = st_init_table(st_ptrcmp, st_ptrhash);
    bypasses = lsCreate();
    
    nodes = network_dfs(network);
    for(i = 0; i < array_n(nodes); ++i) {
	/* Is this node critical?  */
	node = array_fetch(node_t *, nodes, i);
	st_lookup(gbx_node_table, (char *) node, (char **) &node_record);
	if(node_record->slack > epsilon) continue;
/*	if(node_record->mark) continue;  */
 	if(node_function(node) == NODE_PO) continue;
	/* If it is, begin a list of the bypasses that begin with
	   this node */
	bypass_fanouts = lsCreate();
	foreach_fanout(node, gen1, fanout) {
	    st_lookup(gbx_node_table, (char *) fanout, (char **) &node_record);
	    if(node_record->slack > epsilon) continue;
	    j = node_get_fanin_index(fanout, node);
	    if(node_record->path_fanin != node) continue;
	    if(node_record->input_phases[j] == BINATE) continue;
	    if(node_record->input_phases[j] == PHASE_UNKNOWN) continue;
	    if(node_function(fanout) == NODE_PO) continue;
	    (void)lsNewBegin(bypass_fanouts, (lsGeneric) fanout, LS_NH);
	}
	/* Now try to extend each bypass */
	foreach_list_item(bypass_fanouts, gen, fanout) {
	    newer_trace_bypass(fanout, node, epsilon, mux_delay, NIL(node_t),
                               model);
	}
    }
    array_free(nodes);
    return bypasses;
}

void
newer_trace_bypass(fanout, node, epsilon, mux_delay, last_bypass_ended, model)
node_t *fanout, *node, *last_bypass_ended;
double epsilon, mux_delay;
delay_model_t model;
{
    double slack, slackoffset, edgeweight;
    node_t *fanin;
    int j;
    bypass_t *bypass;


    /* There is a theorem that this is a candidate bypass if and only if the
       bypass gain is <= the slack on all the side inputs - the slack on
       the original path input.  So compute this here and ensure that this
       invariant holds.  The way this works is simple.  Keep track of the
       minimum slack, subtracting off the bypass gain each time.  When we
       hit zero, we're done. */
    
    slackoffset = retrieve_slack(fanout, node);
    edgeweight = weight(fanout, node);
    slack = 1E29;
    foreach_fanin(fanout, j, fanin) {
       if (fanin == node) continue;
       slack = min(slack, retrieve_slack(fanout, fanin));
    }

    /* If we're using one of the unit delay models, then we have a 2-input 
       AND/OR node network.  In this case, we know KMS will eliminate the 
       first bypassed node, so we can allow a slack on the first node which 
       is one less than the minimum slack required on the other nodes.
       (We check the fanin number just in case this node is a "3-input,
       1-level" mux generated by a previous pass.) */

    if ((start_node_mode) &&
        (model == DELAY_MODEL_UNIT || model == DELAY_MODEL_UNIT_FANOUT) &&
        (node_num_fanin(fanout) <= 2)) {
       if (slack-slackoffset > 0) slack++;
       else return;      
    }
    else if (slack-slackoffset < edgeweight) return;


    /* OK, so we've bypassed at least one node...*/

    bypass = new_bypass(node, fanout, edgeweight, slack-slackoffset,
			retrieve_phase(fanout, node));  
    
    if (bypass->gain > bypass->side_slack) {
	free_bypass(bypass);
	return;
    }

/*  st_lookup(gbx_node_table, fanout, &record);
    record->mark = 1;  */

    extend_bypass(bypass, fanout, mux_delay);
}
void
extend_bypass(bypass, node, mux_delay)
bypass_t *bypass;
node_t *node;
double mux_delay;
{

    node_t *fanout, *dummy_node;
    int i, j;
    bypass_t *new_bypass, *old_bypass;
    lsList fanouts;
    lsGen gen;
    
    
    fanouts = bypass_extension_fanouts(bypass, node);
    while ((bypass->gain <= bypass->side_slack) &&
	   (fanouts != LS_NIL)) {

	i = 1;
	j = lsLength(fanouts);
	foreach_list_item(fanouts, gen, fanout) {
	    assert(node_function(fanout) != NODE_PO);
	    if(i++ == j) break;
	    new_bypass = bypass_dup(bypass);
	    bypass_new_add_node(new_bypass, fanout, node);
	    extend_bypass(new_bypass, fanout, mux_delay);
	}
	bypass_new_add_node(bypass, fanout, node);
	lsDestroy(fanouts,  (void(*)())0);
	node = fanout;
	fanouts = bypass_extension_fanouts(bypass, node);
    }
    bypass->gain -= mux_delay;
    if (bypass->gain <= 0) {
	/* Unmark node; it's not on a bypass now, is it? */
/*	st_lookup(gbx_node_table, node, &record);
	record->mark = 0;
*/
	free_bypass(bypass);
    } 
    else if (st_lookup(bypass_table, (char *) bypass->last_node, (char **) &old_bypass)) {
       if (bypass->gain > old_bypass->gain) {

          /* Delete old_bypass from the table.  Pass in the key as a dummy
	     since st may change it and I don't trust Moore that much.
	     Don't bother freeing the old_bypass since it's already on
	     the bypass list and will get freed in the terminal cleanup
             anyway. */
	    
	  dummy_node = bypass->last_node;
	  (void) st_delete(bypass_table, (char **) &dummy_node, (char **) &old_bypass);

	  register_bypass(bypass);
          printf("Debug - Need to delete previous bypass\n");  
       } 
       else free_bypass(bypass);  
    }
    else register_bypass(bypass);
}

void
clean_global_tables(network)
network_t *network;
{
   
    st_free_table(bypass_table);
}
void
gbx_clean_node_table(network)
network_t *network;
{
    array_t *nodes;
    int i;
    node_t *node;
    node_bp_t *node_record;
    
    nodes = network_dfs(network);

    for(i = 0; i < array_n(nodes); ++i) {
	node = array_fetch(node_t *, nodes, i);
	if(st_lookup(gbx_node_table, (char *) node, (char **) &node_record))
	    destroy_bp_record(node_record);
    }
    st_free_table(gbx_node_table);
    array_free(nodes);
}
void
print_bypass(bypass)
bypass_t *bypass;
{
    lsGen gen;
    node_t *node;
    
    printf("%s Bypass: weight %f nodes %d first node %s last node %s\n",
	   bypass->phase == POS_UNATE?"Noninverting":"Inverting",
	   bypass->gain, lsLength(bypass->bypassed_nodes),
	   node_name(bypass->first_node), node_name(bypass->last_node));
    printf("Nodes: ");
    foreach_list_item(bypass->bypassed_nodes, gen, node) {
	printf("%s ", node_name(node));
    }
    printf("\n");
}

int
do_gbx_transform(network, epsilon, mux_delay, model, new_proc, actual_mux_delay)
network_t *network;
double epsilon, mux_delay, actual_mux_delay;
delay_model_t model;
gbx_trace_t new_proc;
{
    st_table *cut_table;
    array_t *nodes;
    node_t *node;
    bypass_t *bypass;
    lsList bypasses;
    double maxgain;
    int maxweight, critweight, noncritweight, i, totweight;
    char taken, no_improvement;
    lsGen gen;
    int noncritnodes, ntaken, nnottaken;
    lsList *critnodes;
    int maxslack;
    node_bp_t *node_record;
    

    switch(new_proc) {
    case GBX_NEW_TRACE:
	bypasses = new_find_bypass_nodes(network, epsilon, mux_delay, model);
	break;
    case GBX_NEWER_TRACE:
	bypasses = newer_find_bypass_nodes(network, epsilon, mux_delay, model);
	break;
    default:
	bypasses = newer_find_bypass_nodes(network, epsilon, mux_delay, model);
    }
    if(lsLength(bypasses) == 0) {
	clean_global_tables(network);
	lsDestroy(bypasses,  (void(*)())0);
	return 0;
    }

    /* Otherwise we found some bypasses */

    maxgain = 0.0;
    foreach_list_item(bypasses, gen, bypass) {
	maxgain = max(maxgain, bypass->gain);
    }
    maxgain += 1.0;     /* Make sure the minimum weight of any bypass is 1 */
    maxweight = 0;
    foreach_list_item(bypasses, gen, bypass) {
	bypass->weight = (int)(0.5 + maxgain - (bypass->gain)); /* it works...*/
	maxweight = max(maxweight, bypass->weight);
    }
    nodes = network_dfs(network);
    noncritweight = maxweight * lsLength(bypasses) + 1;
    cut_table = st_init_table(st_ptrcmp, st_ptrhash);
    noncritnodes = 0;
    maxslack = (int)(0.5 + epsilon);
    ++maxslack;
    SAFE_ALLOC(critnodes, lsList, maxslack);
    for(i = 0; i < maxslack; ++i) critnodes[i] = lsCreate();
    for(i = 0; i < array_n(nodes); ++i) {
	node = array_fetch(node_t *, nodes, i);
	if(st_lookup(bypass_table, (char *) node, (char **) &bypass)) {
	    st_insert(cut_table, (char *) node, (char *)bypass->weight);
	} else {
	    st_lookup(gbx_node_table, (char *) node, (char **) &node_record);
	    if(node_function(node) == NODE_PI || node_function(node) == NODE_PO)
		continue;
	    if(node_record->slack <= epsilon) {
		critweight = (int) 0.5 + epsilon - node_record->slack;
		(void)lsNewBegin(critnodes[critweight], (lsGeneric) node, LS_NH);
	    } else {
		++noncritnodes;
		 st_insert(cut_table, (char *) node, (char *) noncritweight); 
		/* st_insert(cut_table, node, 0); */
	    }
	}
    }
    /* Algebraically equivalent to
       maxweight * lsLength(bypasses) + noncritweight * noncritnodes + 1 */
    critweight = noncritweight * (noncritnodes + 1);
    totweight = critweight - 1;
	
    for(i = 0; i < maxslack; ++i) {
	totweight += critweight * lsLength(critnodes[i]);
	if(totweight < critweight) { /* Overflow! */
	    totweight = GBX_MAXWEIGHT;
	}
	foreach_list_item(critnodes[i], gen, node) {
	    st_insert(cut_table, (char *) node, (char *)critweight);
	}
	critweight = totweight + 1;
    }
    
    array_free(nodes);

    /* Now just take the cutset */

    nodes = cutset_interface(network, cut_table, MAXINT);

    taken = 0; no_improvement = 0; ntaken = 0; nnottaken = 0;
    for(i = 0; i < array_n(nodes); ++i) {
	node = array_fetch(node_t *, nodes, i);
	if(st_lookup(bypass_table, (char *) node, (char **) &bypass)) {
/* 	    printf("Taking Bypass\n");
	    print_bypass(bypass);  */ 
	    if (take_bypass(bypass, network, model, mux_delay, actual_mux_delay)) {
	       ++ntaken;
	       taken = 1;
/*             com_execute(&network, "verify -m bdd /users/harkness/GBX/bench/5xp1.blif.opt");
               com_execute(&network, "write_eqn"); */
            }

            else ++nnottaken;   
	} else {
	    st_lookup(gbx_node_table, (char *) node, (char **) &node_record);
	    if(node_record->slack == 0.0) no_improvement = 1;
	}
    }
    if(gbx_verbose_mode) {
	printf("%d bypasses taken", ntaken);
     	if (nnottaken == 0) printf("\n");     
  	else printf(" %d bypasses not taken\n", nnottaken);  
    }

    array_free(nodes);
    st_free_table(cut_table);
    lsDestroy(bypasses,  (void(*)())0);
    for(i = 0; i < maxslack; ++i) lsDestroy(critnodes[i], (void(*)())0);
    clean_global_tables(network);
    if ((model == DELAY_MODEL_UNIT || model == DELAY_MODEL_UNIT_FANOUT) &&
        (actual_mux_delay > 1))  
       decomp_tech_network(network, 2, 2);  
    (void)network_sweep(network); /* Clean up the mess, if necessary */
    if(!taken) return 3;
    if(no_improvement) return 2;
    return 1;
}

int
bypass_compare(bypass1, bypass2)
char  **bypass1, **bypass2;
{
    return ((bypass_t *) *bypass2)->gain - ((bypass_t *) *bypass1)->gain;
}  

int
do_all_bypasses(network, epsilon, mux_delay, actual_mux_delay, model,
                new_proc)
network_t *network;
double epsilon, mux_delay, actual_mux_delay;
delay_model_t model;
gbx_trace_t new_proc;
{
    bypass_t *bypass;
    lsList bypasses;
    lsGen gen;
    int ntaken, nnottaken; 
    array_t * bypass_array; 
    int i;

    switch(new_proc) {
    case GBX_NEW_TRACE:
	bypasses = new_find_bypass_nodes(network, epsilon, mux_delay, model);
	break;
    case GBX_NEWER_TRACE:
	bypasses = newer_find_bypass_nodes(network, epsilon, mux_delay, model);
	break;
    default:
	bypasses = newer_find_bypass_nodes(network, epsilon, mux_delay, model);
    }
    if(lsLength(bypasses) == 0) {
	clean_global_tables(network);
	lsDestroy(bypasses,  (void(*)())0);
	return 0;
    } 
    else {

        bypass_array = array_alloc(bypass_t *, 0);
        foreach_list_item(bypasses, gen, bypass) {
           array_insert_last(bypass_t *, bypass_array, bypass);
        }
        array_sort(bypass_array, bypass_compare);

        ntaken = 0; nnottaken = 0; 
        for (i = 0; i < array_n(bypass_array); ++i) {
	    if (take_bypass(array_fetch(bypass_t *, bypass_array, i), network, model,
                            mux_delay, actual_mux_delay)) 
               ++ntaken;
            else ++nnottaken;
        }
        array_free(bypass_array);

    	if (gbx_verbose_mode) {
	   printf(" %d bypasses taken", ntaken);
     	   if (nnottaken == 0) printf("\n");     
  	   else printf(" %d bypasses not taken\n", nnottaken);  
        }
    }

    clean_global_tables(network);
    if ((model == DELAY_MODEL_UNIT || model == DELAY_MODEL_UNIT_FANOUT) &&
        (actual_mux_delay > 1))
       decomp_tech_network(network, 2, 2);  
    (void)network_sweep(network); /* Clean up the mess, if necessary */
    return 1;
}

void
print_epsilon_paths(nodelist, node, epsilon)
lsList nodelist;
node_t *node;
double epsilon;
{
    lsGen gen;
    lsGen  gen1;
    node_t *fanout;
    node_bp_t *record;
    
    if(node_function(node) == NODE_PO) {
	foreach_list_item(nodelist, gen1, node) {
	    printf("%s ", node_name(node));
	}
	printf("\n");
	return;
    } else {
	foreach_fanout(node, gen, fanout) {
	    (void)st_lookup(gbx_node_table, (char *) node, (char **) &record);
	    if(record->slack > epsilon) continue;
	    (void)lsNewEnd(nodelist, (lsGeneric)fanout, LS_NH);
	    print_epsilon_paths(nodelist, fanout, epsilon);
	    lsDelEnd(nodelist, (lsGeneric *)&fanout);
	}
    }
}
	    
	    

void
print_epsilon_network(network, model, epsilon)
network_t *network;
delay_model_t model;
double epsilon;
{
    node_t *pi;
    lsGen gen;
    lsList nodelist;
    node_bp_t *record;
        

    foreach_primary_input(network, gen, pi) {
	st_lookup(gbx_node_table, (char *) pi, (char **) &record);
	if(record->slack >= epsilon) continue;
	nodelist = lsCreate();
	(void)lsNewBegin(nodelist, (lsGeneric) pi, LS_NH);
	print_epsilon_paths(nodelist, pi, epsilon);
	lsDestroy(nodelist, (void(*)())0);
    }
}

int
do_GBX(network, adaptive_epsilon, epsilon, new_trace_procedure, mux_delay, target_delay, target_chosen,
       model, cutset_mode)
network_t **network;
double epsilon, mux_delay, target_delay;
delay_model_t model;
gbx_trace_t new_trace_procedure;
int adaptive_epsilon, cutset_mode;
char target_chosen;
{
    int retcode, ret;
    double actual_mux_delay, initial_artime, artime, lastartime;
    int nbadbypasses;
    network_t *saved_network;

    nbadbypasses = 0; 
    actual_mux_delay = mux_delay;

    if (model == DELAY_MODEL_UNIT || model == DELAY_MODEL_UNIT_FANOUT) 
       decomp_tech_network(*network, 2, 2);
    (void) network_sweep(*network);
    gbx_init_node_table(*network, model);
    (void) delay_latest_output(*network, &initial_artime);
    if(!target_chosen) {
	target_delay = initial_artime/2; /* ambitious? */
    }
    if (adaptive_epsilon) epsilon = initial_artime - target_delay;
    artime = initial_artime;

    while(artime > target_delay) {
        lastartime = artime;
        if (target_chosen && adaptive_epsilon) epsilon = artime - target_delay;
	else if (adaptive_epsilon) epsilon = min(epsilon, artime - target_delay);
	if (gbx_verbose_mode)
	   printf("Searching for bypasses, delay = %f\n", artime);
        saved_network = network_dup(*network); 
        if (cutset_mode) 
	   retcode = do_gbx_transform(*network,  epsilon,  mux_delay, model,    
                                      new_trace_procedure, actual_mux_delay);
        else retcode = do_all_bypasses(*network,  epsilon,  mux_delay, actual_mux_delay, 
				       model, new_trace_procedure); 
	(void) delay_trace(*network, gbx_delay_model);
        (void) delay_latest_output(*network, &artime);
	gbx_clean_node_table(*network);

        if(artime <= target_delay) {
           if (gbx_verbose_mode && target_chosen) 
	      printf("Success! sped up the network to the target\n");
	   return 0;
        }
	if ((artime-lastartime >= 0.0) && (retcode != 0) && (retcode != 3)) {
           if (gbx_verbose_mode)  
	      printf("No improvement - rejecting changes\n");
           network_free(*network);
           *network = saved_network; 
           if (retcode == 1) retcode = 0;
        } 
	else network_free(saved_network);

	switch (retcode) {
	   case 0:
	      if (gbx_verbose_mode)
		 printf("No bypasses found,");
              if ((artime == lastartime) && (mux_delay > 1)) {
	         if (gbx_verbose_mode)
		    printf(" trying again with smaller estimated mux delay\n");
                 mux_delay = 1;
                 saved_network = network_dup(*network); 
   	         (void) delay_latest_output(*network, &lastartime);
	         gbx_init_node_table(*network, model);
                 if (cutset_mode) 
	            ret = do_gbx_transform(*network,  epsilon,  mux_delay, 
                                     model, new_trace_procedure, actual_mux_delay);
                 else ret = do_all_bypasses(*network, epsilon, mux_delay,
                                     actual_mux_delay, model, new_trace_procedure); 
                 if (gbx_verbose_mode) {     
                    switch (ret) {
                       case 0:  printf("No bypasses found\n");  break;
                       case 3:  printf("Cutset not found, no bypasses taken.\n"); break;
                       case 2:  printf("Cutset not found, some bypasses taken.\n"); break; 
                       case 1:  if (cutset_mode)   
		                   printf("Cutset found, bypasses taken.  Gain %f should be > 0\n",
                                          lastartime - artime);
                                else printf("Bypasses taken.  Gain %f\n", 
                                            lastartime - artime); 
                                break; 
                    }
                 } 
	         (void) delay_trace(*network, gbx_delay_model);
	         (void) delay_latest_output(*network, &artime);
	         gbx_clean_node_table(*network);
	         if(artime-lastartime >= 0.0) {
		    if (gbx_verbose_mode)
		       printf("Last shot missed, quitting\n");
                    network_free(*network);
                    *network = saved_network; 
		    return 0;
                 } 
                 else {
		    if(gbx_verbose_mode)
		       printf("Last Shot hit, kicking the can again\n");
		    gbx_init_node_table(*network, model);
                    network_free(saved_network);
                    mux_delay = 2;
	         }   
              }              
	      else {
                 if (gbx_verbose_mode) 
		    printf(" giving up\n");
                 return 0;
              }  
	      break;
           case 3:
	      if (gbx_verbose_mode) 
		 printf("Cutset not found, no bypasses taken.\n");
              if (adaptive_epsilon && !target_chosen && epsilon > 1) {
                 epsilon = epsilon/2;
	         if (gbx_verbose_mode) 
		    printf("Trying again with smaller epsilon\n");
   	         (void) delay_latest_output(*network, &lastartime);
	         gbx_init_node_table(*network, model);
                 saved_network = network_dup(*network); 
	         ret = do_gbx_transform(*network,  epsilon, mux_delay,      
                                  model, new_trace_procedure, actual_mux_delay);
                 if (gbx_verbose_mode) {     
                    switch (ret) {
                       case 0:  printf("No bypasses found\n");  break;
                       case 3:  printf("Cutset not found, no bypasses taken.\n"); break;
                       case 2:  printf("Cutset not found, some bypasses taken.\n"); break; 
                       case 1:  printf("Cutset found, bypasses taken.\n"); break; 
                    }
                 } 
	         (void) delay_trace(*network, gbx_delay_model);
	         (void) delay_latest_output(*network, &artime);
	         gbx_clean_node_table(*network);
	         if(artime-lastartime >= 0.0) {
		    if(gbx_verbose_mode)
		       printf("Last shot missed, quitting\n");
                    network_free(*network);
                    *network = saved_network; 
		    return 0;
	         } 
                 else {
		    if(gbx_verbose_mode)
		       printf("Last Shot hit, kicking the can again\n");
                    network_free(saved_network);
		    gbx_init_node_table(*network, model);
	         }                 
              } 
	      else {
                 if (gbx_verbose_mode) 
		    printf("Giving up\n");
                 return 0;
              }  
	      break;            
	   case 2:
	      (void) delay_latest_output(*network, &lastartime);
	      if(gbx_verbose_mode) {
		 printf("Cutset not found, some bypasses taken.\n");
		 printf("Delay is %f\n", lastartime);
	      }
              if (adaptive_epsilon && !target_chosen && epsilon > 1) {
                 epsilon = epsilon/2;
	         if (gbx_verbose_mode) 
		    printf("Trying again with smaller epsilon\n");
	         gbx_init_node_table(*network, model);
                 saved_network = network_dup(*network); 
	         ret = do_gbx_transform(*network, epsilon, mux_delay, 
                               model, new_trace_procedure, actual_mux_delay);
                 if (gbx_verbose_mode) {     
                    switch (ret) {
                       case 0:  printf("No bypasses found\n");  break;
                       case 3:  printf("Cutset not found, no bypasses taken.\n"); break;
                       case 2:  printf("Cutset not found, some bypasses taken.\n"); break; 
                       case 1:  printf("Cutset found, bypasses taken.\n"); break; 
                    }
                 } 
	         (void) delay_trace(*network, gbx_delay_model);
	         (void) delay_latest_output(*network, &artime);
	         gbx_clean_node_table(*network); 
  	         if (artime-lastartime >= 0.0) {
 		    if (gbx_verbose_mode)
		       printf("Last shot missed, quitting\n");
                    network_free(*network);
                    *network = saved_network; 
		    return 0; 
	         } 
                 else {
	            if(gbx_verbose_mode)
		       printf("Last Shot hit, kicking the can again\n");
                    network_free(saved_network);
		    gbx_init_node_table(*network, model);
	         }
              }
	      else {
                 if (gbx_verbose_mode) 
		    printf("Giving up\n");
                 return 0;
              }  
	      break;
	   case 1:
	      if (gbx_verbose_mode) {
                 if (cutset_mode)   
		    printf("Cutset found, bypasses taken.  Gain %f should be > 0\n",
		           lastartime - artime);
                 else printf("Bypasses taken.  Gain %f\n", lastartime - artime); 
              }

              /* short term cludge to prevent infinite loop */
              if (lastartime - artime <= 0) {  
                 ++nbadbypasses;
                 if (nbadbypasses == 5) {
                    printf("Bypass algorithm failure - quitting infinite loop\n");
                    return 0;  
                 }
              }  
	      gbx_init_node_table(*network, model);
        }
    }

    return 0;		/* normal exit */
} 

/* To be called from KJ's framework */
int
do_bypass_transform(network, epsilon, adaptive_epsilon, mux_delay, model, new_trace_procedure,
                    verbose_mode, cutset_mode)
network_t **network;
double epsilon, mux_delay;
delay_model_t model;
gbx_trace_t new_trace_procedure;
int adaptive_epsilon, verbose_mode, cutset_mode;
{
    double target_delay;
    char target_chosen;

    target_chosen = 0;
    start_node_mode = 1;
    target_delay = target_chosen = 0;
    if (verbose_mode) gbx_verbose_mode = 1;
    if (verbose_mode) print_bypass_mode = 1;

    do_GBX(network, adaptive_epsilon, epsilon, new_trace_procedure, mux_delay, target_delay,
           target_chosen, model, cutset_mode);

    return 0;  
} 
 
