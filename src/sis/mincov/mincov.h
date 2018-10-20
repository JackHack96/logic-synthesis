/*
 * Revision Control Information
 *
 * $Source: /projects/mvsis/Repository/mvsis-1.3/src/sis/mincov/mincov.h,v $
 * $Author: wjiang $
 * $Revision: 1.1.1.1 $
 * $Date: 2003/02/24 22:24:09 $
 *
 */
/* exported */
EXTERN sm_row *sm_minimum_cover ARGS((sm_matrix *, int *, int, int));

EXTERN sm_row *sm_mat_bin_minimum_cover ARGS((sm_matrix *, int *, int, int, int, int, int (*)()
));
EXTERN sm_row *sm_mat_minimum_cover ARGS((sm_matrix *, int *, int, int, int, int, int (*)()));
