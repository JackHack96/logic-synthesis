/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/factor/factor.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:23 $
 *
 */
#ifndef FACTOR_H
#define FACTOR_H

EXTERN void factor ARGS((node_t *));
EXTERN void factor_quick ARGS((node_t *));
EXTERN void factor_good ARGS((node_t *));

EXTERN void factor_free ARGS((node_t *));
EXTERN void factor_dup ARGS((node_t *, node_t *));
EXTERN void factor_alloc ARGS((node_t *));
EXTERN void factor_invalid ARGS((node_t *));

EXTERN void factor_print ARGS((FILE *, node_t *));
EXTERN int node_value ARGS((node_t *));
EXTERN int factor_num_literal ARGS((node_t *));
EXTERN int factor_num_used ARGS((node_t *, node_t *));

EXTERN void eliminate ARGS((network_t *, int, int));

EXTERN array_t *factor_to_nodes ARGS((node_t *));

#endif
