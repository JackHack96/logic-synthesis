
#include "reductio.h"

choose(parity)
int parity;
/* parity = 1 means that we are working on copertura1 -
   parity = 2 means that we are working on copertura2 */

{

/* CHOOSES A SET OF CHAINS AS A COVER OF THE FSM */

int  stin; /* stin is the number of states currently included in the cover */
pset chosenchain; /* chosenchain[i] = 1 iff the i-th chain has been
								  included in the cover being chosen */
pset currcover; /* array of the states covered by the current
								 "copertura" */
pset vect;
int  newstates, newclasses;
int  currgain, maxgain;
int  bestchain;
int  startchain, endchain;
int  i, j, k;
int  first;

/* printf("\n");
printf("-------------------\n");
printf("-CHOOSE-\n");
printf("-------------------\n");
*/

stin        = 0;
chosenchain = set_new(primes->count);
currcover   = set_new(ns);
vect        = set_new(ns);

/* while there are states not yet included in the cover */
while ( stin < ns )
{
/*printf("\n", stin);
printf("stin = %d\n", stin);*/
/* loop on the chains */
/* choose the chain that maximizes
     maxgain = #new_covered_states - #new_implied_classes_of_the_chain
     ( .e. maxgain = newstates - newclasses ) */
first = 1;

for (
i = 0;
i < primes->
count;
i++)
{

/* if the current chain has not yet been included */
if (!
is_in_set (chosenchain, i
))
{
/* calcolo di newstates */
newstates = 0;
for (
j = 0;
j<ns;
j++)
{
if (

is_in_set (GETSET(firstchain

.cover, i), j) &&
!
is_in_set (currcover, j
))
newstates++;
}
/*printf("newstates = %d\n", newstates);*/

/* chains not covering any new state are discarded */
if (newstates > 0)
{
/* computation of "newclasses" */
/* count how many new implied classes the i-th chain would bring
   to the covering "copertura" */
newclasses = 0;
/* startchain points to the beginning of the current chain -
   endchain to its end */
startchain = endchain = 0;
for (
j = 0;
j<i;
j++) startchain += firstchain.weight[j];
endchain = startchain + firstchain.weight[i];
/* cl_in_cop = 1 iff the j-th implied class is already included
   in "copertura" */
for (
j = startchain;
j<endchain;
j++)
{
set_copy (vect, GETSET(firstchain
.implied, j));
if (
cl_in_cop(vect, parity
) != 1) newclasses++;
}
/*printf("newclasses = %d\n", newclasses);*/

currgain = newstates - newclasses;

/* compare with maxgain */
if (first || currgain  > maxgain)
{
maxgain   = currgain;
bestchain = i;
first     = 0;
}
/* ties are broken against chains made of only one class */
if (currgain == maxgain && firstchain.weight[bestchain] == 1)
bestchain = i;

}
}
}

/* include in "copertura" the chain "bestchain" */
set_insert (chosenchain, bestchain
);
startchain = endchain = 0;
for (
j = 0;
j<bestchain;
j++) startchain += firstchain.weight[j];
endchain = startchain + firstchain.weight[bestchain];
/*printf("startchain = %d\n", startchain);
printf("endchain = %d\n", endchain);*/
for (
i = startchain;
i<endchain;
i++)
{
if (parity == 1)
{
set_copy (vect, GETSET(firstchain
.implied, i));
/* a class is added to "copertura" when not already included */
if (
cl_in_cop(vect, parity
) != 1 )
{
sf_addset (copertura1, GETSET(firstchain
.implied, i));
for (
j = 0;
j<ns;
j++)
{
if (

is_in_set (GETSET(firstchain

.implied, i), j) &&
!
is_in_set (currcover, j
))
{
set_insert (currcover, j
);
stin++;
}
}
}
}
else
{
set_copy (vect, GETSET(firstchain
.implied, i));
/* a class is added to "copertura" when not already included */
if (
cl_in_cop(vect, parity
) != 1 )
{
sf_addset (copertura2, GETSET(firstchain
.implied, i));
for (
j = 0;
j<ns;
j++)
{
if (

is_in_set (GETSET(firstchain

.implied, i), j) &&
!
is_in_set (currcover, j
))
{
set_insert (currcover, j
);
stin++;
}
}
}
}
}

}

set_free (chosenchain);
set_free (currcover);
set_free (vect);
}
