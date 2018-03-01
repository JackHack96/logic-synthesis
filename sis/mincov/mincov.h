/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/mincov/mincov.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:31 $
 *
 */
/* exported */
EXTERN sm_row *sm_minimum_cover ARGS((sm_matrix *, int *, int, int));

EXTERN sm_row *sm_mat_bin_minimum_cover ARGS((sm_matrix *, int *, int, int, int, int, int (*)()
));
EXTERN sm_row *sm_mat_minimum_cover ARGS((sm_matrix *, int *, int, int, int, int, int (*)()));
