
#include "reductio.h"

generate(chain, genclass, ingresso, class, pCHAINS
)
int    chain, genclass, ingresso;
pset   class;
CHAINS *pCHAINS;

{

/* compute the closure of the class implied[pointer] */

int i, pointer;

/* pointer gives the absolute address of the current generating class 
   "genclass" of the chain "chain" in the array pCHAINS->implied */
pointer = 0;
for (
i = 0;
i<chain;
i++) pointer += pCHAINS->weight[i];
pointer += genclass-1;
/*printf("absolute address of the current generating class  = %d\n", pointer);*/

set_clear (class, ns
);

for (
i = 0;
i<np;
i++) /* loop on the symbolic product terms */
{
if (itable[i].ilab-1 == ingresso)
{
if (

is_in_set (GETSET(pCHAINS

->implied, pointer), itable[i].plab-1) &&
itable[i].nlab != 0)
set_insert (class, itable[i]
.nlab-1);
}
}


}
