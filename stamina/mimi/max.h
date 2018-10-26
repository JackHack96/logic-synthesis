
/* SCCSID %W% */

#define CONDITIONAL_COMPATIBLE 1
#define DEFINIT_COMPATIBLE 2
#define COMPATIBLE 3
#define INCOMPATIBLE 4
#define USED_AND_COMPATIBLE 0x1b
#define USED 16
#define N_MAX 100 /* Number of Maximal Compatibles */
#define DONTCARE '-'

typedef struct maxies PRIME, **PRIMES;
typedef struct implied_states IMPLIED;
typedef struct top_implied CLASS;

struct top_implied {
  IMPLIED **imply;
  int many;
  int weight;
};

struct maxies {
  int num;
  int *state;
  CLASS class;
};

struct implied_states {
  int num;
  int *state;
};

#define MAX_PRIME 4084
#define SMASK(x) (x & 0x7fff)
#define SELECTED 0x8000
#define FOREVER 1

p_num;
PRIMES prime;
PRIMES max;
IMPLIED **local_imply;
int *local_state;
