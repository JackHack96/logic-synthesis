
#ifndef IO_H
#define IO_H

#include <stdio.h>
#include "network.h"
#include "graph.h"

void write_blif(FILE *, network_t *, int, int);

void write_pla(FILE *, network_t *);

void write_eqn(FILE *, network_t *, int);

#ifdef SIS

int write_kiss(FILE *, graph_t *);

int read_kiss(FILE *, graph_t **);

network_t *read_slif(FILE *);

#endif /* SIS */

int read_blif(FILE *, network_t **);

network_t *sis_read_pla(FILE *, int);

network_t *read_eqn(FILE *);

network_t *read_eqn_string(char *);

void read_register_filename(char *);

/* Has variable # arguments -- problems with prototype	*/
void read_error(char *, ...);

int init_io(void);

int end_io(void);

#endif
