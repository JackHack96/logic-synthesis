/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/power/power.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:15:05 $
 *
 */
/*--------------------------------------------------------------------------
|   Constants and structures needed to use the power package routines.
|
| Copyright (c) 1993 - Jose' Monteiro
| Massachusetts Institute of Technology
+-------------------------------------------------------------------------*/

#ifndef POWER_H
#define POWER_H

/* Some constant definitions */
#define POWER_ZERO_D      0
#define POWER_UNIT_D      1
#define POWER_GENERAL_D   2
#define COMBINATIONAL    10
#define SEQUENTIAL       11
#define PIPELINE         12
#define DYNAMIC          13
#define APPROXIMATION_PS 20
#define EXACT_PS         21
#define STATELINE_PS     22
#define UNIFORM_PS       23
#define FACTORED_FORM    30
#define SUM_OF_PRODUCTS  31

#define CAPACITANCE  0.01 /* In pico farads per fanout */

typedef struct{
    int cap_factor;        /* The load of the gate */
    double switching_prob; /* Expected number of transistions in one cycle */
} power_info_t;


/* Table with switching, capacitance and delay info */
extern st_table *power_info_table;

extern int power_free_info();          /* power_util.c */
extern int power_print_info();         /* power_util.c */

extern int power_estimate();           /* power_main.c */
extern int power_main_driver();        /* power_main.c */

extern char *power_dummy;          /* For use in the following macros */

#define SWITCH_PROB(node) (st_lookup(power_info_table, (char *) (node), &power_dummy) ? (((power_info_t *) power_dummy)->switching_prob) : (fprintf(siserr, "Error in SWITCH_PROB, no power info for node %s\n", node->name), abort(), 0 /* make compiler happy */))
#define CAP_FACTOR(node) (st_lookup(power_info_table, (char *) (node), &power_dummy) ? (((power_info_t *) power_dummy)->cap_factor) : (fprintf(siserr, "Error in SWITCH_PROB, no power info for node %s\n", node->name), abort(), 0 /* make compiler happy */))

#endif

