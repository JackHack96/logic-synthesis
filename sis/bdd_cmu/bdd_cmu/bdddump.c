/* BDD library dump/undump routines */


#include "bddint.h"


#define MAGIC_COOKIE 0x5e02f795l
#define BDD_IOERROR 100


#define TRUE_ENCODING 0xffffff00l
#define FALSE_ENCODING 0xffffff01l
#define POSVAR_ENCODING 0xffffff02l
#define NEGVAR_ENCODING 0xffffff03l
#define POSNODE_ENCODING 0xffffff04l
#define NEGNODE_ENCODING 0xffffff05l
#define NODELABEL_ENCODING 0xffffff06l
#define CONSTANT_ENCODING 0xffffff07l


static
int
#if defined(__STDC__)
bytes_needed(long n)
#else
bytes_needed(n)
     long n;
#endif
{
  if (n <= 0x100l)
    return (1);
  if (n <= 0x10000l)
    return (2);
  if (n <= 0x1000000l)
    return (3);
  return (4);
}


static
void
#if defined(__STDC__)
write(cmu_bdd_manager bddm, unsigned long n, int bytes, FILE *fp)
#else
write(bddm, n, bytes, fp)
     cmu_bdd_manager bddm;
     unsigned long n;
     int bytes;
     FILE *fp;
#endif
{
  while (bytes)
    {
      if (fputc((char)(n >> (8*(bytes-1)) & 0xff), fp) == EOF)
	longjmp(bddm->abort.context, BDD_IOERROR);
      --bytes;
    }
}


static
void
#if defined(__STDC__)
cmu_bdd_dump_bdd_step(cmu_bdd_manager bddm,
		  bdd f,
		  FILE *fp,
		  hash_table h,
		  bdd_index_type *normalized_indexes,
		  int index_size,
		  int node_number_size)
#else
cmu_bdd_dump_bdd_step(bddm, f, fp, h, normalized_indexes, index_size, node_number_size)
     cmu_bdd_manager bddm;
     bdd f;
     FILE *fp;
     hash_table h;
     bdd_index_type *normalized_indexes;
     int index_size;
     int node_number_size;
#endif
{
  int negated;
  long *number;

  BDD_SETUP(f);
  switch (cmu_bdd_type_aux(bddm, f))
    {
    case BDD_TYPE_ZERO:
      write(bddm, FALSE_ENCODING, index_size+1, fp);
      break;
    case BDD_TYPE_ONE:
      write(bddm, TRUE_ENCODING, index_size+1, fp);
      break;
    case BDD_TYPE_CONSTANT:
      write(bddm, CONSTANT_ENCODING, index_size+1, fp);
      write(bddm, (unsigned long)BDD_DATA(f)[0], sizeof(long), fp);
      write(bddm, (unsigned long)BDD_DATA(f)[1], sizeof(long), fp);
      break;
    case BDD_TYPE_POSVAR:
      write(bddm, POSVAR_ENCODING, index_size+1, fp);
      write(bddm, (unsigned long)normalized_indexes[BDD_INDEX(bddm, f)], index_size, fp);
      break;
    case BDD_TYPE_NEGVAR:
      write(bddm, NEGVAR_ENCODING, index_size+1, fp);
      write(bddm, (unsigned long)normalized_indexes[BDD_INDEX(bddm, f)], index_size, fp);
      break;
    case BDD_TYPE_NONTERMINAL:
      if (bdd_lookup_in_hash_table(h, BDD_NOT(f)))
	{
	  f=BDD_NOT(f);
	  negated=1;
	}
      else
	negated=0;
      number=(long *)bdd_lookup_in_hash_table(h, f);
      if (number && *number < 0)
	{
	  if (negated)
	    write(bddm, NEGNODE_ENCODING, index_size+1, fp);
	  else
	    write(bddm, POSNODE_ENCODING, index_size+1, fp);
	  write(bddm, (unsigned long)(-*number-1), node_number_size, fp);
	}
      else
	{
	  if (number)
	    {
	      write(bddm, NODELABEL_ENCODING, index_size+1, fp);
	      *number= -*number-1;
	    }
	  write(bddm, (unsigned long)normalized_indexes[BDD_INDEX(bddm, f)], index_size, fp);
	  cmu_bdd_dump_bdd_step(bddm, BDD_THEN(f), fp, h, normalized_indexes, index_size, node_number_size);
	  cmu_bdd_dump_bdd_step(bddm, BDD_ELSE(f), fp, h, normalized_indexes, index_size, node_number_size);
	}
      break;
    default:
      cmu_bdd_fatal("cmu_bdd_dump_bdd_step: unknown type returned by cmu_bdd_type");
    }
}


int
#if defined(__STDC__)
cmu_bdd_dump_bdd(cmu_bdd_manager bddm, bdd f, bdd *vars, FILE *fp)
#else
cmu_bdd_dump_bdd(bddm, f, vars, fp)
     cmu_bdd_manager bddm;
     bdd f;
     bdd *vars;
     FILE *fp;
#endif
{
  long i;
  bdd_index_type *normalized_indexes;
  bdd_index_type v_index;
  bdd var;
  bdd_index_type number_vars;
  bdd *support;
  int ok;
  hash_table h;
  int index_size;
  long next;
  int node_number_size;

  if (bdd_check_arguments(1, f))
    {
      for (i=0; vars[i]; ++i)
	if (cmu_bdd_type(bddm, vars[i]) != BDD_TYPE_POSVAR)
	  {
	    cmu_bdd_warning("cmu_bdd_dump_bdd: support is not all positive variables");
	    return (0);
	  }
      normalized_indexes=(bdd_index_type *)mem_get_block((SIZE_T)(bddm->vars*sizeof(bdd_index_type)));
      for (i=0; i < bddm->vars; ++i)
	normalized_indexes[i]=BDD_MAX_INDEX;
      for (i=0; (var=vars[i]); ++i)
	{
	  BDD_SETUP(var);
	  v_index=BDD_INDEX(bddm, var);
	  if (normalized_indexes[v_index] != BDD_MAX_INDEX)
	    {
	      cmu_bdd_warning("cmu_bdd_dump_bdd: variables duplicated in support");
	      mem_free_block((pointer)normalized_indexes);
	      return (0);
	    }
	  normalized_indexes[v_index]=i;
	}
      number_vars=i;
      support=(bdd *)mem_get_block((SIZE_T)((bddm->vars+1)*sizeof(bdd)));
      cmu_bdd_support(bddm, f, support);
      ok=1;
      for (i=0; ok && (var=support[i]); ++i)
	{
	  BDD_SETUP(var);
	  if (normalized_indexes[BDD_INDEX(bddm, var)] == BDD_MAX_INDEX)
	    {
	      cmu_bdd_warning("cmu_bdd_dump_bdd: incomplete support specified");
	      ok=0;
	    }
	}
      if (!ok)
	{
	  mem_free_block((pointer)normalized_indexes);
	  mem_free_block((pointer)support);
	  return (0);
	}
      mem_free_block((pointer)support);
      /* Everything checked now; barring I/O errors, we should be able to */
      /* write a valid output file. */
      h=bdd_new_hash_table(bddm, sizeof(long));
      FIREWALL1(bddm,
		if (retcode == BDD_IOERROR)
		  {
		    cmu_bdd_free_hash_table(h);
		    mem_free_block((pointer)normalized_indexes);
		    return (0);
		  }
		else
		  cmu_bdd_fatal("cmu_bdd_dump_bdd: got unexpected retcode");
		);
      index_size=bytes_needed(number_vars+1);
      bdd_mark_shared_nodes(bddm, f);
      next=0;
      bdd_number_shared_nodes(bddm, f, h, &next);
      node_number_size=bytes_needed(next);
      write(bddm, MAGIC_COOKIE, sizeof(long), fp);
      write(bddm, (unsigned long)number_vars, sizeof(bdd_index_type), fp);
      write(bddm, (unsigned long)next, sizeof(long), fp);
      cmu_bdd_dump_bdd_step(bddm, f, fp, h, normalized_indexes, index_size, node_number_size);
      cmu_bdd_free_hash_table(h);
      mem_free_block((pointer)normalized_indexes);
      return (1);
    }
  return (0);
}


static
unsigned long
#if defined(__STDC__)
read(int *error, int bytes, FILE *fp)
#else
read(error, bytes, fp)
     int *error;
     int bytes;
     FILE *fp;
#endif
{
  int c;
  long result;

  result=0;
  if (*error)
    return (result);
  while (bytes)
    {
      c=fgetc(fp);
      if (c == EOF)
	{
	  if (ferror(fp))
	    *error=BDD_UNDUMP_IOERROR;
	  else
	    *error=BDD_UNDUMP_EOF;
	  return (0l);
	}
      result=(result << 8)+c;
      --bytes;
    }
  return (result);
}


static long index_mask[]={0xffl, 0xffffl, 0xffffffl};


static
bdd
#if defined(__STDC__)
cmu_bdd_undump_bdd_step(cmu_bdd_manager bddm,
		    bdd *vars,
		    FILE *fp,
		    bdd_index_type number_vars,
		    bdd *shared,
		    long number_shared,
		    long *shared_so_far,
		    int index_size,
		    int node_number_size,
		    int *error)
#else
cmu_bdd_undump_bdd_step(bddm, vars, fp, number_vars, shared, number_shared, shared_so_far, index_size, node_number_size, error)
     cmu_bdd_manager bddm;
     bdd *vars;
     FILE *fp;
     bdd_index_type number_vars;
     bdd *shared;
     long number_shared;
     long *shared_so_far;
     int index_size;
     int node_number_size;
     int *error;
#endif
{
  long node_number;
  long encoding;
  bdd_index_type i;
  INT_PTR value1, value2;
  bdd v;
  bdd temp1, temp2;
  bdd result;

  i=read(error, index_size, fp);
  if (*error)
    return ((bdd)0);
  if (i == index_mask[index_size-1])
    {
      encoding=0xffffff00l+read(error, 1, fp);
      if (*error)
	return ((bdd)0);
      switch (encoding)
	{
	case TRUE_ENCODING:
	  return (BDD_ONE(bddm));
	case FALSE_ENCODING:
	  return (BDD_ZERO(bddm));
	case CONSTANT_ENCODING:
	  value1=read(error, sizeof(long), fp);
	  value2=read(error, sizeof(long), fp);
	  if (*error)
	    return ((bdd)0);
	  if ((result=cmu_mtbdd_get_terminal(bddm, value1, value2)))
	    return (result);
	  *error=BDD_UNDUMP_OVERFLOW;
	  return ((bdd)0);
	case POSVAR_ENCODING:
	case NEGVAR_ENCODING:
	  i=read(error, index_size, fp);
	  if (!*error && i >= number_vars)
	    *error=BDD_UNDUMP_FORMAT;
	  if (*error)
	    return ((bdd)0);
	  v=vars[i];
	  if (encoding == POSVAR_ENCODING)
	    return (v);
	  else
	    return (BDD_NOT(v));
	case POSNODE_ENCODING:
	case NEGNODE_ENCODING:
	  node_number=read(error, node_number_size, fp);
	  if (!*error && (node_number >= number_shared || !shared[node_number]))
	    *error=BDD_UNDUMP_FORMAT;
	  if (*error)
	    return ((bdd)0);
	  v=shared[node_number];
	  v=cmu_bdd_identity(bddm, v);
	  if (encoding == POSNODE_ENCODING)
	    return (v);
	  else
	    return (BDD_NOT(v));
	case NODELABEL_ENCODING:
	  node_number= *shared_so_far;
	  ++*shared_so_far;
	  v=cmu_bdd_undump_bdd_step(bddm, vars, fp, number_vars, shared, number_shared,
				shared_so_far, index_size, node_number_size, error);
	  shared[node_number]=v;
	  v=cmu_bdd_identity(bddm, v);
	  return (v);
	default:
	  *error=BDD_UNDUMP_FORMAT;
	  return ((bdd)0);
	}
    }
  if (i >= number_vars)
    {
      *error=BDD_UNDUMP_FORMAT;
      return ((bdd)0);
    }
  temp1=cmu_bdd_undump_bdd_step(bddm, vars, fp, number_vars, shared, number_shared,
			    shared_so_far, index_size, node_number_size, error);
  temp2=cmu_bdd_undump_bdd_step(bddm, vars, fp, number_vars, shared, number_shared,
			    shared_so_far, index_size, node_number_size, error);
  if (*error)
    {
      cmu_bdd_free(bddm, temp1);
      return ((bdd)0);
    }
  result=cmu_bdd_ite(bddm, vars[i], temp1, temp2);
  cmu_bdd_free(bddm, temp1);
  cmu_bdd_free(bddm, temp2);
  if (!result)
    *error=BDD_UNDUMP_OVERFLOW;
  return (result);
}


bdd
#if defined(__STDC__)
cmu_bdd_undump_bdd(cmu_bdd_manager bddm, bdd *vars, FILE *fp, int *error)
#else
cmu_bdd_undump_bdd(bddm, vars, fp, error)
     cmu_bdd_manager bddm;
     bdd *vars;
     FILE *fp;
     int *error;
#endif
{
  long i;
  bdd_index_type number_vars;
  long number_shared;
  int index_size;
  int node_number_size;
  bdd *shared;
  long shared_so_far;
  bdd v;
  bdd result;

  *error=0;
  for (i=0; vars[i]; ++i)
    if (cmu_bdd_type(bddm, vars[i]) != BDD_TYPE_POSVAR)
      {
	cmu_bdd_warning("cmu_bdd_undump_bdd: support is not all positive variables");
	return ((bdd)0);
      }
  if (read(error, sizeof(long), fp) != MAGIC_COOKIE)
    {
      if (!*error)
	*error=BDD_UNDUMP_FORMAT;
      return ((bdd)0);
    }
  number_vars=read(error, sizeof(bdd_index_type), fp);
  if (*error)
    return ((bdd)0);
  if (number_vars != i)
    {
      *error=BDD_UNDUMP_FORMAT;
      return ((bdd)0);
    }
  number_shared=read(error, sizeof(long), fp);
  if (*error)
    return ((bdd)0);
  index_size=bytes_needed(number_vars+1);
  node_number_size=bytes_needed(number_shared);
  if (number_shared < 0)
    {
      *error=BDD_UNDUMP_FORMAT;
      return ((bdd)0);
    }
  shared=(bdd *)mem_get_block((SIZE_T)(number_shared*sizeof(bdd)));
  for (i=0; i < number_shared; ++i)
    shared[i]=0;
  shared_so_far=0;
  result=cmu_bdd_undump_bdd_step(bddm, vars, fp, number_vars, shared, number_shared,
			     &shared_so_far, index_size, node_number_size, error);
  for (i=0; i < number_shared; ++i)
    if ((v=shared[i]))
      cmu_bdd_free(bddm, v);
  if (!*error && shared_so_far != number_shared)
    *error=BDD_UNDUMP_FORMAT;
  mem_free_block((pointer)shared);
  if (*error)
    {
      if (result)
	cmu_bdd_free(bddm, result);
      return ((bdd)0);
    }
  return (result);
}
