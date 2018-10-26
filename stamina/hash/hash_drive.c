
/************************************************************
 *  hash_drive  --- hash driver.                            *
 ************************************************************/

#include "hash.h"
#include <stdio.h>

main() {
  static char *s[] = {"state0",  "state1",  "state2",  "state3",  "state4",
                      "state5",  "state6",  "state7",  "state8",  "state9",
                      "state10", "state11", "state12", "state13", "state14",
                      "state15", "state16", "state9",  "state17", "state18",
                      "state19", "state20", "state21", "state7",  "state22",
                      "state23", "state24", "state25", "state26", "state27",
                      "state28", "state29"};

  int hash_size = 30;
  int count; /* the order index of entries in hashtab */
  int i;
  NLIST **node_hash;
  NLIST **hash_initial();
  NLIST *install();
  NLIST *lookup();

  void hash_dump(); /* void: declare a funtion which returns nothing*/

  (void)printf("These strings will be stored in a hash table \n\n");
  for (i = 0; i < 32; i++) {

    (void)printf("%s \n", s[i]);
  }
  (void)printf("\n\n");

  /*
   * allocate memory for hash_table.
   */
  if ((node_hash = hash_initial(hash_size)) == NIL(NLIST *)) {
    (void)fprintf(stderr,
                  "Memory allocation error, node_hash was not allocated \n");
    exit(1);
  }

  count = 0;
  for (i = 0; i < 32; i++) {

    if (lookup(s[i], node_hash, hash_size) == NIL(NLIST)) {
      (void)install(s[i], count, node_hash, hash_size);
      count++;
    }
  }

  /*
   * print out the hash table.
   */
  (void)hash_dump(node_hash, hash_size);
}
