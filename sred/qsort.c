
#include "reductio.h"

myqsort(array, init, length) int *array[2];
int init;
int length;

/* sort elements of array */

{
  int pivot;      /* the pivot value */
  int pivotindex; /* the index of the pivot element of array */
  int k;          /* beginning index for group of elements >= pivot */
  int i;

  pivotindex = myqspivot(array, init, length);
  if (pivotindex != 0) /* do nothing if array[init-1]=array[length-1] */
  {
    pivot = array[1][pivotindex - 1];
    k = myqspart(array, init, length, pivot);
    myqsort(array, init, k - 1);
    myqsort(array, k, length);
  }
}
