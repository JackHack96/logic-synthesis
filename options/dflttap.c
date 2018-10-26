
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
