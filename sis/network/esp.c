
#include "sis.h"

#ifdef SIS
static void save_latch_info();
#endif /* SIS */

network_t *
network_espresso(network)
network_t *network;
{
    network_t *new_net;
    pPLA PLA;

    PLA = network_to_pla(network);
    if (PLA == 0) return 0;

    if (PLA->R != 0) sf_free(PLA->R);
    if (PLA->D != 0) {
      PLA->R = complement(cube2list(PLA->F,PLA->D));
    } else {
      PLA->D = new_cover(0);
      PLA->R = complement(cube1list(PLA->F));
    }

    PLA->F = espresso(PLA->F, PLA->D, PLA->R);
    new_net = pla_to_network(PLA);
    network_set_name(new_net, network_name(network));
    delay_network_dup(new_net, network);
    new_net->dc_network = network_dup(network->dc_network);
#ifdef SIS
    save_latch_info(new_net, network->latch, new_net->latch, new_net->latch_table);
    new_net->stg = stg_dup(network->stg);
    new_net->astg = astg_dup(network->astg);
    network_clock_dup(network, new_net);
#endif /* SIS */

    discard_pla(PLA);
    return new_net;
}


#ifdef SIS
static void
save_latch_info(net, list, newlist, newtable)
network_t *net;
lsList list, newlist;
st_table *newtable;
{
    lsGen gen;
    latch_t *l1, *l2;
    node_t *in, *out;

    lsForeachItem(list, gen, l1) {
	l2 = latch_alloc();
	in = network_find_node(net, node_long_name(latch_get_input(l1)));
	out = network_find_node(net, node_long_name(latch_get_output(l1)));
        latch_set_input(l2, in);
        latch_set_output(l2, out);
        latch_set_initial_value(l2, latch_get_initial_value(l1));

        latch_set_current_value(l2, latch_get_current_value(l1));
        latch_set_type(l2, latch_get_type(l1));
        latch_set_gate(l2, latch_get_gate(l1));
        if (latch_get_control(l1) != NIL(node_t)) {
            latch_set_control(l2, network_find_node(net,
                              node_long_name(latch_get_control(l1))));
        }
        (void) st_insert(newtable, (char *) in, (char *) l2);
        (void) st_insert(newtable, (char *) out, (char *) l2);
        LS_ASSERT(lsNewEnd(newlist, (lsGeneric) l2, LS_NH));
    }
    return;
}
#endif /* SIS */
