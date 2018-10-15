
#include "espresso.h"
#include "util.h"
#include <stdio.h>

#ifdef define_extern
#define extern
#endif

/* void * malloc (), * realloc (); */
#define CHUNK 10
#define MYREALLOC(type, name, size, num)                                       \
  do {                                                                         \
    while ((num) >= size) {                                                    \
      if (size == 0) {                                                         \
        size = CHUNK;                                                          \
        name = (type *)malloc(size * sizeof(*name));                           \
      } else {                                                                 \
        size += CHUNK;                                                         \
        name = (type *)realloc(name, size * sizeof(*name));                    \
      }                                                                        \
      if (name == NULL) {                                                      \
        perror("name");                                                        \
        exit(1);                                                               \
      }                                                                        \
    }                                                                          \
  } while (0)

#define MYCALLOC(type, name, size)                                             \
  do {                                                                         \
    name = (type *)calloc(size, sizeof(*name));                                \
    if (name == NULL) {                                                        \
      perror("name");                                                          \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

#define MYALLOC(type, name, size)                                              \
  do {                                                                         \
    name = (type *)malloc(size * sizeof(*name));                               \
    if (name == NULL) {                                                        \
      perror("name");                                                          \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

#define MAXNAME 256

#define MOORE 0
#define MEALY 1

#define ASTER "*"

#define max(a, b) (a > b ? a : b)
#define min(a, b) (a < b ? a : b)

typedef struct SYMTABLE {
  char *symbol;
  char *vector;
  struct SYMTABLE *next;
} SYMTABLE;

typedef struct NAMETABLE {
  char *symbol;
  char *wire;
  struct NAMETABLE *next;
} NAMETABLE;

typedef struct INPUTTABLE {
  char *input;
  int ilab;
  char *pstate;
  int plab;
  char *nstate;
  int nlab;
  char *output;
  int olab;
} INPUTTABLE;

typedef struct CODETABLE {
  int code;
  int loc;
} CODETABLE;

typedef struct LATTICE {
  char *vector;
  int weight;
  int depth;
  int type;
} LATTICE;

int isymb, osymb;
int newnp, np, nis, ns, nos, ni, no;
int type;
int errorcount;
int yylineno, mylinepos;
int unspec;

char identification[MAXNAME], startstate[MAXNAME];
char myline[MAXNAME];
char state[MAXNAME], in[MAXNAME], next[MAXNAME], out[MAXNAME];

char **slab;
int slab_size;

char lastnum[MAXNAME], lastid[MAXNAME], lastvect[MAXNAME];
char laststate[MAXNAME], lastin[MAXNAME], lastnext[MAXNAME];
char lastout[MAXNAME];

SYMTABLE *inputlist, *statelist, *outputlist;

NAMETABLE *nametable;

INPUTTABLE *itable;
int itable_size;

FILE *filin, *filout;

/* VARIABLES RELATED TO FUNCTIONAL MINIMIZATION */

typedef struct SUBCOMP {
  int *compnum;
  int cmpnum;
} SUBCOMP;

pset_family incograph;
SUBCOMP incocomp;
int *color;
int colornum;
pset_family maxcompatibles;

pset_family primes;
int reset;

typedef struct CHAINS {
  pset_family implied;
  int *weight;
  int weight_size;
  pset_family cover;
} CHAINS;

CHAINS firstchain;

pset_family copertura1, copertura2;

typedef struct MINITABLE {
  char *input;
  char *pstate;
  char *nstate;
  char *output;
} MINITABLE;

MINITABLE *mintable;
int mintable_size;

int do_print_classes;
char *coloring_algo;

int mygetc();

pset_family chiusura();
