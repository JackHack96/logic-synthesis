
/* SCCSID%W% */
/************************************************************
 * install(name,order,ptr,hashtab, hash_size) --- put name  *
 *					      in hashtab.   					*
 *                                                          *
 *       (a) install uses lookup() to determine whether     *
 *           the name being installed is already present;   *
 *       (b) if so, no action taken, except return the      *
 *           pointer pointing to the exist entry            *  
 *       (c) otherwise, a completely new entry is created.  *
 *	     and return the pointer pointing the new entry  	*
 *       (d) install returns NULL if for any reason there   *
 *           is no room for a new entry.                    *
 ************************************************************/
/* Modified by June Rho  Dec. 1989 */

#include <stdio.h>
#include "hash.h"


NLIST *install(name, order, ptr, hashtab, hash_size)
        char *name;
        char *ptr;        /* Pointer to first structure */
        int order;        /* order index of entries in hashtab */
        NLIST *hashtab[];
{
    NLIST *np;    /* a pointer to nlist */
    NLIST *lookup();
    int   hashval;
    char *str_save();

    if ((np = lookup(name, hashtab, hash_size)) == NIL(NLIST)) {
        /* not found */
        /*np = ALLOC ( NLIST , 1 ); */
        np = (NLIST *) malloc((unsigned) sizeof(NLIST));
        if (np == NIL(NLIST)) {
            (void) fprintf(stderr, "memory allocation error \n");
            return (NIL(NLIST));
        }

        if ((np->name = str_save(name)) == NIL(char))  {
            (void) fprintf(stderr,
                           "fail to copy a string, died in install.\n");
            exit(1);
        }

        hashval = hash(np->name, hash_size);
        np->order_index = order;
        np->next        = hashtab[hashval];
        np->ptr         = (int *) ptr;
        np->h_next      = NIL(NLIST);
        hashtab[hashval] = np;

        return (np);/* return the pointer pointing the new entry */
    } else {
        return (np);/* return the pointer pointing to the exist entry */
    }

}

NLIST
*install_input(name, hashtab, hash_size)
        char *name;
/* char *ptr;		Pointer to first structure */
/* int order;		order index of entries in hashtab */
        NLIST *hashtab[];
{
    NLIST *np;    /* a pointer to nlist */
    NLIST *lookup();
    int   hashval;
    char *str_save();

    if ((np = lookup(name, hashtab, hash_size)) == NIL(NLIST)) {
        /* not found */
        /*np = ALLOC ( NLIST , 1 ); */
        np = (NLIST *) malloc((unsigned) sizeof(NLIST));
        if (np == NIL(NLIST)) {
            panic("install");
        }

        if ((np->name = str_save(name)) == NIL(char))  {
            (void) fprintf(stderr,
                           "fail to copy a string, died in install.\n");
            exit(1);
        }

        hashval = hash(np->name, hash_size);
/*	    np->order_index = order; */
        np->next   = hashtab[hashval];
        np->h_prev = NIL(NLIST);
        np->ptr    = (int *) 0;
        np->h_next = NIL(NLIST);
        hashtab[hashval] = np;

        return (np);/* return the pointer pointing the new entry */
    } else {
        return NIL(NLIST);/* return the pointer pointing to the exist entry */
    }
}
