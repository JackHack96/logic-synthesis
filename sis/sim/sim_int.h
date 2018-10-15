/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/sim/sim_int.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:46 $
 *
 */
#define SIM_SLOT 			simulation
#define GET_VALUE(node)			((int) node->SIM_SLOT)
#define SET_VALUE(node, value)		(node->SIM_SLOT = (char *) value)

extern void simulate_node();
extern array_t *simulate_network();
extern array_t *simulate_stg();

extern int sim_verify_codegen();
