
#include "reductio.h"

exist(chain, class, pCHAINS) int chain;
pset class;
CHAINS *pCHAINS;

{

  /* checks whether "class" is already included in "chain" */

  int found, ident, startchain;
  int i, j;

  /* startchain points to the beginning of the current chain
     in the array implied */
  startchain = 0;
  for (i = 0; i < chain; i++)
    startchain += pCHAINS->weight[i];

  i = 0;
  found = 0;
  while (i < pCHAINS->weight[chain] && found == 0) {
    j = 0;
    if (setp_implies(class, GETSET(pCHAINS->implied, startchain + i))) {
      found = 1;
    }
    i++;
  }

  return (found);
}
