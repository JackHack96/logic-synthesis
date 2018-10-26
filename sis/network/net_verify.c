
#include "sis.h"

int
net_verify_with_dc(network1, network2, method, verbose)
network_t *network1;
network_t *network2;
int method;
int verbose;
{
    network_t *dc1, *dc2;
    network_t *net1, *net2;
    network_t *or_net_dcnet();
    network_t *network_dc_network();
    int   eql;

    /*
     * Make sure method has an appropriate value.
     */
    if ( (method != 0) && (method != 1) ) {
        return 1;
    }

    error_init();
    dc1= network_dc_network(network1);
    dc2= network_dc_network(network2);
    if ((dc1 == NIL (network_t)) && (dc2 == NIL (network_t))){
        eql = network_verify(network1, network2, method);
    }else if ((dc1 != NIL (network_t)) && (dc2 != NIL (network_t))){
        eql = network_verify(dc1, dc2, method);
        if (!eql){
            (void) fprintf(miserr, "%s", error_string());
            (void)fprintf(miserr,"External don't care networks are not equal.\n");
            return 0;
        }
        net1= or_net_dcnet(network1);
        net2= or_net_dcnet(network2);
        eql = network_verify(net1, net2, method);
        network_free(net1);
        network_free(net2);
    }else{
        (void) fprintf(miserr,"External don't care networks are not equal.\n");
        return 0;
    }

    /*
     * Process the eql flag.  Regardless of the value of eql, return 0.  Returning 1
     * when the networks are not equal would indicate that we don't want to continue.
     * However, we want to continue.
     */
    if (eql) {
        if (verbose) {
            (void) fprintf(misout, "Networks compared equal.\n");
        }
    } else {
        (void) fprintf(miserr, "%s", error_string());
        (void) fprintf(miserr, "Networks are not equivalent.\n");
    }
    return 0;
}

int
network_verify(network1_t, network2_t, method)
network_t *network1_t;
network_t *network2_t;
int method;    /* 0 - collapse, 1 - bdd, others - illegal */
{
    network_t *network1, *network2;
    node_t *po1, *po2, *node1, *node2, *new_node;
    lsGen gen;
    char errmsg[1024];
    int eql;

    network1 = network_dup(network1_t);
    network2 = network_dup(network2_t);

    /* add a buffer if PO is directly connected to a PI */
    foreach_primary_output(network1, gen, po1) {
        node1 = node_get_fanin(po1, 0);
        if (node1->type == PRIMARY_INPUT) {
            new_node = node_literal(node1, 1);
            (void) network_add_node(network1, new_node);
            assert(node_patch_fanin(po1, node1, new_node));
        }
    }
    foreach_primary_output(network2, gen, po2) {
        node2 = node_get_fanin(po2, 0);
        if (node2->type == PRIMARY_INPUT) {
            new_node = node_literal(node2, 1);
            (void) network_add_node(network2, new_node);
            assert(node_patch_fanin(po2, node2, new_node));
        }
    }

    /*
     * Make sure that there is a 1-1 name correspondance between the primary outputs of
     * the two networks.  If not, report the differences, and then return.
     */
    eql = 1;
    foreach_primary_output(network1, gen, po1) {
	po2 = network_find_node(network2, po1->name);
	if ((po2 == NIL(node_t)) || (po2->type != PRIMARY_OUTPUT)) {
	    (void) sprintf(errmsg, "output '%s' only in network '%s'\n",
	    po1->name, network_name(network1));
	    error_append(errmsg);
	    eql = 0;
	}
    }
    foreach_primary_output(network2, gen, po2) {
	po1 = network_find_node(network1, po2->name);
	if ((po1 == NIL(node_t)) || (po1->type != PRIMARY_OUTPUT)) {
	    (void) sprintf(errmsg, "output '%s' only in network '%s'\n",
	    po2->name, network_name(network2));
	    error_append(errmsg);
	    eql= 0;
	}
    }
    if (eql == 0) {
        return 0;
    }

    /*
     * Call the appropriate verification method.
     */
    if (method == 0) {
	eql = verify_by_collapse(network1, network2);
    } else if (method == 1) {
	eql = ntbdd_verify_network(network1, network2, DFS_ORDER, ALL_TOGETHER);
    } else {
        (void) fprintf(miserr, "Error: unknown verification method\n");
        return 0;
    }

    network_free(network1);
    network_free(network2);
    return eql;
}

int verify_by_collapse(network1, network2)
network_t *network1;
network_t *network2;
{
    node_t *po1, *po2, *node1, *node2;
    lsGen gen;
    char errmsg[1024];

    (void) network_collapse(network1);
    (void) network_collapse(network2);

    /*
     * Assume calling routine has verified a 1-1 name correspondance between the primary outputs
     * of the two networks.  Quit after finding the first difference.  TODO: add command line option
     * to generate all output differences.
     */
    foreach_primary_output(network1, gen, po1) {
	po2 = network_find_node(network2, po1->name);
        assert((po2 != NIL(node_t)) && (po2->type == PRIMARY_OUTPUT));
        /*  
         * Get the nodes driving the primary outputs.
         */
        node1 = node_get_fanin(po1, 0);
	node2 = node_get_fanin(po2, 0);

	if (! node_equal_by_name(node1, node2)) {
	    (void) sprintf(errmsg, "Networks differ on (at least) primary output %s\n", node_name(po1));
            error_append(errmsg);
	    return 0;
	}
    }

    /*
     * All output functions verified successfully.
     */
    return 1;
}

