/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sred/qspart.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:12 $
 *
 */
#include "reductio.h"

myqspart(array,init,length,pivot)
int *array[2];
int init;
int length;
int pivot;

/* partitions array[init],...,array[length] so keys < pivot are at the left
and keys >= pivot are on the right . Returns the beginning of the group on
the right */

{
int l,r; /* cursors used during the permutation process */
int temp;

l = init;
r = length;
while (l<=r)
{
  /* swap array[1][r-1] & array[1][l-1] */
  temp = array[1][r-1];
  array[1][r-1] = array[1][l-1];
  array[1][l-1] = temp;
  /* swap array[0][r-1] & array[0][l-1] */
  temp = array[0][r-1];
  array[0][r-1] = array[0][l-1];
  array[0][l-1] = temp;

  /* Now the scan phase begins */
  while (array[1][l-1] < pivot) l++;
  while (array[1][r-1] >= pivot) r--;
}
return(l);

}
