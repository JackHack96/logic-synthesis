/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/lsort/luniq.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:29 $
 *
 */
/*
 *  Generic linked-list uniqifier package
 *  Richard Rudell, UC Berkeley, 4/1/87
 *
 *  Use:
 *	#define UNIQ		routine_name
 *	#define TYPE		typedef for structure of the linked-list
 *	#include "luniq.h"
 *
 *  Optional:
 *	#define NEXT		'next' field for the linked-list
 *	#define FREQ		'frequency' field (to maintain histogram)
 *	#define DECL_UNIQ	'static' or undefined
 *
 *  This defines a routine:
 *	DECL_UNIQ TYPE *
 *	UNIQ(list, compare, free)
 *	TYPE *list;
 *      int (*compare)(TYPE *x, TYPE *y);
 *      void free(TYPE *);
 *
 *	    Deletes duplicates from the list 'list'; calls 'free' (if
 *	    non-zero) to dispose of each element in the list.
 *
 *  The routines gracefully handle length == 0 (in which case, list == 0 
 *  is also allowed).
 *
 *  By default, the routine is declared 'static'.  This can be changed
 *  using '#define DECL_UNIQ'.
 */

#ifndef NEXT
#define NEXT next
#endif

#ifndef DECL_UNIQ
#define DECL_UNIQ static
#endif


DECL_UNIQ TYPE *
UNIQ(list, compare, free_routine)
TYPE *list;				/* linked-list of objects */
int (*compare)();			/* how to compare two objects */
void (*free_routine)();			/* dispose of duplicate objects */
{
    register TYPE *p1, *p2;

#ifdef FREQ
    for(p1 = list; p1 != 0; p1 = p1->next) {
	p1->FREQ = 1;
    }
#endif

    if (list != 0) {
	p1 = list;
	while ((p2 = p1->NEXT) != 0) {

#ifdef FIELD
#ifdef DIRECT_COMPARE
	    if (p1->FIELD == p2->FIELD) {
#else
	    if ((*compare)(p1->FIELD, p2->FIELD) == 0) {
#endif
#else
	    if ((*compare)(p1, p2) == 0) {
#endif
		p1->NEXT = p2->NEXT;
#ifdef FREQ
		p1->FREQ++;
#endif
		if (free_routine != 0) (*free_routine)(p2);
	    } else {
		p1 = p2;
	    }
	}
	p1->NEXT = 0;
    }
    return list;
}

#undef UNIQ
#undef FREQ
#undef NEXT
#undef FIELD
#undef DIRECT_COMPARE
#undef DECL_UNIQ
