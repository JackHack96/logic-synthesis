/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/simplify/simplify.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:49 $
 *
 */
#ifndef SIMPLIFY_H
#define SIMPLIFY_H

typedef enum sim_method_enum sim_method_t;
enum sim_method_enum {
    SIM_METHOD_SIMPCOMP, SIM_METHOD_ESPRESSO, SIM_METHOD_EXACT,
    SIM_METHOD_EXACT_LITS, SIM_METHOD_DCSIMP, SIM_METHOD_NOCOMP,
    SIM_METHOD_SNOCOMP, SIM_METHOD_UNKNOWN
};

typedef enum sim_accept_enum sim_accept_t;
enum sim_accept_enum {
    SIM_ACCEPT_FCT_LITS, SIM_ACCEPT_CUBES, SIM_ACCEPT_SOP_LITS,
    SIM_ACCEPT_ALWAYS, SIM_ACCEPT_UNKNOWN
};

typedef enum sim_dctype_enum sim_dctype_t; 
enum sim_dctype_enum {
    SIM_DCTYPE_NONE, SIM_DCTYPE_FANIN, SIM_DCTYPE_FANOUT,
    SIM_DCTYPE_INOUT, SIM_DCTYPE_ALL, SIM_DCTYPE_SUB_FANIN,
    SIM_DCTYPE_LEVEL, SIM_DCTYPE_UNKNOWN,
    SIM_DCTYPE_X
};

typedef enum sim_filter_enum sim_filter_t;
enum sim_filter_enum {
    SIM_FILTER_NONE, SIM_FILTER_EXACT, SIM_FILTER_DISJ_SUPPORT, 
    SIM_FILTER_SIZE, SIM_FILTER_FDIST, SIM_FILTER_SDIST, SIM_FILTER_LEVEL,
    SIM_FILTER_ALL 
};

extern void     simplify_node();
extern void     simplify_all();
extern node_t   *simp_dc_filter();

extern void comp_perm_fn();
extern node_t *cspf_dc();
extern node_t *simp_obsdc_filter();
extern node_t *simp_obssatdc_filter();
#endif
