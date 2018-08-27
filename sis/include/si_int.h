/* global variables */
int g_debug;
int add_red;
int do_reduce;

/* si_com_astg.c */
extern int com_astg_minimize();

extern int com_astg_decomp();

extern int com_astg_analyze();

extern int com_astg_verify();

extern int com_astg_print_sg();

extern int com_astg_print_stat();

extern int com_astg_read_eqn();

/* si_min.c */
extern network_t *astg_min();

/* si_3sim.c */
#define SIM3_SLOT undef1
#define GETVAL(n) ((int)((n)->SIM3_SLOT))
#define SETVAL(n, v) ((n)->SIM3_SLOT = (char *)(v))
#define EMPTY -1

extern void ter_simulate();

extern void ter_simulate_node();

extern void ter_sim_print();

/* si_decomp.c */
#define MAXSTR 512

extern network_t *two_level_decomp();

extern void hack_buf();

extern void set_names();

extern int feeds_po();

extern node_t *get_latch_end();

extern char *get_real_name();

extern int is_latch();

/* si_verify.c */
extern void stg_to_sr();

extern void network_to_sr();

/* si_encode.c */
extern network_t *astg_encode();

/* debug routines (si_min.c) */
extern void print_state();

extern void print_enabled();

extern void print_state_graph();
