
#ifndef BWD_INT_H
#define BWD_INT_H

#define BWD_SLOT undef1
#define BWD(node) ((bwd_node_t *)(node)->BWD_SLOT)
struct bwd_node_struct {
  double arrival;
  int flags;
};
typedef struct bwd_node_struct bwd_node_t;

/* structure to store minimum delays between transitions.
 * One structure for each pair * of signals (s1,s2).
 * delay.rise.rise is the delay between s1+ and s2+, and so on.
 */
struct min_del_struct {
  delay_time_t rise, fall;
};
typedef struct min_del_struct min_del_t;

/* Structure to store potential hazards: the symbol table is indexed by the
 * name of the signal currently checked for hazard. Each record in the dynamic
 * array contains a pair of signal names, ordered s1 -> s2 in the STG, that if
 * reversed can cause a hazard. Later we can add some information on the
 * vectors that cause a hazard.
 */

struct hazard_struct {
  char *s1, *s2;               /* names of involved signals */
  char dir1, dir2, on, off_eq; /* transition type and off_cb1 == off_cb2 */
  pset on_cb1, off_cb1, on_cb2, off_cb2; /* involved cubes */
};
typedef struct hazard_struct hazard_t;

/* structure to store equivalent states for state encoding (EXPERIMENTAL)
 */
struct equiv_struct {
  array_t *index_state;  /* maps index -> name */
  st_table *state_index; /* maps name -> index */
  pset_family classes;   /* each set is a class, with the above indices */
};
typedef struct equiv_struct equiv_t;

struct bwd_struct {
  st_table *hazard_list;
  st_table *slowed_amounts;
  long change_count;
  array_t *pi_names; /* order of PI's for cubes in hazard_list */
};
typedef struct bwd_struct bwd_t;

#define BWD_GET(stg) ((bwd_t *)astg_get_slot((stg), ASTG_BWD_SLOT))
#define BWD_SET(stg, value) (astg_set_slot((stg), ASTG_BWD_SLOT, (value)))

astg_scode dummy_mask;

int enc_summary;

/* defined in bwd_util.c */
extern graph_t *global_orig_stg;
extern st_table *global_orig_stg_names;
extern st_table *global_edge_trans;
extern st_table *global_state_marking;

/* bwd_com_astg.c */
extern void bwd_cmds();

extern int com_bwd_astg_stg_scr();

extern int com_bwd_astg_to_f();

extern int com_bwd_astg_to_stg();

extern int com_bwd_astg_slow_down();

extern int com_stg_scc();

extern int com_bwd_astg_latch();

extern int com_bwd_astg_state_minimize();

extern int com_bwd_astg_add_state();

extern int com_bwd_stg_to_astg();

extern int com_bwd_astg_encode();

extern int com_bwd_astg_write_sg();

/* bwd_stg_to_f.c */
extern void bwd_astg_to_f();

/* bwd_util.c */
extern graph_t *bwd_stg_scr();

extern void bwd_dc_to_self_loop();

extern int bwd_is_enabled();

extern astg_trans *bwd_edge_trans();

extern graph_t *bwd_astg_to_stg();

extern vertex_t *bwd_get_state_by_name();

extern array_t *graph_scc();

extern astg_scode bwd_new_enabled_signals();

extern char *bwd_marking_string();

extern graph_t *bwd_reduced_stg();

extern void bwd_pedge();

extern void bwd_pstate();

extern void bwd_astg_latch();

extern char *bwd_fake_pi_name();

extern char *bwd_po_name();

extern char *bwd_fake_po_name();

extern int bwd_marking_cmp();

extern int bwd_marking_ptr_cmp();

extern int bwd_marking_hash();

extern astg_scode bwd_astg_state_bit();

extern astg_graph *bwd_stg_to_astg();

extern void bwd_write_sg();

/* bwd_lp.c */
extern void bwd_lp_slow();

/* bwd_slow.c */
extern void bwd_slow_down();

extern char *bwd_make_in_name();

extern void bwd_external_delay_update();

/* bwd_hazard.c */
extern void bwd_find_hazards();

/* bwd_min_delay.c */
extern int bwd_min_delay_trace();

/* bwd_code.c */
extern equiv_t *bwd_read_equiv_states();

extern graph_t *bwd_state_minimize();

extern void bwd_add_state();

#endif /* BWD_INT_H */
