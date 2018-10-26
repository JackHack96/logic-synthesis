
#include "reductio.h"

primeones(index) int index;

{
  /*  Counts the number of states included in the class primes[index][] */

  int i;
  int value;

  value = set_ord(GETSET(primes, index));

  return (value);
}
