/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/network/network.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:33 $
 *
 */
#ifndef NETWORK_H 
#define NETWORK_H


#define lsForeachItem(list, gen, data)				\
    for(gen = lsStart(list); 					\
	(lsNext(gen, (lsGeneric *) &data, LS_NH) == LS_OK) 	\
	    || ((void) lsFinish(gen), 0); )

#define LS_ASSERT(fct)		assert((fct) == LS_OK)


/*typedef struct network_struct network_t;*/
struct network_struct {
    char *net_name;
    st_table *name_table;
    st_table *short_name_table;
    lsList nodes;		/* list of all nodes */
    lsList pi;			/* list of just primary inputs */
    lsList po;			/* list of just primary outputs */
    network_t *original;	/* UNUSED: pointer to original network */
    double area;		/* HACK: support area keyword */
    int area_given;		/* HACK: support area keyword */
    lsList bdd_list;            /* list of bdd managers */
    network_t *dc_network;      /* external don't care network */
#ifdef SIS
    st_table *latch_table;      /*      sequential support      */
    lsList latch;               /*      sequential support      */
    graph_t *stg;               /*      sequential support      */
    char *clock;                /*      sequential support      */
#endif /* SIS */
    char *default_delay;	/* Stores default delay info */
#ifdef SIS
    astg_t *astg;		/* Asynch. Signal Transition Graph.	*/
#endif
};


#define foreach_node(network, gen, p)	\
    lsForeachItem((network)->nodes, gen, p)

#define foreach_primary_input(network, gen, p)	\
    lsForeachItem((network)->pi, gen, p)

#define foreach_primary_output(network, gen, p)	\
    lsForeachItem((network)->po, gen, p)

#ifdef SIS
#define foreach_latch(network, gen, l) \
    lsForeachItem((network)->latch, gen, l)
#endif /* SIS */

EXTERN network_t *network_alloc ARGS((void));
EXTERN void network_free ARGS((network_t *));
EXTERN network_t *network_dup ARGS((network_t *));
EXTERN network_t *network_create_from_node ARGS((node_t *));
EXTERN network_t *network_from_nodevec ARGS((array_t *));

EXTERN char *network_name ARGS((network_t *));
EXTERN void network_set_name ARGS((network_t *, char *));
EXTERN int network_num_pi ARGS((network_t *));
EXTERN int network_num_po ARGS((network_t *));
EXTERN int network_num_internal ARGS((network_t *));
EXTERN node_t *network_get_pi ARGS((network_t *, int));
EXTERN node_t *network_get_po ARGS((network_t *, int));

EXTERN void network_add_node ARGS((network_t *, node_t *));
EXTERN void network_add_primary_input ARGS((network_t *, node_t *));
EXTERN node_t *network_add_primary_output ARGS((network_t *, node_t *));
EXTERN void network_delete_node ARGS((network_t *, node_t *));
EXTERN void network_delete_node_gen ARGS((network_t *, lsGen));
EXTERN node_t *network_find_node ARGS((network_t *, char *));
EXTERN void network_change_node_type ARGS((network_t *, node_t *, enum node_type_enum));
EXTERN void network_change_node_name ARGS((network_t *, node_t *, char *));

EXTERN int network_check ARGS((network_t *));
EXTERN int network_is_acyclic ARGS((network_t *));
EXTERN int network_collapse ARGS((network_t *));
EXTERN int network_sweep ARGS((network_t *));
EXTERN int network_csweep ARGS((network_t *));
EXTERN int network_cleanup ARGS((network_t *));
EXTERN int network_ccleanup ARGS((network_t *));
EXTERN network_t *network_espresso ARGS((network_t *));

EXTERN array_t *network_dfs ARGS((network_t *));
#ifdef SIS
EXTERN array_t *network_special_dfs ARGS((network_t *));
#endif /* SIS */
EXTERN array_t *network_dfs_from_input ARGS((network_t *));
EXTERN array_t *network_tfi ARGS((node_t *, int));
EXTERN array_t *network_tfo ARGS((node_t *, int));

EXTERN int network_append ARGS((network_t *, network_t *));

EXTERN pPLA espresso_read_pla ARGS((FILE *));
EXTERN void discard_pla ARGS((pPLA ));
EXTERN pPLA network_to_pla ARGS((network_t *));
EXTERN network_t *pla_to_network ARGS((pPLA ));
EXTERN network_t *pla_to_network_single ARGS((pPLA ));
EXTERN network_t *pla_to_dcnetwork_single ARGS((pPLA ));

EXTERN void network_reset_long_name ARGS((network_t *));
EXTERN void network_reset_short_name ARGS((network_t *));
EXTERN void network_swap_names ARGS((network_t *, node_t *, node_t *));

#ifdef SIS

EXTERN void copy_latch_info ARGS((lsList, lsList, st_table *));
	/* network_create_latch, network_delete_latch declared in latch.h */
EXTERN node_t *network_latch_end ARGS((node_t *));
EXTERN void network_disconnect ARGS((node_t *,node_t *,node_t **,node_t **));
EXTERN void network_connect ARGS((node_t *,node_t *));
EXTERN st_table *snetwork_tfi_po ARGS((network_t *));

#define network_num_latch(n) \
  lsLength((n)->latch)

EXTERN graph_t *network_stg ARGS((network_t *));
EXTERN void network_set_stg ARGS((network_t *,graph_t *));
EXTERN int network_stg_check ARGS((network_t *));

EXTERN int network_is_real_po ARGS((network_t *,node_t *));
EXTERN int network_is_real_pi ARGS((network_t *,node_t *));
EXTERN int network_is_control ARGS((network_t *,node_t *));
EXTERN node_t *network_get_control ARGS((network_t *,node_t *));
EXTERN node_t *network_add_fake_primary_output ARGS((network_t *,node_t *));
EXTERN void network_replace_io_fake_names ARGS((network_t *));

#endif /* SIS */

	/* network_bdd_list, network_add_bdd declared in bdd.h	*/

EXTERN int network_verify ARGS((network_t *, network_t *, int));
EXTERN int net_verify_with_dc ARGS((network_t *, network_t *, int, int));
EXTERN network_t *network_dc_network ARGS((network_t *));
EXTERN st_table *attach_dcnetwork_to_network ARGS((network_t *));
EXTERN node_t *find_ex_dc ARGS((node_t *, st_table *));
EXTERN network_t *or_net_dcnet ARGS((network_t *));

#endif
