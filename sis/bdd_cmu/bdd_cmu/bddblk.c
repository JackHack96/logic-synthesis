/* BDD variable block routines */


#include "bddint.h"


static
void
#if defined(__STDC__)
add_block(block b1, block b2)
#else
add_block(b1, b2)
     block b1;
     block b2;
#endif
{
  long i, j, k;
  block start, end;

  if (b1->num_children)
    {
      i=bdd_find_block(b1, b2->first_index);
      start=b1->children[i];
      j=bdd_find_block(b1, b2->last_index);
      end=b1->children[j];
      if (i == j)
	add_block(start, b2);
      else
	{
	  if (start->first_index != b2->first_index || end->last_index != b2->last_index)
	    cmu_bdd_fatal("add_block: illegal block overlap");
	  b2->num_children=j-i+1;
	  b2->children=(block *)mem_get_block((SIZE_T)(sizeof(block)*b2->num_children));
	  for (k=0; k < b2->num_children; ++k)
	    b2->children[k]=b1->children[i+k];
	  b1->children[i]=b2;
	  ++i;
	  for (k=j+1; k < b1->num_children; ++k, ++i)
	    b1->children[i]=b1->children[k];
	  b1->num_children-=(b2->num_children-1);
	  b1->children=(block *)mem_resize_block((pointer)b1->children, (SIZE_T)(sizeof(block)*b1->num_children));
	}
    }
  else
    {
      /* b1 and b2 are blocks representing just single variables. */
      b1->num_children=1;
      b1->children=(block *)mem_get_block((SIZE_T)(sizeof(block)*b1->num_children));
      b1->children[0]=b2;
      b2->num_children=0;
      b2->children=0;
    }
}


block
#if defined(__STDC__)
cmu_bdd_new_var_block(cmu_bdd_manager bddm, bdd v, long n)
#else
cmu_bdd_new_var_block(bddm, v, n)
     cmu_bdd_manager bddm;
     bdd v;
     long n;
#endif
{
  block b;

  if (bdd_check_arguments(1, v))
    {
      BDD_SETUP(v);
      if (cmu_bdd_type_aux(bddm, v) != BDD_TYPE_POSVAR)
	{
	  cmu_bdd_warning("cmu_bdd_new_var_block: second argument is not a positive variable");
	  if (BDD_IS_CONST(v))
	    return ((block)0);
	}
      b=(block)BDD_NEW_REC(bddm, sizeof(struct block_));
      b->reorderable=0;
      b->first_index=BDD_INDEX(bddm, v);
      if (n <= 0)
	{
	  cmu_bdd_warning("cmu_bdd_new_var_block: invalid final argument");
	  n=1;
	}
      b->last_index=b->first_index+n-1;
      if (b->last_index >= bddm->vars)
	{
	  cmu_bdd_warning("cmu_bdd_new_var_block: range covers non-existent variables");
	  b->last_index=bddm->vars-1;
	}
      add_block(bddm->super_block, b);
      return (b);
    }
  return ((block)0);
}
