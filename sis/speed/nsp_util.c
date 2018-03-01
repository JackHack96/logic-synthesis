/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/speed/nsp_util.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:51 $
 *
 */
#include "sis.h"
#include "speed_int.h"
#include "buffer_int.h"
#include "new_speed_models.h"            /* Definition of "local_transforms" */
#include "gbx_int.h"

#include <signal.h>
#include <setjmp.h>

static node_t *speed_node_dup(); /* For fast duplication of nodes */

static void sp_map_if_required();
static void sp_adjust_input_arrival();
static node_t *sp_find_substitute();
static void nsp_split_fanouts();
int nsp_fanout_compare();

#define LIMIT_NUM_CUBES 200

/*
 * Pretty printing of relevant delay information
 */
void
sp_print_delay_data(fp, network)
FILE *fp;
network_t *network;
{
    lsGen gen;
    node_t *node;
    double area = 0.0;
    int n_failing_outputs;
    delay_time_t arrival, required, slack;
    delay_time_t max_slack, min_slack, sum_slack, max_arrival;

    n_failing_outputs = 0;
    max_slack.rise = max_slack.fall = NEG_LARGE;
    min_slack.rise = min_slack.fall = POS_LARGE;
    sum_slack.rise = sum_slack.fall = 0.0;
    max_arrival.rise = max_arrival.fall = NEG_LARGE;
    foreach_primary_output(network, gen, node) {
	arrival = delay_arrival_time(node);
	required = delay_required_time(node);
	slack = delay_slack_time(node);
	if (MIN(slack.rise, slack.fall) < 0) {
	    n_failing_outputs++;
	    sum_slack.rise += (slack.rise < 0) ? slack.rise : 0.0;
	    sum_slack.fall += (slack.fall < 0) ? slack.fall : 0.0;
	}
	max_slack.rise = MAX(max_slack.rise, slack.rise);
	max_slack.fall = MAX(max_slack.fall, slack.fall);
	min_slack.rise = MIN(min_slack.rise, slack.rise);
	min_slack.fall = MIN(min_slack.fall, slack.fall);
	max_arrival.rise = MAX(max_arrival.rise, arrival.rise);
	max_arrival.fall = MAX(max_arrival.fall, arrival.fall);
    }
    area = sp_get_netw_area(network);
  
    (void) fprintf(fp, "# of outputs:          %d\n", network_num_po(network));
    (void) fprintf(fp, "total gate area:       %2.2f\n", area);
    (void) fprintf(fp, "maximum arrival time: (%2.2f,%2.2f)\n",
	    max_arrival.rise, max_arrival.fall);
    (void) fprintf(fp, "maximum po slack:     (%2.2f,%2.2f)\n",
	    max_slack.rise, max_slack.fall);
    (void) fprintf(fp, "minimum po slack:     (%2.2f,%2.2f)\n",
	    min_slack.rise, min_slack.fall);
    (void) fprintf(fp, "total neg slack:      (%2.2f,%2.2f)\n",
	    sum_slack.rise, sum_slack.fall);
    (void) fprintf(fp, "# of failing outputs:  %d\n", n_failing_outputs);
}
/*
 * In case the fanin of the PO node is a buffer/inverter get the arrival
 * for the previous node since this node will disappear on collapsing
 */
void
new_speed_adjust_po_arrival(rec, model, arrival_p)
sp_clp_t *rec;
delay_model_t model;
delay_time_t *arrival_p;
{
    delay_time_t t;
    node_t *node, *po;
    network_t *network = rec->net;

    /* No adjustment in the case of mapped delay computations */
    if (model == DELAY_MODEL_MAPPED) return;

    po = network_get_po(network, 0);
    node = node_get_fanin(po, 0);
    if (rec->can_adjust && node_num_fanin(node) == 1){
	/*
	 * Adjustment is made to the delay ot the PO node
	 */
	t = nsp_compute_delay_saving(po, node, model);
	if (rec->glb->debug > 1){
	    (void)fprintf(sisout, "\tINVERTER at the root adjusts %.2f:%-.2f\n",
		    t.rise, t.fall);
	}
	arrival_p->rise -= t.rise;
	arrival_p->fall -= t.fall;
    }
}
/*
 * Assumes that the "node" is a fanin of "po" and that node can be
 * deleted. The routine computes the savings that would result
 */
delay_time_t
nsp_compute_delay_saving(po, node, model)
node_t *po, *node;
delay_model_t model;
{
    delay_pin_t *pin_delay;
    delay_time_t drive, t;
    double po_load, load;
    node_t *node1;

    pin_delay = get_pin_delay(node, 0, model);
    t = delay_node_pin(node, 0, model);
    load = delay_get_load((char *)pin_delay);
    node1 = node_get_fanin(node, 0);
    pin_delay = get_pin_delay(node1, 0, model);
    drive = delay_get_drive((char *)pin_delay);
/*    po_load = delay_load(po);*/
    assert(delay_get_po_load(po, &po_load));
    t.rise -= drive.rise * (po_load - load);
    t.fall -= drive.fall * (po_load - load);

    return t;
}

void
sp_print_network(network, comment)
network_t *network;
char *comment;
{
    int i;
    lsGen gen;
    node_t *node;
    lib_gate_t *gate;
    array_t *network_array;

    (void)fprintf(sisout, "%s\n", comment);

    (void)fprintf(sisout, "--in/outs\n    ");
    foreach_primary_input(network, gen, node){
        (void)fprintf(sisout, "%s ", node_long_name(node));
    }
    (void)fprintf(sisout, " -- ");
    foreach_primary_output(network, gen, node){
        (void)fprintf(sisout, "%s ", node_long_name(node));
    }
    (void)fprintf(sisout, "\n");

    (void)fprintf(sisout, "--network\n");
    network_array = network_dfs_from_input(network);
    for (i = 0; i < array_n(network_array); i++) {
        node = array_fetch(node_t *, network_array, i);
	if ((gate = lib_gate_of(node)) != NIL(lib_gate_t)){
	    (void)fprintf(sisout, "%s\t", lib_gate_name(gate));
	}
        node_print(stdout, node);
    }
    array_free(network_array);

}

/*
 * Account for the fact that the original arrival times at the inputs
 * also had the load of the collapsed network. If these nodes are not
 * duplicated (load is retained on duplication) then the arrival times
 * need to be reduced so as not to be pessimistic
 */
static void
sp_adjust_input_arrival(rec, network)
sp_clp_t *rec;
network_t *network;
{
     int i;
     double load;
     lsGen gen, gen1;
     delay_pin_t *pin_delay;
     delay_time_t *tp, t, drive, old_arr;
     node_t *pi, *new, *orig, *fo, *orig_fo;

     rec->adjust = st_init_table(st_ptrcmp, st_ptrhash);
     foreach_primary_input(rec->orig_config, gen, pi){
         new = network_find_node(rec->net, node_long_name(pi));
         if (new == NIL(node_t)) continue;

         assert(delay_get_pi_arrival_time(new, &old_arr));
         orig = nsp_network_find_node(network, node_long_name(pi));
         load = 0;
         foreach_fanout(pi, gen1, fo){
             orig_fo = nsp_network_find_node(network, node_long_name(fo));
             if (orig_fo != NIL(node_t)){
             /*
              * The fanout is retained because of duplication --- do not
              * need to discount its load
              */
        	 continue;
             } else {
        	 assert((i = node_get_fanin_index(fo, pi)) >= 0);
        	 pin_delay = get_pin_delay(fo, i, rec->glb->model);
        	 load += pin_delay->load;
             }
         }
         pin_delay = get_pin_delay(orig, 0, rec->glb->model);
         drive = delay_get_drive((char *) pin_delay);

         t.rise  = drive.rise * load;
         t.fall  = drive.fall * load;
         delay_set_parameter(new, DELAY_ARRIVAL_RISE, old_arr.rise - t.rise);
         delay_set_parameter(new, DELAY_ARRIVAL_FALL, old_arr.fall - t.fall);

         /* Keep track of the adjustment for PRIMARY INPUTS */
         if (orig->type == PRIMARY_INPUT){
             tp = ALLOC(delay_time_t, 1);
             *tp = t;
             (void)st_insert(rec->adjust, (char *)orig, (char *)tp);
         }
     }
}

/*
 * Create a record of the collapsing to be used later.
 */
sp_clp_t *
sp_create_collapse_record(node, wght, speed_param, ti_model)
node_t *node;
sp_weight_t *wght;
speed_global_t *speed_param;
sp_xform_t *ti_model;
{
    sp_clp_t *rec;             /* record of the collapsing information */

    rec = ALLOC(sp_clp_t, 1);
    rec->node = node;
    rec->name = util_strsav(node_long_name(node));
    rec->glb = speed_param;
    rec->delta = NIL(array_t);
    rec->adjust = NIL(st_table);
    /*
     * The inverter at the output can be removed if it does not feed a PO
     */
    rec->can_adjust = (1 - new_speed_is_fanout_po(node));
    rec->ti_model = ti_model;

    if (ti_model->type == FAN){
	rec->net = speed_network_dup(wght->fanout_netw);
	/* NOTE: The rec->old is set in the calling code ... */
    } else if (ti_model->type == DUAL){
	rec->net = speed_network_dup(wght->dual_netw);
	printf("Was rec->old set properly\n");
    } else {
	rec->net = speed_network_dup(wght->clp_netw);
	rec->old = delay_arrival_time(node);
    }
    network_set_name(rec->net, node_long_name(node));
    rec->orig_config = speed_network_dup(rec->net);

    if (ti_model->type != FAN){
	/* Adjust the arrival time to account for the removed load */
	sp_adjust_input_arrival(rec, node_network(node));
    }

    return rec;
}

 /* Free the collapse record: */
void
nsp_free_collapse_record(rec)
sp_clp_t *rec;
{
    node_t *node;
    delay_time_t *tp;
    st_generator *stgen;

    network_free(rec->net);
    network_free(rec->orig_config);
    if (rec->delta != NIL(array_t)) array_free(rec->delta);
    if (rec->adjust != NIL(st_table)){
	st_foreach_item(rec->adjust, stgen, (char **)&node, (char **)&tp){
	    FREE(tp);
	}
	st_free_table(rec->adjust);
    }
    st_free_table(rec->equiv_table);
    FREE(rec->name);
    FREE(rec);
}

/*
 * Returns true if the fanout of this node is a primary output
 */
int
new_speed_is_fanout_po(node)
node_t *node;
{
    lsGen gen;
    node_t *fo;

    foreach_fanout(node, gen, fo){
	if (node_function(fo) == NODE_PO) {
	    (void) lsFinish(gen);
	    return TRUE;
	}
    }
    return FALSE;
}

/*
 * Allocate and create a table of the primary output nodes which 
 * do not have user-defined required time constraints
 */
st_table *
speed_store_required_times(network)
network_t *network;
{
    lsGen gen;
    node_t *po;
    st_table *table;
    delay_time_t req;

    table = st_init_table(st_ptrcmp, st_ptrhash);
    foreach_primary_output(network, gen, po){
	if (!delay_get_po_required_time(po, &req)){
	    req = delay_required_time(po);
	    delay_set_parameter(po, DELAY_REQUIRED_RISE, req.rise);
	    delay_set_parameter(po, DELAY_REQUIRED_FALL, req.fall);
	    (void)st_insert(table, (char *)po, NIL(char));
	}
    }
    return table;
}

/*
 * Unset the required times for the primary output nodes in "table"
 */
void
speed_restore_required_times(table)
st_table *table;
{
    node_t *po;
    char *dummy;
    st_generator *stgen;

    st_foreach_item(table, stgen, (char **)&po, (char **)&dummy){
	delay_set_parameter(po, DELAY_REQUIRED_RISE, DELAY_NOT_SET);
	delay_set_parameter(po, DELAY_REQUIRED_FALL, DELAY_NOT_SET);
    }
    st_free_table(table);
}

/*
 * For all the fanouts of "node" assigns them "new_fanin" as the fanin
 * essentially shifts all fanouts of "node" to be fanouts of "new_fanin"
 */
void
sp_patch_fanouts_of_node(node, new_fanin)
node_t *node, *new_fanin;
{
    int j=0;
    lsGen gen;
    node_t **fo_array, *fanout;
    
    fo_array = ALLOC(node_t *, node_num_fanout(node));
    foreach_fanout(node, gen, fanout){
	fo_array[j++] = fanout;
    }
    for (j = node_num_fanout(node); j-- > 0; ){
	node_patch_fanin(fo_array[j], node, new_fanin);
    }
    FREE(fo_array);
}

/*
 * Routine to create the global representation for the technology-indep
 * restructuring procedures
 */
array_t *
sp_get_local_trans(n_entries, entries)
int n_entries;
char **entries;
{
    int i, j, num;
    sp_xform_t *ti_model;
    static array_t *array_of_methods = NIL(array_t);

    if (array_of_methods == NIL(array_t)){
	array_of_methods = array_alloc(sp_xform_t *, 0);

	num = sizeof(local_transforms) / sizeof(local_transforms[0]);

	for (i = 0; i < num; i++) {
	    ti_model = ALLOC(sp_xform_t, 1);
	    *ti_model = local_transforms[i];
/*
	    ti_model->name = local_transforms[i].name;
	    ti_model->optimize_func = local_transforms[i].optimize_func;
	    ti_model->arr_func = local_transforms[i].arr_func;
	    ti_model->priority = local_transforms[i].priority;
	    ti_model->on_flag = local_transforms[i].on_flag;
	    ti_model->type = local_transforms[i].type;
*/
	    array_insert_last(sp_xform_t *, array_of_methods, ti_model);
	}
    }
    if (n_entries == 0) return array_of_methods;

    /* Reset all the techniques except the one that does no transformation */
    for (i = 0; i < array_n(array_of_methods); i++){
	ti_model = array_fetch(sp_xform_t *, array_of_methods, i);
	/* ti_model->on_flag = (strcmp(ti_model->name, "noalg") == 0); */
	ti_model->on_flag = FALSE;
    }
    for (i = 0; i < array_n(array_of_methods); i++){
	ti_model = array_fetch(sp_xform_t *, array_of_methods, i);
	for (j = 0; j < n_entries; j++){
	    if (strcmp(ti_model->name, entries[j]) == 0){
		ti_model->on_flag = 1;
	    }
	}
    }
    
    return array_of_methods;
}

/*
 * Free the global data-structures that store the technology-indep
 * restructuring methods
 */
void
sp_free_local_trans(p_array_of_methods)
array_t **p_array_of_methods;
{
    int i;
    sp_xform_t *ti_model;

    if (*p_array_of_methods == NIL(array_t)) return;
    for (i = 0; i < array_n(*p_array_of_methods); i++){
       ti_model = array_fetch(sp_xform_t *, *p_array_of_methods, i);
       FREE(ti_model);
    }
    array_free(*p_array_of_methods);
    *p_array_of_methods = NIL(array_t);
}

/*
 * Print the methods currently in use
 */
void
sp_print_local_trans(array_of_methods)
array_t *array_of_methods;
{
    int i;
    sp_xform_t *ti_model;

    if (array_of_methods == NIL(array_t)) return;
    (void)fprintf(sisout, "Tech-Indep methods in use: ");
    for (i = 0; i < array_n(array_of_methods); i++){
       ti_model = array_fetch(sp_xform_t *, array_of_methods, i);
       if (ti_model->on_flag) 
	   (void)fprintf(sisout, "\"%s\" ", ti_model->name);
    }
    (void)fprintf(sisout, "\n");

    (void)fprintf(sisout, "Tech-Indep methods not in use: ");
    for (i = 0; i < array_n(array_of_methods); i++){
       ti_model = array_fetch(sp_xform_t *, array_of_methods, i);
       if (! ti_model->on_flag) 
	   (void)fprintf(sisout, "\"%s\" ", ti_model->name);
    }
    (void)fprintf(sisout, "\n");
}
/*
 * Return the number of active methods
 */
int
sp_num_active_local_trans(speed_param)
speed_global_t *speed_param;
{
    int i, n;
    array_t *array_of_methods = speed_param->local_trans;
    sp_xform_t *ti_model;

    if (array_of_methods == NIL(array_t)) return 0;
    for (i = 0, n = 0; i < array_n(array_of_methods); i++){
       ti_model = array_fetch(sp_xform_t *, array_of_methods, i);
       if (ti_model->on_flag) n++;
    }
    return n;
}
/*
 * Return the number of active  methodsof a certain type
 */
int
sp_num_local_trans_of_type(speed_param, type)
speed_global_t *speed_param;
int type;             /* one of CLP, DUAL, FAN  */
{
    int i, n;
    array_t *array_of_methods = speed_param->local_trans;
    sp_xform_t *ti_model;

    if (array_of_methods == NIL(array_t)) return 0;
    if (type != CLP && type != FAN && type != DUAL) return -1;

    for (i = 0, n = 0; i < array_n(array_of_methods); i++){
       ti_model = array_fetch(sp_xform_t *, array_of_methods, i);
       if (ti_model->on_flag && ti_model->type == type) n++;
    }
    return n;
}
/*
 * Return a pointer the sp_xform_t strcture, given its position in the array
 */
sp_xform_t *
sp_local_trans_from_index(speed_param, n)
speed_global_t *speed_param;
int n;
{
    array_t *array_of_methods = speed_param->local_trans;
    sp_xform_t *ti_model;

    if (array_of_methods == NIL(array_t)) return NIL(sp_xform_t);
    if ((n < 0) || (n >= array_n(array_of_methods))) return NIL(sp_xform_t);
    ti_model = array_fetch(sp_xform_t *, array_of_methods, n);
    return ti_model;
}

 /* Must be called when you are sure that the name corresponds to an edge
    in the original network */
void
nsp_get_orig_edge(network, name, fo, index)
network_t *network;        /* The original network */
char *name;		   /* A composite name of a PO node */
node_t **fo;		   /* The returned fanout */
int *index;		   /* The fanin-index of this edge */
{
    char *pos;
    assert((pos = strrchr(name, NSP_OUTPUT_SEPARATOR))); 
    *pos = '\0';
    *index = atoi(pos+1);
    *fo = network_find_node(network, name);
    *pos = NSP_OUTPUT_SEPARATOR;
    return;
}
 /* Returns a node corresponding to the name that may be a madeup name */
node_t *
nsp_network_find_node(network, name)
network_t *network;
char *name;
{
    char *pos;
    node_t *node;

    if ((pos = strrchr(name, NSP_INPUT_SEPARATOR))){
	*pos = '\0';
	node = network_find_node(network, name);
	*pos = NSP_INPUT_SEPARATOR;
    } else if ((pos = strrchr(name, NSP_OUTPUT_SEPARATOR))){
	*pos = '\0';
	node = network_find_node(network, name);
	*pos = NSP_OUTPUT_SEPARATOR;
    } else {
	node = network_find_node(network, name);
    }
    return node;
}

 /* Given a network that corresponds to a transformed subnetwork, delete it
  * from the network. Add PO's and PI's to allow the modified network to be
  * reinstated by sp_append_network()
  */
void
sp_delete_network(speed_param, network, node, delete_network, select_table,
		  clp_table, equiv_table) 
speed_global_t *speed_param; /* Gloabsl stuff */
network_t *network;	   /* The original network */
node_t *node;		   /* The root node !!! */
network_t *delete_network; /* The network to be deleted */
st_table *select_table;	   /* Table of all the nodes in the selection */
st_table *clp_table;	   /* Table of all th eweight computation data */
st_table *equiv_table;	   /* Table maintaining equivalences !!! */
{
    int index;
    char *name, *pos;
    lsGen gen, gen1;
    node_t *po, *np, *temp, *new_pi, *new_po, *input, *orig_input;
    node_t *special_pi, *actual_node, *corresp_fi, *corresp_node, *po_fi;
    network_t *new_net;
    sp_xform_t *ti_model, *new_ti_model;
    sp_weight_t *wght;
    double val;
    delay_time_t t;


    assert(st_lookup(clp_table, (char *)node, (char **)&wght));
    
    /* Because of the peculiar way in which this routine is called, we
     * may have the "node" not be part of "network". The hack is to
     * detect this and set the node to be an appropriate one
     */
    if (network != node_network(node)){
	temp = network_find_node(network, node->name);
	if (temp == NIL(node_t)){
	    fail("Unable to find node in duplicated network\n");
	} else {
	    node = temp;
	}
    }
    
    ti_model = sp_local_trans_from_index(speed_param,
					 wght->best_technique);

    foreach_primary_input(delete_network, gen, input){
	if ((pos = strrchr(input->name, NSP_INPUT_SEPARATOR))){
	    *pos = '\0';
	    orig_input = network_find_node(network, input->name);
	    *pos = NSP_INPUT_SEPARATOR;
	} else {
	    orig_input = network_find_node(network, node_long_name(input));
	}
	if (orig_input == NIL(node_t)){
	    (void)fprintf(siserr, "Unable to find node %s\n", input->name);
	    fail("SERIOUS ERROR: fix it please !!!");
	}
	/* First check if the orig_input is also being transformed. In
	   that case a link between the two needs to be established
	   This is a special case ....
	   */
	special_pi = NIL(node_t);
	if ((strcmp(input->name, orig_input->name) != 0 &&
	     st_is_member(select_table, (char *)orig_input)) ||
	    (orig_input->type == INTERNAL)) { /* Add a PO node */
	    
	    (void)st_lookup(clp_table, (char *)orig_input, (char
							    **)&wght);
	    new_ti_model = sp_local_trans_from_index(speed_param,
						     wght->best_technique);
	    
	    if (strcmp(input->name, orig_input->name) != 0 && 
		st_is_member(select_table, (char *)orig_input) &&
		new_ti_model->type != CLP){
		if (speed_param->debug) {
		    (void)fprintf(sisout, "FOUND SPECIAL CASE\n");
		}
		/* SITUATION: A and B are being transformed, A is fanin
		   of B. The cutest ordering puts A before B.
		   Need to get B-0 while at B knowing that A has
		   already  created B-0 as PI... we want B-0 to fanout
		   to A-0 so that later on they will be connected
		   
		   Otherwise there would be a PI (B-0) and PO (A-0)
		   without a connection between them.*/
		
		new_net = wght->fanout_netw;
		/* Look for the appropriate PO of the fanin config */
		foreach_primary_output(new_net, gen1, po_fi){
		    corresp_node = nsp_network_find_node(delete_network,
							 po_fi->name);
		    if (corresp_node != NIL(node_t)){
			/* This could be the PO of the network too !!! */
			if (corresp_node->type == PRIMARY_OUTPUT){
			    corresp_node = node_get_fanin(corresp_node, 0);
			}
			/* Ensure that this is the correct connection */
			assert((pos = strrchr(po_fi->name, NSP_OUTPUT_SEPARATOR)));
			*pos = '\0';
			index = atoi(pos+1);
			*pos = NSP_OUTPUT_SEPARATOR;
			corresp_fi = node_get_fanin(corresp_node, index);
			
			if (strcmp(corresp_fi->name, input->name) == 0){
			    special_pi = network_find_node(network, po_fi->name);
			    assert(special_pi != NIL(node_t) && special_pi->type == PRIMARY_INPUT);
			    lsFinish(gen1);
			    break;
			}
		    }
		}
		assert(special_pi != NIL(node_t));
		actual_node = special_pi;
	    }  else {
		/* Just a regular fanin connection !!! */
		actual_node = orig_input;
	    }
	    
	    /* Add (if required) a PO node and set its delay values */
	    t = delay_required_time(orig_input);
	    if ((new_po = network_find_node(network,
					    input->name)) != NIL(node_t) &&
		new_po->type == PRIMARY_OUTPUT){ 
		/* This node is fanin to more than one transform */
		val = delay_get_parameter(new_po, DELAY_REQUIRED_RISE);
		delay_set_parameter(new_po, DELAY_REQUIRED_RISE, 
				    MIN(val, t.rise));
		val = delay_get_parameter(new_po, DELAY_REQUIRED_FALL);
		delay_set_parameter(new_po, DELAY_REQUIRED_FALL,
				    MIN(val, t.rise));
		val = delay_get_parameter(new_po, DELAY_OUTPUT_LOAD);
		delay_set_parameter(new_po, DELAY_OUTPUT_LOAD,
				    val + delay_load(orig_input));
	    } else {
#ifdef SIS
		new_po = network_add_fake_primary_output(network,
							 actual_node);
#else
		new_po = network_add_primary_output(network, actual_node);
		network_swap_names(network, actual_node, new_po);
#endif /* SIS */
		network_change_node_name(network, new_po, util_strsav(input->name));
		delay_set_parameter(new_po, DELAY_REQUIRED_RISE, t.rise);
		delay_set_parameter(new_po, DELAY_REQUIRED_FALL, t.fall);
		delay_set_parameter(new_po, DELAY_OUTPUT_LOAD,
				    delay_load(orig_input));
	    }
	    if (equiv_table != NIL(st_table)) {
		st_insert(equiv_table, input->name, (char *)new_po);
	    }
	} else {
	    if (equiv_table != NIL(st_table)) {
		st_insert(equiv_table, input->name, (char *)orig_input);
	    }
	}
	/* HERE --- set the delay data for the recursion */
    }
    /*
     * Now add primary input nodes to the original network that
     * correspond to the PO's of the resynthesised sections
     */
    if (ti_model->type != CLP){
	foreach_primary_output(delete_network, gen, po){
	    name = util_strsav(po->name);
	    /* Get the fanin index of the fanout node from this */
	    assert((pos = strrchr(name, NSP_OUTPUT_SEPARATOR)));
	    *pos = '\0';
	    index = atoi(pos+1);
	    np = network_find_node(network, name);
	    *pos = NSP_OUTPUT_SEPARATOR;
	    
	    new_pi = node_alloc();
	    network_add_primary_input(network, new_pi);
	    network_change_node_name(network, new_pi, name);
	    if (np->type == PRIMARY_INPUT){
		/* The "np" is also selected for transformantion and
		   has been already made into a PI node */
	    } else {
		node_patch_fanin_index(np, index, new_pi);
	    }
	    if (equiv_table != NIL(st_table)) {
		st_insert(equiv_table, name, (char *)new_pi);
	    }
	}
    } else {
	/* Add a PI node corresponding to output of root node */
	new_pi = node_alloc();
	network_add_primary_input(network, new_pi);
	
	/* Change  pointers for all fanouts to take new_pi as fanin */
	sp_patch_fanouts_of_node(node, new_pi);
	
	/* HERE --- set the adjusted arrival time at the PI */
	
	name = util_strsav(node->name);
	network_delete_node(network, node);
	np = network_find_node(network, name);
	assert(np == NIL(node_t));
	network_change_node_name(network, new_pi, name);
    }
}

/* Routine to keep track of nodes in the original network and added nodes */
node_t *nsp_find_equiv_node(network, equiv_name_table, name)
network_t *network;
st_table *equiv_name_table;
char *name;
{
    node_t *orig;
    char *n, *new_n;

    n = name;
    if ((orig = network_find_node(network, n)) == NIL(node_t)){
	while (st_lookup(equiv_name_table, n, &new_n)){
	    n = new_n;
	}
	orig =  network_find_node(network, n);
    }
    return orig;
}

/*
 * Adds to the "network" the decomposition that is contained in
 * "append_network". On doing that it will add the "root" node to
 * the array "roots" so that appropriate steps can be taken later
 */
void
sp_append_network(speed_param, network, append_network, ti_model,
		  equiv_name_table, roots)  
speed_global_t *speed_param;
network_t *network;
network_t *append_network;
sp_xform_t *ti_model;
st_table *equiv_name_table;
array_t *roots;
{
    int i, j, root_deleted;
    array_t *nodevec, *array;
    st_table *gate_table;
    lib_gate_t *gate;
    char *pos;
    node_t *node, *fanin, *newnode, *orig, *root, *resub_node;

    array = array_alloc(node_t *, 0);
    nodevec = network_dfs_from_input(append_network);
    gate_table = st_init_table(st_ptrcmp, st_ptrhash);
    for (i = 0; i < array_n(nodevec); i++){
        node = array_fetch(node_t *, nodevec, i);
	newnode = node_dup(node);
	node->copy = newnode;
	if ((gate=lib_gate_of(node)) != NIL(lib_gate_t)){
	    (void)st_insert(gate_table, (char *)newnode, (char *) gate);
	}
	array_insert_last(node_t *, array, newnode);
    }

    /* Change the fanin pointers */
    for (i = 0; i < array_n(array); i++){
        node = array_fetch(node_t *, array, i);
        FREE(node->fanin_fanout);
        node->fanin_fanout = ALLOC(lsHandle, node->nin);
        foreach_fanin(node, j, fanin){
            if (fanin->type == PRIMARY_INPUT) {
                /* patch the fanin to original */
		orig = nsp_network_find_node(network, fanin->name);
	        if (orig != NIL(node_t)){
		    if (orig->type == PRIMARY_OUTPUT){
			orig = node_get_fanin(orig, 0);
		    }
                } else if ((pos = strrchr(fanin->name, NSP_INPUT_SEPARATOR)) != NULL){
		    /* fanin->name may be a PI (does not have the separator) */
		    *pos = '\0';
		    orig = nsp_find_equiv_node(network, equiv_name_table,
					       fanin->name); 
		    *pos = NSP_INPUT_SEPARATOR;
		    if (orig == NIL(node_t)){
			fail("Failed(1) to find node corresponding to input");
		    }
		} else {
                    fail("Failed(2) to find node corresponding to input");
                }
		node->fanin[j] = orig;
            }
            else {
                /* update the fanin pointer */
                node->fanin[j] = fanin->copy ;
            }
        }
    }
    array_free(nodevec);

    /*
     * Nodes in the original network lost their names when the
     * dummy PO nodes were added... Now restore these names
     */
/*
    foreach_primary_input(append_network, gen, node){
	orig = nsp_network_find_node(network, node_long_name(node));
	if (orig != NIL(node_t) && orig->type == PRIMARY_OUTPUT){
	    name = util_strsav(node_long_name(orig));
	    temp = node_get_fanin(orig, 0);
	    network_delete_node(network, orig);
	    network_change_node_name(network, temp, name);
	}
    }
*/

    /* Now add the elements of the array into the original network */
    nodevec = array_alloc(node_t *, 0);	/* To store the added nodes */
    for ( i = array_n(array); i-- > 0; ){
	node = array_fetch(node_t *, array, i);
	if (node->type == INTERNAL){
	    /* Add it to the network --- pointers are correct at this point */
	    orig =  network_find_node(network, node_long_name(node));
	    if (orig != NIL(node_t)){
	    /*
	     * Node with same name may exist --- especially if the decomp
	     * is rejected and the original configuration is being used 
	     */
		FREE(node->name);
	    }
	    network_add_node(network, node);
	    array_insert_last(node_t *, nodevec, node);
	    if (st_lookup(gate_table, (char *)node, (char **)&gate)){
		buf_add_implementation(node, gate);
	    }
	} else if (node->type == PRIMARY_OUTPUT) {
	    /*
	     * Patch the output of the subnetwork to where it belongs 
	     */
	    root = node_get_fanin(node, 0);
	    orig = network_find_node(network, node->name);
	    assert(orig != NIL(node_t) && orig->type == PRIMARY_INPUT);
	    sp_patch_fanouts_of_node(orig, root);
	    /* Delete the fake primary input of the network */
	    network_delete_node(network, orig);

	    if (ti_model->type == CLP){
		/* Restore the name to the root --- needed for later use */
		network_change_node_name(network, root, util_strsav(node->name));
	    }

	    /* Free the primary output of the subnetwork */
	    node_free(node);
	} else {
	    /* Get rid of the fake primary outputs in the old network */
	    orig = network_find_node(network, node_long_name(node));
	    if (orig != NIL(node_t) && orig->type == PRIMARY_OUTPUT){
		network_delete_node(network, orig);
	    }
	    /* Just free the fake primary input of the sub-network */
	    node_free(node);
	}
    }

    /* See if the added nodes can be resubstituted in the original netw */
    root_deleted = FALSE;
    if (ti_model->type == CLP && speed_param->area_reclaim){
	for (i = 0; i < array_n(nodevec); i++){
	    node = array_fetch(node_t *, nodevec, i);
	    resub_node = sp_find_substitute(network, node, speed_param->model);
	    if (resub_node != NIL(node_t)) {
		sp_patch_fanouts_of_node(node, resub_node);
		if (node == root) {
		    root_deleted = TRUE;
		}
		(void)st_insert(equiv_name_table, util_strsav(node->name),
				util_strsav(resub_node->name)); 
		network_delete_node(network, node);
	    }
	}
    }
    if (root != NIL(node_t) && !root_deleted){
	array_insert_last(node_t *, roots, root);
    }
    array_free(nodevec);
    array_free(array);
    st_free_table(gate_table);
}
/*
 * Given a "node" finds another node that is identical to it....
 * Done by doing a ntbdd_verify_network() of the network created by "node" and
 * another network that corresponds to another node at the same level.
 */

#define NSP_PO_NAME "po-_-# name"

/* ARGSUSED */
static node_t *
sp_find_substitute(network, node, model)
network_t *network;
node_t *node;
delay_model_t model;
{
    int nin;
    lsGen gen;
    node_t *fanin, *fanin2, *fo, *po, *match;
    network_t *net1, *net2;

    match = NIL(node_t);
    if ((node->type != INTERNAL) ||
	    ((nin = node_num_fanin(node)) < 2)) return match;

    net1 = network_create_from_node(node);
    po = network_get_po(net1, 0);
    network_change_node_name(net1, po, util_strsav(NSP_PO_NAME));
    fanin = node_get_fanin(node, 0);
    fanin2 = node_get_fanin(node, 1);
    foreach_fanout(fanin, gen, fo){
	/* Do the obvious checks first */
	if ((fo == node) || (fo->type == PRIMARY_OUTPUT)) continue;
	if (node_get_fanin_index(fo, fanin2) < 0) continue;
	if (node_num_fanin(fo) !=  nin) continue;

	/*
	 * The resubstitution is accceptable only if the replacement node
	 * does not add undue load to the node on the critical path --- the
	 * one for which the substitute is being found
	 */
	if (delay_compute_fo_load(fo, model) > 1 &&
		 delay_compute_fo_load(node, model) > 1){
	    continue;
	}

	/* Now check if the functionality is the same */
	net2 = network_create_from_node(fo);
	po = network_get_po(net2, 0);
	network_change_node_name(net2, po, util_strsav(NSP_PO_NAME));
	if (ntbdd_verify_network(net1, net2, DFS_ORDER, ALL_TOGETHER)){
	    match = fo;
	    (void)lsFinish(gen);
	    network_free(net2);
	    break;
	}
	network_free(net2);
    }
    network_free(net1);

    return match;
}
#undef NSP_PO_NAME

/*
 * Free the side path information that was stored as part of the weight
 * collapsing.
 */
void
new_free_weight_info(clp_table)
st_table *clp_table;
{
    node_t *np;
    st_generator *stgen;
    sp_weight_t *wght;

    st_foreach_item(clp_table, stgen, (char **)&np, (char **)&wght){
	FREE(wght->improvement);
	FREE(wght->area_cost);
	sp_network_free(wght->clp_netw);    
	sp_network_free(wght->fanout_netw);
	sp_network_free(wght->dual_netw);
	sp_network_free(wght->best_netw);
	FREE(wght);
    }
    st_free_table(clp_table);
}

 /* 
  * Get the input_load_limit based on the load increase that can be
  * tolerated along the connection between "fanin" and "node"
  */
double
nsp_get_load_limit(node, fanin_index, speed_param)
node_t *node;
int fanin_index;
speed_global_t *speed_param;
{
    lsGen gen;
    delay_pin_t *pin_delay;
    node_t *fo, *fanin = node_get_fanin(node, fanin_index);
    double diff, load;
    int i, pin;
    delay_time_t wire_req, drive, min_fo_slack, orig_slack, slack, arr;

    pin_delay = get_pin_delay(node, fanin_index, speed_param->model);
    load = pin_delay->load;
    if (node_num_fanout(fanin) == 1){
	return (load * POS_LARGE); /* Unlimited increase OK !!! */
    }

    /* Keep track of the worst drive of this fanin */
    drive.rise = drive.fall = NEG_LARGE;
    for(i = node_num_fanin(fanin); i-- > 0; ) {
	pin_delay = get_pin_delay(fanin, i, speed_param->model);
	drive.rise = MAX(drive.rise, pin_delay->drive.rise);
	drive.fall = MAX(drive.fall, pin_delay->drive.fall);
    }

    assert(delay_wire_required_time(node, fanin_index, speed_param->model, &wire_req));
    arr = delay_arrival_time(fanin);
    orig_slack.rise = wire_req.rise - arr.rise;
    orig_slack.fall = wire_req.fall - arr.fall;
    
    /*
     * Compute the limit on the load of the new gate !!!!
     * Proportional to the difference in the slacks of the fanouts
     */
    min_fo_slack.rise = min_fo_slack.fall = POS_LARGE;
    foreach_fanout_pin(fanin, gen, fo, pin){
	if (fo == node) continue;
	delay_wire_required_time(fo, pin, speed_param->model, &wire_req);
	slack.rise = wire_req.rise - arr.rise;
	slack.fall = wire_req.fall - arr.fall;
	min_fo_slack.rise = MIN(slack.rise, min_fo_slack.rise);
	min_fo_slack.fall = MIN(slack.fall, min_fo_slack.fall);
    }
    /* Just a pessimistic view (to account for rise and fall asymmetry) */
    diff = MIN(min_fo_slack.rise, min_fo_slack.fall) -
               MIN(orig_slack.rise, orig_slack.fall);

    if (diff > 0){
	load += diff / MAX(drive.rise, drive.fall);
    } else {
	load += NSP_EPSILON; /* Very small increase !!!  */
    }

    return load;
}

/* 
 * This is an "intelligent" routine. First figures out if there is 
 * scope for dualizing the function at a node (invering inp/outputs)
 * and if so returns the dual network
 */
network_t *
nsp_get_dual_network(node, speed_param)
node_t *node;
speed_global_t *speed_param;
{
    (void)fprintf(sisout, "Cannot yet get the dual network\n");
    if (speed_param->model != DELAY_MODEL_MAPPED ||
	node->type == PRIMARY_INPUT || node_num_fanin(node) <= 1){
	return NIL(network_t);
    }
    
    return NIL(network_t);
}

/* Generate a network corresponding to the fanout problem at this node.
   We have to be careful not to overlap the regions that will be part
   of two separate selection procedures (fanout, 2c_kernel for example).
   Due to this we have to restrict the fanout optimization to be only with
   respect to the immediate fanouts of the network
   */

network_t *
buf_get_fanout_network(node, speed_param)
node_t *node;
speed_global_t *speed_param;
{
    char *name;
    int i, pin;
    lsGen gen;
    network_t *new_net;
    lib_gate_t *gate;
    double load, def_dr, ip_load;
    delay_time_t arrival, fi_req, drive;
    delay_pin_t *node_pin_delay, *pin_delay;
    node_t *dup_node, *po, *pi, *fo, *fi;
    
    /* Get a new network and add all the delay default stuff */
    new_net = network_alloc();
    delay_network_dup(new_net, node_network(node));

    dup_node = node_dup(node);
    foreach_fanin(node, i, fi){
	node_pin_delay = get_pin_delay(node, i, speed_param->model);
	pi = node_alloc();
	network_add_primary_input(new_net, pi);
	pin_delay = get_pin_delay(fi, 0, speed_param->model);
	drive = pin_delay->drive;
	arrival = delay_arrival_time(fi);
	arrival.rise -= drive.rise * (node_pin_delay->load);
	arrival.fall -= drive.fall * (node_pin_delay->load);
	delay_set_parameter(pi, DELAY_DRIVE_RISE, drive.rise);
	delay_set_parameter(pi, DELAY_DRIVE_FALL, drive.fall);
	/* Also set the max_ip_load so that the load increase is acceptable */
	ip_load = nsp_get_load_limit(node, i, speed_param);
	delay_set_parameter(pi, DELAY_MAX_INPUT_LOAD, ip_load);
/*
	delay_set_parameter(pi, DELAY_ARRIVAL_RISE, arrival.rise);
	delay_set_parameter(pi, DELAY_ARRIVAL_FALL, arrival.fall);
*/
	delay_set_parameter(pi, DELAY_ARRIVAL_RISE, 1000.0);
	delay_set_parameter(pi, DELAY_ARRIVAL_FALL, 1000.0);
	/* A node may have the same node as separate fanins !!! */
	name = ALLOC(char, strlen(fi->name)+10);
	sprintf(name, "%s%c%d", fi->name, NSP_INPUT_SEPARATOR, i);
	network_change_node_name(new_net, pi, name);
	dup_node->fanin[i] = pi;
    }

    if (dup_node->type == INTERNAL){
	network_add_node(new_net, dup_node);
	gate = lib_gate_of(node);
	buf_add_implementation(dup_node, gate);
    } else {
	network_add_primary_input(new_net, dup_node);
 	if (!delay_get_pi_drive(dup_node, &drive)){
	    def_dr = delay_get_default_parameter(node_network(node), DELAY_DEFAULT_DRIVE_RISE);
	    if (def_dr == DELAY_NOT_SET) def_dr = 0.0;
	    delay_set_parameter(dup_node, DELAY_DRIVE_RISE, def_dr);

	    def_dr = delay_get_default_parameter(node_network(node), DELAY_DEFAULT_DRIVE_FALL);
	    if (def_dr == DELAY_NOT_SET) def_dr = 0.0;
	    delay_set_parameter(dup_node, DELAY_DRIVE_FALL, def_dr);
	}
    }

    /* Now get the fanout set !!!! and add the PO's to the network */
#if FALSE
    inv_node = NIL(node_t);
    number_inv = 0;
    foreach_fanout(node, gen, fo){
	if (node_function(fo) == NODE_INV){
	    inv_node = fo;
	    number_inv++;
	}
    }

    if (number_inv > 1){
	inv_node = NIL(node_t);
    }

    if (inv_node != NIL(node_t)){
	dup_inv_node = node_literal(dup_node, 0);
	network_add_node(new_net, dup_inv_node);
	buf_add_implementation(dup_inv_node, lib_gate_of(inv_node));
	network_change_node_name(new_net, dup_inv_node, util_strsav(inv_node->name));
    }
#endif

    /* Now add all the PO's and annotate them with required times and loads */
    foreach_fanout_pin(node, gen, fo, pin){
#if FALSE
	if (node_function(fo) == NODE_INV && number_inv == 1){
	    foreach_fanout_pin(fo, gen1, fo1, pin1){
		po = network_add_fake_primary_output(new_net, dup_inv_node);
		pin_delay = get_pin_delay(fo1, pin1, speed_param->model);
		load = pin_delay->load;
		delay_wire_required_time(fo1, pin1, speed_param->model, &fi_req);
		name = ALLOC(char, strlen(fo1->name)+10);
		sprintf(name, "%s%c%d", fo1->name, NSP_OUTPUT_SEPARATOR, pin1);
		network_change_node_name(new_net, po, name);
		delay_set_parameter(po, DELAY_REQUIRED_RISE, fi_req.rise);
		delay_set_parameter(po, DELAY_REQUIRED_FALL, fi_req.fall);
		delay_set_parameter(po, DELAY_OUTPUT_LOAD, load);
	    }
	} else {
#endif
#ifdef SIS
	    po = network_add_fake_primary_output(new_net, dup_node);
#else
	    po = network_add_primary_output(new_net, dup_node);
	    network_swap_names(new_net, dup_node, po);
#endif /* SIS */
	    pin_delay = get_pin_delay(fo, pin, speed_param->model);
	    load = (pin_delay->load);
	    delay_wire_required_time(fo, pin, speed_param->model, &fi_req);
	    name = ALLOC(char, strlen(fo->name)+10);
	    sprintf(name, "%s%c%d", fo->name, NSP_OUTPUT_SEPARATOR, pin);
	    network_change_node_name(new_net, po, name);
	    delay_set_parameter(po, DELAY_REQUIRED_RISE, fi_req.rise);
	    delay_set_parameter(po, DELAY_REQUIRED_FALL, fi_req.fall);
	    delay_set_parameter(po, DELAY_OUTPUT_LOAD, load);
#if FALSE
	}
#endif
    }

    return new_net;
}
 /* Compare routine to compare two fanouts based on required time
  * Smallest required time (most critical) is first in the array
  */
int
nsp_fanout_compare(p1, p2)
char *p1, *p2;
{
    sp_fanout_t *f1 = (sp_fanout_t *)p1;
    sp_fanout_t *f2 = (sp_fanout_t *)p2;

    if (REQ_EQUAL(f1->req, f2->req)){
	return 0;
    } else if (BUF_MIN_D(f1->req) < BUF_MIN_D(f2->req)){
	return -1;
    } else {
	return 1;
    }
}
/* Return the min req time at the PI (adjusted for ip drive) */
delay_time_t
nsp_min_req_time(network, model)
network_t *network;
delay_model_t model;
{
    lsGen gen;
    node_t *pi;
    delay_time_t req, best, drive;

    best.rise = best.fall = POS_LARGE;
    foreach_primary_input(network, gen, pi){
	delay_wire_required_time(pi, 0, model, &req);
	assert(delay_get_pi_drive(pi, &drive));
	sp_drive_adjustment(drive, delay_load(pi), &req);
	MIN_DELAY(best, best, req);
    }
    return best;
}
/* 
 * Routine to modify a network by moving the faouts of the node into
 * two sets so that the required time at the input can be reduced
 */
static void
nsp_split_fanouts(network, speed_param)
network_t *network;
speed_global_t *speed_param;
{
    lsGen gen;
    int i, best_index;
    delay_time_t initial, best, req;
    node_t *root, *node, *dup_root, *fanin;
    sp_fanout_t fanout_data, *fanout_ptr;
    array_t *po_array;

    /* No point in running this when  delay model does not care for fanout */
    if (speed_param->model == DELAY_MODEL_UNIT) return;
    
    root = NIL(node_t);
    foreach_node(network, gen, node){
	if (node->type == INTERNAL){
	    if (root == NIL(node_t) || node_num_fanin(node) > 1){
		root = node;
	    }
	}
    }
    if (root == NIL(node_t)) return; /* There is no node to duplicate */
    if (node_num_fanout(root) <= 2) return; /* Not enuff fanouts  */

    delay_trace(network, speed_param->model);
    initial = best = nsp_min_req_time(network, speed_param->model);

    /* Create a list of the fanouts of "root" in order of criticality */
    po_array = array_alloc(sp_fanout_t, 0);
    foreach_primary_output(network, gen, node){
	if (node_get_fanin(node, 0) == root){
	    fanout_data.fanout = node;
	    fanout_data.load = delay_get_parameter(node, DELAY_OUTPUT_LOAD);
	    fanout_data.req = delay_required_time(node);
	    array_insert_last(sp_fanout_t, po_array, fanout_data);
	}
    }
    /* Sort the array */
    array_sort(po_array, nsp_fanout_compare);

    /* Create a duplicate of the root node so that fanouts can be moved */
    dup_root = node_dup(root);
    FREE(dup_root->name); dup_root->name = NIL(char);
    network_add_node(network, dup_root);
    if (speed_param->model == DELAY_MODEL_MAPPED){
	assert(lib_gate_of(root) != NIL(lib_gate_t));
	buf_add_implementation(dup_root, lib_gate_of(root));
    }
    
    /* Evaluate different configs. */
    best_index = -1;
    for (i = 0; i < array_n(po_array); i++){
	fanout_ptr = array_fetch_p(sp_fanout_t, po_array, i);
	node_patch_fanin(fanout_ptr->fanout, root, dup_root);
	delay_trace(network, speed_param->model);
	req = nsp_min_req_time(network, speed_param->model);
	if (REQ_IMPR(best, req)){
	    best_index = i;
	    best = req;
	}
    }
    if (best_index == (array_n(po_array) - 1)) best_index = -1;

    /* Generate the best configuration */
    for (i = 0; i < array_n(po_array); i++){
	fanout_ptr = array_fetch_p(sp_fanout_t, po_array, i);
	fanin = node_get_fanin(fanout_ptr->fanout, 0);
	if (i <= best_index){
	    node_patch_fanin(fanout_ptr->fanout, fanin, dup_root);
	} else {
	    node_patch_fanin(fanout_ptr->fanout, fanin, root);
	}
    }
    if (best_index == -1){
	network_delete_node(network, dup_root);
    }
    array_free(po_array);
}

/*
 * routines that do the different optimizations that are required by
 * the technology-independent models
 */

/*
 * No optimization --- simply return the networek as it is ....
 */
/* ARGSUSED */
network_t *
sp_noalg_opt(network, ti_model, speed_param)
network_t *network;
sp_xform_t *ti_model;
speed_global_t *speed_param;
{
    network_t *opt_network;

    if (network == NIL(network_t)){
	return NIL(network_t);
    }

    opt_network = speed_network_dup(network);
    sp_map_if_required(&opt_network, speed_param);
    (void)delay_trace(opt_network, speed_param->model);

    return opt_network;
}

/*
 * Do the decomposition based on the timing divisors 
 */
extern int kernel_timeout_occured;
static void div_timeout_handler()
{
    kernel_timeout_occured = TRUE;
}
/* ARGSUSED */
network_t *
sp_divisor_opt(network, ti_model, speed_param)
network_t *network;
sp_xform_t *ti_model;
speed_global_t *speed_param;
{
    node_t *clp_node, *temp;
    network_t *opt_network;

    if (network == NIL(network_t)){
	return NIL(network_t);
    }


    (void)signal(SIGALRM, div_timeout_handler);

    opt_network = speed_network_dup(network);
    network_collapse(opt_network);
    clp_node = node_get_fanin(network_get_po(opt_network, 0), 0);
    temp = node_simplify(clp_node, NIL(node_t), NODE_SIM_NOCOMP);
    node_replace(clp_node, temp);
	
    if (node_num_cube(clp_node) < LIMIT_NUM_CUBES) {
	alarm((unsigned int)(speed_param->timeout));
	kernel_timeout_occured = FALSE;
	speed_set_library_accl(speed_param, 1);	/* Not really required */
	speed_decomp_network(opt_network, clp_node, speed_param, 0 /*1 attempt*/);
	kernel_timeout_occured = FALSE;
	alarm((unsigned int) 0);
    } else {
	/* Instead of returning the collapsed netw return copy of original */
	network_free(opt_network);
	opt_network = speed_network_dup(network);
    }


    sp_map_if_required(&opt_network, speed_param);
    (void)delay_trace(opt_network, speed_param->model);
    speed_set_library_accl(speed_param, 0);	/* Not really required */

    return opt_network;
}

/*
 * Do a decomposition of the collapsed fn based on 2-cube kernels !!!
 */
extern int twocube_timeout_occured;
static void twocube_timeout_handler()
{
    twocube_timeout_occured = TRUE;
}
/* ARGSUSED */
network_t *
sp_2c_kernel_opt(network, ti_model, speed_param)
network_t *network;
sp_xform_t *ti_model;
speed_global_t *speed_param;
{
    node_t *clp_node, *temp;
    network_t *opt_network;

    if (network == NIL(network_t)){
	return NIL(network_t);
    }
    opt_network = speed_network_dup(network);
    
    /* use timers to prevent excessive use of computing resource */
    (void)signal(SIGALRM, twocube_timeout_handler);

    network_collapse(opt_network);
    clp_node = node_get_fanin(network_get_po(opt_network, 0), 0);
    temp = node_simplify(clp_node, NIL(node_t), NODE_SIM_NOCOMP);
    node_replace(clp_node, temp);

    if (node_num_cube(clp_node) < LIMIT_NUM_CUBES) {
	speed_set_library_accl(speed_param, 1);	/* Not really required */
	
	/* Initialize the timeout mechanism */
	alarm((unsigned int)(speed_param->timeout));
	twocube_timeout_occured = FALSE;
	speed_2c_decomp(opt_network, clp_node, speed_param, 0 /*1 attempt*/);
	twocube_timeout_occured = FALSE;
	alarm((unsigned int) 0);
    } else {
	/* Instead of returning the collapsed netw return copy of original */
	network_free(opt_network);
	opt_network = speed_network_dup(network);
    }
    

    sp_map_if_required(&opt_network, speed_param);
    (void)delay_trace(opt_network, speed_param->model);
    speed_set_library_accl(speed_param, 0);	/* Not really required */
    twocube_timeout_occured = FALSE;

    return opt_network;
}

/*
 * First invert the function of the collapsed node. Maybe that is a smaller
 * representation. Now, do a decomposition based on the timing divisors on
 * the inverted function. In the end need to make sure that the fuction
 * is evaluated correctly....
 */
static void comp_div_timeout_handler()
{
	kernel_timeout_occured = TRUE;
}
/* ARGSUSED */
network_t *
sp_comp_div_opt(network, ti_model, speed_param)
network_t *network;
sp_xform_t *ti_model;
speed_global_t *speed_param;
{
    node_t *clp_node, *temp;
    network_t *opt_network;

    if (network == NIL(network_t)) {
	return NIL(network_t);
    }
    (void)signal(SIGALRM, comp_div_timeout_handler);

    opt_network = speed_network_dup(network);
    network_collapse(opt_network);
    clp_node = node_get_fanin(network_get_po(opt_network, 0), 0);
    (void)node_invert(clp_node);
    temp = node_simplify(clp_node, NIL(node_t), NODE_SIM_NOCOMP);
    node_replace(clp_node, temp);
    
    if (node_num_cube(clp_node) < LIMIT_NUM_CUBES) {
	speed_set_library_accl(speed_param, 1);	/* Not really required */
	/* The clock is ticking !!! */
	alarm((unsigned int)(speed_param->timeout));
	kernel_timeout_occured = FALSE;
	speed_decomp_network(opt_network, clp_node, speed_param,
			     0 /* 1 attempt*/);
	/* If the inverter at the PO node can be deleted, delete it */
	kernel_timeout_occured = FALSE;
	network_csweep(opt_network);
	alarm((unsigned int) 0);
    } else {
	/* Instead of returning the collapsed netw return copy of original */
	network_free(opt_network);
	opt_network = speed_network_dup(network);
    }


    sp_map_if_required(&opt_network, speed_param);
    /*
     * We need to perform this since the delays for the inverter at the PO node
     * were not computed. Also, in case that was deleted, this is required
     */
    (void)delay_trace(opt_network, speed_param->model);
    speed_set_library_accl(speed_param, 0);	/* Not really required */

    return opt_network;
}

/*
 * First invert the function of the collapsed node. Maybe that is a smaller
 * representation. Now, do a decomposition based on the timing divisors on
 * the inverted function. In the end need to make sure that the fuction
 * is evaluated correctly....
 */
static void comp_2c_timeout_handler()
{
    twocube_timeout_occured = TRUE;
}
/* ARGSUSED */
network_t *
sp_comp_2c_opt(network, ti_model, speed_param)
network_t *network;
sp_xform_t *ti_model;
speed_global_t *speed_param;
{
    node_t *clp_node, *temp;
    network_t *opt_network;

    if (network == NIL(network_t)) {
	return NIL(network_t);
    }
    opt_network = speed_network_dup(network);

    (void)signal(SIGALRM, comp_2c_timeout_handler);

    /* Invert the function at the node */
    network_collapse(opt_network);
    clp_node = node_get_fanin(network_get_po(opt_network, 0), 0);
    (void)node_invert(clp_node);
    temp = node_simplify(clp_node, NIL(node_t), NODE_SIM_NOCOMP);
    node_replace(clp_node, temp);
	
    /* Reset the flag, start the clock and decompose the network */
    if (node_num_cube(clp_node) < LIMIT_NUM_CUBES) {
	alarm((unsigned int)(speed_param->timeout));
	twocube_timeout_occured = FALSE;
	speed_set_library_accl(speed_param, 1);	/* Not really required */
	speed_2c_decomp(opt_network, clp_node, speed_param, 0 /*1 attempt*/);
	alarm((unsigned int) 0);
	twocube_timeout_occured = FALSE;
	/* If the inverter at the PO node can be deleted, delete it */
	network_csweep(opt_network);
    } else {
	/* Instead of returning the collapsed netw return copy of original */
	network_free(opt_network);
	opt_network = speed_network_dup(network);
    }


    sp_map_if_required(&opt_network, speed_param);

    /*
     * We need to perform this since the delays for the inverter at the PO node
     * were not computed. Also, in case that was deleted, this is required
     */
    (void)delay_trace(opt_network, speed_param->model);
    speed_set_library_accl(speed_param, 0);	/* Not really required */

    return opt_network;
}

/*
 * Do an and-or decomposition based only on the arrival times
 */
/* ARGSUSED */
network_t *
sp_and_or_opt(network, ti_model, speed_param)
network_t *network;
sp_xform_t *ti_model;
speed_global_t *speed_param;
{
    node_t *clp_node;
    network_t *opt_network;

    if (network == NIL(network_t)){
	return NIL(network_t);
    }
    
    opt_network = speed_network_dup(network);
    network_collapse(opt_network);
    clp_node = node_get_fanin(network_get_po(opt_network, 0), 0);

    if (node_num_cube(clp_node) < LIMIT_NUM_CUBES) {
	speed_set_library_accl(speed_param, 1);	/* Not really required */
	(void)speed_and_or_decomp(opt_network, clp_node, speed_param);
    } else {
	/* Instead of returning the collapsed netw return copy of original */
	network_free(opt_network);
	opt_network = speed_network_dup(network);
    }


    sp_map_if_required(&opt_network, speed_param);
    (void)delay_trace(opt_network, speed_param->model);
    speed_set_library_accl(speed_param, 0);	/* Not really required */

    return opt_network;
}
/* 
 * Performs a remapping of the circuit for min delay if the delays are
 * being measured using the DELAY_MODEL_MAPPED or DELAY_MODEL_LIBRARY
 */

static void
sp_map_if_required(network, speed_param)
network_t **network;
speed_global_t *speed_param;
{
    network_t *map_net;

    if (*network == NIL(network_t) ||
	speed_param->model != DELAY_MODEL_MAPPED)  return;

    map_net = map_network(*network, lib_get_library(), 0, 1, 1);
    network_free(*network);
    *network = map_net;
}

/*
 * Do an optimization based on creating the appropriate fanout tree ...
 */
 /* Cache the creation of buffers for efficiency : Remember to free it */

static buf_global_t *nsp_buffer_param = NIL(buf_global_t);

buf_global_t *
nsp_init_buf_param(network, model)
network_t *network;
delay_model_t model;
{
    static buf_global_t buf_param;
    int use_mapped;

    if (nsp_buffer_param == NIL(buf_global_t)) {
	(void)buffer_fill_options(&buf_param, 0, NIL(char *));
	buf_param.limit = 2;
	buf_param.trace = 0;
	buf_param.thresh = 0.5;
	buf_param.single_pass = 1;
	buf_param.do_decomp = 0;
	use_mapped = ((model == DELAY_MODEL_MAPPED || 
		       model == DELAY_MODEL_LIBRARY) ? 1 : 0);
	
	buf_set_buffers(network, use_mapped, &buf_param);
	
	nsp_buffer_param = &buf_param;
    }
    return nsp_buffer_param;
}

void
nsp_free_buf_param()
{
    if (nsp_buffer_param != NIL(buf_global_t)){
	buf_free_buffers(nsp_buffer_param);
	nsp_buffer_param = NIL(buf_global_t);
    }
}

/* wrapper routine for the speed_buffer_recover_area() routine that
 * downsizes the non-critical gates to reduce area
 */
int
nsp_downsize_non_crit_gates(network, model)
network_t *network;
delay_model_t model;
{
    int success;

    (void)nsp_init_buf_param(network, model);
    success = speed_buffer_recover_area(network, nsp_buffer_param);
    nsp_free_buf_param();

    return success;
}


/* ARGSUSED */
network_t *
sp_fanout_opt(network, ti_model, speed_param)
network_t *network;
sp_xform_t *ti_model;
speed_global_t *speed_param;
{
    network_t *opt_network;
    buf_global_t *buf_param;

    if (network == NIL(network_t)){
	return NIL(network_t);
    }

    opt_network = speed_network_dup(network);
    if (lib_network_is_mapped(opt_network)) {
	buf_param = nsp_init_buf_param(network, speed_param->model);
	if (strcmp(ti_model->name, "repower") == 0){
	    buf_param->mode = 1; /* Only powering up is allowed */
	} else {
	    buf_param->mode = 7; /* All transformations enabled for "fanout" */
	}
	speed_buffer_network(opt_network, buf_param);
    }
    delay_trace(opt_network, speed_param->model);

    return opt_network;
}

/*
 * Another form of fanout optimization: Here the root node is duplicated
 * so that the critical signals see lesser capacitive loads...
 */
/* ARGSUSED */
network_t *
sp_duplicate_opt(network, ti_model, speed_param)
network_t *network;
sp_xform_t *ti_model;
speed_global_t *speed_param;
{
    network_t *opt_network;

    if (network == NIL(network_t)){
	return NIL(network_t);
    }

    opt_network = speed_network_dup(network);
    nsp_split_fanouts(opt_network, speed_param);
    delay_trace(opt_network, speed_param->model);

    return opt_network;
}
/*
 * As an attempt towards phase assignment we have this dualizing transform
 */
/* ARGSUSED */
network_t *
sp_dual_opt(network, ti_model, speed_param)
network_t *network;
sp_xform_t *ti_model;
speed_global_t *speed_param;
{
    network_t *opt_network;

    if (network == NIL(network_t)){
	return NIL(network_t);
    }

    opt_network = speed_network_dup(network);
    (void)fprintf(sisout, "Not yet implemeted dualizing\n");
    delay_trace(opt_network, speed_param->model);

    return opt_network;
}

/*
 * Do an optimization based on the timing driven cofactoring (tdc) approach
 */
/* ARGSUSED */
network_t *
sp_cofactor_opt(network, ti_model, speed_param)
network_t *network;
sp_xform_t *ti_model;
speed_global_t *speed_param;
{
    network_t *new_net, *clp_net;

    if (network == NIL(network_t)){
	return NIL(network_t);
    }

    clp_net = speed_network_dup(network);
    network_collapse(clp_net);
    new_net = tdc_factor_network(clp_net);
/*
    speed_init_decomp(new_net, speed_param);
 */
 /* We will do a balanced decomp to isolate the cofactoring impr */
    resub_alge_network(new_net, 0);
    decomp_tech_network(new_net, 2, 0);
    network_csweep(new_net);

    network_free(clp_net);
    sp_map_if_required(&new_net, speed_param);
    delay_trace(new_net, speed_param->model);

    return new_net;
}

/* ARGSUSED */
network_t *
sp_bypass_opt(network, ti_model, speed_param)
network_t *network;
sp_xform_t *ti_model;
speed_global_t *speed_param;
{
    network_t *new_net;
    int cutset_flag=0; /* Try the bypasses in sorted order */
/*
    static ref_count = 0;
    char command[256];
*/
    extern int do_bypass_transform();

    if (network == NIL(network_t)){
	return NIL(network_t);
    }

/*
    sprintf(command, "write_blif /usr/tmp/bypass_ex%d.blif", ref_count++);
    com_execute(&network, command);
*/
    /* The bypass transformation code / routine call goes here */
    new_net = speed_network_dup(network);
    do_bypass_transform(&new_net, (double)1, 0, (double)2, speed_param->model,
            GBX_NEWER_TRACE, speed_param->debug, cutset_flag);

    resub_alge_network(new_net, 0);
    com_execute(&new_net, "full_simplify -l");
    sp_map_if_required(&new_net, speed_param);
    delay_trace(new_net, speed_param->model);

    return new_net;
}

/*
 * Use this routine if the delay information is stored in the node->delay field
 */
 /* ARGSUSED */
delay_time_t
new_delay_arrival(node, speed_param)
node_t *node;
speed_global_t *speed_param;
{
    delay_time_t arrival;

    arrival = delay_arrival_time(node);

    return arrival;
}
/*
 * Use this routine if the delay information is stored in the node->delay field
 */
 /* ARGSUSED */
delay_time_t
new_delay_required(node, speed_param)
node_t *node;
speed_global_t *speed_param;
{
    delay_time_t req;

    req = delay_required_time(node);

    return req;
}
/*
 * Use this routine if the delay information is stored in the node->delay field
 */
 /* ARGSUSED */
delay_time_t
new_delay_slack(node, speed_param)
node_t *node;
speed_global_t *speed_param;
{
    delay_time_t req;

    fail("Not yet implemented");

    return req;
}

/* Routines to speedup the network duplication code !!! */

static void
duplicate_list(list, newlist, newnetwork)
lsList list, newlist;
network_t *newnetwork;
{
    lsGen gen;
    node_t *node, *newnode;
    lsHandle handle;

    lsForeachItem(list, gen, node) {
	newnode = speed_node_dup(node);
	node->copy = newnode;
	LS_ASSERT(lsNewEnd(newlist, (lsGeneric) newnode, &handle));
	newnode->network = newnetwork;
	newnode->net_handle = handle;
    }
}

static void
copy_list(list, newlist)
lsList list, newlist;
{
    lsGen gen;
    node_t *node;

    lsForeachItem(list, gen, node) {
	LS_ASSERT(lsNewEnd(newlist, (lsGeneric) node->copy, LS_NH));
    }
}

static void
reset_io(list)
lsList list;
{
    lsGen gen;
    node_t *node, *fanin;
    int i;

    lsForeachItem(list, gen, node) {
	foreach_fanin(node, i, fanin) {
	    node->fanin[i] = fanin->copy;
	}
	fanin_add_fanout(node);
    }
}

network_t *
speed_network_dup(old)
network_t *old;
{
    network_t *new;

    if (old == NIL(network_t)) {
        return(NIL(network_t));
    }
    new = network_alloc();

    if (old->net_name != NIL(char)) {
	new->net_name = util_strsav(old->net_name);
    }

    duplicate_list(old->nodes, new->nodes, new);
    copy_list(old->pi, new->pi);
    copy_list(old->po, new->po);

    network_rehash_names(new, /* long */ 1, /* short */ 1);

    /* this makes sure fanout pointers are up-to-date */
    reset_io(new->nodes);

    /* This makes sure that the global delay information is copied */
    delay_network_dup(new, old);

    /* this makes sure that MAP(node)->save_binding pointers are up-to-date */
    map_network_dup(new);

    new->original = network_dup(old->original);
    new->dc_network = network_dup(old->dc_network);
    new->area_given = old->area_given;
    new->area = old->area;
#ifdef SIS
    copy_latch_info(old->latch, new->latch, new->latch_table);
    new->stg = stg_dup(old->stg);
    new->astg = astg_dup(old->astg);
    network_clock_dup(old, new);
#endif /* SIS */
    return new;
}

static node_t *
speed_node_dup(old)
register node_t *old;
{
    register node_t *new;

    if (old == 0) return 0;

    new = node_alloc();

    if (old->name != 0) {
	new->name = util_strsav(old->name);
    }
    if (old->short_name != 0) {
	new->short_name = util_strsav(old->short_name);
    }

    new->type = old->type;

    new->fanin_changed = old->fanin_changed;
    new->fanout_changed = old->fanout_changed;
    new->is_dup_free = old->is_dup_free;
    new->is_min_base = old->is_min_base;
    new->is_scc_minimal = old->is_scc_minimal;

    new->nin = old->nin;
    if (old->nin != 0) {
	new->fanin = nodevec_dup(old->fanin, old->nin);
    }

    /* do NOT copy old->fanout ... */
    /* do NOT copy old->fanin_fanout ... */
    
    if (old->F != 0) new->F = sf_save(old->F);
    if (old->D != 0) new->D = sf_save(old->D);
    if (old->R != 0) new->R = sf_save(old->R);

    new->copy = 0;				/* ### for saber */

    /* do NOT copy old->network ... */
    /* do NOT copy old->net_handle ... */

    delay_dup(old, new);
    map_dup(old, new);
/*
  for(d = daemon_func[(int) DAEMON_DUP]; d != 0; d = d->next) {
      (*d->func)(old, new);
  }
*/
    return new;
}
