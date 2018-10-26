#ifndef VAR_SET_H /* { */
#define VAR_SET_H

#define VAR_SET_BYTE_SIZE 8
#define VAR_SET_WORD_SIZE ((sizeof(unsigned int)) * (VAR_SET_BYTE_SIZE))
#define VAR_SET_ALL_ZEROS 0
#define VAR_SET_ALL_ONES ((unsigned int)~0)
#define VAR_SET_EXTRACT_BIT(word, pos) (((word) & (1 << (pos))) != 0)

#include <stdio.h>
typedef struct var_set_struct {
  int n_elts;
  int n_words;
  unsigned int *data;
} var_set_t;

var_set_t *var_set_new(int);

var_set_t *var_set_copy(var_set_t *);

var_set_t *var_set_assign(var_set_t *, var_set_t *);

void var_set_free(var_set_t *);

int var_set_n_elts(var_set_t *);

var_set_t *var_set_or(var_set_t *, var_set_t *, var_set_t *);

var_set_t *var_set_and(var_set_t *, var_set_t *, var_set_t *);

var_set_t *var_set_not(var_set_t *, var_set_t *);

int var_set_get_elt(var_set_t *, int);

void var_set_set_elt(var_set_t *, int);

void var_set_clear_elt(var_set_t *, int);

void var_set_clear(var_set_t *);

int var_set_intersect(var_set_t *, var_set_t *);

int var_set_is_empty(var_set_t *);

int var_set_is_full(var_set_t *);

void var_set_print(FILE *, var_set_t *);

int var_set_equal(var_set_t *, var_set_t *);

int var_set_cmp(char *, char *);

unsigned int var_set_hash(var_set_t *);

#endif /* } */
