#ifndef MIN_INT_H
#define MIN_INT_H

#include "ros.h"
#include "espresso.h"

/* Functions defined in  minimize.c */

/* minimize.c */ pcover nocomp();

/* minimize.c */ pcover snocomp();

/* minimize.c */ pset_family ncp_unate_compl();

/* minimize.c */ pset_family ncp_unate_complement();

/* minimize.c */ void ncp_compl_special_cases();

/* minimize.c */ pcover nc_super_gasp();

/* minimize.c */ pcover nc_all_primes();

/* minimize.c */ pcover nc_make_sparse();

/* minimize.c */ pcover nc_expand_gasp();

/* minimize.c */ void nc_expand1_gasp();

/* minimize.c */ pcover nc_last_gasp();

/* minimize.c */ pcover nc_reduce_gasp();

/* minimize.c */ pcover nc_reduce();

/* minimize.c */ pcover nc_random_order();

/* minimize.c */ pcover nc_permute();

/* minimize.c */ pcover nc_mini_sort();

/* minimize.c */ pcube nc_reduce_cube();

/* minimize.c */ pcube nc_sccc();

/* minimize.c */ bool nc_sccc_special_cases();

/* minimize.c */ pcover ncp_expand();

/* minimize.c */ void ncp_expand1();

/* minimize.c */ pcover nc_first_expand();

/* minimize.c */ void rem_unnec_r_cubes();

/* dcsimp.c */ pcover dcsimp();

#endif