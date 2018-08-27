
#include "gcd.h"
#include "gcd_int.h"
#include "sis.h"

#define min(x, y) (x < y ? x : y)

array_t *gcd_prime_factorize(f) node_t *f;
{
  array_t *result = array_alloc(node_t *, 0);
  node_t *p, *cofactor;

  assert(f != NIL(node_t));

  if ((node_function(f) == NODE_PI) || (node_num_cube(f) <= 1)) {
    array_insert_last(node_t *, result, node_dup(f));
    return result;
  }

  p = node_largest_cube_divisor(f);
  array_insert_last(node_t *, result, p);

  f = node_div(f, p, NIL(node_t *));

  while (node_function(f) != NODE_1) {
    p = find_prime_factor(f, &cofactor);
    array_insert_last(node_t *, result, p);
    node_free(f);
    f = cofactor;
  }
  node_free(f);
  return result;
}

node_t *gcd_nodevec(nodevec) array_t *nodevec;
{
  int i, n, best_num, m, j, best_index, new_num, last_loop;
  node_t *best_node, **node_array, *next_cube, *next_result, *r, *div, *result,
      *node, *cur_divisor;
  array_t *gcdvec;

  assert(nodevec != NIL(array_t));
  assert((n = array_n(nodevec)) != 0);

  node_array = array_data(node_t *, nodevec);

  /* Now go through the array, "deleting" 0 and nil elements by cycling them
     to the end of the array and decrementing the end index */

  for (i = 0; i < n; ++i)
    if (node_array[i] == NIL(node_t) || node_function(node_array[i]) == NODE_0)
      node_array[i--] = node_array[n--];

  assert(n != 0); /* n == 0 means no nonzero elements in the array */

  if (n == 1)
    return node_dup(node_array[0]);

  /* OK.  Find the best node to fully prime factor, and while we're at it
     we'll pull out the common cube from the array.  We'll also make every
     node cube-free.  This results in duplicating every node in the array,
     so they'll have to be freed when we return */

  result = node_largest_cube_divisor(node_array[0]);

  best_node = node_array[0] = node_div(node_array[0], result, NIL(node_t *));

  best_num = node_num_cube(best_node);

  best_index = 0;

  for (i = 1; i < n; ++i) {
    node = node_array[i];
    if (node_function(node) == NODE_1) {
      for (j = 0; j < i; ++j)
        node_free(node_array[j]);
      FREE(node_array);
      node_free(result);
      return node_constant(1);
    }
    next_cube = node_largest_cube_divisor(node_array[i]);
    node_array[i] = node_div(node_array[i], next_cube, NIL(node_t *));

    if ((node_function(result) != NODE_1) &&
        (node_function(next_cube) != NODE_1)) {
      next_result = node_or(result, next_cube);
      node_free(result);
      result = node_largest_cube_divisor(next_result);
      node_free(next_result);
    }
    node_free(next_cube);

    if ((new_num = node_num_cube(node)) < best_num) {
      best_num = new_num;
      best_node = node;
      best_index = i;
    }
  }

  node_array[best_index] = node_array[0];
  node_array[0] = best_node;

  gcdvec = gcd_prime_factorize(best_node);

  m = array_n(gcdvec);
  last_loop = 0;

  for (i = 1; (i < m) && !last_loop; ++i) {
    cur_divisor = array_fetch(node_t *, gcdvec, i);
    for (j = 1; j < n; ++j) {
      div = node_div(node_array[j], cur_divisor, &r);
      if (node_function(r) == NODE_0) {
        node_free(node_array[j]);
        node_free(r);
        node_array[j] = div;
        if (node_function(div) == NODE_1)
          last_loop = 1;
      } else {
        node_free(r);
        break;
      }
    }
    if (j == n) {
      next_result = node_and(result, cur_divisor);
      node_free(result);
      result = next_result;
    }
  }

  for (i = 0; i < n; ++i)
    node_free(node_array[i]);

  FREE(node_array);

  for (i = 0; i < m; ++i)
    node_free(array_fetch(node_t *, gcdvec, i));
  array_free(gcdvec);

  return result;
}

static node_t *find_prime_factor(f, cofactor) node_t *f;
node_t **cofactor;
{
  node_t *fanin, *best_fanin, *g, *q, *r;
  int best, num_cubes, i, *lit_count, lit_num, lit_val, phase, best_phase;

  /* Easy cases */

  if ((num_cubes = node_num_cube(f)) <= 1) {
    *cofactor = node_constant(1);
    return node_dup(f);
  }

  /* find the best variable to split around */

  best = num_cubes;
  best_fanin = (node_t *)0;

  lit_count = node_literal_count(f);

  foreach_fanin(f, i, fanin) {
    lit_num = 2 * i;

    /* Look at both phases of a variable, positive (phase 1) first,
       and negative (phase 0) second */

    for (phase = 1; phase >= 0; --phase) {
      lit_val = min(lit_count[lit_num], num_cubes - lit_count[lit_num]);
      if ((lit_val < best) && (lit_val != 0)) {
        best = lit_val;
        best_fanin = fanin;
        best_phase = phase;
      }
      ++lit_num;
    }
  }

  /* Now that the variable is found, split.  We error-check here to ensure
     that we really have found a literal, but I can't imagine how we
     couldn't */

  assert(best_fanin != (node_t *)0);

  /* make a node to contain the literal of the best_fanin */

  best_fanin = node_literal(best_fanin, best_phase);

  q = node_div(f, best_fanin, &r);

  node_free(best_fanin);

  g = internal_gcd(q, r);

  node_free(q);
  node_free(r);

  q = node_div(f, g, &r);

  /* r should be 0, so free it.  We are done.  The prime factor is q, the
     cofactor is g */

  node_free(r);

  *cofactor = g;

  return q;
}

/* Compute the gcd of two nodes by fully prime factoring the smaller, and then
   determining which factors divide the larger */

node_t *internal_gcd(q, r) node_t *q, *r;
{
  node_t *u, *u_cube, *v, *v_cube, *p, *next_u, *result;
  int q_size, r_size;

  q_size = node_num_cube(q);
  r_size = node_num_cube(r);

  /* Strictly speaking, should check that either q or r is == 1.  However,
     here we can take advantage of the fact that we have obtained q and r,
     above, from f = qx + r, whence r != 1.  If internal_gcd is ever to be
     used from any other place, this should be changed */

  if (node_function(q) == NODE_1) {
    return (node_dup(q));
  }

  if (q_size < r_size) {
    u = make_cube_free(q, &u_cube);
    v = make_cube_free(r, &v_cube);
  } else {
    u = make_cube_free(r, &v_cube);
    v = make_cube_free(q, &u_cube);
  }

  /* The initial result is just the largest cube which divides u_cube and
     v_cube evenly.  This is the largest_cube_divisor of u_cube + v_cube,
     a nice formulation due to RR */

  r = node_or(u_cube, v_cube);
  result = node_largest_cube_divisor(r);

  node_free(u_cube);
  node_free(v_cube);
  node_free(r);

  /* Find the gcd by fully prime-factoring the smaller function.  The gcd is
     then the product of those prime factors which divide v evenly */

  while (node_function(u) != NODE_1) {
    p = find_prime_factor(u, &next_u);
    q = node_div(v, p, &r);

    /* if p exactly divides v (r == 0), then p is a common prime factor.
       Set v = v/p, result = result * p with usual cleanup */

    if (node_function(r) == NODE_0) {
      node_free(v);
      v = q;
      node_free(r);
      r = node_and(result, p);
      node_free(result);
      result = r;
    } else {
      node_free(r);
    }

    /* clean up and prepare for the next iteration.  u = u/p; */

    node_free(u);
    node_free(p);
    u = next_u;
  }

  return result;
}

/* return f/c, c is the largest cube evenly dividing f, c is returned in the
   second argument.  Should disappear when Rick puts this into the library */

node_t *make_cube_free(f, f_cube) node_t *f;
node_t **f_cube;
{
  node_t *r;

  *f_cube = node_largest_cube_divisor(f);
  f = node_div(f, *f_cube, &r);
  node_free(r);
  return f;
}
