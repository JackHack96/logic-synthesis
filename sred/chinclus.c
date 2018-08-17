
#include "reductio.h"

ch_inclus(index, genclass, subclass
)
int index, genclass, subclass;

{
/* ch_inclus = 1 iff the chain implied by the c-prime class "index"
   is contained in the chain implied by the class referred to by
   (genclass,subclass) */

int         i;
int         incluso;
pset_family chain;
int         startchain;

/* generation of the chain implied by the class (genclass,subclass) -
   cardchain is the cardinality of the chain */
chain = chiusura(genclass, subclass);

/* startchain points to the beginning of the index-th chain
   in the array implied */
startchain = 0;
for (
i = 0;
i<index;
i++) startchain += firstchain.weight[i];
startchain++; /* the index-th chain header is jumped over */

i       = 0;
incluso = 1;
while ( i < firstchain.weight[index]-1 && incluso == 1 )
{

/*printf("chiamo la cl_in_chain con indice = %d\n", startchain + i);*/
if (
cl_in_chain(startchain
+i, chain) != 1 )
incluso = 0;
i++;
}

/*printf("ch_inclus = %d\n", incluso);*/
return(incluso);

}
