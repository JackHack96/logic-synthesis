/* BDD reduce routines */


#include "bddint.h"


static
bdd
#if defined(__STDC__)
cmu_bdd_reduce_step(cmu_bdd_manager bddm, bdd f, bdd c)
#else
cmu_bdd_reduce_step(bddm, f, c)
     cmu_bdd_manager bddm;
     bdd f;
     bdd c;
#endif
{
  bdd_indexindex_type top_indexindex;
  bdd f1, f2;
  bdd c1, c2;
  bdd temp1, temp2;
  bdd result;

  BDD_SETUP(f);
  BDD_SETUP(c);
  if (BDD_IS_CONST(c))
    {
      if (c == BDD_ZERO(bddm))
	return ((bdd)0);
      BDD_TEMP_INCREFS(f);
      return (f);
    }
  if (BDD_IS_CONST(f))
    return (f);
  if (bdd_lookup_in_cache2(bddm, OP_RED, f, c, &result))
    return (result);
  BDD_TOP_VAR2(top_indexindex, bddm, f, c);
  BDD_COFACTOR(top_indexindex, f, f1, f2);
  BDD_COFACTOR(top_indexindex, c, c1, c2);
  if (f == f1)
    {
      bddm->op_cache.cache_level++;
      temp1=cmu_bdd_ite_step(bddm, c1, BDD_ONE(bddm), c2);
      bddm->op_cache.cache_level--;
      result=cmu_bdd_reduce_step(bddm, f, temp1);
      {
	BDD_SETUP(temp1);
	BDD_TEMP_DECREFS(temp1);
      }
    }
  else
    {
      temp1=cmu_bdd_reduce_step(bddm, f1, c1);
      temp2=cmu_bdd_reduce_step(bddm, f2, c2);
      if (!temp1)
	result=temp2;
      else if (!temp2)
	result=temp1;
      else
	result=bdd_find(bddm, top_indexindex, temp1, temp2);
    }
  bdd_insert_in_cache2(bddm, OP_RED, f, c, result);
  return (result);
}


/* cmu_bdd_reduce(bddm, f, c) returns a BDD which agrees with f for all */
/* valuations for which c is true, and which is hopefully smaller than */
/* f. */

bdd
#if defined(__STDC__)
cmu_bdd_reduce(cmu_bdd_manager bddm, bdd f, bdd c)
#else
cmu_bdd_reduce(bddm, f, c)
     cmu_bdd_manager bddm;
     bdd f;
     bdd c;
#endif
{
  bdd result;

  if (bdd_check_arguments(2, f, c))
    {
      FIREWALL(bddm);
      result=cmu_bdd_reduce_step(bddm, f, c);
      if (!result)
	return (BDD_ZERO(bddm));
      RETURN_BDD(result);
    }
  return ((bdd)0);
}


static
bdd
#if defined(__STDC__)
cmu_bdd_cofactor_step(cmu_bdd_manager bddm, bdd f, bdd c)
#else
cmu_bdd_cofactor_step(bddm, f, c)
     cmu_bdd_manager bddm;
     bdd f;
     bdd c;
#endif
{
  bdd_indexindex_type top_indexindex;
  bdd f1, f2;
  bdd c1, c2;
  bdd temp1, temp2;
  bdd result;

  BDD_SETUP(f);
  BDD_SETUP(c);
  if (BDD_IS_CONST(c))
    {
      if (c == BDD_ZERO(bddm))
	return ((bdd)0);
      BDD_TEMP_INCREFS(f);
      return (f);
    }
  if (BDD_IS_CONST(f))
    return (f);
  if (bdd_lookup_in_cache2(bddm, OP_COFACTOR, f, c, &result))
    return (result);
  BDD_TOP_VAR2(top_indexindex, bddm, f, c);
  BDD_COFACTOR(top_indexindex, f, f1, f2);
  BDD_COFACTOR(top_indexindex, c, c1, c2);
  temp1=cmu_bdd_cofactor_step(bddm, f1, c1);
  temp2=cmu_bdd_cofactor_step(bddm, f2, c2);
  if (!temp1)
    result=temp2;
  else if (!temp2)
    result=temp1;
  else
    result=bdd_find(bddm, top_indexindex, temp1, temp2);
  bdd_insert_in_cache2(bddm, OP_COFACTOR, f, c, result);
  return (result);
}


/* cmu_bdd_cofactor(bddm, f, c) returns a BDD for the generalized cofactor */
/* of f by c.  This operation has the useful property that if */
/* [f1, ..., fn] is a function vector and fi|c denotes the g.c. of fi */
/* by c, then the image of [f1|c, ..., fn|c] over all valuations is */
/* the same as the image of [f1, ..., fn] over the valuations for */
/* which c is true. */

bdd
#if defined(__STDC__)
cmu_bdd_cofactor(cmu_bdd_manager bddm, bdd f, bdd c)
#else
cmu_bdd_cofactor(bddm, f, c)
     cmu_bdd_manager bddm;
     bdd f;
     bdd c;
#endif
{
  if (bdd_check_arguments(2, f, c))
    {
      if (c == BDD_ZERO(bddm))
	{
	  cmu_bdd_warning("cmu_bdd_cofactor: second argument is false");
	  return (BDD_ONE(bddm));
	}
      FIREWALL(bddm);
      RETURN_BDD(cmu_bdd_cofactor_step(bddm, f, c));
    }
  return ((bdd)0);
}
