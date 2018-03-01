/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/stamina/hash/hash_initial.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:16 $
 *
 */
/************************************************************
 *  hash_initial() --- Create an empty hash-table.          *
 *		       Return a pointer to the beginning of *
 *                     the hash-table.                      *
 *  str_save(s)    --- Save a string 's' to somewhere.      *
 ************************************************************/

#include <stdio.h>
#include "hash.h"


NLIST **hash_initial(hash_size)
int hash_size;
{
	NLIST **np;	/* it is an array of pointers to hash-table */
	int i;

	np = ALLOC ( NLIST *, hash_size);
	if ( np == NIL( NLIST * ) )  {
	    (void) fprintf(stderr, "Memory allocation error \n");
	    return (NIL(NLIST *));
	}

	for ( i = 0; i < hash_size; i++ )  {
	    *(np + i) = NIL(NLIST);
	}

	return (np);

}

char *str_save(s)		/* save a string 's' somewhere */
char *s;
{
	char *p;
        int num;
	
/*	if ( (p = malloc((unsigned)(strlen(s)+1))) == NIL(char) )  {
	    (void) fprintf(stderr, "Memory allocation error \n");
	    exit(1);
	}
*/
        num = strlen(s) + 1;
	if ( (p = ALLOC(char, num )) == NIL(char) )  {
	    (void) fprintf(stderr, "Memory allocation error \n");
	    return(NIL(char));
	}

	(void) strcpy(p,s);
	return (p);

}
