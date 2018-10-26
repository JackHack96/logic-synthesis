/* Basic operation tests */


#include <stdio.h>
#if defined(__STDC__)
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include "bddint.h"


#define VARS 500


#define TT_BITS 32		/* Size of tt in bits */
#define TT_VARS 5		/* log2 of BITS */
/* Also see cofactor_masks below. */


#define ITERATIONS 20000	/* Number of trials to run */


#if defined(__STDC__)
extern void srandom(int);
extern long random(void);
extern char *mktemp(char *);
extern int unlink(char *);
#else
extern void srandom();
extern long random();
extern char *mktemp();
extern int unlink();
#endif


typedef unsigned long tt;	/* "Truth table" */


static cmu_bdd_manager bddm;


static bdd vars[VARS];
static bdd aux_vars[VARS];


static tt cofactor_masks[]=
{
  0xffff0000,
  0xff00ff00,
  0xf0f0f0f0,
  0xcccccccc,
  0xaaaaaaaa,
};


static
bdd
#if defined(__STDC__)
decode(int var, tt table)
#else
decode(var, table)
     int var;
     tt table;
#endif
{
  bdd temp1, temp2;
  bdd result;

  if (var == TT_VARS)
    return ((table & 1) ? cmu_bdd_one(bddm) : cmu_bdd_zero(bddm));
  temp1=decode(var+1, table >> (1 << (TT_VARS-var-1)));
  temp2=decode(var+1, table);
  result=cmu_bdd_ite(bddm, vars[var], temp1, temp2);
  cmu_bdd_free(bddm, temp1);
  cmu_bdd_free(bddm, temp2);
  return (result);
}


#define encoding_to_bdd(table) (decode(0, (table)))


static INT_PTR as_double_space[2];


static
double
#if defined(__STDC__)
as_double(INT_PTR v1, INT_PTR v2)
#else
as_double(v1, v2)
     INT_PTR v1;
     INT_PTR v2;
#endif
{
  as_double_space[0]=v1;
  as_double_space[1]=v2;
  return (*(double *)as_double_space);
}


static
void
#if defined(__STDC__)
as_INT_PTRs(double n, INT_PTR *r1, INT_PTR *r2)
#else
as_INT_PTRs(n, r1, r2)
     double n;
     INT_PTR *r1;
     INT_PTR *r2;
#endif
{
  (*(double *)as_double_space)=n;
  *r1=as_double_space[0];
  *r2=as_double_space[1];
}


static
char *
#if defined(__STDC__)
terminal_id_fn(cmu_bdd_manager bddm, INT_PTR v1, INT_PTR v2, pointer junk)
#else
terminal_id_fn(bddm, v1, v2, junk)
     cmu_bdd_manager bddm;
     INT_PTR v1;
     INT_PTR v2;
     pointer junk;
#endif
{
  static char result[100];

  sprintf(result, "%g", as_double(v1, v2));
  return (result);
}


static
void
#if defined(__STDC__)
print_bdd(bdd f)
#else
print_bdd(f)
     bdd f;
#endif
{
  cmu_bdd_print_bdd(bddm, f, bdd_naming_fn_none, terminal_id_fn, (pointer)0, stderr);
}


#if defined(__STDC__)
static
void
error(char *op, bdd result, bdd expected, ...)
{
  int i;
  va_list ap;
  bdd f;

  va_start(ap, expected);
  fprintf(stderr, "\nError: operation %s:\n", op);
  i=0;
  while (1)
    {
      f=va_arg(ap, bdd);
      if (f)
	{
	  ++i;
	  fprintf(stderr, "Argument %d:\n", i);
	  print_bdd(f);
	}
      else
	break;
    }
  fprintf(stderr, "Result:\n");
  print_bdd(result);
  fprintf(stderr, "Expected result:\n");
  print_bdd(expected);
  va_end(ap);
}
#else
static
void
error(va_alist)
     va_dcl
{
  int i;
  va_list ap;
  char *op;
  bdd result;
  bdd expected;
  bdd f;

  va_start(ap);
  op=va_arg(ap, char *);
  result=va_arg(ap, bdd);
  expected=va_arg(ap, bdd);
  fprintf(stderr, "\nError: operation %s:\n", op);
  i=0;
  while (1)
    {
      f=va_arg(ap, bdd);
      if (f)
	{
	  ++i;
	  fprintf(stderr, "Argument %d:\n", i);
	  print_bdd(f);
	}
      else
	break;
    }
  fprintf(stderr, "Result:\n");
  print_bdd(result);
  fprintf(stderr, "Expected result:\n");
  print_bdd(expected);
  va_end(ap);
}
#endif


static
tt
#if defined(__STDC__)
cofactor(tt table, int var, int value)
#else
cofactor(table, var, value)
     tt table;
     int var;
     int value;
#endif
{
  int shift;

  shift=1 << (TT_VARS-var-1);
  if (value)
    {
      table&=cofactor_masks[var];
      table|=table >> shift;
    }
  else
    {
      table&=~cofactor_masks[var];
      table|=table << shift;
    }
  return (table);
}


static
void
#if defined(__STDC__)
test_ite(bdd f1, tt table1, bdd f2, tt table2, bdd f3, tt table3)
#else
test_ite(f1, table1, f2, table2, f3, table3)
     bdd f1;
     tt table1;
     bdd f2;
     tt table2;
     bdd f3;
     tt table3;
#endif
{
  bdd result;
  tt resulttable;
  bdd expected;

  result=cmu_bdd_ite(bddm, f1, f2, f3);
  resulttable=(table1 & table2) | (~table1 & table3);
  expected=encoding_to_bdd(resulttable);
  if (result != expected)
    error("ITE", result, expected, f1, f2, f3, (bdd)0);
  cmu_bdd_free(bddm, result);
  cmu_bdd_free(bddm, expected);
}


static
void
#if defined(__STDC__)
test_and(bdd f1, tt table1, bdd f2, tt table2)
#else
test_and(f1, table1, f2, table2)
     bdd f1;
     tt table1;
     bdd f2;
     tt table2;
#endif
{
  bdd result;
  tt resulttable;
  bdd expected;

  result=cmu_bdd_and(bddm, f1, f2);
  resulttable=table1 & table2;
  expected=encoding_to_bdd(resulttable);
  if (result != expected)
    error("and", result, expected, f1, f2, (bdd)0);
  cmu_bdd_free(bddm, result);
  cmu_bdd_free(bddm, expected);
}


static
void
#if defined(__STDC__)
test_or(bdd f1, tt table1, bdd f2, tt table2)
#else
test_or(f1, table1, f2, table2)
     bdd f1;
     tt table1;
     bdd f2;
     tt table2;
#endif
{
  bdd result;
  tt resulttable;
  bdd expected;

  result=cmu_bdd_or(bddm, f1, f2);
  resulttable=table1 | table2;
  expected=encoding_to_bdd(resulttable);
  if (result != expected)
    error("or", result, expected, f1, f2, (bdd)0);
  cmu_bdd_free(bddm, result);
  cmu_bdd_free(bddm, expected);
}


static
void
#if defined(__STDC__)
test_xor(bdd f1, tt table1, bdd f2, tt table2)
#else
test_xor(f1, table1, f2, table2)
     bdd f1;
     tt table1;
     bdd f2;
     tt table2;
#endif
{
  bdd result;
  tt resulttable;
  bdd expected;

  result=cmu_bdd_xor(bddm, f1, f2);
  resulttable=table1 ^ table2;
  expected=encoding_to_bdd(resulttable);
  if (result != expected)
    error("xor", result, expected, f1, f2, (bdd)0);
  cmu_bdd_free(bddm, result);
  cmu_bdd_free(bddm, expected);
}


static
void
#if defined(__STDC__)
test_id_not(bdd f, tt table)
#else
test_id_not(f, table)
     bdd f;
     tt table;
#endif
{
  bdd result;
  tt resulttable;
  bdd expected;

  result=cmu_bdd_not(bddm, f);
  resulttable= ~table;
  expected=encoding_to_bdd(resulttable);
  if (result != expected)
    error("not", result, expected, f, (bdd)0);
  cmu_bdd_free(bddm, result);
  cmu_bdd_free(bddm, expected);
  result=cmu_bdd_identity(bddm, f);
  resulttable=table;
  expected=encoding_to_bdd(resulttable);
  if (result != expected)
    error("identity", result, expected, f, (bdd)0);
  cmu_bdd_free(bddm, result);
  cmu_bdd_free(bddm, expected);
}


static
void
#if defined(__STDC__)
test_compose(bdd f1, tt table1, bdd f2, tt table2)
#else
test_compose(f1, table1, f2, table2)
     bdd f1;
     tt table1;
     bdd f2;
     tt table2;
#endif
{
  int var;
  bdd result;
  tt resulttable;
  bdd expected;

  var=((unsigned long)random())%TT_VARS;
  result=cmu_bdd_compose(bddm, f1, vars[var], cmu_bdd_one(bddm));
  resulttable=cofactor(table1, var, 1);
  expected=encoding_to_bdd(resulttable);
  if (result != expected)
    error("restrict1", result, expected, f1, vars[var], (bdd)0);
  cmu_bdd_free(bddm, result);
  cmu_bdd_free(bddm, expected);
  result=cmu_bdd_compose(bddm, f1, vars[var], cmu_bdd_zero(bddm));
  resulttable=cofactor(table1, var, 0);
  expected=encoding_to_bdd(resulttable);
  if (result != expected)
    error("restrict0", result, expected, f1, vars[var], (bdd)0);
  cmu_bdd_free(bddm, result);
  cmu_bdd_free(bddm, expected);
  result=cmu_bdd_compose(bddm, f1, vars[var], f2);
  resulttable=(table2 & cofactor(table1, var, 1)) | (~table2 & cofactor(table1, var, 0));
  expected=encoding_to_bdd(resulttable);
  if (result != expected)
    error("compose", result, expected, f1, vars[var], f2, (bdd)0);
  cmu_bdd_free(bddm, result);
  cmu_bdd_free(bddm, expected);
}


static
void
#if defined(__STDC__)
test_qnt(bdd f, tt table)
#else
test_qnt(f, table)
     bdd f;
     tt table;
#endif
{
  int var1, var2;
  bdd assoc[3];
  bdd temp;
  bdd result;
  tt resulttable;
  bdd expected;

  var1=((unsigned long)random())%TT_VARS;
  do
    var2=((unsigned long)random())%TT_VARS;
  while (var1 == var2);
  assoc[0]=vars[var1];
  assoc[1]=vars[var2];
  assoc[2]=0;
  cmu_bdd_temp_assoc(bddm, assoc, 0);
  cmu_bdd_assoc(bddm, -1);
  if (random()%2)
    result=cmu_bdd_exists(bddm, f);
  else
    {
      temp=cmu_bdd_not(bddm, f);
      result=cmu_bdd_forall(bddm, temp);
      cmu_bdd_free(bddm, temp);
      temp=result;
      result=cmu_bdd_not(bddm, temp);
      cmu_bdd_free(bddm, temp);
    }
  resulttable=cofactor(table, var1, 1) | cofactor(table, var1, 0);
  resulttable=cofactor(resulttable, var2, 1) | cofactor(resulttable, var2, 0);
  expected=encoding_to_bdd(resulttable);
  if (result != expected)
    error("quantification", result, expected, f, vars[var1], vars[var2], (bdd)0);
  cmu_bdd_free(bddm, result);
  cmu_bdd_free(bddm, expected);
}


static
void
#if defined(__STDC__)
test_rel_prod(bdd f1, tt table1, bdd f2, tt table2)
#else
test_rel_prod(f1, table1, f2, table2)
     bdd f1;
     tt table1;
     bdd f2;
     tt table2;
#endif
{
  int var1, var2;
  bdd assoc[3];
  bdd result;
  tt resulttable;
  bdd expected;

  var1=((unsigned long)random())%TT_VARS;
  do
    var2=((unsigned long)random())%TT_VARS;
  while (var1 == var2);
  assoc[0]=vars[var1];
  assoc[1]=vars[var2];
  assoc[2]=0;
  cmu_bdd_temp_assoc(bddm, assoc, 0);
  cmu_bdd_assoc(bddm, -1);
  result=cmu_bdd_rel_prod(bddm, f1, f2);
  table1&=table2;
  resulttable=cofactor(table1, var1, 1) | cofactor(table1, var1, 0);
  resulttable=cofactor(resulttable, var2, 1) | cofactor(resulttable, var2, 0);
  expected=encoding_to_bdd(resulttable);
  if (result != expected)
    error("relational product", result, expected, f1, f2, vars[var1], vars[var2], (bdd)0);
  cmu_bdd_free(bddm, result);
  cmu_bdd_free(bddm, expected);
}


static
void
#if defined(__STDC__)
test_subst(bdd f1, tt table1, bdd f2, tt table2, bdd f3, tt table3)
#else
test_subst(f1, table1, f2, table2, f3, table3)
     bdd f1;
     tt table1;
     bdd f2;
     tt table2;
     bdd f3;
     tt table3;
#endif
{
  int var1, var2;
  bdd assoc[6];
  bdd result;
  tt resulttable;
  tt temp1, temp2, temp3, temp4;
  bdd expected;

  var1=((unsigned long)random())%TT_VARS;
  do
    var2=((unsigned long)random())%TT_VARS;
  while (var1 == var2);
  assoc[0]=vars[var1];
  assoc[1]=f2;
  assoc[2]=vars[var2];
  assoc[3]=f3;
  assoc[4]=0;
  assoc[5]=0;
  cmu_bdd_temp_assoc(bddm, assoc, 1);
  cmu_bdd_assoc(bddm, -1);
  result=cmu_bdd_substitute(bddm, f1);
  temp1=cofactor(cofactor(table1, var1, 1), var2, 1);
  temp2=cofactor(cofactor(table1, var1, 1), var2, 0);
  temp3=cofactor(cofactor(table1, var1, 0), var2, 1);
  temp4=cofactor(cofactor(table1, var1, 0), var2, 0);
  resulttable=table2 & table3 & temp1;
  resulttable|=table2 & ~table3 & temp2;
  resulttable|=~table2 & table3 & temp3;
  resulttable|=~table2 & ~table3 & temp4;
  expected=encoding_to_bdd(resulttable);
  if (result != expected)
    error("substitute", result, expected, f1, vars[var1], f2, vars[var2], f3, (bdd)0);
  cmu_bdd_free(bddm, result);
  cmu_bdd_free(bddm, expected);
}


static
void
#if defined(__STDC__)
test_inter_impl(bdd f1, tt table1, bdd f2, tt table2)
#else
test_inter_impl(f1, table1, f2, table2)
     bdd f1;
     tt table1;
     bdd f2;
     tt table2;
#endif
{
  bdd result;
  tt resulttable;
  bdd expected;
  bdd implies_result;

  result=cmu_bdd_intersects(bddm, f1, f2);
  resulttable=table1 & table2;
  expected=encoding_to_bdd(resulttable);
  implies_result=cmu_bdd_implies(bddm, result, expected);
  if (implies_result != cmu_bdd_zero(bddm))
    {
      error("intersection test", result, expected, f1, f2, (bdd)0);
      cmu_bdd_free(bddm, implies_result);
    }
  cmu_bdd_free(bddm, result);
  cmu_bdd_free(bddm, expected);
}


static
void
#if defined(__STDC__)
test_sat(bdd f, tt table)
#else
test_sat(f, table)
     bdd f;
     tt table;
#endif
{
  int var1, var2;
  bdd assoc[TT_VARS+1];
  bdd result;
  bdd temp1, temp2, temp3;

  if (f == cmu_bdd_zero(bddm))
    return;
  result=cmu_bdd_satisfy(bddm, f);
  temp1=cmu_bdd_not(bddm, f);
  temp2=cmu_bdd_intersects(bddm, temp1, result);
  if (temp2 != cmu_bdd_zero(bddm))
    error("intersection of satisfy result with negated argument", temp2, cmu_bdd_zero(bddm), f, (bdd)0);
  cmu_bdd_free(bddm, temp1);
  cmu_bdd_free(bddm, temp2);
  var1=((unsigned long)random())%TT_VARS;
  do
    var2=((unsigned long)random())%TT_VARS;
  while (var1 == var2);
  assoc[0]=vars[var1];
  assoc[1]=vars[var2];
  assoc[2]=0;
  cmu_bdd_temp_assoc(bddm, assoc, 0);
  cmu_bdd_assoc(bddm, -1);
  temp1=cmu_bdd_satisfy_support(bddm, result);
  temp2=cmu_bdd_not(bddm, result);
  temp3=cmu_bdd_intersects(bddm, temp2, temp1);
  if (temp3 != cmu_bdd_zero(bddm))
    error("intersection of satisfy support result with negated argument", temp3, cmu_bdd_zero(bddm), result, (bdd)0);
  cmu_bdd_free(bddm, result);
  cmu_bdd_free(bddm, temp1);
  cmu_bdd_free(bddm, temp2);
  cmu_bdd_free(bddm, temp3);
  temp1=cmu_bdd_compose(bddm, f, vars[var1], cmu_bdd_zero(bddm));
  temp2=cmu_bdd_compose(bddm, f, vars[var1], cmu_bdd_one(bddm));
  if (cmu_bdd_satisfying_fraction(bddm, temp1)+cmu_bdd_satisfying_fraction(bddm, temp2) !=
      2.0*cmu_bdd_satisfying_fraction(bddm, f))
    {
      fprintf(stderr, "\nError: operation satisfying fraction:\n");
      fprintf(stderr, "Argument:\n");
      print_bdd(f);
      fprintf(stderr, "Cofactor on:\n");
      print_bdd(vars[var1]);
    }
  cmu_bdd_free(bddm, temp1);
  cmu_bdd_free(bddm, temp2);
}


static
void
#if defined(__STDC__)
test_gen_cof(bdd f1, tt table1, bdd f2, tt table2)
#else
test_gen_cof(f1, table1, f2, table2)
     bdd f1;
     tt table1;
     bdd f2;
     tt table2;
#endif
{
  int var1, var2;
  bdd result;
  bdd temp1, temp2, temp3;
  tt resulttable;
  bdd expected;

  result=cmu_bdd_cofactor(bddm, f1, f2);
  temp1=cmu_bdd_xnor(bddm, result, f1);
  temp2=cmu_bdd_not(bddm, f2);
  temp3=cmu_bdd_or(bddm, temp1, temp2);
  if (temp3 != cmu_bdd_one(bddm))
    error("d.c. comparison of generalized cofactor", temp3, cmu_bdd_one(bddm), f1, f2, (bdd)0);
  cmu_bdd_free(bddm, result);
  cmu_bdd_free(bddm, temp1);
  cmu_bdd_free(bddm, temp2);
  cmu_bdd_free(bddm, temp3);
  var1=((unsigned long)random())%TT_VARS;
  do
    var2=((unsigned long)random())%TT_VARS;
  while (var1 == var2);
  temp1=cmu_bdd_not(bddm, vars[var2]);
  temp2=cmu_bdd_and(bddm, vars[var1], temp1);
  cmu_bdd_free(bddm, temp1);
  result=cmu_bdd_cofactor(bddm, f1, temp2);
  resulttable=cofactor(cofactor(table1, var1, 1), var2, 0);
  expected=encoding_to_bdd(resulttable);
  if (result != expected)
    error("generalized cofactor", result, expected, f1, temp2, (bdd)0);
  cmu_bdd_free(bddm, result);
  cmu_bdd_free(bddm, expected);
  cmu_bdd_free(bddm, temp2);
}


static
void
#if defined(__STDC__)
test_reduce(bdd f1, tt table1, bdd f2, tt table2)
#else
test_reduce(f1, table1, f2, table2)
     bdd f1;
     tt table1;
     bdd f2;
     tt table2;
#endif
{
  bdd result;
  bdd temp1, temp2, temp3;

  result=cmu_bdd_reduce(bddm, f1, f2);
  temp1=cmu_bdd_xnor(bddm, result, f1);
  temp2=cmu_bdd_not(bddm, f2);
  temp3=cmu_bdd_or(bddm, temp1, temp2);
  if (temp3 != cmu_bdd_one(bddm))
    error("d.c. comparison of reduce", temp3, cmu_bdd_one(bddm), f1, f2, (bdd)0);
  cmu_bdd_free(bddm, result);
  cmu_bdd_free(bddm, temp1);
  cmu_bdd_free(bddm, temp2);
  cmu_bdd_free(bddm, temp3);
}


static
bdd
#if defined(__STDC__)
apply_and(cmu_bdd_manager bddm, bdd *f, bdd *g, pointer env)
#else
apply_and(bddm, f, g, env)
     cmu_bdd_manager bddm;
     bdd *f;
     bdd *g;
     pointer env;
#endif
{
  bdd f1, g1;

  f1= *f;
  g1= *g;
  {
    if (f1 == BDD_ZERO(bddm))
      return (f1);
    if (g1 == BDD_ZERO(bddm))
      return (g1);
    if (f1 == BDD_ONE(bddm))
      return (g1);
    if (g1 == BDD_ONE(bddm))
      return (f1);
    if ((INT_PTR)f1 < (INT_PTR)g1)
      {
	*f=g1;
	*g=f1;
      }
    return ((bdd)0);
  }
}


static
void
#if defined(__STDC__)
test_apply(bdd f1, tt table1, bdd f2, tt table2)
#else
test_apply(f1, table1, f2, table2)
     bdd f1;
     tt table1;
     bdd f2;
     tt table2;
#endif
{
  bdd result;
  tt resulttable;
  bdd expected;

  result=bdd_apply2(bddm, apply_and, f1, f2, (pointer)0);
  resulttable=table1 & table2;
  expected=encoding_to_bdd(resulttable);
  if (result != expected)
    error("apply2", result, expected, f1, f2, (bdd)0);
  cmu_bdd_free(bddm, result);
  cmu_bdd_free(bddm, expected);
}


static
void
#if defined(__STDC__)
test_size(bdd f1, tt table1, bdd f2, tt table2)
#else
test_size(f1, table1, f2, table2)
     bdd f1;
     tt table1;
     bdd f2;
     tt table2;
#endif
{
  int i;
  long size;
  long profile[2*TT_VARS+1];
  bdd fs[3];

  size=cmu_bdd_size(bddm, f1, 1);
  cmu_bdd_profile(bddm, f1, profile, 1);
  for (i=0; i < 2*TT_VARS+1; ++i)
    size-=profile[i];
  if (size)
    {
      fprintf(stderr, "\nError: size count vs. profile sum:\n");
      fprintf(stderr, "Argument:\n");
      print_bdd(f1);
    }
  size=cmu_bdd_size(bddm, f1, 0);
  cmu_bdd_profile(bddm, f1, profile, 0);
  for (i=0; i < 2*TT_VARS+1; ++i)
    size-=profile[i];
  if (size)
    {
      fprintf(stderr, "\nError: no negout size count vs. profile sum:\n");
      fprintf(stderr, "Argument:\n");
      print_bdd(f1);
    }
  fs[0]=f1;
  fs[1]=f2;
  fs[2]=0;
  size=cmu_bdd_size_multiple(bddm, fs, 1);
  cmu_bdd_profile_multiple(bddm, fs, profile, 1);
  for (i=0; i < 2*TT_VARS+1; ++i)
    size-=profile[i];
  if (size)
    {
      fprintf(stderr, "\nError: multiple size count vs. multiple profile sum:\n");
      fprintf(stderr, "Argument 1:\n");
      print_bdd(f1);
      fprintf(stderr, "Argument 2:\n");
      print_bdd(f2);
    }
}


static
int
#if defined(__STDC__)
canonical_fn(cmu_bdd_manager bddm, INT_PTR v1, INT_PTR v2, pointer env)
#else
canonical_fn(bddm, v1, v2, env)
     cmu_bdd_manager bddm;
     INT_PTR v1;
     INT_PTR v2;
     pointer env;
#endif
{
  return (as_double(v1, v2) > 0);
}


static
void
#if defined(__STDC__)
transform_fn(cmu_bdd_manager bddm, INT_PTR v1, INT_PTR v2, INT_PTR *r1, INT_PTR *r2, pointer env)
#else
transform_fn(bddm, v1, v2, r1, r2, env)
     cmu_bdd_manager bddm;
     INT_PTR v1;
     INT_PTR v2;
     INT_PTR *r1;
     INT_PTR *r2;
     pointer env;
#endif
{
  as_INT_PTRs(-as_double(v1, v2), r1, r2);
}


static
bdd
#if defined(__STDC__)
terminal(double n)
#else
terminal(n)
     double n;
#endif
{
  INT_PTR v1, v2;

  as_INT_PTRs(n, &v1, &v2);
  return (cmu_mtbdd_get_terminal(bddm, v1, v2));
}


static
bdd
#if defined(__STDC__)
walsh_matrix(int n)
#else
walsh_matrix(n)
     int n;
#endif
{
  bdd temp1, temp2, temp3;
  bdd result;

  if (n == TT_VARS)
    return (terminal(1.0));
  temp1=walsh_matrix(n+1);
  temp2=mtbdd_transform(bddm, temp1);
  temp3=temp2;
  temp2=mtcmu_bdd_ite(bddm, aux_vars[n], temp3, temp1);
  cmu_bdd_free(bddm, temp3);
  result=mtcmu_bdd_ite(bddm, vars[n], temp2, temp1);
  cmu_bdd_free(bddm, temp1);
  cmu_bdd_free(bddm, temp2);
  return (result);
}


#define OP_MULT 1000l
#define OP_ADD 1100l


static
bdd
#if defined(__STDC__)
mtbdd_mult_step(cmu_bdd_manager bddm, bdd f, bdd g)
#else
mtbdd_mult_step(bddm, f, g)
     cmu_bdd_manager bddm;
     bdd f;
     bdd g;
#endif
{
  bdd_indexindex_type top_indexindex;
  bdd f1, f2;
  bdd g1, g2;
  bdd temp1, temp2;
  bdd result;
  INT_PTR u1, u2;
  INT_PTR v1, v2;
  
  BDD_SETUP(f);
  BDD_SETUP(g);
  if (BDD_IS_CONST(f) && BDD_IS_CONST(g))
    {
      cmu_mtbdd_terminal_value_aux(bddm, f, &u1, &u2);
      cmu_mtbdd_terminal_value_aux(bddm, g, &v1, &v2);
      as_INT_PTRs(as_double(u1, u2)*as_double(v1, v2), &u1, &u2);
      return (bdd_find_terminal(bddm, u1, u2));
    }
  if (BDD_OUT_OF_ORDER(f, g))
    BDD_SWAP(f, g);
  if (bdd_lookup_in_cache2(bddm, OP_MULT, f, g, &result))
    return (result);
  BDD_TOP_VAR2(top_indexindex, bddm, f, g);
  BDD_COFACTOR(top_indexindex, f, f1, f2);
  BDD_COFACTOR(top_indexindex, g, g1, g2);
  temp1=mtbdd_mult_step(bddm, f1, g1);
  temp2=mtbdd_mult_step(bddm, f2, g2);
  result=bdd_find(bddm, top_indexindex, temp1, temp2);
  bdd_insert_in_cache2(bddm, OP_MULT, f, g, result);
  return (result);
}


static
bdd
#if defined(__STDC__)
mtbdd_mult(cmu_bdd_manager bddm, bdd f, bdd g)
#else
mtbdd_mult(bddm, f, g)
     cmu_bdd_manager bddm;
     bdd f;
     bdd g;
#endif
{
  if (bdd_check_arguments(2, f, g))
    {
      FIREWALL(bddm);
      RETURN_BDD(mtbdd_mult_step(bddm, f, g));
    }
  return ((bdd)0);
}


static
bdd
#if defined(__STDC__)
mtbdd_add_step(cmu_bdd_manager bddm, bdd f, bdd g)
#else
mtbdd_add_step(bddm, f, g)
     cmu_bdd_manager bddm;
     bdd f;
     bdd g;
#endif
{
  bdd_indexindex_type top_indexindex;
  bdd f1, f2;
  bdd g1, g2;
  bdd temp1, temp2;
  bdd result;
  INT_PTR u1, u2;
  INT_PTR v1, v2;
  
  BDD_SETUP(f);
  BDD_SETUP(g);
  if (BDD_IS_CONST(f) && BDD_IS_CONST(g))
    {
      cmu_mtbdd_terminal_value_aux(bddm, f, &u1, &u2);
      cmu_mtbdd_terminal_value_aux(bddm, g, &v1, &v2);
      as_INT_PTRs(as_double(u1, u2)+as_double(v1, v2), &u1, &u2);
      return (bdd_find_terminal(bddm, u1, u2));
    }
  if (BDD_OUT_OF_ORDER(f, g))
    BDD_SWAP(f, g);
  if (bdd_lookup_in_cache2(bddm, OP_ADD, f, g, &result))
    return (result);
  BDD_TOP_VAR2(top_indexindex, bddm, f, g);
  BDD_COFACTOR(top_indexindex, f, f1, f2);
  BDD_COFACTOR(top_indexindex, g, g1, g2);
  temp1=mtbdd_add_step(bddm, f1, g1);
  temp2=mtbdd_add_step(bddm, f2, g2);
  result=bdd_find(bddm, top_indexindex, temp1, temp2);
  bdd_insert_in_cache2(bddm, OP_ADD, f, g, result);
  return (result);
}


static
bdd
#if defined(__STDC__)
mtbdd_add(cmu_bdd_manager bddm, bdd f, bdd g)
#else
mtbdd_add(bddm, f, g)
     cmu_bdd_manager bddm;
     bdd f;
     bdd g;
#endif
{
  if (bdd_check_arguments(2, f, g))
    {
      FIREWALL(bddm);
      RETURN_BDD(mtbdd_add_step(bddm, f, g));
    }
  return ((bdd)0);
}


static
bdd
#if defined(__STDC__)
transform(bdd f, bdd g, bdd *elim_vars)
#else
transform(f, g, elim_vars)
     bdd f;
     bdd g;
     bdd *elim_vars;
#endif
{
  int i;
  bdd temp1, temp2;
  bdd result;

  result=mtbdd_mult(bddm, f, g);
  for (i=0; i < TT_VARS; ++i)
    {
      temp1=cmu_bdd_compose(bddm, result, elim_vars[i], cmu_bdd_one(bddm));
      temp2=cmu_bdd_compose(bddm, result, elim_vars[i], cmu_bdd_zero(bddm));
      cmu_bdd_free(bddm, result);
      result=mtbdd_add(bddm, temp1, temp2);
      cmu_bdd_free(bddm, temp1);
      cmu_bdd_free(bddm, temp2);
    }
  return (result);
}


static
void
#if defined(__STDC__)
test_mtbdd(bdd f1, tt table1)
#else
test_mtbdd(f1, table1)
     bdd f1;
     tt table1;
#endif
{
  bdd wm;
  bdd temp1, temp2;
  bdd result;

  wm=walsh_matrix(0);
  temp1=transform(wm, f1, vars);
  temp2=temp1;
  temp1=transform(wm, temp2, aux_vars);
  cmu_bdd_free(bddm, wm);
  cmu_bdd_free(bddm, temp2);
  temp2=terminal(1.0/TT_BITS);
  result=mtbdd_mult(bddm, temp1, temp2);
  cmu_bdd_free(bddm, temp1);
  cmu_bdd_free(bddm, temp2);
  if (f1 != result)
    error("Walsh transformation and inverse", result, f1, (bdd)0);
  cmu_bdd_free(bddm, result);
}


static
void
#if defined(__STDC__)
test_swap(bdd f, tt table)
#else
test_swap(f, table)
     bdd f;
     tt table;
#endif
{
  int var1, var2;
  bdd result;
  tt resulttable;
  tt temp1, temp2, temp3, temp4;
  bdd expected;

  var1=((unsigned long)random())%TT_VARS;
  var2=((unsigned long)random())%TT_VARS;
  result=cmu_bdd_swap_vars(bddm, f, vars[var1], vars[var2]);
  temp1=cofactor(cofactor(table, var1, 1), var2, 1);
  temp2=cofactor(cofactor(table, var1, 1), var2, 0);
  temp3=cofactor(cofactor(table, var1, 0), var2, 1);
  temp4=cofactor(cofactor(table, var1, 0), var2, 0);
  resulttable=cofactor_masks[var2] & cofactor_masks[var1] & temp1;
  resulttable|=cofactor_masks[var2] & ~cofactor_masks[var1] & temp2;
  resulttable|=~cofactor_masks[var2] & cofactor_masks[var1] & temp3;
  resulttable|=~cofactor_masks[var2] & ~cofactor_masks[var1] & temp4;
  expected=encoding_to_bdd(resulttable);
  if (result != expected)
    error("swap variables", result, expected, f, vars[var1], vars[var2], (bdd)0);
  cmu_bdd_free(bddm, result);
  cmu_bdd_free(bddm, expected);
}


static
char filename[]="/tmp/tmpXXXXXX";


static
void
#if defined(__STDC__)
test_dump(bdd f, tt table)
#else
test_dump(f, table)
     bdd f;
     tt table;
#endif
{
  FILE *fp;
  int i, j;
  bdd dump_vars[TT_VARS+1];
  bdd temp;
  bdd result;
  int err;

  mktemp(filename);
  if (!(fp=fopen(filename, "w+")))
    {
      fprintf(stderr, "could not open temporary file %s\n", filename);
      exit(1);
    }
  unlink(filename);
  for (i=0; i < TT_VARS; ++i)
    dump_vars[i]=vars[i];
  dump_vars[i]=0;
  for (i=0; i < TT_VARS-1; ++i)
    {
      j=i+((unsigned long)random())%(TT_VARS-i);
      temp=dump_vars[i];
      dump_vars[i]=dump_vars[j];
      dump_vars[j]=temp;
    }
  if (!cmu_bdd_dump_bdd(bddm, f, dump_vars, fp))
    {
      fprintf(stderr, "Error: dump failure:\n");
      fprintf(stderr, "Argument:\n");
      print_bdd(f);
      fclose(fp);
      return;
    }
  rewind(fp);
  if (!(result=cmu_bdd_undump_bdd(bddm, dump_vars, fp, &err)) || err)
    {
      fprintf(stderr, "Error: undump failure: code %d:\n", err);
      fprintf(stderr, "Argument:\n");
      print_bdd(f);
      fclose(fp);
      return;
    }
  fclose(fp);
  if (result != f)
    error("dump/undump", result, f, f, (bdd)0);
  cmu_bdd_free(bddm, result);
}


static
void
#if defined(__STDC__)
check_leak(void)
#else
check_leak()
#endif
{
  bdd assoc[1];

  assoc[0]=0;
  cmu_bdd_temp_assoc(bddm, assoc, 0);
  cmu_bdd_gc(bddm);
  if (cmu_bdd_total_size(bddm) != 2*TT_VARS+1l)
    fprintf(stderr, "Memory leak somewhere...\n");
}


static
void
#if defined(__STDC__)
random_tests(int iterations)
#else
random_tests(iterations)
     int iterations;
#endif
{
  int i;
  tt table1, table2, table3;
  bdd f1, f2, f3;
  INT_PTR v1, v2;

  printf("Random operation tests...\n");
  bddm=cmu_bdd_init();
  cmu_bdd_node_limit(bddm, 5000);
  mtbdd_transform_closure(bddm, canonical_fn, transform_fn, (pointer)0);
  as_INT_PTRs(-1.0, &v1, &v2);
  mtcmu_bdd_one_data(bddm, v1, v2);
  vars[1]=cmu_bdd_new_var_last(bddm);
  vars[0]=cmu_bdd_new_var_first(bddm);
  vars[4]=cmu_bdd_new_var_after(bddm, vars[1]);
  vars[3]=cmu_bdd_new_var_before(bddm, vars[4]);
  vars[2]=cmu_bdd_new_var_after(bddm, vars[1]);
  for (i=0; i < 5; ++i)
    aux_vars[i]=cmu_bdd_new_var_after(bddm, vars[i]);
  for (i=0; i < iterations; ++i)
    {
      if ((i & 0xf) == 0)
	{
	  putchar('.');
	  fflush(stdout);
	}
      if ((i & 0x3ff) == 0x3ff)
	{
	  putchar('\n');
	  fflush(stdout);
	  check_leak();
	}
      table1=random();
      table2=random();
      table3=random();
      f1=encoding_to_bdd(table1);
      f2=encoding_to_bdd(table2);
      f3=encoding_to_bdd(table3);
      test_ite(f1, table1, f2, table2, f3, table3);
      test_and(f1, table1, f2, table2);
      test_or(f1, table1, f2, table2);
      test_xor(f1, table1, f2, table2);
      test_id_not(f1, table1);
      test_compose(f1, table1, f2, table2);
      test_qnt(f1, table1);
      test_rel_prod(f1, table1, f2, table2);
      test_subst(f1, table1, f2, table2, f3, table3);
      test_inter_impl(f1, table1, f2, table2);
      test_sat(f1, table1);
      test_gen_cof(f1, table1, f2, table2);
      test_reduce(f1, table1, f2, table2);
      test_apply(f1, table1, f2, table2);
      test_size(f1, table1, f2, table2);
      test_mtbdd(f1, table1);
      test_swap(f1, table1);
      if (i < 100)
	test_dump(f1, table1);
      cmu_bdd_free(bddm, f1);
      cmu_bdd_free(bddm, f2);
      cmu_bdd_free(bddm, f3);
    }
  putchar('\n');
  cmu_bdd_stats(bddm, stdout);
  cmu_bdd_quit(bddm);
}


void
#if defined(__STDC__)
main(void)
#else
main()
#endif
{
  srandom(1);
  random_tests(ITERATIONS);
}
