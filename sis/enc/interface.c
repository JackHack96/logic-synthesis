#include "enc_int.h"
#include "sis.h"

extern bool enc_debug;

/* ARGSUSED */
int com_enc_sat(cons, nsymb, codes) cons_t **cons;
int nsymb;
char ***codes;
{
  dic_family_t *seed_list, *dic_list, *prime_list, *uncover_list;
  dic_family_t *filter_list, *satisfy_list, *temp_list1;
  sm_matrix *table;
  sm_row *cover;
  bool **dom_closure;
  int i, code_length;

  long start_time, elapse_time, util_cpu_time();

  enc_debug = FALSE;

  start_time = util_cpu_time();
  dom_closure = transitive_closure(cons, nsymb);
  for (i = 0; i < nsymb; i++) {
    if (dom_closure[i][i]) {
      free_closure(dom_closure, nsymb);
      return 0;
    }
  }

  (void)read_cons(nsymb + 1, nsymb, cons, &seed_list);
  if ((seed_list == NIL(dic_family_t)) || (seed_list->dcount == 0)) {
    (void)fprintf(stdout, "No constraints\n");
  }

  dic_list = reduce_seeds(gen_uniq(seed_list));
  (void)dic_family_free(seed_list);
  temp_list1 = pre_filter_dic(dom_closure, dic_list, cons);
  if (enc_debug) {
    fprintf(stdout, "Raised seeds are\n");
    dic_family_print(temp_list1);
    fprintf(stdout, "END Raised seeds are\n");
  }
  filter_list = reduce_seeds(temp_list1);
  if (enc_debug) {
    fprintf(stdout, "Reduced seeds are\n");
    dic_family_print(filter_list);
  }
  prime_list = gen_eqn(filter_list, LIMIT);
  (void)dic_family_free(filter_list);
  satisfy_list = filter_dic(dom_closure, prime_list, cons);
  free_closure(dom_closure, nsymb);
  (void)dic_family_free(prime_list);
  printf("===***===Valid primes are %d\n", satisfy_list->dcount);

  uncover_list = dic_to_sm(&table, satisfy_list, dic_list);
  if (uncover_list->dcount != 0) {
    if (enc_debug) {
      (void)fprintf(stdout, "uncovered seeds are:\n");
      (void)dic_family_print(uncover_list);
      (void)fprintf(stdout, "face constraints not satisfiable\n");
    }
    (void)dic_family_free(dic_list);
    (void)dic_family_free(satisfy_list);
    (void)dic_family_free(uncover_list);
    sm_free(table);
    return 0;
  }

  cover = sm_minimum_cover(table, NIL(int), 1 /* heuristic */, 0);
  code_length = cover->length;
  *codes = derive_codes(table, cover, nsymb);

  (void)dic_family_free(uncover_list);
  (void)dic_family_free(dic_list);
  (void)dic_family_free(satisfy_list);
  sm_row_free(cover);
  sm_free(table);
  elapse_time = util_cpu_time() - start_time;
  printf("===***===Encoding time %s\n", util_print_time(elapse_time));
  return code_length;
}
