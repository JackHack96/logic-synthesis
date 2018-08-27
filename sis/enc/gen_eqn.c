#include "enc_int.h"
#include "sis.h"

extern bool enc_debug;

static void cnf_array_free();

static pset_family cnf_expand();

static int cnf_split();

static pset_family cnf_merge();

static bool special_cases();

static dic_family_t *cnf_to_prime();

/* prime generation by generating a CNF form and using the URP to
 * obtain a SOP */
dic_family_t *gen_eqn(dic_list, climit) dic_family_t *dic_list;
int climit;
{
  pset_family eqn_cov;
  array_t *eqn_arr;
  cnf_t *cnf;
  pset p1, p2;
  int n, i, j, *count;
  bool f1, f2;

  eqn_arr = array_alloc(cnf_t *, ALLOCSIZE);
  n = dic_list->dcount;
  /* set up count of variables */
  count = ALLOC(int, n);
  for (i = 0; i < n; i++) {
    count[i] = 0;
  }
  for (i = 0; i < n; i++) {
    p1 = GETDIC(dic_list, i);
    for (j = i + 1; j < n; j++) {
      p2 = GETDIC(dic_list, j);

      f1 = setp_disjoint(lhs_dic(p1), rhs_dic(p2));
      f2 = setp_disjoint(rhs_dic(p1), lhs_dic(p2));

      /* cannot merge these two dichotomies */
      if (!f1 || !f2) {
        cnf = ALLOC(cnf_t, 1);
        cnf->var1 = i;
        cnf->var2 = j;
        count[i]++;
        count[j]++;
        (void)array_insert_last(cnf_t *, eqn_arr, cnf);
      }
    }
  }

  /*
      (void) fprintf(sisout, "There are %-1d pairwise incompatibilities\n",
                              array_n(eqn_arr));
  */

  /* now apply URP to cnf_arr to get an SOP form */
  eqn_cov = cnf_expand(eqn_arr, count, n, climit);
  FREE(count);

  /* interpret cover to obtain list of prime dichotomies */
  /*
      (void) fprintf(sisout, "\nFinal cover size is %d\n", eqn_cov->count);
  */
  return cnf_to_prime(dic_list, eqn_cov, n);
}

static pset_family cnf_expand(cnf_arr, count, vars, limit) array_t *cnf_arr;
int *count;
int vars;
int limit;
{
  pset_family cov1, sop_cov;
  pset tmp, rhs_prod;
  array_t *lhs_arr;
  cnf_t *cnf;
  int n, split_var;

  /*
  if (enc_debug) {
      (void) fprintf(sisout, "cnf_expand:\n");
      for (n = 0 ; n < array_n(cnf_arr); n ++) {
          cnf = array_fetch(cnf_t *, cnf_arr, n);
          (void) fprintf(sisout, "(%-1d + %-1d)", cnf->var1, cnf->var2);
      }
      (void) fprintf(sisout, "\n");
  }
  */
  n = array_n(cnf_arr);
  if (n == 0) {
    (void)cnf_array_free(cnf_arr);
    sop_cov = sf_new(1, vars);
    tmp = set_new(vars);
    sop_cov = sf_addset(sop_cov, tmp);
    set_free(tmp);
    return sop_cov;
  }

  /* now apply URP */
  if (!special_cases(n, count, vars, &sop_cov)) {
    /* do a split and recur */
    split_var = cnf_split(cnf_arr, count, vars, &lhs_arr, &rhs_prod);
    (void)cnf_array_free(cnf_arr);
    /*
    if (enc_debug) {
        (void) fprintf(sisout, "split = %-1d\n", split_var);
    }
    */
    cov1 = cnf_expand(lhs_arr, count, vars, limit);
    sop_cov = cnf_merge(vars, split_var, rhs_prod, cov1);
    set_free(rhs_prod);
    sf_free(cov1);
  } else {
    (void)cnf_array_free(cnf_arr);
  }
  if (enc_debug) {
    (void)fprintf(sisout, " %-1d", sop_cov->count);
    (void)fflush(sisout);
    /* (void) sf_bm_print(sop_cov); */
  }
  if (sop_cov->count >= limit) {
    (void)fprintf(sisout, "Cover size exceeded\n");
    exit(0);
  }
  return sop_cov;
}

/* free the cnf forms and then the array of pointers */
static void cnf_array_free(cnf_arr) array_t *cnf_arr;
{
  cnf_t *cnf;
  int n, i;

  n = array_n(cnf_arr);
  for (i = 0; i < n; i++) {
    cnf = array_fetch(cnf_t *, cnf_arr, i);
    FREE(cnf);
  }
  array_free(cnf_arr);
}

/* choose a splitting variable and compute the left and right hand branches
 * of the tree. also update the variables that get multiplied on the right
 * hand side. the splitting variable is returned by the function */
static int cnf_split(cnf_arr, count, vars, lhs_arr, rhs_prod) array_t *cnf_arr;
int *count;
int vars;
array_t **lhs_arr;
pset *rhs_prod;
{
  cnf_t *cnf, *cnf_l;
  int n, i, max, max_i;

  /* find most frequently occurring variable */
  max = -1;
  for (i = 0; i < vars; i++) {
    if (count[i] > max) {
      max = count[i];
      max_i = i;
    }
  }

  *lhs_arr = array_alloc(cnf_t *, 0);
  *rhs_prod = set_new(vars);
  n = array_n(cnf_arr);
  for (i = 0; i < n; i++) {
    cnf = array_fetch(cnf_t *, cnf_arr, i);
    if (cnf->var1 == max_i) {
      count[max_i]--;
      count[cnf->var2]--;
      (void)set_insert((*rhs_prod), cnf->var2);
    } else if (cnf->var2 == max_i) {
      count[max_i]--;
      count[cnf->var1]--;
      (void)set_insert((*rhs_prod), cnf->var1);
    } else {
      cnf_l = ALLOC(cnf_t, 1);
      cnf_l->var1 = cnf->var1;
      cnf_l->var2 = cnf->var2;
      (void)array_insert_last(cnf_t *, *lhs_arr, cnf_l);
    }
  }
  return max_i;
}

/* merge the left and right covers - some check for merging with redundancy
 * removal. merging with single cube containment is done */
static pset_family cnf_merge(vars, split_var, rhs_prod, cov_l) int vars;
int split_var;
pset rhs_prod;
pset_family cov_l;
{
  pset_family sop;
  pset last, p, tmp;
  bool rem_contain;

  sop = sf_save(cov_l);

  /* cofactor - or rhs_prod elements */
  rem_contain = FALSE;
  foreach_set(sop, last, p) {

    /* set flag if rhs_prod is contained by any set */
    if (setp_implies(p, rhs_prod)) {
      rem_contain = TRUE;
      sf_free(sop);
      sop = sf_new(cov_l->count, vars);
      sop = sf_addset(sop, rhs_prod);
      break;
    }
    set_or(p, p, rhs_prod);
  }
  if (!rem_contain) {
    sop = sf_rev_contain(sop);
  }

  /* add lhs side */
  tmp = set_new(vars);
  foreach_set(cov_l, last, p) {
    if (setp_implies(rhs_prod, p)) {
      continue;
    }
    set_copy(tmp, p);
    set_insert(tmp, split_var);
    sop = sf_addset(sop, tmp);
  }
  set_free(tmp);
  return sop;
}

static bool special_cases(n_terms, count, vars, sop) int n_terms;
int *count;
int vars;
pset_family *sop;
{
  pset tmp;
  int i, j;

  for (i = 0; i < vars; i++) {
    if (count[i] == n_terms) {
      *sop = sf_new(2, vars);
      tmp = set_new(vars);
      set_insert(tmp, i);
      *sop = sf_addset(*sop, tmp);
      set_clear(tmp, vars);
      for (j = 0; j < vars; j++) {
        if ((count[j] != 0) && (j != i)) {
          set_insert(tmp, j);
        }
      }
      *sop = sf_addset(*sop, tmp);
      set_free(tmp);
      return TRUE;
    }
  }
  return FALSE;
}

static dic_family_t *cnf_to_prime(dic_list, cov, n) dic_family_t *dic_list;
pcover cov;
int n;
{
  dic_family_t *new;
  pset full, dic, d, last, p, pcomp;
  int i, base;
  unsigned int val;

  new = dic_family_alloc(n, dic_list->dset_elem);
  full = set_full(cov->sf_size);
  pcomp = set_new(cov->sf_size);
  dic = dic_new(dic_list->dset_elem);
  foreach_set(cov, last, p) {
    (void)set_clear(lhs_dic(dic), dic_list->dset_elem);
    (void)set_clear(rhs_dic(dic), dic_list->dset_elem);
    (void)set_xor(pcomp, full, p);
    foreach_set_element(pcomp, i, val, base) {
      d = GETDIC(dic_list, base);
      (void)set_or(lhs_dic(dic), lhs_dic(dic), lhs_dic(d));
      (void)set_or(rhs_dic(dic), rhs_dic(dic), rhs_dic(d));
    }
    (void)dic_family_add(new, dic);
  }
  set_free(full);
  set_free(pcomp);
  dic_free(dic);
  sf_free(cov);
  return new;
}
