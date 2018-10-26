#ifndef SI_INT_H
#define SI_INT_H

#include "node.h"

/* global variables */
int g_debug;
int add_red;
int do_reduce;

/* si_com_astg.c */
int com_astg_minimize();

int com_astg_decomp();

int com_astg_analyze();

int com_astg_verify();

int com_astg_print_sg();

int com_astg_print_stat();

int com_astg_read_eqn();

/* si_min.c */
network_t *astg_min();

/* si_3sim.c */
#define SIM3_SLOT undef1
#define GETVAL(n) ((int)((n)->SIM3_SLOT))
#define SETVAL(n, v) ((n)->SIM3_SLOT = (char *)(v))
#define EMPTY -1

void ter_simulate();

void ter_simulate_node();

void ter_sim_print();

/* si_decomp.c */
#define MAXSTR 512

network_t *two_level_decomp();

void hack_buf();

void set_names();

int feeds_po();

node_t *get_latch_end();

char *get_real_name();

int is_latch();

/* si_verify.c */
void stg_to_sr();

void network_to_sr();

/* si_encode.c */
network_t *astg_encode();

/* debug routines (si_min.c) */
void print_state();

void print_enabled();

void print_state_graph();

#endif