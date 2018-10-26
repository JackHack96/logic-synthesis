
#define SIM_SLOT 			simulation
#define GET_VALUE(node)			((int) node->SIM_SLOT)
#define SET_VALUE(node, value)		(node->SIM_SLOT = (char *) value)

extern void simulate_node();
extern array_t *simulate_network();
extern array_t *simulate_stg();

extern int sim_verify_codegen();
