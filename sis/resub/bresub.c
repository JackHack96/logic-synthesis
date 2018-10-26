
#include "resub.h"
#include "sis.h"

void resub_bool_node(node_t *f) {
    (void) fprintf(miserr, "Warning!: Boolean resub has not been ");
    (void) fprintf(miserr, "implemented, algebraic resub is used.\n");
    (void) resub_alge_node(f, 1);
}

void resub_bool_network(network_t *network) {
    (void) fprintf(miserr, "Warning!: Boolean resub has not been ");
    (void) fprintf(miserr, "implemented, algebraic resub is used.\n");
    resub_alge_network(network, 1);
}
