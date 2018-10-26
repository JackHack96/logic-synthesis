

#include "reductio.h"

ibincompa()

 /* GENERATES ALL INCOMPATIBLE PAIRS OF STATES
incograph[i][j]=1 iff (si,sj) is an in incompatible pair of states
incograph[i][j]=0 otherwise
incograph is symmetric */

{

int i,j,newpair;
pset tmp;

/* CLEARS THE ARRAY incograph */
tmp = set_new (ns);
incograph = sf_new (ns, ns);
for (i=0; i<ns; i++)
{
	sf_addset (incograph, tmp);
}
set_free (tmp);

/* FINDS ALL OUTPUT INCOMPATIBLE PAIRS */
for (i=0; i<np; i++)
{
  for (j=0; j<i; j++)
  {
    if (boolcmp(itable[i].input,itable[j].input) == 0)
    { 
        if (strbar(itable[i].output) !=1 &&
          strbar(itable[j].output) !=1 &&
          strcmp(itable[i].output,itable[j].output) != 0)   
      {
        set_insert (GETSET (incograph, itable[i].plab-1), itable[j].plab-1);
        set_insert (GETSET (incograph, itable[j].plab-1), itable[i].plab-1);
      }
    }
  }
}
          

/* FINDS ALL INCOMPATIBLE PAIRS OF STATES IMPLYING THE OUTPUT
   INCOMPATIBLE PAIRS */
newpair = 1;
while (newpair == 1)
{ 
  newpair = 0;
  for (i=0; i<np; i++)
  {
    for (j=0; j<i; j++)
    {
      if (boolcmp(itable[i].input,itable[j].input) == 0)
      {
        if (is_in_set (GETSET (incograph, itable[i].nlab-1), itable[j].nlab-1))
        {
          if (! is_in_set (GETSET (incograph, itable[i].plab-1),
			itable[j].plab-1))
          {
            set_insert (GETSET(incograph, itable[i].plab-1), itable[j].plab-1);
            set_insert (GETSET(incograph, itable[j].plab-1), itable[i].plab-1);
            newpair = 1;
          }
        }
      }
    }
  }
}



connected(incograph,&incocomp);



}
