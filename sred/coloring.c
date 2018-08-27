
#include "reductio.h"

fast_coloring()

/* Colors the connected components of the incompatibles graph */

{

  int i, j, maxk, mink, vi, vj, i1, k;
  int flagadja;
  int *LForder[2];

  /* Largest-first vertex ordering - vertices are ordered as follows :
     deg(v1) >= deg(v2) >= ... >= deg(vn) .
     Before the lfs-ordering :
     LForder[2,i] = j iff vertex #(i+1) is connected to j other vertices -
     After the lfs-ordering :
     LForder[2,i1] <= LForder[2,i2] iff
                deg(vertex#(LForder[1,i1])) <= deg(vertex#(LForder[1,i2]))
     from right to left is the largest-first ordering */

  MYALLOC(int, LForder[0], ns);
  MYALLOC(int, LForder[1], ns);
  /* Before the lfs-ordering */
  /*
  printf("\n");
  printf("************************************\n");
  printf("Degrees array : ");
  */
  for (i = 0; i < ns; i++) {
    LForder[0][i] = i + 1;
    /*  printf("%d", LForder[0][i]); */
  }
  /*
  printf("\n");
  printf("                ");
  */

  for (i = 0; i < ns; i++) {
    LForder[1][i] = 0;
    for (j = 0; j < ns; j++) {
      if (is_in_set(GETSET(incograph, i), j))
        LForder[1][i]++;
    }
    /* printf("%d", LForder[1][i]); */
  }
  /* printf("\n"); */

  /* QUICKSORT */
  myqsort(LForder, 1, ns);

  /* After the lfs-ordering */
  /*
  printf("\n");
  printf("*******************************************\n");
  printf("Sorted degrees array : ");
  for (i=0; i<ns; i++) printf("%d", LForder[0][i]);
  printf("\n");
  printf("                       ");

  for (i=0; i<ns; i++) printf("%d", LForder[1][i]);
  printf("\n");
  */

  /* SEQUENTIAL COLORING */
  if (color)
    free(color);
  MYALLOC(int, color, ns);
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
          if (is_in_set(GETSET(incograph, LForder[0][vi - 1] - 1), i1) &&
              color[i1] == k)
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

  /*
  printf("\n");
  printf("*******************************\n");
  printf("The number of colors used is  %d\n", colornum);
  printf("Color array : ");
  for (i=0; i<ns; i++) printf("%d", color[i]);
  printf("\n");
  */
}
