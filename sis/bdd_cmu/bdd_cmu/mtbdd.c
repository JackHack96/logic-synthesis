/* Basic multi-terminal BDD routines */


#include "bddint.h"


/* mtbdd_transform_closure(bddm, canonical_fn, transform_fn, env) sets */
/* the transformation for MTBDD terminal values for the "negative-output" */
/* pointer flag.  The canonical_fn receives the BDD manager, two longs */
/* representing the input value, and the value of env.  It should return */
/* a non-zero value if the result needs to be transformed.  The */
/* transform_fn receives the BDD manager, two longs (the input value), */
/* pointers to two longs (for the output) and the value of env.  This */
/* should not be called after any MTBDD terminals are created. */

void
#if defined(__STDC__)
mtbdd_transform_closure(cmu_bdd_manager bddm,
			int (*canonical_fn)(cmu_bdd_manager, INT_PTR, INT_PTR, pointer),
			void (*transform_fn)(cmu_bdd_manager, INT_PTR, INT_PTR, INT_PTR *, INT_PTR *, pointer),
			pointer transform_env)
#else
mtbdd_transform_closure(bddm, canonical_fn, transform_fn, transform_env)
     cmu_bdd_manager bddm;
     int (*canonical_fn)();
     void (*transform_fn)();
     pointer transform_env;
#endif
{
  bddm->transform_fn=transform_fn;
  bddm->transform_env=transform_env;
  bddm->canonical_fn=canonical_fn;
}


/* mtcmu_bdd_one_data(bddm, value1, value2) sets the MTBDD value for TRUE. */
/* This should not be called after MTBDD terminals have been created. */

void
#if defined(__STDC__)
mtcmu_bdd_one_data(cmu_bdd_manager bddm, INT_PTR value1, INT_PTR value2)
#else
mtcmu_bdd_one_data(bddm, value1, value2)
     cmu_bdd_manager bddm;
     INT_PTR value1;
     INT_PTR value2;
#endif
{
  var_table table;
  long hash;

  table=bddm->unique_table.tables[BDD_CONST_INDEXINDEX];
  if (table->entries != 1)
    cmu_bdd_fatal("mtcmu_bdd_one_data: other terminal nodes already exist");
  hash=HASH_NODE(bddm->one->data[0], bddm->one->data[1]);
  BDD_REDUCE(hash, table->size);
  table->table[hash]=0;
  bddm->one->data[0]=value1;
  bddm->one->data[1]=value2;
  hash=HASH_NODE(bddm->one->data[0], bddm->one->data[1]);
  BDD_REDUCE(hash, table->size);
  table->table[hash]=bddm->one;
}


/* cmu_mtbdd_free_terminal_closure(bddm, free_terminal_fn, free_terminal_env) */
/* sets the closure to be invoked on when freeing MTBDD terminals.  If */
/* free_terminal_fn is null, it indicates that no function should be */
/* called.  The free_terminal_fn gets the BDD manager, two longs */
/* holding the data for the terminal, and the value of free_terminal_env. */

void
#if defined(__STDC__)
cmu_mtbdd_free_terminal_closure(cmu_bdd_manager bddm,
			    void (*free_terminal_fn)(cmu_bdd_manager, INT_PTR, INT_PTR, pointer),
			    pointer free_terminal_env)
#else
cmu_mtbdd_free_terminal_closure(bddm, free_terminal_fn, free_terminal_env)
     cmu_bdd_manager bddm;
     void (*free_terminal_fn)();
     pointer free_terminal_env;
#endif
{
  bddm->unique_table.free_terminal_fn=free_terminal_fn;
  bddm->unique_table.free_terminal_env=free_terminal_env;
}


/* cmu_mtbdd_get_terminal(bddm, value1, value2) returns the multi-terminal */
/* BDD for a constant. */

bdd
#if defined(__STDC__)
cmu_mtbdd_get_terminal(cmu_bdd_manager bddm, INT_PTR value1, INT_PTR value2)
#else
cmu_mtbdd_get_terminal(bddm, value1, value2)
     cmu_bdd_manager bddm;
     INT_PTR value1;
     INT_PTR value2;
#endif
{
  FIREWALL(bddm);
  RETURN_BDD(bdd_find_terminal(bddm, value1, value2));
}


/* cmu_mtbdd_terminal_value(bddm, f, value1, value2) returns the data value */
/* for the terminal node f. */

void
#if defined(__STDC__)
cmu_mtbdd_terminal_value(cmu_bdd_manager bddm, bdd f, INT_PTR *value1, INT_PTR *value2)
#else
cmu_mtbdd_terminal_value(bddm, f, value1, value2)
     cmu_bdd_manager bddm;
     bdd f;
     INT_PTR *value1;
     INT_PTR *value2;
#endif
{
  if (bdd_check_arguments(1, f))
    {
      BDD_SETUP(f);
      if (!BDD_IS_CONST(f))
	{
	  cmu_bdd_warning("mtbdd_terminal_data: argument is terminal node");
	  *value1=0;
	  *value2=0;
	  return;
	}
      cmu_mtbdd_terminal_value_aux(bddm, f, value1, value2);
    }
}


static
bdd
#if defined(__STDC__)
mtcmu_bdd_ite_step(cmu_bdd_manager bddm, bdd f, bdd g, bdd h)
#else
mtcmu_bdd_ite_step(bddm, f, g, h)
     cmu_bdd_manager bddm;
     bdd f;
     bdd g;
     bdd h;
#endif
{
  bdd_indexindex_type top_indexindex;
  bdd f1, f2;
  bdd g1, g2;
  bdd h1, h2;
  bdd temp1, temp2;
  bdd result;

  BDD_SETUP(f);
  BDD_SETUP(g);
  BDD_SETUP(h);
  if (BDD_IS_CONST(f))
    {
      if (f == BDD_ONE(bddm))
	{
	  BDD_TEMP_INCREFS(g);
	  return (g);
	}
      BDD_TEMP_INCREFS(h);
      return (h);
    }
  /* f is not constant. */
  if (g == h)
    {
      BDD_TEMP_INCREFS(g);
      return (g);
    }
  /* f is not constant, g and h are distinct. */
  if (!BDD_IS_OUTPOS(f))
    {
      f=BDD_NOT(f);
      BDD_SWAP(g, h);
    }
  /* f is now an uncomplemented output pointer. */
  if (bdd_lookup_in_cache31(bddm, CACHE_TYPE_ITE, (INT_PTR)f, (INT_PTR)g, (INT_PTR)h, (INT_PTR *)&result))
    return (result);
  BDD_TOP_VAR3(top_indexindex, bddm, f, g, h);
  BDD_COFACTOR(top_indexindex, f, f1, f2);
  BDD_COFACTOR(top_indexindex, g, g1, g2);
  BDD_COFACTOR(top_indexindex, h, h1, h2);
  temp1=mtcmu_bdd_ite_step(bddm, f1, g1, h1);
  temp2=mtcmu_bdd_ite_step(bddm, f2, g2, h2);
  result=bdd_find(bddm, top_indexindex, temp1, temp2);
  bdd_insert_in_cache31(bddm, CACHE_TYPE_ITE, (INT_PTR)f, (INT_PTR)g, (INT_PTR)h, (INT_PTR)result);
  return (result);
}


/* mtcmu_bdd_ite(bddm, f, g, h) returns the BDD for "if f then g else h", */
/* where g and h are multi-terminal BDDs. */

bdd
#if defined(__STDC__)
mtcmu_bdd_ite(cmu_bdd_manager bddm, bdd f, bdd g, bdd h)
#else
mtcmu_bdd_ite(bddm, f, g, h)
     cmu_bdd_manager bddm;
     bdd f;
     bdd g;
     bdd h;
#endif
{
  if (bdd_check_arguments(3, f, g, h))
    {
      FIREWALL(bddm);
      RETURN_BDD(mtcmu_bdd_ite_step(bddm, f, g, h));
    }
  return ((bdd)0);
}


/* mtcmu_bdd_substitute(bddm, f) does the analog of cmu_bdd_substitute for MTBDDs. */

bdd
#if defined(__STDC__)
mtcmu_bdd_substitute(cmu_bdd_manager bddm, bdd f)
#else
mtcmu_bdd_substitute(bddm, f)
     cmu_bdd_manager bddm;
     bdd f;
#endif
{
  long op;

  if (bdd_check_arguments(1, f))
    {
      FIREWALL(bddm);
      if (bddm->curr_assoc_id == -1)
	op=bddm->temp_op--;
      else
	op=OP_SUBST+bddm->curr_assoc_id;
      RETURN_BDD(cmu_bdd_substitute_step(bddm, f, op, mtcmu_bdd_ite_step, bddm->curr_assoc));
    }
  return ((bdd)0);
}


static
bdd
#if defined(__STDC__)
cmu_mtbdd_equal_step(cmu_bdd_manager bddm, bdd f, bdd g)
#else
cmu_mtbdd_equal_step(bddm, f, g)
     cmu_bdd_manager bddm;
     bdd f;
     bdd g;
#endif
{
  bdd_indexindex_type top_indexindex;
  bdd f1, f2;
  bdd g1, g2;
  bdd temp1, temp2;
  bdd result;

  BDD_SETUP(f);
  BDD_SETUP(g);
  if (f == g)
    return (BDD_ONE(bddm));
  if (BDD_IS_CONST(f) && BDD_IS_CONST(g))
    return (BDD_ZERO(bddm));
  if (BDD_OUT_OF_ORDER(f, g))
    BDD_SWAP(f, g);
  if (bdd_lookup_in_cache2(bddm, OP_EQUAL, f, g, &result))
    return (result);
  BDD_TOP_VAR2(top_indexindex, bddm, f, g);
  BDD_COFACTOR(top_indexindex, f, f1, f2);
  BDD_COFACTOR(top_indexindex, g, g1, g2);
  temp1=cmu_mtbdd_equal_step(bddm, f1, g1);
  temp2=cmu_mtbdd_equal_step(bddm, f2, g2);
  result=bdd_find(bddm, top_indexindex, temp1, temp2);
  bdd_insert_in_cache2(bddm, OP_EQUAL, f, g, result);
  return (result);
}


/* cmu_mtbdd_equal(bddm, f, g) returns a BDD indicating when the */
/* multi-terminal BDDs f and g are equal. */

bdd
#if defined(__STDC__)
cmu_mtbdd_equal(cmu_bdd_manager bddm, bdd f, bdd g)
#else
cmu_mtbdd_equal(bddm, f, g)
     cmu_bdd_manager bddm;
     bdd f;
     bdd g;
#endif
{
  if (bdd_check_arguments(2, f, g))
    {
      FIREWALL(bddm);
      RETURN_BDD(cmu_mtbdd_equal_step(bddm, f, g));
    }
  return ((bdd)0);
}
