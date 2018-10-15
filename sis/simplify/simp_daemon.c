/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/simplify/simp_daemon.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:49 $
 *
 */
#include "sis.h"
#include "simp_int.h"


void
simp_free(f)
node_t *f;
{
    FREE(f->SIM_SLOT);
}

void
simp_alloc(f)
node_t *f;
{
    f->SIM_SLOT = (char *) ALLOC(sim_flag_t, 1);
    SIM_FLAG(f)->method = SIM_METHOD_UNKNOWN;
    SIM_FLAG(f)->accept = SIM_ACCEPT_UNKNOWN;
    SIM_FLAG(f)->dctype = SIM_DCTYPE_UNKNOWN;
}


void
simp_invalid(f)
node_t *f;
{
    SIM_FLAG(f)->method = SIM_METHOD_UNKNOWN;
    SIM_FLAG(f)->accept = SIM_ACCEPT_UNKNOWN;
    SIM_FLAG(f)->dctype = SIM_DCTYPE_UNKNOWN;
}


void
simp_dup(old, new)
node_t *old, *new;
{
    SIM_FLAG(new)->method = SIM_FLAG(old)->method;
    SIM_FLAG(new)->accept = SIM_FLAG(old)->accept;
    SIM_FLAG(new)->dctype = SIM_FLAG(old)->dctype;
}
