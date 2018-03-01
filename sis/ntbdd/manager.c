#include "sis.h"
#include "ntbdd_int.h"

/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/ntbdd/manager.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:27 $
 * $Log: manager.c,v $
 * Revision 1.1.1.1  2004/02/07 10:14:27  pchong
 * imported
 *
 * Revision 1.6  1993/06/04  19:42:26  sis
 * *** empty log message ***
 *
 * Revision 1.7  1993/06/04  15:39:16  shiple
 * Removed unnecessary void casts of functions calls.  Made changes to adhere to exported BDD interface.
 *
 * Revision 1.6  1993/06/02  21:31:48  shiple
 * Corrected incorrect cast of (char *) to (char **).
 *
 * Revision 1.5  1992/04/01  22:42:42  shiple
 * Removed temporary code for garbage collector experiments.
 *
 * Revision 1.4  1992/02/12  23:58:49  shiple
 * In ntbdd_end_manager, added call to free NTBDD_HOOK. Added
 * temporary code to dump BDD survival rates from ntbdd_end_manager.
 *
 * Revision 1.3  91/03/31  23:04:40  shiple
 * small revisions for Version 3.0
 * 
 * Revision 1.2  91/03/28  16:03:56  shiple
 * explicitly include bdd_int.h
 * 
 * Revision 1.1  91/03/20  14:38:40  shiple
 * Initial revision
 * 
 */

static void bdd_network_free();

/*
 * ntbdd_start_manager - Initialize a network bdd manager.
 * Set up a data structure to contain references
 * to networks when necessary.
 * Return the bdd_manager.
 */
bdd_manager *ntbdd_start_manager(num_vars)
int num_vars;
{
    bdd_manager *manager;
    ntbdd_t *ntbdd_data;
    bdd_external_hooks *hooks;
    
    manager = bdd_start(num_vars);
    ntbdd_data = ALLOC(ntbdd_t, 1);
    ntbdd_data->last_network = NIL(network_t);
    ntbdd_data->network_table = st_init_table(st_ptrcmp, st_ptrhash);
    hooks = bdd_get_external_hooks(manager);
    hooks->network = (char *) ntbdd_data;
    return manager;
}

/*
 * ntbdd_end_manager - terminate a network bdd manager
 * Frees all the BDD's associated to any node in any network
 * with that bdd manager.
 */
void ntbdd_end_manager(manager)
bdd_manager *manager;
{
  st_generator *gen;
  network_t *network;
  st_table *table; 
  bdd_external_hooks *hooks;
  ntbdd_t *ntbdd_data;

  hooks = bdd_get_external_hooks(manager);
  ntbdd_data = (ntbdd_t *) hooks->network;
  table = ntbdd_data->network_table;

  st_foreach_item(table, gen, (char **) &network, NIL(char *)) {
    bdd_network_free(manager, network);
  }
  st_free_table(table);
  FREE(ntbdd_data);
  bdd_end(manager);
}

/* 
 * Free the BDD's of all the nodes in the network
 * with the specified BDD manager.
 */
static void bdd_network_free(manager, network)
bdd_manager *manager;
network_t *network;
{
  lsGen gen;
  node_t *node;
  bdd_t *fn;

  foreach_node(network, gen, node) {
    fn = ntbdd_at_node(node);
    if (fn != NIL(bdd_t)) {
      if (bdd_get_manager(fn) == manager) {
        bdd_free(fn);
        /* do not use ntbdd_set_at_node here: problems with manager's hooks->network.network_table */
        BDD_SET(node, NIL(bdd_t));
      }
    }
  }
}
