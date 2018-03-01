/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sred/qsort.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:12 $
 *
 */
#include "reductio.h"

myqsort(array,init,length)
int *array[2];
int init;
int length;

/* sort elements of array */
  
{
	int pivot; /* the pivot value */
	int pivotindex; /* the index of the pivot element of array */
	int k; /* beginning index for group of elements >= pivot */
	int i;


	pivotindex = myqspivot(array,init,length);
	if (pivotindex != 0) /* do nothing if array[init-1]=array[length-1] */
	{
	  pivot = array[1][pivotindex-1];
	  k = myqspart(array,init,length,pivot);
	  myqsort(array,init,k-1);
	  myqsort(array,k,length);
	}


}
