
/*******************************************************************
 *                                                                 *
 *                                                                 *
 *******************************************************************/

#include <stdio.h>
#include "stamina/hash/hash.h"

#define MAXSTRLEN    1024

typedef char          *STRING;
/*
typedef struct nlist NLIST;
*/
typedef struct state  STATE;
typedef struct edge   EDGE;
typedef struct p_edge P_EDGE;
typedef struct s_edge S_EDGE;
typedef struct xnode  NODE;
typedef struct isomor ISO;

struct xnode {
    NODE  *right;
    NODE  *left;
    int   literal;
    NLIST *cubes;
    int   *on_off;
};

struct sbase {
    int state;
    int where;
};

struct isomor {
    int sum;
    int num;
    int uvoid;
    int *list;
};

struct state {
    STRING state_name;
    STRING code;
    int    state_index;
    EDGE   *edge;
/*
	P_EDGE *pedge;
*/
    int    assigned;
};


struct edge {
    STRING input;        /* primary input */
    STRING output;        /* primary output*/
    STATE  *p_state;        /* pointer to a present state */
    STATE  *n_state;        /* pointer to a next state */
    int    p_star;        /* a flag to indicate if p_state is a '*'.*/
    int    n_star;        /* a flag to indicate if n_state is a '*'.*/
    EDGE   *next;
};

struct p_edge {
    STRING input;        /* primary input */
    STRING output;        /* primary output*/
    STATE  *p_state;        /* pointer to a present state */
    STATE  *n_state;        /* pointer to a next state */
    int    p_star;        /* a flag to indicate if p_state is a '*'.*/
    int    n_star;        /* a flag to indicate if n_state is a '*'.*/
    P_EDGE *v_next;
    P_EDGE *h_next;
};

struct s_edge {
    STRING output;        /* primary output*/
    STATE  *p_state;        /* pointer to a present state */
    STATE  *n_state;        /* pointer to a next state */
    int    p_star;        /* a flag to indicate if p_state is a '*'.*/
    int    n_star;        /* a flag to indicate if n_state is a '*'.*/
    S_EDGE *v_next;
};
/*

#include "util.h"
*/

/*
#define HASHSIZE	1000	/* It is not used *


struct nlist {  /* basic table entry *
	char *name;
	S_EDGE *ptr;		/* this element is not used *
	int order_index;	/* order index of entries in hashtab *
	NLIST *h_next;		/* Horizontal chain for flow table *
	NLIST *h_prev;
	struct nlist *next; 	/* next entry in chain *
};
*/

#define MEMCPY(FROM, TO, NBYTE) (void) memcpy((char *)(FROM),(char *)(TO),(int)(NBYTE))
