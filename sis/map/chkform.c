
/* file @(#)chkform.c	1.2 */
/* last modified on 5/1/91 at 15:50:10 */
#include "sis.h"
#include "../include/map_int.h"


int
map_check_form(network, nand_flag)
        network_t *network;
        int nand_flag;
{
    lsGen           gen;
    node_t          *p;
    int             i, *count;
    node_function_t func;
    char            errmsg[1024];

    foreach_node(network, gen, p)
    {
        if (p->type == INTERNAL) {
            if (node_num_fanin(p) > 2) goto bad_network;
            func = node_function(p);

            if (func == NODE_0 || func == NODE_1) continue;

            if (nand_flag) {
                if (func != NODE_INV && func != NODE_OR) {
                    goto bad_network;
                }
            } else {
                if (func != NODE_INV && func != NODE_AND) {
                    goto bad_network;
                }
            }

            count  = node_literal_count(p);
            for (i = node_num_fanin(p) - 1; i >= 0; i--) {
                if (count[2 * i] != 0) goto bad_network;    /* pos phase */
                if (count[2 * i + 1] != 1) goto bad_network;/* neg phase */
            }
            FREE(count);
        }
    }
    return 1;


    bad_network:
    (void) sprintf(errmsg,
                   "\"%s\": '%s' is not a 1 or 2-input %s gate\n",
                   network_name(network), node_name(p), nand_flag ? "nand" : "nor");
    error_append(errmsg);
    return 0;
}
