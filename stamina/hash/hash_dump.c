/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/stamina/hash/hash_dump.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:16 $
 *
 */
/************************************************************
 *  hash_dump() --- dump the hash table                     *
 ************************************************************/

#include <stdio.h>
#include "hash.h"


void hash_dump(hashtab, hash_size)
NLIST *hashtab[];
int hash_size;
{
	NLIST *np;	/* a pointer to nlist */
	int i;

	(void) printf("This is a dump of hash table \n\n");

	for ( i = 0; i < hash_size; i++ )  {
	    if ((np = hashtab[i]) == NIL(NLIST) ) {
		continue;
	    }
	    for (np = hashtab[i]; np != NULL; np = np->next)  {
		(void) printf("%s  %4d ", np->name, hash(np->name,hash_size) );
		(void) printf("  %4d \t ", np->order_index );
	    }
	    (void) printf("\n");
	}

	return ;

}



