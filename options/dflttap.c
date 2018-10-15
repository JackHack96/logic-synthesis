/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/options/dflttap.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:10 $
 *
 */
#include "copyright.h"
#include "port.h"
#include "utility.h"
#include "options.h"

/*
 * This gets linked in if the program doesn't use tap
 *	It always returns NIL(char) so the rest of `options' can tell
 *	it is the bogus one
 */

#ifndef lint

/*ARGSUSED*/
char *tapRootDirectory(new)
char *new;
{
    return(NIL(char));
}

#endif /*lint*/
