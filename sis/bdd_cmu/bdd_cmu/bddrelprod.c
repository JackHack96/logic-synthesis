/* BDD relational product routine */


#include "bddint.h"


static
bdd
#if defined(__STDC__)
cmu_bdd_rel_prod_step(cmu_bdd_manager bddm, bdd f, bdd g, long op, var_assoc vars)
#else
cmu_bdd_rel_prod_step(bddm, f, g, op, vars)
     cmu_bdd_manager bddm;
     bdd f;
     bdd g;
     long op;
     var_assoc vars;
#endif
{
  bdd_indexindex_type top_indexindex;
  bdd f1, f2;
  bdd g1, g2;
  bdd temp1, temp2;
  bdd result;
  int quantifying;

  BDD_SETUP(f);
  BDD_SETUP(g);
  if (BDD_IS_CONST(f) || BDD_IS_CONST(g))
    {
      if (f == BDD_ZERO(bddm) || g == BDD_ZERO(bddm))
	return (BDD_ZERO(bddm));
      if (f == BDD_ONE(bddm))
	return (cmu_bdd_exists_temp(bddm, g, op-1));
      return (cmu_bdd_exists_temp(bddm, f, op-1));
    }
  if ((long)BDD_INDEX(bddm, f) > vars->last && (long)BDD_INDEX(bddm, g) > vars->last)
    return (cmu_bdd_ite_step(bddm, f, g, BDD_ZERO(bddm)));
  /* Put in canonical order. */
  if (BDD_OUT_OF_ORDER(f, g))
    BDD_SWAP(f, g);
  if (bdd_lookup_in_cache2(bddm, op, f, g, &result))
    return (result);
  BDD_TOP_VAR2(top_indexindex, bddm, f, g);
  quantifying=(vars->assoc[top_indexindex] != 0);
  BDD_COFACTOR(top_indexindex, f, f1, f2);
  BDD_COFACTOR(top_indexindex, g, g1, g2);
  temp1=cmu_bdd_rel_prod_step(bddm, f1, g1, op, vars);
  if (quantifying && temp1 == BDD_ONE(bddm))
    result=temp1;
  else
    {
      temp2=cmu_bdd_rel_prod_step(bddm, f2, g2, op, vars);
      if (quantifying)
	{
	  BDD_SETUP(temp1);
	  BDD_SETUP(temp2);
	  bddm->op_cache.cache_level++;
	  result=cmu_bdd_ite_step(bddm, temp1, BDD_ONE(bddm), temp2);
	  BDD_TEMP_DECREFS(temp1);
	  BDD_TEMP_DECREFS(temp2);
	  bddm->op_cache.cache_level--;
	}
      else
	result=bdd_find(bddm, top_indexindex, temp1, temp2);
    }
  bdd_insert_in_cache2(bddm, op, f, g, result);
  return (result);
}


/* cmu_bdd_rel_prod(bddm, f, g) returns the BDD for "f and g" with those */
/* variables which are associated with something in the current */
/* variable association quantified out. */

bdd
#if defined(__STDC__)
cmu_bdd_rel_prod(cmu_bdd_manager bddm, bdd f, bdd g)
#else
cmu_bdd_rel_prod(bddm, f, g)
     cmu_bdd_manager bddm;
     bdd f;
     bdd g;
#endif
{
  long op;

  if (bdd_check_arguments(2, f, g))
    {
      FIREWALL(bddm);
      if (bddm->curr_assoc_id == -1)
	{
	  op=bddm->temp_op--;
	  /* We decrement the temporary opcode once more because */
	  /* cmu_bdd_rel_prod may call cmu_bdd_exists_temp, and we don't */
	  /* want to generate new temporary opcodes for each such */
	  /* call.  Instead, we pass op-1 to cmu_bdd_exists_temp, and */
	  /* have it use this opcode for caching. */
	  bddm->temp_op--;
	}
      else
	op=OP_RELPROD+bddm->curr_assoc_id;
      RETURN_BDD(cmu_bdd_rel_prod_step(bddm, f, g, op, bddm->curr_assoc));
    }
  return ((bdd)0);
}
