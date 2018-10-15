#ifndef MINCOV_H
#define MINCOV_H

#include "sparse.h"

/* exported */
sm_row *sm_minimum_cover(sm_matrix *, int *, int, int);

sm_row *sm_mat_bin_minimum_cover(sm_matrix *, int *, int, int, int, int,
                                        int (*)());

sm_row *sm_mat_minimum_cover(sm_matrix *, int *, int, int, int, int,
                                    int (*)());

#endif