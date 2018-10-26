#ifndef GCD_INT_H
#define GCD_INT_H

/*
 *  internal definitions for gcd
 */

#include "node.h"

static int com_print_gcd();

static node_t *find_prime_factor();

node_t *internal_gcd(), *make_cube_free();

#endif