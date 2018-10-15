/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sred/newprime.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:12 $
 *
 */
#include "reductio.h"

newprime(genclass,subclass)
int genclass,subclass;

{
  /* newprime = 1 iff the class (genclass, subclass)
     does not already exist in the array primes[][] */

  int i,j;
  int found,new,ident;
  pset class, p;

  class = set_new (ns);
  /* compute class (genclass, subclass) */
  set_copy (class, GETSET (primes, genclass));
  set_remove (class, subclass);
  
  new = 1;
  foreachi_set (primes, i, p) {
	if (setp_equal (p, class)) {
		  new = 0;
		break;
	}
  }
  return(new);

}
