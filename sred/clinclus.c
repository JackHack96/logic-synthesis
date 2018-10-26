
#include "reductio.h"

cl_inclus(genclass, subclass, index) int genclass, subclass, index;

{
  /* cl_inclus = 1  iff the class referred to by ( genclass,subclass )
     is strictly included in the c-prime class of position "index" */

  /* strict > 0 iff class(genclass,subclass) is included in class(index)
     strictly and doesn't coincide with it ( strict = 0 )
     ========                                                               */

  int i;
  int inclusa, strict, value;
  pset classi, classj;

  classi = set_new(ns);
  classj = set_new(ns);
  set_copy(classj, GETSET(primes, genclass));
  set_remove(classj, subclass);

  set_copy(classi, GETSET(primes, index));

  i = 0;
  inclusa = 1;
  strict = 0;
  while (i < ns && inclusa == 1) {
    if (is_in_set(classj, i)) {
      if (is_in_set(classi, i))
        inclusa = 1;
      else
        inclusa = 0;
    }
    if (!is_in_set(classj, i) && is_in_set(classi, i))
      strict++;
    i++;
  }

  if (inclusa == 1 && strict > 0)
    value = 1;
  else
    value = 0;
  /*printf("cl_inclus  = %d\n", value);*/
  set_free(classi);
  set_free(classj);
  return (value);
}
