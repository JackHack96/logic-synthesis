/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/extract/extract.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:22 $
 *
 */
extern void ex_kernel_gen();
extern void ex_subkernel_gen();
extern node_t *ex_find_divisor();
extern node_t *ex_find_divisor_quick();

/* Some definitions for making the structures in fast_extract exportable */

#define SHORT short int
#define UNSIGNED unsigned char

/*  Define the data structure a double cube diviosr */
typedef struct double_cube_divisor_struct ddivisor_t;
struct double_cube_divisor_struct {
    sm_row *cube1;          /* the first cube of the double-cube divisor */
    sm_row *cube2;          /* the second cube of the double-cube divisor */
    lsHandle handle;        /* lsHandle to double-cube divisor set */
    lsHandle sthandle;      /* lsHandle to corresponoding searching table */
    lsList div_index;       /* indicate where the divisor comes from */
    SHORT weight;           /* the weight of double-cube divisor */
    UNSIGNED dtype;         /* the type of double-cube divisor */
    UNSIGNED weight_status; /* indicate the weight can be changed or not */
    UNSIGNED status;        /* indicate whether the divisor is changed or not */
    UNSIGNED level;         /* indicates the level of 2-cube kernels */
};


#define fx_get_div_handle(p)    ((p)->handle)
