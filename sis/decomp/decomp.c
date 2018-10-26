
#include "sis.h"
#include "decomp.h"
#include "decomp_int.h"

void
decomp_quick_network(network) 
network_t *network;
{
    lsGen gen;
    node_t *f;

    foreach_node(network, gen, f) {
	decomp_quick_node(network, f);
    }
}


void
decomp_quick_node(network, f)
network_t *network;
node_t *f;
{
    array_t *fa;
    int i;

    if (f->type == INTERNAL) {
	fa = decomp_quick(f);
	if (array_n(fa) > 1) {
	    node_replace(f, array_fetch(node_t *, fa, 0));
	    for(i = 1; i < array_n(fa); i++) {
		network_add_node(network, array_fetch(node_t *, fa, i));
	    }
	} else {
	    node_free(array_fetch(node_t *, fa, 0));
	}
	array_free(fa);
    }
}

void
decomp_good_network(network)
network_t *network;
{
    lsGen gen;
    node_t *f;

    foreach_node(network, gen, f) {
	decomp_good_node(network, f);
    }
}


void
decomp_good_node(network, f)
network_t *network;
node_t *f;
{
    int i;
    array_t *fa;

    if (f->type == INTERNAL) {
	fa = decomp_good(f);
	if (array_n(fa) > 1) {
	    node_replace(f, array_fetch(node_t *, fa, 0));
	    for(i = 1; i < array_n(fa); i++) {
		network_add_node(network, array_fetch(node_t *, fa, i));
	    }
	} else {
	    node_free(array_fetch(node_t *, fa, 0));
	}
	array_free(fa);
    }
}

void
decomp_disj_network(network)
network_t *network;
{
    lsGen gen;
    node_t *f;

    foreach_node(network, gen, f) {
	decomp_disj_node(network, f);
    }
}


void
decomp_disj_node(network, f)
network_t *network;
node_t *f;
{
    int i;
    array_t *fa;

    if (f->type == INTERNAL) {
	fa = decomp_disj(f);
	if (array_n(fa) > 1) {
	    node_replace(f, array_fetch(node_t *, fa, 0));
	    for(i = 1; i < array_n(fa); i++) {
		network_add_node(network, array_fetch(node_t *, fa, i));
	    }
	} else {
	    node_free(array_fetch(node_t *, fa, 0));
	}
	array_free(fa);
    }
}

array_t *
decomp_quick(f)
node_t *f;
{
    node_scc(f);
    return decomp_recur(node_dup(f), decomp_quick_kernel);
}


array_t *
decomp_good(f)
node_t *f;
{
    node_scc(f);
    return decomp_recur(node_dup(f), decomp_good_kernel);
}
