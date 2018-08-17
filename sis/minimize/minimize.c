
/******************************************************************************
 * This module has been written by Abdul A. Malik                             *
 * Please forward all feedback, comments, bugs etc to:                        *
 *                                                                            *
 * ABDUL A. MALIK                                                             *
 * malik@ic.berkeley.edu                                                      *
 *                                                                            *
 * Department of Electrical Engineering and Computer Sciences                 *
 * University of California at Berkeley                                       *
 * Berkeley, CA 94704.                                                        *
 *                                                                            *
 * Telephone: (415) 642-5048                                                  *
 ******************************************************************************/

/*
 *  Module: minimize.c
 *
 *  Purpose: To do minimization without evaluating the complement of (FUD).
 *           This module contains two routines for minimization: nocomp()
 *	     and snocomp(). nocomp() is similar to espresso() except that
 *	     it uses reduced offset rather than the entire offset. snocomp()
 *	     also uses reduced offsets rather than entire offset but it does
 * 	     a single pass consisting of essential(), reduce(), expand(),
 *	     and irredundant().
 *
 *	     The subroutine minimize(F, D, single_pass) is written to decide
 *	     whether nocomp() or snocomp() is to be used. If the third 
 *	     argument single_pass is TRUE sncomp(F, D) is passed, otherwise
 *	     nocomp(F, D) is called. F and D are the Onset and the Don't Care
 *	     sets.
 *
 *
 *  Returns a minimized version of the ON-set of a function
 *
 *  The following global variables affect the operation of nocomp:
 *
 *  MISCELLANEOUS:
 *      trace
 *          print trace information as the minimization progresses
 *
 *      remove_essential
 *          remove essential primes
 *
 *      single_expand
 *          if true, stop after first expand/irredundant
 *
 *  LAST_GASP or SUPER_GASP strategy:
 *      use_super_gasp
 *          uses the super_gasp strategy rather than last_gasp
 *
 *  SETUP strategy:
 *      recompute_onset
 *          recompute onset using the complement before starting
 *
 *      unwrap_onset
 *          unwrap the function output part before first expand
 *
 *  MAKE_SPARSE strategy:
 *      force_irredundant
 *          iterates make_sparse to force a minimal solution (used
 *          indirectly by make_sparse)
 *
 *      skip_make_sparse
 *          skip the make_sparse step (used by opo only)
 *
 */

#include "sis.h"

#define extern

#include "../include/min_int.h"

#undef extern

#include "../include/minimize.h"

static pset_family ncp_abs_covered_many();

static pset_family ncp_abs_covered();

static int ncp_abs_select_restricted();

static bool toggle = TRUE;

/*
 * minimize -- choose between nocomp(), snocomp(), dc_simplify().
 */

pcover
minimize(F, D, type)
        pcover F;
        pcover D;
        int type;

{

    if (type == SNOCOMP)
        return (snocomp(F, D));

    else if (type == NOCOMP)
        return (nocomp(F, D));

    else if (type == DCSIMPLIFY)
        return (dcsimp(F, D));

    else {
        fail("Wrong minimize type");
        exit(-1);
        return (F);
    }

}

/*
 * nocomp(): Do minimization whithout computing complement of (F U D).
 */

pcover
nocomp(F, D1)
        pcover F, D1;

{
    pcover E, D, Fsave, Fold, Fbest_save;
    pset   last, p;
    cost_t cost, best_cost, best_save_cost;
    bool   skip_gasp = FALSE;

    /* Skip make_sparse if single output binary function */
    if ((cube.size - 2 * cube.num_binary_vars) <= 1) {
        skip_make_sparse = TRUE;
        unwrap_onset     = FALSE;
    }


    begin:
    Fsave = sf_save(F);        /* Save the original cover */
    D     = sf_save(D1);        /* make a scratch copy of D */

    /* Initialize variables to collect statistics about ROS */
    init_ROS(F, D, 1, 3, cube.size);

    /* Setup has always been a problem */
    if (recompute_onset) {
        EXEC(E = simplify(cube1list(F)), "SIMPLIFY   ", E);
        free_cover(F);
        F = E;
    }
    cover_cost(F, &cost);
    if (unwrap_onset && (cube.part_size[cube.num_vars - 1] > 1)
        && (cost.out != cost.cubes * cube.part_size[cube.num_vars - 1])
        && (cost.out < 5000))
        EXEC(F = sf_contain(unravel(F, cube.num_vars - 1)), "SETUP      ", F);

    /* Initial expand and irredundant */
    foreach_set(F, last, p)
    {
        RESET(p, PRIME);
    }

    if (F->count > 6) {
        EXECUTE(F = ncp_expand(F, (pcover) NULL, FALSE), EXPAND_TIME, F, cost);
    } else {
        EXECUTE(F = nc_first_expand(F, FALSE), EXPAND_TIME, F, cost);
    }

    EXECUTE(F = irredundant(F, D), IRRED_TIME, F, cost);

    if (!single_expand) {
        if (remove_essential) {
            EXECUTE(E = essential(&F, &D), ESSEN_TIME, E, cost);
        } else {
            E = new_cover(0);
        }

        cover_cost(F, &cost);
        copy_cost(&cost, &best_save_cost);
        Fbest_save = sf_save(F);
        do {

            /* Repeat inner loop until solution becomes "stable" */
            do {
                copy_cost(&cost, &best_cost);
                EXECUTE(F = nc_reduce(F, &Fold, D), REDUCE_TIME, F, cost);
                EXECUTE(F = ncp_expand(F, Fold, FALSE),
                        EXPAND_TIME, F, cost);

                if (F->count == 1) { /* No need iterate any more */
                    goto done;
                }

                EXECUTE(F = irredundant(F, D), IRRED_TIME, F, cost);

                /* Save the literal minimum solution */
                if ((best_save_cost.cubes > cost.cubes) ||
                    (best_save_cost.cubes == cost.cubes) &&
                    (best_save_cost.in > cost.in)) {
                    sf_free(Fbest_save);
                    Fbest_save = sf_save(F);
                }

            } while (cost.cubes < best_cost.cubes);

            if (skip_gasp) /* Skip last_gasp and super_gasp if TRUE */
                goto done;

            /* Perturb solution to see if we can continue to iterate */
            copy_cost(&cost, &best_cost);
            if (use_super_gasp) {
                F = nc_super_gasp(F, D, &cost);
                if (cost.cubes >= best_cost.cubes)
                    break;
            } else {
                F = nc_last_gasp(F, D, &cost);
            }

            if ((best_save_cost.cubes > cost.cubes) ||
                (best_save_cost.cubes == cost.cubes) &&
                (best_save_cost.in > cost.in)) {
                sf_free(Fbest_save);
                Fbest_save = sf_save(F);
            }

        } while (cost.cubes < best_cost.cubes ||
                 (cost.cubes == best_cost.cubes && cost.total < best_cost.total));

        done:
        /* Save the literal minimum solution */
        if ((best_save_cost.cubes > cost.cubes) ||
            (best_save_cost.cubes == cost.cubes) &&
            (best_save_cost.in > cost.in)) {
            sf_free(Fbest_save);
            Fbest_save = sf_save(F);
        }


        /* Append the essential cubes to F */
        sf_free(F);
        F = sf_append(Fbest_save, E);
        if (trace) size_stamp(F, "ADJUST     ");
    }

    /* Free the D which we used */
    free_cover(D);

    /* Attempt to make the PLA matrix sparse */
    if (!skip_make_sparse) {
        F = nc_make_sparse(F, D1);
    }

    /* Close out all stuff about reduced offsets */
    close_ROS();

    /*
     *  Check to make sure function is actually smaller !!
     *  This can only happen because of the initial unravel.  If we fail,
     *  then run the whole thing again without the unravel.
     */
    if ((Fsave->count < F->count) && (!unwrap_onset)) {
        free_cover(F);
        F            = Fsave;
        unwrap_onset = FALSE;
        goto begin;
    } else {
        free_cover(Fsave);
    }

    return F;
}

/*
 * Do a single pass consisting of essential, reduce, expand, irredundant.
 * The purpose is to do some reasonable amount of optimization in a short time.
 */

pcover
snocomp(F, D1)
        pcover F, D1;

{
    pcover E, D, Fold;
    cost_t cost;
    pcube  p, last;

    D = sf_save(D1);

    /* Initialize variables to collect statistics about ROS */
    init_ROS(F, D, 1, 3, cube.size);

    foreach_set(F, last, p)
    {
        SET(p, RELESSEN);
        RESET(p, NONESSEN);
    }

    EXECUTE(E = essential(&F, &D), ESSEN_TIME, E, cost);
    EXECUTE(F = nc_reduce(F, &Fold, D), REDUCE_TIME, F, cost);

    /* Since the initial expand that is done in espresso is not done
       here, any cube that could not be reduced by "reduce" is not 
       necessarily prime. Make sure that no cube has PRIME flag set
       before "expand" is called. */

    foreach_set(F, last, p)
    {
        RESET(p, PRIME);
    }

    EXEC(F     = ncp_expand(F, Fold, FALSE), "PRIMES     ", F);
    if (F->count > 1)
        EXEC(F = irredundant(F, D), "IRRED_TIME     ", F);

    /* Append the essential cubes to F */
    F = sf_append(F, E);
    if (trace) size_stamp(F, "ADJUST     ");

    /* Free the D which we used */
    free_cover(D);

    /* Close out all stuff about reduced offsets */
    close_ROS();

    return F;

}

/*
 *  unate_compl
 */

pset_family
ncp_unate_compl(A, max_lit)
        pset_family A;
        int max_lit;

{
    register pset p, last;

    foreach_set(A, last, p)
    {
        PUTSIZE(p, set_ord(p));
    }

    /* Recursively find the complement */
    A = ncp_unate_complement(A, max_lit);

    /* Now, we can guarantee a minimal result by containing the result */
    A = sf_rev_contain(A);
    return A;
}


/*
 *  Assume SIZE(p) records the size of each set
 */

pset_family
ncp_unate_complement(A, max_lit)
        pset_family A;            /* disposes of A */
        int max_lit;

{
    pset_family   Abar;
    register pset p, p1, restrict;
    register int i;
    int          max_i, min_set_ord, j;

    /* Check for no sets in the matrix -- complement is the universe */
    if (A->count == 0) {
        sf_free(A);
        Abar = sf_new(1, A->sf_size);
        (void) set_clear(GETSET(Abar, Abar->count++), A->sf_size);

        /* If the number of literals used so far is larger than or equal to
           the number of literals allowed then return */
    } else if (max_lit <= 0) {
        Abar = A;
        Abar->count = 0;

        /* Check for a single set in the maxtrix -- compute de Morgan complement */
    } else if (A->count == 1) {
        p      = A->data;
        Abar   = sf_new(A->sf_size, A->sf_size);
        for (i = 0; i < A->sf_size; i++) {
            if (is_in_set(p, i)) {
                p1 = set_clear(GETSET(Abar, Abar->count++), A->sf_size);
                set_insert(p1, i);
            }
        }
        sf_free(A);

    } else {

        /* Select splitting variable as the variable which belongs to a set
         * of the smallest size, and which has greatest column count
         */
        restrict = set_new(A->sf_size);
        min_set_ord = A->sf_size + 1;
        foreachi_set(A, i, p)
        {
            if (SIZE(p) < min_set_ord) {
                (void) set_copy(restrict, p);
                min_set_ord = SIZE(p);
            } else if (SIZE(p) == min_set_ord) {
                (void) set_or(restrict, restrict, p);
            }
        }

        /* Check for no data (shouldn't happen ?) */
        if (min_set_ord == 0) {
            A->count = 0;
            Abar = A;

            /* Check for "essential" columns */
        } else if (min_set_ord == 1) {
            i = set_ord(restrict);
            if (i > max_lit) {
                Abar = A;
                Abar->count = 0;
            } else {
                Abar = ncp_unate_complement(ncp_abs_covered_many(A, restrict),
                max_lit - i);
                sf_free(A);
                foreachi_set(Abar, i, p)
                {
                    (void) set_or(p, p, restrict);
                }
            }

            /* else, recur as usual */
        } else {
            max_i = ncp_abs_select_restricted(A, restrict);

            /* Select those rows of A which are not covered by max_i,
             * recursively find all minimal covers of these rows, and
             * then add back in max_i
             */
            Abar = ncp_unate_complement(ncp_abs_covered(A, max_i), max_lit - 1);
            foreachi_set(Abar, i, p)
            {
                set_insert(p, max_i);
            }

            /* Now recur on A with all zero's on column max_i */
            foreachi_set(A, i, p)
            {
                if (is_in_set(p, max_i)) {
                    set_remove(p, max_i);
                    j = SIZE(p) - 1;
                    PUTSIZE(p, j);
                }
            }

            Abar = sf_append(Abar, ncp_unate_complement(A, max_lit));
        }
        set_free(restrict);
    }

    return Abar;
}

/*
 *  ncp_abs_covered_many -- after selecting many columns for ther selected set,
 *  create a new matrix which is only those rows which are still uncovered
 */
static pset_family
ncp_abs_covered_many(A, pick_set)
        pset_family A;
        register pset pick_set;
{
    register pset        last, p, pdest;
    register pset_family Aprime;

    Aprime = sf_new(A->count, A->sf_size);
    pdest  = Aprime->data;
    foreach_set(A, last, p)
    if (setp_disjoint(p, pick_set)) {
        INLINEset_copy(pdest, p);
        Aprime->count++;
        pdest += Aprime->wsize;
    }
    return Aprime;
}


/*
 *  ncp_abs_select_restricted -- select the column of maximum column count which
 *  also belongs to the set "restrict"; weight each column of a set as
 *  1 / (set_ord(p) - 1).
 */
static int
ncp_abs_select_restricted(A, restrict)
        pset_family A;
        pset restrict;
{
    register int i, best_var, best_count, *count;

    /* Sum the elements in these columns */
    count = sf_count_restricted(A, restrict);

    /* Find which variable has maximum weight */
    best_var   = -1;
    best_count = 0;
    for (i     = 0; i < A->sf_size; i++) {
        if (count[i] > best_count) {
            best_var   = i;
            best_count = count[i];
        }
    }
    FREE(count);

    if (best_var == -1)
        fatal("ncp_abs_select_restricted: should not have best_var == -1");

    return best_var;
}

/*
 *  abs_covered -- after selecting a new column for the selected set,
 *  create a new matrix which is only those rows which are still uncovered
 */
static pset_family
ncp_abs_covered(A, pick)
        pset_family A;
        register int pick;
{
    register pset        last, p, pdest;
    register pset_family Aprime;

    Aprime = sf_new(A->count, A->sf_size);
    pdest  = Aprime->data;
    foreach_set(A, last, p)
    if (!is_in_set(p, pick)) {
        INLINEset_copy(pdest, p);
        Aprime->count++;
        pdest += Aprime->wsize;
    }
    return Aprime;
}

/*
 * ncp_compl-special_cases is to be used only to complement unate functions
 * when the number of literals in the complement are to be restricted to
 * max_lit.
 */
void
ncp_compl_special_cases(T, Tbar, max_lit)
        pcube *T;            /* will be disposed of */
        pcover *Tbar;            /* returned only if answer determined */
        int max_lit;

{
    register pcube *T1, p, ceil, cof = T[0];
    pcover         A, ceil_compl;

    /* Check for no cubes in the cover */
    if (T[2] == NULL) {
        *Tbar = sf_addset(new_cover(1), cube.fullset);
        free_cubelist(T);
        return;
    }

    /* Check for only a single cube in the cover */
    if (T[3] == NULL) {
        *Tbar = nc_compl_cube(set_or(cof, cof, T[2]));
        free_cubelist(T);
        return;
    }

    /* Check for a row of all 1's (implies complement is null) */
    for (T1 = T + 2; (p = *T1++) != NULL;) {
        if (full_row(p, cof)) {
            *Tbar = new_cover(0);
            free_cubelist(T);
            return;
        }
    }

    /* Check for a column of all 0's which can be factored out */
    ceil    = set_save(cof);
    for (T1 = T + 2; (p = *T1++) != NULL;) {
        INLINEset_or(ceil, ceil, p);
    }
    if (!setp_equal(ceil, cube.fullset)) {
        ceil_compl = nc_compl_cube(ceil);
        (void) set_or(cof, cof, set_diff(ceil, cube.fullset, ceil));
        set_free(ceil);
        ncp_compl_special_cases(T, Tbar, max_lit);
        *Tbar = sf_append(*Tbar, ceil_compl);
        return;
    }
    set_free(ceil);

    /* Collect column counts, determine unate variables, etc. */
    massive_count(T);

    /* If single active variable not factored out above, then tautology ! */
    if (cdata.vars_active == 1) {
        *Tbar = new_cover(0);
        free_cubelist(T);
        return;

        /* The cover is unate since all else failed */
    } else {
        A = map_cover_to_unate(T);
        free_cubelist(T);
        A = ncp_unate_compl(A, max_lit);
        *Tbar = map_unate_to_cover(A);
        sf_free(A);
        return;
    }

}

/* 
 * nc_super_gasp
 */

pcover
nc_super_gasp(F, D, cost)
        pcover F, D;
        cost_t *cost;
{
    pcover G, G1;

    EXECUTE(G  = nc_reduce_gasp(F, D), GREDUCE_TIME, G, *cost);
    EXECUTE(G1 = nc_all_primes(G), GEXPAND_TIME, G1, *cost);
    free_cover(G);
    EXEC(G    = sf_dupl(sf_append(F, G1)), "NEWPRIMES", G);
    EXECUTE(F = irredundant(G, D), IRRED_TIME, F, *cost);
    return F;
}

/*
 *  nc_all_primes -- foreach cube in F, generate all of the primes
 *  which cover the cube.
 */

pcover
nc_all_primes(F)
        pcover F;
{
    register pcube last, p, RAISE = nc_tmp_cube[24], FREESET = nc_tmp_cube[25];
    pcover         Fall_primes, B1;

    Fall_primes = new_cover(F->count);

    foreach_set(F, last, p)
    {
        if (TESTP(p, PRIME)) {
            Fall_primes = sf_addset(Fall_primes, p);
        } else {

            /* Find the blocking matrix */
            get_ROS1(p, cube.fullset, root);

            /* Setup for call to essential parts */
            (void) set_copy(RAISE, p);
            (void) set_diff(FREESET, cube.fullset, RAISE);
            setup_BB_CC(ROS, /* CC */ (pcover) NULL);
            essen_parts(ROS, /* CC */ (pcover) NULL, RAISE, FREESET);

            /* Find all of the primes, and add them to the prime set */
            B1          = find_all_primes(ROS, RAISE, FREESET);
            Fall_primes = sf_append(Fall_primes, B1);

        }
    }

    return Fall_primes;
}

/*
 * Similar to make_sparse except that the complete off_set is
 * not used.
 */

pcover
nc_make_sparse(F, D)
        pcover F, D;
{
    cost_t cost, best_cost;

    cover_cost(F, &best_cost);

    do {
        EXECUTE(F = mv_reduce(F, D), MV_REDUCE_TIME, F, cost);
        if (cost.total == best_cost.total)
            break;
        copy_cost(&cost, &best_cost);

        EXECUTE(F = ncp_expand(F, (pcover) NULL, TRUE), RAISE_IN_TIME, F, cost);
        if (cost.total == best_cost.total)
            break;
        copy_cost(&cost, &best_cost);
    } while (force_irredundant);

    return F;
}

/*
 *  nc_expand_gasp -- similar to expand_gasp except complete
 *  off set is not used.
 *
 *  expand each nonprime cube of F into a prime implicant
 *  The gasp strategy differs in that only those cubes which expand to
 *  cover some other cube are saved; also, all cubes are expanded
 *  regardless of whether they become covered or not.
 */

pcover
nc_expand_gasp(F, D, Foriginal)

INOUT       pcover
            F;
IN          pcover
            D;
IN          pcover
            Foriginal;
{
int    c1index;
pcover G;

/* Try to expand each nonprime and noncovered cube */
G = new_cover(10);
for (
c1index = 0;
c1index < F->
count;
c1index++) {
nc_expand1_gasp(F, D, Foriginal, c1index,
&G);
}
G = sf_dupl(G);
G = ncp_expand(G, (pcover)
NULL, /*nonsparse*/ FALSE);/* Make them prime ! */
return
      G;
}

/*
 *  nc_expand1_gasp -- similar to expand1_gasp
 *
 *  Expand a single cube against the OFF-set, using the gasp strategy
 */
void
nc_expand1_gasp(F, D, Foriginal, c1index, G)
        pcover F;        /* reduced cubes of ON-set */
        pcover D;        /* DC-set */
        pcover Foriginal;    /* ON-set before reduction (same order as F) */
        int c1index;        /* which index of F (or Freduced) to be checked */
        pcover *G;
{
    register int   c2index;
    register pcube p, last, c2under;
    pcube          RAISE     = nc_tmp_cube[28],
                   FREESET   = nc_tmp_cube[29],
                   temp      = nc_tmp_cube[30];
    pcube          *FD, c2essential;
    pcube          new_lower = nc_tmp_cube[32];
    pcover
    F1;

    if (debug & EXPAND1) {
        (void) printf("\nEXPAND1_GASP:    \t%s\n", pc1(GETSET(F, c1index)));
    }

    /* Initialize the raising and unassigned sets */
    (void) set_copy(RAISE, GETSET(F, c1index));
    (void) set_diff(FREESET, cube.fullset, RAISE);

    ROS = get_ROS(c1index, F, (pcover)
    NULL);

    /* Initialize the Blocking Matrix */
    ROS->active_count = ROS->count;
    foreach_set(ROS, last, p)
    {
        SET(p, ACTIVE);
    }
    /* Initialize the reduced ON-set, all nonprime cubes become active */
    F->active_count = F->count;
    foreachi_set(F, c2index, c2under)
    {
        if (c1index == c2index || TESTP(c2under, PRIME)) {
            F->active_count--;
            RESET(c2under, ACTIVE);
        } else {
            SET(c2under, ACTIVE);
        }
    }

    /* Determine parts which must be lowered */
    essen_parts(ROS, F, RAISE, FREESET);

    /* Determine parts which can always be raised */
    essen_raising(ROS, RAISE, FREESET);

    /* See which, if any, of the reduced cubes we can cover */
    foreachi_set(F, c2index, c2under)
    {
        if (TESTP(c2under, ACTIVE)) {
            /* See if this cube can be covered by an expansion */
            if (setp_implies(c2under, RAISE) ||
                feasibly_covered(ROS, c2under, RAISE, new_lower)) {

                /* See if c1under can expanded to cover c2 reduced against
                 * (F - c1) u c1under; if so, c2 can definitely be removed !
                 */

                /* Copy F and replace c1 with c1under */
                F1 = sf_save(Foriginal);
                (void) set_copy(GETSET(F1, c1index), GETSET(F, c1index));

                /* Reduce c2 against ((F - c1) u c1under) */
                FD          = cube2list(F1, D);
                c2essential = nc_reduce_cube(FD, GETSET(F1, c2index));
                free_cubelist(FD);
                sf_free(F1);

                /* See if c2essential is covered by an expansion of c1under */
                if (feasibly_covered(ROS, c2essential, RAISE, new_lower)) {
                    (void) set_or(temp, RAISE, c2essential);
                    RESET(temp, PRIME);        /* cube not prime */
                    *G = sf_addset(*G, temp);
                }
                set_free(c2essential);
            }
        }
    }

}

/*
 * nc_last_gasp
 */

pcover
nc_last_gasp(F, D, cost)
        pcover F, D;
        cost_t *cost;
{
    pcover
    G, G1;

    EXECUTE(G  = nc_reduce_gasp(F, D), GREDUCE_TIME, G, *cost);
    EXECUTE(G1 = nc_expand_gasp(G, D, F), GEXPAND_TIME, G1, *cost);
    free_cover(G);
    EXECUTE(F = irred_gasp(F, D, G1), GIRRED_TIME, F, *cost);
    return F;
}

/*
 *  nc_reduce_gasp -- compute the maximal reduction of each cube of F
 *
 *  If a cube does not reduce, it remains prime; otherwise, it is marked
 *  as nonprime.   If the cube is redundant (should NEVER happen here) we
 *  just crap out ...
 *
 *  A cover with all of the cubes of F is returned.  Those that did
 *  reduce are marked "NONPRIME"; those that reduced are marked "PRIME".
 *  The cubes are in the same order as in F.
 */
pcover
nc_reduce_gasp(F, D)
        pcover F, D;
{
    pcube p, last, cunder, *FD;
    pcover
    G;

    G  = new_cover(F->count);
    FD = cube2list(F, D);

    /* Reduce cubes of F without replacement */
    foreach_set(F, last, p)
    {
        cunder = nc_reduce_cube(FD, p);
        if (setp_empty(cunder)) {
            fatal("empty reduction in nc_reduce_gasp, shouldn't happen");
        } else if (setp_equal(cunder, p)) {
            SET(cunder, PRIME);            /* just to make sure */
            G = sf_addset(G, p);        /* it did not reduce ... */
        } else {
            RESET(cunder, PRIME);        /* it reduced ... */
            G = sf_addset(G, cunder);
        }
        if (debug & GASP) {
            (void) printf("REDUCE_GASP: %s reduced to %s\n", pc1(p), pc2(cunder));
        }
        free_cube(cunder);
    }

    free_cubelist(FD);
    return G;
}

/*
 * This is similar to reduce, except that it returns pre-reduced
 * cubes in Fold in the same order that the reduced cubes are returned
 * in F.
 *
 * WARNING:
 *    variable toggle used in reduce is defined as a global
 * variable in reduce.c. The variable is not available here so it
 * has been declared as a global variable in this file and set to
 * TRUE. So long as toggle here and toggle in reduce.c have the same value,
 * The order in which cubes are reduced in the two subroutines will
 * be the same. Otherwise, the order will be different.
 */
pcover
nc_reduce(F, Fold, D)

INOUT pcover
      F;
OUT   pcover
*
   Fold;
IN pcover
   D;
{
register pcube p, cunder, *FD;
register       index;
bool           inactive;

/* Order the cubes */
if (use_random_order)
F = random_order(F);
else {
F      = toggle ? sort_reduce(F) : mini_sort(F, descend);
toggle = !toggle;
}

*
Fold = sf_save(F);

inactive = FALSE;
/* Try to reduce each cube */
FD       = cube2list(F, D);
foreachi_set(F, index, p
) {
cunder = nc_reduce_cube(FD, p);        /* reduce the cube */
if (
setp_equal(cunder, p
)) {            /* see if it actually did */
SET(p, ACTIVE
);    /* cube remains active */
SET(p, PRIME
);    /* cube remains prime ? */
} else {
if (debug & REDUCE) {
(void) printf("REDUCE: %s to %s %s\n",
pc1(p), pc2(cunder), print_time(ptime())
);
}
(void)
set_copy(p, cunder
);                /* save reduced version */
RESET(p, PRIME
);                    /* cube is no longer prime */
if (
setp_empty(cunder)
) {
RESET(p, ACTIVE
);               /* if null, kill the cube */
inactive = TRUE;
}
else
SET(p, ACTIVE
);                 /* cube is active */
}
free_cube(cunder);
}
free_cubelist(FD);

if (inactive) {

/* Copy the flags from cubes in F to those in Fold */
foreachi_set(F, index, p
) {
if
TESTP(p, ACTIVE
)

SET (GETSET((

*Fold), index), ACTIVE);
else

RESET (GETSET((

*Fold), index), ACTIVE);
}

/* Delete any cubes of F which reduced to the empty cube */
F = sf_inactive(F);

/* Also delete old cubes corresponding to the deleted cubes from F */
*
Fold = sf_inactive(*Fold);

}

return
   F;

}

/*
 * Similar to random_order except that it changes the order of cubes in
 * Fold so that the ith cube in Fold is the ith cube in F before reduce.
 */

pcover
nc_random_order(F, Fold)
        register pcover F, Fold;
{
    pset         temp;
    register int i, k;
#ifdef RANDOM
    long random();
#endif

    temp   = set_new(F->sf_size);
    for (i = F->count - 1; i > 0; i--) {
        /* Choose a random number between 0 and i */
#ifdef RANDOM
        k = random() % i;
#else
        /* this is not meant to be really used; just provides an easy
           "out" if random() and srandom() aren't around
        */
        k = (i * 23 + 997) % i;
#endif
        /* swap sets i and k in F */
        (void) set_copy(temp, GETSET(F, k));
        (void) set_copy(GETSET(F, k), GETSET(F, i));
        (void) set_copy(GETSET(F, i), temp);

        /* swap sets i and k in Fold */
        (void) set_copy(temp, GETSET(Fold, k));
        (void) set_copy(GETSET(Fold, k), GETSET(Fold, i));
        (void) set_copy(GETSET(Fold, i), temp);
    }
    set_free(temp);
    return F;
}

/*
 * The cubes in F1 are permuted in T1 (e.g the first cube in F1 may
 * be in T1[5]).  This subroutine returns a cover G that has the cubes
 * of F2 permuted in the same way as the cubes of F1 are in T1.
 */

pcover
nc_permute(F1, T1, F2)
        pcover F1;
        pcube *T1;
        pcover F2;

{

    pcover
    G;
    pcube *T;
    int   count, diff, i;

    count = F1->count;
    T     = ALLOC(pcube, count + 1);
    diff  = (F2->data) - (F1->data);

    for (i = 0; i < count; i++)
        T[i] = T1[i] + diff;
    T[count] = (pcube) NULL;

    G = sf_unlist(T, count, cube.size);
    sf_free(F2);

    return (G);

}

/* 
 * nc_mini_sort -- sort cubes according to the heuristics of mini
 * Put cube in G in the same order as the new order for the cubes
 * in F.
 */
pcover
nc_mini_sort(F, G, compare)
        pcover F;
        pcover *G;
        int (*compare)();
{
    register int   *count, cnt, n = cube.size, i;
    register pcube p, last;
    pcover
    F_sorted;
    pcube *F1;

    /* Perform a column sum over the set family */
    count = sf_count(F);

    /* weight is "inner product of the cube and the column sums" */
    foreach_set(F, last, p)
    {
        cnt    = 0;
        for (i = 0; i < n; i++)
            if (is_in_set(p, i))
                cnt += count[i];
        PUTSIZE(p, cnt);
    }
    FREE(count);

    /* use qsort to sort the array */
    qsort((char *) (F1 = sf_list(F)), F->count, sizeof(pcube), compare);
    *G = nc_permute(F, F1, *G);
    F_sorted = sf_unlist(F1, F->count, F->sf_size);
    free_cover(F);

    return F_sorted;
}

/*
 * nc_reduce_cube -- find the maximal reduction of a cube
 */

pcube
nc_reduce_cube(FD, p)

IN pcube
*FD,
p;

{
pcube cunder;

cunder                    = nc_sccc(cofactor(FD, p), 1);
return
set_and(cunder, cunder, p
);
}


/* nc_sccc -- find Smallest Cube Containing the Complement of a cover */
pcube
nc_sccc(T, level)

INOUT           pcube
*
       T;         /* T will be disposed of */
IN int level;
{
pcube          r, *Tl, *Tr;
register pcube cl, cr;
register int   best;
static int     sccc_level = 0;

if (debug & REDUCE1) {
debug_print(T,
"SCCC", sccc_level++);
}

if (
nc_sccc_special_cases(T,
&r, level) == MAYBE) {
cl   = new_cube();
cr   = new_cube();
best = binate_split_select(T, cl, cr, REDUCE1);

Tl = scofactor(T, cl, best);
Tr = scofactor(T, cr, best);
free_cubelist(T);

r                           = sccc_merge(nc_sccc(Tl, level + 1), nc_sccc(Tr, level + 1), cl, cr);
}

if (debug & REDUCE1)
(void) printf("SCCC[%d]: result is %s\n", --sccc_level,
pc1(r)
);
return
      r;
}

/*
 *   nc_sccc_special_cases -- check the special cases for sccc
 */

bool
nc_sccc_special_cases(T, result, level)

INOUT pcube
*
    T;                 /* will be disposed if answer is determined */
OUT pcube
*
       result;              /* returned only if answer determined */
IN int level;
{
register pcube *T1, p, temp = cube.temp[1], ceil, cof = T[0];
pcube          *A, *B;
int            size;

/* empty cover => complement is universe => SCCC is universe */
if (T[2] == NULL) {
*
result = set_save(cube.fullset);
free_cubelist(T);
return
TRUE;
}

/* row of 1's => complement is empty => SCCC is empty */
for (
T1 = T + 2;
(
p = *T1++
) !=
NULL;
) {
if (
full_row(p, cof
)) {
*
result = new_cube();
free_cubelist(T);
return
TRUE;
}
}

/* Throw away the unnecessary cubes */
size = CUBELISTSIZE(T);
if ((size > 200) || ((level % N_level == 0) && size > 50))
rem_unnec_r_cubes(T);

/* Collect column counts, determine unate variables, etc. */
massive_count(T);

/* If cover is unate (or single cube), apply simple rules to find SCCCU */
if (cdata.vars_unate == cdata.vars_active || T[3] == NULL) {
*
result = set_save(cube.fullset);
for (
T1 = T + 2;
(
p = *T1++
) !=
NULL;
) {
(void)
sccc_cube(*result, set_or(temp, p, cof)
);
}
free_cubelist(T);
return
TRUE;
}

/* Check for column of 0's (which can be easily factored( */
ceil = set_save(cof);
for (
T1 = T + 2;
(
p = *T1++
) !=
NULL;
) {
INLINEset_or(ceil, ceil, p
);
}
if (!
setp_equal(ceil, cube
.fullset)) {
*
result = sccc_cube(set_save(cube.fullset), ceil);
if (
setp_equal(*result, cube
.fullset)) {
free_cube(ceil);
} else {
*
result = sccc_merge(nc_sccc(cofactor(T, ceil), level + 1),
                    set_save(cube.fullset), ceil, *result);
}
free_cubelist(T);
return
TRUE;
}
free_cube(ceil);

/* Single active column at this point => tautology => SCCC is empty */
if (cdata.vars_active == 1) {
*
result = new_cube();
free_cubelist(T);
return
TRUE;
}

/* Check for components */
if (cdata.var_zeros[cdata.best] <
CUBELISTSIZE(T)
/2) {
if (
cubelist_partition(T,
&A, &B, debug & REDUCE1) == 0) {
return
MAYBE;
} else {
free_cubelist(T);
*
result = nc_sccc(A, level + 1);
ceil   = nc_sccc(B, level + 1);
(void)
set_and(*result, *result, ceil
);
set_free(ceil);
return
TRUE;
}
}

/* Not much we can do about it */
return
      MAYBE;
}

/*
 * ncp_expand -- similar to nc_expand but uses ncp_expand1 instead of
 * expand1
 */
pcover
ncp_expand(F, Fold, nonsparse)

INOUT pcover
      F;
IN    pcover
      Fold;
IN    bool
      nonsparse;              /* expand non-sparse variables only */

{
register pcube last, p;

register pcube
        RAISE             = nc_tmp_cube[0],
        FREESET           = nc_tmp_cube[1],
        INIT_LOWER        = nc_tmp_cube[2],
        SUPER_CUBE        = nc_tmp_cube[3],
        OVEREXPANDED_CUBE = nc_tmp_cube[4];

int  var, num_covered;
int  index;
bool change;

/* Order the cubes according to "chewing-away from the edges" of mini */
if (use_random_order) {
if (Fold != (pcover) NULL)
F = nc_random_order(F, Fold);
else
F = random_order(F);
}
else {
if (Fold != (pcover) NULL)
F = nc_mini_sort(F, &Fold, ascend);
else
F = mini_sort(F, ascend);
}

/* Setup the initial lowering set (differs only for nonsparse) */
if (nonsparse)
for (
var = 0;
var < cube.
num_vars;
var++)
if (cube.sparse[var])
(void)
set_or(INIT_LOWER, INIT_LOWER, cube
.var_mask[var]);

/* Mark all cubes as not covered, and maybe essential */
foreach_set(F, last, p
) {
RESET(p, COVERED
);
RESET(p, NONESSEN
);
}

/* Try to expand each nonprime and noncovered cube */
foreachi_set(F, index, p
) {
/* do not expand if PRIME or if covered by previous expansion */
if (!
TESTP(p, PRIME
) && !
TESTP(p, COVERED
)) {

/* find the reduced offset for p */

ROS = get_ROS(index, F, Fold);

/* expand the cube p, result is RAISE */
ncp_expand1(ROS, F, RAISE, FREESET, OVEREXPANDED_CUBE, SUPER_CUBE,
                 INIT_LOWER,
&num_covered, p);

if (debug & EXPAND)
(void) printf("EXPAND: %s (covered %d)\n",
pc1(p), num_covered
);

(void)
set_copy(p, RAISE
);

SET(p, PRIME
);
RESET(p, COVERED
);        /* not really necessary */

/* See if we generated an inessential prime */
if (num_covered == 0 && !
setp_equal(p, OVEREXPANDED_CUBE
)) {
SET(p, NONESSEN
);
}

}
}

/* Delete any cubes of F which became covered during the expansion */
F->
active_count = 0;
change       = FALSE;
foreach_set(F, last, p
) {
if (
TESTP(p, COVERED
)) {
RESET(p, ACTIVE
);
change = TRUE;
} else {
SET(p, ACTIVE
);
F->active_count++;
}
}
if (change)
F = sf_inactive(F);

if (Fold != (pcover) NULL)
sf_free(Fold);

return
      F;
}

/*
 *  ncp_expand1 -- Similar to expand1 except that when there are no
 *  more feasible cubes, it goes to mincov even if there are some
 *  yet uncovered cubes left in the cover.
 *
 */

void
ncp_expand1(BB, CC, RAISE, FREESET, OVEREXPANDED_CUBE, SUPER_CUBE,
            INIT_LOWER, num_covered, c)
        pcover BB;            /* Blocking matrix (OFF-set) */
        pcover CC;            /* Covering matrix (ON-set) */
        pcube RAISE;            /* The current parts which have been raised */
        pcube FREESET;            /* The current parts which are free */
        pcube OVEREXPANDED_CUBE;    /* Overexpanded cube of c */
        pcube SUPER_CUBE;        /* Supercube of all cubes of CC we cover */
        pcube INIT_LOWER;        /* Parts to initially remove from FREESET */
        int *num_covered;        /* Number of cubes of CC which are covered */
        pcube c;            /* The cube to be expanded */

{

    if (debug & EXPAND1)
        (void) printf("\nEXPAND1:    \t%s\n", pc1(c));

    /* initialize BB and CC */
    SET(c, PRIME);        /* don't try to cover ourself */
    setup_BB_CC(BB, CC);

    /* initialize count of # cubes covered, and the supercube of them */
    *num_covered = 0;
    (void) set_copy(SUPER_CUBE, c);

    /* Initialize the lowering, raising and unassigned sets */
    (void) set_copy(RAISE, c);
    (void) set_diff(FREESET, cube.fullset, RAISE);

    /* If some parts are forced into lowering set, remove them */
    if (!setp_empty(INIT_LOWER)) {
        (void) set_diff(FREESET, FREESET, INIT_LOWER);
        elim_lowering(BB, CC, RAISE, FREESET);
    }

    /* Determine what can be raised, and return the over-expanded cube */
    essen_parts(BB, CC, RAISE, FREESET);
    (void) set_or(OVEREXPANDED_CUBE, RAISE, FREESET);

    /* While there are still cubes which can be covered, cover them ! */
    if (CC->active_count > 0) {
        select_feasible(BB, CC, RAISE, FREESET, SUPER_CUBE, num_covered);
    }

    /* Finally, when all else fails, choose the largest possible prime */
    /* We will loop only if we decide unravelling OFF-set is too expensive */
    while (BB->active_count > 0) {
        mincov(BB, RAISE, FREESET);
    }

    /* Raise any remaining free coordinates */
    (void) set_or(RAISE, RAISE, FREESET);
}

/*
 *  nc_first_expand -- expand each nonprime cube of F into a prime implicant
 *  
 *  This subroutine differs from expand in that it is not passed on
 *  the global off set. For each cube to be expanded, this subroutine 
 *  computes a subset of off set which is sufficient for the expansion 
 *  of the cube.
 * 
 *  The subset produced is very small is limited by the number of
 *  literal in the cube to be expanded. This is an attempt to handle
 *  functions with very large off sets.
 *
 *  If nonsparse is true, only the non-sparse variables will be expanded;
 *  this is done by forcing all of the sparse variables out of the free set.
 */

pcover
nc_first_expand(F, nonsparse)

INOUT pcover
      F;
IN    bool
      nonsparse;              /* expand non-sparse variables only */

{
register pcube last, p;

register pcube
        RAISE             = nc_tmp_cube[0],
        FREESET           = nc_tmp_cube[1],
        INIT_LOWER        = nc_tmp_cube[2],
        SUPER_CUBE        = nc_tmp_cube[3],
        OVEREXPANDED_CUBE = nc_tmp_cube[4],
        overexpanded_cube = nc_tmp_cube[5],
        init_lower        = nc_tmp_cube[6];

pcube q, qlast;
int   var, num_covered;
int   index;
bool  change;

/* Order the cubes according to "chewing-away from the edges" of mini */
if (use_random_order)
F = random_order(F);
else
F = mini_sort(F, ascend);

/* Setup the initial lowering set (differs only for nonsparse) */
if (nonsparse)
for (
var = 0;
var < cube.
num_vars;
var++)
if (cube.sparse[var])
(void)
set_or(INIT_LOWER, INIT_LOWER, cube
.var_mask[var]);

/* Mark all cubes as not covered, and maybe essential */
foreach_set(F, last, p
) {
RESET(p, COVERED
);
RESET(p, NONESSEN
);
}

/* Try to expand each nonprime and noncovered cube */
foreachi_set(F, index, p
) {

if (!
TESTP(p, PRIME
) && !
TESTP(p, COVERED
)) {

/* find the overexpanded cube of p */
get_OC(p, overexpanded_cube, (pcube)
NULL);

/* If the overexpanded_cube is the same as p then p cannot
   be expanded */
if (
setp_equal(p, overexpanded_cube
)) {
foreach_set(F, qlast, q
)
if (
setp_implies(q, p
))
SET(q, COVERED
);
SET(p, PRIME
);
RESET(p, COVERED
);

continue;
}

/* Remove from overexpanded_cube any parts in INIT_LOWER */
if (nonsparse) {
(void)
set_diff(init_lower, INIT_LOWER, p
);
(void)
set_diff(overexpanded_cube, overexpanded_cube, init_lower
);
}

get_ROS1(p, overexpanded_cube, root
);

/* If the blocking matrix is empty then p can be expanded to
   its overexpanded cube */
if (ROS->count == 0) {
(void)
set_copy(p, overexpanded_cube
);
foreach_set(F, qlast, q
)
if (
setp_implies(q, p
))
SET(q, COVERED
);
SET(p, PRIME
);
RESET(p, COVERED
);

continue;
}

(void)
set_diff(init_lower, cube
.fullset, overexpanded_cube);

/* expand the cube p, result is RAISE */
expand1(ROS, F, RAISE, FREESET, OVEREXPANDED_CUBE, SUPER_CUBE,
             init_lower,
&num_covered, p);

if (debug & EXPAND)
(void) printf("EXPAND: %s (covered %d)\n",
pc1(p), num_covered
);

(void)
set_copy(p, RAISE
);

SET(p, PRIME
);
RESET(p, COVERED
);        /* not really necessary */

/* See if we generated an inessential prime */
if (num_covered == 0 && !
setp_equal(p, OVEREXPANDED_CUBE
)) {
SET(p, NONESSEN
);
}

}
}

/* Delete any cubes of F which became covered during the expansion */
F->
active_count = 0;
change       = FALSE;
foreach_set(F, last, p
) {
if (
TESTP(p, COVERED
)) {
RESET(p, ACTIVE
);
change = TRUE;
} else {
SET(p, ACTIVE
);
F->active_count++;
}
}
if (change)
F = sf_inactive(F);

return
F;
}

/*
 * Remove the cubes from T that wont affect the cube p being reduced.
 * These are the cubes that have atleast two variables in which T is
 * unate.
 */
void
rem_unnec_r_cubes(T)
        pcube *T;

{

    pcube
    temp, temp1;
    pcube
    q, *T1;
    pcube * Tc, *Topen;
    int i;
    int temp_int, last;

    /* If the cubelist is empty or has only one cube, don't do anything */
    if ((T[2] == NULL) || (T[3] == NULL))
        return;

    temp  = new_cube();
    temp1 = new_cube();

    (void) set_copy(temp, cube.fullset);

    /* "And" all the cubes together. */
    for (T1 = T + 2; (q = *T1++) != NULL;)
        (void) set_and(temp, temp, q);

    (void) set_or(temp, temp, T[0]);

    /* Find the non-unate and inactive binary variables. Set their values
       to all 1s in temp1. Set the unate binary variables to their complements.
       Set all mvs to all 1s */

    last = cube.last_word[cube.num_binary_vars - 1];
    for (i = 1; i <= last; i++) {
        temp_int = (temp[i] & (temp[i] >> 1)) | (~temp[i] & ((~temp[i]) >> 1));
        temp_int &= DISJOINT;
        temp1[i] = ~temp[i] | temp_int | (temp_int << 1);
    }
    (void) set_and(temp1, temp1, cube.fullset); /* Some problem with set_ord if I
					    don't do this */
    (void) set_or(temp1, temp1, cube.mv_mask);

    /* If temp1 has less than two zeros then no cubes will be removed so
       we might as well leave. */
    if (cube.size - set_ord(temp1) < 2) {
        free_cube(temp);
        free_cube(temp1);
        return;
    }

    /* Remove each cube q from the cubelist which has more than two 
       unate variables in it. All such cubes are distance two or more
       from temp1 */

    for (Tc = T + 2; (q = *Tc++) != NULL;) {
        if (cdist01(temp1, q) >= 2)
            break;
    }

    /* If there is no such cube in the cube list, return */
    if (q == NULL) {
        free_cube(temp);
        free_cube(temp1);
        return;
    }

    Topen = Tc - 1;

    while ((q = *Tc++) != NULL) {
        if (cdist01(temp1, q) < 2)
            *Topen++ = q;
    }

    *Topen++ = (pcube)
    NULL;
    T[1] = (pcube)
    Topen;

    free_cube(temp);
    free_cube(temp1);

}
