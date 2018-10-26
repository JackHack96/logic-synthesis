
#include "reductio.h"

cl_in_chain(indice,chain)
int indice;
pset_family chain;

{
  /* cl_in_chain = 1 iff the class indice in the array implied
     is included in chain */

  int i,j;
  pset p;

  foreachi_set (chain, i, p) {
  /* the chain header is jumped over */
  if (i == 0) continue;
  if (setp_implies (GETSET (firstchain.implied, indice), p)) {
	return 1;
  }
  }
  return 0;
}
