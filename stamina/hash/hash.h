/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/stamina/hash/hash.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:16 $
 *
 */
/************************************************************
 *  hash.h --- The hash header file.                        *
 ************************************************************/

#include "util.h"

#define HASHSIZE	1000	/* It is not used */

typedef struct nlist NLIST;

struct nlist {  /* basic table entry */
	char *name;
	int *ptr;		/* this element is not used */
	int *nptr;		/* For the new flow table */
	int order_index;	/* order index of entries in hashtab */
	NLIST *h_next;		/* Horizontal chain for flow table */
	NLIST *h_prev;
	struct nlist *next; 	/* next entry in chain */
};

/*NLIST **hashtab;*/	/* the hashtable should be allocated in 
			   the calling function by calling 
			   hash_initial() .            */
