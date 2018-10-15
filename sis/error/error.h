/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/error/error.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:20 $
 *
 */
EXTERN void error_init ARGS((void));
EXTERN void error_append ARGS((char *));
EXTERN char *error_string ARGS((void));
EXTERN void error_cleanup ARGS((void));
