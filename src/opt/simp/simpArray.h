/**CFile****************************************************************

  FileName    [simpArray.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Two-level node minimization using don't cares.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: simpArray.h,v 1.3 2003/05/27 23:16:12 alanmi Exp $]

***********************************************************************/


#ifndef SIMP_ARRAY_H
#define SIMP_ARRAY_H

typedef struct SimpArrayStruct {
    void **space;
    int	   num;
    int	   size;
} sarray_t;

EXTERN sarray_t *sarray_do_alloc ARGS((int));
EXTERN sarray_t *sarray_dup      ARGS((sarray_t *));
EXTERN void      sarray_free     ARGS((sarray_t *));
EXTERN sarray_t *sarray_join     ARGS((sarray_t *, sarray_t *));
EXTERN int       sarray_append   ARGS((sarray_t *, sarray_t *));
EXTERN int       sarray_resize   ARGS((sarray_t *, int));
EXTERN void      sarray_sort     ARGS((sarray_t *, int (*)()));
EXTERN void      sarray_compact  ARGS((sarray_t *));

#define SARRAY_OUT_OF_MEM -10000

#define sarray_alloc(type, number)                                 \
    (sarray_do_alloc (number))

#define sarray_n(a)                                                \
    ((a)->num)

#define sarray_insert(type, a, i, datum)                           \
    ((a)->space[(i)] = (void *)(datum) )

#define sarray_insert_safe(type, a, i, datum)                      \
    (((i) >= (a)->size)? sarray_resize(a, (a)->size*2) : 0,        \
     ((a)->space[(i)] = (void *)(datum)) )

#define sarray_insert_last(type, a, datum)                         \
     ((a)->space[(a)->num++] = (void *)(datum))

#define sarray_insert_last_safe(type, a, datum)                    \
    ((((a)->num) == (a)->size)? sarray_resize(a, (a)->size*2) : 0, \
     ((a)->space[(a)->num++] = (void *)(datum)) )

#define sarray_fetch(type, a, i)			                       \
      *((type *) ((a)->space + i))

#define sarrayForEachItem(                                      \
  type,  /* type of object stored in array */                   \
  array, /* array to iterate */                                 \
  i,     /* int, local variable for iterator */                 \
  data   /* object of type */                                   \
)                                                               \
  for((i) = 0;                                                  \
      (((i) < array_n((array)))                                 \
       && (((data) = sarray_fetch(type, (array), (i))), 1));    \
      (i)++)

#endif
