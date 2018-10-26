

#ifndef PRL_INT_H
#define PRL_INT_H

#include "prl_seqbdd.h"

/* 
 * "typedef struct seq_info_t seq_info_t;" is already in "seqbdd.h"
 */

struct seq_info_t {
    /* 
     * OUTPUTS 
     */
    array_t *next_state_fns;		 /* of bdd_t *'s */
    array_t *external_output_fns;	 /* of bdd_t *'s */
    array_t *output_nodes;		 /* of node_t *'s: the corresponding PO in order: NS first */

    /* 
     * INPUTS 
     */
    array_t *present_state_vars;	 /* of bdd_t *'s; only present state PI's */
    array_t *external_input_vars;	 /* of bdd_t *'s; only the external PI's */
    array_t *input_vars;		 /* all input variables in BDD order */
    array_t *input_nodes;		 /* of node_t *'s: 1-1 correspondance with input_vars */

    bdd_t *init_state_fn;		 /* the initial state (may be a set) */

    st_table *dc_map;			 /* the map between PI's and PO's of network and
					  * network->dc.network (both ways) 
					  */

    /* 
     * The BDD's for the DC network: 
     * 1-1 correspondance with previous BDD's 
     */
    array_t *next_state_dc;		 /* of bdd_t *'s */
    array_t *external_output_dc;	 /* of bdd_t *'s */

    /* 
     * BDD Manager and related information
     */
    bdd_manager *manager;
    st_table *leaves;			 /* specify the BDD variable ordering: map PI's to indices */
    array_t *var_names;			 /* of char *'s: input_names[i] is name of BDD var of id=i */
					 /* size of this array is input_vars+transition_vars */

    /* 
     * Other info of global interest 
     */
    network_t *network;

    /* 
     * Method specific info 
     */
    struct {
	array_t *transition_vars;	 /* of bdd_t *'s; next_state_vars, 
					  * same order as next_state_fns 
					  */
	array_t *next_state_vars;	 /* of bdd_t *'s; next_state_vars, 
					  * same order as present_state_vars 
					  */
	struct {
	    array_t *transition_fns;	 /* array of bdd_t *'s; y_j==f_j(x,i) */
	} product;
    } transition;
};

/* 
 * Procedures used to manipulate 'seq_info_t' records
 */

EXTERN seq_info_t *Prl_SeqInitNetwork 	    	ARGS((network_t *,  prl_options_t *));
EXTERN bdd_t 	  *Prl_ExtractReachableStates 	ARGS((seq_info_t *, prl_options_t *));
EXTERN void 	   Prl_SeqInfoFree 	       	ARGS((seq_info_t *, prl_options_t *));

/* utilities for dc_networks */

EXTERN void	   Prl_RemoveDcNetwork 		ARGS((network_t *));
EXTERN void	   Prl_CleanupDcNetwork		ARGS((network_t *));
EXTERN bdd_t *	   Prl_GetSimpleDc		ARGS((seq_info_t *));


 /* utilities for networks */

EXTERN void    Prl_StoreAsSingleOutputDcNetwork ARGS((network_t *, network_t *));
EXTERN void    Prl_CleanupDcNetwork 		ARGS((network_t *));
EXTERN void    Prl_SetupCopyFields 		ARGS((network_t *, network_t *));
EXTERN node_t *Prl_CopySubnetwork  		ARGS((network_t *, node_t *));
EXTERN char   *Prl_DisambiguateName		ARGS((network_t *, char *, node_t *));
EXTERN void    Prl_CleanupDcNetwork		ARGS((network_t *));


 /* BDD utilities */

EXTERN void      Prl_FreeBddArray       ARGS((array_t *));
EXTERN array_t  *Prl_OrderSetHeuristic  ARGS((set_info_t *, int, int));
EXTERN void      Prl_GetOneEdge	        ARGS((bdd_t *, seq_info_t *, bdd_t **, bdd_t **));
EXTERN st_table *Prl_GetPiToVarTable    ARGS((seq_info_t *));


 /* Misc. utilities */

EXTERN void Prl_ReportElapsedTime ARGS((prl_options_t *, char *));


 /* utilities from other directories */

#define SIM_SLOT 			simulation
#define GET_VALUE(node)			((int) node->SIM_SLOT)
#define SET_VALUE(node, value)		(node->SIM_SLOT = (char *) value)
EXTERN array_t *simulate_network ARGS((network_t *, array_t *, array_t *));

#endif /* PRL_INT_H */
