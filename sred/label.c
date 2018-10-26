
#include "reductio.h"

label()
{

/* gives a label to every state , symbolic input and symbolic output */

int i,j,k;

/*
printf("\n");
printf("**************************\n");
printf("LABELING\n");
*/

/* gives a label to every present state */
for (k=0; k<np; k++)
{
  if (strcmp(itable[k].pstate , ASTER) == 0) continue;

  for (j=0; j<k; j++)
  {
    if (strcmp(itable[k].pstate , itable[j].pstate) == 0)
    { 
      itable[k].plab = itable[j].plab;
      goto contin1; 
    }
  }
  
  ns++;
  itable[k].plab = ns;
  MYREALLOC (char*, slab, slab_size, ns - 1);
  slab[ns-1] = itable[k].pstate;
  /*printf("The state %s is labelled %d\n", itable[k].pstate , ns);*/
  contin1: ;
}

/* gives a label to every next state not yet appeared as a present state */
for (k=0; k<np; k++)
{
  if (strcmp(itable[k].nstate , ASTER) == 0) continue;

  for (j=0; j<np; j++)
  {
    if (strcmp(itable[k].nstate , itable[j].pstate) == 0)
    {
      itable[k].nlab = itable[j].plab;
      goto contin2;
    }
  }

  for (j=0; j<k; j++)
  {
    if (strcmp(itable[k].nstate , itable[j].nstate)==0)
    {
      itable[k].nlab = itable[j].nlab;
      goto contin2;
    }
  }

  ns++ ;
  itable[k].nlab = ns;
  /*printf("The state %s is labelled %d\n", itable[k].nstate , ns);*/
  contin2: ;
}


/* gives a label to every symbolic input */
if (isymb)
{
  ni = 0;
  for (k=0; k<np;  k++)
  {
    if (strcmp(itable[k].input , ASTER)==0) continue;

    for (j=0; j<k; j++)
    {
      if (strcmp(itable[k].input , itable[j].input)==0)
      {
        itable[k].ilab = itable[j].ilab;
        goto contin3;
      }
    }

    ni++ ;
    itable[k].ilab = ni;
    /*printf("The input %s is labelled %d\n", itable[k].input , ni);*/
    contin3: ;
  }
}


/* gives a label to every symbolic output */
if (osymb) 
{
  no = 0;
  for (k=0; k<np; k++)
  {
    if (strcmp(itable[k].output , ASTER) == 0) continue;

    for (j=0; j<k; j++)
    {
      if (strcmp(itable[k].output , itable[j].output) == 0)
      {
        itable[k].olab = itable[j].olab;
        goto contin4;
      }
    }

    no++ ; 
    itable[k].olab = no;
    /*printf("The output %s is labelled %d\n", itable[k].output , no);*/
    contin4: ;
  }
}

}
