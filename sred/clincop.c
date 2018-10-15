/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sred/clincop.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:11 $
 *
 */
#include "reductio.h"

cl_in_cop(class,parity)
pset class;
int parity;

{
  /* checks whether "class" is already included in "copertura" */

  int found,ident;
  pset_family copertura;
  pset p;
  int i,j;

  found = 0;
  if (parity == 1)
  {
	copertura = copertura1;
  }
  else 
  {
	copertura = copertura2;
  }
  foreachi_set (copertura, i, p) {
	if (setp_implies (class, p)) {
	 return 1;
	}
  }
  return 0;

}
