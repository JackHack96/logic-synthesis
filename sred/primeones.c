/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sred/primeones.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:12 $
 *
 */
#include "reductio.h"

primeones(index)
int index;

{
  /*  Counts the number of states included in the class primes[index][] */

  int i;
  int value;

  value = set_ord (GETSET (primes, index));

  return(value);
    
}
