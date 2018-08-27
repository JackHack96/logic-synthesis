
#include "extract_int.h"
#include "sis.h"

/*
 *  cubeindex functions -- hash table of cubes <-> integers
 */

cubeindex_t *cubeindex_alloc() {
  cubeindex_t *table;

  table = ALLOC(cubeindex_t, 1);
  table->cube_to_integer = st_init_table(sm_row_compare, sm_row_hash);
  table->integer_to_cube = array_alloc(sm_row *, 0);
  return table;
}

void cubeindex_free(table) cubeindex_t *table;
{
  st_generator *gen;
  char *key;

  gen = st_init_gen(table->cube_to_integer);
  while (st_gen(gen, &key, NIL(char *))) {
    sm_row_free((sm_row *)key);
  }
  st_free_gen(gen);

  st_free_table(table->cube_to_integer);
  array_free(table->integer_to_cube);
  FREE(table);
}

int cubeindex_getindex(table, cube) cubeindex_t *table;
sm_row *cube;
{
  int index;
  char *value;
  sm_row *cube_save;

  if (st_lookup(table->cube_to_integer, (char *)cube, &value)) {
    index = (int)value;
  } else {
    index = st_count(table->cube_to_integer);
    cube_save = sm_row_dup(cube);
    (void)st_insert(table->cube_to_integer, (char *)cube_save, (char *)index);
    array_insert(sm_row *, table->integer_to_cube, index, cube_save);
  }
  return index;
}

sm_row *cubeindex_getcube(table, index) cubeindex_t *table;
int index;
{ return array_fetch(sm_row *, table->integer_to_cube, index); }
