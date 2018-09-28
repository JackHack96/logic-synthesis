
/* -------------------------------------------------------------------------- *\
   com_astg.c -- Add commands for ASTG package.

        $Revision: 1.1.1.1 $
        $Date: 2004/02/07 10:14:59 $

   Package initialization and cleanup.
\* ---------------------------------------------------------------------------*/

#ifdef SIS

#include "astg_int.h"
#include "bwd_int.h"
#include "si_int.h"
#include "sis.h"

void init_astg() {
    astg_basic_cmds(ASTG_TRUE);
    si_cmds();
    bwd_cmds();
}

void end_astg() { astg_basic_cmds(ASTG_FALSE); }

#endif /* SIS */
