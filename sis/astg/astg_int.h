/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/astg/astg_int.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:58 $
 *
 */
/* -------------------------------------------------------------------------- *\
   astg_int.h -- asynchronous signal transition graph (ASTG) interface.

   This is the "interface" which the rest of the astg package uses to access
   the astg "core".  This interface is not exported to the rest of SIS since
   it is not of general interest.
\* ---------------------------------------------------------------------------*/

#ifndef ASTG_INT_H
#define ASTG_INT_H

#include "sis.h"
#include "astg.h"

extern int astg_debug_flag;

typedef unsigned astg_bool;
#define ASTG_FALSE	0
#define ASTG_TRUE	1

#define ASTG_NO_CALLBACK	NULL
#define ASTG_NO_DATA		NULL
#define ASTG_ALL		ASTG_FALSE
#define ASTG_SUBSET		ASTG_TRUE
#define ASTG_ALL_CYCLES		NIL(astg_vertex)


typedef enum astg_retval {
    ASTG_OK,			/* No error, success.			*/
    ASTG_NOT_SAFE,		/* STG is not safe.			*/
    ASTG_NOT_LIVE,		/* STG is not live.			*/
    ASTG_NOT_IRRED,		/* Has redundant constraints.		*/
    ASTG_NOT_USC,		/* Not unique state coding.		*/
    ASTG_NOT_CSC,		/* Not complete state coding.		*/
    ASTG_NOT_CSA,		/* Not consistent state assignment.	*/
    ASTG_NO_MARKING,		/* No initial marking found.		*/
    ASTG_BAD_SIGNAME,		/* Invalid signal name.			*/
    ASTG_GOT_MARKING,		/* For finding partial traces.		*/
    ASTG_NOT_COVER,		/* Components don't cover STG.		*/
    ASTG_BAD_OPTION,		/* No such option for command.		*/
    ASTG_ERROR			/* Generic error.			*/
} astg_retval;

typedef enum astg_signal_enum {
    ASTG_ANY_SIG,		/* Only for calls to astg_find_signal.	*/
    ASTG_INPUT_SIG,		/* Rest of these are real signal types.	*/
    ASTG_OUTPUT_SIG,
    ASTG_INTERNAL_SIG,
    ASTG_DUMMY_SIG
} astg_signal_enum;

typedef enum astg_vertex_enum {
    ASTG_VERTEX,
    ASTG_PLACE,
    ASTG_TRANS
} astg_vertex_enum;

typedef enum astg_trans_enum {
    ASTG_ALL_X,
    ASTG_POS_X,
    ASTG_NEG_X,
    ASTG_TOGGLE_X,
    ASTG_HATCH_X,
    ASTG_DUMMY_X
} astg_trans_enum;

typedef enum astg_daemon_enum {
    ASTG_DAEMON_ALLOC,
    ASTG_DAEMON_DUP,
    ASTG_DAEMON_INVALID,
    ASTG_DAEMON_FREE
} astg_daemon_enum;

typedef enum astg_slot_enum {
    ASTG_BWD_SLOT,
    ASTG_UNDEF1_SLOT,
    ASTG_N_SLOTS
} astg_slot_enum;

typedef enum astg_io_enum {
    ASTG_REAL_PI,
    ASTG_REAL_PO,
    ASTG_FAKE_PI,
    ASTG_FAKE_PO
} astg_io_enum;

		/* Basic types: only declare pointers to these. */

typedef struct astg_graph	astg_graph;
typedef struct astg_signal	astg_signal;
typedef struct astg_vertex	astg_vertex;
typedef struct astg_vertex	astg_place;
typedef struct astg_vertex	astg_trans;
typedef struct astg_edge	astg_edge;
typedef struct astg_state	astg_state;
typedef struct astg_marking	astg_marking;
typedef unsigned long		astg_scode;

typedef struct astg_generator {
    astg_bool	 first_time;
    astg_edge	*ge;
    astg_vertex	*gv;
    astg_trans	*gt;
    void	*state_gen;
    int		 marking_n;
    int		 gen_index;
    astg_signal	*sig_p;
} astg_generator;

typedef void (*astg_daemon) ARGS((astg_graph*, astg_graph*));

#ifndef ARGS
#define ARGS(args) args
#endif

astg_vertex_enum astg_v_type ARGS((astg_vertex *));
char		*astg_v_name ARGS((astg_vertex *));
void		 astg_set_useful ARGS((astg_vertex *, astg_bool));
astg_bool	 astg_get_useful ARGS((astg_vertex *));
void		 astg_set_hilite ARGS((astg_vertex *, astg_bool));
astg_bool	 astg_get_hilite ARGS((astg_vertex *));
astg_bool	 astg_get_selected ARGS((astg_vertex *));
void		 astg_set_v_locn ARGS((astg_vertex *, float*, float*));
void		 astg_get_v_locn ARGS((astg_vertex *, float*, float*));

astg_place	*astg_new_place ARGS((astg_graph *, char *, void *));
void		 astg_delete_place ARGS((astg_place *));
char		*astg_place_name ARGS((astg_place *));
astg_place	*astg_find_place ARGS((astg_graph *, char *, astg_bool));
astg_bool	 astg_boring_place ARGS((astg_place *));
astg_bool	 astg_get_marked ARGS((astg_marking *, astg_place *));
void		 astg_set_marked ARGS((astg_marking *, astg_place *, astg_bool));

astg_trans	*astg_new_trans ARGS((astg_graph *,char *, void *));
void		 astg_delete_trans ARGS((astg_trans *));
char		*astg_trans_name ARGS((astg_trans *));
char		*astg_trans_alpha_name ARGS((astg_trans *));
astg_trans	*astg_find_trans ARGS((astg_graph *, char *, astg_trans_enum, int, astg_bool));
astg_trans	*astg_noninput_trigger ARGS((astg_trans *));
astg_bool	 astg_has_constraint ARGS((astg_trans *, astg_trans *));
astg_trans_enum	 astg_trans_type ARGS((astg_trans *));
astg_signal	*astg_trans_sig ARGS((astg_trans *));

astg_edge	*astg_new_edge ARGS((astg_vertex *, astg_vertex *));
void		 astg_delete_edge ARGS((astg_edge *));
char		*astg_edge_name ARGS((astg_edge *));
astg_vertex	*astg_tail ARGS((astg_edge *));
astg_vertex	*astg_head ARGS((astg_edge *));
astg_edge	*astg_find_edge ARGS((astg_vertex *, astg_vertex *, astg_bool));

astg_graph	*astg_new ARGS((char *));
astg_graph	*astg_read ARGS((FILE *, char *));
astg_graph	*astg_contract ARGS((astg_graph *, astg_signal *, astg_bool));
void		 astg_delete ARGS((astg_graph *));
long		 astg_change_count ARGS((astg_graph *));
char		*astg_name ARGS((astg_graph *));
astg_graph	*astg_duplicate ARGS((astg_graph *, char *));
void		 astg_check ARGS((astg_graph *));
void		 astg_add_constraint ARGS((astg_trans *, astg_trans *, astg_bool));
void		 astg_register_daemon ARGS((astg_daemon_enum, astg_daemon));
void		*astg_get_slot ARGS((astg_graph *, astg_slot_enum));
void		 astg_set_slot ARGS((astg_graph *, astg_slot_enum, void *));

void		 astg_sel_new ARGS((astg_graph *, char *, astg_bool));
void		 astg_sel_show ARGS((astg_graph *));
void		 astg_sel_clear ARGS((astg_graph *));
void		 astg_sel_vertex ARGS((astg_vertex *, astg_bool));
void		 astg_sel_edge ARGS((astg_edge *, astg_bool));

astg_bool	 astg_is_trigger ARGS ((astg_signal *, astg_signal *));
astg_bool	 astg_is_input ARGS((astg_signal *));
astg_bool	 astg_is_dummy ARGS((astg_signal *));
astg_bool	 astg_is_noninput ARGS((astg_signal *));
astg_signal	*astg_find_named_signal ARGS ((astg_graph *, char *));
astg_signal	*astg_find_signal ARGS((astg_graph *, char *, astg_signal_enum, astg_bool));
char		*astg_signal_name ARGS((astg_signal *));
astg_signal_enum astg_signal_type ARGS((astg_signal *));
astg_bool	 astg_get_level ARGS((astg_signal *, astg_scode));
void		 astg_set_level ARGS((astg_signal *, astg_scode *, astg_bool));
astg_retval	 astg_unique_state ARGS((astg_graph *, astg_scode *));

astg_scode	 astg_state_bit ARGS((astg_signal *));
astg_scode	 astg_state_code ARGS((astg_state *));
astg_retval	 astg_token_flow ARGS((astg_graph *, astg_bool));
astg_bool	 astg_one_sm_token ARGS((astg_graph *));
astg_retval	 astg_initial_state ARGS((astg_graph *, astg_scode *));
astg_scode	 astg_state_enabled ARGS((astg_state *));
astg_bool	 astg_state_coding ARGS((astg_graph *, astg_bool));
astg_state	*astg_new_state ARGS((astg_graph *));
void		 astg_add_marking ARGS((astg_state *, astg_marking *));
astg_bool	 astg_is_dummy_marking ARGS((astg_marking *));
void		 astg_print_state ARGS((astg_graph *, astg_scode));

void		 astg_to_blif ARGS((FILE *, astg_graph *, char *));
astg_bool	 astg_reset_state_graph ARGS((astg_graph *));
astg_retval	 astg_get_flow_status ARGS((astg_graph *));
void		 astg_set_flow_status ARGS((astg_graph *, astg_retval));
astg_state	*astg_find_state ARGS((astg_graph *, astg_scode, astg_bool));
void		 astg_delete_state ARGS((astg_state *));
void		 astg_set_marking ARGS((astg_graph *, astg_marking *));
astg_retval	 astg_set_marking_by_code ARGS((astg_graph *, astg_scode, astg_scode));
astg_scode	 astg_fire ARGS((astg_marking *, astg_trans *));
astg_scode	 astg_unfire ARGS((astg_marking *, astg_trans *));
astg_marking	*astg_initial_marking ARGS((astg_graph *));
astg_marking	*astg_dup_marking ARGS((astg_marking *));
void		 astg_delete_marking ARGS((astg_marking *));
astg_scode	 astg_get_enabled ARGS((astg_marking *));
void		 astg_print_marking ARGS ((int, astg_graph *, astg_marking *));

astg_bool	 astg_is_marked_graph ARGS((astg_graph *));
astg_bool	 astg_is_state_machine ARGS((astg_graph *));
astg_bool	 astg_is_free_choice_net ARGS((astg_graph *));
astg_bool	 astg_is_place_simple ARGS((astg_graph *));
astg_bool	 astg_is_pure ARGS((astg_graph *));
int		 astg_num_vertex ARGS((astg_graph *));
astg_scode	 astg_adjusted_code ARGS((astg_graph *, astg_scode));

void		 astg_hilite ARGS((astg_graph *));
void		 astg_basic_cmds ARGS((astg_bool));
char		*astg_pi_name ARGS((char *));
void		 astg_usage ARGS((char**, char*));

#ifdef SIS_H	/* Declarations depending on <sis.h>.	*/

astg_graph	*astg_current ARGS((network_t *));
void		 astg_set_current ARGS((network_t **, astg_graph *, astg_bool));

node_t		*astg_guard_node ARGS((astg_graph *, astg_edge *));
astg_retval	 astg_set_marking_by_name ARGS((astg_graph *, st_table *));

int		 astg_simple_cycles ARGS((astg_graph *, astg_vertex *, int (*)(astg_graph *, void *), void *, astg_bool));
array_t		*astg_top_sort ARGS((astg_graph *, astg_bool));
int		 astg_connected_comp ARGS((astg_graph *, int (*)(array_t *, int, void *), void *, astg_bool));
int		 astg_strong_comp ARGS((astg_graph *, int (*)(array_t *, int, void *), void *, astg_bool));
int		 astg_has_cycles ARGS((astg_graph *));
void		 astg_shortest_path ARGS((astg_graph *, astg_vertex *));
network_t	*astg_to_network ARGS((astg_graph*, latch_synch_t));
node_t		*astg_find_pi_or_po ARGS((network_t *, char *, astg_io_enum));

#endif /* SIS_H */

			/* Not for direct use: see iterators below. 	*/

astg_bool astg_vertex_iter ARGS((astg_graph *, astg_generator *, astg_vertex_enum, astg_vertex **));
astg_bool astg_edge_iter ARGS((astg_graph *, astg_generator *, astg_edge **));
astg_bool astg_path_iter ARGS((astg_graph *, astg_generator *, astg_vertex **));
astg_bool astg_out_edge_iter ARGS((astg_vertex *, astg_generator *, astg_edge **));
astg_bool astg_in_edge_iter ARGS((astg_vertex *, astg_generator *, astg_edge **));
astg_bool astg_in_vertex_iter ARGS((astg_vertex *, astg_generator *, astg_vertex **));
astg_bool astg_out_vertex_iter ARGS((astg_vertex *, astg_generator *, astg_vertex **));
astg_bool astg_sig_iter ARGS((astg_graph *, astg_generator *, astg_signal **));
astg_bool astg_sig_x_iter ARGS((astg_signal *, astg_generator *, astg_trans_enum, astg_trans **));
astg_bool astg_state_iter ARGS((astg_graph *, astg_generator *, astg_state **));
astg_bool astg_marking_iter ARGS((astg_state *, astg_generator *, astg_marking **));
astg_bool astg_spline_iter ARGS((astg_edge *, astg_generator *, float *, float *));


/*  DOCUMENT Public use iterators.
    Iterators:

    astg_graph *stg;
    astg_generator gen;
    astg_trans *t;
    astg_place *p;
    astg_signal *sig_p;

    astg_foreach_trans (stg,gen,t)
    astg_foreach_place (stg,gen,t)
    astg_foreach_input_place (t,gen,p)
    astg_foreach_output_place (t,gen,p)
    astg_foreach_input_trans (p,gen,t)
    astg_foreach_output_trans (p,gen,t)
    astg_foreach_signal (stg,gen,sig_p)
    astg_foreach_pos_trans (sig_p,gen,t)
    astg_foreach_neg_trans (sig_p,gen,t)
     */

#define astg_iter(g,f)	for (g.first_time=ASTG_TRUE;f;g.first_time=ASTG_FALSE)

#define astg_foreach_vertex(stg,gen,v)		astg_iter (gen, astg_vertex_iter(stg,&gen,ASTG_VERTEX,&v))
#define astg_foreach_place(stg,gen,p)		astg_iter (gen, astg_vertex_iter(stg,&gen,ASTG_TRANS,&p))
#define astg_foreach_trans(stg,gen,t)		astg_iter (gen, astg_vertex_iter(stg,&gen,ASTG_PLACE,&t))
#define astg_foreach_edge(stg,gen,e)		astg_iter (gen, astg_edge_iter(stg,&gen,&e))
#define astg_foreach_path_vertex(stg,gen,v)	astg_iter (gen, astg_path_iter(stg,&gen,&v))
#define astg_foreach_out_edge(v,gen,e)		astg_iter (gen, astg_out_edge_iter(v,&gen,&e))
#define astg_foreach_in_edge(v,gen,e)		astg_iter (gen, astg_in_edge_iter(v,&gen,&e))
#define astg_foreach_output_place(t,gen,p)	astg_iter (gen, astg_out_vertex_iter(t,&gen,&p))
#define astg_foreach_input_place(t,gen,p)	astg_iter (gen, astg_in_vertex_iter(t,&gen,&p))
#define astg_foreach_output_trans(p,gen,t) 	astg_iter (gen, astg_out_vertex_iter(p,&gen,&t))
#define astg_foreach_input_trans(p,gen,t)	astg_iter (gen, astg_in_vertex_iter(p,&gen,&t))
#define astg_foreach_signal(stg,gen,sig)	astg_iter (gen, astg_sig_iter(stg,&gen,&sig))
#define astg_foreach_sig_trans(sig,gen,t)	astg_iter (gen, astg_sig_x_iter(sig,&gen,ASTG_ALL_X,&t))
#define astg_foreach_pos_trans(sig,gen,t)	astg_iter (gen, astg_sig_x_iter(sig,&gen,ASTG_POS_X,&t))
#define astg_foreach_neg_trans(sig,gen,t)	astg_iter (gen, astg_sig_x_iter(sig,&gen,ASTG_NEG_X,&t))
#define astg_foreach_state(sig,gen,state)	astg_iter (gen, astg_state_iter(stg,&gen,&state))
#define astg_foreach_marking(state,gen,m)	astg_iter (gen, astg_marking_iter(state,&gen,&m))
#define astg_foreach_spline_point(e,gen,x,y)	astg_iter (gen, astg_spline_iter(e,&gen,&x,&y))

#endif /* ASTG_INT_H */
