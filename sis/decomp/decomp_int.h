/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/decomp/decomp_int.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:19 $
 *
 */
extern node_t 	*decomp_quick_kernel();
extern node_t 	*decomp_good_kernel();
extern array_t  *decomp_recur();
extern array_t  *my_array_append();
extern array_t  *decomp_tech_recur();
extern node_t   *dec_node_cube();
extern int      dec_block_partition();

extern sm_matrix *dec_node_to_sm();
extern node_t    *dec_sm_to_node();
