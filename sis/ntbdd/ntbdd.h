#ifndef NTBDD_H /* { */
#define NTBDD_H

/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/ntbdd/ntbdd.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:28 $
 * $Log: ntbdd.h,v $
 * Revision 1.1.1.1  2004/02/07 10:14:28  pchong
 * imported
 *
 * Revision 1.6  1993/05/28  23:18:55  sis
 * Aesthetic changes to prototypes.
 *
 * Revision 1.5  1993/02/25  01:05:23  sis
 * Shiple updates; 2/24/93.  See Shiple's RCS message.
 *
 * Revision 1.5  1993/02/25  01:05:23  sis
 * Shiple updates; 2/24/93.  See Shiple's RCS message.
 *
 * Revision 1.7  1993/01/14  01:34:01  shiple
 * Added EXTERNs for ntbdd_bdd_single_to_network and ntbdd_bdd_array_to_network.
 * Removed EXTERN for ntbdd_bdd_to_network.
 *
 * Revision 1.6  1991/06/27  09:14:12  shiple
 * Moved enum declaration before prototype declaration so new gcc
 * compiler does not complain.
 *
 * Revision 1.5  91/05/01  17:44:40  shiple
 *  convert to new declaration format using EXTERN and ARGS
 * 
 * Revision 1.4  91/04/26  17:57:55  shiple
 * removed declaration of ordering routines
 * 
 * Revision 1.3  91/03/31  23:06:50  shiple
 * eliminated obsolete functions; renamed remaining functions; for Version 3.0
 * 
 * Revision 1.2  91/03/28  16:04:37  shiple
 * removed extern declaration for bdd_order_nodes, which was placed there
 * inadvertently
 * 
 * Revision 1.1  91/03/27  14:12:37  shiple
 * Initial revision
 * 
 *
 */

/* 
 * Verification methods
 */
typedef enum {ONE_AT_A_TIME, ALL_TOGETHER} ntbdd_verify_method_t;

/* 
 * Utilities
 */
EXTERN bdd_t *ntbdd_at_node ARGS((node_t *));
EXTERN network_t *ntbdd_bdd_single_to_network ARGS((bdd_t *, char *, array_t *));
EXTERN network_t *ntbdd_bdd_array_to_network ARGS((array_t *, array_t *, array_t *));
EXTERN void ntbdd_end_manager ARGS((bdd_manager *));
EXTERN void ntbdd_free_at_node ARGS((node_t *));
EXTERN bdd_t *ntbdd_node_to_bdd ARGS((node_t *, bdd_manager *, st_table *));
EXTERN bdd_t *ntbdd_node_to_local_bdd ARGS((node_t *, bdd_manager *, st_table *));
EXTERN void ntbdd_set_at_node ARGS((node_t *, bdd_t *));
EXTERN bdd_manager *ntbdd_start_manager ARGS((int));
EXTERN int ntbdd_verify_network ARGS((network_t *, network_t *, order_method_t, ntbdd_verify_method_t));

#endif /* } */
