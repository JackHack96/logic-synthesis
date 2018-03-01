/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sred/existdf.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:11 $
 *
 */
#include "reductio.h"

existdf(class,chain)
pset class;
pset_family chain;

{
  /* checks whether "class" is already included in "chain" */

  pset p;
  int found,ident;
  int i,j;

  foreachi_set (chain, i, p) {
	if (setp_implies (class, p)) {
		return 1;
	}
  }
  return 0;
}
