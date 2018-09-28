
#ifdef SIS

#include "clock.h"
#include "sis.h"

static sis_clock_t *clock_get_transitive_recur();

/*
 * Given a node in the network, find the relevant clock signal that it
 * is generated from. "offset" return the delay of from the clock to the
 * actual control node and "phasep" returns the phase relationship of the
 * node and the respective clock input.
 */
sis_clock_t *clock_get_transitive_clock(network_t *network, node_t *node,
                                        delay_model_t model,
                                        delay_time_t *offset,
                                        input_phase_t *phasep) {
    delay_time_t  t;
    input_phase_t phase;
    sis_clock_t   *clock;

    t.rise = t.fall = 0.0;
    phase = POS_UNATE;

    clock = clock_get_transitive_recur(network, node, model, &t, &phase);

    *phasep = phase;
    *offset = t;
    return clock;
}

static sis_clock_t *
clock_get_transitive_recur(network_t *network, node_t *node, delay_model_t model, delay_time_t *offset,
                           input_phase_t *phasep) {
    int           i;
    node_t        *fanin;
    sis_clock_t   *clock;
    input_phase_t phase;
    delay_time_t  delay;

    if (node->type == PRIMARY_OUTPUT) {
        node = node_get_fanin(node, 0);
    }

    if (node->type == PRIMARY_INPUT) {
        return clock_get_by_name(network, node_long_name(node));
    }

    foreach_fanin(node, i, fanin) {
        if ((clock = clock_get_transitive_recur(network, fanin, model, offset,
                                                phasep)) != NIL(sis_clock_t)) {
            delay = delay_node_pin(node, i, model);
            offset->rise += delay.rise;
            offset->fall += delay.fall;
            /*
             * Get the correct phase
             */
            phase = node_input_phase(node, fanin);
            if (phase == BINATE) {
                *phasep = BINATE;
            } else if (phase == NEG_UNATE) {
                if (*phasep == POS_UNATE) {
                    *phasep = NEG_UNATE;
                } else if (*phasep == NEG_UNATE) {
                    *phasep = POS_UNATE;
                }
            }

            return clock;
        }
    }
    return NIL(sis_clock_t);
}

#endif /* SIS */
