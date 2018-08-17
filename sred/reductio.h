
#include <stdio.h>
#include "sis/util/util.h"
#include "espresso/inc/espresso.h"

#ifdef define_extern
#define extern
#endif

/* void * malloc (), * realloc (); */
#define CHUNK 10
#define MYREALLOC(type, name, size, num) do{ \
    while ((num) >= size) { \
        if (size == 0) { \
            size = CHUNK; \
            name = (type*) malloc (size * sizeof (*name)); \
        } \
        else { \
            size += CHUNK; \
            name = (type*) realloc (name, size * sizeof (*name)); \
        } \
        if (name == NULL) { \
            perror ("name"); \
            exit (1); \
        } \
    } \
}while(0)

#define MYCALLOC(type, name, size) do{ \
    name = (type*) calloc (size, sizeof (*name)); \
    if (name == NULL) { \
        perror ("name"); \
        exit (1); \
    } \
}while(0)

#define MYALLOC(type, name, size) do{ \
    name = (type*) malloc (size * sizeof (*name)); \
    if (name == NULL) { \
        perror ("name"); \
        exit (1); \
    } \
}while(0)


#define MAXNAME 256

#define MOORE 0
#define MEALY 1

#define ASTER "*"

#define max(a, b) (a>b?a:b)
#define min(a, b) (a<b?a:b)

typedef struct SYMTABLE {
    char            *symbol;
    char            *vector;
    struct SYMTABLE *next;
} SYMTABLE;

typedef struct NAMETABLE {
    char             *symbol;
    char             *wire;
    struct NAMETABLE *next;
} NAMETABLE;

typedef struct INPUTTABLE {
    char *input;
    int  ilab;
    char *pstate;
    int  plab;
    char *nstate;
    int  nlab;
    char *output;
    int  olab;
} INPUTTABLE;

typedef struct CODETABLE {
    int code;
    int loc;
} CODETABLE;

typedef struct LATTICE {
    char *vector;
    int  weight;
    int  depth;
    int  type;
} LATTICE;

extern int isymb, osymb;
extern int newnp, np, nis, ns, nos, ni, no;
extern int type;
extern int errorcount;
extern int yylineno, mylinepos;
extern int unspec;

extern char identification[MAXNAME], startstate[MAXNAME];
extern char myline[MAXNAME];
extern char state[MAXNAME], in[MAXNAME], next[MAXNAME], out[MAXNAME];

extern char **slab;
extern int  slab_size;

extern char lastnum[MAXNAME], lastid[MAXNAME], lastvect[MAXNAME];
extern char laststate[MAXNAME], lastin[MAXNAME], lastnext[MAXNAME];
extern char lastout[MAXNAME];

extern SYMTABLE *inputlist, *statelist, *outputlist;

extern NAMETABLE *nametable;

extern INPUTTABLE *itable;
extern int        itable_size;

extern FILE *filin, *filout;

/* VARIABLES RELATED TO FUNCTIONAL MINIMIZATION */

typedef struct SUBCOMP {
    int *compnum;
    int cmpnum;
}           SUBCOMP;

extern pset_family incograph;
extern SUBCOMP     incocomp;
extern int         *color;
extern int         colornum;
extern pset_family maxcompatibles;

extern pset_family primes;
extern int         reset;

typedef struct CHAINS {
    pset_family implied;
    int         *weight;
    int         weight_size;
    pset_family cover;
}                  CHAINS;

extern CHAINS firstchain;

extern pset_family copertura1, copertura2;

typedef struct MINITABLE {
    char *input;
    char *pstate;
    char *nstate;
    char *output;
}                  MINITABLE;

extern MINITABLE *mintable;
extern int       mintable_size;

extern int  do_print_classes;
extern char *coloring_algo;

int mygetc();


pset_family chiusura();
