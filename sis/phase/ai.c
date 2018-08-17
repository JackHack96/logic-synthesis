
#include "sis.h"


void
add_inv_network(network)
        network_t *network;
{
    int     i;
    array_t *nodevec;

    nodevec = network_dfs(network);
    for (i  = 0; i < array_n(nodevec); i++) {
        (void) add_inv_node(network, array_fetch(node_t * , nodevec, i));
    }
    array_free(nodevec);
}


int
add_inv_node(network, f)
        network_t *network;
        node_t *f;
{
    node_t *inv, *flit, *fpos, *g, *q, *r, *gnew, *temp;
    bool   changed;
    lsGen  gen;

    /* try to find an inverter which is already in our fanout */
    inv = NIL(node_t);
    foreach_fanout(f, gen, g)
    {
        if (node_function(g) == NODE_INV) {
            inv = g;
            (void) lsFinish(gen);
            break;
        }
    }

    changed = FALSE;
    flit    = node_literal(f, 1);
    foreach_fanout(f, gen, g)
    {
        if (g->type != PRIMARY_OUTPUT) {
            /* divide by the positive literal of f -- check for nonzero q */
            q = node_div(g, flit, &r);
            if (node_function(q) != NODE_0) {

                /* create an inverter if we don't have one so far */
                if (inv == NIL(node_t)) {
                    inv = node_literal(f, 0);
                    network_add_node(network, inv);
                }

                /* munge the fanout so it depends on the inverter */
                fpos = node_literal(inv, 0);
                temp = node_and(q, fpos);
                gnew = node_or(temp, r);
                node_replace(g, gnew);
                node_free(temp);
                node_free(fpos);
                changed = TRUE;
            }
            node_free(q);
            node_free(r);
        }
    }
    node_free(flit);
    return changed;
}
