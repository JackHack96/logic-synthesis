/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/speed/buf_delay.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:50 $
 *
 */
#include "sis.h"
#include "speed_int.h"
#include "buffer_int.h"

/*
 * Computes the required time at the "pin" input signal of node "fo".
 * This differs from the required time at the node generating the signal
 */
void
sp_buf_req_time(fo, buf_param, pin, reqp, loadp)
node_t *fo;
buf_global_t *buf_param;
int pin;
delay_time_t *reqp;	/* RETURN: The required time at the signal */
double *loadp;		/* RETURN: The input cap + wire load cap at input */
{
    delay_model_t model = buf_param->model;
    delay_pin_t *pin_delay;

    /*
     * The req time has to be at the signal rather than the gate 
     */
    (void)delay_wire_required_time(fo, pin, model, reqp);
    pin_delay = get_pin_delay(fo, pin, model);
    
    *loadp = pin_delay->load + buf_param->auto_route;
}


/*
 * Gets the input capacitance of node. In the case of a gate it will
 * get the input cap of the first pin for now -- Eventually it should
 * get the input cap of most critical input 
 */
double
sp_get_pin_load(node, model)
node_t *node;
delay_model_t model;
{
    double load;
    delay_pin_t *pin_delay;

    if (node->type == PRIMARY_INPUT){
	load = 0;
    } else if (BUFFER(node)->type == BUF){
	load = BUFFER(node)->impl.buffer->ip_load;
    } else {
	pin_delay = get_pin_delay(node, BUFFER(node)->cfi, model);
	load = pin_delay->load;
    }
    return load;
}

/*
 * Returns the drive of the node that fans into nodep. In the case of a
 * gate it will get the drive of the first fanin for now -- Eventually it
 * should get the drive of most critical fanin.  Also returns the phase of
 * selected fanin -- INVERTING || NONINVERTING || NEITHER
 */
delay_time_t
sp_get_input_drive(nodep, model, phase)
node_t *nodep;
delay_model_t model;
pin_phase_t *phase;	/* RETURN: The phase of the selected fanin */
{
    node_t *node;
    delay_time_t drive;
    delay_pin_t *pin_delay;

    if (nodep->type == PRIMARY_INPUT){
	drive.fall = drive.rise = 0.0;
	*phase = PHASE_NONINVERTING;
    } else {
	node = node_get_fanin(nodep, BUFFER(nodep)->cfi);
	if (BUFFER(node)->type == BUF){
	    drive = BUFFER(node)->impl.buffer->drive;
	    *phase = BUFFER(node)->impl.buffer->phase;
	} else {
	    pin_delay = get_pin_delay(node, 0, model);
	    drive = pin_delay->drive;
	    *phase = pin_delay->phase;
	}
    }
    return drive;
}


void
sp_subtract_delay(phase, block, drive, load, req)
pin_phase_t phase;
delay_time_t block, drive, *req;
double load;
{
    delay_time_t delay;

    delay.rise = block.rise + drive.rise * load;
    delay.fall = block.fall + drive.fall * load;

    sp_compute(phase, req, delay);
}

/*
 * Utility routine that will subtract "t" from "req" depending on
 * "phase" -- Used to update required times : "req" is changed in place
 */
void
sp_compute(phase, req, t)
pin_phase_t phase;
delay_time_t *req, t;
{
    delay_time_t ip_req;

    ip_req.rise = ip_req.fall = INFINITY;

    if (phase == PHASE_INVERTING || phase == PHASE_NEITHER) {
        ip_req.rise = MIN(ip_req.rise, (req->fall - t.fall));
        ip_req.fall = MIN(ip_req.fall, (req->rise - t.rise));
    }
    if (phase == PHASE_NONINVERTING || phase == PHASE_NEITHER) {
        ip_req.rise = MIN(ip_req.rise, (req->rise - t.rise));
        ip_req.fall = MIN(ip_req.fall, (req->fall - t.fall));
    }

    *req = ip_req;
}
/*
 * After a redistribution used to update required time data 
 * k is the index of the partition and nodeL is the buffer
 * that was added during the redistribution. Since the required
 * times and caps are known for the fanouts we pass them too.
 */
void
sp_buf_compute_req_time(node, model, req_times, cap_K, k, nodeL)
node_t *node, *nodeL;
delay_model_t model;
delay_time_t *req_times;
double cap_K;
int k;
{
    int i, cfi;
    double load;
    pin_phase_t phase;
    delay_time_t drive, block, t, best;
    delay_pin_t *pin_delay;

    if (node->type == PRIMARY_INPUT || BUFFER(node)->type == GATE){
	cfi = node->type == PRIMARY_INPUT ? 0 : BUFFER(node)->cfi;
	pin_delay = get_pin_delay(node, cfi, model);
	block = pin_delay->block;
	drive = pin_delay->drive;
	phase = pin_delay->phase;
    } else {
	phase = BUFFER(node)->impl.buffer->phase;
	drive = BUFFER(node)->impl.buffer->drive;
	block = BUFFER(node)->impl.buffer->block;
    }

    best.rise = best.fall = POS_LARGE;
    for (i = 0; i < k; i++) {
	best.rise = MIN(best.rise, req_times[i].rise);
	best.fall = MIN(best.fall, req_times[i].fall);
    }
    best.rise = MIN(best.rise, BUFFER(nodeL)->req_time.rise);
    best.fall = MIN(best.fall, BUFFER(nodeL)->req_time.fall);

    load = cap_K + sp_get_pin_load(nodeL, model) + compute_wire_load(node_network(node), 1);
    t.rise = block.rise + drive.rise * load; 
    t.fall = block.fall + drive.fall * load; 
    sp_compute(phase, &best, t);
    BUFFER(node)->req_time = best;
}



    /*
     * Set drive and load of Primary IP/OP respectively to be that of the
     * second (next to the smallest) inverter in the library
     */
int
buf_set_pipo_defaults(network, buf_param, load_table, drive_table)
network_t *network;
buf_global_t *buf_param;
st_table **load_table, **drive_table;
{
    lsGen gen;
    node_t *node;
    double load;
    delay_time_t drive;
    int setting_env;
    int default_inv_index;
    
    *load_table = st_init_table(st_ptrcmp, st_ptrhash);
    *drive_table = st_init_table(st_ptrcmp, st_ptrhash);
    default_inv_index = 
    (buf_param->num_inv > 1 ? buf_param->num_inv-2 : buf_param->num_inv-1);
    if (buf_param->model == DELAY_MODEL_MAPPED){
	load = (buf_param->buf_array[default_inv_index])->ip_load;
    } else {
	load = 1.0;
    }
    foreach_primary_output(network, gen, node){
	if (delay_get_parameter(node, DELAY_OUTPUT_LOAD) < 0){
	    setting_env = TRUE;
	    (void)st_insert(*load_table, (char *)node, NIL(char));
	    delay_set_parameter(node, DELAY_OUTPUT_LOAD, load);
	}
    }
    if (setting_env && buf_param->interactive){
	(void)fprintf(sisout,"WARNING: Unspecified primary output loads assumed to be %6.4f\n", load); 
    }
    
    setting_env = FALSE;
    if (buf_param->model == DELAY_MODEL_MAPPED){
	drive = (buf_param->buf_array[default_inv_index])->drive;
    } else {
	drive.rise = drive.fall = 0.2;
    }
    foreach_primary_input(network, gen, node){
	if (delay_get_parameter(node, DELAY_DRIVE_RISE) < 0 ||
	    delay_get_parameter(node, DELAY_DRIVE_FALL) < 0){
	    setting_env = TRUE;
	    (void)st_insert(*drive_table, (char *)node, NIL(char));
	    delay_set_parameter(node, DELAY_DRIVE_RISE, drive.rise);
	    delay_set_parameter(node, DELAY_DRIVE_FALL, drive.fall);
	}
    }
    if (setting_env && buf_param->interactive){
	(void)fprintf(sisout,"WARNING: Unspecified primary input drive assumed to be  (%6.4f:%6.4f)\n", drive.rise, drive.fall); 
    }
}
