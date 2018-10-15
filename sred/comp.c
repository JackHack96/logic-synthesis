/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sred/comp.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:11 $
 *
 */
#include "reductio.h"

comp(vert,adjacency,pSUBCOMP)
int vert;
pset_family adjacency;
SUBCOMP *pSUBCOMP;

/* Assigns the nodes of the adjacency graph to its connected components -
   called by the routine connected */

{

int k;

pSUBCOMP->compnum[vert] = pSUBCOMP->cmpnum;

for (k=0; k<ns; k++)
{
  if (is_in_set (GETSET (adjacency, vert), k))
  {
    if (pSUBCOMP->compnum[k] == 0) comp(k,adjacency,pSUBCOMP);
  }
}

}
