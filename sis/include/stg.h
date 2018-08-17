
#define START        0
#define CURRENT        1
#define NUM_INPUTS    2
#define    NUM_OUTPUTS    3
#define NUM_PRODUCTS    4
#define NUM_STATES    5
#define STG_INPUT_NAMES    6
#define STG_OUTPUT_NAMES 7
#define CLOCK_DATA      8
#define EDGE_INDEX      9
#define NUM_G_SLOTS    10    /* update this when a new slot is added */

#define STATE_STRING    0
#define ENCODING_STRING 1
#define NUM_S_SLOTS    2    /* update this when a new slot is added */

#define INPUT_STRING    0
#define OUTPUT_STRING    1
#define NUM_T_SLOTS    2    /* update this when a new slot is added */

extern graph_t *stg_alloc(void);

extern void stg_free(graph_t *);

extern graph_t *stg_dup(graph_t *);

extern int stg_check(graph_t *);

extern void stg_dump_graph(graph_t *, network_t *);

extern void stg_reset(graph_t *);

extern void stg_sim(graph_t *, char *);

extern int stg_save_names(network_t *, graph_t *, int);

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

extern void stg_set_start(graph_t *, vertex_t *);

#define stg_get_current(stg) ((vertex_t *) g_get_g_slot_static((stg),CURRENT))

extern void stg_set_current(graph_t *, vertex_t *);

extern void stg_set_names(graph_t *, array_t *, int);

extern array_t *stg_get_names(graph_t *, int);

extern vertex_t *stg_create_state(graph_t *, char *, char *);

extern edge_t *stg_create_transition(vertex_t *, vertex_t *, char *, char *);

extern vertex_t *stg_get_state_by_name(graph_t *, char *);

extern vertex_t *stg_get_state_by_encoding(graph_t *, char *);

#define stg_get_state_name(v) \
  ((char *) g_get_v_slot_static((v),STATE_STRING))

extern void stg_set_state_name(vertex_t *, char *);

#define stg_get_state_encoding(v) \
  ((char *) g_get_v_slot_static((v), ENCODING_STRING))

extern void stg_set_state_encoding(vertex_t *, char *);


#define stg_foreach_state(stg, lgen, s)                \
    for (lgen = lsStart(g_get_vertices(stg));        \
        lsNext(lgen, (lsGeneric *) &s, LS_NH) == LS_OK    \
           || ((void) lsFinish(lgen), 0); )

#define stg_foreach_transition(stg, lgen, e)            \
    for (lgen = lsStart(g_get_edges(stg));            \
        lsNext(lgen, (lsGeneric *) &e, LS_NH) == LS_OK    \
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

extern graph_t *stg_extract(network_t *, int);

extern network_t *stg_to_network(graph_t *, int);
