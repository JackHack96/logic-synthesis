/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sred/connected.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:11 $
 *
 */
#include "reductio.h"

connected(adjacency,pSUBCOMP)
pset_family adjacency;
SUBCOMP *pSUBCOMP;

{
   /* The structure SUBCOMP includes compnum and cmpnum :
   the array compnum stores the connected components of the graph
   the integer cmpnum gives the number of connected components -
   compnum[i]=j iff node i+1 belongs to the j-th component of the graph */

int i;

/* clears the array compnum */
if (pSUBCOMP->compnum) free(pSUBCOMP->compnum);
MYALLOC (int, pSUBCOMP->compnum, ns);
for (i=0; i<ns; i++)
  pSUBCOMP->compnum[i] = 0;

pSUBCOMP->cmpnum = 0;


/* finds a new connected component calling the subroutine comp */
for (i=0; i<ns; i++)
{
  if (pSUBCOMP->compnum[i] == 0)
  {
    pSUBCOMP->cmpnum++;
    comp(i,adjacency,pSUBCOMP);
  }
}

/*
printf("\n");
printf("************************************\n");
printf("The graph has %d connected components\n",pSUBCOMP->cmpnum);
printf("Connected components array\n");
for (i=0; i<ns; i++)
  printf("%d",pSUBCOMP->compnum[i]);
printf("\n");
*/

}

