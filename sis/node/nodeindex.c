/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/node/nodeindex.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:47 $
 *
 */
#include "sis.h"


/*
 *  nodeindex functions -- association table of nodes <-> small ints
 */


nodeindex_t *
nodeindex_alloc()
{
    nodeindex_t *table;

    table = ALLOC(nodeindex_t, 1);
    table->node_to_int = st_init_table(st_ptrcmp, st_ptrhash);
    table->int_to_node = array_alloc(node_t *, 0);
    return table;
}


void
nodeindex_free(table)
nodeindex_t *table;
{
    st_free_table(table->node_to_int);
    array_free(table->int_to_node);
    FREE(table);
}


nodeindex_insert(table, node) 
nodeindex_t *table;
node_t *node;
{
    char *value;
    int index;

    if (st_lookup(table->node_to_int, (char *) node, &value)) {
	index = (int) value;
    } else {
	index = st_count(table->node_to_int);
	(void) st_insert(table->node_to_int, (char *) node, (char *) index);
	array_insert(node_t *, table->int_to_node, index, node);
    }
    return index;
}


nodeindex_indexof(table, node) 
nodeindex_t *table;
node_t *node;
{
    char *value;

    if (st_lookup(table->node_to_int, (char *) node, &value)) {
	return (int) value;
    } else {
	return -1;
    }
}


node_t *
nodeindex_nodeof(table, index)
nodeindex_t *table;
int index;
{
    return array_fetch(node_t *, table->int_to_node, index);
}
