/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/map/multi_array.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:26 $
 *
 */
/* file @(#)multi_array.h	1.2 */
/* last modified on 5/1/91 at 15:51:39 */
#ifndef MULTDIM_ARRAY_H
#define MULTDIM_ARRAY_H

typedef struct multidim_struct multidim_t;
struct multidim_struct {
  int n_indices;
  int *max_index;
  int n_entries;
  int type_size;
  char *array;
};

extern multidim_t *generic_multidim_alloc(/* int type_size, int n_indices, int *max_index */);
extern void multidim_free(/* multidim_t * array */);

#define multidim_alloc(TYPE, n_indices, max_index)\
  generic_multidim_alloc(sizeof(TYPE), n_indices, max_index)

#define multidim_init(TYPE,ARRAY,INIT_VALUE)\
{\
  int i;\
  for (i = 0; i < (ARRAY)->n_entries; i++)\
    ((TYPE *) (ARRAY)->array)[i] = INIT_VALUE;\
}

extern int multidim_array_abort();

#ifndef FAST_ARRAY

#define INDEX2(TYPE,A,a,b)\
((sizeof(TYPE) != (A)->type_size ? multidim_array_abort(0) : 0),\
 (((a) < 0 || (a) >= (A)->max_index[0]) ? multidim_array_abort(1) : 0),\
 (((b) < 0 || (b) >= (A)->max_index[1]) ? multidim_array_abort(1) : 0),\
 (((TYPE *) (A)->array)[   (a)*(A)->max_index[1]+(b)]))

#define INDEX3(TYPE,A,a,b,c)\
((sizeof(TYPE) != (A)->type_size ? multidim_array_abort(0) : 0),\
 (((a) < 0 || (a) >= (A)->max_index[0]) ? multidim_array_abort(1) : 0),\
 (((b) < 0 || (b) >= (A)->max_index[1]) ? multidim_array_abort(1) : 0),\
 (((c) < 0 || (c) >= (A)->max_index[2]) ? multidim_array_abort(1) : 0),\
 (((TYPE *) (A)->array)[  ((a)*(A)->max_index[1]+(b))*(A)->max_index[2]+(c)]))

#define INDEX4(TYPE,A,a,b,c,d)\
((sizeof(TYPE) != (A)->type_size ? multidim_array_abort(0) : 0),\
 (((a) < 0 || (a) >= (A)->max_index[0]) ? multidim_array_abort(1) : 0),\
 (((b) < 0 || (b) >= (A)->max_index[1]) ? multidim_array_abort(1) : 0),\
 (((c) < 0 || (c) >= (A)->max_index[2]) ? multidim_array_abort(1) : 0),\
 (((d) < 0 || (d) >= (A)->max_index[3]) ? multidim_array_abort(1) : 0),\
 (((TYPE *) (A)->array)[ (((a)*(A)->max_index[1]+(b))*(A)->max_index[2]+(c))*(A)->max_index[3]+(d)]))

#define INDEX5(TYPE,A,a,b,c,d,e)\
((sizeof(TYPE) != (A)->type_size ? multidim_array_abort(0) : 0),\
 (((a) < 0 || (a) >= (A)->max_index[0]) ? multidim_array_abort(1) : 0),\
 (((b) < 0 || (b) >= (A)->max_index[1]) ? multidim_array_abort(1) : 0),\
 (((c) < 0 || (c) >= (A)->max_index[2]) ? multidim_array_abort(1) : 0),\
 (((d) < 0 || (d) >= (A)->max_index[3]) ? multidim_array_abort(1) : 0),\
 (((e) < 0 || (e) >= (A)->max_index[4]) ? multidim_array_abort(1) : 0),\
 (((TYPE *) (A)->array)[((((a)*(A)->max_index[1]+(b))*(A)->max_index[2]+(c))*(A)->max_index[3]+(d))*(A)->max_index[4]+(e)]))

#define INDEX2P(TYPE,A,a,b)\
(((sizeof(TYPE) != (A)->type_size ? multidim_array_abort(0) : 0),\
 (((a) < 0 || (a) >= (A)->max_index[0]) ? multidim_array_abort(1) : 0),\
 (((b) < 0 || (b) >= (A)->max_index[1]) ? multidim_array_abort(1) : 0)),\
 &(((TYPE *) (A)->array)[   (a)*(A)->max_index[1]+(b)]))

#define INDEX3P(TYPE,A,a,b,c)\
(((sizeof(TYPE) != (A)->type_size ? multidim_array_abort(0) : 0),\
 (((a) < 0 || (a) >= (A)->max_index[0]) ? multidim_array_abort(1) : 0),\
 (((b) < 0 || (b) >= (A)->max_index[1]) ? multidim_array_abort(1) : 0),\
 (((c) < 0 || (c) >= (A)->max_index[2]) ? multidim_array_abort(1) : 0)),\
 &(((TYPE *) (A)->array)[  ((a)*(A)->max_index[1]+(b))*(A)->max_index[2]+(c)]))

#define INDEX4P(TYPE,A,a,b,c,d)\
(((sizeof(TYPE) != (A)->type_size ? multidim_array_abort(0) : 0),\
 (((a) < 0 || (a) >= (A)->max_index[0]) ? multidim_array_abort(1) : 0),\
 (((b) < 0 || (b) >= (A)->max_index[1]) ? multidim_array_abort(1) : 0),\
 (((c) < 0 || (c) >= (A)->max_index[2]) ? multidim_array_abort(1) : 0),\
 (((d) < 0 || (d) >= (A)->max_index[3]) ? multidim_array_abort(1) : 0)),\
 &(((TYPE *) (A)->array)[ (((a)*(A)->max_index[1]+(b))*(A)->max_index[2]+(c))*(A)->max_index[3]+(d)]))

#define INDEX5P(TYPE,A,a,b,c,d,e)\
(((sizeof(TYPE) != (A)->type_size ? multidim_array_abort(0) : 0),\
 (((a) < 0 || (a) >= (A)->max_index[0]) ? multidim_array_abort(1) : 0),\
 (((b) < 0 || (b) >= (A)->max_index[1]) ? multidim_array_abort(1) : 0),\
 (((c) < 0 || (c) >= (A)->max_index[2]) ? multidim_array_abort(1) : 0),\
 (((d) < 0 || (d) >= (A)->max_index[3]) ? multidim_array_abort(1) : 0),\
 (((e) < 0 || (e) >= (A)->max_index[4]) ? multidim_array_abort(1) : 0)),\
 &(((TYPE *) (A)->array)[((((a)*(A)->max_index[1]+(b))*(A)->max_index[2]+(c))*(A)->max_index[3]+(d))*(A)->max_index[4]+(e)]))

#else /* i.e. ifdef FAST_ARRAY */

/* this is a lot faster than the check-me-all macros listed above: no seat belts! */

#define INDEX2(TYPE,A,a,b)\
(((TYPE *) (A)->array)[   (a)*(A)->max_index[1]+(b)])

#define INDEX3(TYPE,A,a,b,c)\
(((TYPE *) (A)->array)[  ((a)*(A)->max_index[1]+(b))*(A)->max_index[2]+(c)])

#define INDEX4(TYPE,A,a,b,c,d)\
(((TYPE *) (A)->array)[ (((a)*(A)->max_index[1]+(b))*(A)->max_index[2]+(c))*(A)->max_index[3]+(d)])

#define INDEX5(TYPE,A,a,b,c,d,e)\
(((TYPE *) (A)->array)[((((a)*(A)->max_index[1]+(b))*(A)->max_index[2]+(c))*(A)->max_index[3]+(d))*(A)->max_index[4]+(e)])

#define INDEX2P(TYPE,A,a,b)\
&(((TYPE *) (A)->array)[   (a)*(A)->max_index[1]+(b)])

#define INDEX3P(TYPE,A,a,b,c)\
&(((TYPE *) (A)->array)[  ((a)*(A)->max_index[1]+(b))*(A)->max_index[2]+(c)])

#define INDEX4P(TYPE,A,a,b,c,d)\
&(((TYPE *) (A)->array)[ (((a)*(A)->max_index[1]+(b))*(A)->max_index[2]+(c))*(A)->max_index[3]+(d)])

#define INDEX5P(TYPE,A,a,b,c,d,e)\
&(((TYPE *) (A)->array)[((((a)*(A)->max_index[1]+(b))*(A)->max_index[2]+(c))*(A)->max_index[3]+(d))*(A)->max_index[4]+(e)])

#endif /* FAST_ARRAY */

#endif /* MULTIDIM_ARRAY_H */
