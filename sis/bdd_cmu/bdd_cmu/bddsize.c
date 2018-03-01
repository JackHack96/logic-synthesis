/* BDD size and profile routines */


#include "bddint.h"


static
void
#if defined(__STDC__)
bdd_mark_bdd(bdd f)
#else
bdd_mark_bdd(f)
     bdd f;
#endif
{
  int curr_marking, this_marking;

  BDD_SETUP(f);
  curr_marking=BDD_MARK(f);
  this_marking=(1 << TAG(f));
  if (curr_marking & this_marking)
    return;
  BDD_MARK(f)=curr_marking | this_marking;
  if (BDD_IS_CONST(f))
    return;
  bdd_mark_bdd(BDD_THEN(f));
  bdd_mark_bdd(BDD_ELSE(f));
}


static
int
#if defined(__STDC__)
bdd_count_no_nodes(bdd f)
#else
bdd_count_no_nodes(f)
     bdd f;
#endif
{
  BDD_SETUP(f);
  return (BDD_MARK(f) > 0);
}


static
int
#if defined(__STDC__)
bdd_count_nodes(bdd f)
#else
bdd_count_nodes(f)
     bdd f;
#endif
{
  int mark;

  BDD_SETUP(f);
  mark=BDD_MARK(f);
  return (((mark & 0x1) != 0)+((mark & 0x2) != 0));
}


static
#if defined(__STDC__)
int (*(counting_fns[]))(bdd)=
#else
int (*(counting_fns[]))()=
#endif
{
  bdd_count_no_nodes,
  bdd_count_nodes,
};


static
long
#if defined(__STDC__)
cmu_bdd_size_step(bdd f, int (*count_fn)(bdd))
#else
cmu_bdd_size_step(f, count_fn)
     bdd f;
     int (*count_fn)();
#endif
{
  long result;

  BDD_SETUP(f);
  if (!BDD_MARK(f))
    return (0l);
  result=(*count_fn)(f);
  if (!BDD_IS_CONST(f))
    result+=cmu_bdd_size_step(BDD_THEN(f), count_fn)+cmu_bdd_size_step(BDD_ELSE(f), count_fn);
  BDD_MARK(f)=0;
  return (result);
}


/* cmu_bdd_size(bddm, f, negout) returns the number of nodes in f when */
/* negout is nonzero.  If negout is zero, we pretend that the BDDs */
/* don't have negative-output pointers. */

long
#if defined(__STDC__)
cmu_bdd_size(cmu_bdd_manager bddm, bdd f, int negout)
#else
cmu_bdd_size(bddm, f, negout)
     cmu_bdd_manager bddm;
     bdd f;
     int negout;
#endif
{
  bdd g;

  if (bdd_check_arguments(1, f))
    {
      g=BDD_ONE(bddm);
      {
	BDD_SETUP(g);
	BDD_MARK(g)=0;
      }
      bdd_mark_bdd(f);
      return (cmu_bdd_size_step(f, counting_fns[!negout]));
    }
  return (0l);
}


/* cmu_bdd_size_multiple is like cmu_bdd_size, but takes a null-terminated */
/* array of BDDs and accounts for sharing of nodes. */

long
#if defined(__STDC__)
cmu_bdd_size_multiple(cmu_bdd_manager bddm, bdd* fs, int negout)
#else
cmu_bdd_size_multiple(bddm, fs, negout)
     cmu_bdd_manager bddm;
     bdd *fs;
     int negout;
#endif
{
  long size;
  bdd *f;
  bdd g;

  bdd_check_array(fs);
  g=BDD_ONE(bddm);
  {
    BDD_SETUP(g);
    BDD_MARK(g)=0;
  }
  for (f=fs; *f; ++f)
    bdd_mark_bdd(*f);
  size=0l;
  for (f=fs; *f; ++f)
    size+=cmu_bdd_size_step(*f, counting_fns[!negout]);
  return (size);
}


static
void
#if defined(__STDC__)
cmu_bdd_profile_step(cmu_bdd_manager bddm, bdd f, long *level_counts, int (*count_fn)(bdd))
#else
cmu_bdd_profile_step(bddm, f, level_counts, count_fn)
     cmu_bdd_manager bddm;
     bdd f;
     long *level_counts;
     int (*count_fn)();
#endif
{
  BDD_SETUP(f);
  if (!BDD_MARK(f))
    return;
  if (BDD_IS_CONST(f))
    level_counts[bddm->vars]+=(*count_fn)(f);
  else
    {
      level_counts[BDD_INDEX(bddm, f)]+=(*count_fn)(f);
      cmu_bdd_profile_step(bddm, BDD_THEN(f), level_counts, count_fn);
      cmu_bdd_profile_step(bddm, BDD_ELSE(f), level_counts, count_fn);
    }
  BDD_MARK(f)=0;
}


/* cmu_bdd_profile(bddm, f, level_counts, negout) returns a "node profile" */
/* of f, i.e., the number of nodes at each level in f.  negout is as in */
/* cmu_bdd_size.  level_counts should be an array of size cmu_bdd_vars(bddm)+1 */
/* to hold the profile. */

void
#if defined(__STDC__)
cmu_bdd_profile(cmu_bdd_manager bddm, bdd f, long *level_counts, int negout)
#else
cmu_bdd_profile(bddm, f, level_counts, negout)
     cmu_bdd_manager bddm;
     bdd f;
     long *level_counts;
     int negout;
#endif
{
  bdd_index_type i;
  bdd g;

  for (i=0; i <= bddm->vars; ++i)
    level_counts[i]=0l;
  if (bdd_check_arguments(1, f))
    {
      g=BDD_ONE(bddm);
      {
	BDD_SETUP(g);
	BDD_MARK(g)=0;
      }
      bdd_mark_bdd(f);
      cmu_bdd_profile_step(bddm, f, level_counts, counting_fns[!negout]);
    }
}


/* cmu_bdd_profile_multiple is to cmu_bdd_profile as cmu_bdd_size_multiple is to */
/* cmu_bdd_size. */

void
#if defined(__STDC__)
cmu_bdd_profile_multiple(cmu_bdd_manager bddm, bdd *fs, long *level_counts, int negout)
#else
cmu_bdd_profile_multiple(bddm, fs, level_counts, negout)
     cmu_bdd_manager bddm;
     bdd *fs;
     long *level_counts;
     int negout;
#endif
{
  bdd_index_type i;
  bdd *f;
  bdd g;

  bdd_check_array(fs);
  for (i=0; i <= bddm->vars; ++i)
    level_counts[i]=0l;
  g=BDD_ONE(bddm);
  {
    BDD_SETUP(g);
    BDD_MARK(g)=0;
  }
  for (f=fs; *f; ++f)
    bdd_mark_bdd(*f);
  for (f=fs; *f; ++f)
    cmu_bdd_profile_step(bddm, *f, level_counts, counting_fns[!negout]);
}


static
void
#if defined(__STDC__)
bdd_highest_ref_step(cmu_bdd_manager bddm, bdd f, hash_table h)
#else
bdd_highest_ref_step(bddm, f, h)
     cmu_bdd_manager bddm;
     bdd f;
     hash_table h;
#endif
{
  long *hash_result;
  long f_index;

  BDD_SETUP(f);
  if (BDD_IS_CONST(f))
    return;
  f_index=BDD_INDEX(bddm, f);
  if ((hash_result=(long *)bdd_lookup_in_hash_table(h, BDD_THEN(f))))
    {
      if (*hash_result > f_index)
	*hash_result=f_index;
    }
  else
    {
      bdd_insert_in_hash_table(h, BDD_THEN(f), (pointer)&f_index);
      bdd_highest_ref_step(bddm, BDD_THEN(f), h);
    }
  if ((hash_result=(long *)bdd_lookup_in_hash_table(h, BDD_ELSE(f))))
    {
      if (*hash_result > f_index)
	*hash_result=f_index;
    }
  else
    {
      bdd_insert_in_hash_table(h, BDD_ELSE(f), (pointer)&f_index);
      bdd_highest_ref_step(bddm, BDD_ELSE(f), h);
    }
}


static
void
#if defined(__STDC__)
bdd_dominated_step(cmu_bdd_manager bddm, bdd f, long *func_counts, hash_table h)
#else
bdd_dominated_step(bddm, f, func_counts, h)
     cmu_bdd_manager bddm;
     bdd f;
     long *func_counts;
     hash_table h;
#endif
{
  long *hash_result;

  hash_result=(long *)bdd_lookup_in_hash_table(h, f);
  if (*hash_result >= 0)
    func_counts[*hash_result]-=2;
  if (*hash_result > -2)
    {
      BDD_SETUP(f);
      *hash_result= -2;
      if (!BDD_IS_CONST(f))
	{
	  bdd_dominated_step(bddm, BDD_THEN(f), func_counts, h);
	  bdd_dominated_step(bddm, BDD_ELSE(f), func_counts, h);
	}
    }
}


/* cmu_bdd_function_profile(bddm, f, func_counts) returns a "function */
/* profile" for f.  The nth entry of the function profile array is the */
/* number of subfunctions of f which may be obtained by restricting */
/* the variables whose index is less than n.  An entry of zero */
/* indicates that f is independent of the variable with the */
/* corresponding index. */

void
#if defined(__STDC__)
cmu_bdd_function_profile(cmu_bdd_manager bddm, bdd f, long *func_counts)
#else
cmu_bdd_function_profile(bddm, f, func_counts)
     cmu_bdd_manager bddm;
     bdd f;
     long *func_counts;
#endif
{
  long i;
  bdd_index_type j;
  hash_table h;

  /* The number of subfunctions obtainable by restricting the */
  /* variables of index < n is the number of subfunctions whose top */
  /* variable has index n plus the number of subfunctions obtainable */
  /* by restricting the variables of index < n+1 minus the number of */
  /* these latter subfunctions whose highest reference is by a node at */
  /* level n. */
  /* The strategy will be to start with the number of subfunctions */
  /* whose top variable has index n.  We compute the highest level at */
  /* which each subfunction is referenced.  Then we work bottom up; at */
  /* level n we add in the result from level n+1 and subtract the */
  /* number of subfunctions whose highest reference is at level n. */
  cmu_bdd_profile(bddm, f, func_counts, 0);
  if (bdd_check_arguments(1, f))
    {
      /* Encode the profile.  The low bit of a count will be zero for */
      /* those levels where f actually has a node. */
      for (j=0; j < bddm->vars; ++j)
	if (!func_counts[j])
	  func_counts[j]=1;
	else
	  func_counts[j]<<=1;
      h=bdd_new_hash_table(bddm, sizeof(long));
      /* For each subfunction in f, compute the highest level where it is */
      /* referenced.  f itself is conceptually referenced at the highest */
      /* possible level, which we represent by -1. */
      i= -1;
      bdd_insert_in_hash_table(h, f, (pointer)&i);
      bdd_highest_ref_step(bddm, f, h);
      /* Walk through these results.  For each subfunction, decrement the */
      /* count at the highest level where it is referenced. */
      bdd_dominated_step(bddm, f, func_counts, h);
      cmu_bdd_free_hash_table(h);
      /* Now add each level n+1 result to that of level n. */
      for (i=bddm->vars-1, j=i+1; i>= 0; --i)
	if (func_counts[i] != 1)
	  {
	    func_counts[i]=(func_counts[i] >> 1)+func_counts[j];
	    j=i;
	  }
	else
	  func_counts[i]=0;
    }
}


/* cmu_bdd_function_profile_multiple is to cmu_bdd_function_profile as */
/* cmu_bdd_size_multiple is to cmu_bdd_size. */

void
#if defined(__STDC__)
cmu_bdd_function_profile_multiple(cmu_bdd_manager bddm, bdd *fs, long *func_counts)
#else
cmu_bdd_function_profile_multiple(bddm, fs, func_counts)
     cmu_bdd_manager bddm;
     bdd *fs;
     long *func_counts;
#endif
{
  long i;
  bdd_index_type j;
  bdd *f;
  long *hash_result;
  hash_table h;

  bdd_check_array(fs);
  /* See cmu_bdd_function_profile for the strategy involved here. */
  cmu_bdd_profile_multiple(bddm, fs, func_counts, 0);
  for (j=0; j < bddm->vars; ++j)
    if (!func_counts[j])
      func_counts[j]=1;
    else
      func_counts[j]<<=1;
  h=bdd_new_hash_table(bddm, sizeof(long));
  for (f=fs; *f; ++f)
    bdd_highest_ref_step(bddm, *f, h);
  i= -1;
  for (f=fs; *f; ++f)
    if ((hash_result=(long *)bdd_lookup_in_hash_table(h, *f)))
      *hash_result= -1;
    else
      bdd_insert_in_hash_table(h, *f, (pointer)&i);
  for (f=fs; *f; ++f)
    bdd_dominated_step(bddm, *f, func_counts, h);
  cmu_bdd_free_hash_table(h);
  for (i=bddm->vars-1, j=i+1; i>= 0; --i)
    if (func_counts[i] != 1)
      {
	func_counts[i]=(func_counts[i] >> 1)+func_counts[j];
	j=i;
      }
    else
      func_counts[i]=0;
}
