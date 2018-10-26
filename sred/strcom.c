
#include "reductio.h"

strcom(s,t)
char s[], t[];

{

/* return <0 if s<t, 0 if s==t, >0 if s>t */

int i;

i=0;

while (s[i] == '-' || t[i] == '-' || s[i] == t[i])
  if (s[i++] == '\0')
    return(0);
return(s[i] - t[i]);

}
