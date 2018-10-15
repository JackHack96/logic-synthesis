/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sred/strbar.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:12 $
 *
 */
#include "reductio.h"

strbar(s)
char s[];

{
/* returns 1 if s is a string of all '-' (bars , meaning don't care) */
int i,allbar;

i = 0;
if (osymb) {
    allbar = !strcmp(s, ASTER);
}
else {
  allbar = 1;
  while (s[i] != '\0' && allbar == 1)
  {
    if (s[i] != '-') allbar = 0;
    i++;
  }
}  
return(allbar);

}
