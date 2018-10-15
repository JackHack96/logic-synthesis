/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/network/netmake.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:33 $
 *
 */
#include "sis.h"

network_t *
network_from_nodevec(nodevec)
array_t *nodevec;
{
    int i, j;
    st_table *table;
    node_t *node, *node_copy, *fanin, **new_fanin;
    char *dummy;
    network_t *new_network;
    lsGen gen;

    new_network = network_alloc();

    table = st_init_table(st_ptrcmp, st_ptrhash);
    for(i = 0; i < array_n(nodevec); i++) {
        node = array_fetch(node_t *, nodevec, i);
	(void) st_insert(table, (char *) node, (char *) node_dup(node));
    }

    for(i = 0; i < array_n(nodevec); ++i) {
        node = array_fetch(node_t *, nodevec, i);

        new_fanin = ALLOC(node_t *, node_num_fanin(node));
        foreach_fanin(node, j, fanin) {
	    if (st_lookup(table, (char *) fanin, &dummy)) {
	        new_fanin[j] = (node_t *) dummy;
	    } else {
		new_fanin[j] = node_alloc();
		new_fanin[j]->name = util_strsav(fanin->name);
		network_add_primary_input(new_network, new_fanin[j]);
		(void) st_insert(table, (char *) fanin, (char *) new_fanin[j]);
	    }
	}

	(void) st_lookup(table, (char *) node, &dummy);
	node_copy = (node_t *) dummy;
	if (node->nin > 0) {
	    node_replace_internal(node_copy, new_fanin, 
					    node->nin, sf_save(node->F));
	} else {
	    FREE(new_fanin);
	}
	network_add_node(new_network, node_copy);
    }

    foreach_node(new_network, gen, node_copy) {
        if (node_function(node_copy) != NODE_PO && 
					    node_num_fanout(node_copy) == 0) {
	   (void) network_add_primary_output(new_network, node_copy);
	}
    }

    st_free_table(table);
    return new_network;
}



network_t *
network_create_from_node(user_node)
node_t *user_node;
{
    array_t *nodevec;
    network_t *network;

    nodevec = array_alloc(node_t *, 10);
    array_insert_last(node_t *, nodevec, user_node);
    network = network_from_nodevec(nodevec);
    array_free(nodevec);
    return network;
}
