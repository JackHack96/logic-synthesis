
/* -------------------------------------------------------------------------- *\
   com_astg.c -- Add commands for ASTG package.
   Package initialization and cleanup.
\* ---------------------------------------------------------------------------*/

#ifdef SIS
#include "sis.h"
#include "astg_int.h"
#include "si_int.h" 
#include "bwd_int.h"

void init_astg()
{
    astg_basic_cmds (ASTG_TRUE);
    si_cmds();
    bwd_cmds ();
}

void end_astg()
{
    astg_basic_cmds (ASTG_FALSE);
}
#endif /* SIS */
