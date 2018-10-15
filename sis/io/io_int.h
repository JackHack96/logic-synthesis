#ifndef IO_INT_H
#define IO_INT_H

#include <stdio.h>
#include "array.h"
#include "network.h"
#include "list.h"

void io_fprintf_break(FILE *, char *, ...);

void io_fputs_break(register FILE *fp, register char *s);

void io_fputc_break();

void io_fput_params();

void write_sop();

char *io_node_name();

char *io_name();

void io_write_name();

void io_write_node();

void io_write_gate();

int io_po_fanout_count();

int io_rpo_fanout_count();

int io_lpo_fanout_count();

int io_node_should_be_printed();

void read_filename_to_netname(network_t *network, char *filename);

node_t *read_find_or_create_node();

node_t *read_slif_find_or_create_node();

int read_check_io_list();

void read_hack_outputs();

void read_cleanup_buffers();

#ifdef SIS

void read_delay_cleanup();

void read_change_madeup_name();

#endif /* SIS */

node_t *read_add_fake_primary_output();

void write_blif_for_bdsyn();

void write_blif_slif_delay();

int  read_lineno;
char *read_filename;

int com_write_slif();

int com_plot_blif();

char *gettoken();

#define MAX_WORD 1024

typedef struct {
    array_t *actuals; /* Array of nodes: inputs, then outputs */
    char    **formals;   /* Array of formals */
    int     inputs;       /* How many entries in `actuals' are inputs */
    char    *netname;    /* Network to patch this network into */
}           patchinfo;

typedef struct {           /* Data field in the models table */
    network_t *network;      /* The network */
    lsList    po_list;          /* The po_list for the network */
    lsList    latch_order_list; /* Ordering for latches in stg */
    lsList    patch_lists;      /* Who depends on me */
    int       depends_on;          /* How many I depend on */
    int       library;             /* Is this going to be put in library? */
}           modelinfo;

modelinfo *read_find_or_create_model();

#ifdef SIS

int slif_add_to_library();

#endif /* SIS */

int read_blif_slif();

#endif