/* BDD variable association routines */


#include "bddint.h"


static
int
#if defined(__STDC__)
cmu_bdd_assoc_eq(cmu_bdd_manager bddm, bdd *p, bdd *q)
#else
cmu_bdd_assoc_eq(bddm, p, q)
     cmu_bdd_manager bddm;
     bdd *p;
     bdd *q;
#endif
{
  bdd_indexindex_type i;

  for (i=0; i < bddm->maxvars; ++i)
    if (p[i+1] != q[i+1])
      return (0);
  return (1);
}


static
int
#if defined(__STDC__)
check_assoc(cmu_bdd_manager bddm, bdd *assoc_info, int pairs)
#else
check_assoc(bddm, assoc_info, pairs)
     cmu_bdd_manager bddm;
     bdd *assoc_info;
     int pairs;
#endif
{
  bdd_check_array(assoc_info);
  if (pairs)
    while (assoc_info[0] && assoc_info[1])
      {
	if (cmu_bdd_type_aux(bddm, assoc_info[0]) != BDD_TYPE_POSVAR)
	  {
	    cmu_bdd_warning("check_assoc: first element in pair is not a positive variable");
	    return (0);
	  }
	assoc_info+=2;
      }
  return (1);
}

  
/* cmu_bdd_new_assoc(bddm, assoc_info, pairs) creates or finds a variable */
/* association given by assoc_info.  pairs is 0 if the information */
/* represents only a list of variables rather than a full association. */

int
#if defined(__STDC__)
cmu_bdd_new_assoc(cmu_bdd_manager bddm, bdd *assoc_info, int pairs)
#else
cmu_bdd_new_assoc(bddm, assoc_info, pairs)
     cmu_bdd_manager bddm;
     bdd *assoc_info;
     int pairs;
#endif
{
  long i;
  assoc_list p, *q;
  bdd f;
  bdd *assoc;
  bdd_indexindex_type j;
  long last;

  if (!check_assoc(bddm, assoc_info, pairs))
    return (-1);
  assoc=(bdd *)mem_get_block((SIZE_T)((bddm->maxvars+1)*sizeof(bdd)));
  /* Unpack the association. */
  for (i=0; i < bddm->maxvars; ++i)
    assoc[i+1]=0;
  if (pairs)
    for (i=0; (f=assoc_info[i]) && assoc_info[i+1]; i+=2)
      {
	BDD_SETUP(f);
	assoc[BDD_INDEXINDEX(f)]=assoc_info[i+1];
      }
  else
    for (i=0; (f=assoc_info[i]); ++i)
      {
	BDD_SETUP(f);
	assoc[BDD_INDEXINDEX(f)]=BDD_ONE(bddm);
      }
  /* Check for existence. */
  for (p=bddm->assocs; p; p=p->next)
    if (cmu_bdd_assoc_eq(bddm, p->va.assoc, assoc))
      {
	mem_free_block((pointer)assoc);
	p->refs++;
	return (p->id);
      }
  /* Find the first unused id. */
  for (q= &bddm->assocs, p= *q, i=0; p && p->id == i; q= &p->next, p= *q, ++i);
  p=(assoc_list)BDD_NEW_REC(bddm, sizeof(struct assoc_list_));
  p->id=i;
  p->next= *q;
  *q=p;
  p->va.assoc=assoc;
  last= -1;
  if (pairs)
    for (i=0; (f=assoc_info[i]) && assoc_info[i+1]; i+=2)
      {
	BDD_SETUP(f);
	j=BDD_INDEXINDEX(f);
	if ((long)bddm->indexes[j] > last)
	  last=bddm->indexes[j];
      }
  else
    for (i=0; (f=assoc_info[i]); ++i)
      {
	BDD_SETUP(f);
	j=BDD_INDEXINDEX(f);
	if ((long)bddm->indexes[j] > last)
	  last=bddm->indexes[j];
      }
  p->va.last=last;
  p->refs=1;
  /* Protect BDDs in the association. */
  if (pairs)
    for (i=0; assoc_info[i] && (f=assoc_info[i+1]); i+=2)
      {
	BDD_SETUP(f);
	BDD_INCREFS(f);
      }
  return (p->id);
}


static
int
#if defined(__STDC__)
bdd_flush_id_entries(cmu_bdd_manager bddm, cache_entry p, pointer closure)
#else
bdd_flush_id_entries(bddm, p, closure)
     cmu_bdd_manager bddm;
     cache_entry p;
     pointer closure;
#endif
{
#if defined(__STDC__)
  int (*flush_fn)(cmu_bdd_manager, cache_entry, pointer);
#else
  int (*flush_fn)();
#endif

  flush_fn=bddm->op_cache.flush_fn[TAG(p)];
  if (flush_fn)
    return ((*flush_fn)(bddm, CACHE_POINTER(p), closure));
  return (0);
}


/* cmu_bdd_free_assoc(bddm, id) deletes the variable association given by */
/* id. */

void
#if defined(__STDC__)
cmu_bdd_free_assoc(cmu_bdd_manager bddm, int assoc_id)
#else
cmu_bdd_free_assoc(bddm, assoc_id)
     cmu_bdd_manager bddm;
     int assoc_id;
#endif
{
  bdd_indexindex_type i;
  bdd f;
  assoc_list p, *q;

  if (bddm->curr_assoc_id == assoc_id)
    {
      bddm->curr_assoc_id= -1;
      bddm->curr_assoc= &bddm->temp_assoc;
    }
  for (q= &bddm->assocs, p= *q; p; q= &p->next, p= *q)
    if (p->id == assoc_id)
      {
	p->refs--;
	if (!p->refs)
	  {
	    /* Unprotect the BDDs in the association. */
	    for (i=0; i < bddm->vars; ++i)
	      if ((f=p->va.assoc[i+1]))
		{
		  BDD_SETUP(f);
		  BDD_DECREFS(f);
		}
	    /* Flush old cache entries. */
	    bdd_flush_cache(bddm, bdd_flush_id_entries, (pointer)assoc_id);
	    *q=p->next;
	    mem_free_block((pointer)(p->va.assoc));
	    BDD_FREE_REC(bddm, (pointer)p, sizeof(struct assoc_list_));
	  }
	return;
      }
  cmu_bdd_warning("cmu_bdd_free_assoc: no variable association with specified ID");
}


/* cmu_bdd_augment_temp_assoc(bddm, assoc_info, pairs) adds to the temporary */
/* variable association as specified by assoc_info.  pairs is 0 if the */
/* information represents only a list of variables rather than a full */
/* association. */

void
#if defined(__STDC__)
cmu_bdd_augment_temp_assoc(cmu_bdd_manager bddm, bdd *assoc_info, int pairs)
#else
cmu_bdd_augment_temp_assoc(bddm, assoc_info, pairs)
     cmu_bdd_manager bddm;
     bdd *assoc_info;
     int pairs;
#endif
{
  long i;
  bdd_indexindex_type j;
  bdd f;
  long last;

  if (check_assoc(bddm, assoc_info, pairs))
    {
      last=bddm->temp_assoc.last;
      if (pairs)
	for (i=0; (f=assoc_info[i]) && assoc_info[i+1]; i+=2)
	  {
	    BDD_SETUP(f);
	    j=BDD_INDEXINDEX(f);
	    if ((long)bddm->indexes[j] > last)
	      last=bddm->indexes[j];
	    if ((f=bddm->temp_assoc.assoc[j]))
	      {
		BDD_SETUP(f);
		BDD_DECREFS(f);
	      }
	    f=assoc_info[i+1];
	    BDD_RESET(f);
	    bddm->temp_assoc.assoc[j]=f;
	    /* Protect BDDs in the association. */
	    BDD_INCREFS(f);
	  }
      else
	for (i=0; (f=assoc_info[i]); ++i)
	  {
	    BDD_SETUP(f);
	    j=BDD_INDEXINDEX(f);
	    if ((long)bddm->indexes[j] > last)
	      last=bddm->indexes[j];
	    if ((f=bddm->temp_assoc.assoc[j]))
	      {
		BDD_SETUP(f);
		BDD_DECREFS(f);
	      }
	    bddm->temp_assoc.assoc[j]=BDD_ONE(bddm);
	  }
      bddm->temp_assoc.last=last;
    }
}


/* cmu_bdd_temp_assoc(bddm, assoc_info, pairs) sets the temporary variable */
/* association as specified by assoc_info.  pairs is 0 if the */
/* information represents only a list of variables rather than a full */
/* association. */

void
#if defined(__STDC__)
cmu_bdd_temp_assoc(cmu_bdd_manager bddm, bdd *assoc_info, int pairs)
#else
cmu_bdd_temp_assoc(bddm, assoc_info, pairs)
     cmu_bdd_manager bddm;
     bdd *assoc_info;
     int pairs;
#endif
{
  long i;
  bdd f;

  /* Clean up old temporary association. */
  for (i=0; i < bddm->vars; ++i)
    {
      if ((f=bddm->temp_assoc.assoc[i+1]))
	{
	  BDD_SETUP(f);
	  BDD_DECREFS(f);
	}
      bddm->temp_assoc.assoc[i+1]=0;
    }
  bddm->temp_assoc.last= -1;
  cmu_bdd_augment_temp_assoc(bddm, assoc_info, pairs);
}


/* cmu_bdd_assoc(bddm, id) sets the current variable association to the */
/* one given by id and returns the ID of the old association.  An */
/* id of -1 indicates the temporary association. */

int
#if defined(__STDC__)
cmu_bdd_assoc(cmu_bdd_manager bddm, int assoc_id)
#else
cmu_bdd_assoc(bddm, assoc_id)
     cmu_bdd_manager bddm;
     int assoc_id;
#endif
{
  int old_assoc;
  assoc_list p;

  old_assoc=bddm->curr_assoc_id;
  if (assoc_id != -1)
    {
      for (p=bddm->assocs; p; p=p->next)
	if (p->id == assoc_id)
	  {
	    bddm->curr_assoc_id=p->id;
	    bddm->curr_assoc= &p->va;
	    return (old_assoc);
	  }
      cmu_bdd_warning("cmu_bdd_assoc: no variable association with specified ID");
    }
  bddm->curr_assoc_id= -1;
  bddm->curr_assoc= &bddm->temp_assoc;
  return (old_assoc);
}
