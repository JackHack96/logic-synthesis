/* BDD comparison routine */


#include "bddint.h"


static
int
#if defined(__STDC__)
bdd_fraction_compare(cmu_bdd_manager bddm, bdd f, bdd g)
#else
bdd_fraction_compare(bddm, f, g)
     cmu_bdd_manager bddm;
     bdd f;
     bdd g;
#endif
{
  double f_frac, g_frac;

  bddm->op_cache.cache_level++;
  f_frac=cmu_bdd_satisfying_fraction_step(bddm, f);
  g_frac=cmu_bdd_satisfying_fraction_step(bddm, g);
  bddm->op_cache.cache_level--;
  if (f_frac < g_frac)
    return (-1);
  if (f_frac > g_frac)
    return (1);
  return (0);
}


static
int
#if defined(__STDC__)
cmu_bdd_compare_step(cmu_bdd_manager bddm, bdd f, bdd g, bdd_indexindex_type v_indexindex, long op)
#else
cmu_bdd_compare_step(bddm, f, g, v_indexindex, op)
     cmu_bdd_manager bddm;
     bdd f;
     bdd g;
     bdd_index_type v_indexindex;
     long op;
#endif
{
  bdd f1, f2;
  bdd g1, g2;
  bdd_indexindex_type top_indexindex;
  INT_PTR result;

  BDD_SETUP(f);
  BDD_SETUP(g);
  if (f == g)
    return (0);
  if (BDD_IS_CONST(f) || BDD_IS_CONST(g))
    {
      if (f == BDD_ZERO(bddm) || g == BDD_ONE(bddm))
	return (-1);
      return (1);
    }
  if (bdd_lookup_in_cache2d(bddm, op, f, g, &result))
    return ((int)result);
  BDD_TOP_VAR2(top_indexindex, bddm, f, g);
  if (bddm->indexes[top_indexindex] > bddm->indexes[v_indexindex])
    result=bdd_fraction_compare(bddm, f, g);
  else
    {
      BDD_COFACTOR(top_indexindex, f, f1, f2);
      BDD_COFACTOR(top_indexindex, g, g1, g2);
      if (!(result=cmu_bdd_compare_step(bddm, f2, g2, v_indexindex, op)))
	result=cmu_bdd_compare_step(bddm, f1, g1, v_indexindex, op);
    }
  bdd_insert_in_cache2d(bddm, op, f, g, result);
  return ((int)result);
}


int
#if defined(__STDC__)
cmu_bdd_compare_temp(cmu_bdd_manager bddm, bdd f, bdd g, bdd v)
#else
cmu_bdd_compare_temp(bddm, f, g, v)
     cmu_bdd_manager bddm;
     bdd f;
     bdd g;
     bdd v;
#endif
{
  BDD_SETUP(v);
  return (cmu_bdd_compare_step(bddm, f, g, BDD_INDEXINDEX(v), OP_CMPTO+BDD_INDEXINDEX(v)));
}


int
#if defined(__STDC__)
cmu_bdd_compare(cmu_bdd_manager bddm, bdd f, bdd g, bdd v)
#else
cmu_bdd_compare(bddm, f, g, v)
     cmu_bdd_manager bddm;
     bdd f;
     bdd g;
     bdd v;
#endif
{
  if (bdd_check_arguments(3, f, g, v))
    {
      BDD_SETUP(v);
      if (cmu_bdd_type_aux(bddm, v) != BDD_TYPE_POSVAR)
	{
	  cmu_bdd_warning("cmu_bdd_compare: third argument is not a positive variable");
	  return (0);
	}
      return (cmu_bdd_compare_step(bddm, f, g, BDD_INDEXINDEX(v), OP_CMPTO+BDD_INDEXINDEX(v)));
    }
  return (0);
}
