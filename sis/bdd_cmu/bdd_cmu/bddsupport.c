/* BDD support routines */


#include "bddint.h"


static
int
#if defined(__STDC__)
cmu_bdd_depends_on_step(cmu_bdd_manager bddm, bdd f, bdd_index_type var_index, int mark)
#else
cmu_bdd_depends_on_step(bddm, f, var_index, mark)
     cmu_bdd_manager bddm;
     bdd f;
     bdd_index_type var_index;
     int mark;
#endif
{
  bdd_index_type f_index;

  BDD_SETUP(f);
  f_index=BDD_INDEX(bddm, f);
  if (f_index > var_index)
    return (0);
  if (f_index == var_index)
    return (1);
  if (BDD_MARK(f) == mark)
    return (0);
  BDD_MARK(f)=mark;
  if (cmu_bdd_depends_on_step(bddm, BDD_THEN(f), var_index, mark))
    return (1);
  return (cmu_bdd_depends_on_step(bddm, BDD_ELSE(f), var_index, mark));
}


/* cmu_bdd_depends_on(bddm, f, var) returns 1 if f depends on var and */
/* returns 0 otherwise. */

int
#if defined(__STDC__)
cmu_bdd_depends_on(cmu_bdd_manager bddm, bdd f, bdd var)
#else
cmu_bdd_depends_on(bddm, f, var)
     cmu_bdd_manager bddm;
     bdd f;
     bdd var;
#endif
{
  if (bdd_check_arguments(2, f, var))
    {
      BDD_SETUP(var);
      if (cmu_bdd_type_aux(bddm, var) != BDD_TYPE_POSVAR)
	{
	  cmu_bdd_warning("cmu_bdd_depends_on: second argument is not a positive variable");
	  if (BDD_IS_CONST(var))
	    return (1);
	}
      (void)cmu_bdd_depends_on_step(bddm, f, BDD_INDEX(bddm, var), 1);
      return (cmu_bdd_depends_on_step(bddm, f, BDD_INDEX(bddm, var), 0));
    }
  return (0);
}


static
void
#if defined(__STDC__)
bdd_unmark_nodes(cmu_bdd_manager bddm, bdd f)
#else
bdd_unmark_nodes(bddm, f)
     cmu_bdd_manager bddm;
     bdd f;
#endif
{
  bdd temp;

  BDD_SETUP(f);
  if (!BDD_MARK(f) || BDD_IS_CONST(f))
    return;
  BDD_MARK(f)=0;
  temp=BDD_IF(bddm, f);
  {
    BDD_SETUP(temp);
    BDD_MARK(temp)=0;
  }
  bdd_unmark_nodes(bddm, BDD_THEN(f));
  bdd_unmark_nodes(bddm, BDD_ELSE(f));
}


static
bdd *
#if defined(__STDC__)
cmu_bdd_support_step(cmu_bdd_manager bddm, bdd f, bdd *support)
#else
cmu_bdd_support_step(bddm, f, support)
     cmu_bdd_manager bddm;
     bdd f;
     bdd *support;
#endif
{
  bdd temp;

  BDD_SETUP(f);
  if (BDD_MARK(f) || BDD_IS_CONST(f))
    return (support);
  temp=BDD_IF(bddm, f);
  {
    BDD_SETUP(temp);
    if (!BDD_MARK(temp))
      {
	BDD_MARK(temp)=1;
	*support=temp;
	++support;
      }
  }
  BDD_MARK(f)=1;
  support=cmu_bdd_support_step(bddm, BDD_THEN(f), support);
  return (cmu_bdd_support_step(bddm, BDD_ELSE(f), support));
}


/* cmu_bdd_support(bddm, f, support) returns the support of f as a */
/* null-terminated array of variables. */

void
#if defined(__STDC__)
cmu_bdd_support(cmu_bdd_manager bddm, bdd f, bdd *support)
#else
cmu_bdd_support(bddm, f, support)
     cmu_bdd_manager bddm;
     bdd f;
     bdd *support;
#endif
{
  bdd *end;

  if (bdd_check_arguments(1, f))
    {
      end=cmu_bdd_support_step(bddm, f, support);
      *end=0;
      bdd_unmark_nodes(bddm, f);
    }
  else
    *support=0;
}
