
/*
 * Bill Lin
 * University of California, Berkeley
 * Comments to billlin@ic.Berkeley.EDU
 *
 * Copyright (c) 1989 Bill Lin, UC Berkeley CAD Group
 *     Permission is granted to do anything with this
 *     code except sell it or remove this message.
 */

#define SA_ERROR 0
#define SA_OK 1
#define SA_FULL_RANGE 2
#define SA_SMALL_RANGE 3
#define SA_DEFAULT_PARAMETER -1.0

#define SA_ASSERT(fct)                                                         \
  if ((fct) != SA_OK) {                                                        \
    fprintf(stderr, "SA Assertion failed: file %s, line %d\n", __FILE__,       \
            __LINE__);                                                         \
    exit(-1);                                                                  \
  }

void combinatorial_optimize();

double random_generator();

int int_random_generator();
