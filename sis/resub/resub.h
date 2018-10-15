/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/resub/resub.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:48 $
 *
 */
#ifndef RESUB_H
#define RESUB_H

EXTERN int resub_alge_node ARGS((node_t *, int));
EXTERN void resub_alge_network ARGS((network_t *, int));
EXTERN void resub_bool_node ARGS((node_t *));
EXTERN void resub_bool_network ARGS((network_t *));

#endif
