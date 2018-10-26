
#ifndef COMMAND_H
#define COMMAND_H

#include "array.h"
#include "network.h"
#include <stdio.h>

array_t *com_get_nodes(network_t *, int, char **);

array_t *com_get_true_nodes(network_t *, int, char **);

array_t *com_get_true_io_nodes(network_t *, int, char **);

char *com_get_flag(char *);

void com_add_command(char *, int (*)(), int);

FILE *com_open_file(char *, char *, char **, int);

int com_execute(network_t **, char *);

/* Functions to send graphics commands and data to a graphical front end. */

int com_graphics_enabled();

FILE *com_graphics_open(char *, char *, char *);

void com_graphics_close(FILE *);

void com_graphics_exec(char *, char *, char *, char *);

int init_command(int graphics_flag);

int end_command(void);

void com_graphics_help (void);

#endif /* COMMAND_H */
