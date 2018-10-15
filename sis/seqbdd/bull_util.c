/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/seqbdd/bull_util.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:54 $
 *
 */
 /* file bull1.c release 1.1 */
 /* last modified: 8/21/90 at 17:02:17 */
#ifdef SIS
#include "sis.h"

bdd_t *input_cofactor(bdd_list,pi_list,mg,total_set,cache,leaves,options,andset,pt)
array_t *bdd_list;
array_t *pi_list;
bdd_manager *mg;
bdd_t *total_set;
st_table *cache;
st_table *leaves;
verif_options_t *options;
var_set_t *andset;
int pt;
{
  array_t *right_list, *left_list, *right_pi, *left_pi;
  int i,j;
  bdd_t *c, *cbar, *f1, *rtotal_set, *ltotal_set;
  bdd_t *bdd, *left_bdd, *right_bdd;
  bdd_t *tmpbdd;

  for (j= pt ; j< andset->n_elts; j++){
    if (var_set_get_elt(andset,j)){
    c= bdd_get_variable(mg,j);
    right_list = array_alloc(bdd_t *, 0);
    left_list = array_alloc(bdd_t *, 0);
    right_pi = array_alloc(bdd_t *, 0);
    left_pi = array_alloc(bdd_t *, 0);
    cbar= bdd_not(c);
    for (i= 0 ; i< array_n(bdd_list); i++){
       bdd = array_fetch(bdd_t *,bdd_list,i);
       if (bdd == NIL(bdd_t))
       continue;
       array_insert_last(bdd_t *, right_list, bdd_cofactor(bdd,c));
       array_insert_last(bdd_t *, left_list, bdd_cofactor(bdd,cbar));
       f1= array_fetch(bdd_t *, pi_list, i);
       array_insert_last(bdd_t *, right_pi,f1); 
       array_insert_last(bdd_t *, left_pi,f1); 
    }
    bdd_free(cbar);
    bdd_free(c);
    if (!options->does_verification){
    rtotal_set= bdd_dup(total_set);
    ltotal_set= bdd_dup(total_set);
    }
    right_bdd= input_cofactor(right_list, right_pi, mg, rtotal_set, cache,leaves,options,andset,j+1);
    left_bdd= input_cofactor(left_list, left_pi, mg, ltotal_set, cache,leaves,options,andset,j+1);
    tmpbdd= bdd_or(right_bdd, left_bdd, 1, 1);
    bdd_free(right_bdd);
    bdd_free(left_bdd);
    if (pt > 0){
    for (i= 0 ; i< array_n(bdd_list); i++){
      bdd = array_fetch(bdd_t *,bdd_list,i);
      if (bdd != NIL(bdd_t))
      bdd_free(bdd);
    }
    array_free(bdd_list);
    array_free(pi_list);
    if (!options->does_verification)
    bdd_free(total_set);
    }
    return(tmpbdd);
    }
  }
  return(bull_cofactor(bdd_list,pi_list,mg,total_set,cache,leaves,options));
}

array_t *disjoint_support_functions(list)
array_t *list;
{
  int row,column;
  int i,j;
  int flag;
  array_t *tmp;
  var_set_t *set, *fset, *nset;
  array_t *prtn_list;

  row= array_n(list);
  tmp= array_alloc(var_set_t *, row);
  for(i=0; i< row; i++){
    set= array_fetch(var_set_t *, list, i);
    nset= var_set_copy(set);
    array_insert(var_set_t *, tmp, i, nset);
  }
  set= array_fetch(var_set_t *, list, 0);
  column= set->n_elts;

  for(i=0; i< column; i++){
    flag=1;
    for(j=0; j< row; j++){
      set= array_fetch(var_set_t *, tmp, j);
      if (set != NIL(var_set_t) ){
        if (flag){
          if (var_set_get_elt(set, i)){
            fset= set;
            flag= 0;
            continue;
          }
        }
        if (var_set_get_elt(set, i)){
          fset= var_set_or(fset, fset, set);
          var_set_free(set);
          array_insert(var_set_t *, tmp, j, NIL(var_set_t ));
        }
      }
    }
  }

  prtn_list = array_alloc(var_set_t *, 0);
  for(j=0; j< row; j++){
    set= array_fetch(var_set_t *, tmp, j);
    if (set != NIL(var_set_t) ){
      nset= var_set_new(row);
      for(i=0; i< row; i++){
        fset= array_fetch(var_set_t *, list, i);
        if (var_set_intersect(set, fset))
          var_set_set_elt(nset, i);               
      }
      var_set_free(set);
      array_insert(var_set_t *, tmp, j, NIL(var_set_t ));
      array_insert_last(var_set_t *, prtn_list, nset);
    }
  }
  array_free(tmp);
  return(prtn_list);
}

bdd_t *range_2_compute(set, bdd_list, pi_list)
var_set_t *set;
array_t *bdd_list;
array_t *pi_list;
{
  int i,j;
  int k1,k2;
  bdd_t *bdd, *n1, *n2, *f1, *f2, *g, *out1, *out2, *f1_bar;
  bdd_t *c1, *c2;
  
  k1= -1;
  for(i=0,j=0; i< array_n(bdd_list); i++){
    bdd = array_fetch(bdd_t *,bdd_list,i);
    if(bdd == NIL(bdd_t))
      continue;
    if(!var_set_get_elt(set, j++))
      continue;
    if(k1 == -1){
      k1=i;
      f1= bdd;
    }else{
      k2=i;
      f2= bdd;
      break;
    }
  }
  n1= array_fetch(bdd_t *, pi_list, k1);
  n2= array_fetch(bdd_t *, pi_list, k2);
  f1_bar= bdd_not(f1);
  out1= bdd_cofactor(f2, f1);
  out2= bdd_cofactor(f2, f1_bar);
  bdd_free(f1_bar);
  if (bdd_is_tautology(out1, 1))
    c1= bdd_and(n1,n2,1,1);
  else if (bdd_is_tautology(out1, 0))
    c1= bdd_and(n1,n2,1,0);
  else
    c1= bdd_dup(n1);
  if (bdd_is_tautology(out2, 1))
    c2= bdd_and(n1,n2,0,1);
  else if (bdd_is_tautology(out2, 0))
    c2= bdd_and(n1,n2,0,0);
  else
    c2= bdd_not(n1);
  g= bdd_or(c1, c2, 1, 1);
  bdd_free(c1);
  bdd_free(c2);
  bdd_free(out1);
  bdd_free(out2);
  return(g);
}
#endif /* SIS */
