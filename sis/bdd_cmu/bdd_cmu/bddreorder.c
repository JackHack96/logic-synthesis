/* BDD dynamic reordering stuff */


#include "bddint.h"


static
void
#if defined(__STDC__)
increfs(bdd f)
#else
increfs(f)
     bdd f;
#endif
{
  bdd g;

  BDD_SETUP(f);
  BDD_INCREFS(f);
  if (BDD_REFS(f) == 1 && !BDD_TEMP_REFS(f))
    {
      g=(bdd)BDD_DATA0(f);
      {
	BDD_SETUP(g);
	BDD_INCREFS(g);
      }
      g=(bdd)BDD_DATA1(f);
      {
	BDD_SETUP(g);
	BDD_INCREFS(g);
      }
    }
}


static
void
#if defined(__STDC__)
decrefs(bdd f)
#else
decrefs(f)
     bdd f;
#endif
{
  bdd g;

  BDD_SETUP(f);
  BDD_DECREFS(f);
  if (!BDD_REFS(f) && !BDD_TEMP_REFS(f))
    {
      g=(bdd)BDD_DATA0(f);
      {
	BDD_SETUP(g);
	BDD_DECREFS(g);
      }
      g=(bdd)BDD_DATA1(f);
      {
	BDD_SETUP(g);
	BDD_DECREFS(g);
      }
    }
}


static
void
#if defined(__STDC__)
bdd_exchange_aux(cmu_bdd_manager bddm, bdd f, bdd_indexindex_type next_indexindex)
#else
bdd_exchange_aux(bddm, f, next_indexindex)
     cmu_bdd_manager bddm;
     bdd f;
     bdd_indexindex_type next_indexindex;
#endif
{
  bdd f1, f2;
  bdd f11, f12, f21, f22;
  bdd temp1, temp2;

  BDD_SETUP(f);
  f1=BDD_THEN(f);
  f2=BDD_ELSE(f);
  {
    BDD_SETUP(f1);
    BDD_SETUP(f2);
    if (BDD_INDEXINDEX(f1) == next_indexindex)
      {
	f11=BDD_THEN(f1);
	f12=BDD_ELSE(f1);
      }
    else
      {
	f11=f1;
	f12=f1;
      }
    if (BDD_INDEXINDEX(f2) == next_indexindex)
      {
	f21=BDD_THEN(f2);
	f22=BDD_ELSE(f2);
      }
    else
      {
	f21=f2;
	f22=f2;
      }
    if (f11 == f21)
      temp1=f11;
    else
      temp1=bdd_find_aux(bddm, BDD_INDEXINDEX(f), (INT_PTR)f11, (INT_PTR)f21);
    if (f12 == f22)
      temp2=f12;
    else if (BDD_IS_OUTPOS(f12))
      temp2=bdd_find_aux(bddm, BDD_INDEXINDEX(f), (INT_PTR)f12, (INT_PTR)f22);
    else
      temp2=BDD_NOT(bdd_find_aux(bddm, BDD_INDEXINDEX(f), (INT_PTR)BDD_NOT(f12), (INT_PTR)BDD_NOT(f22)));
    BDD_INDEXINDEX(f)=next_indexindex;
    BDD_DATA0(f)=(INT_PTR)temp1;
    BDD_DATA1(f)=(INT_PTR)temp2;
    if (BDD_REFS(f) || BDD_TEMP_REFS(f))
      {
	increfs(temp1);
	increfs(temp2);
	decrefs(f1);
	decrefs(f2);
      }
    else
      cmu_bdd_fatal("bdd_exchange_aux: how did this happen?");
  }
}


static
void
#if defined(__STDC__)
fixup_assoc(cmu_bdd_manager bddm, long indexindex1, long indexindex2, var_assoc va)
#else
fixup_assoc(bddm, indexindex1, indexindex2, va)
     cmu_bdd_manager bddm;
     long indexindex1;
     long indexindex2;
     var_assoc va;
#endif
{
  /* Variable with indexindex1 is moving down a spot. */
  if (va->assoc[indexindex1] && va->last == bddm->indexes[indexindex1])
    va->last++;
  else if (!va->assoc[indexindex1] && va->assoc[indexindex2] && va->last == bddm->indexes[indexindex2])
    va->last--;
}


static
void
#if defined(__STDC__)
sweep_var_table(cmu_bdd_manager bddm, long i)
#else
sweep_var_table(bddm, i)
     cmu_bdd_manager bddm;
     long i;
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
	if (BDD_REFS(f) || BDD_TEMP_REFS(f))
	  p= &f->next;
	else
	  {
	    *p=f->next;
	    BDD_FREE_REC(bddm, (pointer)f, sizeof(struct bdd_));
	    table->entries--;
	    bddm->unique_table.entries--;
	    bddm->unique_table.freed++;
	  }
      }
}


static
void
#if defined(__STDC__)
bdd_exchange(cmu_bdd_manager bddm, long i)
#else
bdd_exchange(bddm, i)
     cmu_bdd_manager bddm;
     long i;
#endif
{
  bdd_indexindex_type next_indexindex;
  var_table table, next_table;
  long j;
  bdd f, *p;
  bdd f1, f2;
  bdd g;
  long hash;
  bdd_index_type temp;
  assoc_list q;

  next_indexindex=bddm->indexindexes[bddm->indexes[i]+1];
  table=bddm->unique_table.tables[i];
  next_table=bddm->unique_table.tables[next_indexindex];
  g=0;
  for (j=0; j < table->size; ++j)
    for (p= &table->table[j], f= *p; f; f= *p)
      {
	BDD_SETUP(f);
	if (BDD_REFS(f) || BDD_TEMP_REFS(f))
	  {
	    f1=(bdd)BDD_DATA0(f);
	    f2=(bdd)BDD_DATA1(f);
	    {
	      BDD_SETUP(f1);
	      BDD_SETUP(f2);
	      if (BDD_INDEXINDEX(f1) != next_indexindex && BDD_INDEXINDEX(f2) != next_indexindex)
		p= &f->next;
	      else
		{
		  *p=f->next;
		  f->next=g;
		  g=f;
		}
	    }
	  }
	else
	  {
	    *p=f->next;
	    BDD_FREE_REC(bddm, (pointer)f, sizeof(struct bdd_));
	    table->entries--;
	    bddm->unique_table.entries--;
	    bddm->unique_table.freed++;
	  }
      }
  for (f=g; f; f=g)
    {
      bdd_exchange_aux(bddm, f, next_indexindex);
      g=f->next;
      hash=HASH_NODE(f->data[0], f->data[1]);
      BDD_REDUCE(hash, next_table->size);
      f->next=next_table->table[hash];
      next_table->table[hash]=f;
      table->entries--;
      next_table->entries++;
      if (4*next_table->size < next_table->entries)
	bdd_rehash_var_table(next_table, 1);
    }
  fixup_assoc(bddm, i, next_indexindex, &bddm->temp_assoc);
  for (q=bddm->assocs; q; q=q->next)
    fixup_assoc(bddm, i, next_indexindex, &q->va);
  sweep_var_table(bddm, next_indexindex);
  temp=bddm->indexes[i];
  bddm->indexes[i]=bddm->indexes[next_indexindex];
  bddm->indexes[next_indexindex]=temp;
  bddm->indexindexes[temp]=next_indexindex;
  bddm->indexindexes[bddm->indexes[i]]=i;
}


void
#if defined(__STDC__)
cmu_bdd_var_block_reorderable(cmu_bdd_manager bddm, block b, int reorderable)
#else
cmu_bdd_var_block_reorderable(bddm, b, reorderable)
     cmu_bdd_manager bddm;
     block b;
     int reorderable;
#endif
{
  b->reorderable=reorderable;
}


static
void
#if defined(__STDC__)
bdd_exchange_var_blocks(cmu_bdd_manager bddm, block parent, long bi)
#else
bdd_exchange_var_blocks(bddm, parent, bi)
     cmu_bdd_manager bddm;
     block parent;
     long bi;
#endif
{
  block b1, b2;
  long i, j, k, l;
  long delta;
  block temp;

  b1=parent->children[bi];
  b2=parent->children[bi+1];
  /* This slides the blocks past each other in a kind of interleaving */
  /* fashion. */
  for (i=0; i <= b1->last_index-b1->first_index+b2->last_index-b2->first_index; ++i)
    {
      j=i-b1->last_index+b1->first_index;
      if (j < 0)
	j=0;
      k=i;
      if (k > b2->last_index-b2->first_index)
	k=b2->last_index-b2->first_index;
      while (j <= k)
	{
	  l=b2->first_index+j-i+j;
	  bdd_exchange(bddm, bddm->indexindexes[l-1]);
	  ++j;
	}
    }
  delta=b2->last_index-b2->first_index+1;
  bdd_block_delta(b1, delta);
  delta=b1->last_index-b1->first_index+1;
  bdd_block_delta(b2, -delta);
  temp=parent->children[bi];
  parent->children[bi]=parent->children[bi+1];
  parent->children[bi+1]=temp;
}


static
int
#if defined(__STDC__)
cmu_bdd_reorder_window2(cmu_bdd_manager bddm, block b, long i)
#else
cmu_bdd_reorder_window2(bddm, b, i)
     cmu_bdd_manager bddm;
     block b;
     long i;
#endif
{
  long size, best_size;

  /* 1 2 */
  best_size=bddm->unique_table.entries;
  bdd_exchange_var_blocks(bddm, b, i);
  /* 2 1 */
  size=bddm->unique_table.entries;
  if (size < best_size)
    return (1);
  bdd_exchange_var_blocks(bddm, b, i);
  return (0);
}


static
int
#if defined(__STDC__)
cmu_bdd_reorder_window3(cmu_bdd_manager bddm, block b, long i)
#else
cmu_bdd_reorder_window3(bddm, b, i)
     cmu_bdd_manager bddm;
     block b;
     long i;
#endif
{
  int best;
  long size, best_size;

  best=0;
  /* 1 2 3 */
  best_size=bddm->unique_table.entries;
  bdd_exchange_var_blocks(bddm, b, i);
  /* 2 1 3 */
  size=bddm->unique_table.entries;
  if (size < best_size)
    {
      best=1;
      best_size=size;
    }
  bdd_exchange_var_blocks(bddm, b, i+1);
  /* 2 3 1 */
  size=bddm->unique_table.entries;
  if (size < best_size)
    {
      best=2;
      best_size=size;
    }
  bdd_exchange_var_blocks(bddm, b, i);
  /* 3 2 1 */
  size=bddm->unique_table.entries;
  if (size < best_size)
    {
      best=3;
      best_size=size;
    }
  bdd_exchange_var_blocks(bddm, b, i+1);
  /* 3 1 2 */
  size=bddm->unique_table.entries;
  if (size < best_size)
    {
      best=4;
      best_size=size;
    }
  bdd_exchange_var_blocks(bddm, b, i);
  /* 1 3 2 */
  size=bddm->unique_table.entries;
  if (size < best_size)
    {
      best=5;
      best_size=size;
    }
  switch (best)
    {
    case 0:
      bdd_exchange_var_blocks(bddm, b, i+1);
      break;
    case 1:
      bdd_exchange_var_blocks(bddm, b, i+1);
      bdd_exchange_var_blocks(bddm, b, i);
      break;
    case 2:
      bdd_exchange_var_blocks(bddm, b, i+1);
      bdd_exchange_var_blocks(bddm, b, i);
      bdd_exchange_var_blocks(bddm, b, i+1);
      break;
    case 3:
      bdd_exchange_var_blocks(bddm, b, i);
      bdd_exchange_var_blocks(bddm, b, i+1);
      break;
    case 4:
      bdd_exchange_var_blocks(bddm, b, i);
      break;
    case 5:
      break;
    }
  return (best > 0);
}


static
void
#if defined(__STDC__)
cmu_bdd_reorder_stable_window3_aux(cmu_bdd_manager bddm, block b, char *levels)
#else
cmu_bdd_reorder_stable_window3_aux(bddm, b, levels)
     cmu_bdd_manager bddm;
     block b;
     char *levels;
#endif
{
  long i;
  int moved;
  int any_swapped;

  if (b->reorderable)
    {
      for (i=0; i < b->num_children-1; ++i)
	levels[i]=1;
      do
	{
	  any_swapped=0;
	  for (i=0; i < b->num_children-1; ++i)
	    if (levels[i])
	      {
		if (i < b->num_children-2)
		  moved=cmu_bdd_reorder_window3(bddm, b, i);
		else
		  moved=cmu_bdd_reorder_window2(bddm, b, i);
		if (moved)
		  {
		    if (i > 0)
		      {
			levels[i-1]=1;
			if (i > 1)
			  levels[i-2]=1;
		      }
		    levels[i]=1;
		    levels[i+1]=1;
		    if (i < b->num_children-2)
		      {
			levels[i+2]=1;
			if (i < b->num_children-3)
			  {
			    levels[i+3]=1;
			    if (i < b->num_children-4)
			      levels[i+4]=1;
			  }
		      }
		    any_swapped=1;
		  }
		else
		  levels[i]=0;
	      }
	}
      while (any_swapped);
    }
  for (i=0; i < b->num_children; ++i)
    cmu_bdd_reorder_stable_window3_aux(bddm, b->children[i], levels);
}


void
#if defined(__STDC__)
cmu_bdd_reorder_stable_window3(cmu_bdd_manager bddm)
#else
cmu_bdd_reorder_stable_window3(bddm)
     cmu_bdd_manager bddm;
#endif
{
  char *levels;

  levels=(char *)mem_get_block(bddm->vars*sizeof(char));
  cmu_bdd_reorder_stable_window3_aux(bddm, bddm->super_block, levels);
  mem_free_block((pointer)levels);
}


static
void
#if defined(__STDC__)
bdd_sift_block(cmu_bdd_manager bddm, block b, long start_pos, double max_size_factor)
#else
bdd_sift_block(bddm, b, start_pos, max_size_factor)
     cmu_bdd_manager bddm;
     block b;
     long start_pos;
     double max_size_factor;
#endif
{
  long start_size;
  long best_size;
  long best_pos;
  long curr_size;
  long curr_pos;
  long max_size;

  start_size=bddm->unique_table.entries;
  best_size=start_size;
  best_pos=start_pos;
  curr_size=start_size;
  curr_pos=start_pos;
  max_size=max_size_factor*start_size;
  if (bddm->unique_table.node_limit && max_size > bddm->unique_table.node_limit)
    max_size=bddm->unique_table.node_limit;
  while (curr_pos < b->num_children-1 && curr_size <= max_size)
    {
      bdd_exchange_var_blocks(bddm, b, curr_pos);
      ++curr_pos;
      curr_size=bddm->unique_table.entries;
      if (curr_size < best_size)
	{
	  best_size=curr_size;
	  best_pos=curr_pos;
	}
    }
  while (curr_pos != start_pos)
    {
      --curr_pos;
      bdd_exchange_var_blocks(bddm, b, curr_pos);
    }
  curr_size=start_size;
  while (curr_pos && curr_size <= max_size)
    {
      --curr_pos;
      bdd_exchange_var_blocks(bddm, b, curr_pos);
      curr_size=bddm->unique_table.entries;
      if (curr_size < best_size)
	{
	  best_size=curr_size;
	  best_pos=curr_pos;
	}
    }
  while (curr_pos != best_pos)
    {
      bdd_exchange_var_blocks(bddm, b, curr_pos);
      ++curr_pos;
    }
}


static
void
#if defined(__STDC__)
cmu_bdd_reorder_sift_aux(cmu_bdd_manager bddm, block b, block *to_sift, double max_size_factor)
#else
cmu_bdd_reorder_sift_aux(bddm, b, to_sift, max_size_factor)
     cmu_bdd_manager bddm;
     block b;
     block *to_sift;
     double max_size_factor;
#endif
{
  long i, j, k;
  long w;
  long max_w;
  long widest;

  if (b->reorderable)
    {
      for (i=0; i < b->num_children; ++i)
	to_sift[i]=b->children[i];
      while (i)
	{
	  --i;
	  max_w=0;
	  widest=0;
	  for (j=0; j <= i; ++j)
	    {
	      for (w=0, k=to_sift[j]->first_index; k <= to_sift[j]->last_index; ++k)
		w+=bddm->unique_table.tables[bddm->indexindexes[k]]->entries;
	      w/=to_sift[j]->last_index-to_sift[j]->first_index+1;
	      if (w > max_w)
		{
		  max_w=w;
		  widest=j;
		}
	    }
	  if (max_w > 1)
	    {
	      for (j=0; b->children[j] != to_sift[widest]; ++j);
	      bdd_sift_block(bddm, b, j, max_size_factor);
	      to_sift[widest]=to_sift[i];
	    }
	  else
	    break;
	}
    }
  for (i=0; i < b->num_children; ++i)
    cmu_bdd_reorder_sift_aux(bddm, b->children[i], to_sift, max_size_factor);
}


static
void
#if defined(__STDC__)
cmu_bdd_reorder_sift_aux1(cmu_bdd_manager bddm, double max_size_factor)
#else
cmu_bdd_reorder_sift_aux1(bddm, max_size_factor)
     cmu_bdd_manager bddm;
     double max_size_factor;
#endif
{
  block *to_sift;

  to_sift=(block *)mem_get_block(bddm->vars*sizeof(block));
  cmu_bdd_reorder_sift_aux(bddm, bddm->super_block, to_sift, max_size_factor);
  mem_free_block((pointer)to_sift);
}


void
#if defined(__STDC__)
cmu_bdd_reorder_sift(cmu_bdd_manager bddm)
#else
cmu_bdd_reorder_sift(bddm)
     cmu_bdd_manager bddm;
#endif
{
  cmu_bdd_reorder_sift_aux1(bddm, 2.0);
}


void
#if defined(__STDC__)
cmu_bdd_reorder_hybrid(cmu_bdd_manager bddm)
#else
cmu_bdd_reorder_hybrid(bddm)
     cmu_bdd_manager bddm;
#endif
{
  long nodes;
  double max_size_factor;

  nodes=bddm->unique_table.entries;
  max_size_factor= *(double *)bddm->reorder_data;
  if (max_size_factor > 2.0 || nodes < 10000)
    max_size_factor=2.0;
  cmu_bdd_reorder_sift_aux1(bddm, max_size_factor);
  *(double *)bddm->reorder_data=1.0+(2.0*(nodes-bddm->unique_table.entries))/nodes;
}


/* cmu_bdd_dynamic_reordering(bddm, reorder_fn) sets the dynamic reordering */
/* method to that specified by reorder_fn. */

void
#if defined(__STDC__)
cmu_bdd_dynamic_reordering(cmu_bdd_manager bddm, void (*reorder_fn)(cmu_bdd_manager))
#else
cmu_bdd_dynamic_reordering(bddm, reorder_fn)
     cmu_bdd_manager bddm;
     void (*reorder_fn)();
#endif
{
  bddm->reorder_fn=reorder_fn;
  if (bddm->reorder_data)
    mem_free_block(bddm->reorder_data);
  bddm->reorder_data=0;
  if (reorder_fn == cmu_bdd_reorder_hybrid)
    {
      bddm->reorder_data=mem_get_block((SIZE_T)sizeof(double));
      *(double *)bddm->reorder_data=2.0;
    }
}


static
void
#if defined(__STDC__)
bdd_add_internal_references(cmu_bdd_manager bddm)
#else
bdd_add_internal_references(bddm)
     cmu_bdd_manager bddm;
#endif
{
  long i, j;
  var_table table;
  bdd *f, g, h;

  for (i=0; i <= bddm->vars; ++i)
    {
      if (i == bddm->vars)
	table=bddm->unique_table.tables[BDD_CONST_INDEXINDEX];
      else
	table=bddm->unique_table.tables[bddm->indexindexes[i]];
      for (j=0; j < table->size; ++j)
	{
	  f= &table->table[j];
	  while ((g= *f))
	    {
	      BDD_SETUP(g);
	      if (BDD_REFS(g) || BDD_TEMP_REFS(g))
		{
		  if (!BDD_IS_CONST(g))
		    {
		      h=(bdd)BDD_DATA0(g);
		      {
			BDD_SETUP(h);
			BDD_INCREFS(h);
		      }
		      h=(bdd)BDD_DATA1(g);
		      {
			BDD_SETUP(h);
			BDD_INCREFS(h);
		      }
		    }
		  f= &g->next;
		}
	      else
		{
		  *f=g->next;
		  if (i == bddm->vars && bddm->unique_table.free_terminal_fn)
		    (*bddm->unique_table.free_terminal_fn)(bddm,
							   BDD_DATA0(g),
							   BDD_DATA1(g),
							   bddm->unique_table.free_terminal_env);
		  BDD_FREE_REC(bddm, (pointer)g, sizeof(struct bdd_));
		  table->entries--;
		  bddm->unique_table.entries--;
		  bddm->unique_table.freed++;
		}
	    }
	}
    }
}


static
void
#if defined(__STDC__)
bdd_nuke_internal_references(cmu_bdd_manager bddm)
#else
bdd_nuke_internal_references(bddm)
     cmu_bdd_manager bddm;
#endif
{
  long i, j;
  var_table table;
  bdd *f, g, h;

  for (i=bddm->vars-1; i >= 0; --i)
    {
      table=bddm->unique_table.tables[bddm->indexindexes[i]];
      for (j=0; j < table->size; ++j)
	{
	  f= &table->table[j];
	  while ((g= *f))
	    {
	      BDD_SETUP(g);
	      if (BDD_REFS(g) || BDD_TEMP_REFS(g))
		{
		  h=(bdd)BDD_DATA0(g);
		  {
		    BDD_SETUP(h);
		    BDD_DECREFS(h);
		  }
		  h=(bdd)BDD_DATA1(g);
		  {
		    BDD_SETUP(h);
		    BDD_DECREFS(h);
		  }
		  f= &g->next;
		}
	      else
		cmu_bdd_fatal("bdd_nuke_internal_references: what happened?");
	    }
	}
    }
}


void
#if defined(__STDC__)
cmu_bdd_reorder_aux(cmu_bdd_manager bddm)
#else
cmu_bdd_reorder_aux(bddm)
     cmu_bdd_manager bddm;
#endif
{
  if (bddm->reorder_fn)
    {
      bdd_flush_all(bddm);
      bdd_add_internal_references(bddm);
      (*bddm->reorder_fn)(bddm);
      bdd_nuke_internal_references(bddm);
    }
}


/* cmu_bdd_reorder(bddm) invokes the current dynamic reordering method. */

void
#if defined(__STDC__)
cmu_bdd_reorder(cmu_bdd_manager bddm)
#else
cmu_bdd_reorder(bddm)
     cmu_bdd_manager bddm;
#endif
{
  cmu_bdd_gc(bddm);
  cmu_bdd_reorder_aux(bddm);
}
