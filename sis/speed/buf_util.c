/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/speed/buf_util.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:50 $
 *
 */
#include "sis.h"
#include <math.h>
#include "speed_int.h"
#include "buffer_int.h"

static int sp_compare_buf();
static int speed_lib_buffers();
static void sp_insert_buf_in_list();

static double sp_inv_load;
static delay_time_t sp_unit_fanout_block ={ 1.0, 1.0};


/*
 * node daemon to allocate the storage with every node 
 */
void
buffer_alloc(node)
node_t *node;
{
    node->BUFFER_SLOT = (char *) ALLOC(sp_node_t, 1);
    BUFFER(node)->type  = NONE;
    BUFFER(node)->cfi = -1;
}

/*
 * node daemon to free the memory associated with the BUFFER_SLOT field 
 */
void
buffer_free(node)
node_t *node;
{
    FREE(node->BUFFER_SLOT);
}
static delay_time_t sp_unit_fanout_drive ={ 0.2, 0.2};


/*
 * get all the buffers and inverters from the library . If none is 
 * found then returns a dummy buffer with the delay characteristics of
 * the unit fanout model
 */
void
buf_set_buffers(network, use_mapped, buf_param)
network_t *network;	/* The network we are looking at for buffering */
int use_mapped;	/* When set use the library information */
buf_global_t *buf_param;
{
    double d;
    library_t *lib;

    buf_param->auto_route = compute_wire_load(network, 1);
    buf_param->glbuftable = st_init_table(st_ptrcmp, st_ptrhash);

    if (use_mapped || lib_network_is_mapped(network)){
	lib = lib_get_library();
	(void)speed_lib_buffers(lib, 2, buf_param, &d);
	buf_param->model = DELAY_MODEL_MAPPED;
    } else {
	(void)speed_lib_buffers(NIL(library_t), 2, buf_param, &d);
	buf_param->model = DELAY_MODEL_UNIT_FANOUT;

    }
    buf_param->min_req_diff = d;

    sp_buf_array_from_list(buf_param);
}
/*
 * Free the structures that store the buffers used during buffering
 */
void
buf_free_buffers(buf_param)
buf_global_t *buf_param;
{
    int i, n = buf_param->num_buf;

    /* Delete the buffer structures --- They are in the list and the array */
    for (i = 0; i < n; i++){
	FREE(buf_param->buf_array[i]->gate);
	FREE(buf_param->buf_array[i]);
    }
    FREE(buf_param->buf_array);
    st_free_table(buf_param->glbuftable);
    (void)lsDestroy(buf_param->buffer_list,(void(*)())0);
}

static int
speed_lib_buffers(lib, limit, buf_param, p_min_req_diff)
library_t *lib;
int limit;
buf_global_t *buf_param;
double *p_min_req_diff;
{
    lsGen gen;
    lsGeneric dummy;
    lib_gate_t *gate;
    lib_class_t *class;
    sp_buffer_t *buffer;
    double d, min_req_diff;

    *p_min_req_diff = 0.0;
    buf_param->buffer_list = lsCreate();
    if (lib == NIL(library_t)){
	sp_insert_buf_in_list(buf_param->buffer_list, buf_param, NIL(sp_buffer_t), NIL(lib_gate_t));
	return 1;
    }
    if ((gate = sp_lib_get_inv(lib)) == NIL(lib_gate_t)){
	sp_insert_buf_in_list(buf_param->buffer_list, buf_param, NIL(sp_buffer_t), NIL(lib_gate_t));
	return 1;
    }
    class = lib_gate_class(gate);
    sp_inv_load = limit * ((delay_pin_t *)(gate->delay_info[0]))->load;
    gen = lib_gen_gates(class);
    while (lsNext(gen, &dummy, LS_NH) == LS_OK) {
	gate = (lib_gate_t *)dummy;
	sp_insert_buf_in_list(buf_param->buffer_list, buf_param, NIL(sp_buffer_t), gate);
    }
    (void)lsFinish(gen);

    /* Now we want to get the buffers from the library */
    if ((gate = sp_lib_get_buffer(lib)) != NIL(lib_gate_t)){
	class = lib_gate_class(gate);
	gen = lib_gen_gates(class);
	while (lsNext(gen, &dummy, LS_NH) == LS_OK) {
	    gate = (lib_gate_t *)dummy;
	    sp_insert_buf_in_list(buf_param->buffer_list, buf_param, NIL(sp_buffer_t), gate);
	}
	(void)lsFinish(gen);
    }
    /* Sort the inverters in list -- smallest last */
    (void)lsSort(buf_param->buffer_list, sp_compare_buf);

    gen = lsStart(buf_param->buffer_list);
    min_req_diff = POS_LARGE;
    while (lsNext(gen, &dummy, LS_NH) == LS_OK) {
	buffer = (sp_buffer_t *)dummy;
	if (buffer->phase == PHASE_NONINVERTING){
	    d = MIN(buffer->block.rise, buffer->block.fall);
	    min_req_diff = MIN(d, min_req_diff);
	}
    }
    lsFinish(gen);

    if(buf_param->debug > 10){
	speed_dump_buffer_list(sisout, buf_param->buffer_list);
    }

    if (min_req_diff != POS_LARGE){
	*p_min_req_diff = min_req_diff;
    }
    return 0;
}


void
speed_dump_buffer_list(fp, buffer_list)
FILE *fp;
lsList buffer_list;
{
    lsGen gen;
    char *dummy;
    sp_buffer_t *buffer;
    double d1, d2;

    (void)fprintf(fp,"type        name       area ipcap  blck_r blck_f drve_r drve_f rise   fall\n");
    (void)fprintf(fp,"--------------------------------------------------------------------------\n");
    gen = lsStart(buffer_list);
    while (lsNext(gen, &dummy, LS_NH) == LS_OK) {
	buffer = (sp_buffer_t *)dummy;
	d1 = buffer->block.rise + sp_inv_load * buffer->drive.rise;
	d2 = buffer->block.fall + sp_inv_load * buffer->drive.fall;
	(void)fprintf(fp,"%s    %-14s %5.1f %-6.3f %-6.3f %-6.3f %-6.3f %-6.3f %-6.3f %-6.3f\n",
	(buffer->phase == PHASE_INVERTING? "INV" : "BUF"),
	sp_buffer_name(buffer), buffer->area,buffer->ip_load,
	buffer->block.rise, buffer->block.fall,
	buffer->drive.rise, buffer->drive.fall,
	d1, d2);
    }
    (void)lsFinish(gen);
}

/* Sort the buffers according to size -- hope that the smallest is slowest */
static int
sp_compare_buf(item1, item2)
lsGeneric item1, item2;
{
    sp_buffer_t *buf1, *buf2;

    buf1 = (sp_buffer_t *)item1;
    buf2 = (sp_buffer_t *)item2;

    if (buf1->phase == buf2->phase){
	if (buf2->area < buf1->area) return -1;
	else if (buf2->area > buf1->area) return 1;
	else return 0;
    } else {
	if (buf1->phase == PHASE_INVERTING){
	    return -1;
	} else {
	    return 1;
	}
    }


}


/*
 * Generates the array of buffers that we need to use  -- 
 * The buf_list is sorted (inverters come earlier) and so is the buf_array
 */
void
sp_buf_array_from_list(buf_param)
buf_global_t *buf_param;
{
    int i = 0;
    lsGen gen;
    lsGeneric dummy;
    sp_buffer_t *buffer;
    lsList buf_list = buf_param->buffer_list;

    buf_param->num_inv = 0;
    buf_param->num_buf = 0;
    buf_param->buf_array = ALLOC(sp_buffer_t *, lsLength(buf_list));
    gen = lsStart(buf_list);
    while (lsNext(gen, &dummy, LS_NH) == LS_OK) {
	buffer = (sp_buffer_t *)dummy;
	if (buffer->phase == PHASE_NONINVERTING){
	    buf_param->num_buf++;
	} else {
	    buf_param->num_inv++;
	}
	buf_param->buf_array[i++] = buffer;
    }
    buf_param->num_buf += buf_param->num_inv;
    (void)lsFinish(gen);
}

/*
 * Sets "gate" as the impl of "node = fanin';"
 * gate must be an inverter in the library 
 */
void
sp_set_inverter(node, fanin, gate)
node_t *node, *fanin;
lib_gate_t *gate;
{
    char *inv_formals[1];
    node_t *inv_actuals[1];

    inv_actuals[0] = fanin;
    inv_formals[0] = lib_gate_pin_name(gate, 0, 1);
    (void)lib_set_gate(node, gate, inv_formals ,inv_actuals, 1);
}

/*
 * Will take the gate implementing "node" and get the extreme gate
 * of that class and fix that as the implementation of "newnode". In the
 * case that "node" has its impl set as a BUF we will set the impl to
 * be the smallest buffer.
 */
/* ARGSUSED */
void
sp_buf_annotate_gate(node, buf_param)
node_t *node;
buf_global_t *buf_param;
{
    int cfi;
    lib_gate_t *gate;
    pin_phase_t phase;
    delay_pin_t *pin_delay;
    delay_time_t t, desired, drive;
    sp_buffer_t **buf_array = buf_param->buf_array;

    if ((gate = lib_gate_of(node)) == NIL(lib_gate_t)) {
	if (BUFFER(node)->type == NONE){
	    BUFFER(node)->type = BUF;
	    BUFFER(node)->impl.buffer = buf_array[buf_param->num_buf -1];
	}
    } else {
	BUFFER(node)->type = GATE;
	BUFFER(node)->impl.gate = gate;
    }
    cfi = sp_buf_get_crit_fanin(node, buf_param->model);
    pin_delay = get_pin_delay(node, cfi, buf_param->model);
    phase = pin_delay->phase;
    t = delay_node_pin(node, cfi, buf_param->model);
    desired = delay_required_time(node);
    sp_compute(phase, &desired, t);
    BUFFER(node)->load = delay_load(node);
    BUFFER(node)->cfi = cfi;
    BUFFER(node)->req_time = desired;	/* req time at cfi input of node */

    drive = sp_get_input_drive(node, buf_param->model, &phase);
    BUFFER(node)->prev_dr = drive;
    BUFFER(node)->prev_ph = phase;
    return;
}

lib_gate_t *
buf_get_gate_version(gate, version)
lib_gate_t *gate;
int version;
{
    lsGen gen;
    char *dummy;
    int num_gates;
    lib_gate_t *root_gate;

    num_gates = 0; root_gate = NIL(lib_gate_t);
    gen = lib_gen_gates(lib_gate_class(gate));
    while (lsNext(gen, &dummy, LS_NH) == LS_OK){
	if (num_gates++ == version) {
	    root_gate = (lib_gate_t *)dummy;
	    break;
	}
    }
    (void)lsFinish(gen);

    return root_gate;
}
/*
 * Check to see if for the given value of req_time and load at the output
 * is the minimum slack worsened at any of the inputs. If the 
 */
int
buf_failed_slack_test(node, buf_param, gate_version, op_req, op_load)
node_t *node;
buf_global_t *buf_param;
int gate_version;
delay_time_t op_req;
double op_load;
{
    int i;
    node_t *fanin;
    lib_gate_t *root_gate;
    double fom, fom_best, new_fom_best;
    delay_model_t model = buf_param->model;
    pin_phase_t phase;
    delay_time_t block, drive;
    delay_time_t arrival, req, slack, new_req, new_slack;

    if (lib_gate_of(node) == NIL(lib_gate_t) || node_num_fanin(node) == 1){
	return 0;
    }

    root_gate = buf_get_gate_version(lib_gate_of(node), gate_version);
    assert(root_gate != NIL(lib_gate_t));

    fom_best = new_fom_best = POS_LARGE;
    foreach_fanin(node, i, fanin){
	(void)delay_wire_required_time(node, i, model, &req);
	arrival = delay_arrival_time(fanin);
	slack.rise = req.rise - arrival.rise;
	slack.fall = req.fall - arrival.fall;

	fom = BUF_MIN_D(slack);
	fom_best = MIN(fom, fom_best);

	phase = ((delay_pin_t *)(root_gate->delay_info[i]))->phase;
	drive = ((delay_pin_t *)(root_gate->delay_info[i]))->drive;
	block = ((delay_pin_t *)(root_gate->delay_info[i]))->block;
	new_req = op_req;
	sp_subtract_delay(phase, block, drive, op_load, &new_req);
	new_slack.rise = new_req.rise - arrival.rise;
	new_slack.fall = new_req.fall - arrival.fall;
	
	fom = BUF_MIN_D(new_slack);
	new_fom_best = MIN(fom, new_fom_best);
    }
    
    if (new_fom_best < fom_best){
	if (buf_param->debug > 10) {
	    (void)fprintf(sisout, "WARNING: slack increase at other input\n");
	    (void)fprintf(sisout, "\tNode %s, gate %s, R=%.3f:%.3f, L=%.3f\n",
			  node->name, root_gate->name, op_req.rise,
			  op_req.fall, op_load);
	}
	return 1;
    }
    return 0;
}
/*
 * Look at the fanins of node and find the index that is most
 * critical. This index gives us the fanin such that if we decrease
 * the required time at that fanin -- we would most likely decrease the
 * delay thru the entire netw
 */
int 
sp_buf_get_crit_fanin(node, model)
node_t *node;
delay_model_t model;
{
    int i, cfi;
    node_t *fanin;
    double fom, fom_best;
    delay_time_t arrival, req, slack;

    if (node_num_fanin(node) == 1) return 0;

    fom_best = POS_LARGE;
    foreach_fanin(node, i, fanin){
	(void)delay_wire_required_time(node, i, model, &req);
	arrival = delay_arrival_time(fanin);
	slack.rise = req.rise - arrival.rise;
	slack.fall = req.fall - arrival.fall;
	/*
	 * figure of merit -- slack value at the signal
	 */
	fom = BUF_MIN_D(slack);
	if (fom < fom_best){
	   fom_best = fom; cfi = i;
	}
    }
    return cfi;
}
/*
 * Set the threshold for determining the critical paths in the network
 */
void
set_buf_thresh(network, buf_param)
network_t *network;
buf_global_t *buf_param;
{
    delay_time_t time;
    double my_thresh, min_t;
    node_t *po;
    lsGen gen;

    my_thresh = POS_LARGE;
    foreach_primary_output(network, gen, po){
	time = delay_slack_time(po);
	min_t = D_MIN(time.rise, time.fall);
	my_thresh = D_MIN(my_thresh, min_t);
    }
    if (my_thresh > 0 ) {
	/* 
	 * All output nodes meet the required
	 * time costraints. 
	 */
	buf_param->crit_slack = -1.0;
    } else {
	/* 
	 * Set the threshold for the epsilon
	 * network to be within thresh of the most
	 * most critical node.
	 */
    	buf_param->crit_slack = my_thresh + buf_param->thresh;
    }
}

/*
 * Return 1 if the node is within "thresh" of the most critical
 */
int
buf_critical(np, buf_param)
node_t *np;
buf_global_t *buf_param;
{
    delay_time_t time;

    time = delay_slack_time(np);
    if (time.rise <= buf_param->crit_slack || time.fall <= buf_param->crit_slack) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/*
 * Replace gate by pref_gate as the implementation of "node" 
 */
void
sp_replace_lib_gate(node, gate, pref_gate)
node_t *node;
lib_gate_t *gate, *pref_gate;
{
    int i, nin;
    char **formals;
    node_t **actuals, *fanin;

    assert(lib_gate_class(gate) == lib_gate_class(pref_gate));
    nin = node_num_fanin(node);
    formals = ALLOC(char *, nin);
    actuals = ALLOC(node_t *, nin);
    foreach_fanin(node, i, fanin){
	formals[i] = lib_gate_pin_name(gate, i, 1);
	actuals[i] = fanin;
    }
    (void)lib_set_gate(node, pref_gate, formals, actuals, nin);
    FREE(formals);
    FREE(actuals);
}


/*
 * Determine the name of the implementation of the node ---
 * Useful only for debugging the buffering routines 
 * Successive calls to this routine use the same storage and
 * so to save the name need to strsav the returned string
 */
char *
sp_name_of_impl(node)
node_t *node;
{
    static char name[BUFSIZ];
    if (node == NIL(node_t)){
	(void)strcpy(name,"--NONE--");
    } else if (node->type == PRIMARY_INPUT){
	(void)strcpy(name,"NODE_PI");
    } else if (BUFFER(node)->type == BUF){
	(void)strcpy(name, sp_buffer_name(BUFFER(node)->impl.buffer));
    } else if (BUFFER(node)->type == GATE){
	(void)strcpy(name, lib_gate_name(BUFFER(node)->impl.gate));
    } else {
	(void)strcpy(name,"-NONE-");
    }
    return name;
}

/*
 * Returns a contrived name for the buffer implementing the node --
 * The name is  of the form "inv1-inv2"  -- uses a static storage too
 */
char *
sp_buffer_name(buffer)
sp_buffer_t *buffer;
{
    int i;
    static char name[BUFSIZ];

    if (buffer == NIL(sp_buffer_t)) (void)strcpy(name,"NONE");
    if (buffer->gate[0] == NIL(lib_gate_t)){ (void)strcpy(name,"UNIT_FAN");
    } else {
	(void)strcpy(name,lib_gate_name(buffer->gate[0]));
	for (i = 1; i < buffer->depth; i++){
	    (void)strcat(name, "-");
	    (void)strcat(name, lib_gate_name(buffer->gate[i]));
	}
    }
    return name;
}


/*
 * Creates a buffer that is buffer "prev" followed by the gate "gate"
 * and inserts the buffer just created into the lsList "buffer_list"
 * Note: "prev" can be a NULL and so can gate
 */
static void
sp_insert_buf_in_list(buffer_list, buf_param, prev, gate)
lsList buffer_list;
buf_global_t *buf_param;
sp_buffer_t *prev;
lib_gate_t *gate;
{
    int i;
    delay_time_t block;
    sp_buffer_t *inv_buffer, *buffer;
    double load, auto_route = buf_param->auto_route;

    buffer = ALLOC(sp_buffer_t,1);
    if (prev == NIL(sp_buffer_t)){
	buffer->depth = 1;
	buffer->gate = ALLOC(lib_gate_t *, 1);
	buffer->gate[0] = gate;
	if (gate != NIL(lib_gate_t)){
	    buffer->area = lib_gate_area(gate);
	    /* Single fanin gates only -- use 0 fanin */
	    buffer->phase = ((delay_pin_t *)(gate->delay_info[0]))->phase;
	    buffer->block = ((delay_pin_t *)(gate->delay_info[0]))->block;
	    buffer->drive = ((delay_pin_t *)(gate->delay_info[0]))->drive;
	    buffer->ip_load = ((delay_pin_t *)(gate->delay_info[0]))->load;
	    buffer->max_load = ((delay_pin_t *)(gate->delay_info[0]))->max_load;
	} else {
	    /* Add non-inverting buffer to the list */
	    buffer->area = 1.0;
	    buffer->phase =  PHASE_NONINVERTING;
	    buffer->block = sp_unit_fanout_block;
	    buffer->drive = sp_unit_fanout_drive;
	    buffer->ip_load = 1.0;
	    buffer->max_load = POS_LARGE;
	    (void)lsNewBegin(buffer_list, (lsGeneric)buffer, LS_NH);

	    /* Add inverting buffer too */
	    inv_buffer = ALLOC(sp_buffer_t,1);
	    *inv_buffer = *buffer;
	    inv_buffer->gate = ALLOC(lib_gate_t *, 1);
	    inv_buffer->gate[0] = gate;
	    inv_buffer->phase =  PHASE_INVERTING;
	    buffer = inv_buffer;
	}
    } else {
	buffer->depth = prev->depth + 1;
	buffer->gate = ALLOC(lib_gate_t *,buffer->depth);
	for (i = 0; i < prev->depth; i++){
	    buffer->gate[i] = prev->gate[i];
	}
	buffer->gate[prev->depth] = gate;
	/*
	 * We only chain inverters -- hence dont bother to compute the
	 * phase of the chain.  Assume it is NONINVERTING
	 */
	buffer->phase = PHASE_NONINVERTING;
	buffer->area = prev->area + lib_gate_area(gate);
	load = auto_route + ((delay_pin_t *)(gate->delay_info[0]))->load;
	block = ((delay_pin_t *)(gate->delay_info[0]))->block;
	buffer->block.rise = prev->block.fall + prev->drive.fall * load + block.rise;
	buffer->block.fall = prev->block.rise + prev->drive.rise * load + block.fall;
	buffer->drive = ((delay_pin_t *)(gate->delay_info[0]))->drive;
	buffer->ip_load = prev->ip_load;
	buffer->max_load = delay_get_load_limit(gate->delay_info[0]);
    }
    (void)lsNewBegin(buffer_list, (lsGeneric)buffer, LS_NH);
}

/* 
 * Free the storage associated with the buffers in "list" and
 * then destroy the list.   Garbage collection function
 */
void
speed_free_buffer_list(list)
lsList list;
{
    lsGen gen;
    lsGeneric dummy;

    gen = lsStart(list);
    while (lsNext(gen, &dummy, LS_NH) == LS_OK) {
	FREE(((sp_buffer_t *)dummy)->gate);
	FREE(dummy);
    }
    (void)lsDestroy(list,(void(*)())0);
    (void)lsFinish(gen);
}

/*
 * If the node is implemented as a buffer then replace the node by the
 * gates that make up the buffer... Even thought there are more efficient
 * implemetations this particular one is chosen since it retains the 
 * node as the root of the chain (closest to input )
 */
void
sp_implement_buffer_chain(network, node)
network_t *network;
node_t *node;
{
    lsGen gen;
    int n, i, j;
    node_t *temp, *fanin, **fanouts, *fo;
    sp_buffer_t *buffer;

    if (node == NIL(node_t) || node->type == PRIMARY_INPUT) return;
    if (BUFFER(node)->type == BUF){
	buffer = BUFFER(node)->impl.buffer;	
	if (buffer->gate[0] != NIL(lib_gate_t)){
	    fanin = node_get_fanin(node, 0);
	    if (buffer->depth == 1){
		sp_set_inverter(node, fanin, buffer->gate[0]);
		BUFFER(node)->type = GATE;
		BUFFER(node)->cfi = 0;
		BUFFER(node)->impl.gate = buffer->gate[0];
	    } else {
		n = node_num_fanout(node);
		fanouts = ALLOC(node_t *, n);
		i = 0;
		foreach_fanout(node, gen, fo){
		    fanouts[i++] = fo;
		}
		sp_set_inverter(node, fanin, buffer->gate[0]);
		BUFFER(node)->type = GATE;
		BUFFER(node)->cfi = 0;
		BUFFER(node)->impl.gate = buffer->gate[0];
		fanin = node;
		for (j = 1; j < buffer->depth; j++){
		    temp = node_alloc();
		    sp_set_inverter(temp, fanin, buffer->gate[j]);
		    network_add_node(network, temp);
		    fanin = temp;
		    BUFFER(temp)->type = GATE;
		    BUFFER(temp)->cfi = 0;
		    BUFFER(temp)->impl.gate = buffer->gate[j];
		    if (j == buffer->depth - 1){
			/* patch the fanouts of the original node to fanin */
			for (i = 0; i < n; i++){
			    assert(node_patch_fanin(fanouts[i], node, fanin));
			}
		    }
		}
		FREE(fanouts);
	    }
	}
    }
}

/*
 * Add the implementation of the node to be gate
 */
void
buf_add_implementation(node, gate)
node_t *node;
lib_gate_t *gate;
{
    int i, nin;
    char **formals;
    node_t **actuals, *fanin;

    nin = node_num_fanin(node);
    formals = ALLOC(char *, nin);
    actuals = ALLOC(node_t *, nin);
    foreach_fanin(node, i, fanin){
	formals[i] = lib_gate_pin_name(gate, i, 1);
	actuals[i] = fanin;
    }
    (void)lib_set_gate(node, gate, formals, actuals, nin);

    BUFFER(node)->type = GATE;
    BUFFER(node)->impl.gate = gate;
    BUFFER(node)->cfi = 0;

    FREE(formals);
    FREE(actuals);
}

/*
 * Some routines to allow the mapping to interface this package
 * We need a static variable here to store some information of the
 * behalf of the mapping package. 
 */

static buf_global_t map_buf_param;

void
buf_init_top_down(network, mode, debug)
network_t *network;
int mode, debug;
{
    (void)buffer_fill_options(&map_buf_param, 0, NIL(char *));
    map_buf_param.mode = mode;
    map_buf_param.debug = debug;
    /* The setting of buffers is a constant core leak per mapping procedure */
    buf_set_buffers(network, 1, &map_buf_param);
}
void
buf_map_interface(network, data)
network_t *network;
buf_alg_input_t *data; /* Data for the buffering */
{
    int level = 0;
    delay_time_t impr; /* amount of improvement desired */

    impr.rise = impr.fall = POS_LARGE;
    (void)sp_buffer_recur(network, &map_buf_param, data, impr, &level);
}

delay_model_t
buf_get_model()
{
    return (map_buf_param.model);
}

double
buf_get_auto_route()
{
    return (map_buf_param.auto_route);
}

delay_time_t
buf_get_required_time_at_input(node)
node_t *node;
{
    return (BUFFER(node)->req_time);
}

void
buf_set_required_time_at_input(node, req)
node_t *node;
delay_time_t req;
{
    BUFFER(node)->req_time = req;
}

void
buf_set_prev_drive(node, drive)
node_t *node;
delay_time_t drive;
{
    BUFFER(node)->prev_dr = drive;
}

void
buf_set_prev_phase(node, phase)
node_t *node;
pin_phase_t phase;
{
    BUFFER(node)->prev_ph = phase;
}





