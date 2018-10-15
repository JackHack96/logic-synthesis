/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/var_set/var_set.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:15:00 $
 * $Log: var_set.c,v $
 * Revision 1.1.1.1  2004/02/07 10:15:00  pchong
 * imported
 *
 * Revision 1.2  1993/07/19  23:34:41  sis
 * *** empty log message ***
 *
 * Revision 1.4  1993/06/25  16:33:05  shiple
 * Fixed bug in print statement.
 *
 * Revision 1.3  1993/02/25  02:04:41  shiple
 * Added file pointer argument to var_set_print.
 *
 * Revision 1.2  1993/02/24  23:34:46  shiple
 * Replace "8" by VAR_SET_BYTE_SIZE.
 *
 * Revision 1.1  1993/02/23  22:57:44  shiple
 * Initial revision
 *
 *
 */
#include "util.h"
#include "var_set.h"

var_set_t *var_set_new(size)
int size;
{
  var_set_t *result = ALLOC(var_set_t, 1);

  result->n_elts = size;
  result->n_words = size / VAR_SET_WORD_SIZE + ((size % VAR_SET_WORD_SIZE == 0) ? 0 : 1);
  result->data = ALLOC(unsigned int, result->n_words);
  (void) var_set_clear(result);
  return result;
}

var_set_t *var_set_copy(set)
var_set_t *set;
{
  int i;
  var_set_t *result = ALLOC(var_set_t, 1);

  *result = *set;
  result->data = ALLOC(unsigned int, result->n_words);
  for (i = 0; i < result->n_words; i++)
    result->data[i] = set->data[i];
  return result;
}

var_set_t *var_set_assign(result, set)
var_set_t *result;
var_set_t *set;
{
  int i;

  assert(result->n_elts == set->n_elts);
  for (i = 0; i < result->n_words; i++)
    result->data[i] = set->data[i];
  return result;
}

void var_set_free(set)
var_set_t *set;
{
  FREE(set->data);
  FREE(set);
}

static int size_array[256];

static int init_size_array()
{
  int i, j;
  int count;

  for (i = 0; i < 256; i++) {
    count = 0;
    for (j = 0; j < VAR_SET_WORD_SIZE; j++) {
      count += VAR_SET_EXTRACT_BIT(i, j);
    }
    size_array[i] = count;
  }
}

int var_set_n_elts(set)
var_set_t *set;
{
  register int i, j;
  register unsigned int value;
  int n_bytes = VAR_SET_WORD_SIZE / VAR_SET_BYTE_SIZE;
  int count = 0;

  if (size_array[1] == 0) init_size_array();
  for (i = 0; i < set->n_words; i++) {
    value = set->data[i];
    for (j = 0; j < n_bytes; j++) {
      count += size_array[value & 0xff];
      value >>= VAR_SET_BYTE_SIZE;
    }
  }
  return count;
}

var_set_t *var_set_or(result, a, b)
var_set_t *result;
var_set_t *a;
var_set_t *b;
{
  int i;
  assert(result->n_elts == a->n_elts);
  assert(result->n_elts == b->n_elts);
  for (i = 0; i < result->n_words; i++)
    result->data[i] = a->data[i] | b->data[i];
  return result;
}

var_set_t *var_set_and(result, a, b)
var_set_t *result;
var_set_t *a;
var_set_t *b;
{
  int i;
  assert(result->n_elts == a->n_elts);
  assert(result->n_elts == b->n_elts);
  for (i = 0; i < result->n_words; i++)
    result->data[i] = a->data[i] & b->data[i];
  return result;
}

var_set_t *var_set_not(result, a)
var_set_t *result;
var_set_t *a;
{
  int i;
  unsigned int mask;

  assert(result->n_elts == a->n_elts);
  for (i = 0; i < a->n_words; i++)
    result->data[i] = ~a->data[i];
  mask = (unsigned int) VAR_SET_ALL_ONES >> (a->n_words * VAR_SET_WORD_SIZE - a->n_elts);
  result->data[a->n_words - 1] &= mask;
  return result;
}

int var_set_get_elt(set, index)
var_set_t *set;
int index;
{
  assert(index >= 0 && index < set->n_elts);
  return VAR_SET_EXTRACT_BIT(set->data[index / VAR_SET_WORD_SIZE], index % VAR_SET_WORD_SIZE);
}

void var_set_set_elt(set, index)
var_set_t *set;
int index;
{
  unsigned int *value;
  assert(index >= 0 && index < set->n_elts);
  value = &(set->data[index / VAR_SET_WORD_SIZE]);
  *value = *value | (1 << (index % VAR_SET_WORD_SIZE));
}

void var_set_clear_elt(set, index)
var_set_t *set;
int index;
{
  unsigned int *value;
  assert(index >= 0 && index < set->n_elts);
  value = &(set->data[index / VAR_SET_WORD_SIZE]);
  *value = *value & ~(1 << (index % VAR_SET_WORD_SIZE));
}

void var_set_clear(set)
var_set_t *set;
{
  int i;

  for (i = 0; i < set->n_words; i++)
    set->data[i] = 0;
}

int var_set_intersect(a, b)
var_set_t *a;
var_set_t *b;
{
  int i;
  assert(a->n_elts == b->n_elts);
  for (i = 0; i < a->n_words; i++)
    if (a->data[i] & b->data[i]) return 1;
  return 0;
}

int var_set_is_empty(a)
var_set_t *a;
{
  int i;
  for (i = 0; i < a->n_words; i++)
    if (a->data[i]) return 0;
  return 1;
}

int var_set_is_full(a)
var_set_t *a;
{
  int i;
  int value;
  for (i = 0; i < a->n_words - 1; i++)
    if (a->data[i] != VAR_SET_ALL_ONES) return 0;
  value = VAR_SET_ALL_ONES >> (a->n_words * VAR_SET_WORD_SIZE - a->n_elts);
  return (a->data[a->n_words - 1] == value);
}

void var_set_print(fp, set)
FILE *fp;
var_set_t *set;
{
  int i;
  for (i = 0; i < set->n_elts; i++) {
    fprintf(fp, "%d ", var_set_get_elt(set, i));
  }
  fprintf(fp, "\n");
}

 /* returns 1 if equal, 0 otherwise */

int var_set_equal(a, b)
var_set_t *a;
var_set_t *b;
{
  int i;

  assert(a->n_elts == b->n_elts);
  for (i = 0; i < a->n_words; i++)
    if (a->data[i] != b->data[i]) return 0;
  return 1;
}

 /* returns 0 if equal, 1 otherwise */

int var_set_cmp(obj1, obj2)
char *obj1;
char *obj2;
{
  int i;
  var_set_t *a = (var_set_t *) obj1;
  var_set_t *b = (var_set_t *) obj2;

  assert(a->n_elts == b->n_elts);
  for (i = 0; i < a->n_words; i++)
    if (a->data[i] != b->data[i]) return 1;
  return 0;
}

 /* to be used when sets are used as keys in hash tables */
unsigned int var_set_hash(set)
var_set_t *set;
{
  int i;
  unsigned int result = 0;

  for (i = 0; i < set->n_words; i++)
    result += (unsigned int) set->data[i];
  return result;
}
