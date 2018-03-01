/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/stg/stg.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:52 $
 *
 */
#define START		0
#define CURRENT		1
#define NUM_INPUTS	2
#define	NUM_OUTPUTS	3
#define NUM_PRODUCTS	4
#define NUM_STATES	5
#define STG_INPUT_NAMES	6
#define STG_OUTPUT_NAMES 7
#define CLOCK_DATA      8
#define EDGE_INDEX      9
#define NUM_G_SLOTS	10	/* update this when a new slot is added */

#define STATE_STRING	0
#define ENCODING_STRING 1
#define NUM_S_SLOTS	2	/* update this when a new slot is added */

#define INPUT_STRING	0
#define OUTPUT_STRING	1
#define NUM_T_SLOTS	2	/* update this when a new slot is added */

EXTERN graph_t *stg_alloc ARGS((void));
EXTERN void stg_free ARGS((graph_t *));
EXTERN graph_t *stg_dup ARGS((graph_t *));
EXTERN int stg_check ARGS((graph_t *));
EXTERN void stg_dump_graph ARGS((graph_t *, network_t *));
EXTERN void stg_reset ARGS((graph_t *));
EXTERN void stg_sim ARGS((graph_t *, char *));

EXTERN int stg_save_names ARGS((network_t *, graph_t *, int));

#define stg_get_num_inputs(stg)  (int) g_get_g_slot_static((stg), NUM_INPUTS)
#define stg_set_num_inputs(stg, i)  (void) g_set_g_slot_static((stg), NUM_INPUTS, (gGeneric) i)

#define stg_get_num_outputs(stg) (int) g_get_g_slot_static((stg), NUM_OUTPUTS)
#define stg_set_num_outputs(stg, i)  (void) g_set_g_slot_static((stg), NUM_OUTPUTS, (gGeneric) i)

#define stg_get_edge_index(stg) (int) g_get_g_slot_static((stg), EDGE_INDEX)
#define stg_set_edge_index(stg, i)  (void) g_set_g_slot_static((stg), EDGE_INDEX, (gGeneric) i)

#define stg_get_num_products(stg) (int) g_get_g_slot_static((stg), NUM_PRODUCTS)
#define stg_set_num_products(stg, i)  (void) g_set_g_slot_static((stg), NUM_PRODUCTS, (gGeneric) i)

#define stg_get_num_states(stg)  (int) g_get_g_slot_static((stg),NUM_STATES)
#define stg_set_num_states(stg, i)  (void) g_set_g_slot_static((stg),NUM_STATES, (gGeneric) i)

#define stg_get_start(stg) ((vertex_t *) g_get_g_slot_static((stg),START))

EXTERN void stg_set_start ARGS((graph_t *, vertex_t *));

#define stg_get_current(stg) ((vertex_t *) g_get_g_slot_static((stg),CURRENT))

EXTERN void stg_set_current ARGS((graph_t *, vertex_t *));
EXTERN void stg_set_names ARGS((graph_t *, array_t *, int));
EXTERN array_t *stg_get_names ARGS((graph_t *, int));

EXTERN vertex_t *stg_create_state ARGS((graph_t *, char *, char *));
EXTERN edge_t *stg_create_transition ARGS((vertex_t *, vertex_t *, char *, char *));
EXTERN vertex_t *stg_get_state_by_name ARGS((graph_t *, char *));
EXTERN vertex_t *stg_get_state_by_encoding ARGS((graph_t *, char *));

#define stg_get_state_name(v) \
  ((char *) g_get_v_slot_static((v),STATE_STRING))

EXTERN void stg_set_state_name ARGS((vertex_t *, char *));

#define stg_get_state_encoding(v) \
  ((char *) g_get_v_slot_static((v), ENCODING_STRING))

EXTERN void stg_set_state_encoding ARGS((vertex_t *, char *));



#define stg_foreach_state(stg, lgen, s)				\
	for (lgen = lsStart(g_get_vertices(stg));		\
		lsNext(lgen, (lsGeneric *) &s, LS_NH) == LS_OK	\
		   || ((void) lsFinish(lgen), 0); )

#define stg_foreach_transition(stg, lgen, e)			\
	for (lgen = lsStart(g_get_edges(stg));			\
		lsNext(lgen, (lsGeneric *) &e, LS_NH) == LS_OK	\
		   || ((void) lsFinish(lgen), 0); )

#define foreach_state_inedge(v, lgen, e)                        \
        for (lgen = lsStart(g_get_in_edges(v));                 \
	     lsNext(lgen, (lsGeneric *) &e, LS_NH) == LS_OK     \
	        || ((void) lsFinish(lgen), 0); )

#define foreach_state_outedge(v, lgen, e)                       \
        for (lgen = lsStart(g_get_out_edges(v));                \
	     lsNext(lgen, (lsGeneric *) &e, LS_NH) == LS_OK     \
	        || ((void) lsFinish(lgen), 0); )

#define stg_edge_input_string(e) \
  ((char *) g_get_e_slot_static((e), INPUT_STRING))
#define stg_edge_output_string(e) \
  ((char *) g_get_e_slot_static((e), OUTPUT_STRING))

#define stg_edge_from_state(e)  (g_e_source(e))
#define stg_edge_to_state(e)    (g_e_dest(e))

EXTERN graph_t *stg_extract ARGS((network_t *, int));
EXTERN network_t *stg_to_network ARGS((graph_t *, int));
