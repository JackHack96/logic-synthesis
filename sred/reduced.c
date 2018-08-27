
#include "reductio.h"

print_classes() {
  int i, j;
  pset p;

  fprintf(stdout, "#begin_classes %d\n", copertura1->count);
  foreachi_set(copertura1, i, p) {
    for (j = 0; j < ns; j++) {
      if (is_in_set(p, j)) {
        fprintf(stdout, "# %s %d\n", slab[j], i);
      }
    }
  }
  fprintf(stdout, "#end_classes\n");
}

reduced() {
  /* Builds the final reduced machine */

  int i, j, k, l;
  int ingresso, nxstate, redoutput;
  pset scanned;

  scanned = set_new(np);
  /*
  printf("\n");
  printf("~~~~~~~~~~~~~~~ \n");
  printf("REDUCED MACHINE \n");
  printf("~~~~~~~~~~~~~~~ \n");
  */

  reset = -1;
  if (strlen(startstate) > 0) {
    for (i = 0; i < copertura1->count; i++) {
      reset = newresetstate(i + 1);
      if (reset > 0) {
        break;
      }
    }
  }

  l = 0;

  /* loop on the states (covering classes) of the reduced machine */
  for (i = 0; i < copertura1->count; i++) {
    /* "scanned" is cleared inside the outer loop because some states
        appear in more than one class and so the row terms pertaining
        to them must be considered again and again                     */
    set_clear(scanned, np);

    for (j = 0; j < np; j++) {
      /* pick up the first row term not already dealt with and
         containing a state of the current (i-th) class -
         if it has not already been considered it must be because
         its input part has not been yet considered together with that class */
      if (is_in_set(GETSET(copertura1, i), itable[j].plab - 1) &&
          !is_in_set(scanned, j)) {
        ingresso = j; /* we are now considering the input "ingresso"
  /* all the states of the same class under the same input are
     taken care of (as is meant by marking the "scanned" array) */
        for (k = j; k < np; k++)
          if (strcmp(itable[k].input, itable[ingresso].input) == 0 &&
              is_in_set(GETSET(copertura1, i), itable[k].plab - 1))
            set_insert(scanned, k);

        /* formation of a new row of the reduced machine */
        MYREALLOC(MINITABLE, mintable, mintable_size, l);
        mintable[l].input = malloc(strlen(itable[ingresso].input) + 1);
        mintable[l].pstate = malloc(strlen(itable[ingresso].pstate) + 5);
        mintable[l].nstate = malloc(strlen(itable[ingresso].nstate) + 5);
        mintable[l].output = malloc(strlen(itable[ingresso].output) + 5);

        /* INPUT */
        strcpy(mintable[l].input, itable[ingresso].input);
        /*printf("%s",mintable[l].input);*/
        /* PRESENT STATE */
        sprintf(mintable[l].pstate, "s%d", i + 1);
        /*printf("%s",mintable[l].pstate);*/
        /* NEXT STATE */
        nxstate = newnstate(i + 1, ingresso);
        if (nxstate != -1)
          sprintf(mintable[l].nstate, "s%d", nxstate);
        else
          sprintf(mintable[l].nstate, "*");
        /*printf("%s",mintable[l].nstate);*/
        /* OUTPUT */
        redoutput = newoutput(i + 1, ingresso);
        if (redoutput != -1)
          strcpy(mintable[l].output, itable[redoutput].output);
        else
          strcpy(mintable[l].output, itable[ingresso].output);
        /*printf("%s\n",mintable[l].output);*/

        l++;
      }
    }
  }

  newnp = l; /* the number of product terms is updated */

  /*printf("Product terms of the reduced machine = %d\n", l);*/

  /* The reduced machine table is copied back into "itable" */
  for (i = 0; i < l; i++) {
    if (i >= np) {
      itable[i].input = malloc(strlen(mintable[i].input) + 1);
      itable[i].pstate = malloc(strlen(mintable[i].pstate) + 1);
      itable[i].nstate = malloc(strlen(mintable[i].nstate) + 1);
      itable[i].output = malloc(strlen(mintable[i].output) + 1);
    }

    strcpy(itable[i].input, mintable[i].input);
    strcpy(itable[i].pstate, mintable[i].pstate);
    strcpy(itable[i].nstate, mintable[i].nstate);
    strcpy(itable[i].output, mintable[i].output);

    /*printf("%s",itable[i].input);
    printf(" %s",itable[i].pstate);
    printf(" %s",itable[i].nstate);
    printf(" %s\n",itable[i].output);
    */
  }

  set_free(scanned);
}
