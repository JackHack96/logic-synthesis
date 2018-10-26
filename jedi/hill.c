
/*
 * Symbolic encoding program for compiling a symbolic
 * description into a binary representation.  Target
 * is multi-level logic synthesis
 *
 * History:
 *
 * Bill Lin
 * University of California, Berkeley
 * Comments to billlin@ic.Berkeley.EDU
 *
 * Copyright (c) 1989 Bill Lin, UC Berkeley CAD Group
 *     Permission is granted to do anything with this
 *     code except sell it or remove this message.
 */

#include "inc/jedi.h"
#include "inc/rp.h"

#define DISPLACE1 0   /* flag for displacement */
#define DISPLACE2 1   /* flag for displacement */
#define INTERCHANGE 2 /* flag for interchange */

/*
 * global variables
 */
int move_type;       /* move type for SA */
int random1;         /* random variable for SA */
int random2;         /* random variable for SA */
int currentEnumtype; /* states the current type being encoded */

/*
 * external declarations
 */
double rint();                /* math.h */
char *int_to_binary(); /* util.c */
int binary_to_int();   /* util.c */

/*
 * forward declarations
 */
int opt_embedding();

int cost_function();

int generate_function();

int accept_function();

int reject_function();

opt_embedding() {
  int i;

  for (i = 0; i < ne; i++) {
    if (verboseFlag) {
      (void)fprintf(stderr, "\nencoding type (%s) ...\n\n", enumtypes[i].name);
    }

    /*
     * encode the type if it has weights on it
     */
    if (enumtypes[i].input_flag || enumtypes[i].output_flag) {
      currentEnumtype = i;

      (void)combinatorial_optimize(
          cost_function, generate_function, accept_function, reject_function,
          stdin, stderr, stderr, beginningStates, endingStates,
          startingTemperature, maximumTemperature, enumtypes[i].ns - 1,
          verboseFlag);
    }
  }
} /* end of opt_embedding */

cost_function(ret_cost) double *ret_cost;
{
  int i, j, k, m;
  int cost;
  int inc_cost;
  int dist;
  int we;
  int ns;

  cost = 0;
  ns = enumtypes[currentEnumtype].ns;
  for (k = 0; k < ns; k++) {
    for (m = 0; m < ns; m++) {
      if (k < m) {
        i = enumtypes[currentEnumtype].symbols[k].code_ptr;
        j = enumtypes[currentEnumtype].symbols[m].code_ptr;
        dist = enumtypes[currentEnumtype].distances[i][j];

        we = enumtypes[currentEnumtype].links[k][m].weight;
        inc_cost = we * (dist - 1);
        cost += inc_cost;
      }
    }
  }

  *ret_cost = (double)cost;
  return SA_OK;
} /* end of cost_function */

generate_function(range) int range;
{
  Boolean continue_flag = TRUE;
  Boolean cond1, cond2, cond3;
  char *tmp_code;
  int num_codes;
  int num_bits;
  int ran;
  int c, d;
  int tmp_symbol_ptr;

  /*
   * set num_codes and num_bits for the
   * current enum type
   */
  num_codes = enumtypes[currentEnumtype].nc;
  num_bits = enumtypes[currentEnumtype].nb;

  /*
   * pick two valid codes to exchange
   */
  while (continue_flag) {
    random1 = int_random_generator(0, num_codes - 1);
    if (range == SA_FULL_RANGE) {
      random2 = int_random_generator(0, num_codes - 1);
    } else {
      /*
       * short range just toggle one bit
       */
      tmp_code = int_to_binary(random1, num_bits);
      ran = int_random_generator(0, num_bits - 1);
      if (tmp_code[ran] == '1') {
        tmp_code[ran] = '0';
      } else if (tmp_code[ran] == '0') {
        tmp_code[ran] = '1';
      } else {
        (void)exit(-1);
      }
      random2 = binary_to_int(tmp_code, num_bits);
      FREE(tmp_code);
    }
    cond1 = (random1 == random2);
    cond2 = (!enumtypes[currentEnumtype].codes[random1].assigned);
    cond3 = (!enumtypes[currentEnumtype].codes[random2].assigned);
    continue_flag = (cond1 || (cond2 && cond3));
  }

  if (!enumtypes[currentEnumtype].codes[random1].assigned) {
    /*
     * displace into random code 1 if
     * it is unassigned yet
     */
    move_type = DISPLACE1;
    enumtypes[currentEnumtype].codes[random1].assigned = TRUE;
    c = enumtypes[currentEnumtype].codes[random1].symbol_ptr =
        enumtypes[currentEnumtype].codes[random2].symbol_ptr;
    enumtypes[currentEnumtype].symbols[c].code_ptr = random1;
    enumtypes[currentEnumtype].codes[random2].assigned = FALSE;
    enumtypes[currentEnumtype].codes[random2].symbol_ptr = 0;
  } else if (!enumtypes[currentEnumtype].codes[random2].assigned) {
    /*
     * displace into random code 2 if
     * it is unassigned yet
     */
    move_type = DISPLACE2;
    enumtypes[currentEnumtype].codes[random2].assigned = TRUE;
    c = enumtypes[currentEnumtype].codes[random2].symbol_ptr =
        enumtypes[currentEnumtype].codes[random1].symbol_ptr;
    enumtypes[currentEnumtype].symbols[c].code_ptr = random2;
    enumtypes[currentEnumtype].codes[random1].assigned = FALSE;
    enumtypes[currentEnumtype].codes[random1].symbol_ptr = 0;
  } else {
    /*
     * interchange the two code assignments
     */
    move_type = INTERCHANGE;
    tmp_symbol_ptr = enumtypes[currentEnumtype].codes[random1].symbol_ptr;
    c = enumtypes[currentEnumtype].codes[random1].symbol_ptr =
        enumtypes[currentEnumtype].codes[random2].symbol_ptr;
    d = enumtypes[currentEnumtype].codes[random2].symbol_ptr = tmp_symbol_ptr;
    enumtypes[currentEnumtype].symbols[c].code_ptr = random1;
    enumtypes[currentEnumtype].symbols[d].code_ptr = random2;
  }

  return SA_OK;
} /* end of generate_function */

accept_function() {
  /* do nothing */
  return SA_OK;
} /* end of accept_function */

reject_function() {
  int c, d;
  int tmp_symbol_ptr;

  if (move_type == DISPLACE2) {
    enumtypes[currentEnumtype].codes[random1].assigned = TRUE;
    c = enumtypes[currentEnumtype].codes[random1].symbol_ptr =
        enumtypes[currentEnumtype].codes[random2].symbol_ptr;
    enumtypes[currentEnumtype].symbols[c].code_ptr = random1;
    enumtypes[currentEnumtype].codes[random2].assigned = FALSE;
    enumtypes[currentEnumtype].codes[random2].symbol_ptr = 0;
  } else if (move_type == DISPLACE1) {
    enumtypes[currentEnumtype].codes[random2].assigned = TRUE;
    c = enumtypes[currentEnumtype].codes[random2].symbol_ptr =
        enumtypes[currentEnumtype].codes[random1].symbol_ptr;
    enumtypes[currentEnumtype].symbols[c].code_ptr = random2;
    enumtypes[currentEnumtype].codes[random1].assigned = FALSE;
    enumtypes[currentEnumtype].codes[random1].symbol_ptr = 0;
  } else {
    /*
     * interchange back the encodings
     */
    tmp_symbol_ptr = enumtypes[currentEnumtype].codes[random1].symbol_ptr;
    c = enumtypes[currentEnumtype].codes[random1].symbol_ptr =
        enumtypes[currentEnumtype].codes[random2].symbol_ptr;
    d = enumtypes[currentEnumtype].codes[random2].symbol_ptr = tmp_symbol_ptr;
    enumtypes[currentEnumtype].symbols[c].code_ptr = random1;
    enumtypes[currentEnumtype].symbols[d].code_ptr = random2;
  }

  return SA_OK;
} /* end of reject_function */
