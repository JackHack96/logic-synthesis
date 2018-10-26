#ifndef PLD_H
#define PLD_H

#include "network.h"

void merge_node();          /* xln_merge.c */
void partition_network();   /* xln_map_par.c */
int part_network();         /* xln_part.c */
void split_network();        /*  xln_part_dec.c */
void karp_decomp_network(); /* xln_k_decomp.c */

network_t *act_map_network(); /* act_map.c */
void free_cost_table();       /* act_map.c */

int init_pld();
int end_pld();
#endif