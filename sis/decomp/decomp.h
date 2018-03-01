/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/decomp/decomp.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:19 $
 *
 */
#ifndef DECOMP_H
#define DECOMP_H

EXTERN void decomp_quick_network ARGS((network_t *));
EXTERN void decomp_quick_node ARGS((network_t *, node_t *));
EXTERN array_t *decomp_quick ARGS((node_t *));

EXTERN void decomp_good_network ARGS((network_t *));
EXTERN void decomp_good_node ARGS((network_t *, node_t *));
EXTERN array_t *decomp_good ARGS((node_t *));

EXTERN void decomp_disj_network ARGS((network_t *));
EXTERN void decomp_disj_node ARGS((network_t *, node_t *));
EXTERN array_t *decomp_disj ARGS((node_t *));

EXTERN void decomp_tech_network ARGS((network_t *, int, int));

#endif
