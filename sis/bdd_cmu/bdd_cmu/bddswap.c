/* BDD variable exchange routine */


#include "bddint.h"


static
bdd
#if defined(__STDC__)
cmu_bdd_swap_vars_aux_step(cmu_bdd_manager bddm, bdd f, bdd g, bdd h, bdd_indexindex_type h_indexindex, long op)
#else
cmu_bdd_swap_vars_aux_step(bddm, f, g, h, h_indexindex, op)
     cmu_bdd_manager bddm;
     bdd f;
     bdd g;
     bdd h;
     bdd_indexindex_type h_indexindex;
     long op;
#endif
{
  bdd_indexindex_type top_indexindex;
  bdd_index_type f_index, g_index;
  bdd f1, f2;
  bdd g1, g2;
  bdd temp1, temp2;
  bdd result;

  BDD_SETUP(f);
  BDD_SETUP(g);
  f_index=BDD_INDEX(bddm, f);
  g_index=BDD_INDEX(bddm, g);
  if (f_index == bddm->indexes[h_indexindex])
    {
      if (op & 1)
	f=BDD_THEN(f);
      else
	f=BDD_ELSE(f);
      BDD_RESET(f);
    }
  if (g_index == bddm->indexes[h_indexindex])
    {
      if (op & 1)
	g=BDD_THEN(g);
      else
	g=BDD_ELSE(g);
      BDD_RESET(g);
    }
  if (f == g)
    if (op & 1)
      return (cmu_bdd_compose_temp(bddm, f, h, BDD_ONE(bddm)));
    else
      return (cmu_bdd_compose_temp(bddm, f, h, BDD_ZERO(bddm)));
  if (f_index >= bddm->indexes[h_indexindex] && g_index >= bddm->indexes[h_indexindex])
    {
      BDD_TEMP_INCREFS(f);
      BDD_TEMP_INCREFS(g);
      return (bdd_find(bddm, h_indexindex, f, g));
    }
  if (bdd_lookup_in_cache2(bddm, op, f, g, &result))
    return (result);
  BDD_TOP_VAR2(top_indexindex, bddm, f, g);
  BDD_COFACTOR(top_indexindex, f, f1, f2);
  BDD_COFACTOR(top_indexindex, g, g1, g2);
  temp1=cmu_bdd_swap_vars_aux_step(bddm, f1, g1, h, h_indexindex, op);
  temp2=cmu_bdd_swap_vars_aux_step(bddm, f2, g2, h, h_indexindex, op);
  result=bdd_find(bddm, top_indexindex, temp1, temp2);
  bdd_insert_in_cache2(bddm, op, f, g, result);
  return (result);
}


static
bdd
#if defined(__STDC__)
cmu_bdd_swap_vars_step(cmu_bdd_manager bddm, bdd f, bdd_indexindex_type g_indexindex, bdd h, long op)
#else
cmu_bdd_swap_vars_step(bddm, f, g_indexindex, h, op)
     cmu_bdd_manager bddm;
     bdd f;
     bdd_indexindex_type g_indexindex;
     bdd h;
     long op;
#endif
{
  bdd_index_type f_index;
  bdd temp1, temp2;
  bdd result;

  BDD_SETUP(f);
  if (BDD_IS_CONST(f))
    return (f);
  if (bdd_lookup_in_cache2(bddm, op, f, h, &result))
    return (result);
  f_index=BDD_INDEX(bddm, f);
  if (f_index > bddm->indexes[g_indexindex])
    {
      temp1=cmu_bdd_compose_temp(bddm, f, h, BDD_ONE(bddm));
      temp2=cmu_bdd_compose_temp(bddm, f, h, BDD_ZERO(bddm));
      result=bdd_find(bddm, g_indexindex, temp1, temp2);
    }
  else if (f_index < bddm->indexes[g_indexindex])
    {
      temp1=cmu_bdd_swap_vars_step(bddm, BDD_THEN(f), g_indexindex, h, op);
      temp2=cmu_bdd_swap_vars_step(bddm, BDD_ELSE(f), g_indexindex, h, op);
      result=bdd_find(bddm, BDD_INDEXINDEX(f), temp1, temp2);
    }
  else
    {
      BDD_SETUP(h);
      temp1=cmu_bdd_swap_vars_aux_step(bddm, BDD_THEN(f), BDD_ELSE(f), h, BDD_INDEXINDEX(h), OP_SWAPAUX+2*BDD_INDEXINDEX(h)+1);
      temp2=cmu_bdd_swap_vars_aux_step(bddm, BDD_THEN(f), BDD_ELSE(f), h, BDD_INDEXINDEX(h), OP_SWAPAUX+2*BDD_INDEXINDEX(h));
      result=bdd_find(bddm, g_indexindex, temp1, temp2);
    }
  bdd_insert_in_cache2(bddm, op, f, h, result);
  return (result);
}


bdd
#if defined(__STDC__)
cmu_bdd_swap_vars_temp(cmu_bdd_manager bddm, bdd f, bdd g, bdd h)
#else
cmu_bdd_swap_vars_temp(bddm, f, g, h)
     cmu_bdd_manager bddm;
     bdd f;
     bdd g;
     bdd h;
#endif
{
  bdd_index_type g_index, h_index;

  BDD_SETUP(f);
  BDD_SETUP(g);
  BDD_SETUP(h);
  g_index=BDD_INDEX(bddm, g);
  h_index=BDD_INDEX(bddm, h);
  if (g_index == h_index)
    {
      BDD_TEMP_INCREFS(f);
      return (f);
    }
  if (g_index > h_index)
    return (cmu_bdd_swap_vars_step(bddm, f, BDD_INDEXINDEX(h), g, OP_SWAP+BDD_INDEXINDEX(h)));
  else
    return (cmu_bdd_swap_vars_step(bddm, f, BDD_INDEXINDEX(g), h, OP_SWAP+BDD_INDEXINDEX(g)));
}


/* cmu_bdd_swap_vars(bddm, f, g, h) substitutes g for h and h for g in f. */

bdd
#if defined(__STDC__)
cmu_bdd_swap_vars(cmu_bdd_manager bddm, bdd f, bdd g, bdd h)
#else
cmu_bdd_swap_vars(bddm, f, g, h)
     cmu_bdd_manager bddm;
     bdd f;
     bdd g;
     bdd h;
#endif
{
  bdd_index_type g_index, h_index;

  if (bdd_check_arguments(3, f, g, h))
    {
      BDD_SETUP(f);
      BDD_SETUP(g);
      BDD_SETUP(h);
      if (cmu_bdd_type_aux(bddm, g) != BDD_TYPE_POSVAR || cmu_bdd_type_aux(bddm, h) != BDD_TYPE_POSVAR)
	{
	  cmu_bdd_warning("cmu_bdd_swap_vars: second and third arguments are not both positive variables");
	  BDD_INCREFS(f);
	  return (f);
	}
      FIREWALL(bddm);
      g_index=BDD_INDEX(bddm, g);
      h_index=BDD_INDEX(bddm, h);
      if (g_index == h_index)
	{
	  BDD_INCREFS(f);
	  return (f);
	}
      if (g_index > h_index)
	RETURN_BDD(cmu_bdd_swap_vars_step(bddm, f, BDD_INDEXINDEX(h), g, OP_SWAP+BDD_INDEXINDEX(h)));
      else
	RETURN_BDD(cmu_bdd_swap_vars_step(bddm, f, BDD_INDEXINDEX(g), h, OP_SWAP+BDD_INDEXINDEX(g)));
    }
  return ((bdd)0);
}
