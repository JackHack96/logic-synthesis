
#include "reductio.h"

solution() {

  /* FINDS A MINIMAL CLOSED COVERING THROUGH AN ITERATIVE PROCESS
     ( CREATING AT EACH STEP NEW C-PRIME COMPATIBLES ) */

  int parity; /* parity = 1 iff "choose" is going to work on "copertura1",
             parity = 2 iff "choose" is going to work on "copertura2" */
  int refin;  /* refin  = 1 iff a new refinement step is needed ( the current
             criterion is to refine until there are c-prime classes with
             at least two states in the last c-prime generation ) */
  int bound1, bound2; /* bound1 and bound2 point to the beginning and the end
                    of the last generation of c-primes in "primes" */
  int i, j, k;

  /* The maximal compatibles classes are copied at the beginning of "primes" */
  primes = sf_save(maxcompatibles);

  /* Call closure on the maximal compatibles */
  /*printf("\n");
  printf("********************************************\n");
  printf("Closure is called on the maximal compatibles\n");
  */
  closure(0, maxcompatibles->count);

  /* Choose a complete closed minimal covering from the current chains */
  parity = 1;
  copertura1 = sf_new(0, ns);
  copertura2 = sf_new(0, ns);
  choose(parity);

  /* copertura1->count is the cardinality of the smallest covering found -
     it represents the objective function to minimize */

  /* The iterative prime refinement procedure is computationally heavy
     and needs an awful amount of available memory . It is better to use
     it only for medium-sized machines ( up to 32 states ) .
     A future revision of the program will try to improve it , in order
     to cope in a sophisticated way with machines of arbitrary size       */
  /* ITERATIVE PRIME REFINEMENT */

  /* while there are still classes from which to extract c-primes,
     i.e. classes with at least two states ( refin = 1 )  -
     and the lower bound has not yet been reached */

  /*
  printf("\n");
  printf("=======================================\n");
  printf("We are in the iterative refinement loop\n");
  printf("=======================================\n");
  printf("\n");
  */

  bound1 = 0;
  bound2 = primes->count;
  refin = 1;
  while (refin == 1 && copertura1->count > maxcompatibles->count) {

    /* The coming while controls the iteration of the refinement step */
    refin = 0;
    i = bound1;
    while (i < bound2 & refin == 0) {
      /* "primeones" returns the # of classes in primes[i][] */
      if (primeones(i) >= 2)
        refin = 1;
      i++;
    }

    /* extract new c-primes from the last c-prime generation ( when it
       contains classes with at least two states ) */
    for (i = bound1; i < bound2; i++) {
      if (primeones(i) >= 2) {
        /* examine all subsets with cardinality smaller of one
           of the last c-prime generation */
        for (j = 0; j < ns; j++) {
          /* j is the subclass index */
          /* if the j-th state is in the i-th c-prime */
          if (is_in_set(GETSET(primes, i), j)) {
            /* if the current subclass is new and is c-prime
               (the current subclass is primes[i][] with 0 in the j-th
               position, i.e. without the j-th state) */
            if (newprime(i, j) == 1 && prime(i, j) == 1) {
              /* add this c-prime subclass to the "primes" array */
              sf_addset(primes, GETSET(primes, i));
              set_remove(GETSET(primes, primes->count - 1), j);
              /* compute the implied chain of this new c-prime class */
              closure(primes->count - 1, primes->count);
            }
          }
        }
      }
    }
    /* bound1 and bound2 are updated to the last c-prime generation */
    bound1 = bound2;
    bound2 = primes->count;

    parity = 2;
    /* compute the covering from the new c-prime set */
    /* clears the last covering stored in copertura2 */
    sf_free(copertura2);
    copertura2 = sf_new(0, ns);
    if (refin == 1)
      choose(parity);
    /* the new covering is accepted only if it has a cardinality smaller
       than the current solution */
    /* the solution covering is stored in copertura1 - the provisional
       one is stored in copertura2 */
    if (copertura2->count > 0 && copertura2->count < copertura1->count) {
      sf_free(copertura1);
      copertura1 = sf_save(copertura2);
    }
  }
}
