
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
