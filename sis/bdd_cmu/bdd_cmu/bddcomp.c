/* BDD composition routines */


#include "bddint.h"


static
bdd
#if defined(__STDC__)
bdd_restrict_step(cmu_bdd_manager bddm, bdd f, bdd_indexindex_type g_indexindex, bdd h, long op)
#else
bdd_restrict_step(bddm, f, g_indexindex, h, op)
     cmu_bdd_manager bddm;
     bdd f;
     bdd_indexindex_type g_indexindex;
     bdd h;
     long op;
#endif
{
  bdd temp1, temp2;
  bdd result;

  BDD_SETUP(f);
  if (BDD_INDEX(bddm, f) > bddm->indexes[g_indexindex])
    {
      /* f doesn't depend on the variable g. */
      BDD_TEMP_INCREFS(f);
      return (f);
    }
  if (BDD_INDEXINDEX(f) == g_indexindex)
    {
      /* Do the restriction. */
      result=(h == BDD_ONE(bddm) ? BDD_THEN(f) : BDD_ELSE(f));
      {
	BDD_SETUP(result);
	BDD_TEMP_INCREFS(result);
	return (result);
      }
    }
  if (bdd_lookup_in_cache2(bddm, op, BDD_OUTPOS(f), h, &result))
    return (BDD_IS_OUTPOS(f) ? result : BDD_NOT(result));
  temp1=bdd_restrict_step(bddm, BDD_THEN(f), g_indexindex, h, op);
  temp2=bdd_restrict_step(bddm, BDD_ELSE(f), g_indexindex, h, op);
  result=bdd_find(bddm, BDD_INDEXINDEX(f), temp1, temp2);
  if (BDD_IS_OUTPOS(f))
    bdd_insert_in_cache2(bddm, op, f, h, result);
  else
    bdd_insert_in_cache2(bddm, op, BDD_NOT(f), h, BDD_NOT(result));
  return (result);
}


static
bdd
#if defined(__STDC__)
cmu_bdd_compose_step(cmu_bdd_manager bddm, bdd f, bdd_indexindex_type g_indexindex, bdd h, long op)
#else
cmu_bdd_compose_step(bddm, f, g_indexindex, h, op)
     cmu_bdd_manager bddm;
     bdd f;
     bdd_indexindex_type g_indexindex;
     bdd h;
     long op;
#endif
{
  bdd_indexindex_type top_indexindex;
  bdd f1, f2;
  bdd h1, h2;
  bdd temp1, temp2;
  bdd result;

  BDD_SETUP(f);
  BDD_SETUP(h);
  /* Use restriction if possible. */
  if (BDD_IS_CONST(h))
    return (bdd_restrict_step(bddm, f, g_indexindex, h, op));
  if (BDD_INDEX(bddm, f) > bddm->indexes[g_indexindex])
    {
      /* f doesn't depend on the variable g. */
      BDD_TEMP_INCREFS(f);
      return (f);
    }
  if (bdd_lookup_in_cache2(bddm, op, BDD_OUTPOS(f), h, &result))
    return (BDD_IS_OUTPOS(f) ? result : BDD_NOT(result));
  if (BDD_INDEXINDEX(f) == g_indexindex)
    {
      /* Do the replacement. */
      bddm->op_cache.cache_level++;
      result=cmu_bdd_ite_step(bddm, h, BDD_THEN(f), BDD_ELSE(f));
      bddm->op_cache.cache_level--;
    }
  else
    {
      BDD_TOP_VAR2(top_indexindex, bddm, f, h);
      BDD_COFACTOR(top_indexindex, f, f1, f2);
      BDD_COFACTOR(top_indexindex, h, h1, h2);
      temp1=cmu_bdd_compose_step(bddm, f1, g_indexindex, h1, op);
      temp2=cmu_bdd_compose_step(bddm, f2, g_indexindex, h2, op);
      result=bdd_find(bddm, top_indexindex, temp1, temp2);
    }
  if (BDD_IS_OUTPOS(f))
    bdd_insert_in_cache2(bddm, op, f, h, result);
  else
    bdd_insert_in_cache2(bddm, op, BDD_NOT(f), h, BDD_NOT(result));
  return (result);
}


/* cmu_bdd_compose_temp is used internally by cmu_bdd_swap_vars. */

bdd
#if defined(__STDC__)
cmu_bdd_compose_temp(cmu_bdd_manager bddm, bdd f, bdd g, bdd h)
#else
cmu_bdd_compose_temp(bddm, f, g, h)
     cmu_bdd_manager bddm;
     bdd f, g, h;
#endif
{
  BDD_SETUP(g);
  return (cmu_bdd_compose_step(bddm, f, BDD_INDEXINDEX(g), h, OP_COMP+BDD_INDEXINDEX(g)));
}


/* cmu_bdd_compose(bddm, f, g, h) returns the BDD for substituting h for */
/* the variable g in f.  h may depend on g. */

bdd
#if defined(__STDC__)
cmu_bdd_compose(cmu_bdd_manager bddm, bdd f, bdd g, bdd h)
#else
cmu_bdd_compose(bddm, f, g, h)
     cmu_bdd_manager bddm;
     bdd f, g, h;
#endif
{
  if (bdd_check_arguments(3, f, g, h))
    {
      BDD_SETUP(f);
      BDD_SETUP(g);
      if (cmu_bdd_type_aux(bddm, g) != BDD_TYPE_POSVAR)
	{
	  cmu_bdd_warning("cmu_bdd_compose: second argument is not a positive variable");
	  BDD_INCREFS(f);
	  return (f);
	}
      FIREWALL(bddm);
      RETURN_BDD(cmu_bdd_compose_step(bddm, f, BDD_INDEXINDEX(g), h, OP_COMP+BDD_INDEXINDEX(g)));
    }
  return ((bdd)0);
}


bdd
#if defined(__STDC__)
cmu_bdd_substitute_step(cmu_bdd_manager bddm, bdd f, long op, bdd (*ite_fn)(cmu_bdd_manager, bdd, bdd, bdd), var_assoc subst)
#else
cmu_bdd_substitute_step(bddm, f, op, ite_fn, subst)
     cmu_bdd_manager bddm;
     bdd f;
     long op;
     bdd (*ite_fn)();
     var_assoc subst;
#endif
{
  bdd g;
  bdd temp1, temp2;
  bdd result;
  bdd_index_type g_index;

  BDD_SETUP(f);
  if ((long)BDD_INDEX(bddm, f) > subst->last)
    {
      BDD_TEMP_INCREFS(f);
      return (f);
    }
  if (bdd_lookup_in_cache1(bddm, op, BDD_OUTPOS(f), &result))
    return (BDD_IS_OUTPOS(f) ? result : BDD_NOT(result));
  g=subst->assoc[BDD_INDEXINDEX(f)];
  /* See if we are substituting a constant at this level. */
  if (g == BDD_ONE(bddm))
    return (cmu_bdd_substitute_step(bddm, BDD_THEN(f), op, ite_fn, subst));
  if (g == BDD_ZERO(bddm))
    return (cmu_bdd_substitute_step(bddm, BDD_ELSE(f), op, ite_fn, subst));
  temp1=cmu_bdd_substitute_step(bddm, BDD_THEN(f), op, ite_fn, subst);
  temp2=cmu_bdd_substitute_step(bddm, BDD_ELSE(f), op, ite_fn, subst);
  if (!g)
    g=BDD_IF(bddm, f);
  {
    BDD_SETUP(g);
    BDD_SETUP(temp1);
    BDD_SETUP(temp2);
    if (g == BDD_IF(bddm, g) &&
	(g_index=BDD_INDEX(bddm, g)) < BDD_INDEX(bddm, temp1) &&
	g_index < BDD_INDEX(bddm, temp2))
      /* Substituting with variable above the tops of the subresults. */
      result=bdd_find(bddm, BDD_INDEXINDEX(g), temp1, temp2);
    else
      {
	/* Do an ITE. */
	bddm->op_cache.cache_level++;
	result=(*ite_fn)(bddm, g, temp1, temp2);
	BDD_TEMP_DECREFS(temp1);
	BDD_TEMP_DECREFS(temp2);
	bddm->op_cache.cache_level--;
      }
  }
  if (BDD_IS_OUTPOS(f))
    bdd_insert_in_cache1(bddm, op, f, result);
  else
    bdd_insert_in_cache1(bddm, op, BDD_NOT(f), BDD_NOT(result));
  return (result);
}


/* cmu_bdd_substitute(bddm, f) returns the BDD for substituting in f */
/* according to the current variable association. */

bdd
#if defined(__STDC__)
cmu_bdd_substitute(cmu_bdd_manager bddm, bdd f)
#else
cmu_bdd_substitute(bddm, f)
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
      RETURN_BDD(cmu_bdd_substitute_step(bddm, f, op, cmu_bdd_ite_step, bddm->curr_assoc));
    }
  return ((bdd)0);
}
