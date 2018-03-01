/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/mincov/bin_sol.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:31 $
 *
 */
#include "sis.h"
#include "mincov_int.h"

bin_solution_t *
bin_solution_alloc(size)
int size;
{
    bin_solution_t *sol;

    sol = ALLOC(bin_solution_t, 1);
    sol->cost = 0;
    sol->set = set_new (size);
    return sol;
}


void
bin_solution_free(sol)
bin_solution_t *sol;
{
    set_free(sol->set);
    FREE(sol);
}


bin_solution_t *
bin_solution_dup(sol, size)
bin_solution_t *sol;
int size;
{
    bin_solution_t *new_sol;

    new_sol = ALLOC(bin_solution_t, 1);
    new_sol->cost = sol->cost;
    new_sol->set = set_new (size);
	INLINEset_copy (new_sol->set, sol->set);
    return new_sol;
}


void 
bin_solution_del(sol, weight, col)
bin_solution_t *sol;
int *weight;
int col;
{
    set_remove(sol->set, col);
    sol->cost -= WEIGHT(weight, col);
}

void 
bin_solution_add(sol, weight, col)
bin_solution_t *sol;
int *weight;
int col;
{
    set_insert(sol->set, col);
    sol->cost += WEIGHT(weight, col);
}

