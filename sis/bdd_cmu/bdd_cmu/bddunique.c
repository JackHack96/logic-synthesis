/* BDD unique table routines */


#include "bddint.h"


#define MIN_GC_LIMIT 10000


void
#if defined(__STDC__)
bdd_rehash_var_table(var_table table, int grow)
#else
bdd_rehash_var_table(table, grow)
     var_table table;
     int grow;
#endif
{
  long i;
  long hash;
  long oldsize;
  bdd *newtable;
  bdd p, q;

  oldsize=table->size;
  if (grow)
    table->size_index++;
  else
    table->size_index--;
  table->size=TABLE_SIZE(table->size_index);
  newtable=(bdd *)mem_get_block((SIZE_T)(table->size*sizeof(bdd)));
  for (i=0; i < table->size; ++i)
    newtable[i]=0;
  for (i=0; i < oldsize; ++i)
    for (p=table->table[i]; p; p=q)
      {
	q=p->next;
	hash=HASH_NODE(p->data[0], p->data[1]);
	BDD_REDUCE(hash, table->size);
	p->next=newtable[hash];
	newtable[hash]=p;
      }
  mem_free_block((pointer)table->table);
  table->table=newtable;
}


static
void
#if defined(__STDC__)
bdd_mark(cmu_bdd_manager bddm)
#else
bdd_mark(bddm)
     cmu_bdd_manager bddm;
#endif
{
  long i, j;
  var_table table;
  bdd f, g;

  for (i=0; i <= bddm->vars; ++i)
    {
      if (i == bddm->vars)
	table=bddm->unique_table.tables[BDD_CONST_INDEXINDEX];
      else
	table=bddm->unique_table.tables[bddm->indexindexes[i]];
      for (j=0; j < table->size; ++j)
	for (f=table->table[j]; f; f=f->next)
	  {
	    BDD_SETUP(f);
	    if (BDD_REFS(f) || BDD_TEMP_REFS(f))
	      BDD_MARK(f)|=BDD_GC_MARK;
	    if (BDD_IS_USED(f) && !BDD_IS_CONST(f))
	      {
		g=(bdd)BDD_DATA0(f);
		{
		  BDD_SETUP(g);
		  BDD_MARK(g)|=BDD_GC_MARK;
		}
		g=(bdd)BDD_DATA1(f);
		{
		  BDD_SETUP(g);
		  BDD_MARK(g)|=BDD_GC_MARK;
		}
	      }
	  }
    }
}


void
#if defined(__STDC__)
bdd_sweep_var_table(cmu_bdd_manager bddm, long i, int maybe_rehash)
#else
bdd_sweep_var_table(bddm, i, maybe_rehash)
     cmu_bdd_manager bddm;
     long i;
     int maybe_rehash;
#endif
{
  long j;
  var_table table;
  bdd f, *p;

  table=bddm->unique_table.tables[i];
  for (j=0; j < table->size; ++j)
    for (p= &table->table[j], f= *p; f; f= *p)
      {
	BDD_SETUP(f);
	if (BDD_IS_USED(f))
	  {
	    BDD_SETUP(f);
	    BDD_MARK(f)&=~BDD_GC_MARK;
	    p= &f->next;
	  }
	else
	  {
	    *p=f->next;
	    if (i == BDD_CONST_INDEXINDEX && bddm->unique_table.free_terminal_fn)
	      (*bddm->unique_table.free_terminal_fn)(bddm,
						     BDD_DATA0(f),
						     BDD_DATA1(f),
						     bddm->unique_table.free_terminal_env);
	    BDD_FREE_REC(bddm, (pointer)f, sizeof(struct bdd_));
	    table->entries--;
	    bddm->unique_table.entries--;
	    bddm->unique_table.freed++;
	  }
      }
  if (maybe_rehash && table->size > table->entries && table->size_index > 3)
    bdd_rehash_var_table(table, 0);
}


void
#if defined(__STDC__)
bdd_sweep(cmu_bdd_manager bddm)
#else
bdd_sweep(bddm)
     cmu_bdd_manager bddm;
#endif
{
  long i;

  for (i=0; i <= bddm->vars; ++i)
    bdd_sweep_var_table(bddm, i, 1);
}


/* cmu_bdd_gc(bddm) performs a garbage collection. */

void
#if defined(__STDC__)
cmu_bdd_gc(cmu_bdd_manager bddm)
#else
cmu_bdd_gc(bddm)
     cmu_bdd_manager bddm;
#endif
{
  bdd_mark(bddm);
  bdd_purge_cache(bddm);
  bdd_sweep(bddm);
  bddm->unique_table.gcs++;
}


static
void
#if defined(__STDC__)
bdd_set_gc_limit(cmu_bdd_manager bddm)
#else
bdd_set_gc_limit(bddm)
     cmu_bdd_manager bddm;
#endif
{
  bddm->unique_table.gc_limit=2*bddm->unique_table.entries;
  if (bddm->unique_table.gc_limit < MIN_GC_LIMIT)
    bddm->unique_table.gc_limit=MIN_GC_LIMIT;
  if (bddm->unique_table.node_limit &&
      bddm->unique_table.gc_limit > bddm->unique_table.node_limit)
    bddm->unique_table.gc_limit=bddm->unique_table.node_limit;
}


void
#if defined(__STDC__)
bdd_clear_temps(cmu_bdd_manager bddm)
#else
bdd_clear_temps(bddm)
     cmu_bdd_manager bddm;
#endif
{
  long i, j;
  var_table table;
  bdd f;

  for (i=0; i <= bddm->vars; ++i)
    {
      table=bddm->unique_table.tables[i];
      for (j=0; j < table->size; ++j)
	for (f=table->table[j]; f; f=f->next)
	  {
	    BDD_SETUP(f);
	    BDD_TEMP_REFS(f)=0;
	  }
    }
  cmu_bdd_gc(bddm);
  bdd_set_gc_limit(bddm);
}  


void
#if defined(__STDC__)
bdd_cleanup(cmu_bdd_manager bddm, int code)
#else
bdd_cleanup(bddm, code)
     cmu_bdd_manager bddm;
     int code;
#endif
{
  bdd_clear_temps(bddm);
  switch (code)
    {
    case BDD_ABORTED:
      (*bddm->bag_it_fn)(bddm, bddm->bag_it_env);
      break;
    case BDD_OVERFLOWED:
      if (bddm->overflow_fn)
	(*bddm->overflow_fn)(bddm, bddm->overflow_env);
      break;
    }
}


bdd
#if defined(__STDC__)
bdd_find_aux(cmu_bdd_manager bddm, bdd_indexindex_type indexindex, INT_PTR d1, INT_PTR d2)
#else
bdd_find_aux(bddm, indexindex, d1, d2)
     cmu_bdd_manager bddm;
     bdd_indexindex_type indexindex;
     INT_PTR d1;
     INT_PTR d2;
#endif
{
  var_table table;
  long hash;
  bdd temp;

  table=bddm->unique_table.tables[indexindex];
  hash=HASH_NODE(d1, d2);
  BDD_REDUCE(hash, table->size);
  for (temp=table->table[hash]; temp; temp=temp->next)
    if (temp->data[0] == d1 && temp->data[1] == d2)
      break;
  if (!temp)
    {
      /* Not found; make a new node. */
      temp=(bdd)BDD_NEW_REC(bddm, sizeof(struct bdd_));
      temp->indexindex=indexindex;
      temp->refs=0;
      temp->mark=0;
      temp->data[0]=d1;
      temp->data[1]=d2;
      temp->next=table->table[hash];
      table->table[hash]=temp;
      table->entries++;
      bddm->unique_table.entries++;
      if (4*table->size < table->entries)
	bdd_rehash_var_table(table, 1);
    }
  bddm->unique_table.finds++;
  return (temp);
}


static
void
#if defined(__STDC__)
bdd_check(cmu_bdd_manager bddm)
#else
bdd_check(bddm)
     cmu_bdd_manager bddm;
#endif
{
  long nodes;

  bddm->check=100;
  /* When bag_it_fn set, clean up and abort immediately. */
  if (bddm->bag_it_fn)
    longjmp(bddm->abort.context, BDD_ABORTED);
  if (bddm->unique_table.entries > bddm->unique_table.gc_limit)
    {
      cmu_bdd_gc(bddm);
      /* Table full. */
      nodes=bddm->unique_table.entries;
      if (3*nodes > 2*bddm->unique_table.gc_limit && bddm->allow_reordering && bddm->reorder_fn)
	{
	  cmu_bdd_reorder_aux(bddm);
	  if (4*bddm->unique_table.entries > 3*nodes && 3*nodes > 4*bddm->nodes_at_start)
	    /* If we didn't save much, but we have created a reasonable number */
	    /* of nodes, then don't bother reordering next time. */
	    bddm->allow_reordering=0;
	  /* Go try again. */
	  bdd_set_gc_limit(bddm);
	  longjmp(bddm->abort.context, BDD_REORDERED);
	}
      bdd_set_gc_limit(bddm);
      if (bddm->unique_table.node_limit &&
	  bddm->unique_table.entries >= bddm->unique_table.node_limit-1000)
	{
	  /* Out of memory; go clean up. */
	  bddm->overflow=1;
	  longjmp(bddm->abort.context, BDD_OVERFLOWED);
	}
    }
  /* Maybe increase cache size if it's getting full. */
  if (3*bddm->op_cache.size < 2*bddm->op_cache.entries &&
      32*bddm->op_cache.size < bddm->op_cache.cache_ratio*bddm->unique_table.entries)
    bdd_rehash_cache(bddm, 1);
}


/* bdd_find(bddm, indexindex, f, g) creates or finds a node with the */
/* given indexindex, "then" pointer, and "else" pointer. */

bdd
#if defined(__STDC__)
bdd_find(cmu_bdd_manager bddm, bdd_indexindex_type indexindex, bdd f, bdd g)
#else
bdd_find(bddm, indexindex, f, g)
     cmu_bdd_manager bddm;
     bdd_indexindex_type indexindex;
     bdd f;
     bdd g;
#endif
{
  bdd temp;

  BDD_SETUP(f);
  BDD_SETUP(g);
  if (f == g)
    {
      BDD_TEMP_DECREFS(f);
      temp=f;
    }
  else
    {
      if (BDD_IS_OUTPOS(f))
	temp=bdd_find_aux(bddm, indexindex, (INT_PTR)f, (INT_PTR)g);
      else
	temp=BDD_NOT(bdd_find_aux(bddm, indexindex, (INT_PTR)BDD_NOT(f), (INT_PTR)BDD_NOT(g)));
      {
	BDD_SETUP(temp);
	BDD_TEMP_INCREFS(temp);
      }
      BDD_TEMP_DECREFS(f);
      BDD_TEMP_DECREFS(g);
    }
  bddm->check--;
  if (!bddm->check)
    bdd_check(bddm);
  return (temp);
}


/* bdd_find_terminal(bddm, value1, value2) creates or finds a terminal */
/* node with the given data value. */

bdd
#if defined(__STDC__)
bdd_find_terminal(cmu_bdd_manager bddm, INT_PTR value1, INT_PTR value2)
#else
bdd_find_terminal(bddm, value1, value2)
     cmu_bdd_manager bddm;
     INT_PTR value1;
     INT_PTR value2;
#endif
{
  bdd temp;

  if ((*bddm->canonical_fn)(bddm, value1, value2, bddm->transform_env))
    {
      (*bddm->transform_fn)(bddm, value1, value2, &value1, &value2, bddm->transform_env);
      temp=BDD_NOT(bdd_find_aux(bddm, BDD_CONST_INDEXINDEX, value1, value2));
    }
  else
    temp=bdd_find_aux(bddm, BDD_CONST_INDEXINDEX, value1, value2);
  {
    BDD_SETUP(temp);
    BDD_TEMP_INCREFS(temp);
  }
  bddm->check--;
  if (!bddm->check)
    bdd_check(bddm);
  return (temp);
}


/* cmu_bdd_clear_refs(bddm) sets the reference count of all nodes to 0. */

void
#if defined(__STDC__)
cmu_bdd_clear_refs(cmu_bdd_manager bddm)
#else
cmu_bdd_clear_refs(bddm)
     cmu_bdd_manager bddm;
#endif
{
  long i, j;
  var_table table;
  bdd f;
  assoc_list q;

  for (i=0; i <= bddm->vars; ++i)
    {
      table=bddm->unique_table.tables[i];
      for (j=0; j < table->size; ++j)
	for (f=table->table[j]; f; f=f->next)
	  {
	    BDD_SETUP(f);
	    BDD_REFS(f)=0;
	  }
    }
  for (i=0; i < bddm->vars; ++i)
    bddm->variables[i+1]->refs=BDD_MAX_REFS;
  bddm->one->refs=BDD_MAX_REFS;
  for (q=bddm->assocs; q; q=q->next)
    for (i=0; i < bddm->vars; ++i)
      if ((f=q->va.assoc[i+1]))
	{
	  BDD_SETUP(f);
	  BDD_INCREFS(f);
	}
}


var_table
#if defined(__STDC__)
bdd_new_var_table(cmu_bdd_manager bddm)
#else
bdd_new_var_table(bddm)
     cmu_bdd_manager bddm;
#endif
{
  long i;
  var_table table;

  table=(var_table)BDD_NEW_REC(bddm, sizeof(struct var_table_));
  table->size_index=3;
  table->size=TABLE_SIZE(table->size_index);
  table->entries=0;
  table->table=(bdd *)mem_get_block((SIZE_T)(table->size*sizeof(bdd)));
  for (i=0; i < table->size; ++i)
    table->table[i]=0;
  return (table);
}


void
#if defined(__STDC__)
cmu_bdd_init_unique(cmu_bdd_manager bddm)
#else
cmu_bdd_init_unique(bddm)
     cmu_bdd_manager bddm;
#endif
{
  bddm->unique_table.tables=(var_table *)mem_get_block((SIZE_T)((bddm->maxvars+1)*sizeof(var_table)));
  bddm->unique_table.tables[BDD_CONST_INDEXINDEX]=bdd_new_var_table(bddm);
  bddm->unique_table.gc_limit=MIN_GC_LIMIT;
  bddm->unique_table.node_limit=0;
  bddm->unique_table.entries=0;
  bddm->unique_table.freed=0;
  bddm->unique_table.gcs=0;
  bddm->unique_table.finds=0;
  bddm->unique_table.free_terminal_fn=0;
  bddm->unique_table.free_terminal_env=0;
}


void
#if defined(__STDC__)
cmu_bdd_free_unique(cmu_bdd_manager bddm)
#else
cmu_bdd_free_unique(bddm)
     cmu_bdd_manager bddm;
#endif
{
  long i, j;
  var_table table;
  bdd p, q;

  for (i=0; i <= bddm->vars; ++i)
    {
      table=bddm->unique_table.tables[i];
      for (j=0; j < table->size; ++j)
	for (p=table->table[j]; p; p=q)
	  {
	    q=p->next;
	    BDD_FREE_REC(bddm, (pointer)p, sizeof(struct bdd_));
	  }
      mem_free_block((pointer)table->table);
      BDD_FREE_REC(bddm, (pointer)table, sizeof(struct var_table_));
    }
  mem_free_block((pointer)bddm->unique_table.tables);
}
