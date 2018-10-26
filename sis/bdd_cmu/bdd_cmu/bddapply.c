/* BDD generic apply routines */


#include "bddint.h"


static
bdd
#if defined(__STDC__)
bdd_apply2_step(cmu_bdd_manager bddm,
		bdd (*terminal_fn)(cmu_bdd_manager, bdd *, bdd *, pointer),
		long op,
		bdd f,
		bdd g,
		pointer env)
#else
bdd_apply2_step(bddm, terminal_fn, op, f, g, env)
     cmu_bdd_manager bddm;
     bdd (*terminal_fn)();
     long op;
     bdd f;
     bdd g;
     pointer env;
#endif
{
  bdd_indexindex_type top_indexindex;
  bdd f1, f2;
  bdd g1, g2;
  bdd temp1, temp2;
  bdd result;

  if ((result=(*terminal_fn)(bddm, &f, &g, env)))
    {
      BDD_SETUP(result);
      BDD_TEMP_INCREFS(result);
      return (result);
    }
  if (bdd_lookup_in_cache2(bddm, op, f, g, &result))
    return (result);
  {
    BDD_SETUP(f);
    BDD_SETUP(g);
    BDD_TOP_VAR2(top_indexindex, bddm, f, g);
    BDD_COFACTOR(top_indexindex, f, f1, f2);
    BDD_COFACTOR(top_indexindex, g, g1, g2);
    temp1=bdd_apply2_step(bddm, terminal_fn, op, f1, g1, env);
    temp2=bdd_apply2_step(bddm, terminal_fn, op, f2, g2, env);
    result=bdd_find(bddm, top_indexindex, temp1, temp2);
    bdd_insert_in_cache2(bddm, op, f, g, result);
    return (result);
  }
}


bdd
#if defined(__STDC__)
bdd_apply2(cmu_bdd_manager bddm, bdd (*terminal_fn)(cmu_bdd_manager, bdd *, bdd *, pointer), bdd f, bdd g, pointer env)
#else
bdd_apply2(bddm, terminal_fn, f, g, env)
     cmu_bdd_manager bddm;
     bdd (*terminal_fn)();
     bdd f;
     bdd g;
     pointer env;
#endif
{
  long op;

  if (bdd_check_arguments(2, f, g))
    {
      FIREWALL(bddm);
      op=bddm->temp_op--;
      RETURN_BDD(bdd_apply2_step(bddm, terminal_fn, op, f, g, env));
    }
  return ((bdd)0);
}


static
bdd
#if defined(__STDC__)
bdd_apply1_step(cmu_bdd_manager bddm, bdd (*terminal_fn)(cmu_bdd_manager, bdd *, pointer), long op, bdd f, pointer env)
#else
bdd_apply1_step(bddm, terminal_fn, op, f, env)
     cmu_bdd_manager bddm;
     bdd (*terminal_fn)();
     long op;
     bdd f;
     pointer env;
#endif
{
  bdd temp1, temp2;
  bdd result;

  if ((result=(*terminal_fn)(bddm, &f, env)))
    {
      BDD_SETUP(result);
      BDD_TEMP_INCREFS(result);
      return (result);
    }
  if (bdd_lookup_in_cache1(bddm, op, f, &result))
    return (result);
  {
    BDD_SETUP(f);
    temp1=bdd_apply1_step(bddm, terminal_fn, op, BDD_THEN(f), env);
    temp2=bdd_apply1_step(bddm, terminal_fn, op, BDD_ELSE(f), env);
    result=bdd_find(bddm, BDD_INDEXINDEX(f), temp1, temp2);
    bdd_insert_in_cache1(bddm, op, f, result);
    return (result);
  }
}


bdd
#if defined(__STDC__)
bdd_apply1(cmu_bdd_manager bddm, bdd (*terminal_fn)(cmu_bdd_manager, bdd *, pointer), bdd f, pointer env)
#else
bdd_apply1(bddm, terminal_fn, f, g, env)
     cmu_bdd_manager bddm;
     bdd (*terminal_fn)();
     bdd f;
     pointer env;
#endif
{
  long op;

  if (bdd_check_arguments(1, f))
    {
      FIREWALL(bddm);
      op=bddm->temp_op--;
      RETURN_BDD(bdd_apply1_step(bddm, terminal_fn, op, f, env));
    }
  return ((bdd)0);
}
