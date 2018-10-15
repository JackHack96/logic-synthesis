/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/factor/factor.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:23 $
 *
 */
/*
 *  factor: factoring routine
 *  routines provided:
 *      factor();
 */

#include "sis.h"
#include "factor.h"
#include "factor_int.h"

/*
 *  factors nodep if nodep->factored is not NULL.
 */
void
factor(f)
node_t *f;
{
    if (f->factored == NIL(char)) {
	factor_quick(f);
    }
}

/* 
 *   quick factoring 
 */
void
factor_quick(f)
node_t *f;
{
    node_t *np;

    factor_free(f);
    node_scc(f);
    np = node_dup(f);
    np->type = UNASSIGNED;
    factor_recur(np, factor_quick_kernel);
    f->factored = (char *) factor_nt_to_ft(f, np);
    factor_nt_free(np);
}

/* 
 *   good factoring 
 */
void
factor_good(f)
node_t *f;
{
    node_t *np;

    factor_free(f);
    node_scc(f);
    np = node_dup(f);
    np->type = UNASSIGNED;
    factor_recur(np, factor_best_kernel);
    f->factored = (char *) factor_nt_to_ft(f, np);
    factor_nt_free(np);
}
