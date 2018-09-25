
#ifndef COMMAND_H
#define COMMAND_H

#include "array.h"
#include "network.h"
#include <stdio.h>

extern array_t *com_get_nodes(network_t *, int, char **);

extern array_t *com_get_true_nodes(network_t *, int, char **);

extern array_t *com_get_true_io_nodes(network_t *, int, char **);

extern char *com_get_flag(char *);

extern void com_add_command(char *, int (*)(), int);

extern FILE *com_open_file(char *, char *, char **, int);

extern int com_execute(network_t **, char *);

/* Functions to send graphics commands and data to a graphical front end. */

extern int com_graphics_enabled();

extern FILE *com_graphics_open(char *, char *, char *);

extern void com_graphics_close(FILE *);

extern void com_graphics_exec(char *, char *, char *, char *);

extern int init_command(int graphics_flag);

int end_command(void);

void com_graphics_help (void);

#endif /* COMMAND_H */
