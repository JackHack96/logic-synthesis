
#ifndef IO_H
#define IO_H

#include <stdio.h>
#include "network.h"
#include "graph.h"

extern void write_blif(FILE *, network_t *, int, int);

extern void write_pla(FILE *, network_t *);

extern void write_eqn(FILE *, network_t *, int);

#ifdef SIS

extern int write_kiss(FILE *, graph_t *);

extern int read_kiss(FILE *, graph_t **);

extern network_t *read_slif(FILE *);

#endif /* SIS */

extern int read_blif(FILE *, network_t **);

extern network_t *sis_read_pla(FILE *, int);

extern network_t *read_eqn(FILE *);

extern network_t *read_eqn_string(char *);

extern void read_register_filename(char *);

/* Has variable # arguments -- problems with prototype	*/
extern void read_error(char *, ...);

int init_io(void);

int end_io(void);

#endif
