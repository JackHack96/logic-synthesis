/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/stamina/hash/hash.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:16 $
 *
 */
/************************************************************
 *  hash(s) --- The hashing function which forms hash value *
 *              for string s .                              *
 ************************************************************/

#include <stdio.h>
#include "hash.h"


int hash(s, hash_size)
char *s;
int hash_size;
{
	int hashval;		/* hash value of string s */


	for ( hashval = 0; *s != '\0'; )  {
	    hashval += *s;
	    s++ ;
	}

	return ( hashval % hash_size );

}

	


