
/* SCCSID %W% */
#include "espresso/inc/espresso.h"
#include "global.h"
#include "max.h"
#include "merge.h"
#include "sis/util/util.h"
#include "stack.h"
#include "struct.h"
#include "user.h"

char time_msg[][14] = {"compatible",    "disjoint", "iso",          "maximal",
                       "class & bound", "prime",    "binate_cover", "map"};

int *b_cover;
int *c_cover;

say_solution() {
  sm_element *p;
  register i, j, m, k;
  long t_start;
  int tmp_states;
  long t_stop;

  if (!user.opt.verbose)
    exit(0);

  t_stop = util_cpu_time();

  (void)fprintf(stderr, "*** Solution for %s ***\n\n", user.fname);

  tmp_states = 0;
  for (i = 0; b_cover[i] != -1; i++) { /* For all closed cover */
    (void)fprintf(stderr, "state %2d: ", tmp_states);
    tmp_states++;

    (void)fprintf(stderr, "(");
    for (j = 0; j < SMASK(prime[b_cover[i]]->num); j++) {
      (void)fprintf(stderr, "%s",
                    states[prime[b_cover[i]]->state[j]]->state_name);
      if (j != SMASK(prime[b_cover[i]]->num) - 1)
        (void)fprintf(stderr, ",");
    }
    (void)fprintf(stderr, ")\n");
  }
  for (i = 0; i < user.stat.ostates; i++) {
    (void)fprintf(stderr, "state %2d: (%s)\n", tmp_states,
                  states[c_cover[i]]->state_name);
    tmp_states++;
  }
  (void)fprintf(stderr, "\n");
  for (i = 0; i < user.level; i++)
    (void)fprintf(stderr, "Lap time%d %s for %s\n", i + 1,
                  util_print_time(user.ltime[i + 1] - user.ltime[i]),
                  time_msg[i]);
  (void)fprintf(stderr, "Elapsed CPU Time %s\n",
                util_print_time(t_stop - t_start));
  if (user.opt.solution) {
    (void)fprintf(stderr, "Lower bound %d\n", user.stat.low);
    (void)fprintf(stderr, "Upper bound %d\n", user.stat.high);
  }
  (void)fprintf(
      stderr, "Mapping alternative in %d product %d total %f quality\n",
      user.stat.map_alternative, user.stat.map_total, user.stat.quality);
  (void)fprintf(stderr, "Number of shrinked states %d\n", user.stat.shrink);
  (void)fprintf(stderr, "Number of input before disjoint %d\n",
                user.stat.xinput);
  (void)fprintf(stderr, "Number of disjoint input %d\n", user.stat.disjoint);
  (void)fprintf(stderr, "Number of isomorphic states %d\n", user.stat.n_iso);
  (void)fprintf(stderr, "Number of compatible pairs %d\n",
                user.stat.n_compatible);
  (void)fprintf(stderr, "Number of Maximal Compatibles %d\n", user.stat.n_max);
  (void)fprintf(stderr, "Number of primes %d\n", p_num);
  (void)fprintf(stderr, "Number of initial states %d\n", num_st);
  (void)fprintf(stderr, "Number of incompatible states %d\n",
                user.stat.ostates);
  if (user.level > 7)
    (void)fprintf(stderr, "Number of products %d\n", user.stat.product);
  else
    (void)fprintf(stderr, "Number of products -\n");
  (void)fprintf(stderr, "Number of states after minimization %d\n",
                user.stat.rstates);
  exit(0);
}

say_max_only() {
  int i, j;

  for (i = 0; i < user.stat.n_max; i++) {
    (void)fprintf(stderr, "Maximal %d  (", i);
    for (j = 0; j < SMASK(max[i]->num); j++) {
      (void)fprintf(stderr, "%s", states[max[i]->state[j]]->state_name);
      if (j != SMASK(max[i]->num) - 1)
        (void)fprintf(stderr, ",");
    }
    (void)fprintf(stderr, ")\n");
  }
  /*
          user.ltime[2]=util_cpu_time();
          printf("Number of Maximal Compatibles %d\n",user.stat.n_max);
          printf("Elapsed CPU Time %s\n",
                  util_print_time(user.ltime[2]-user.ltime[0]));
          fflush(stdout);
  */
}

say_max() {
  int i, k, j, m;

  for (i = 0; i < user.stat.n_max; i++) {
    (void)fprintf(stderr, "Maximal %d  (", i);
    for (j = 0; j < SMASK(max[i]->num); j++) {
      (void)fprintf(stderr, "%s", states[max[i]->state[j]]->state_name);
      if (j != SMASK(max[i]->num) - 1)
        (void)fprintf(stderr, ",");
    }

    (void)fprintf(stderr, ")\nClass set : {");

    if (max[i]->class.many) {
      for (k = 0, m = 0; m < max[i]->class.many; k++) {
        /*
                                        printf("i %d, k %d, num
           %d\n",i,k,max[i]->class.imply[k]->num);
        */
        if (max[i]->class.imply[k]->num) {
          m++;
          (void)fprintf(stderr, "(");
          for (j = 0; j < max[i]->class.imply[k]->num; j++) {
            (void)fprintf(stderr, "%s",
                          states[max[i]->class.imply[k]->state[j]]->state_name);
            if (j != max[i]->class.imply[k]->num - 1)
              (void)fprintf(stderr, ",");
          }
          (void)fprintf(stderr, ")");
        }
      }
    }
    (void)fprintf(stderr, "}\n");
  }
  /*
          printf("Elapsed CPU Time %s\n",
                  util_print_time(user.ltime[2]-user.ltime[1]));
  */
}

say_prime() {
  int i;

  (void)fprintf(stderr, "\n");
  for (i = 0; i < p_num; i++) {
    write_prime(i);
    write_imply(i);
  }
  (void)fprintf(stderr, "\n");
}
