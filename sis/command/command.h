/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/command/command.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:18 $
 *
 */
#ifndef COMMAND_H
#define COMMAND_H

EXTERN array_t *com_get_nodes ARGS((network_t *, int, char **));
EXTERN array_t *com_get_true_nodes ARGS((network_t *, int, char **));
EXTERN array_t *com_get_true_io_nodes ARGS((network_t *, int, char **));
EXTERN char *com_get_flag ARGS((char*));
EXTERN void com_add_command ARGS((char*, int (*)(), int));
EXTERN FILE *com_open_file ARGS((char *, char *, char **, int));
EXTERN int com_execute ARGS((network_t **, char *));

/* Functions to send graphics commands and data to a graphical front end. */

EXTERN int   com_graphics_enabled ARGS(());
EXTERN FILE *com_graphics_open ARGS((char *, char *, char *));
EXTERN void  com_graphics_close ARGS((FILE *));
EXTERN void  com_graphics_exec ARGS((char *, char *, char *, char *));

#endif /* COMMAND_H */
