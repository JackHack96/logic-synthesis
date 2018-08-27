
#include "inc/nova.h"

coloring(ns, inco_array) int ns;
int **inco_array;

/* Colors the connected components of the incompatibles graph */

{

  int i, j, maxk, mink, vi, vj, i1, k;
  int flagadja, colornum;
  int *color;
  int **LForder;

  /* Largest-first vertex ordering - vertices are ordered as follows :
     deg(v1) >= deg(v2) >= ... >= deg(vn) .
     Before the lfs-ordering :
     LForder[2,i] = j iff vertex #(i+1) is connected to j other vertices -
     After the lfs-ordering :
     LForder[2,i1] <= LForder[2,i2] iff
                deg(vertex#(LForder[1,i1])) <= deg(vertex#(LForder[1,i2]))
     from right to left is the largest-first ordering */

  if ((LForder = (int **)calloc((unsigned)2, sizeof(int *))) == (int **)0) {
    fprintf(stderr, "Insufficient memory for 'LForder'");
    exit(-1);
  }
  for (i = 0; i < 2; i++) {
    if ((LForder[i] = (int *)calloc((unsigned)ns, sizeof(int))) == (int *)0) {
      fprintf(stderr, "Insufficient memory for LForder[i]");
      exit(-1);
    }
  }

  if ((color = (int *)calloc((unsigned)ns, sizeof(int))) == (int *)0) {
    fprintf(stderr, "Insufficient memory for 'color'");
    exit(-1);
  }

  /* lfs-ordering */
  for (i = 0; i < ns; i++) {
    LForder[0][i] = i + 1;
  }

  for (i = 0; i < ns; i++) {
    LForder[1][i] = 0;
    for (j = 0; j < ns; j++) {
      if (inco_array[i][j] == 1)
        LForder[1][i]++;
    }
  }

  /* QUICKSORT */
  quicksort(LForder, 1, ns);

  /* SEQUENTIAL COLORING */
  for (i = 0; i < ns; i++)
    color[i] = 0; /* clears the array color */
  color[LForder[0][ns - 1] - 1] = 1;
  maxk = 1; /* maxk is the biggest colour assigned up to now */
  for (vi = ns - 1; vi > 0; vi--) /* loop on the nodes to be colored */
  {
    mink = ns + 1; /* mink is the smallest colour assignable to vi */
    for (vj = 0; vj < ns; vj++) /* scan the colour array */
    {
      if (color[vj] != 0) {
        k = color[vj]; /* k is the color of node vj */
        /* vj is a colored node */
        i1 = 0;
        flagadja = 1;
        /* check whether color k has not been assigned to any node
        adjacent to vi (iff flagadja = 1) */
        while (i1 < ns & flagadja == 1) /* i1 scans the nodes */
        {                               /* adjacent to vi */
          if (inco_array[LForder[0][vi - 1] - 1][i1] == 1 & color[i1] == k)
            flagadja = 0;
          i1++;
        }
        if (flagadja == 1) /* colour k can be given to vi */
        {
          if (mink > k)
            mink = k;
        }
      }
    }
    /* when no old colour can be assigned to vi, a new one is given */
    if (mink != ns + 1)
      color[LForder[0][vi - 1] - 1] = mink;
    else {
      maxk++;
      color[LForder[0][vi - 1] - 1] = maxk;
    }
  }
  colornum = maxk;

  printf("\nThe number of colors used is  %d\n", colornum);
  /*printf("Color array : ");
  for (i=0; i<ns; i++) printf("%d", color[i]);
  printf("\n");*/

  return (colornum);
}

quicksort(array, init, length) int **array;
int init;
int length;

/* sort elements of array */

{
  int pivot;      /* the pivot value */
  int pivotindex; /* the index of the pivot element of array */
  int k;          /* beginning index for group of elements >= pivot */

  pivotindex = qspivot(array, init, length);
  if (pivotindex != 0) /* do nothing if array[init-1]=array[length-1] */
  {
    pivot = array[1][pivotindex - 1];
    k = qspart(array, init, length, pivot);
    quicksort(array, init, k - 1);
    quicksort(array, k, length);
  }
}

qspivot(array, init, length) int **array;
int init;
int length;

/* returns 0 if array[init],...array[length] have identical keys,
   otherwise the index of the larger of the leftmost two different keys */

{
  int k;        /* runs left to right looking for a different key */
  int firstkey; /* value of first key found */

  firstkey = array[1][init - 1];
  for (k = init + 1; k < length; k++) /* scan for different key */
  {
    if (array[1][k - 1] > firstkey)
      return (k); /* select larger key */
    else if (array[1][k - 1] < firstkey)
      return (init);
  }
  return (0);
}

qspart(array, init, length, pivot) int **array;
int init;
int length;
int pivot;

/* partitions array[init],...,array[length] so keys < pivot are at the left
   and keys >= pivot are on the right . Returns the beginning of the group on
   the right */

{
  int l, r; /* cursors used during the permutation process */
  int temp;

  l = init;
  r = length;
  while (l <= r) {
    /* swap array[1][r-1] & array[1][l-1] */
    temp = array[1][r - 1];
    array[1][r - 1] = array[1][l - 1];
    array[1][l - 1] = temp;
    /* swap array[0][r-1] & array[0][l-1] */
    temp = array[0][r - 1];
    array[0][r - 1] = array[0][l - 1];
    array[0][l - 1] = temp;

    /* Now the scan phase begins */
    while (array[1][l - 1] < pivot)
      l++;
    while (array[1][r - 1] >= pivot)
      r--;
  }
  return (l);
}
