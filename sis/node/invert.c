
#include "sis.h"
#include "../include/node_int.h"


int
node_invert(node)
        node_t *node;
{
    pset_family   temp;
    lsList        po_list;
    lsGen         gen;
    node_t        *node1, *fanout;
    int           pin;
    register pset last, p;
    register int  pin2;

    if (node->type == PRIMARY_INPUT || node->type == PRIMARY_OUTPUT) {
        return 0;
    }
    if (node->F == 0) {
        fail("node_invert: node does not have a function");
    }

    if (node->R != 0) {
        sf_free(node->R);
        node->R = 0;
    }
    node_complement(node);
    temp = node->F;
    node->F              = node->R;
    node->R              = temp;
    node->is_scc_minimal = 1;        /* node_complement() assures this */
    node_minimum_base(node);

    po_list = 0;    /* record fanouts which are PO's */

    if (node->network != 0) {
        foreach_fanout_pin(node, gen, fanout, pin)
        {
            if (fanout->type == INTERNAL) {
                foreach_set(fanout->F, last, p)
                {
                    pin2 = 2 * pin;
                    switch (GETINPUT(p, pin)) {
                        case ZERO: set_remove(p, pin2);
                            set_insert(p, pin2 + 1);
                            break;
                        case ONE: set_remove(p, pin2 + 1);
                            set_insert(p, pin2);
                            break;
                        case TWO: break;
                        default: fail("node_invert: bad cube literal");
                    }
                }
                node_invalid(fanout);

            } else if (fanout->type == PRIMARY_OUTPUT) {
                /* save this node so we can add an inverter later */
                if (po_list == 0) po_list = lsCreate();
                LS_ASSERT(lsNewEnd(po_list, (char *) fanout, LS_NH));

            } else {
                fail("node_invert: bad node type");
            }
        }

        /* Add a single invert (if necessary) to feed the primary outputs */
        if (po_list != 0) {
            node1 = node_literal(node, 0);
            network_add_node(node->network, node1);
            lsForeachItem(po_list, gen, fanout)
            {
                assert(node_patch_fanin(fanout, node, node1));
            }
            LS_ASSERT(lsDestroy(po_list, (void (*)()) 0));
        }
    }

    factor_invalid(node);
    return 1;
}
