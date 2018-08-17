
#ifdef SIS
#include "sis.h"


latch_t *
latch_alloc()
{
    latch_t *l;

    l = ALLOC(latch_t, 1);
    l->latch_input = NIL(node_t);
    l->latch_output = NIL(node_t);
    l->initial_value = 3;
    l->current_value = 3;
    l->synch_type = UNKNOWN;
    l->gate = NIL(lib_gate_t);
    l->control = NIL(node_t);
    l->undef1 = NIL(char);
    return l;
}

void
latch_free(l)
latch_t *l;
{
    if (l != NIL(latch_t)) {
    FREE(l);
    }
}


void
latch_set_control(latch, control)
latch_t *latch;
node_t *control;
{
    network_t *network;

    if (control != NIL(node_t)) {
        network = node_network(control);
    if (network == NIL(network_t)) {
        fail("latch_set_control:  node not part of network");
    }
        (void) st_insert(network->latch_table, (char *) control, NIL(char));
    }
    latch->control = control;
}


latch_t *
latch_from_node(n)
node_t *n;
{
    network_t *network;
    latch_t *latch;

    network = node_network(n);
    if (network == NIL(network_t)) {
    fail("latch_from_node: node not part of a network");
    }
    if (st_lookup(network->latch_table, (char *) n, (char **) &latch)) {
    return latch;
    } else {
       return NIL(latch_t);
    }
}


int
latch_equal(l1, l2)
latch_t *l1, *l2;
{
    if (latch_get_initial_value(l1) != latch_get_initial_value(l2)) return 0;
    if (latch_get_type(l1) != latch_get_type(l2)) return 0;
    if (latch_get_control(l1) != latch_get_control(l2)) return 0;
    if (latch_get_gate(l1) != latch_get_gate(l2)) return 0;
    return 1;
}
#endif /* SIS */
