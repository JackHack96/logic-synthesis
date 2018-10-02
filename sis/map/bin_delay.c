
/* file @(#)bin_delay.c	1.8 */
/* last modified on 7/24/91 at 16:46:37 */
#include "fanout_delay.h"
#include "fanout_int.h"
#include "map_delay.h"
#include "map_int.h"
#include "map_macros.h"
#include "sis.h"
#include <math.h>

static bin_global_t global;

#include "bin_delay_static.h"

static char errmsg[1024];
#ifdef SIS

static void hack_po();

#endif /* SIS */

/* performs minimum delay tree covering using piece-wise linear functions */
/* to model the achievable arrival times at each node as a function of the */
/* load. If the network was originally annotated, makes sure the loads at */
/* multiple fanout point is not any larger than the current load */

/* EXTERNAL INTERFACE */

/* ARGSUSED */
int bin_delay_area_tree_match(network_t *network, library_t *library, bin_global_t globals) {
    (void) fprintf(siserr, "ERROR: '-n x' with 0 < x < 1 no longer supported\n");
    return 0;
}

int bin_delay_tree_match(network_t *network, library_t *library, bin_global_t globals) {
    global = globals;
    global.library = library;
    init_delay_bucket_storage();
    bin_for_all_nodes_inputs_first(network, bin_alloc);
    if (!compute_best_match(network)) {
        return 0;
    }
#ifdef SIS
    hack_po(network);
#endif /* SIS */
    bin_for_all_nodes_outputs_first(network, select_best_gate);
    /*
      if (global.fanout_iterate) {
        fanout_est_save_fanin_fanout_indices(network);
      }
    */
    if (global.verbose > 1) {
        print_po_estimated_arrival_times(network);
    }
    bin_for_all_nodes_inputs_first(network, bin_free);
    free_delay_bucket_storage();
    return 1;
}

/* some utilities */

void bin_for_all_nodes_inputs_first(network_t *network, VoidFn fn) {
    int     i;
    array_t *nodevec;
    node_t  *node;

    nodevec = network_dfs(network);
    for (i  = 0; i < array_n(nodevec); i++) {
        node = array_fetch(node_t *, nodevec, i);
        (*fn)(node);
    }
    array_free(nodevec);
}

void bin_for_all_nodes_outputs_first(network_t *network, VoidFn fn) {
    int     i;
    array_t *nodevec;
    node_t  *node;

    nodevec = network_dfs_from_input(network);
    for (i  = 0; i < array_n(nodevec); i++) {
        node = array_fetch(node_t *, nodevec, i);
        (*fn)(node);
    }
    array_free(nodevec);
}

/* INTERNAL INTERFACE */

/* HOW delay_bucket_t's ARE USED */

/*
 * The delay information at a node is stored in BIN(node)->pwl
 * This only contains a pwl_delay_t pwl entry (and an area_bucket)
 *
 * typedef struct delay_bucket_struct {
 *   lib_gate_t *gate;
 *   int ninputs;
 *   node_t **save_binding;
 *   struct {
 *     pwl_t *rise;
 *     pwl_t *fall;
 *   } pwl;
 *   delay_bucket_t *next;	// used in chain of buckets to free them all
 * };
 *
 * Once the bucket is computed for that gate, it is added to the pwl stored at
 * the node by computing the max of bucket->pwl.rise and bucket->pwl.fall
 * and storing the result in the node with bin_delay_compute_pwl_delay_min.
 *
 * The main trick that makes it all work is that the pwl stored in a bucket
 * contains a data field for each segment of the PWL function. And for each
 * segment, this data field is just a pointer to the bucket itself. Now, when
 * several pwl's are merged using delay_min, only the best segments are saved.
 * and their data fields are pointing to the bucket (i.e. the gate info)
 * that realizes this minimum for a given load. Simple and elegant.
 *
 * ATTENTION! The buckets keep the distinction between rise and fall. Not
 * the pwl stored at the node. The pwl stored at the node computes the gate
 * that has the smallest MAX(rise,fall). Once the gate is selected, we can get
 * from the bucket the rise and fall info for that gate. Use
 * bin_delay_compute_pwl_delay for that.
 */

/* if the node was obtained from a fanout tree, it should stay mapped the way it
 * is */
/* we just compute its pwl so that we get an uniform interface for the gate
 * selection part */
/* otherwise, standard tree mapping stuff apply */
/* WARNING: a multiple fanout node can be an internal node, a PI or a constant
 * node. */

static int compute_best_match(network_t *network) {
    lsGen          gen;
    prim_t         *prim;
    delay_bucket_t *bucket;
    array_t        *nodevec;
    int            i;
    node_t         *node;
#ifdef SIS
    latch_t *latch;
#endif /* SIS */

    nodevec = network_dfs(network);
    for (i  = 0; i < array_n(nodevec); i++) {
        node = array_fetch(node_t *, nodevec, i);
#ifdef SIS
        if (node->type == PRIMARY_INPUT)
            continue;
        if (node->type == PRIMARY_OUTPUT && !IS_LATCH_END(node))
            continue;
#else
        if (node->type != INTERNAL)
          continue;
#endif /* SIS */
        if (node_num_fanin(node) == 0) {
            bucket = compute_constant_gate_bucket(node);
            bin_delay_update_node_pwl(node, bucket);
            continue;
        }
#ifdef SIS
        latch = NIL(latch_t);
        /* set latch type to rising-edge if it is not already set */
        if (IS_LATCH_END(node)) {
            latch = latch_from_node(node);
            if (latch == NIL(latch_t)) {
                (void) sprintf(errmsg, "latch node '%s' has no latch structure\n",
                               node_name(node));
                error_append(errmsg);
                return 0;
            }
        }
#endif /* SIS */
        lsForeachItem(global.library->patterns, gen, prim) {
            gen_all_matches(node, prim, find_best_delay, global.verbose > 5,
                            global.allow_internal_fanout, global.fanout_limit);
        }
        if (BIN(node)->pwl->n_points == 0) {
#ifdef SIS
            if (latch == NIL(latch_t) || latch_get_type(latch) == ASYNCH) {
#endif /* SIS */
                (void) sprintf(errmsg, "library error: no gate matches at %s\n",
                               node_name(node));
                error_append(errmsg);
                return 0;
#ifdef SIS
            } else {
                latch_synch_t latch_type;

                /* Synchronous latch - try an opposite polarity latch */
                latch_type = latch_get_type(latch);
                switch ((int) latch_type) {
                    case (int) RISING_EDGE:latch_set_type(latch, FALLING_EDGE);
                        break;
                    case (int) FALLING_EDGE:latch_set_type(latch, RISING_EDGE);
                        break;
                    case (int) ACTIVE_HIGH:latch_set_type(latch, ACTIVE_LOW);
                        break;
                    case (int) ACTIVE_LOW:latch_set_type(latch, ACTIVE_HIGH);
                        break;
                    default: fail("do_tree_match: bad latch type");
                        break;
                }
                lsForeachItem(global.library->patterns, gen, prim) {
                    gen_all_matches(node, prim, find_best_delay, global.verbose > 5,
                                    global.allow_internal_fanout, global.fanout_limit);
                }
                if (BIN(node)->pwl->n_points == 0) {
                    (void) sprintf(errmsg, "library error: no latch matches at %s\n",
                                   node_name(node));
                    error_append(errmsg);
                    return 0;
                } else {
                    node_t     *clock, *new_node, *fanin;
                    lib_gate_t *inv;

                    if (!global.no_warning)
                        (void) fprintf(siserr,
                                       "warning: no %s type latch matches at %s (opposite "
                                       "polarity latch used)\n",
                                       (latch_type == RISING_EDGE)
                                       ? "RISING_EDGE"
                                       : (latch_type == FALLING_EDGE)
                                         ? "FALLING_EDGE"
                                         : (latch_type == ACTIVE_HIGH)
                                           ? "ACTIVE_HIGH"
                                           : "ACTIVE_LOW",
                                       node_name(node));
                    /* Insert a small inverter at the clock */
                    inv   = choose_smallest_gate(global.library, "f=!a;");
                    clock = latch_get_control(latch);
                    assert(clock != NIL(node_t) && node_type(clock) == PRIMARY_OUTPUT);
                    fanin    = node_get_fanin(clock, 0);
                    new_node = node_alloc();
                    node_replace(new_node, node_literal(fanin, 0));
                    assert(node_patch_fanin(clock, fanin, new_node));
                    network_add_node(network, new_node);
                    map_alloc(new_node);
                    MAP(new_node)->gate             = inv;
                    MAP(new_node)->map_area         = inv->area;
                    MAP(new_node)->map_arrival.rise = MAP(fanin)->map_arrival.rise;
                    MAP(new_node)->map_arrival.fall = MAP(fanin)->map_arrival.fall;
                    MAP(new_node)->ninputs          = 1;
                    MAP(new_node)->save_binding     = ALLOC(node_t *, 1);
                    MAP(new_node)->save_binding[0] = fanin;
                }
            }
#endif /* SIS */
        }
        if (BIN(node)->fanout != NIL(multi_fanout_t)) {
            fanout_est_compute_fanout_info(node, &global);
            if (global.verbose > 1) {
                delay_time_t arrival;
                pin_info_t   pin_info;

                (void) fprintf(sisout, "node %s: ", node->name);
                pin_info.leaf         = node;
                pin_info.fanout_index = 0;
                pin_info.pin_polarity = POLAR_X;
                arrival = fanout_est_get_pin_arrival_time(&pin_info, 0.0);
                (void) fprintf(sisout, "a(X)=(%2.3f,%2.3f) ", arrival.rise,
                               arrival.fall);
                pin_info.pin_polarity = POLAR_Y;
                arrival = fanout_est_get_pin_arrival_time(&pin_info, 0.0);
                (void) fprintf(sisout, "a(Y)=(%2.3f,%2.3f)\n", arrival.rise,
                               arrival.fall);
            }
        }
    }
    array_free(nodevec);
    return 1;
}

/* goes through all the gates corresponding to the prim and record */
/* their delay information in the BIN(node)->pwl routine */
/* this means computing the arrival times at the input pins of the prim */
/* then making a new pwl pair (rise/fall) per gate */
/* then registrating it in the BIN(node)->pwl */
/* NOTE: also computes the optimal area and sticks it into MAP entry */
/* If the network is ANNOTATED already, add the following two modifications: */
/* (1) if gate has an input pin with a fanout > 1, the gate gets an very large
 */
/* arrival time, unless it is the current annotation */
/* (2) if the gate is a wire, and its input pin or the intermediate node */
/* has a fanout > 1, the wire gets a very large arrival time */

static pin_info_t DEFAULT_PIN_INFO = {{INFINITY, INFINITY}, 0, 0, -1, -1};

static int find_best_delay(prim_t *prim) {
    int            i;
    lsGen          gen;
    lib_gate_t     *gate;
    node_t         *node;
    int            ninputs;
    delay_bucket_t *bucket;
    pin_info_t     *pin_info;

#ifdef SIS
    /* Make sure that we distinguish between combinational gates and latches. */
    if (seq_filter(prim, global.verbose)) {
#endif /* SIS */
        node = map_prim_get_root(prim);

        /* first make a copy of the input bindings */
        ninputs  = prim->ninputs;
        pin_info = ALLOC(pin_info_t, ninputs);
        for (i   = 0; i < ninputs; i++) {
            pin_info[i] = DEFAULT_PIN_INFO;
            pin_info[i].input = prim->inputs[i]->binding;
            fanout_est_get_prim_pin_fanout(prim, i, &pin_info[i]);
        }

        /* then try all gates associated to the prim */
        lsForeachItem(prim->gates, gen, gate) {
            if (global.verbose > 2) {
                (void) fprintf(sisout, "trying gate %s at node %s...\n", gate->name,
                               node->name);
            }
            if (strcmp(gate->name, "**wire**") == 0) {
                bucket = compute_wire_bucket(gate, pin_info[0].input);
            } else {
                bucket = bin_delay_compute_gate_bucket(gate, ninputs, pin_info);
            }
            bin_delay_update_node_pwl(node, bucket);
            preserve_best_area(&(BIN(node)->area), gate, ninputs, pin_info);
        }
        FREE(pin_info);
#ifdef SIS
    }
#endif /* SIS */
    return 1;
}

/* computes the optimal area */

static void preserve_best_area(area_bucket_t *bucket, lib_gate_t *gate, int ninputs, pin_info_t *pin_info) {
    int    i;
    double area = gate->area;

    for (i = 0; i < ninputs; i++) {
        area += get_area_from_pin_info(&pin_info[i]);
    }
    if (area >= bucket->area)
        return;
    bucket->area = area;
    bucket->gate = gate;
    if (bucket->ninputs > 0)
        FREE(bucket->save_binding);
    bucket->ninputs = ninputs;
    if (bucket->ninputs == 0)
        return;
    bucket->save_binding = ALLOC(node_t *, bucket->ninputs);
    for (i = 0; i < bucket->ninputs; i++) {
        bucket->save_binding[i] = get_input_from_pin_info(&pin_info[i]);
    }
}

/* simply copies the pwl of the source node */
/* except if there is a node with multiple fanout in the middle */
/* and some special wire optimization is active */
/* Wires are forbidden to feed to a multiple fanout point */

static delay_bucket_t *compute_wire_bucket(lib_gate_t *gate, node_t *input) {
    delay_bucket_t *bucket;
    int            infinity_flag;

    infinity_flag = (BIN(input)->fanout != NIL(multi_fanout_t)) ? 1 : 0;
    bucket        = delay_bucket_alloc(WIRE_BUCKET);
    bucket->gate         = gate;
    bucket->ninputs      = 1;
    bucket->save_binding = ALLOC(node_t *, 1);
    bucket->save_binding[0] = input;
    bucket->pin_info = ALLOC(pin_info_t, 1);
    bucket->pin_info[0] = DEFAULT_PIN_INFO;
    if (infinity_flag) {
        bucket->pwl.rise = gen_infinitely_slow_pwl();
    } else {
        bucket->pwl.rise = pwl_dup(BIN(input)->pwl);
    }
    bucket->pwl.fall    = pwl_dup(bucket->pwl.rise);
    return bucket;
}

static pwl_t *gen_infinitely_slow_pwl() {
    pwl_point_t point;

    point.x     = 0.0;
    point.y     = INFINITY;
    point.slope = INFINITY;
    point.data  = NIL(char);
    return pwl_create_linear_max(1, &point);
}

/* in case of constant node: starting time is set to 0.0 */
/* slope and intrinsic is set to 0 */
/* trust that nodes that are constant have been premapped */

static delay_bucket_t *compute_constant_gate_bucket(node_t *node) {
    delay_bucket_t *bucket;

    assert(MAP(node)->gate != NIL(lib_gate_t));
    bucket = delay_bucket_alloc(CONSTANT_BUCKET);
    bucket->gate     = MAP(node)->gate;
    bucket->pwl.rise = gen_constant_pwl();
    bucket->pwl.fall = gen_constant_pwl();
    pwl_set_data(bucket->pwl.rise, (char *) bucket);
    pwl_set_data(bucket->pwl.fall, (char *) bucket);
    return bucket;
}

static pwl_t *gen_constant_pwl() {
    pwl_point_t point;

    point.x     = 0.0;
    point.y     = 0.0;
    point.slope = 0.0;
    point.data  = NIL(char);
    return pwl_create_linear_max(1, &point);
}

/* compute the pwl delay model at the node for that gate */
/* The infinity_flag is set only if one of the input pins */
/* has a fanout > 1 and the network is ANNNOTATED and current gate */
/* is not the gate used in annotation */
/* Why not just computing the pwl_delay for pin_node's that are not PI's? */
/* because of rise and fall delays. The best pwl_delay may be obtained for
 * different gates. */
/* compute_consistent_pwl_delay guarantees a realizable, if not always optimal,
 * value. */
/* NOTE: exported to fanout_est.c */

delay_bucket_t *bin_delay_compute_gate_bucket(lib_gate_t *gate, int ninputs, pin_info_t *pin_info) {
    int            i;
    double         pin_load;
    delay_bucket_t *bucket;

    /* alloc a bucket and set it up */
    bucket = delay_bucket_alloc(GATE_BUCKET);
    bucket->gate         = gate;
    bucket->ninputs      = ninputs;
    bucket->save_binding = ALLOC(node_t *, ninputs);
    bucket->pin_info     = ALLOC(pin_info_t, ninputs);

    for (i      = 0; i < ninputs; i++) {
        bucket->save_binding[i] = get_input_from_pin_info(&pin_info[i]);
        bucket->pin_info[i]     = pin_info[i];
        pin_load = delay_get_load(gate->delay_info[i]);
        pin_info[i].arrival = get_arrival_from_pin_info(&pin_info[i], pin_load);
        if (global.verbose > 2) {
            (void) fprintf(sisout, "pin %d ", i);
            (void) fprintf(sisout, "a(%2.3f,%2.3f) l(%2.3f)\n",
                           pin_info[i].arrival.rise, pin_info[i].arrival.fall,
                           pin_load);
        }
    }
    bucket->pwl = bin_delay_compute_gate_pwl(gate, ninputs, pin_info);
    pwl_set_data(bucket->pwl.rise, (char *) bucket);
    pwl_set_data(bucket->pwl.fall, (char *) bucket);
    return bucket;
}

delay_pwl_t bin_delay_compute_gate_pwl(lib_gate_t *gate, int ninputs, pin_info_t *pin_info) {
    int         n_points;
    delay_pwl_t result;
    int         i, pt;
    pwl_point_t *rise_points, *fall_points;
    delay_pin_t *pin_delay, **lib_delay_model;

    /* count the number of points: two points per 'PHASE_NEITHER' pin */
    lib_delay_model = (delay_pin_t **) gate->delay_info;
    for (n_points   = 0, i = 0; i < ninputs; i++) {
        pin_delay = lib_delay_model[i];
        n_points += (pin_delay->phase == PHASE_NEITHER) ? 2 : 1;
    }
    rise_points            = ALLOC(pwl_point_t, n_points);
    fall_points            = ALLOC(pwl_point_t, n_points);

    /* store info in points; there is no data field at this point */
    for (pt = 0, i = 0; i < ninputs; i++) {
        pin_delay = lib_delay_model[i];
        switch (pin_delay->phase) {
            case PHASE_INVERTING:rise_points[pt].x = 0.0;
                rise_points[pt].y                  = pin_info[i].arrival.fall + pin_delay->block.rise;
                rise_points[pt].slope              = pin_delay->drive.rise;
                rise_points[pt].data               = NIL(char);
                fall_points[pt].x                  = 0.0;
                fall_points[pt].y                  = pin_info[i].arrival.rise + pin_delay->block.fall;
                fall_points[pt].slope              = pin_delay->drive.fall;
                rise_points[pt].data               = NIL(char);
                pt++;
                break;
            case PHASE_NONINVERTING:rise_points[pt].x = 0.0;
                rise_points[pt].y                     = pin_info[i].arrival.rise + pin_delay->block.rise;
                rise_points[pt].slope                 = pin_delay->drive.rise;
                rise_points[pt].data                  = NIL(char);
                fall_points[pt].x                     = 0.0;
                fall_points[pt].y                     = pin_info[i].arrival.fall + pin_delay->block.fall;
                fall_points[pt].slope                 = pin_delay->drive.fall;
                rise_points[pt].data                  = NIL(char);
                pt++;
                break;
            case PHASE_NEITHER:rise_points[pt].x = 0.0;
                rise_points[pt].y                = pin_info[i].arrival.rise + pin_delay->block.rise;
                rise_points[pt].slope            = pin_delay->drive.rise;
                rise_points[pt].data             = NIL(char);
                fall_points[pt].x                = 0.0;
                fall_points[pt].y                = pin_info[i].arrival.fall + pin_delay->block.fall;
                fall_points[pt].slope            = pin_delay->drive.fall;
                rise_points[pt].data             = NIL(char);
                pt++;
                rise_points[pt].x     = 0.0;
                rise_points[pt].y     = pin_info[i].arrival.fall + pin_delay->block.rise;
                rise_points[pt].slope = pin_delay->drive.rise;
                rise_points[pt].data  = NIL(char);
                fall_points[pt].x     = 0.0;
                fall_points[pt].y     = pin_info[i].arrival.rise + pin_delay->block.fall;
                fall_points[pt].slope = pin_delay->drive.fall;
                rise_points[pt].data  = NIL(char);
                pt++;
                break;
        }
    }
    assert(pt == n_points);
    result.rise = pwl_create_linear_max(n_points, rise_points);
    result.fall = pwl_create_linear_max(n_points, fall_points);
    FREE(rise_points);
    FREE(fall_points);
    return result;
}

delay_time_t bin_delay_compute_pwl_delay(pwl_t *pwl, double load) {
    delay_pwl_t delay_pwl;

    delay_pwl = bin_delay_select_active_pwl_delay(pwl, load);
    assert(delay_pwl.rise != NIL(pwl_t));
    return bin_delay_compute_delay_pwl_delay(delay_pwl, load);
}

delay_time_t bin_delay_compute_delay_pwl_delay(delay_pwl_t pwl, double load) {
    pwl_point_t  *p_rise;
    pwl_point_t  *p_fall;
    delay_time_t result;

    p_rise = pwl_lookup(pwl.rise, load);
    result.rise = pwl_point_eval(p_rise, load);
    p_fall = pwl_lookup(pwl.fall, load);
    result.fall = pwl_point_eval(p_fall, load);
    return result;
}

/* the pwl given here is associated to a node. */
/* It contains the MIN of MAX(rise,fall) for each gate matching at that node */
/* the first thing to do is to find the bucket corresponding to the best gate.
 */
/* When this is done, there is two cases left: either the bucket is a gate, so
 * we can go ahead and */
/* compute the delay directly using the pwl.rise and pwl.fall in that bucket. Or
 * the bucket is a */
/* wire, and in that case, we continue recursively. Since in the case of a wire,
 * pwl.rise == */
/* pwl.fall we can simply recur on pwl.rise. */

delay_pwl_t bin_delay_select_active_pwl_delay(pwl_t *pwl, double load) {
    pwl_point_t    *point;
    delay_bucket_t *bucket;

    point  = pwl_lookup(pwl, load);
    bucket = (delay_bucket_t *) pwl_point_data(point);
    switch (bucket->type) {
        case GATE_BUCKET:
        case PI_BUCKET:
        case CONSTANT_BUCKET:return bucket->pwl;
        case WIRE_BUCKET:return bin_delay_select_active_pwl_delay(bucket->pwl.rise, load);
        default: fail("unexpected bucket type");
            /* NOTREACHED */
    }
}

/* given a new bucket, add the gate instance delay info to what is there in the
 * node already */
/* trivial, except for the debugging print outs really. */

void bin_delay_update_node_pwl(node_t *node, delay_bucket_t *bucket) {
    pwl_t *max_pwl, *min_pwl;

    max_pwl = get_bucket_pwl_max(bucket);
    min_pwl = pwl_min(BIN(node)->pwl, max_pwl);
    if (global.verbose > 2) {
        (void) fprintf(sisout, "new rise = ");
        pwl_print(sisout, bucket->pwl.rise, delay_bucket_print);
        (void) fprintf(sisout, "new fall = ");
        pwl_print(sisout, bucket->pwl.fall, delay_bucket_print);
        (void) fprintf(sisout, "max rise & fall = ");
        pwl_print(sisout, max_pwl, delay_bucket_print);
        (void) fprintf(sisout, "old = ");
        pwl_print(sisout, BIN(node)->pwl, delay_bucket_print);
        (void) fprintf(sisout, "new = ");
        pwl_print(sisout, min_pwl, delay_bucket_print);
    }
    pwl_free(BIN(node)->pwl);
    pwl_free(max_pwl);
    BIN(node)->pwl = min_pwl;
}

/* given the delay info at a bucket, produce a pwl that is the max */
/* of the rise and fall info at that bucket */
/* make sure that the max_pwl contains a pointer back to the bucket */
/* we cannot put that pointer in bucket->pwl.{rise,fall} because */
/* it would not work for wires: we need to keep back the info of gate origin */
/* the pwl at a WIRE_BUCKET is just the copy of the BIN(node)->pwl of its input
 */
/* thus there is no need to compute a max or sum. */

static pwl_t *get_bucket_pwl_max(delay_bucket_t *bucket) {
    pwl_t *max_pwl;

    if (bucket->type == WIRE_BUCKET) {
        max_pwl = pwl_dup(bucket->pwl.rise);
    } else if (global.cost_function == 0) {
        max_pwl = pwl_max(bucket->pwl.rise, bucket->pwl.fall);
    } else {
        max_pwl = pwl_sum(bucket->pwl.rise, bucket->pwl.fall);
    }
    pwl_set_data(max_pwl, (char *) bucket);
    return max_pwl;
}

static void delay_bucket_print(FILE *fp, delay_bucket_t *bucket) {
    int i;

    if (bucket == NIL(delay_bucket_t)) {
        (void) fprintf(fp, "--");
    } else if (bucket->type == PI_BUCKET) {
        /* do nothing */
    } else {
        (void) fprintf(fp, "gate=%s [", bucket->gate->name);
        for (i = 0; i < bucket->ninputs; i++) {
            (void) fprintf(fp, "%s ", bucket->save_binding[i]->name);
        }
        (void) fprintf(fp, "] 0x%x 0x%x", (int) bucket->pwl.rise,
                       (int) bucket->pwl.fall);
    }
}

/* GATE_SELECTION */

/* where the final selection of gates takes place */
/* go through the network and select the best gates, given the loads */
/* at the outputs. Put the resulting selection into the MAP fields */
/* a node participates if root or pin of a final gate */
/* otherwise its load stays to INFINITY and we can ignore it */

static void select_best_gate(node_t *node) {
    double          load;
    pwl_point_t     *point;
    delay_bucket_t  *bucket;
    node_function_t node_fn;

    if (BIN(node)->visited == 0) {
        if (MAP(node)->gate != NIL(lib_gate_t)) {
            MAP(node)->gate    = NIL(lib_gate_t);
            MAP(node)->ninputs = 0;
            FREE(MAP(node)->save_binding);
            MAP(node)->save_binding = NIL(node_t *);
        }
        return;
    }
    node_fn = node_function(node);
    if (node_fn == NODE_0 || node_fn == NODE_1)
        return;
    if (node_fn == NODE_PO) {
        copy_bucket_to_map(node, NIL(delay_bucket_t));
        if (global.verbose > 1)
            map_report_node_data(node);
        return;
    }
    if (node_fn == NODE_PI) {
        if (global.verbose > 1)
            map_report_node_data(node);
        return;
    }
    if (BIN(node)->fanout != NIL(multi_fanout_t)) {
        load = BIN(node)->fanout->loads[BIN(node)->fanout->polarity];
    } else {
        load = MAP(node)->load;
    }
    point   = pwl_lookup(BIN(node)->pwl, load);
    bucket  = (delay_bucket_t *) pwl_point_data(point);
    copy_bucket_to_map(node, bucket);
    assert(MAP(node)->gate != NIL(lib_gate_t));
    if (global.verbose > 1)
        map_report_node_data(node);
    increment_input_loads(node, bucket);
}

/* install into MAP fields contents of selected bucket  */

static void copy_bucket_to_map(node_t *node, delay_bucket_t *bucket) {
    int        i;
    int        n_inputs;
    pin_info_t pin_info;

    n_inputs = (node->type == PRIMARY_OUTPUT) ? 1 : bucket->ninputs;
    if (n_inputs > 0) {
        if (MAP(node)->pin_info != NIL(fanin_fanout_t)) {
            FREE(MAP(node)->pin_info);
            MAP(node)->pin_info = NIL(fanin_fanout_t);
        }
        MAP(node)->pin_info = ALLOC(fanin_fanout_t, n_inputs);
    }

    if (node->type == PRIMARY_OUTPUT) {
        pin_info.input = node;
        fanout_est_get_prim_pin_fanout(NIL(prim_t), 0, &pin_info);
        MAP(node)->pin_info[0].pin_source   = pin_info.leaf;
        MAP(node)->pin_info[0].fanout_index = pin_info.fanout_index;
        return;
    }

    MAP(node)->gate = bucket->gate;
    MAP(node)->map_arrival =
            bin_delay_compute_pwl_delay(BIN(node)->pwl, MAP(node)->load);
    MAP(node)->ninputs = bucket->ninputs;
    if (MAP(node)->save_binding != NIL(node_t *)) {
        FREE(MAP(node)->save_binding);
    }
    if (bucket->ninputs > 0) {
        MAP(node)->save_binding = ALLOC(node_t *, bucket->ninputs);
    }
    for (i = 0; i < bucket->ninputs; i++) {
        MAP(node)->save_binding[i] = bucket->save_binding[i];
        BIN(bucket->save_binding[i])->visited = 1;
        MAP(node)->pin_info[i].pin_source   = bucket->pin_info[i].leaf;
        MAP(node)->pin_info[i].fanout_index = bucket->pin_info[i].fanout_index;
    }
}

static void increment_input_loads(node_t *node, delay_bucket_t *bucket) {
    int    pin;
    node_t *fanin;

    /*
     * multiple fanout node: if best match is inverter, take the right load
     * only case when node has n_fanouts > 1 and node is NODE_Y and node mapped as
     * inverter
     */

    if (BIN(node)->fanout != NIL(multi_fanout_t) && bucket->ninputs == 1) {
        assert(BIN(node)->fanout->polarity == POLAR_Y);
        fanin = bucket->save_binding[0];
        if (fanin == MAP(node)->fanout_source) {
            MAP(fanin)->load = BIN(node)->fanout->loads[POLAR_X];
        } else {
            MAP(fanin)->load = delay_get_load(bucket->gate->delay_info[0]);
        }
        return;
    }

    /*
     * if we allow logic duplication and this node is a multiple fanout node
     * we should make sure that the load of the other source is selected properly.
     * This is only necessary when the node is not a constant or a PRIMARY_INPUT.
     * A node cannot be a PRIMARY_INPUT or a constant here: should have been
     * caught earlier.
     */

    if (BIN(node)->fanout != NIL(multi_fanout_t) && global.allow_duplication) {
        assert(BIN(node)->fanout->polarity == POLAR_Y);
        assert(BIN(node)->fanout->best_is_inverter);
        fanin = node_get_fanin(node, 0);
        MAP(fanin)->load = BIN(node)->fanout->loads[POLAR_X];
        return;
    }

    /* wire: take the load at the node and propagate it through */
    if (bucket->ninputs == 1 && strcmp(bucket->gate->name, "**wire**") == 0) {
        fanin = bucket->save_binding[0];
        MAP(fanin)->load += MAP(node)->load;
        return;
    }

    /* case of a complex gate */
    for (pin = 0; pin < bucket->ninputs; pin++) {
        fanin = bucket->save_binding[pin];
        MAP(fanin)->load += delay_get_load(bucket->gate->delay_info[pin]);
    }
}

/* would be nice to have this arbitrary delay model at PI's */
/* right now, we simply have a single point */

static void bin_init_pi(node_t *node) {
    delay_bucket_t *bucket;

    bucket = delay_bucket_alloc(PI_BUCKET);
    bucket->pwl = bin_delay_get_pi_delay_pwl(node);
    pwl_set_data(bucket->pwl.rise, (char *) bucket);
    pwl_set_data(bucket->pwl.fall, (char *) bucket);
    bin_delay_update_node_pwl(node, bucket);
}

delay_pwl_t bin_delay_get_pi_delay_pwl(node_t *node) {
    delay_pwl_t  result;
    pwl_point_t  rise_point;
    pwl_point_t  fall_point;
    delay_time_t drive, arrival;

    assert(node->type == PRIMARY_INPUT);
    drive   = pipo_get_pi_drive(node);
    arrival = pipo_get_pi_arrival(node);
    rise_point.x     = 0.0;
    rise_point.y     = arrival.rise;
    rise_point.slope = drive.rise;
    rise_point.data  = NIL(char);
    fall_point.x     = 0.0;
    fall_point.y     = arrival.fall;
    fall_point.slope = drive.fall;
    fall_point.data  = NIL(char);
    result.rise      = pwl_create_linear_max(1, &rise_point);
    result.fall      = pwl_create_linear_max(1, &fall_point);
    return result;
}

/* MEMORY ALLOCATION ROUTINES */

static bin_t         DEFAULT_BIN_STRUCT  = {0, {0, 0, 0, 0.0}, 0, 0};
static fanout_leaf_t DEFAULT_FANOUT_LEAF = {INFINITY, 0};

/* should be visited inputs first */
/* the load is set to infinite unless the network is annotated */
/* the node has multiple fanout and is mapped */

static void bin_alloc(node_t *node) {
    int             i;
    int             p;
    node_t          *fanin;
    node_t          *fanout;
    int             n_fanouts;
    node_function_t type;

    /* fanout consistency check */
    if (node_num_fanout(node) > 1 && node_num_fanin(node) > 0) {
        assert(node_function(node) == NODE_INV);
        fanin = node_get_fanin(node, 0);
        type  = node_function(fanin);
        assert(type == NODE_PI || type == NODE_0 || type == NODE_1 ||
               type == NODE_OR || type == NODE_AND);
    }

    node->BIN_SLOT = (char *) ALLOC(bin_t, 1);
    *(BIN(node)) = DEFAULT_BIN_STRUCT;
    BIN(node)->pwl = pwl_create_empty();

    MAP(node)->load = 0.0;

    /* take care of PO's first */
    if (node->type == PRIMARY_OUTPUT) {
        BIN(node)->visited = 1;
        MAP(node)->load    = pipo_get_po_load(node);
        fanin = node_get_fanin(node, 0);
        BIN(fanin)->visited = 1;
        MAP(fanin)->load += MAP(node)->load;
        return;
    }

    /* take care of PI's first: beware, a PI may have n_fanouts > 1 */
    if (node->type == PRIMARY_INPUT) {
        bin_init_pi(node);
    }

    /* take care of nodes with n_fanouts > 1  */
    n_fanouts = node_num_fanout(node);
    if (n_fanouts > 1) {
        BIN(node)->fanout                   = ALLOC(multi_fanout_t, 1);
        BIN(node)->fanout->best_is_inverter = 0;
        BIN(node)->fanout->n_fanouts        = n_fanouts;
        BIN(node)->fanout->buckets          = NIL(fanout_bucket_t);
        BIN(node)->fanout->index_table      = st_init_table(st_ptrcmp, st_ptrhash);
        for (i = 0; i < n_fanouts; i++) {
            fanout = node_get_fanout(node, i);
            st_insert(BIN(node)->fanout->index_table, (char *) fanout, (char *) i);
        }
        BIN(node)->fanout->loads[POLAR_X] = INFINITY;
        BIN(node)->fanout->loads[POLAR_Y] = INFINITY;
        BIN(node)->fanout->polarity =
                (node_num_fanin(node) == 0) ? POLAR_X : POLAR_Y;
        foreach_polarity(p) {
            BIN(node)->fanout->leaves[p] = ALLOC(fanout_leaf_t, n_fanouts);
            for (i = 0; i < n_fanouts; i++) {
                BIN(node)->fanout->leaves[p][i] = DEFAULT_FANOUT_LEAF;
            }
        }
    }
}

/* the delay_bucket_t are freed elsewhere */

static void bin_free(node_t *node) {
    int p;

    pwl_free(BIN(node)->pwl);
    if (BIN(node)->area.ninputs > 0) {
        FREE(BIN(node)->area.save_binding);
    }
    if (BIN(node)->fanout) {
        fanout_est_free_fanout_info(node);
        st_free_table(BIN(node)->fanout->index_table);
        foreach_polarity(p) { FREE(BIN(node)->fanout->leaves[p]); }
        FREE(BIN(node)->fanout);
    }
    FREE(node->BIN_SLOT);
    node->BIN_SLOT = NIL(char);
}

/* need some way to allocate and free the buckets with gate information */

static delay_bucket_t *global_head_bucket;

static void init_delay_bucket_storage() {
    global_head_bucket = NIL(delay_bucket_t);
}

static void free_delay_bucket_storage() {
    delay_bucket_t *head;
    delay_bucket_t *next;

    for (head = global_head_bucket; head != NIL(delay_bucket_t); head = next) {
        pwl_free(head->pwl.rise);
        pwl_free(head->pwl.fall);
        if (head->ninputs > 0) {
            FREE(head->save_binding);
        }
        if (head->pin_info != NIL(pin_info_t)) {
            FREE(head->pin_info);
        }
        next = head->next;
        FREE(head);
    }
}

static delay_bucket_t *delay_bucket_alloc(delay_bucket_type_t type) {
    delay_bucket_t *bucket;

    bucket = ALLOC(delay_bucket_t, 1);
    bucket->type         = type;
    bucket->pwl.rise     = NIL(pwl_t);
    bucket->pwl.fall     = NIL(pwl_t);
    bucket->gate         = NIL(lib_gate_t);
    bucket->ninputs      = 0;
    bucket->save_binding = NIL(node_t *);
    bucket->pin_info     = NIL(pin_info_t);
    bucket->next         = global_head_bucket;
    global_head_bucket = bucket;
    return bucket;
}

static void print_po_estimated_arrival_times(network_t *network) {
    lsGen  gen;
    node_t *node;

    foreach_primary_output(network, gen, node) { map_report_node_data(node); }
}

/* if no multiple fanout, no problem */
/* if multiple fanout and signal of polarity POLAR_Y */
/* (but convention, polarity of multiple fanout point) */
/* then return the multiple fanout point */
/* For POLAR_X signals, it depends. If multiple fanout point */
/* is going to be implemented as an inverter, then can take its fanin */
/* otherwise, we take the corresponding fanout */

static node_t *get_input_from_pin_info(pin_info_t *pin_info) {
    if (pin_info->leaf == NIL(node_t)) {
        return pin_info->input;
    } else if (pin_info->pin_polarity == BIN(pin_info->leaf)->fanout->polarity) {
        return pin_info->leaf;
    } else if (BIN(pin_info->leaf)->fanout->best_is_inverter) {
        return node_get_fanin(pin_info->leaf, 0);
    } else {
        return node_get_fanout(pin_info->leaf, pin_info->fanout_index);
    }
}

static double get_area_from_pin_info(pin_info_t *pin_info) {
    if (pin_info->leaf == NIL(node_t)) {
        return BIN(pin_info->input)->area.area;
    } else {
        return BIN(pin_info->leaf)->area.area /
               BIN(pin_info->leaf)->fanout->n_fanouts;
    }
}

static delay_time_t get_arrival_from_pin_info(pin_info_t *pin_info, double load) {
    if (pin_info->leaf == NIL(node_t)) {
        return bin_delay_compute_pwl_delay(BIN(pin_info->input)->pwl, load);
    } else {
        return fanout_est_get_pin_arrival_time(pin_info, load);
    }
}

#ifdef SIS

/* After initial mapping, the function associated with
 * latches are temporarily stored at PO/latch-input nodes.
 * Now create an extra buffer node to move the func info to.
 * Also modify the fanout of the new node
 * such that it points to the real PO.
 * Before:  (internal node) -> real PO (no func)
 *                          -> PO/latch-input (has latch func)
 *                          -> PO/latch-input (has latch func)
 *                                 ...
 * After:   (internal node) -> real PO (no func)
 *                          -> (internal node - has latch func)     ->
 * PO/latch-input (no func)
 *                          -> (internal node - has latch func)     ->
 * PO/latch-input (no func)
 *                                 ...
 */
static void hack_po(network_t *network) {
    lsGen   gen;
    node_t  *po, *fanin, *new_node;
    int     i;
    latch_t *latch;

    foreach_latch(network, gen, latch) {
        po       = latch_get_input(latch);
        fanin    = node_get_fanin(po, 0);
        new_node = node_alloc();
        node_replace(new_node, node_literal(fanin, 1));
        assert(node_patch_fanin(po, fanin, new_node));
        network_add_node(network, new_node);
        /* PO derives its output from its fanin node */
        /* Swap names to preserve the IO names*/
        network_swap_names(network, new_node, fanin);
        map_alloc(new_node);
        bin_alloc(new_node);
        BIN(new_node)->pwl               = pwl_dup(BIN(po)->pwl);
        BIN(new_node)->area.gate         = BIN(po)->area.gate;
        BIN(new_node)->area.ninputs      = BIN(po)->area.ninputs;
        BIN(new_node)->area.save_binding = ALLOC(node_t *, BIN(po)->area.ninputs);
        for (i = 0; i < BIN(po)->area.ninputs; i++) {
            BIN(new_node)->area.save_binding[i] = BIN(po)->area.save_binding[i];
        }
        BIN(new_node)->area.area = BIN(po)->area.area;
        BIN(new_node)->visited   = 1;
    }
}

#endif /* SIS */
