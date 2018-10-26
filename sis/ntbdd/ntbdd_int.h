#ifndef NTBDD_INT_H /* { */
#define NTBDD_INT_H

/*
 * Revision 1.4  1993/06/04  19:42:26  sis
 * *** empty log message ***
 *
 * Revision 1.4  1993/06/04  15:40:19  shiple
 * Removed NTBDD and NTBDD_HOOK macros.
 *
 * Revision 1.3  1991/03/31  23:07:30  shiple
 * cleanup for Version 3.0
 *
 * Revision 1.2  91/03/28  16:05:34  shiple
 * removed include of bdd_int.h because wanted it to be explicit in each
 * of the .c files which use it; someday, none of the files should depend
 * on bdd_int.h, since this violates the BDD interface
 * 
 * Revision 1.1  91/03/27  14:13:20  shiple
 * Initial revision
 * 
 *
 */

/* 
 * Stuff hooked on the bdd manager so that everything is
 * freed at the appropriate point.
 */
typedef struct {
  network_t *last_network;
  st_table *network_table;
} ntbdd_t;

/* 
 * Macro to access bdd_t's at nodes.
 */
#define BDD(node) ((bdd_t *) (node)->bdd)
#define BDD_SET(node,value) ((node)->bdd = (char *) (value))

/* 
 * Stuff internal to the ntbdd package.
 */
extern void bdd_alloc_demon(/* node_t *node */);
extern void bdd_free_demon(/* node_t *node; */);

/* 
 * enum type to denote what type of BDD is being created: LOCAL
 * means just in terms of the node's immediate fanin; GLOBAL 
 * means in terms of the nodes in the leaves table.
 */
typedef enum {LOCAL, GLOBAL} ntbdd_type_t;
  
#endif /* } */

