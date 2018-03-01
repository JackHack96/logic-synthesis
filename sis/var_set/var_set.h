#ifndef VAR_SET_H /* { */
#define VAR_SET_H

/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/var_set/var_set.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:15:00 $
 * $Log: var_set.h,v $
 * Revision 1.1.1.1  2004/02/07 10:15:00  pchong
 * imported
 *
 * Revision 1.3  1993/05/28  23:49:29  sis
 * Aesthetic changes to prototypes.
 *
 * Revision 1.2  1993/05/11  19:49:14  sis
 * Changes for ANSI C compatibility.
 *
 * Revision 1.1  1993/03/01  16:24:39  sis
 * Initial revision
 *
 * Revision 1.1  1993/03/01  16:23:57  sis
 * Initial revision
 *
 * Revision 1.3  1993/02/25  02:04:41  shiple
 *  Added file pointer argument to declaration of var_set_print.
 *
 * Revision 1.2  1993/02/24  23:35:16  shiple
 * Add VAR_SET_BYTE_SIZE macro. Fix newly introduced bug in
 * definition of VAR_SET_WORD_SIZE.
 *
 * Revision 1.1  1993/02/23  22:58:28  shiple
 * Initial revision
 *
 *
 */

#define VAR_SET_BYTE_SIZE 8
#define VAR_SET_WORD_SIZE ((sizeof(unsigned int))*(VAR_SET_BYTE_SIZE))
#define VAR_SET_ALL_ZEROS 0
#define VAR_SET_ALL_ONES  ((unsigned int) ~0)
#define VAR_SET_EXTRACT_BIT(word,pos) (((word) & (1 << (pos))) != 0)

typedef struct var_set_struct {
  int n_elts;
  int n_words;
  unsigned int *data;
} var_set_t;

EXTERN var_set_t *var_set_new ARGS((int));
EXTERN var_set_t *var_set_copy ARGS((var_set_t *));
EXTERN var_set_t *var_set_assign ARGS((var_set_t *, var_set_t *));
EXTERN void       var_set_free ARGS((var_set_t *));
EXTERN int        var_set_n_elts ARGS((var_set_t *));
EXTERN var_set_t *var_set_or ARGS((var_set_t *, var_set_t *, var_set_t *));
EXTERN var_set_t *var_set_and ARGS((var_set_t *, var_set_t *, var_set_t *));
EXTERN var_set_t *var_set_not ARGS((var_set_t *, var_set_t *));
EXTERN int        var_set_get_elt ARGS((var_set_t *, int));
EXTERN void       var_set_set_elt ARGS((var_set_t *, int));
EXTERN void       var_set_clear_elt ARGS((var_set_t *, int));
EXTERN void       var_set_clear ARGS((var_set_t *));
EXTERN int        var_set_intersect ARGS((var_set_t *, var_set_t *));
EXTERN int        var_set_is_empty ARGS((var_set_t *));
EXTERN int        var_set_is_full ARGS((var_set_t *));
EXTERN void       var_set_print ARGS((FILE *, var_set_t *));
EXTERN int        var_set_equal ARGS((var_set_t *, var_set_t *));
EXTERN int        var_set_cmp ARGS((char *, char *));
EXTERN unsigned int var_set_hash ARGS((var_set_t *));

#endif /* } */
