/* BDD hash table routines */


#include "bddint.h"


#define HASH(d) ((INT_PTR)(d))


/* bdd_rehash_hash_table(h) increases the size of h by roughly a */
/* factor of 2 and rehashes all of its entries. */

static
void
#if defined(__STDC__)
bdd_rehash_hash_table(hash_table h)
#else
bdd_rehash_hash_table(h)
     hash_table h;
#endif
{
  long i;
  long hash;
  long oldsize;
  hash_rec *newtable;
  hash_rec p, q;

  oldsize=h->size;
  h->size_index++;
  h->size=TABLE_SIZE(h->size_index);
  newtable=(hash_rec *)mem_get_block((SIZE_T)(h->size*sizeof(hash_rec)));
  for (i=0; i < h->size; ++i)
    newtable[i]=0;
  for (i=0; i < oldsize; ++i)
    for (p=h->table[i]; p; p=q)
      {
	q=p->next;
	hash=HASH(p->key);
	BDD_REDUCE(hash, h->size);
	p->next=newtable[hash];
	newtable[hash]=p;
      }
  mem_free_block((pointer)h->table);
  h->table=newtable;
}


/* bdd_insert_in_hash_table(h, f, data) associates the specified data */
/* with f in h. */

void
#if defined(__STDC__)
bdd_insert_in_hash_table(hash_table h, bdd f, pointer data)
#else
bdd_insert_in_hash_table(h, f, data)
     hash_table h;
     bdd f;
     pointer data;
#endif
{
  long hash;
  hash_rec p;

  p=(hash_rec)BDD_NEW_REC(h->bddm, ALIGN(sizeof(struct hash_rec_))+h->item_size);
  p->key=f;
  mem_copy((pointer)(ALIGN(sizeof(struct hash_rec_))+(INT_PTR)p), data, (SIZE_T)h->item_size);
  hash=HASH(f);
  BDD_REDUCE(hash, h->size);
  p->next=h->table[hash];
  h->table[hash]=p;
  h->entries++;
  if ((h->size << 2) < h->entries)
    bdd_rehash_hash_table(h);
}


/* bdd_lookup_in_hash_table(h, f) looks up f in h and returns either a */
/* pointer to the associated data or null. */

pointer
#if defined(__STDC__)
bdd_lookup_in_hash_table(hash_table h, bdd f)
#else
bdd_lookup_in_hash_table(h, f)
     hash_table h;
     bdd f;
#endif
{
  long hash;
  hash_rec p;

  hash=HASH(f);
  BDD_REDUCE(hash, h->size);
  for (p=h->table[hash]; p; p=p->next)
    if (p->key == f)
      return ((pointer)(ALIGN(sizeof(struct hash_rec_))+(char *)p));
  return ((pointer)0);
}


/* bdd_new_hash_table(bddm, item_size) creates a new hash table with */
/* the specified data item size. */

hash_table
#if defined(__STDC__)
bdd_new_hash_table(cmu_bdd_manager bddm, int item_size)
#else
bdd_new_hash_table(bddm, item_size)
     cmu_bdd_manager bddm;
     int item_size;
#endif
{
  long i;
  hash_table h;

  h=(hash_table)BDD_NEW_REC(bddm, sizeof(struct hash_table_));
  h->size_index=10;
  h->size=TABLE_SIZE(h->size_index);
  h->table=(hash_rec *)mem_get_block((SIZE_T)(h->size*sizeof(hash_rec)));
  for (i=0; i < h->size; ++i)
    h->table[i]=0;
  h->entries=0;
  h->item_size=item_size;
  h->bddm=bddm;
  return (h);
}


/* cmu_bdd_free_hash_table(h) frees up the storage associated with h. */

void
#if defined(__STDC__)
cmu_bdd_free_hash_table(hash_table h)
#else
cmu_bdd_free_hash_table(h)
     hash_table h;
#endif
{
  long i;
  hash_rec p, q;

  for (i=0; i < h->size; ++i)
    for (p=h->table[i]; p; p=q)
      {
	q=p->next;
	BDD_FREE_REC(h->bddm, (pointer)p, ALIGN(sizeof(struct hash_rec_))+h->item_size);
      }
  mem_free_block((pointer)h->table);
  BDD_FREE_REC(h->bddm, (pointer)h, sizeof(struct hash_table_));
}
