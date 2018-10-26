
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
