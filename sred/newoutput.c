/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sred/newoutput.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:12 $
 *
 */


#include "reductio.h"

newoutput(pclass,ingresso)
int pclass,ingresso;

{
  /* finds the output asserted from "pclass" (the current state in the reduced 
     machine ) under input "ingresso" and returns its value             */

    int i,proutput,index,value;

    i = 0;
    proutput = 0;
    while (i<np  &&  proutput == 0 )
    {
      if ( strcmp(itable[i].input,itable[ingresso].input) == 0 )
      {
        if ( is_in_set (GETSET(copertura1, pclass-1), itable[i].plab-1))
        {
          if ( strbar(itable[i].output) != 1 ) { proutput = 1; index = i; }
        }
      }
      i++;
    }

    if ( proutput == 0 ) value = -1; else value = index;

    return(value);

}
