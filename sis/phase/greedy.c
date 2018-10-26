
#include "sis.h"
#include "phase.h"
#include "phase_int.h"

static void	greedy_down();
static bool	KL_up();

void
phase_random_greedy(network, num)
network_t *network;
int num;		/* number of random assignment */
{
    net_phase_t *net_phase, *best_net_phase;
    int i;
    double cost;
    bool trace;

    if (phase_trace) {
	trace = TRUE;
	phase_trace_unset();
    } else {
	trace = FALSE;
    }

    net_phase = phase_setup(network);

    best_net_phase = phase_dup(net_phase);
    for (i = 0; i < num; i++) {
	phase_random_assign(net_phase);
	cost = network_cost(net_phase);
	greedy_down(net_phase);

	if (trace) {
	    (void) fprintf(misout, "%3d random assignment: ", i+1);
	    (void) fprintf(misout, "%6f -> ", cost);
	    (void) fprintf(misout, "%6f\n", network_cost(net_phase));
	}

	if (network_cost(net_phase) < network_cost(best_net_phase)) {
	    phase_free(best_net_phase);
	    best_net_phase = phase_dup(net_phase);
	}
    }

    phase_record(network, best_net_phase);
    phase_free(net_phase);
    phase_free(best_net_phase);
}

void
phase_quick(network)
network_t *network;
{
    net_phase_t *net_phase;

    net_phase = phase_setup(network);

    greedy_down(net_phase);

    phase_record(network, net_phase);
    phase_free(net_phase);
}

void
phase_good(network)
network_t *network;
{
    net_phase_t *net_phase;
    bool not_done;

    net_phase = phase_setup(network);

    not_done = TRUE;
    while (not_done) {
	greedy_down(net_phase);
	not_done = KL_up(net_phase);
    }

    phase_record(network, net_phase);
    phase_free(net_phase);
}

static void
greedy_down(net_phase)
net_phase_t *net_phase;
{
    node_phase_t *node_phase;

    for (;;) {
	node_phase = phase_get_best(net_phase);
	if (node_phase != NIL(node_phase_t) && phase_value(node_phase) > 0) {
	    phase_invert(net_phase, node_phase);
	} else {
	    break;
	}
    }
}

/*
 *  Allow increase in number of inverters, but only flip a node
 *  once.  Return TRUE as soon as a better network is found.
 *  If all the nodes are inverted and no better network is found,
 *  return FALSE.
 */
static bool
KL_up(net_phase)
net_phase_t *net_phase;
{
    net_phase_t *net_phase_best;
    node_phase_t *node_phase;

    net_phase_best = phase_dup(net_phase);

    for (;;) {
	node_phase = phase_get_best(net_phase);

	if (node_phase == NIL(node_phase_t)) {	
	    /* no luck, all nodes are inverted */
	    phase_replace(net_phase, net_phase_best);
	    phase_unmark_all(net_phase);
	    return FALSE;
	}

	phase_invert(net_phase, node_phase);
	phase_mark(node_phase);

	if (network_cost(net_phase) < network_cost(net_phase_best)) {
	    /* good!, a better network is found */
	    phase_free(net_phase_best);
	    phase_unmark_all(net_phase);
	    return TRUE;
	}
    }
}
