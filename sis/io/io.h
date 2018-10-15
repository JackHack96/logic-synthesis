/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/io/io.h,v $
 * $Author: pchong $
 * $Revision: 1.2 $
 * $Date: 2004/02/09 07:06:11 $
 *
 */
#ifndef IO_H
#define IO_H

EXTERN void write_blif ARGS((FILE *, network_t *, int, int));
EXTERN void write_pla ARGS((FILE *, network_t *));
EXTERN void write_eqn ARGS((FILE *, network_t *, int));

#ifdef SIS
EXTERN int write_kiss ARGS((FILE *, graph_t *));
EXTERN int read_kiss ARGS((FILE *, graph_t **));
EXTERN network_t *read_slif ARGS((FILE *));
#endif /* SIS */

EXTERN int read_blif ARGS((FILE *, network_t **));
EXTERN network_t *sis_read_pla ARGS((FILE *, int));
EXTERN network_t *read_eqn ARGS((FILE *));
EXTERN network_t *read_eqn_string ARGS((char *));

EXTERN void read_register_filename ARGS((char *));

	/* Has variable # arguments -- problems with prototype	*/
EXTERN void read_error (char *, ...);

#endif
