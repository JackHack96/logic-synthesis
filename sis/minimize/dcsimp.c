
#include <stdio.h>
#include "espresso.h"

pcover dcsimp();

static bool dc_special_cases();

pcover myexpand();

int mydescend();

void dc_simplify();

bool is_orthagonal();

pcover expand_to_largest();

extern void init_ROS();

extern void close_ROS();

extern pcover get_ROS();

pcover ROS;

int DC_DEBUG = 1; /* if set to 1, verifies the final cover for correctness*/

/* a two-level logic minimizer for multi-level logic synthesis. */
/* tries to minimize the number of literals in the on-set cover. */

pcover dcsimp(F, Old_D)

        pcover F;        /* cover for the onset*/
        pcover Old_D;    /* cover for the don't care set*/

{

    pcover D;
    pcover T;
    pcube  p;
    pcube  last;
    pcover FF;
    pcover E;
    int    error;
    int    lit_dis;
    cost_t cost;

    FF = sf_save(F);

    D = sf_save(Old_D);


    /* Initialize data structures for reduced offset */
    init_ROS(FF, D, 1, 3, cube.size);


/* check if any of the cubes in the on-set is essential? 
*  most of the time cubes in the original on-set are primes. therefore,
*  it pays off to find essential ones and avoid extra work during reduction
*  and expansion.
*/

    foreach_set(FF, last, p)
    {
        SET(p, RELESSEN);
        RESET(p, NONESSEN);
    }
    EXECUTE(E = essential(&FF, &D), ESSEN_TIME, E, cost);

/*
		if (essen_cube(FF,D,p)){
			E= sf_addset(E,p);
			RESET(p, ACTIVE);
			FF->active_count-- ;
		}else{
			SET(p, ACTIVE);
		}
	}
	FF= sf_inactive(FF);
	D= sf_join(D,E);
*/

/*  reduce cubes in the onset as much as possible. T is the new cover. */

    dc_simplify(cube1list(FF), cube1list(D), &T);


    if (T->count > 0) {
        ROS     = get_ROS(0, T, (pcover)NULL);


/* if the number of reduced cubes is large try to minimize the number of
*  cubes in the onset in two steps.
*/
        if (T->count > 20) {
            lit_dis = (cube.num_binary_vars + 1) / 3;

            T = myexpand(T, lit_dis);
            T = reduce(T, D);
        }
        lit_dis = cube.num_binary_vars;

        if (T->count > 1)
            T = myexpand(T, lit_dis);
        T = expand_to_largest(T, ROS);

    }


    T = sf_append(T, E);


/* do a cover verification*/
    if (DC_DEBUG) {
        error = verify(T, F, D);
        if (error) {
            print_solution = FALSE;
            sf_free(T);
            T = sf_save(F);
        }
    }
    free_cover(F);
    sf_free(FF);
    sf_free(D);
    close_ROS();
    return (T);
}

pcover myexpand(LKset, lit_dis)
/* a greedy algorithm to reduce the number of cubes in the cover.
* sort all the cubes in the reduced on-set.
* make supercube of every two cube and check if the supercube is in
*  F U D.
*/
        pcover LKset;    /* reduced on-set cover */
        int lit_dis;    /* prameter to decide which supercubes to generate*/
{
    pcover T3;
    pcover T;
    pcube  p, mp, por;
    pcube  pp;
    pcube  last1, last2;
    pcube  *L;

    T = new_cover(10);
/* sort all the cubes in the reduced onset so that cube with maximum number
*  of 2's is on top.
*/
    qsort((char *) (L = sf_list(LKset)), LKset->count, sizeof(pset), mydescend);
    T3 = sf_unlist(L, LKset->count, LKset->sf_size);

    foreach_set(T3, last1, p)
    {
        SET(p, ACTIVE);
    }

    pp = set_save(T3->data);
    por = new_cube();
    foreach_active_set(T3, last1, p)
    {
        RESET(p, ACTIVE);
        INLINEset_copy(por, p);
        INLINEset_copy(pp, p);

        foreach_active_set(T3, last2, mp)
        {
/* see how many literals need to be raised in order to make the supercube
*  if the number of literals is less than limit then do orhagonality test.
*/
            if (literals_to_raise(pp, mp) < lit_dis) {
                INLINEset_or(por, por, mp);
                if (is_orthagonal(por, ROS)) {
                    INLINEset_copy(p, por);
                    RESET(mp, ACTIVE);
                } else {
                    INLINEset_copy(por, p);
                }
            }
        }
        T = sf_addset(T, por);
    }
    set_free(por);
    set_free(pp);
    sf_free(LKset);
    sf_free(T3);
    return (T);
}


int literals_to_raise(p, q)
        pcube p;
        pcube q;

{

    int count = 0;
    int i;
    int lit;

    for (i = 0; i < (cube.num_binary_vars); i++) {
        if ((lit = GETINPUT(p, i)) != TWO && GETINPUT(q, i) != lit) count++;
    }
    return (count);
}


int mydescend(a, b)
        pset *a, *b;
{
    register pset a1 = *a, b1 = *b;
    if (cube_order(a1) < cube_order(b1)) return 1;
    else if (cube_order(a1) > cube_order(b1)) return -1;
    else return 0;
}


/*split until all the cubes in each leaf are unate then reduce each
* one of the cubes and store all the cubes in one pset-family.
*/

void dc_simplify(F, D, LKset)
        pcube *F;
        pcube *D;
        pcover *LKset;
{
    register pcube cl, cr;
    register pcube last, p;
    register int   best;
    pcover         LKsetr, LKsetl;


    if (dc_special_cases(F, D, LKset) == MAYBE) {

        /* Allocate space for the partition cubes */
        cl   = new_cube();
        cr   = new_cube();
        best = binate_split_select(F, cl, cr, COMPL);

        /* Complement the left and right halves */
        dc_simplify(scofactor(F, cl, best), scofactor(D, cl, best), &LKsetl);

        dc_simplify(scofactor(F, cr, best), scofactor(D, cr, best), &LKsetr);

        foreach_set(LKsetl, last, p)
        {
            INLINEset_and(p, p, cl);
        }
        foreach_set(LKsetr, last, p)
        {
            INLINEset_and(p, p, cr);
        }

        *LKset = sf_append(LKsetr, LKsetl);
        free_cube(cl);
        free_cube(cr);
        free_cubelist(F);
        free_cubelist(D);
    }
}

static bool dc_special_cases(F, D, FD)
        pcube *F;
        pcube *D;
        pcover *FD;
{
    pcover A, B;

    /* Check for no cubes in the cover (function is empty) */

    if (F[2] == NULL) {
        *FD = new_cover(0);
        free_cubelist(F);
        free_cubelist(D);
        return TRUE;
    }

    /* Collect column counts, determine unate variables, etc. */
    massive_count(F);


    /* Check for unate cover */
    if (cdata.vars_unate == cdata.vars_active) {
        /* Make the cover minimum by single-cube containment */
        A = cubeunlist(F);
        A = sf_contain(A);
        if (D[2] != NULL) {
            B = cubeunlist(D);
            *FD = reduce(A, B);
            sf_free(B);
        } else {
            *FD = A;
        }
        free_cubelist(F);
        free_cubelist(D);
        return TRUE;

        /* Not much we can do about it */
    } else {
        return MAYBE;
    }
}



/*count the number of 2's in a cube.*/
int cube_order(p)
        pcube p;
{
    int i, j;
    for (j = 0, i = 0; j < (cube.num_binary_vars); j++)
        if (GETINPUT(p, j) == TWO) i++;
    return (i);

}

/*
 * Return TRUE if p is orthagonal to each cube in A; otherwise,
 * return FALSE.
 */

bool is_orthagonal(p, A)
        pcube p;
        pcover A;

{

    pcube last, q;

    foreach_set(A, last, q)
    {
        if (cdist0(p, q))
            return (FALSE);
    }

    return (TRUE);

}

/* 
 * expand_to_largest -- expand each cube p in F to a larget prime
 * containing p. ros should be useful for all the cubes in F.
 */

pcover
expand_to_largest(F, ros)
        pcover F;
        pcover ros;

{

    pcube RAISE, FREESET;
    pcube p, q, last;
    int   i;

    RAISE   = new_cube();
    FREESET = new_cube();

    /* Find the larget prime of each cube in F */
    foreachi_set(F, i, p)
    {

        /* Set all cubes in ros active */
        ros->active_count = ros->count;
        foreach_set(ros, last, q)
        {
            SET(q, ACTIVE);
        }

        /* Initialize the lowering, raising and unassigned sets */
        (void) set_copy(RAISE, p);
        (void) set_diff(FREESET, cube.fullset, RAISE);

        /* Remove essentially lowered and essentially raised parts */
        (void) essen_parts(ros, /*CC*/ (pcover)NULL, RAISE, FREESET);

        /* Finally, choose the largest possible prime. We will loop only
             if we decide unravelling OFF-set is too expensive */
        while (ros->active_count > 0) {
            mincov(ros, RAISE, FREESET);
        }

        /* Raise any remaining free coordinates */
        (void) set_or(RAISE, RAISE, FREESET);

        /* Copy RAISE into p */
        (void) set_copy(p, RAISE);
        SET(p, PRIME);
        RESET(p, COVERED);

    }

    free_cube(RAISE);
    free_cube(FREESET);

    return (F);

}
