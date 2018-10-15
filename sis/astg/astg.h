/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/astg/astg.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:58 $
 *
 */
/* astg.h -- exported programming interface to ASTG package. */

#ifndef ASTG_H
#define ASTG_H

typedef char astg_t;

void	 init_astg ARGS(());
void	 end_astg ARGS(());
astg_t	*astg_dup ARGS((astg_t *));
void	 astg_free ARGS((astg_t *));

#endif /* ASTG_H */
