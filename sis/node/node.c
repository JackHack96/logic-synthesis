
#include "node_int.h"
#include "sis.h"

/*
 *  node_and -- form f * g
 */

node_t *node_and(f, g) node_t *f, *g;
{
  pset_family newf, newg, func;
  node_t **new_fanin, *r;
  int new_nin;

  if (f->F == 0 || g->F == 0) {
    fail("node_and: node does not have a function");
  }
  if (f->nin == 0) {
    if (f->F->count == 0) {
      return node_constant(0); /* f == 0, therefore 0 & g == 0 */
    } else {
      r = node_dup(g);
      FREE(r->name);
      r->name = NIL(char); /* FREE does set it to nil, this is */
      /* for safety */
      FREE(r->short_name);
      r->short_name = NIL(char);
      return r; /* f == 1, therefore 1 & g == g */
    }
  } else if (g->nin == 0) {
    if (g->F->count == 0) { /* g == 0, therefore f & 0 == 0 */
      return node_constant(0);
    } else {
      r = node_dup(f);
      FREE(r->name);
      r->name = NIL(char); /* FREE does set it to nil, this is */
      /* for safety */
      FREE(r->short_name);
      r->short_name = NIL(char);
      return r; /* g == 1, therefore f & 1 == f */
    }
  }

  /* express f and g in a common base */
  make_common_base(f, g, &new_fanin, &new_nin, &newf, &newg);

  /* define the cube size (needed for intersection) */
  define_cube_size(new_nin);

  /* compute intersection */
  func = cv_intersect(newf, newg);
  sf_free(newf);
  sf_free(newg);

  /* allocate a new node */
  r = node_create(func, new_fanin, new_nin);
  r->is_dup_free = 1;    /* make_common_base assures this */
  r->is_scc_minimal = 1; /* cv_intersect() assures this */
  node_minimum_base(r);
  return r;
}

/*
 *  node_or -- form f + g
 */

node_t *node_or(f, g) node_t *f, *g;
{
  pset_family newf, newg, func;
  node_t **new_fanin, *r;
  int new_nin;

  if (f->F == 0 || g->F == 0) {
    fail("node_or: node does not have a function");
  }
  if (f->nin == 0) {
    if (f->F->count == 0) {
      r = node_dup(g);
      FREE(r->name);
      r->name = NIL(char); /* FREE does set it to nil, this is */
      /* for safety */
      FREE(r->short_name);
      r->short_name = NIL(char);
      return r; /* f == 0, therefore 0 | g == g */
    } else {
      return node_constant(1); /* f == 1, therefore 1 | g == 1 */
    }
  } else if (g->nin == 0) {
    if (g->F->count == 0) { /* g == 0, therefore f | 0 == f */
      r = node_dup(f);
      FREE(r->name);
      r->name = NIL(char); /* FREE does set it to nil, this is */
      /* for safety */
      FREE(r->short_name);
      r->short_name = NIL(char);
      return r;
    } else {
      return node_constant(1); /* g == 1, therefore f | 1 == 1 */
    }
  }

  /* express f and g in a common base */
  make_common_base(f, g, &new_fanin, &new_nin, &newf, &newg);

  /* compute union */
  func = sf_contain(sf_append(newf, newg));

  /* allocate a new node */
  r = node_create(func, new_fanin, new_nin);
  r->is_dup_free = 1;    /* make_common_base assures this */
  r->is_scc_minimal = 1; /* sf_contain() above assures this */
  node_minimum_base(r);
  return r;
}

/*
 *  node_not -- form NOT f
 */

node_t *node_not(f) node_t *f;
{
  node_t *r;

  if (f->F == 0) {
    fail("node_not: node does not have a function");
  }
  if (f->nin == 0) {
    return node_constant(f->F->count == 0);
  }

  node_complement(f);
  r = node_create(sf_save(f->R), nodevec_dup(f->fanin, f->nin), f->nin);
  r->is_dup_free = f->is_dup_free; /* inherit whatever f was */
  r->is_scc_minimal = 1;           /* node_complement() guarantees this */
  node_minimum_base(r);
  return r;
}

/*
 *  node_xor -- form f <xor> g
 */

node_t *node_xor(f, g) node_t *f, *g;
{
  node_t *fbar, *gbar, *t0, *t1, *r;

  fbar = node_not(f);
  gbar = node_not(g);
  t0 = node_and(fbar, g);
  t1 = node_and(f, gbar);
  r = node_or(t0, t1);
  node_free(fbar);
  node_free(gbar);
  node_free(t0);
  node_free(t1);
  return r;
}

/*
 *  node_xnor -- form f <xnor> g    (also known as eqv ...)
 */

node_t *node_xnor(f, g) node_t *f, *g;
{
  node_t *fbar, *gbar, *t0, *t1, *r;

  fbar = node_not(f);
  gbar = node_not(g);
  t0 = node_and(f, g);
  t1 = node_and(fbar, gbar);
  r = node_or(t0, t1);
  node_free(fbar);
  node_free(gbar);
  node_free(t0);
  node_free(t1);
  return r;
}

node_t *node_literal(f, phase) node_t *f;
int phase;
{
  node_t *r, **fanin;
  pset_family func;
  pset pdest;

  fanin = ALLOC(node_t *, 1);
  fanin[0] = f;

  func = sf_new(1, 2);
  pdest = GETSET(func, func->count++);
  (void)set_clear(pdest, 2);
  set_insert(pdest, phase);
  r = node_create(func, fanin, 1);
  r->is_dup_free = 1;    /* isn't it obvious ... */
  r->is_scc_minimal = 1; /* obviously ... */
  return r;
}

node_t *node_constant(phase) int phase;
{
  node_t *r;
  pset_family func;

  func = sf_new(1, 0);
  switch (phase) {
  case 0:
    break;
  case 1:
    func->count++;
    break;
  default:
    fail("node_constant: phase must be 0 or 1");
  }
  r = node_create(func, NIL(node_t *), 0);
  r->is_dup_free = 1;    /* isn't it obvious ... */
  r->is_scc_minimal = 1; /* obviously ... */
  return r;
}

node_t *node_largest_cube_divisor(f) node_t *f;
{
  pset_family func;
  pset supercube;
  node_t *r;

  if (f->F == 0) {
    fail("node_largest_cube_divisor: node does not have a function");
  }
  if (f->nin == 0) {
    return node_constant(1);
  }

  supercube = sf_or(f->F);
  func = sf_new(1, f->nin * 2);
  (void)set_copy(GETSET(func, func->count++), supercube);
  set_free(supercube);

  r = node_create(func, nodevec_dup(f->fanin, f->nin), f->nin);
  r->is_dup_free = f->is_dup_free; /* inherit whatever f was */
  r->is_scc_minimal = 1;           /* a single cube is scc-minimal */
  node_minimum_base(r);
  return r;
}

/*
 *  node_contains -- see if node f contains node g
 */

int node_contains(f, g) node_t *f, *g;
{
  pset last, p, *flist;
  pset_family newf, newg;
  node_t **new_fanin;
  int contains, new_nin;

  if (f->F == 0 || g->F == 0) {
    fail("node_contains: node does not have a function");
  }

  /* express f and g in a common base */
  make_common_base(f, g, &new_fanin, &new_nin, &newf, &newg);

  /* define the cube size */
  define_cube_size(new_nin);

  flist = cube1list(newf);
  contains = 1;
  foreach_set(newg, last, p) {
    if (!cube_is_covered(flist, p)) {
      contains = 0;
      break;
    }
  }

  FREE(new_fanin);
  free_cubelist(flist);
  sf_free(newf);
  sf_free(newg);
  return contains;
}

/*
 *  node_equal -- see if node f and node g are Boolean equal
 */

int node_equal(f, g) node_t *f, *g;
{ return node_contains(f, g) && node_contains(g, f); }

/*
 *  node_equal_by_name -- see if node1 == node2 (boolean sense)
 */

int node_equal_by_name(f, g) node_t *f, *g;
{
  pset last, p, *flist, *glist;
  pset_family newf, newg;
  node_t **new_fanin;
  int eql, new_nin;

  if (f->type == PRIMARY_INPUT || f->type == PRIMARY_OUTPUT) {
    return f->type == g->type;
  }
  if (g->type == PRIMARY_INPUT || g->type == PRIMARY_OUTPUT) {
    return f->type == g->type;
  }

  if (f->F == 0 || g->F == 0) {
    fail("node_equal_by_name: node does not have a function");
  }

  /* express f and g in a common base */
  make_common_base_by_name(f, g, &new_fanin, &new_nin, &newf, &newg);
  if (new_nin == 0) {
    eql = (newf->count == newg->count);
    goto exit_free1;
  }

  /* define the cube size */
  define_cube_size(new_nin);

  flist = cube1list(newf);
  glist = cube1list(newg);

  foreach_set(newf, last, p) {
    if (!cube_is_covered(glist, p)) {
      eql = 0;
      goto exit_free;
    }
  }

  foreach_set(newg, last, p) {
    if (!cube_is_covered(flist, p)) {
      eql = 0;
      goto exit_free;
    }
  }
  eql = 1;

exit_free:
  free_cubelist(flist);
  free_cubelist(glist);
exit_free1:
  FREE(new_fanin);
  sf_free(newf);
  sf_free(newg);
  return eql;
}

node_t *node_sort_for_printing(f) node_t *f;
{
  pset_family newf, newg, func;
  node_t **new_fanin, *g, *r;
  int new_nin;

  if (f->type == PRIMARY_INPUT || f->type == PRIMARY_OUTPUT) {
    return node_dup(f);
  }
  if (f->F == 0) {
    fail("node_sort_for_printing: node does not have a function");
  }
  if (f->nin == 0 || f->F->count <= 0) {
    return node_dup(f);
  }

  g = node_constant(0);
  make_common_base_by_name(f, g, &new_fanin, &new_nin, &newf, &newg);

  func = sf_unlist(sf_sort(newf, fancy_lex_order), newf->count, newf->sf_size);

  /* allocate a new node */
  r = node_create(func, new_fanin, new_nin);
  r->is_dup_free = 1;    /* make_common_base assures this */
  r->is_scc_minimal = 1; /* pretend like it is */
  sf_free(newf);
  sf_free(newg);
  node_free(g);
  return r;
}

node_function_t node_function(node) node_t *node;
{
  register pset last, p;

  if (node->type == PRIMARY_INPUT) {
    return NODE_PI;

  } else if (node->type == PRIMARY_OUTPUT) {
    return NODE_PO;

  } else if (node->F == 0) {
    return NODE_UNDEFINED;

  } else if (node->F->count == 0) {
    if (node->nin != 0)
      goto failure;
    return NODE_0;

  } else if (node->F->count == 1) {
    p = GETSET(node->F, 0);
    if (node->nin == 0) {
      return NODE_1;
    } else if (node->nin == 1) {
      switch (GETINPUT(p, 0)) {
      case ONE:
        return NODE_BUF;
      case ZERO:
        return NODE_INV;
      default:
        goto failure;
      }
    } else {
      if (node->nin * 2 - set_ord(p) != node->nin)
        goto failure;
      return NODE_AND;
    }

  } else {
    foreach_set(node->F, last, p) {
      if (set_ord(p) != node->nin * 2 - 1) {
        return NODE_COMPLEX;
      }
    }
    return NODE_OR;
  }

failure:
  fail("node_function: function is not minimum-base");
  return NODE_COMPLEX; /* never actually executed */
}

node_type_t node_type(node) node_t *node;
{ return (node->type); }

int node_simplify_replace(F, D, mode) node_t *F, *D;
node_sim_type_t mode;
{
  node_t *newF;

  newF = node_simplify(F, D, mode);
  if (node_num_literal(newF) < node_num_literal(F)) {
    node_replace(F, newF);
    return 1;
  } else {
    node_free(newF);
    return 0;
  }
}

node_t *node_simplify(node_F, node_D, mode) node_t *node_F, *node_D;
node_sim_type_t mode;
{
  int new_nin, dummy;
  pset_family newf, newd, newr, temp;
  node_t *r, **new_fanin;

  if (node_F->F == 0) {
    fail("node_simplify: node does not have a function");
  }
  if (node_F->F->count <= 0 || node_F->nin == 0) {
    return node_dup(node_F);
  }

  /* force a D, even if we just make a constant function */
  dummy = 0;
  if (node_D == NIL(node_t)) {
    node_D = node_constant(0);
    dummy = 1;
  }

  /* express F and D in a common base */
  make_common_base(node_F, node_D, &new_fanin, &new_nin, &newf, &newd);
  define_cube_size(new_nin);

  if (dummy) {
    node_free(node_D);
  }

  switch (mode) {
  case NODE_SIM_SCC:
    newf = sf_contain(newf);
    break;
  case NODE_SIM_SIMPCOMP:
    temp = simplify(cube1list(newf));
    sf_free(newf);
    newf = temp;
    break;
  case NODE_SIM_ESPRESSO:
    newr = complement(cube2list(newf, newd));
    newf = espresso(newf, newd, newr);
    sf_free(newr);
    break;
  case NODE_SIM_EXACT:
    newf = minimize_exact(newf, newd, NIL(set_family_t), /*exact*/ 1);
    break;
  case NODE_SIM_EXACT_LITS:
    newf = minimize_exact_literals(newf, newd, NIL(set_family_t), /*exact*/ 1);
    break;
  case NODE_SIM_DCSIMP:
    newf = minimize(newf, newd, DCSIMPLIFY);
    break;
  case NODE_SIM_NOCOMP:
    newf = minimize(newf, newd, NOCOMP);
    break;
  case NODE_SIM_SNOCOMP:
    newf = minimize(newf, newd, SNOCOMP);
    break;
  default:
    fail("node_simplify: bad mode\n");
    break;
  }

  /* allocate a new node */
  r = node_create(newf, new_fanin, new_nin);
  r->is_dup_free = 1;    /* make_common_base assures this */
  r->is_scc_minimal = 1; /* minimization assures this */
  node_minimum_base(r);
  sf_free(newd);
  return r;
}

void node_scc(node) node_t *node;
{
  node->is_scc_minimal = 0; /* be pessimistic */
  node->is_dup_free = 0;    /* be pessimistic */
  node_minimum_base(node);
}

int node_num_literal(node) node_t *node;
{
  register int count;
  register pset last, p;

  if (node->type == PRIMARY_INPUT || node->type == PRIMARY_OUTPUT) {
    return 0;
  }
  if (node->F == 0) {
    fail("node_num_literal: node does not have a function");
  }
  if (node->F->count <= 0 || node->nin == 0) {
    return 0;
  }

  count = 0;
  foreach_set(node->F, last, p) { count += node->nin * 2 - set_ord(p); }
  return count;
}

int node_num_cube(node) node_t *node;
{
  if (node->type == PRIMARY_INPUT || node->type == PRIMARY_OUTPUT) {
    return 0;
  }
  if (node->F == 0) {
    fail("node_num_cube: node does not have a function");
  }
  return node->F->count;
}

int *node_literal_count(node) node_t *node;
{
  register pset last, p, fullset;
  register int *count;
  pset_family temp;

  if (node->type == PRIMARY_INPUT || node->type == PRIMARY_OUTPUT) {
    return 0;
  }
  if (node->F == 0) {
    fail("node_literal_count: node does not have a function");
  }
  if (node->nin == 0) {
    return ALLOC(int, 1); /* no literals in any variables */
  }

  temp = sf_save(node->F);
  fullset = set_fill(set_new(node->nin * 2), node->nin * 2);
  foreach_set(temp, last, p) { (void)set_diff(p, fullset, p); }
  count = sf_count(temp);
  set_free(fullset);
  sf_free(temp);

  return count;
}

void node_complement(node) node_t *node;
{
  /* if its already there, assume its correct */
  if (node->R != 0) {
    return;
  }
  if (node->type == PRIMARY_INPUT || node->type == PRIMARY_OUTPUT) {
    return;
  }
  if (node->F == 0) {
    fail("node_complement: node does not have a function");
  }
  if (node->nin == 0) {
    node->R = sf_new(4, 0);
    node->R->count = 1 - (node->F->count > 0);
  } else {
    define_cube_size(node->nin);
    node->R = sf_contain(complement(cube1list(node->F)));
  }
}

/* repeated distance 1 merge */
void node_d1merge(f) node_t *f;
{

  int nin, i, lit_count;

  nin = node_num_fanin(f);
  define_cube_size(nin);

  do {
    lit_count = node_num_literal(f);
    for (i = 0; i < nin; i++) {
      f->F = d1merge(f->F, i);
    }
  } while (lit_count > node_num_literal(f));

  f->is_dup_free = 1;
  f->is_scc_minimal = 1;
  node_minimum_base(f);
}

int node_error(code) int code;
{
  switch (code) {
  case 0:
    fail("node_get_cube: node does not have a function");
    /* NOTREACHED */
  case 1:
    fail("node_get_cube: cube index out of bounds");
    /* NOTREACHED */
  case 2:
    fail("node_get_literal: bad cube");
    /* NOTREACHED */
  case 4:
    fail("foreach_fanin: node changed during generation");
    /* NOTREACHED */
  case 5:
    fail("foreach_fanout: node changed during generation");
    /* NOTREACHED */
  default:
    fail("error code unused");
  }
  return 0;
}
