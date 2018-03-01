/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/octio/farray.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:47 $
 *
 */
#ifndef F_ARRAY_H
#define F_ARRAY_H

#define f_array_insert(type, a, i, datum) {		\
    int array_i;\
    (array_i = (i),				\
      array_i >= (a)->n_size ? array_resize(a, array_i + 1) : 0,\
      array_i >= (a)->num ? (a)->num = array_i + 1 : 0,\
      *((type *) ((a)->space + array_i * sizeof(type))) = datum);\
    }

#define f_array_insert_last(type, array, datum)	\
    f_array_insert(type, array, (array)->num, datum)

#endif /* F_ARRAY_H */
