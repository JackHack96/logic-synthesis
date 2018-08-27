
/************************************************************
 *  hash(s) --- The hashing function which forms hash value *
 *              for string s .                              *
 ************************************************************/

#include "hash.h"
#include <stdio.h>

int hash(s, hash_size) char *s;
int hash_size;
{
  int hashval; /* hash value of string s */

  for (hashval = 0; *s != '\0';) {
    hashval += *s;
    s++;
  }

  return (hashval % hash_size);
}
