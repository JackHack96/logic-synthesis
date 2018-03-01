/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/atpg/atpg_int.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:17 $
 *
 */
#include "atpg.h"

#define ALL_ZERO 0
#define ALL_ONE ~0
#define WORD_LENGTH 32
#define ATPG_DEBUG 0

#define EXTRACT_BIT(word,pos) (((word) & (1 << (pos))) != 0)

/* fault list data structures */
#define foreach_fault(faults, gen, f)  			     \
    for(gen = lsStart(faults);                               \
	(lsNext(gen, (lsGeneric *) &f, LS_NH) == LS_OK)      \
		    || ((void) lsFinish(gen), 0); )


