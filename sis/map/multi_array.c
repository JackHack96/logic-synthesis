/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/map/multi_array.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:26 $
 *
 */
/* file @(#)multi_array.c	1.2 */
/* last modified on 5/1/91 at 15:51:37 */
#include "sis.h"
#include "multi_array.h"

multidim_t *generic_multidim_alloc(type_size, n_indices, max_index)
int type_size;
int n_indices;
int *max_index;
{
  int i;
  int size = type_size;
  multidim_t *array = ALLOC(multidim_t, 1);

  array->n_indices = n_indices;
  array->max_index = ALLOC(int, n_indices);
  for (i = 0; i < n_indices; i++) {
    array->max_index[i] = max_index[i];
    size *= max_index[i];
  }
  assert(size > 0);
  array->n_entries = size / type_size;
  array->array = ALLOC(char, size);
  array->type_size = type_size;
  return array;
}

void multidim_free(array)
multidim_t *array;
{
  FREE(array->array);
  FREE(array->max_index);
  FREE(array);
}

int multidim_array_abort(code)
int code;
{
  fprintf(stderr, "multidim array error: ");
  switch (code) {
  case 0:
    fprintf(stderr, "inconsistent object size");
    break;
  case 1:
    fprintf(stderr, "index out of bounds ");
    break;
  default:
    fprintf(stderr, "unknown error");
    break;
  }
  fail("\n");
}
