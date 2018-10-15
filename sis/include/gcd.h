
#ifndef GCD_H
#define GCD_H

#include "array.h"
#include "node.h"

array_t *gcd_prime_factorize(node_t *);

node_t *gcd_nodevec(array_t *);

int init_gcd();

int end_gcd();

#endif
