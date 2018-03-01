/* Support functions need by miscellaneous BDD routines */


#include "bddint.h"


void
#if defined(__STDC__)
bdd_mark_shared_nodes(cmu_bdd_manager bddm, bdd f)
#else
bdd_mark_shared_nodes(bddm, f)
     cmu_bdd_manager bddm;
     bdd f;
#endif
{
  BDD_SETUP(f);
  f=BDD_OUTPOS(f);
  if (BDD_IS_CONST(f) || cmu_bdd_type_aux(bddm, f) == BDD_TYPE_POSVAR)
    return;
  if (BDD_MARK(f))
    {
      if (BDD_MARK(f) == 1)
	BDD_MARK(f)=2;
      return;
    }
  BDD_MARK(f)=1;
  bdd_mark_shared_nodes(bddm, BDD_THEN(f));
  bdd_mark_shared_nodes(bddm, BDD_ELSE(f));
}


void
#if defined(__STDC__)
bdd_number_shared_nodes(cmu_bdd_manager bddm, bdd f, hash_table h, long *next)
#else
bdd_number_shared_nodes(bddm, f, h, next)
     cmu_bdd_manager bddm;
     bdd f;
     hash_table h;
     long *next;
#endif
{
  BDD_SETUP(f);
  if (BDD_IS_CONST(f) || ((1 << cmu_bdd_type_aux(bddm, f)) & ((1 << BDD_TYPE_POSVAR) | (1 << BDD_TYPE_NEGVAR))))
    return;
  if (BDD_MARK(f) == 0)
    return;
  if (BDD_MARK(f) == 2)
    {
      bdd_insert_in_hash_table(h, f, (pointer)next);
      ++*next;
    }
  BDD_MARK(f)=0;
  bdd_number_shared_nodes(bddm, BDD_THEN(f), h, next);
  bdd_number_shared_nodes(bddm, BDD_ELSE(f), h, next);
}


static char default_terminal_id[]="terminal XXXXXXXXXX XXXXXXXXXX";
static char default_var_name[]="var.XXXXXXXXXX";


char *
#if defined(__STDC__)
bdd_terminal_id(cmu_bdd_manager bddm, bdd f, char *(*terminal_id_fn)(cmu_bdd_manager, INT_PTR, INT_PTR, pointer), pointer env)
#else
bdd_terminal_id(bddm, f, terminal_id_fn, env)
     cmu_bdd_manager bddm;
     bdd f;
     char *(*terminal_id_fn)();
     pointer env;
#endif
{
  char *id;
  INT_PTR v1, v2;

  cmu_mtbdd_terminal_value_aux(bddm, f, &v1, &v2);
  if (terminal_id_fn)
    id=(*terminal_id_fn)(bddm, v1, v2, env);
  else
    id=0;
  if (!id)
    {
      if (f == BDD_ONE(bddm))
	return ("1");
      if (f == BDD_ZERO(bddm))
	return ("0");
      sprintf(default_terminal_id, "terminal %ld %ld", (long)v1, (long)v2);
      id=default_terminal_id;
    }
  return (id);
}


char *
#if defined(__STDC__)
bdd_var_name(cmu_bdd_manager bddm, bdd v, char *(*var_naming_fn)(cmu_bdd_manager, bdd, pointer), pointer env)
#else
bdd_var_name(bddm, v, var_naming_fn, env)
     cmu_bdd_manager bddm;
     bdd v;
     char *(*var_naming_fn)();
     pointer env;
#endif
{
  char *name;

  if (var_naming_fn)
    name=(*var_naming_fn)(bddm, v, env);
  else
    name=0;
  if (!name)
    {
      BDD_SETUP(v);
      sprintf(default_var_name, "var.%d", BDD_INDEX(bddm, v));
      name=default_var_name;
    }
  return (name);
}


void
#if defined(__STDC__)
cmu_mtbdd_terminal_value_aux(cmu_bdd_manager bddm, bdd f, INT_PTR *value1, INT_PTR *value2)
#else
cmu_mtbdd_terminal_value_aux(bddm, f, value1, value2)
     cmu_bdd_manager bddm;
     bdd f;
     INT_PTR *value1;
     INT_PTR *value2;
#endif
{
  BDD_SETUP(f);
  if (BDD_IS_OUTPOS(f))
    {
      *value1=BDD_DATA0(f);
      *value2=BDD_DATA1(f);
    }
  else
    (*bddm->transform_fn)(bddm, BDD_DATA0(f), BDD_DATA1(f), value1, value2, bddm->transform_env);
}
