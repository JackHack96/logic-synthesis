
#ifndef BWD_INT_H
#define BWD_INT_H

#include "delay.h"
#include "array.h"
#include "st.h"
#include "astg_int.h"

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
graph_t *global_orig_stg;
st_table *global_orig_stg_names;
st_table *global_edge_trans;
st_table *global_state_marking;

/* bwd_com_astg.c */
void bwd_cmds();

int com_bwd_astg_stg_scr();

int com_bwd_astg_to_f();

int com_bwd_astg_to_stg();

int com_bwd_astg_slow_down();

int com_stg_scc();

int com_bwd_astg_latch();

int com_bwd_astg_state_minimize();

int com_bwd_astg_add_state();

int com_bwd_stg_to_astg();

int com_bwd_astg_encode();

int com_bwd_astg_write_sg();

/* bwd_stg_to_f.c */
void bwd_astg_to_f();

/* bwd_util.c */
graph_t *bwd_stg_scr();

void bwd_dc_to_self_loop();

int bwd_is_enabled();

astg_trans *bwd_edge_trans();

graph_t *bwd_astg_to_stg();

vertex_t *bwd_get_state_by_name();

array_t *graph_scc();

astg_scode bwd_new_enabled_signals();

char *bwd_marking_string();

graph_t *bwd_reduced_stg();

void bwd_pedge();

void bwd_pstate();

void bwd_astg_latch();

char *bwd_fake_pi_name();

char *bwd_po_name();

char *bwd_fake_po_name();

int bwd_marking_cmp();

int bwd_marking_ptr_cmp();

int bwd_marking_hash();

astg_scode bwd_astg_state_bit();

astg_graph *bwd_stg_to_astg();

void bwd_write_sg();

/* bwd_lp.c */
void bwd_lp_slow();

/* bwd_slow.c */
void bwd_slow_down();

char *bwd_make_in_name();

void bwd_external_delay_update();

/* bwd_hazard.c */
void bwd_find_hazards();

/* bwd_min_delay.c */
int bwd_min_delay_trace();

/* bwd_code.c */
equiv_t *bwd_read_equiv_states();

graph_t *bwd_state_minimize();

void bwd_add_state();

#endif /* BWD_INT_H */
