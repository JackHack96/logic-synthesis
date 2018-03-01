/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/resub/bresub.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:48 $
 *
 */
#include "sis.h"
#include "resub.h"
#include "resub_int.h"

void
resub_bool_node(f)
node_t *f;
{
    (void) fprintf(miserr, "Warning!: Boolean resub has not been ");
    (void) fprintf(miserr, "implemented, algebraic resub is used.\n");
    (void) resub_alge_node(f, 1);
}

void
resub_bool_network(network)
network_t *network;
{
    (void) fprintf(miserr, "Warning!: Boolean resub has not been ");
    (void) fprintf(miserr, "implemented, algebraic resub is used.\n");
    resub_alge_network(network, 1);
}
