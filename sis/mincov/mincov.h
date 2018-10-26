
/* exported */
EXTERN sm_row *sm_minimum_cover ARGS((sm_matrix *, int *, int, int));

EXTERN sm_row *sm_mat_bin_minimum_cover ARGS((sm_matrix *, int *, int, int, int, int, int (*)()
));
EXTERN sm_row *sm_mat_minimum_cover ARGS((sm_matrix *, int *, int, int, int, int, int (*)()));
