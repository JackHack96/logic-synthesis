
/* file @(#)fanout_delay.c	1.8 */
/* last modified on 7/3/91 at 14:51:37 */
/*
 * $Log: fanout_delay.c,v $
 * Revision 1.1.1.1  2004/02/07 10:14:24  pchong
 * imported
 *
 * Revision 1.3  1992/05/06  18:55:51  sis
 * SIS release 1.1
 *
 * Revision 1.3  1992/05/06  18:55:51  sis
 * SIS release 1.1
 *
 * Revision 1.2  1992/04/17  21:53:13  sis
 * *** empty log message ***
 *
 * Revision 1.1  92/01/08  17:40:35  sis
 * Initial revision
 *
 * Revision 1.3  91/07/02  10:52:13  touati
 * add support to check for load_limit violations.
 * when violated, load is multiplied by a prespecified constant value.
 *
 * Revision 1.2  91/06/29  22:54:36  touati
 * added support for non-inverting buffers.
 *
 * Revision 1.1  91/06/28  22:17:20  touati
 * Initial revision
 *
 */
#include "fanout_delay.h"
#include "bin_int.h"
#include "fanout_int.h"
#include "map_int.h"
#include "sis.h"
#include <math.h>

typedef enum buffer_type_enum buffer_type_t;
enum buffer_type_enum {
    BUFFER_NORMAL, BUFFER_INVERT
};

typedef struct delay_buffer_struct delay_buffer_t;
struct delay_buffer_struct {
    buffer_type_t type;
    double        area;
    double        load;
    delay_time_t  alpha;
    delay_time_t  beta;
    lib_gate_t    *gate;
};

typedef enum source_type_enum source_type_t;
enum source_type_enum {
    SOURCE_PI, SOURCE_INTERNAL, SOURCE_PWL
};

typedef struct delay_pi_struct delay_pi_t;
struct delay_pi_struct {
    delay_time_t arrival;
    delay_time_t drive;
};

typedef struct delay_internal_struct delay_internal_t;
struct delay_internal_struct {
    int          n_inputs;
    char         **delay_info;
    delay_time_t **arrival_times;
};

typedef struct delay_source_struct delay_source_t;
struct delay_source_struct {
    source_type_t    type;
    node_t           *node;
    int              polarity;
    delay_pi_t       pi;
    delay_internal_t internal;
    pwl_t            *pwl;
};

typedef enum gate_type_enum gate_type_t;
enum gate_type_enum {
    GATE_BUFFER, GATE_SOURCE
};

typedef struct delay_gate_struct delay_gate_t;
struct delay_gate_struct {
    gate_type_t    type;
    delay_buffer_t buffer;
    delay_source_t source;
    double         load_limit;
    DelayFn        intrinsic;
    DelayFn        load_dependent;
    DelayFn        forward_intrinsic;
    DelayFn        forward_load_dependent;
    PwlFn          get_delay_pwl;
};

#include "fanout_delay_static.h"

static n_gates_t    n_gates;
static array_t      *gate_array;
static bin_global_t globals;

/* file containing the routines needed for delay estimation */
/* by the fanout optimizer */

/* need code to initialize basic information */

void fanout_delay_init(bin_global_t *options) {
    int          i;
    library_t    *library;
    delay_gate_t *buffer;

    globals = *options;
    library = lib_get_library();
    assert(library != NIL(library_t));
    gate_array = array_alloc(delay_gate_t *, 0);
    add_buffers(library, "f=a;");
    n_gates.n_pos_buffers = array_n(gate_array);
    add_buffers(library, "f=!a;");
    n_gates.n_buffers     = n_gates.n_neg_buffers = array_n(gate_array);
    n_gates.n_pos_sources = n_gates.n_neg_sources = n_gates.n_gates =
            n_gates.n_buffers;
    for (i = 0; i < n_gates.n_buffers; i++) {
        buffer = array_fetch(delay_gate_t *, gate_array, i);
        delay_gate_initialize(buffer);
    }
}

void fanout_delay_end() {
    int          i;
    delay_gate_t *buffer;

    for (i = 0; i < n_gates.n_buffers; i++) {
        buffer = array_fetch(delay_gate_t *, gate_array, i);
        delay_gate_free(buffer);
    }
    array_free(gate_array);
}

void fanout_delay_add_source(node_t *source_node, int source_polarity) {
    source_type_t type =
                          (source_node->type == PRIMARY_INPUT) ? SOURCE_PI : SOURCE_INTERNAL;
    fanout_delay_add_general_source(source_node, source_polarity, type);
}

void fanout_delay_add_pwl_source(node_t *source_node, int source_polarity) {
    source_type_t type =
                          (source_node->type == PRIMARY_INPUT) ? SOURCE_PI : SOURCE_PWL;
    fanout_delay_add_general_source(source_node, source_polarity, type);
}

/* for 0 <= i < n_pos_sources, nodes[i] is a positive source */
/* for n_pos_sources <= i < n_neg_sources, nodes[i] is a negative source */
/* positive sources should always be added before negative sources */

static void fanout_delay_add_general_source(node_t *source_node,
                                            int source_polarity,
                                            source_type_t source_type) {
    delay_gate_t *source;
    if (source_polarity == POLAR_X &&
        n_gates.n_neg_sources > n_gates.n_pos_sources) fail(
            "fanout_delay_add_source: should add positive sources first");

    source = ALLOC(delay_gate_t, 1);
    source->type            = GATE_SOURCE;
    source->source.type     = source_type;
    source->source.node     = source_node;
    source->source.polarity = source_polarity;
    delay_gate_initialize(source);
    switch (source_polarity) {
        case POLAR_X:array_insert(delay_gate_t *, gate_array, n_gates.n_pos_sources, source);
            n_gates.n_pos_sources++;
            n_gates.n_neg_sources++;
            n_gates.n_gates++;
            break;
        case POLAR_Y:array_insert(delay_gate_t *, gate_array, n_gates.n_neg_sources, source);
            n_gates.n_neg_sources++;
            n_gates.n_gates++;
            break;
        default: fail("illegal polarity in fanout_delay_add_source");
            /* NOTREACHED */
    }
}

void fanout_delay_free_sources() {
    int          source_index;
    delay_gate_t *source;
    foreach_source(n_gates, source_index) {
        source = array_fetch(delay_gate_t *, gate_array, source_index);
        delay_gate_free(source);
    }
    n_gates.n_pos_sources = n_gates.n_neg_sources = n_gates.n_gates =
            n_gates.n_buffers;
}

double fanout_delay_get_buffer_load(int gate_index) {
    delay_gate_t *buffer = array_fetch(delay_gate_t *, gate_array, gate_index);
    assert(buffer->type == GATE_BUFFER);
    return buffer->buffer.load;
}

delay_time_t fanout_delay_backward_intrinsic(delay_time_t required,
                                             int gate_index) {
    delay_gate_t *gate = array_fetch(delay_gate_t *, gate_array, gate_index);
    return (*gate->intrinsic)(required, gate);
}

delay_time_t fanout_delay_backward_load_dependent(delay_time_t required,
                                                  int gate_index, double load) {
    delay_gate_t *gate = array_fetch(delay_gate_t *, gate_array, gate_index);
    if (globals.check_load_limit && load > gate->load_limit) {
        load *= globals.penalty_factor;
    }
    return (*gate->load_dependent)(required, gate, load);
}

delay_time_t fanout_delay_forward_intrinsic(delay_time_t arrival,
                                            int gate_index) {
    delay_gate_t *gate = array_fetch(delay_gate_t *, gate_array, gate_index);
    return (*gate->forward_intrinsic)(arrival, gate);
}

delay_time_t fanout_delay_forward_load_dependent(delay_time_t arrival,
                                                 int gate_index, double load) {
    delay_gate_t *gate = array_fetch(delay_gate_t *, gate_array, gate_index);
    if (globals.check_load_limit && load > gate->load_limit) {
        load *= globals.penalty_factor;
    }
    return (*gate->forward_load_dependent)(arrival, gate, load);
}

/* this arrival time is after intrinsic has been computed, but before load
 * dependent part */
/* thus there is no need to worry about inversion. If gate_index is a gate,
 * arrival should be */
/* equal to ZERO_DELAY. */
/* the load argument is necessary in case we need to handle a pwl source: we
 * need to choose */
/* which gate to use before returning the delay_pwl_t for that gate. */

delay_pwl_t fanout_delay_get_delay_pwl(int gate_index, double load,
                                       delay_time_t arrival) {
    delay_gate_t *gate = array_fetch(delay_gate_t *, gate_array, gate_index);
    return (*gate->get_delay_pwl)(gate, load, arrival);
}

/* used only by "two_level.c" */

int compute_best_number_of_inverters(int source_index, int buffer_index,
                                     double load, int max_n) {
    int from, to;

    if (source_index < n_gates.n_buffers) {
        delay_time_t a, b, c;
        delay_gate_t *source =
                             array_fetch(delay_gate_t *, gate_array, source_index);
        delay_gate_t *buffer =
                             array_fetch(delay_gate_t *, gate_array, buffer_index);

        assert(source->type == GATE_BUFFER && buffer->type == GATE_BUFFER);
        a.rise = source->buffer.beta.rise *
                 (buffer->buffer.load + map_compute_wire_load(1));
        a.fall = source->buffer.beta.fall *
                 (buffer->buffer.load + map_compute_wire_load(1));
        switch (buffer->buffer.type) {
            case BUFFER_NORMAL:b.rise = buffer->buffer.beta.rise * load;
                b.fall                = buffer->buffer.beta.fall * load;
                break;
            case BUFFER_INVERT:b.rise = buffer->buffer.beta.fall * load;
                b.fall                = buffer->buffer.beta.rise * load;
                break;
            default: fail("unexpected buffer type");
                /* NOTREACHED */
        }
        if (FP_EQUAL(a.rise, 0.0)) {
            c.rise = max_n;
        } else {
            c.rise = sqrt(b.rise / a.rise);
        }
        if (FP_EQUAL(a.fall, 0.0)) {
            c.fall = max_n;
        } else {
            c.fall = sqrt(b.fall / a.fall);
        }
        from     = (int) floor(GETMIN(c));
        to       = (int) floor(GETMAX(c) + 1);
        if (to > max_n)
            to   = max_n;
        if (from <= 0)
            from = 1;
        if (from > to)
            from = to;
    } else {
        from = 1;
        to   = max_n;
    }
    return linear_search_best_number_of_inverters(source_index, buffer_index,
                                                  load, max_n, from, to);
}

n_gates_t fanout_delay_get_n_gates() { return n_gates; }

double fanout_delay_get_area(int gate_index) {
    delay_gate_t *delay_gate = array_fetch(delay_gate_t *, gate_array, gate_index);
    return (delay_gate->type == GATE_BUFFER) ? delay_gate->buffer.area : 0.0;
}

node_t *fanout_delay_get_source_node(int source_index) {
    delay_gate_t *delay_gate = array_fetch(delay_gate_t *, gate_array, source_index);
    assert(delay_gate->type == GATE_SOURCE);
    return delay_gate->source.node;
}

lib_gate_t *fanout_delay_get_gate(int gate_index) {
    delay_gate_t *delay_gate = array_fetch(delay_gate_t *, gate_array, gate_index);
    assert(delay_gate->type == GATE_BUFFER);
    return delay_gate->buffer.gate;
}

int fanout_delay_get_source_polarity(int source_index) {
    delay_gate_t *delay_gate = array_fetch(delay_gate_t *, gate_array, source_index);
    assert(delay_gate->type == GATE_SOURCE);
    return delay_gate->source.polarity;
}

int fanout_delay_get_buffer_polarity(int buffer_index) {
    delay_gate_t *delay_gate = array_fetch(delay_gate_t *, gate_array, buffer_index);
    assert(delay_gate->type == GATE_BUFFER);
    return (delay_gate->buffer.type == BUFFER_NORMAL) ? POLAR_X : POLAR_Y;
}

int fanout_delay_get_buffer_index(lib_gate_t *buffer) {
    int          i;
    delay_gate_t *delay_gate;

    for (i = 0; i < array_n(gate_array); i++) {
        delay_gate = array_fetch(delay_gate_t *, gate_array, i);
        if (delay_gate->type != GATE_BUFFER)
            continue;
        if (delay_gate->buffer.gate == buffer)
            return i;
    }
    return -1;
}

int fanout_delay_get_source_index(node_t *source) {
    int          i;
    delay_gate_t *delay_gate;

    for (i = 0; i < array_n(gate_array); i++) {
        delay_gate = array_fetch(delay_gate_t *, gate_array, i);
        if (delay_gate->type != GATE_SOURCE)
            continue;
        if (delay_gate->source.node == source)
            return i;
    }
    return -1;
}

/* INTERNAL INTERFACE */

static void add_buffers(library_t *library, char *string) {
    lsGen       gen;
    lib_gate_t  *gate;
    network_t   *network;
    lib_class_t *class;

    network = read_eqn_string(string);
    if (network == NIL(network_t))
        return;
    class = lib_get_class(network, library);
    if (class == NIL(lib_class_t)) {
        network_free(network);
        return;
    }
    gen = lib_gen_gates(class);
    while (lsNext(gen, (char **) &gate, LS_NH) == LS_OK) {
        delay_gate_t *buffer;
        buffer = ALLOC(delay_gate_t, 1);
        array_insert_last(delay_gate_t *, gate_array, buffer);
        buffer->type        = GATE_BUFFER;
        buffer->buffer.gate = gate;
    }
    (void) lsFinish(gen);
    network_free(network);
}

static void delay_gate_initialize(delay_gate_t *delay_gate) {
    int        i;
    lib_gate_t *gate;
    node_t     *node;

    switch (delay_gate->type) {
        case GATE_BUFFER:gate = delay_gate->buffer.gate;
            assert(gate != NIL(lib_gate_t));
            delay_gate->load_limit   = delay_get_load_limit(gate->delay_info[0]);
            delay_gate->buffer.area  = gate->area;
            delay_gate->buffer.load  = delay_get_load(gate->delay_info[0]);
            delay_gate->buffer.alpha = delay_get_block(gate->delay_info[0]);
            delay_gate->buffer.beta  = delay_get_drive(gate->delay_info[0]);
            switch (delay_get_polarity(gate->delay_info[0])) {
                case PHASE_INVERTING:delay_gate->buffer.type = BUFFER_INVERT;
                    delay_gate->intrinsic                    = delay_intrinsic_neg_buffer;
                    delay_gate->load_dependent               = delay_load_dependent_buffer;
                    delay_gate->forward_intrinsic            = delay_forward_intrinsic_neg_buffer;
                    delay_gate->forward_load_dependent       = delay_forward_load_dependent_buffer;
                    delay_gate->get_delay_pwl                = delay_get_delay_pwl_buffer;
                    break;
                case PHASE_NONINVERTING:delay_gate->buffer.type = BUFFER_NORMAL;
                    delay_gate->intrinsic                       = delay_intrinsic_pos_buffer;
                    delay_gate->load_dependent                  = delay_load_dependent_buffer;
                    delay_gate->forward_intrinsic               = delay_forward_intrinsic_pos_buffer;
                    delay_gate->forward_load_dependent          = delay_forward_load_dependent_buffer;
                    delay_gate->get_delay_pwl                   = delay_get_delay_pwl_buffer;
                    break;
                default: fail("unexpected pin polarity");
                    /* NOTREACHED */
            }
            break;
        case GATE_SOURCE:node = delay_gate->source.node;
            assert(node != NIL(node_t));
            switch (delay_gate->source.type) {
                case SOURCE_PI:delay_gate->source.type = SOURCE_PI;
                    delay_gate->load_limit             = pipo_get_pi_load_limit(node);
                    delay_gate->source.pi.arrival      = pipo_get_pi_arrival(node);
                    delay_gate->source.pi.drive        = pipo_get_pi_drive(node);
                    delay_gate->intrinsic              = delay_intrinsic_fatal_error;
                    delay_gate->load_dependent         = delay_load_dependent_pi_source;
                    delay_gate->forward_intrinsic      = delay_intrinsic_fatal_error;
                    delay_gate->forward_load_dependent =
                            delay_forward_load_dependent_pi_source;
                    delay_gate->get_delay_pwl = delay_get_delay_pwl_pi;
                    break;
                case SOURCE_INTERNAL:delay_gate->source.type = SOURCE_INTERNAL;
                    assert(MAP(node)->gate != NIL(lib_gate_t));
                    assert(MAP(node)->ninputs > 0);
                    delay_gate->load_limit =
                            delay_get_load_limit(MAP(node)->gate->delay_info[0]);
                    delay_gate->source.internal.n_inputs   = MAP(node)->ninputs;
                    delay_gate->source.internal.delay_info = MAP(node)->gate->delay_info;
                    delay_gate->source.internal.arrival_times =
                            ALLOC(delay_time_t *, MAP(node)->ninputs);
                    for (i = 0; i < MAP(node)->ninputs; i++) {
                        delay_gate->source.internal.arrival_times[i] =
                                &(MAP(node)->arrival_info[i]);
                    }
                    delay_gate->intrinsic         = delay_intrinsic_fatal_error;
                    delay_gate->load_dependent    = delay_load_dependent_internal_source;
                    delay_gate->forward_intrinsic = delay_intrinsic_fatal_error;
                    delay_gate->forward_load_dependent =
                            delay_forward_load_dependent_internal_source;
                    delay_gate->get_delay_pwl = delay_get_delay_pwl_internal_source;
                    break;
                case SOURCE_PWL:node = delay_gate->source.node;
                    assert(node != NIL(node_t));
                    assert(node->type != PRIMARY_INPUT);
                    assert(BIN(node) != NIL(bin_t));
                    delay_gate->load_limit        = INFINITY;
                    delay_gate->source.type       = SOURCE_PWL;
                    delay_gate->source.pwl        = BIN(node)->pwl;
                    delay_gate->intrinsic         = delay_intrinsic_fatal_error;
                    delay_gate->load_dependent    = delay_load_dependent_pwl_source;
                    delay_gate->forward_intrinsic = delay_intrinsic_fatal_error;
                    delay_gate->forward_load_dependent =
                            delay_forward_load_dependent_pwl_source;
                    delay_gate->get_delay_pwl = delay_get_delay_pwl_pwl_source;
                    break;
                default: fail("unexpected delay_gate type");
                    break;
            }
            break;
        default: fail("unexpected delay_gate type");
            break;
    }
}

static void delay_gate_free(delay_gate_t *delay_gate) {
    switch (delay_gate->type) {
        case GATE_BUFFER:break;
        case GATE_SOURCE:
            if (delay_gate->source.type == SOURCE_INTERNAL) {
                FREE(delay_gate->source.internal.arrival_times);
            }
            break;
        default: fail("unexpected delay_gate type");
            /* NOTREACHED */
    }
    FREE(delay_gate);
}

static delay_time_t delay_load_dependent_buffer(delay_time_t required, delay_gate_t *buffer, double load) {
    delay_time_t result;
    result.rise = required.rise - buffer->buffer.beta.rise * load;
    result.fall = required.fall - buffer->buffer.beta.fall * load;
    return result;
}

static delay_time_t delay_forward_load_dependent_buffer(delay_time_t arrival, delay_gate_t *buffer, double load) {
    delay_time_t result;
    result.rise = arrival.rise + buffer->buffer.beta.rise * load;
    result.fall = arrival.fall + buffer->buffer.beta.fall * load;
    return result;
}

static delay_time_t delay_intrinsic_pos_buffer(delay_time_t required, delay_gate_t *buffer) {
    delay_time_t result;
    SETSUB(result, required, buffer->buffer.alpha);
    return result;
}

static delay_time_t delay_forward_intrinsic_pos_buffer(delay_time_t arrival, delay_gate_t *buffer) {
    delay_time_t result;
    SETADD(result, arrival, buffer->buffer.alpha);
    return result;
}

static delay_time_t delay_intrinsic_neg_buffer(delay_time_t required, delay_gate_t *buffer) {
    delay_time_t result;
    result.rise = required.fall - buffer->buffer.alpha.fall;
    result.fall = required.rise - buffer->buffer.alpha.rise;
    return result;
}

static delay_time_t delay_forward_intrinsic_neg_buffer(delay_time_t arrival, delay_gate_t *buffer) {
    delay_time_t result;
    result.rise = arrival.fall + buffer->buffer.alpha.rise;
    result.fall = arrival.rise + buffer->buffer.alpha.fall;
    return result;
}

static delay_time_t delay_load_dependent_pi_source(delay_time_t required, delay_gate_t *source, double load) {
    delay_time_t arrival;
    delay_time_t result;
    assert(source->type == GATE_SOURCE && source->source.type == SOURCE_PI);
    arrival.rise =
            source->source.pi.drive.rise * load + source->source.pi.arrival.rise;
    arrival.fall =
            source->source.pi.drive.fall * load + source->source.pi.arrival.fall;
    SETSUB(result, required, arrival);
    return result;
}

static delay_time_t delay_forward_load_dependent_pi_source(delay_time_t arrival, delay_gate_t *source, double load) {
    assert(source->type == GATE_SOURCE && source->source.type == SOURCE_PI);
    assert(IS_EQUAL(arrival, ZERO_DELAY));
    arrival.rise =
            source->source.pi.drive.rise * load + source->source.pi.arrival.rise;
    arrival.fall =
            source->source.pi.drive.fall * load + source->source.pi.arrival.fall;
    return arrival;
}

static delay_time_t delay_load_dependent_internal_source(delay_time_t required, delay_gate_t *source, double load) {
    delay_time_t     arrival;
    delay_time_t     result;
    delay_internal_t *internal = &(source->source.internal);

    assert(source->type == GATE_SOURCE && source->source.type == SOURCE_INTERNAL);
    arrival = delay_map_simulate(internal->n_inputs, internal->arrival_times,
                                 internal->delay_info, load);
    SETSUB(result, required, arrival);
    return result;
}

static delay_time_t delay_load_dependent_pwl_source(delay_time_t required, delay_gate_t *source, double load) {
    delay_time_t arrival;
    delay_time_t result;

    assert(source->type == GATE_SOURCE && source->source.type == SOURCE_PWL);
    arrival = bin_delay_compute_pwl_delay(source->source.pwl, load);
    SETSUB(result, required, arrival);
    return result;
}

static delay_time_t
delay_forward_load_dependent_internal_source(delay_time_t arrival, delay_gate_t *source, double load) {
    delay_internal_t *internal = &(source->source.internal);

    assert(source->type == GATE_SOURCE && source->source.type == SOURCE_INTERNAL);
    assert(IS_EQUAL(arrival, ZERO_DELAY));
    arrival = delay_map_simulate(internal->n_inputs, internal->arrival_times,
                                 internal->delay_info, load);
    return arrival;
}

static delay_time_t
delay_forward_load_dependent_pwl_source(delay_time_t arrival, delay_gate_t *source, double load) {
    assert(source->type == GATE_SOURCE && source->source.type == SOURCE_PWL);
    assert(IS_EQUAL(arrival, ZERO_DELAY));
    return bin_delay_compute_pwl_delay(source->source.pwl, load);
}

/* ARGSUSED */
static delay_time_t delay_intrinsic_fatal_error(delay_time_t required, delay_gate_t *buffer) {
    fail("attempt to compute the intrinsic of a source");
    /* NOTREACHED */
}

/* ARGSUSED */
static delay_pwl_t delay_get_delay_pwl_internal_source(delay_gate_t *gate, double load, delay_time_t arrival) {
    int         i;
    node_t      *source;
    int         ninputs;
    pin_info_t  *pin_info;
    delay_pwl_t result;

    assert(IS_EQUAL(arrival, ZERO_DELAY));
    assert(gate->type == GATE_SOURCE && gate->source.type == SOURCE_INTERNAL);
    source   = gate->source.node;
    ninputs  = MAP(source)->ninputs;
    pin_info = ALLOC(pin_info_t, ninputs);
    for (i   = 0; i < ninputs; i++) {
        pin_info[i].arrival = *gate->source.internal.arrival_times[i];
    }
    result = bin_delay_compute_gate_pwl(MAP(source)->gate, ninputs, pin_info);
    FREE(pin_info);
    return result;
}

/* ARGSUSED */
static delay_pwl_t delay_get_delay_pwl_pwl_source(delay_gate_t *gate, double load, delay_time_t arrival) {
    delay_pwl_t result;

    assert(IS_EQUAL(arrival, ZERO_DELAY));
    assert(gate->type == GATE_SOURCE && gate->source.type == SOURCE_PWL);
    result = bin_delay_select_active_pwl_delay(gate->source.pwl, load);
    result.rise = pwl_dup(result.rise);
    result.fall = pwl_dup(result.fall);
    pwl_set_data(result.rise, NIL(char));
    pwl_set_data(result.fall, NIL(char));
    return result;
}

/* ARGSUSED */
static delay_pwl_t delay_get_delay_pwl_pi(delay_gate_t *gate, double load, delay_time_t arrival) {
    assert(IS_EQUAL(arrival, ZERO_DELAY));
    assert(gate->type == GATE_SOURCE && gate->source.type == SOURCE_PI);
    return bin_delay_get_pi_delay_pwl(gate->source.node);
}

/* ARGSUSED */
static delay_pwl_t delay_get_delay_pwl_buffer(delay_gate_t *gate, double load, delay_time_t arrival) {
    delay_pwl_t result;
    pwl_point_t rise_point, fall_point;

    assert(gate->type == GATE_BUFFER);
    rise_point.x     = 0.0;
    rise_point.y     = arrival.rise;
    rise_point.slope = gate->buffer.beta.rise;
    rise_point.data  = NIL(char);
    fall_point.x     = 0.0;
    fall_point.y     = arrival.fall;
    fall_point.slope = gate->buffer.beta.fall;
    fall_point.data  = NIL(char);
    result.rise      = pwl_create_linear_max(1, &rise_point);
    result.fall      = pwl_create_linear_max(1, &fall_point);
    return result;
}

static int
linear_search_best_number_of_inverters(int source_index, int buffer_index, double load, int max_n, int from, int to) {
    int          i;
    delay_time_t required;
    delay_gate_t *buffer = array_fetch(delay_gate_t *, gate_array, buffer_index);
    int          k       = -1;

    assert(from <= to && from > 0 && to <= max_n);
    if (from == to)
        return from;
    required = MINUS_INFINITY;
    for (i   = from; i <= to; i++) {
        double       local_load;
        delay_time_t local_required;
        local_required = fanout_delay_backward_load_dependent(
                ZERO_DELAY, buffer_index, load / i);
        local_required =
                fanout_delay_backward_intrinsic(local_required, buffer_index);
        local_load     = buffer->buffer.load * i + map_compute_wire_load(i);
        local_required = fanout_delay_backward_load_dependent(
                local_required, source_index, local_load);
        if (GETMIN(required) < GETMIN(local_required)) {
            required = local_required;
            k        = i;
        }
    }
    assert(k >= 0);
    return k;
}
