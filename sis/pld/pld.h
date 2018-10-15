/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/pld/pld.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:56 $
 *
 */
extern void merge_node();            /* xln_merge.c */
extern void partition_network();     /* xln_map_par.c */
extern int part_network();           /* xln_part.c */
extern int split_network();         /*  xln_part_dec.c */
extern void karp_decomp_network();   /* xln_k_decomp.c */

extern network_t *act_map_network(); /* act_map.c */
extern void free_cost_table();              /* act_map.c */

