
#include "reductio.h"

pset_family expand_and_cover(F, R) pset_family F, R;
{
  pset p, old_p;
  int i, old_i;
  pset_family old_F;
  sm_matrix *matrix;
  sm_row *cover;
  sm_element *el;

  old_F = sf_save(F);
  F = expand(F, R, /*nonsparse*/ 0);

  matrix = sm_alloc();
  foreachi_set(old_F, old_i, old_p) {
    foreachi_set(F, i, p) {
      if (setp_implies(old_p, p)) {
        sm_insert(matrix, old_i, i);
      }
    }
  }
  cover = sm_minimum_cover(matrix, NIL(int), /*heuristic*/ 0, /*debug*/ 0);

  /* now copy back in F only cubes in the minimum cover */
  sf_free(old_F);
  old_F = F;
  F = sf_new(cover->length, old_F->sf_size);
  sm_foreach_row_element(cover, el) {
    p = GETSET(old_F, el->col_num);
    sf_addset(F, p);
  }

  return F;
}

espresso_coloring()

/* Colors the connected components of the incompatibles graph */

{
  int i, j;
  pset_family F, D, R, new_F;
  pset p;

  cube.num_vars = ns;
  cube.num_binary_vars = ns;
  MYALLOC(int, cube.part_size, ns);
  cube_setup();

  F = sf_new(0, 2 * ns);
  R = sf_new(0, 2 * ns);
  p = set_new(2 * ns);

  for (j = 0; j < ns; j++) {
    PUTINPUT(p, j, ZERO);
  }

  for (i = 0; i < ns; i++) {
    PUTINPUT(p, i, ONE);
    sf_addset(F, p);
    PUTINPUT(p, i, ZERO);
  }

  for (i = 0; i < ns; i++) {
    for (j = 0; j < ns; j++) {
      if (is_in_set(GETSET(incograph, i), j)) {
        PUTINPUT(p, i, ONE);
        PUTINPUT(p, j, ONE);
        sf_addset(R, p);
        PUTINPUT(p, i, ZERO);
        PUTINPUT(p, j, ZERO);
      }
    }
  }

  F = sf_contain(F);
  R = sf_contain(R);
  if (!strcmp(coloring_algo, "espresso")) {
    D = complement(cube2list(F, R));
    F = espresso(F, D, R);
    sf_free(D);
  } else if (!strcmp(coloring_algo, "espresso_exact")) {
    D = complement(cube2list(F, R));
    F = minimize_exact(F, D, R, TRUE);
    sf_free(D);
  } else if (!strcmp(coloring_algo, "expand_and_cover")) {
    F = expand_and_cover(F, R);
  } else {
    fprintf(stderr, "invalid coloring algorithm: %s\n", coloring_algo);
    exit(1);
  }

  if (color)
    free(color);
  MYALLOC(int, color, ns);
  for (i = 0; i < ns; i++)
    color[i] = 0;
  foreachi_set(F, i, p) {
    for (j = 0; j < ns; j++) {
      if (GETINPUT(p, j) != ZERO) {
        color[j] = i + 1;
      }
    }
  }

  sf_free(F);
  sf_free(R);
  set_free(p);
}
