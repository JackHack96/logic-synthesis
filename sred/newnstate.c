

#include "reductio.h"

newresetstate(pclass) int pclass;

{
  int i;

  for (i = 0; i < np; i++) {
    if (strcmp(itable[i].pstate, startstate) == 0) {
      if (

          is_in_set(GETSET(copertura1, pclass

                                           - 1),
                    itable[i].plab - 1)) {
        return pclass;
      }
    }
  }
  return -1;
}

newnstate(pclass, ingresso) int pclass, ingresso;

{
  /* finds the next-states class ( i.e. the next state in the reduced
     machine ) evolving from "pclass" (the current state in the reduced
     machine ) under input "ingresso" and returns its value             */

  int i, j, found, good, count, value;
  pset nclass, p;

  nclass = set_new(ns);

  for (i = 0; i < np; i++) {
    if (strcmp(itable[i].input, itable[ingresso].input) == 0) {
      if (

          is_in_set(GETSET(copertura1, pclass

                                           - 1),
                    itable[i].plab - 1)) {
        if (itable[i].nlab != 0)
          set_insert(nclass, itable[i].nlab - 1);
      }
    }
  }

  count = set_ord(nclass);

  if (count > 0) {
    foreachi_set(copertura1, i, p) {
      if (setp_implies(nclass, p)) {
        break;
      }
    }
  }

  if (count == 0)
    value = -1;
  else
    value = i + 1;

  set_free(nclass);

  return (value);
}
