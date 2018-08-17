
#include "sis.h"
#include "speed_int.h"

void speed_reset_arrival_time();

static delay_time_t speed_delay_node_pin();

static int speed_update_arrival_time_recur();

static delay_pin_t *get_pin_delay_of_function();

/* Analogous to delay_latest_output() ... However gets the most critical
 * PO and the corresponding slack time
 */
node_t *
sp_minimum_slack(network, min_slack)
        network_t *network;
        double *min_slack;
{
    lsGen        gen;
    node_t       *best, *node;
    delay_time_t cur_slack, slack;

    slack.rise = slack.fall = POS_LARGE;

    best = NIL(node_t);
    foreach_primary_output(network, gen, node)
    {
        cur_slack = delay_slack_time(node);
        if (MIN(cur_slack.rise, cur_slack.fall) < MIN(slack.rise, slack.fall)) {
            slack = cur_slack;
            best  = node;
        }
    }
    *min_slack = MIN(slack.rise, slack.fall);
    return best;
}

int
sp_po_req_times_set(network)
        network_t *network;
{
    int          flag;
    lsGen        gen;
    node_t       *node;
    delay_time_t req;

    if (delay_get_default_parameter(network, DELAY_DEFAULT_REQUIRED_RISE) != DELAY_NOT_SET ||
        delay_get_default_parameter(network, DELAY_DEFAULT_REQUIRED_FALL) != DELAY_NOT_SET) {
        return TRUE;
    }

    flag = FALSE;
    foreach_primary_output(network, gen, node)
    {
        flag = flag || delay_get_po_required_time(node, &req);
    }
    return flag;
}

/*
 *  If the delay model being used is the MAPPED one, then we
 * should get the delay through primitive NAND and use it instead
 * of always having to get it after a mapping of the node
 */
speed_set_delay_data(speed_param, use_accl
)
speed_global_t *speed_param;
int            use_accl;
{
library_t     *lib;
delay_model_t model   = speed_param->model;
double        pin_cap = -1.0;
delay_pin_t   *pin_delay;

speed_param->
library_accl = use_accl;

if (model == DELAY_MODEL_MAPPED){
lib       = lib_get_library();  /* Assured that the library is present */
pin_delay = get_pin_delay_of_function("f=!a;", lib);
speed_param->
inv_pin_delay = *pin_delay;
pin_cap       = MAX(pin_cap, pin_delay->load);
pin_delay     = get_pin_delay_of_function("f=!(a+b);", lib);
speed_param->
nand_pin_delay = *pin_delay;
pin_cap        = MAX(pin_cap, pin_delay->load);
speed_param->
pin_cap = pin_cap;
}
}

void
speed_set_library_accl(speed_param, value)
        speed_global_t *speed_param;
        int value;
{
    speed_param->library_accl = value;
}

int
speed_get_library_accl(speed_param)
        speed_global_t *speed_param;
{
    return speed_param->library_accl;
}

/*
 * Return the delay model of the smallest agte implementing the given fucction
 */
static delay_pin_t *
get_pin_delay_of_function(s, lib)
        char *s;
        library_t *lib;
{
    network_t   *network;
    lib_class_t *class;
    lib_gate_t  *gate, *best_gate;
    lsGen       gen;
    delay_pin_t *pin_delay;
    double      area, best_area;

    network   = read_eqn_string(s);
    class     = lib_get_class(network, lib);
    best_area = -1;
    best_gate = NIL(lib_gate_t);
    gen       = lib_gen_gates(class);
    while (lsNext(gen, (char **) &gate, LS_NH) == LS_OK) {
        area = lib_gate_area(gate);
        if (best_area < 0 || area < best_area) {
            best_area = area;
            best_gate = gate;
        }
    }
    (void) lsFinish(gen);
    network_free(network);

    pin_delay = (delay_pin_t * )(best_gate->delay_info[0]);
    return pin_delay;
}


int
speed_update_arrival_time(node, speed_param)
        node_t *node;
        speed_global_t *speed_param;
{
    delay_time_t time;

    if (!speed_update_arrival_time_recur(node, speed_param, &time)) {
        return 0;
    }
    return 1;
}

static int
speed_update_arrival_time_recur(node, speed_param, delay)
        node_t *node;
        speed_global_t *speed_param;
        delay_time_t *delay;
{
    int          i;
    double       temp;
    node_t       *fanin;
    delay_time_t t, time, fanin_time;

    delay->rise = NEG_LARGE;
    delay->fall = NEG_LARGE;

    /*
     * Recursion stops when a primary input is reached 
     * or if a node with computed delay values is found
     */

    if (node_function(node) == NODE_PI) {
        /*
         * Add the delay due to the fanouts
         */
        fanin_time = speed_delay_node_pin(node, 0, speed_param);
        if (speed_param->debug && (fanin_time.rise < 0.0 || fanin_time.fall < 0.0)) {
            (void) fprintf(sisout, "WARNING-1: speed_delay_node_pin\n");
        }

        temp = delay_get_parameter(node, DELAY_ARRIVAL_FALL);
        delay->fall = (temp < 0.0 ? 0.0 : temp) + fanin_time.fall;

        temp = delay_get_parameter(node, DELAY_ARRIVAL_RISE);
        delay->rise = (temp < 0.0 ? 0.0 : temp) + fanin_time.rise;

    } else if (node_function(node) == NODE_PO) {
        fanin = node_get_fanin(node, 0);
        if (!speed_update_arrival_time_recur(fanin, speed_param, &fanin_time)) {
            error_append("Failed in speed_update_arrival_time_recur");
            return 0;
        }
        *delay = fanin_time;
    } else {
        t.rise = delay_get_parameter(node, DELAY_ARRIVAL_RISE);
        t.fall = delay_get_parameter(node, DELAY_ARRIVAL_FALL);
        if ((t.fall < 0.0) || (t.rise < 0.0)) {
            /*
             * Recursively find the arrival time. Keep the max of the
             * (a_time+delay) as the arrival time at the node output
             */
            foreach_fanin(node, i, fanin)
            {
                if (!speed_update_arrival_time_recur(fanin, speed_param,
                                                     &fanin_time)) {
                    error_append("Failed in speed_update_arrival_time_recur");
                    return 0;
                }
                time = speed_delay_node_pin(node, i, speed_param);
                if (speed_param->debug && (time.rise < 0.0 || time.fall < 0.0)) {
                    (void) fprintf(sisout, "WARNING-2: speed_delay_node_pin\n");
                }

                /* Depending on the phase add the delay components */
                switch (node_input_phase(node, fanin)) {
                    case POS_UNATE :
                        delay->rise = MAX(delay->rise,
                                          (fanin_time.rise + time.rise));
                        delay->fall = MAX(delay->fall,
                                          (fanin_time.fall + time.fall));
                        break;
                    case NEG_UNATE :
                        delay->rise = MAX(delay->rise,
                                          (fanin_time.fall + time.rise));
                        delay->fall = MAX(delay->fall,
                                          (fanin_time.rise + time.fall));
                        break;
                    case BINATE :
                        delay->rise = MAX(delay->rise,
                                          (fanin_time.rise + time.rise));
                        delay->rise = MAX(delay->rise,
                                          (fanin_time.fall + time.rise));
                        delay->fall = MAX(delay->fall,
                                          (fanin_time.rise + time.fall));
                        delay->fall = MAX(delay->fall,
                                          (fanin_time.fall + time.fall));
                        break;
                    case PHASE_UNKNOWN :
                    default: break;
                }
            }
            if (node_num_literal(node) == 0) {
                delay->fall = 0.0;
                delay->rise = 0.0;
            }
            speed_set_arrival_time(node, *delay);
        } else {
            /* return the value of the delay */
            *delay = t;
        }
    }
    return 1;
}

void
speed_delay_arrival_time(node, speed_param, time)
        node_t *node;
        speed_global_t *speed_param;
        delay_time_t *time;
{
    node_t       *fanin;
    delay_time_t fanin_time;
    double       temp;

    if (node_function(node) == NODE_PO) {
        fanin = node_get_fanin(node, 0);
        speed_delay_arrival_time(fanin, speed_param, time);
    } else if (node_function(node) == NODE_PI) {
        /*
         * Add the delay due to the fanouts
         */
        fanin_time = speed_delay_node_pin(node, 0, speed_param);
        if (speed_param->debug && (fanin_time.rise < 0.0 || fanin_time.fall < 0.0)) {
            (void) fprintf(sisout, "WARNING-3: speed_delay_node_pin\n");
        }

        temp = delay_get_parameter(node, DELAY_ARRIVAL_FALL);
        time->fall = (temp < 0.0 ? 0.0 : temp) + fanin_time.fall;

        temp = delay_get_parameter(node, DELAY_ARRIVAL_RISE);
        time->rise = (temp < 0.0 ? 0.0 : temp) + fanin_time.rise;
    } else {
        temp = delay_get_parameter(node, DELAY_ARRIVAL_FALL);
        if (temp < 0) {
            (void) fprintf(sisout, "negative fall arrival time for");
            node_print(sisout, node);
            temp = 0;
        }
        time->fall = temp;
        temp = delay_get_parameter(node, DELAY_ARRIVAL_RISE);
        if (temp < 0) {
            (void) fprintf(sisout, "negative rise arrival time for");
            node_print(sisout, node);
            temp = 0;
        }
        time->rise = temp;
    }
}

void
speed_set_arrival_time(node, time)
        node_t *node;
        delay_time_t time;
{
    if ((time.rise < 0) || (time.fall < 0)) {
        (void) fprintf(sisout, "Setting negative arrival time %7.2f:%-7.2f for ",
                       time.rise, time.fall);
        node_print(sisout, node);
        delay_set_parameter(node, DELAY_ARRIVAL_FALL, 0.0);
        delay_set_parameter(node, DELAY_ARRIVAL_RISE, 0.0);
    }
    delay_set_parameter(node, DELAY_ARRIVAL_FALL, time.fall);
    delay_set_parameter(node, DELAY_ARRIVAL_RISE, time.rise);
}

int
speed_delay_trace(network, speed_param)
        network_t *network;
        speed_global_t *speed_param;
{
    lsGen  gen;
    node_t *po;

    foreach_node(network, gen, po)
    {
        if (po->type == INTERNAL) {
            speed_reset_arrival_time(po);
        }
    }
    foreach_primary_output(network, gen, po)
    {
        if (!speed_update_arrival_time(po, speed_param)) {
            (void) lsFinish(gen);
            return 0;
        }
    }

    return 1;
}

void
speed_single_level_update(node, speed_param, delay)
        node_t *node;
        speed_global_t *speed_param;
        delay_time_t *delay;
{
    int          i;
    node_t       *fanin;
    delay_time_t fanin_time, time;

    delay->rise = NEG_LARGE;
    delay->fall = NEG_LARGE;

    if (node->type == INTERNAL) {
        foreach_fanin(node, i, fanin)
        {
            speed_delay_arrival_time(fanin, speed_param, &fanin_time);
            time = speed_delay_node_pin(node, i, speed_param);
            if (speed_param->debug && (time.rise < 0.0 || time.fall < 0.0)) {
                (void) fprintf(sisout, "WARNING-4: speed_delay_node_pin\n");
            }

            /* Depending on the phase add the delay components */
            switch (node_input_phase(node, fanin)) {
                case POS_UNATE :
                    delay->rise = MAX(delay->rise,
                                      (fanin_time.rise + time.rise));
                    delay->fall = MAX(delay->fall,
                                      (fanin_time.fall + time.fall));
                    break;
                case NEG_UNATE :
                    delay->rise = MAX(delay->rise,
                                      (fanin_time.fall + time.rise));
                    delay->fall = MAX(delay->fall,
                                      (fanin_time.rise + time.fall));
                    break;
                case BINATE :
                    delay->rise = MAX(delay->rise,
                                      (fanin_time.rise + time.rise));
                    delay->rise = MAX(delay->rise,
                                      (fanin_time.fall + time.rise));
                    delay->fall = MAX(delay->fall,
                                      (fanin_time.rise + time.fall));
                    delay->fall = MAX(delay->fall,
                                      (fanin_time.fall + time.fall));
                    break;
                case PHASE_UNKNOWN : (void) fprintf(sisout, "Unknown phase\n");
                    break;
                default: break;
            }
        }
        if (node_num_literal(node) == 0) {
            delay->fall = 0.0;
            delay->rise = 0.0;
        }
        speed_set_arrival_time(node, *delay);
    }
}

void
speed_reset_arrival_time(node)
        node_t *node;
{
    delay_set_parameter(node, DELAY_ARRIVAL_RISE, DELAY_NOT_SET);
    delay_set_parameter(node, DELAY_ARRIVAL_FALL, DELAY_NOT_SET);
}


/* ARGSUSED */
void
speed_update_fanout(network, nodevec, fanout_list, speed_param)
        network_t *network;
        array_t *nodevec, *fanout_list;
        speed_global_t *speed_param;
{
    lsGen        gen;
    delay_time_t t;
    int          first, last;
    int          i, j, more_to_come;
    node_t       *node, *np, *fo;
    array_t      *tfi_array, *temp_array;
    st_table     *node_table, *temp_table;

    if (array_n(fanout_list) == 0) return;

    node_table = st_init_table(st_ptrcmp, st_ptrhash);
    temp_table = st_init_table(st_ptrcmp, st_ptrhash);
    temp_array = array_alloc(node_t * , 0);

    for (i = 0; i < array_n(fanout_list); i++) {
        node = array_fetch(node_t * , fanout_list, i);
        (void) st_insert(node_table, (char *) node, "");
        tfi_array = network_tfi(node, POS_LARGE);
        for (j    = 0; j < array_n(tfi_array); j++) {
            np = array_fetch(node_t * , tfi_array, j);
            (void) st_insert(node_table, (char *) np, "");
        }
        array_free(tfi_array);
    }

    for (i = 0; i < array_n(nodevec); i++) {
        node = array_fetch(node_t * , nodevec, i);
        array_insert_last(node_t * , temp_array, node);
    }


    first        = 0;
    more_to_come = TRUE;
    while (more_to_come) {
        more_to_come = FALSE;
        last         = array_n(temp_array);
        for (i       = first; i < last; i++) {
            node = array_fetch(node_t * , temp_array, i);
            if (st_is_member(node_table, (char *) node)) {
                (void) st_insert(temp_table, (char *) node, "");
                foreach_fanout(node, gen, fo)
                {
                    if ((!st_insert(temp_table, (char *) fo, "")) &&
                        (st_is_member(node_table, (char *) fo))) {
                        array_insert_last(node_t * , temp_array, fo);
                        more_to_come = TRUE;
                    }
                }
            }
        }
        first        = last;
    }
    st_free_table(node_table);
    array_free(temp_array);

    /* 
     * At this stage temp_table has all the nodes
     * that need to be re_evaluated . Sort them in a 
     * depth first manner and evaluate the delays
     */
    temp_array = network_dfs(network);
    for (i     = 0; i < array_n(temp_array); i++) {
        node = array_fetch(node_t * , temp_array, i);
        if (st_is_member(temp_table, (char *) node)) {
            speed_single_level_update(node, speed_param, &t);
        }
    }
    array_free(temp_array);
    st_free_table(temp_table);
}

static delay_time_t
speed_delay_node_pin(node, i, speed_param)
        node_t *node;
        int i;
        speed_global_t *speed_param;
{
    double        load;
    int           nin, nout;
    delay_model_t model = speed_param->model;
    delay_time_t  delay;
    delay_pin_t   *pin_delay;

    delay.rise = delay.fall = 0.0;

    if (model == DELAY_MODEL_MAPPED && speed_get_library_accl(speed_param) &&
        (nin = node_num_fanin(node)) < 3) {
        /* User decided to speed_up using the NAND-INV delay data */
        if (nin > 0) {
            if (nin == 2) {
                pin_delay = &(speed_param->nand_pin_delay);
            } else if (nin == 1) {
                pin_delay = &(speed_param->inv_pin_delay);
            }
            nout = node_num_fanout(node);
            load = compute_wire_load(node_network(node), nout) + nout * speed_param->pin_cap;
            delay.rise = pin_delay->drive.rise * load + pin_delay->block.rise;
            delay.fall = pin_delay->drive.fall * load + pin_delay->block.fall;
        }
    } else {
        delay = delay_node_pin(node, i, model);
    }
    return delay;
}


